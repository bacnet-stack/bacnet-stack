/**
 * @file
 * @brief command line tool that sends a BACnet BVLC message, and displays
 * the reply, for a Read-Broadcast-Distribution-Table message to a peer BBMD.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
#include <ctype.h> /* for toupper */
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
/* some demo stuff needed */
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlenv.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* targets interpreted from the command line options */
static BACNET_IP_ADDRESS Target_BBMD_Address;

static bool Error_Detected = false;

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    (void)server;
    printf("BACnet Abort: %s\r\n", bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    printf("BACnet Reject: %s\r\n", bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    /* handle the reply (request) coming back */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_add);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    time_t total_seconds = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    long port = 0;

    if (argc < 2) {
        printf("Usage: %s IP [port]\r\n", filename_remove_path(argv[0]));
        return 0;
    }
    if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
        printf(
            "Send a Read-Broadcast-Distribution-Table message to a BBMD.\r\n"
            "\r\n"
            "IP:\r\n"
            "IP address of the BBMD in dotted decimal notation\r\n"
            "[port]\r\n"
            "optional BACnet/IP port number (default=47808=0xBAC0)\r\n"
            "\r\n"
            "To send a Read-Broadcast-Distribution-Table message to a BBMD\r\n"
            "at 192.168.0.1 using port 47808:\r\n"
            "%s 192.168.0.1 47808\r\n",
            filename_remove_path(argv[0]));
        return 0;
    }
    /* decode the command line parameters */
    if (argc > 1) {
        if (!bip_get_addr_by_name(argv[1], &Target_BBMD_Address)) {
            fprintf(stderr, "IP=%s - failed to convert address.\r\n", argv[1]);
            return 1;
        }
    }
    if (argc > 2) {
        port = strtol(argv[2], NULL, 0);
        if ((port > 0) && (port <= 65535)) {
            Target_BBMD_Address.port = (uint16_t)port;
        } else {
            fprintf(
                stderr, "port=%ld - port must be between 0-65535.\r\n", port);
            return 1;
        }
    } else {
        Target_BBMD_Address.port = 0xBAC0U;
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    address_init();
    dlenv_init();
    atexit(datalink_cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = apdu_timeout() / 1000;
    /* send the request */
    bvlc_bbmd_read_bdt(&Target_BBMD_Address);
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
        if (Error_Detected) {
            break;
        }
        /* increment timer - exit if timed out */
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
            datalink_maintenance_timer(elapsed_seconds);
        }
        total_seconds += elapsed_seconds;
        if (total_seconds > timeout_seconds) {
            break;
        }
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }

    return 0;
}
