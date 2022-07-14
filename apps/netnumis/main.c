/**
 * @file
 * @brief Application to send a BACnet Network Network-Number-Is message
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2022
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
#include <errno.h>
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
/* some demo stuff needed */
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* where the message gets sent */
static BACNET_ADDRESS Target_Router_Address;
static int32_t Target_Network_Number = 0;
static int32_t Target_Network_Number_Status = 0;

static bool Error_Detected = false;

/**
 * Handler for an Abort PDU.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 * @param abort_reason [in] the reason for the message abort
 * @param server
 */
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

/**
 * @brief Handler for a Reject PDU.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 * @param reject_reason [in] the reason for the rejection
 */
static void MyRejectHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    printf("BACnet Reject: %s\r\n", bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

/**
 * @brief Handler for a Network messages
 * @param npdu_data - NPDU options
 * @param npdu - NPDU data
 * @param npdu_len - NPDU data length
 */
static void My_Router_Handler(BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *npdu,
    uint16_t npdu_len)
{
    uint16_t dnet = 0;
    uint16_t len = 0;
    uint16_t j = 0;

    switch (npdu_data->network_message_type) {
        case NETWORK_MESSAGE_WHAT_IS_NETWORK_NUMBER:
            printf("What-Is-Network-Number from ");
            for (j = 0; j < MAX_MAC_LEN; j++) {
                if (j < src->mac_len) {
                    printf("%02X", src->mac[j]);
                }
            }
            printf("\n");
            break;
        case NETWORK_MESSAGE_NETWORK_NUMBER_IS:
            printf("Network-Number-Is from ");
            for (j = 0; j < MAX_MAC_LEN; j++) {
                if (j < src->mac_len) {
                    printf("%02X", src->mac[j]);
                }
            }
            if (src->net == 0) {
                /*  It shall be transmitted with a local broadcast address,
                    and shall never be routed. */
                if (npdu_len >= 2) {
                    len += decode_unsigned16(npdu, &dnet);
                    printf(": network number = %u\n", (unsigned)dnet);
                } else {
                    printf(": network number = missing!\n");
                }
            } else {
                /*  Devices shall ignore Network-Number-Is messages that
                    contain SNET/SADR or DNET/DADR information In the NPCI or
                    that are sent with a local unicast address. */
                if (npdu_len >= 2) {
                    len += decode_unsigned16(npdu, &dnet);
                    printf(": network number = %u. SNET=%u\n",
                        (unsigned)dnet, (unsigned)src->net);
                } else {
                    printf(": network number = missing! SNET=%u\n", src->net);
                }
            }
            break;
        case NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK:
        case NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK:
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

static void My_NPDU_Handler(BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* PDU data */
    uint16_t pdu_len)
{ /* length PDU  */
    int apdu_offset = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };

    apdu_offset = npdu_decode(&pdu[0], &dest, src, &npdu_data);
    if (npdu_data.network_layer_message) {
        My_Router_Handler(src, &npdu_data, &pdu[apdu_offset],
            (uint16_t)(pdu_len - apdu_offset));
    } else if ((apdu_offset > 0) && (apdu_offset <= pdu_len)) {
        if ((npdu_data.protocol_version == BACNET_PROTOCOL_VERSION) &&
            ((dest.net == 0) || (dest.net == BACNET_BROADCAST_NETWORK))) {
            /* only handle the version that we know how to handle */
            /* and we are not a router, so ignore messages with
               routing information cause they are not for us */
            apdu_handler(
                src, &pdu[apdu_offset], (uint16_t)(pdu_len - apdu_offset));
        } else {
            if (dest.net) {
                debug_printf("NPDU: DNET=%d.  Discarded!\n", dest.net);
            } else {
                debug_printf("NPDU: BACnet Protocol Version=%d.  Discarded!\n",
                    npdu_data.protocol_version);
            }
        }
    }

    return;
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

static void address_parse(BACNET_ADDRESS *dst, int argc, char *argv[])
{
    int dnet = 0;
    unsigned mac[6];
    int count = 0;
    int index = 0;

    if (argc > 0) {
        count = sscanf(argv[0], "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2],
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
            count = sscanf(argv[2], "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1],
                &mac[2], &mac[3], &mac[4], &mac[5]);
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

    if (argc < 3) {
        printf("Usage: %s DNET status [MAC]\r\n", filename_remove_path(argv[0]));
        return 0;
    }
    if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
        printf("Send BACnet What-Is-Network-Number message to a network.\r\n"
               "\r\n"
               "DNET:\r\n"
               "BACnet destination network number 0-65535\r\n"
               "To omit the BACnet destination network number, use -1.\r\n"
               "Network Number Status:\r\n"
               "0=learned\r\n"
               "1=configured\r\n"
               "MAC:\r\n"
               "Optional MAC address of router for unicast message\r\n"
               "Format: xx[:xx:xx:xx:xx:xx] [dnet xx[:xx:xx:xx:xx:xx]]\r\n"
               "Use hexidecimal MAC addresses.\r\n"
               "\r\n"
               "To send a What-Is-Network-Number request to DNET 86:\r\n"
               "%s 86\r\n"
               "To send a What-Is-Network-Number request to all devices:\r\n"
               "%s -1\r\n",
            filename_remove_path(argv[0]), filename_remove_path(argv[0]));
        return 0;
    }
    /* decode the command line parameters */
    if (argc > 1) {
        Target_Network_Number = strtol(argv[1], NULL, 0);
        if (Target_Network_Number > UINT16_MAX) {
            fprintf(stderr, "DNET=%d - it must be 0 to 65535\r\n",
                Target_Network_Number);
            return 1;
        }
    }
    if (argc > 2) {
        Target_Network_Number_Status = strtol(argv[2], NULL, 0);
        if (Target_Network_Number > UINT8_MAX) {
            fprintf(stderr, "status=%d - it must be 0 to 255\r\n",
                Target_Network_Number_Status);
            return 1;
        }
    }
    if (argc > 3) {
        address_parse(&Target_Router_Address, argc - 2, &argv[2]);
    } else {
        datalink_get_broadcast_address(&Target_Router_Address);
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
    Send_Network_Number_Is(
        &Target_Router_Address, Target_Network_Number,
        Target_Network_Number_Status);
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);
        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            My_NPDU_Handler(&src, &Rx_Buf[0], pdu_len);
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
