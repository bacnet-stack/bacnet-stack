/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

/* command line tool that sends a BACnet service, and displays the reply */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>               /* for time */
#include <errno.h>
#include "bactext.h"
#include "iam.h"
#include "arf.h"
#include "tsm.h"
#include "address.h"
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "net.h"
#include "datalink.h"
#include "whois.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static bool Error_Detected = false;
static BACNET_ADDRESS Target_Address;

static void MyErrorHandler(BACNET_ADDRESS * src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class, BACNET_ERROR_CODE error_code)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    /* Ignore errors for EPICS application */
    /*
    printf("BACnet Error: %s: %s\r\n",
        bactext_error_class_name(error_class),
        bactext_error_code_name(error_code));
    */
    Error_Detected = true;
}

void MyAbortHandler(BACNET_ADDRESS * src,
    uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    (void) server;
    /* Ignore errors for EPICS application */
    /*
    printf("BACnet Abort: %s\r\n",
        bactext_abort_reason_name(abort_reason));
    */
    Error_Detected = true;
}

void MyRejectHandler(BACNET_ADDRESS * src,
    uint8_t invoke_id, uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    /* Ignore errors for EPICS application */
    /*
    printf("BACnet Reject: %s\r\n",
        bactext_reject_reason_name(reject_reason));
    */
    Error_Detected = true;
}

static void Init_Service_Handlers(void)
{
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM,
        handler_i_am_bind);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property_ack);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static uint8_t Read_Properties(uint32_t device_instance)
{
    uint8_t invoke_id = 0;
    static unsigned index = 0;
    /* list of required (and some optional) properties in the
       Device Object
       note: you could just loop through
       all the properties in all the objects. */
    const int object_props[] = {
        PROP_OBJECT_IDENTIFIER,
        PROP_OBJECT_NAME,
        PROP_OBJECT_TYPE,
        PROP_SYSTEM_STATUS,
        PROP_VENDOR_NAME,
        PROP_VENDOR_IDENTIFIER,
        PROP_MODEL_NAME,
        PROP_FIRMWARE_REVISION,
        PROP_APPLICATION_SOFTWARE_VERSION,
        PROP_PROTOCOL_VERSION,
        PROP_PROTOCOL_CONFORMANCE_CLASS,
        PROP_PROTOCOL_SERVICES_SUPPORTED,
        PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
        PROP_MAX_APDU_LENGTH_ACCEPTED,
        PROP_SEGMENTATION_SUPPORTED,
        PROP_LOCAL_TIME,
        PROP_LOCAL_DATE,
        PROP_UTC_OFFSET,
        PROP_DAYLIGHT_SAVINGS_STATUS,
        PROP_APDU_SEGMENT_TIMEOUT,
        PROP_APDU_TIMEOUT,
        PROP_NUMBER_OF_APDU_RETRIES,
        PROP_TIME_SYNCHRONIZATION_RECIPIENTS,
        PROP_MAX_MASTER,
        PROP_MAX_INFO_FRAMES,
        PROP_DEVICE_ADDRESS_BINDING,
        /* note: PROP_OBJECT_LIST is missing cause
           we need to get it with an index method since
           the list could be very large */
        /* some proprietary properties */
        514, 515,
        /* end of list */
        -1
    };
    
    if (object_props[index] != -1) {
        printf("%s: ",bactext_property_name(object_props[index]));
        invoke_id = Send_Read_Property_Request(device_instance,
            OBJECT_DEVICE,
            device_instance, 
            object_props[index], 
            BACNET_ARRAY_ALL);
        if (invoke_id) {
            index++;
        }
    }

    return invoke_id;
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    unsigned max_apdu = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    uint8_t invoke_id = 0;
    bool found = false;

    if (argc < 2) {
        printf
            ("%s device-instance\r\n",
            filename_remove_path(argv[0]));
        return 0;
    }
    /* decode the command line parameters */
    Target_Device_Object_Instance = strtol(argv[1], NULL, 0);
    if (Target_Device_Object_Instance >= BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance=%u - it must be less than %u\r\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
        return 1;
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    address_init();
    Init_Service_Handlers();
#if defined(BACDL_ETHERNET)
    /* init the physical layer */
    if (!ethernet_init("eth0"))
        return 1;
#elif defined(BACDL_BIP)
    bip_set_interface("eth0");
    if (!bip_init())
        return 1;
    printf("bip: using port %hu\r\n", bip_get_port());
#elif defined(BACDL_ARCNET)
    if (!arcnet_init("arc0"))
        return 1;
#else
#error Datalink (BACDL_ETHERNET,BACDL_BIP, or BACDL_ARCNET) undefined
#endif
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (Device_APDU_Timeout() / 1000) *
        Device_Number_Of_APDU_Retries();
    /* try to bind with the device */
    Send_WhoIs(Target_Device_Object_Instance,
        Target_Device_Object_Instance);
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);

        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        /* at least one second has passed */
        if (current_seconds != last_seconds)
            tsm_timer_milliseconds(((current_seconds -
                        last_seconds) * 1000));
        /* wait until the device is bound, or timeout and quit */
        found = address_bind_request(Target_Device_Object_Instance,
            &max_apdu, &Target_Address);
        if (found) {
            if (invoke_id == 0) {
                invoke_id =  Read_Properties(Target_Device_Object_Instance);
                if (invoke_id == 0) {
                    break;
                }
            } else if (tsm_invoke_id_free(invoke_id)) {
                invoke_id = 0;
            }
            else if (tsm_invoke_id_failed(invoke_id)) {
                fprintf(stderr, "\rError: TSM Timeout!\r\n");
                tsm_free_invoke_id(invoke_id);
                invoke_id = 0;
            } else if (Error_Detected) {
                invoke_id = 0;
            }
        } else {
            /* increment timer - exit if timed out */
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                printf("\rError: APDU Timeout!\r\n");
                break;
            }
        }
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }

    return 0;
}
