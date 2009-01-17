/**************************************************************************
*
* Copyright (C) 2008 Steve Karg <skarg@users.sourceforge.net>
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
#include "address.h"
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#if defined(BACDL_MSTP)
#include "rs485.h"
#endif

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
#define MAX_ROUTER_DNETS 64
static int Target_Router_Networks[MAX_ROUTER_DNETS] = { -1 };

static bool Error_Detected = false;

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
    printf("BACnet Abort: %s\r\n", bactext_abort_reason_name(abort_reason));
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
    printf("BACnet Reject: %s\r\n", bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

static void Init_Service_Handlers(
    void)
{
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle the reply (request) coming back */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_add);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void Init_DataLink(
    void)
{
    char *pEnv = NULL;
#if defined(BACDL_BIP) && BBMD_ENABLED
    long bbmd_port = 0xBAC0;
    long bbmd_address = 0;
    long bbmd_timetolive_seconds = 60000;
#endif

#if defined(BACDL_ALL)
    pEnv = getenv("BACNET_DATALINK");
    if (pEnv) {
        datalink_set(pEnv));
    } else {
        datalink_set(NULL);
    }
#endif

#if defined(BACDL_BIP)
    pEnv = getenv("BACNET_IP_PORT");
    if (pEnv) {
        bip_set_port(strtol(pEnv, NULL, 0));
    } else {
        bip_set_port(0xBAC0);
    }
#elif defined(BACDL_MSTP)
    pEnv = getenv("BACNET_MAX_INFO_FRAMES");
    if (pEnv) {
        dlmstp_set_max_info_frames(strtol(pEnv, NULL, 0));
    } else {
        dlmstp_set_max_info_frames(1);
    }
    pEnv = getenv("BACNET_MAX_MASTER");
    if (pEnv) {
        dlmstp_set_max_master(strtol(pEnv, NULL, 0));
    } else {
        dlmstp_set_max_master(127);
    }
    pEnv = getenv("BACNET_MSTP_BAUD");
    if (pEnv) {
        RS485_Set_Baud_Rate(strtol(pEnv, NULL, 0));
    } else {
        RS485_Set_Baud_Rate(38400);
    }
    pEnv = getenv("BACNET_MSTP_MAC");
    if (pEnv) {
        dlmstp_set_mac_address(strtol(pEnv, NULL, 0));
    } else {
        dlmstp_set_mac_address(127);
    }
#endif
    if (!datalink_init(getenv("BACNET_IFACE"))) {
        exit(1);
    }
#if defined(BACDL_BIP) && BBMD_ENABLED
    pEnv = getenv("BACNET_BBMD_PORT");
    if (pEnv) {
        bbmd_port = strtol(pEnv, NULL, 0);
        if (bbmd_port > 0xFFFF) {
            bbmd_port = 0xBAC0;
        }
    }
    pEnv = getenv("BACNET_BBMD_TIMETOLIVE");
    if (pEnv) {
        bbmd_timetolive_seconds = strtol(pEnv, NULL, 0);
        if (bbmd_timetolive_seconds > 0xFFFF) {
            bbmd_timetolive_seconds = 0xFFFF;
        }
    }
    pEnv = getenv("BACNET_BBMD_ADDRESS");
    if (pEnv) {
        bbmd_address = bip_getaddrbyname(pEnv);
        if (bbmd_address) {
            struct in_addr addr;
            addr.s_addr = bbmd_address;
            printf("WhoIs: Registering with BBMD at %s:%ld for %ld seconds\n",
                inet_ntoa(addr), bbmd_port, bbmd_timetolive_seconds);
            bvlc_register_with_bbmd(bbmd_address, bbmd_port,
                bbmd_timetolive_seconds);
        }
    }
#endif
}

int main(int argc, char *argv[]) {
    unsigned arg_count = 0;

    if       (argc < 2) {
        printf("Usage: %s DNET [DNET] [DNET] [...]\r\n",
            filename_remove_path(argv[0]));
        return 0;
    }
    if   ((
            argc > 1) && (
            strcmp(argv[1],
                "--help") == 0)) {
        printf("Send BACnet I-Am-Router-To-Network message for \r\n"
            "one or more networks.\r\n" "\r\nDNET:\r\n"
            "BACnet destination network number 0-65534\r\n"
            "To send a I-Am-Router-To-Network message for DNET 86:\r\n"
            "%s 86\r\n"
            "To send a I-Am-Router-To-Network message for multiple DNETs\r\n"
            "use the following command:\r\n" "%s 86 42 24 14\r\n",
            filename_remove_path(argv[0]), filename_remove_path(argv[0]));
        return 0;
    }
    /* decode the command line parameters */
    if (argc > 1) {
        for (arg_count = 1; arg_count < argc; arg_count++) {
            if (arg_count > MAX_ROUTER_DNETS) {
                fprintf(stderr, "Limited to %u DNETS.  Sorry!\r\n",
                    MAX_ROUTER_DNETS);
                break;
            }
            Target_Router_Networks[arg_count - 1] =
                strtol(argv[arg_count], NULL, 0);
            /* mark the end of list */
            Target_Router_Networks[arg_count] = -1;
            /* invalid DNET? */
            if (Target_Router_Networks[arg_count - 1] >= 65535) {
                fprintf(stderr, "DNET=%u - it must be less than %u\r\n",
                    Target_Router_Networks[arg_count - 1], 65535);
                return 1;
            }
        }
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    address_init();
    Init_DataLink();
    /* send the request */
    Send_I_Am_Router_To_Network(Target_Router_Networks);

    return 0;
}
