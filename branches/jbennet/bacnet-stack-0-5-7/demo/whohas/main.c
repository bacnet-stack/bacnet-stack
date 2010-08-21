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
#include "whohas.h"
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
static BACNET_OBJECT_TYPE Target_Object_Type = MAX_BACNET_OBJECT_TYPE;
static uint32_t Target_Object_Instance = BACNET_MAX_INSTANCE;
static char *Target_Object_Name = NULL;

static bool Error_Detected = false;

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

static void Init_Service_Handlers(
    struct bacnet_session_object *sess)
{
    Device_Init(sess);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(sess, SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(sess,
        handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(sess, SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle the reply (request) coming back */
    apdu_set_unconfirmed_handler(sess, SERVICE_UNCONFIRMED_I_HAVE,
        handler_i_have);
    /* handle any errors coming back */
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
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    struct bacnet_session_object *sess = NULL;

    if (argc < 2) {
        /* note: priority 16 and 0 should produce the same end results... */
        printf("Usage: %s <object-type object-instance | object-name>\r\n"
            "Send BACnet WhoHas request to devices, and wait for responses.\r\n"
            "\r\n" "Use either:\r\n" "The object-type can be 0 to %d.\r\n"
            "The object-instance can be 0 to %d.\r\n" "or:\r\n"
            "The object-name can be any string of characters.\r\n",
            filename_remove_path(argv[0]), MAX_BACNET_OBJECT_TYPE - 1,
            BACNET_MAX_INSTANCE);
        return 0;
    }
    /* decode the command line parameters */
    if (argc < 3) {
        Target_Object_Name = argv[1];
    } else {
        Target_Object_Type = strtol(argv[1], NULL, 0);
        Target_Object_Instance = strtol(argv[2], NULL, 0);
        if (Target_Object_Instance > BACNET_MAX_INSTANCE) {
            fprintf(stderr, "object-instance=%u - it must be less than %u\r\n",
                Target_Object_Instance, BACNET_MAX_INSTANCE + 1);
            return 1;
        }
        if (Target_Object_Type > MAX_BACNET_OBJECT_TYPE) {
            fprintf(stderr, "object-type=%u - it must be less than %u\r\n",
                Target_Object_Type, MAX_BACNET_OBJECT_TYPE + 1);
            return 1;
        }
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(sess, BACNET_MAX_INSTANCE);
    Init_Service_Handlers(sess);
    dlenv_init(sess);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = apdu_timeout(sess) / 1000;
    /* send the request */
    if (argc < 3)
        Send_WhoHas_Name(sess, -1, -1, Target_Object_Name);
    else
        Send_WhoHas_Object(sess, -1, -1, Target_Object_Type,
            Target_Object_Instance);
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
        if (Error_Detected)
            break;
        /* increment timer - exit if timed out */
        elapsed_seconds += (current_seconds - last_seconds);
        if (elapsed_seconds > timeout_seconds)
            break;
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }
    /* perform memory desallocation */
    bacnet_destroy_session(sess);

    return 0;
}
