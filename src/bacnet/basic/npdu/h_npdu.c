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
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacaddr.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/bacenum.h"
#include "bacnet/bits.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

#if PRINT_ENABLED
#include <stdio.h>
#endif

static uint16_t Local_Network_Number;
static uint8_t Local_Network_Number_Status = NETWORK_NUMBER_LEARNED;

/**
 * @brief get the local network number
 * @return local network number
 */
uint16_t npdu_network_number(void)
{
    return Local_Network_Number;
}

/**
 * @brief set the local network number
 * @param net - local network number
 */
void npdu_network_number_set(uint16_t net)
{
    Local_Network_Number = net;
}

/**
 * @brief send the local network number is message
 * @param dst - the destination address for the message
 * @param net - local network number
 * @param status - 0=learned, 1=assigned
 * @return number of bytes sent
 */
int npdu_send_network_number_is(
    BACNET_ADDRESS *dst, uint16_t net, uint8_t status)
{
    uint16_t len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    bool data_expecting_reply = false;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    /*  Upon receipt of a What-Is-Network-Number message,
        a device that knows the local network number shall
        transmit a local broadcast Network-Number-Is message
        back to the source device. */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_network(&npdu_data, NETWORK_MESSAGE_NETWORK_NUMBER_IS,
        data_expecting_reply, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], dst, &my_address, &npdu_data);
    len = encode_unsigned16(&Handler_Transmit_Buffer[pdu_len], net);
    pdu_len += len;
    Handler_Transmit_Buffer[pdu_len] = status;
    pdu_len++;
    bytes_sent = datalink_send_pdu(
        dst, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);

    return bytes_sent;
}

/**
 * @brief send the local network number is message
 * @param dst - the destination address for the message
 * @param net - local network number
 * @return number of bytes sent
 */
int npdu_send_what_is_network_number(BACNET_ADDRESS *dst)
{
    int pdu_len = 0;
    bool data_expecting_reply = false;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS daddr = { 0 };
    BACNET_ADDRESS saddr = { 0 };

    if (dst) {
        bacnet_address_copy(&daddr, dst);
    } else {
        datalink_get_broadcast_address(&daddr);
    }
    datalink_get_my_address(&saddr);
    npdu_encode_npdu_network(&npdu_data, NETWORK_MESSAGE_WHAT_IS_NETWORK_NUMBER,
        data_expecting_reply, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], &daddr, &saddr, &npdu_data);

    /* Now send the message */
    return datalink_send_pdu(
        dst, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
}

/** @file h_npdu.c  Handles messages at the NPDU level of the BACnet stack. */

/** Handler to manage the Network Layer Control Messages received in a packet.
 *  This handler is called if the NCPI bit 7 indicates that this packet is a
 *  network layer message and there is no further DNET to pass it to.
 *  The NCPI has already been decoded into the npdu_data structure.
 * @ingroup MISCHNDLR
 *
 * @param src  [in] The routing source information, if any.
 *                   If src->net and src->len are 0, there is no
 *                   routing source information.
 * @param npdu_data [in] Contains a filled-out structure with information
 * 					 decoded from the NCPI and other NPDU
 * bytes.
 *  @param npdu [in]  Buffer containing the rest of the NPDU, following the
 *  				 bytes that have already been decoded.
 *  @param npdu_len [in] The length of the remaining NPDU message in npdu[].
 */
static void network_control_handler(BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *npdu,
    uint16_t npdu_len)
{
    uint16_t dnet = 0;
    uint8_t status = 0;

    switch (npdu_data->network_message_type) {
        case NETWORK_MESSAGE_WHAT_IS_NETWORK_NUMBER:
            if (src->net == 0) {
                if (Local_Network_Number) {
                    npdu_send_network_number_is(
                        src, Local_Network_Number, Local_Network_Number_Status);
                } else {
                    /*  Upon receipt of a What-Is-Network-Number message,
                        a device that does not know the local network number
                        shall discard the message. */
                }
            } else {
                /*  Devices shall ignore What-Is-Network-Number messages
                    that contain SNET/SADR or DNET/DADR information in
                    the NPCI. */
            }
            break;
        case NETWORK_MESSAGE_NETWORK_NUMBER_IS:
            if (src->net == 0) {
                /*  It shall be transmitted with a local broadcast address,
                    and shall never be routed. */
                if (npdu_len >= 2) {
                    (void)decode_unsigned16(npdu, &dnet);
                    Local_Network_Number = dnet;
                }
                if (npdu_len >= 3) {
                    status = npdu[2];
                    /*  Our net number is always learned, unless we
                        are a router.  Ignore the learned/configured
                        status */
                    (void)status;
                }
            } else {
                /*  Devices shall ignore Network-Number-Is messages that
                    contain SNET/SADR or DNET/DADR information In the NPCI or
                    that are sent with a local unicast address. */
            }
            break;
        default:
            break;
    }
}

/** Handler for the NPDU portion of a received packet.
 *  Aside from error-checking, if the NPDU doesn't contain routing info,
 *  this handler doesn't do much besides stepping over the NPDU header
 *  and passing the remaining bytes to the apdu_handler.
 *  @note The routing (except src) and NCPI information, including
 *  npdu_data->data_expecting_reply, are discarded.
 * @see routing_npdu_handler
 *
 * @ingroup MISCHNDLR
 *
 * @param src  [out] Returned with routing source information if the NPDU
 *                   has any and if this points to non-null storage for it.
 *                   If src->net and src->len are 0 on return, there is no
 *                   routing source information.
 *                   This src describes the original source of the message when
 *                   it had to be routed to reach this BACnet Device, and this
 *                   is passed down into the apdu_handler; however, I don't
 *                   think this project's code has any use for the src info
 *                   on return from this handler, since the response has
 *                   already been sent via the apdu_handler.
 *  @param pdu [in]  Buffer containing the NPDU and APDU of the received packet.
 *  @param pdu_len [in] The size of the received message in the pdu[] buffer.
 */
void npdu_handler(BACNET_ADDRESS *src, uint8_t *pdu, uint16_t pdu_len)
{
    int apdu_offset = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };

    if (pdu_len < 1) {
        return;
    }

    /* only handle the version that we know how to handle */
    if (pdu[0] == BACNET_PROTOCOL_VERSION) {
        apdu_offset =
            bacnet_npdu_decode(&pdu[0], pdu_len, &dest, src, &npdu_data);
        if (npdu_data.network_layer_message) {
            if ((dest.net == 0) || (dest.net == BACNET_BROADCAST_NETWORK)) {
                network_control_handler(src, &npdu_data, &pdu[apdu_offset],
                    (uint16_t)(pdu_len - apdu_offset));
            } else {
                debug_printf("NPDU: message for router. Discarded!\n");
            }
        } else if ((apdu_offset > 0) && (apdu_offset < pdu_len)) {
            if ((dest.net == 0) || (dest.net == BACNET_BROADCAST_NETWORK)) {
                /* only handle the version that we know how to handle */
                /* and we are not a router, so ignore messages with
                   routing information cause they are not for us */
                if ((dest.net == BACNET_BROADCAST_NETWORK) &&
                    ((pdu[apdu_offset] & 0xF0) ==
                        PDU_TYPE_CONFIRMED_SERVICE_REQUEST)) {
                    /* hack for 5.4.5.1 - IDLE */
                    /* ConfirmedBroadcastReceived */
                    /* then enter IDLE - ignore the PDU */
                } else {
                    apdu_handler(src, &pdu[apdu_offset],
                        (uint16_t)(pdu_len - apdu_offset));
                }
            } else {
#if PRINT_ENABLED
                printf("NPDU: DNET=%u.  Discarded!\n", (unsigned)dest.net);
#endif
            }
        }
    } else {
#if PRINT_ENABLED
        printf("NPDU: BACnet Protocol Version=%u.  Discarded!\n",
            (unsigned)pdu[0]);
#endif
    }

    return;
}
