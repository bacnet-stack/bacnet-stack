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
#include <time.h>       /* for time */
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
#include "rp.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#include "keylist.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static bool Error_Detected = false;
static BACNET_ADDRESS Target_Address;

typedef struct BACnet_RP_Service_Data_t {
    bool new_data;
    BACNET_CONFIRMED_SERVICE_ACK_DATA service_data;
    BACNET_READ_PROPERTY_DATA data;
} BACNET_RP_SERVICE_DATA;
static BACNET_RP_SERVICE_DATA Read_Property_Data;

/* FIXME: keep the object list in here */
/* static OS_Keylist Object_List; */

static void MyErrorHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
#if 1
    printf("BACnet Error: %s: %s\r\n", bactext_error_class_name(error_class),
        bactext_error_code_name(error_code));
#else
    (void) error_class;
    (void) error_code;
#endif
    Error_Detected = true;
}

void MyAbortHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t abort_reason,
    bool server)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    (void) server;
#if 1
    printf("BACnet Abort: %s\r\n", bactext_abort_reason_name(abort_reason));
#else
    (void) abort_reason;
#endif
    Error_Detected = true;
}

void MyRejectHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
#if 1
    printf("BACnet Reject: %s\r\n", bactext_reject_reason_name(reject_reason));
#else
    (void) reject_reason;
#endif
    Error_Detected = true;
}

void PrintReadPropertyData(
    BACNET_READ_PROPERTY_DATA * data)
{
    BACNET_APPLICATION_DATA_VALUE value;        /* for decode value data */
    int len = 0;
    uint8_t *application_data;
    int application_data_len;
    bool first_value = true;
    bool print_brace = false;

    if (data) {
#if 0
        if (data->array_index == BACNET_ARRAY_ALL)
            fprintf(stderr, "%s #%u %s\n",
                bactext_object_type_name(data->object_type),
                data->object_instance,
                bactext_property_name(data->object_property));
        else
            fprintf(stderr, "%s #%u %s[%d]\n",
                bactext_object_type_name(data->object_type),
                data->object_instance,
                bactext_property_name(data->object_property),
                data->array_index);
#endif
        application_data = data->application_data;
        application_data_len = data->application_data_len;
        /* value? loop until all of the len is gone... */
        for (;;) {
            len =
                bacapp_decode_application_data(application_data,
                (uint8_t) application_data_len, &value);
            if (first_value && (len < application_data_len)) {
                first_value = false;
                fprintf(stdout, "{");
                print_brace = true;
            }
            bacapp_print_value(stdout, &value, data->object_property);
            if (len) {
                if (len < application_data_len) {
                    application_data += len;
                    application_data_len -= len;
                    /* there's more! */
                    fprintf(stdout, ",");
                } else
                    break;
            } else
                break;
        }
        if (print_brace)
            fprintf(stdout, "}");
        fprintf(stdout, "\r\n");
    }
}

void MyReadPropertyAckHandler(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    int len = 0;
    BACNET_READ_PROPERTY_DATA data;

    (void) src;
    len = rp_ack_decode_service_request(service_request, service_len, &data);
    if (len > 0) {
        memmove(&Read_Property_Data.service_data, service_data, sizeof(data));
        memmove(&Read_Property_Data.data, &data, sizeof(data));
        Read_Property_Data.new_data = true;
    }
}

static void Init_Service_Handlers(
    void)
{
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        MyReadPropertyAckHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static uint8_t Read_Properties(
    uint32_t device_instance)
{
    uint8_t invoke_id = 0;
    static unsigned index = 0;
    /* note: you could just loop through
       all the properties in all the objects. */
    static const int *pRequired = NULL;

    if (!pRequired) {
        Device_Property_Lists(&pRequired, NULL, NULL);
    }

    if (pRequired[index] != -1) {
        printf("    %s: ", bactext_property_name(pRequired[index]));
        invoke_id =
            Send_Read_Property_Request(device_instance, OBJECT_DEVICE,
            device_instance, pRequired[index], BACNET_ARRAY_ALL);
        if (invoke_id) {
            index++;
        }
    }

    return invoke_id;
}

int main(
    int argc,
    char *argv[])
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

    /* FIXME: handle multi homed systems - use an argument passed to the datalink_init() */

    /* print help if not enough arguments */
    if (argc < 2) {
        printf("%s device-instance\r\n", filename_remove_path(argv[0]));
        return 0;
    }

    /* decode the command line parameters */
    Target_Device_Object_Instance = strtol(argv[1], NULL, 0);
    if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance=%u - it must be less than %u\r\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE + 1);
        return 1;
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    address_init();
    Init_Service_Handlers();
    if (!datalink_init(getenv("BACNET_IFACE")))
        return 1;
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds =
        (Device_APDU_Timeout() / 1000) * Device_Number_Of_APDU_Retries();
    /* try to bind with the device */
    Send_WhoIs(Target_Device_Object_Instance, Target_Device_Object_Instance);
    printf("List of Objects in test device:\r\n");
    printf("{\r\n");
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
        if (current_seconds != last_seconds) {
            tsm_timer_milliseconds(((current_seconds - last_seconds) * 1000));
        }
        /* wait until the device is bound, or timeout and quit */
        found =
            address_bind_request(Target_Device_Object_Instance, &max_apdu,
            &Target_Address);
        if (found) {
            /* invoke ID is set to zero when it is not in use */
            if (invoke_id == 0) {
                invoke_id = Read_Properties(Target_Device_Object_Instance);
                if (invoke_id == 0) {
                    break;
                }
            } else if ((Read_Property_Data.new_data) &&
                (invoke_id == Read_Property_Data.service_data.invoke_id)) {
                Read_Property_Data.new_data = false;
                PrintReadPropertyData(&Read_Property_Data.data);
                if (tsm_invoke_id_free(invoke_id)) {
                    invoke_id = 0;
                }
            } else if (tsm_invoke_id_free(invoke_id)) {
                invoke_id = 0;
            } else if (tsm_invoke_id_failed(invoke_id)) {
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
    printf("}\r\n");

    return 0;
}
