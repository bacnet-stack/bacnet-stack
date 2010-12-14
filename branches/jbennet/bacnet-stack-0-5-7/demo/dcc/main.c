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

/* command line tool demo for BACnet stack */
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
#include "dcc.h"
#include "bacnet-session.h"
#include "handlers-data.h"
#include "session.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#include "dlenv.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_ADDRESS Target_Address;
static uint16_t Communication_Timeout_Minutes = 0;
static BACNET_COMMUNICATION_ENABLE_DISABLE Communication_State =
    COMMUNICATION_ENABLE;
static BACNET_CHARACTER_STRING Communication_Password;
static bool Has_Communication_Password = false;

static bool Error_Detected = false;

static void MyErrorHandler(
    struct bacnet_session_object *sess,
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    printf("BACnet Error: %s: %s\r\n", bactext_error_class_name(error_class),
        bactext_error_code_name(error_code));
    Error_Detected = true;
}

void MyAbortHandler(
    struct bacnet_session_object *sess,
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t abort_reason,
    bool server)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    (void) server;
    printf("BACnet Abort: %s\r\n", bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

void MyRejectHandler(
    struct bacnet_session_object *sess,
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    printf("BACnet Reject: %s\r\n", bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

void MyDeviceCommunicationControlSimpleAckHandler(
    struct bacnet_session_object *sess,
    BACNET_ADDRESS * src,
    uint8_t invoke_id)
{
    (void) src;
    (void) invoke_id;
    printf("DeviceCommunicationControl Acknowledged!\r\n");
}

static void Init_Service_Handlers(
    struct bacnet_session_object *sess)
{
    Device_Init(sess);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(sess, SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(sess, SERVICE_UNCONFIRMED_I_AM,
        handler_i_am_bind);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(sess,
        handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(sess, SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(sess,
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* handle the ack coming back */
    apdu_set_confirmed_simple_ack_handler(sess,
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        MyDeviceCommunicationControlSimpleAckHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(sess,
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL, MyErrorHandler);
    apdu_set_abort_handler(sess, MyAbortHandler);
    apdu_set_reject_handler(sess, MyRejectHandler);
}

int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = {
        0
    };  /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    unsigned max_apdu = 0;
    uint8_t segmentation = 0;
    uint32_t maxsegments = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    uint8_t invoke_id = 0;
    bool found = false;
    struct bacnet_session_object *sess = NULL;

    if (argc < 3) {
        printf("Usage: %s device-instance state timeout [password]\r\n"
            "Send BACnet DeviceCommunicationControl service to device.\r\n"
            "\r\n" "The device-instance can be 0 to %d.\r\n"
            "Possible state values:\r\n" "  0=enable\r\n" "  1=disable\r\n"
            "  2=disable-initiation\r\n"
            "The timeout can be 0 for infinite, or a value in minutes for disable.\r\n"
            "The optional password is a character string of 1 to 20 characters.\r\n"
            "Use BACNET_IFACE environment variable for the interface\r\n",
            filename_remove_path(argv[0]), BACNET_MAX_INSTANCE - 1);
        return 0;
    }
    /* decode the command line parameters */
    Target_Device_Object_Instance = strtol(argv[1], NULL, 0);
    Communication_State = strtol(argv[2], NULL, 0);
    Communication_Timeout_Minutes = strtol(argv[3], NULL, 0);
    /* optional password */
    if (argc > 4) {
        characterstring_init(&Communication_Password, CHARACTER_ANSI_X34,
            argv[4], strlen(argv[4]));
        Has_Communication_Password = true;
    }

    if (Target_Device_Object_Instance >= BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance=%u - it must be less than %u\r\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
        return 1;
    }

    /* setup my info */
    sess = create_bacnet_session();
    Device_Set_Object_Instance_Number(sess, BACNET_MAX_INSTANCE);
    address_init(sess);
    Init_Service_Handlers(sess);
    dlenv_init(sess);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout(sess) / 1000) * apdu_retries(sess);
    /* try to bind with the device */
    Send_WhoIs(sess, Target_Device_Object_Instance,
        Target_Device_Object_Instance);
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);

        /* returns 0 bytes on timeout */
        pdu_len =
            sess->datalink_receive(sess, &src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(sess, &src, &Rx_Buf[0], pdu_len);
        }
        /* at least one second has passed */
        if (current_seconds != last_seconds)
            tsm_timer_milliseconds(sess,
                ((current_seconds - last_seconds) * 1000));
        if (Error_Detected)
            break;
        /* wait until the device is bound, or timeout and quit */
        found =
            address_bind_request(sess, Target_Device_Object_Instance,
            &max_apdu, &segmentation, &maxsegments, &Target_Address);
        if (found) {
            if (invoke_id == 0) {
                invoke_id =
                    Send_Device_Communication_Control_Request(sess, NULL,
                    Target_Device_Object_Instance,
                    Communication_Timeout_Minutes, Communication_State,
                    Has_Communication_Password ? &Communication_Password :
                    NULL);
            } else if (tsm_invoke_id_free(sess, invoke_id))
                break;
            else if (tsm_invoke_id_failed(sess, invoke_id)) {
                fprintf(stderr, "\rError: TSM Timeout!\r\n");
                tsm_free_invoke_id_check(sess, invoke_id, &Target_Address,
                    true);
                /* try again or abort? */
                break;
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
    /* perform memory desallocation */
    bacnet_destroy_session(sess);

    return 0;
}
