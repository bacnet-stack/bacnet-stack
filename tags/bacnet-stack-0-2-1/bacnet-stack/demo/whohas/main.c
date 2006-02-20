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
#include "address.h"
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "net.h"
#include "datalink.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static int32_t Target_Object_Instance_Min = 0;
static int32_t Target_Object_Instance_Max = BACNET_MAX_INSTANCE;

static bool Error_Detected = false;

void MyAbortHandler(BACNET_ADDRESS * src,
    uint8_t invoke_id, uint8_t abort_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    printf("BACnet Abort: %s\r\n",
        bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

void MyRejectHandler(BACNET_ADDRESS * src,
    uint8_t invoke_id, uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    printf("BACnet Reject: %s\r\n",
        bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

static void Init_Service_Handlers(void)
{
    /* we need to handle who-is 
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle the reply (request) coming back */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM,
        handler_i_am_add);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

#ifdef BIP_DEBUG
static void print_address(char *name, BACNET_ADDRESS * dest)
{                               /* destination address */
    int i = 0;                  /* counter */

    if (dest) {
        printf("%s: ", name);
        for (i = 0; i < dest->mac_len; i++) {
            printf("%02X", dest->mac[i]);
        }
        printf("\n");
    }
}
#endif

static void print_address_cache(void)
{
    int i, j;
    BACNET_ADDRESS address;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;

    fprintf(stderr, "Device\tMAC\tMaxAPDU\tNet\n");
    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        if (address_get_by_index(i, &device_id, &max_apdu, &address)) {
            fprintf(stderr, "%u\t", device_id);
            for (j = 0; j < address.mac_len; j++) {
                fprintf(stderr, "%02X", address.mac[j]);
            }
            fprintf(stderr, "\t");
            fprintf(stderr, "%hu\t", max_apdu);
            fprintf(stderr, "%hu\n", address.net);
        }
    }
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
#ifdef BIP_DEBUG
    BACNET_ADDRESS my_address, broadcast_address;
#endif

    if (argc < 2) {
        printf("Usage: %s device-instance | device-instance-min device-instance-max\r\n"
            "Send BACnet WhoIs request to devices, and wait for responses.\r\n"
            "\r\n"
            "The device-instance can be 0 to %d, or -1 for ALL.\r\n"
            "The device-instance can also be specified as a range.\r\n",
            filename_remove_path(argv[0]),
            BACNET_MAX_INSTANCE);
        return 0;
    }
    /* decode the command line parameters */
    if (argc < 3) {
        Target_Object_Instance_Min = strtol(argv[1], NULL, 0);
        Target_Object_Instance_Max = Target_Object_Instance_Min;
        if (Target_Object_Instance_Min > BACNET_MAX_INSTANCE) {
            fprintf(stderr,
                "object-instance-min=%u - it must be less than %u\r\n",
                Target_Object_Instance_Min, BACNET_MAX_INSTANCE + 1);
            return 1;
        }
    } else {
        Target_Object_Instance_Min = strtol(argv[1], NULL, 0);
        Target_Object_Instance_Max = strtol(argv[2], NULL, 0);
        if (Target_Object_Instance_Min > BACNET_MAX_INSTANCE) {
            fprintf(stderr,
                "object-instance-min=%u - it must be less than %u\r\n",
                Target_Object_Instance_Min, BACNET_MAX_INSTANCE + 1);
            return 1;
        }
        if (Target_Object_Instance_Max > BACNET_MAX_INSTANCE) {
            fprintf(stderr,
                "object-instance-max=%u - it must be less than %u\r\n",
                Target_Object_Instance_Max, BACNET_MAX_INSTANCE + 1);
            return 1;
        }
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    /* configure standard BACnet/IP port */
    bip_set_interface("eth0");  /* for linux */
    bip_set_port(0xBAC0);
    if (!bip_init())
        return 1;
#ifdef BIP_DEBUG
    datalink_get_broadcast_address(&broadcast_address);
    print_address("Broadcast", &broadcast_address);
    datalink_get_my_address(&my_address);
    print_address("Address", &my_address);
#endif
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = Device_APDU_Timeout() / 1000;
    /* send the request */
    Send_WhoIs(Target_Object_Instance_Min, Target_Object_Instance_Max);
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);
        /* returns 0 bytes on timeout */
        pdu_len = bip_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
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
    print_address_cache();

    return 0;
}
