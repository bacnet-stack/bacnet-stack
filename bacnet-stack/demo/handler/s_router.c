/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "config.h"
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "tsm.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
/* some demo stuff needed */
#include "handlers.h"
#include "txbuf.h"

static void npdu_encode_npdu_network(
    BACNET_NPDU_DATA * npdu_data,
    BACNET_NETWORK_MESSAGE_TYPE network_message_type,
    BACNET_MESSAGE_PRIORITY priority)
{
    if (npdu_data) {
        npdu_data->data_expecting_reply = false;
        npdu_data->protocol_version = BACNET_PROTOCOL_VERSION;
        npdu_data->network_layer_message = true;       /* false if APDU */
        npdu_data->network_message_type = network_message_type;      /* optional */
        npdu_data->vendor_id = 0;       /* optional, if net message type is > 0x80 */
        npdu_data->priority = priority;
        npdu_data->hop_count = 255;
    }
}

/* find a specific router, or use -1 for limit if you want unlimited */
void Send_Who_Is_Router_To_Network(
    BACNET_ADDRESS *dst,
    int dnet)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    npdu_encode_npdu_network(&npdu_data,
        NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK,
        MESSAGE_PRIORITY_NORMAL);
    /* fixme: should dnet/dlen/dadr be set in NPDU?  */
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], NULL, NULL, &npdu_data);
    /* encode the optional DNET portion of the packet */
    if (dnet >= 0) {
        len = encode_unsigned16(&Handler_Transmit_Buffer[pdu_len], dnet);
        pdu_len += len;
#if PRINT_ENABLED
        fprintf(stderr, "Send Who-Is-Router-To-Network message to %u\n",dnet);
#endif
    } else {
#if PRINT_ENABLED
        fprintf(stderr, "Send Who-Is-Router-To-Network message\n");
#endif
    }
    bytes_sent =
        datalink_send_pdu(dst, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to Send Who-Is-Router-To-Network Request (%s)!\n",
            strerror(errno));
#endif
}
 
/* pDNET_list: list of networks for which I am a router, 
   terminated with -1 */
void Send_I_Am_Router_To_Network(
  const int DNET_list[])
{
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    uint16_t dnet = 0;
    unsigned index = 0;

    npdu_encode_npdu_network(&npdu_data,
        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK,
        MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], NULL, NULL, &npdu_data);
    /* encode the optional DNET list portion of the packet */
#if PRINT_ENABLED
    fprintf(stderr, "Send I-Am-Router-To-Network message to:\n");
#endif
    while (DNET_list[index] != -1) {
        dnet = DNET_list[index];
        len = encode_unsigned16(&Handler_Transmit_Buffer[pdu_len], dnet);
        pdu_len += len;
        index++;
#if PRINT_ENABLED
        fprintf(stderr, "%u\n",dnet);
#endif
    }
    /* I-Am-Router-To-Network shall always be transmitted with 
       a broadcast MAC address. */
    datalink_get_broadcast_address(&dest);
    bytes_sent =
        datalink_send_pdu(&dest, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to send I-Am-Router-To-Network message (%s)!\n",
            strerror(errno));
#endif
}

/* */
void Send_Initialize_Routing_Table(
  BACNET_ROUTER_PORT *router_port_list)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    uint8_t number_of_ports = 0;
    BACNET_ROUTER_PORT *router_port;
    unsigned i = 0; /* counter */

    npdu_encode_npdu_network(&npdu_data,
        NETWORK_MESSAGE_INIT_RT_TABLE,
        MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], NULL, NULL, &npdu_data);
    /* encode the optional port_info list portion of the packet */
    router_port = router_port_list;
    while (router_port) {
        number_of_ports++;
        router_port = router_port->next;
    }
    Handler_Transmit_Buffer[pdu_len++] = number_of_ports;
    if (number_of_ports) {
        router_port = router_port_list;
        while (router_port) {
            len = encode_unsigned16(
                &Handler_Transmit_Buffer[pdu_len], 
                router_port->dnet);
            pdu_len += len;
            Handler_Transmit_Buffer[pdu_len++] = router_port->id;
            Handler_Transmit_Buffer[pdu_len++] = router_port->info_len;
            for (i = 0; i < router_port->info_len; i++) {
                Handler_Transmit_Buffer[pdu_len++] = router_port->info[i];
            }
            router_port = router_port->next;
        }
    }
    /* Init Routing Table shall always be transmitted with 
       a broadcast MAC address. */
    datalink_get_broadcast_address(&dest);
    bytes_sent =
        datalink_send_pdu(&dest, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to send Initialize-Routing-Table message (%s)!\n",
            strerror(errno));
#endif
}

/* */
void Send_Initialize_Routing_Table_Ack(
  BACNET_ROUTER_PORT *router_port_list)
{
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    npdu_encode_npdu_network(&npdu_data,
        NETWORK_MESSAGE_INIT_RT_TABLE_ACK,
        MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], NULL, NULL, &npdu_data);
    /* encode the optional DNET list portion of the packet */
    datalink_get_broadcast_address(&dest);
    bytes_sent =
        datalink_send_pdu(&dest, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to Send Initialize-Routing-Table-Ack message (%s)!\n",
            strerror(errno));
#endif
}
