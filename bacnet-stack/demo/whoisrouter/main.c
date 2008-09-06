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
#include "debug.h"
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
static int32_t Target_Router_Network = 0;
static BACNET_ADDRESS Target_Router_Address;

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

void router_handler(
    BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t * npdu,      /* PDU data */
    uint16_t npdu_len)
{
    uint16_t npdu_offset = 0;
    uint16_t dnet = 0;
    uint16_t len = 0;
    uint16_t j = 0;

    switch (npdu_data->network_message_type) {
        case NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK:
            break;
        case NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK:
            printf("I-Am Router to Network from ");
            for (j = 0; j < MAX_MAC_LEN; j++) {
                if (j < src->mac_len) {
                    printf("%02X", src->mac[j]);
                }
            }
            printf("\nNetworks: ");
            while (npdu_len) {
                len = decode_unsigned16(&npdu[npdu_offset], &dnet);
                printf("%hu",dnet);
                npdu_len -= len;
                if (npdu_len) {
                    printf(", ");
                }
            }
            printf("\n");
            break;
        case NETWORK_MESSAGE_I_COULD_BE_ROUTER_TO_NETWORK:
        case NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK:
        case NETWORK_MESSAGE_ROUTER_BUSY_TO_NETWORK:
        case NETWORK_MESSAGE_ROUTER_AVAILABLE_TO_NETWORK:
        case NETWORK_MESSAGE_INIT_RT_TABLE:
        case NETWORK_MESSAGE_INIT_RT_TABLE_ACK:
        case NETWORK_MESSAGE_ESTABLISH_CONNECTION_TO_NETWORK:
        case NETWORK_MESSAGE_DISCONNECT_CONNECTION_TO_NETWORK:
        default:
            break;
    }
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

static void address_parse(BACNET_ADDRESS * dst,
    int argc, char *argv[]) {
    int dnet = 0;
    unsigned mac[6];
    int count = 0;
    int index = 0;

    if  (argc > 0) {
        count =
            sscanf(argv[0], "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2],
            &mac[3], &mac[4], &mac[5]);
        dst->mac_len = count;
        for (index = 0; index < MAX_MAC_LEN; index++) {
            if (index < count) {
                dst->mac[index] = mac[index];
            } else {
                dst->mac[index] = 0;
            }
        }
    }
    if (argc > 1) {
        count = sscanf(argv[1], "%d", &dnet);
        dst->net = dnet;
    }
    if (dnet) {
        if (argc > 2) {
            count =
                sscanf(argv[2], "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2],
                &mac[3], &mac[4], &mac[5]);
            dst->len = count;
            for (index = 0; index < MAX_MAC_LEN; index++) {
                if (index < count) {
                    dst->adr[index] = mac[index];
                } else {
                    dst->adr[index] = 0;
                }
            }
        } else {
            fprintf(stderr, "A non-zero DNET requires a DADR.\r\n");
        }
    } else {
        dst->len = 0;
        for (index = 0; index < MAX_MAC_LEN; index++) {
            dst->adr[index] = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    BACNET_ADDRESS src = {
    0}; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    time_t total_seconds = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;

    if (argc < 2) {
        printf("Usage: %s DNET [MAC]\r\n", filename_remove_path(argv[0]));
        return 0;
    }
    if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
        printf("Send BACnet Who-Is-Router-To-Network message to a network.\r\n"
            "\r\n" "DNET:\r\n" "BACnet destination network number 0-65534\r\n"
            "MAC:\r\n" "Optional MAC address of router for unicast message\r\n"
            "Format: xx[:xx:xx:xx:xx:xx] [dnet xx[:xx:xx:xx:xx:xx]]\r\n"
            "Use hexidecimal MAC addresses.\r\n" "\r\n"
            "To send a Who-Is-Router-To-Network request to DNET 86:\r\n"
            "%s 86\r\n"
            "To send a Who-Is-Router-To-Network request to all devices:\r\n"
            "%s -1\r\n", filename_remove_path(argv[0]),
            filename_remove_path(argv[0]));
        return 0;
    }
    /* decode the command line parameters */
    if (argc > 1) {
        Target_Router_Network = strtol(argv[1], NULL, 0);
        if (Target_Router_Network >= 65535) {
            fprintf(stderr, "DNET=%u - it must be less than %u\r\n",
                Target_Router_Network, 65535);
            return 1;
        }
    }
    if (argc > 2) {
        address_parse(&Target_Router_Address, argc - 2, &argv[2]);
    } else {
        datalink_get_broadcast_address(&Target_Router_Address);
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    address_init();
    Init_DataLink();
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = apdu_timeout() / 1000;
    /* send the request */
    Send_Who_Is_Router_To_Network(&Target_Router_Address,
        Target_Router_Network);
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
        if (Error_Detected)
            break;
        /* increment timer - exit if timed out */
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
#if defined(BACDL_BIP) && BBMD_ENABLED
            bvlc_maintenance_timer(elapsed_seconds);
#endif
        }
        total_seconds += elapsed_seconds;
        if (total_seconds > timeout_seconds)
            break;
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }

    return 0;
}
