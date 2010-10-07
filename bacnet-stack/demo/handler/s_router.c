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
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "tsm.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "bactext.h"
/* some demo stuff needed */
#include "handlers.h"
#include "txbuf.h"
#include "client.h"

/** @file s_router.c  Methods to send various BACnet Router Network Layer Messages. */

/** Initialize an npdu_data structure with given parameters and good defaults,
 * and add the Network Layer Message fields.
 * The name is a misnomer, as it doesn't do any actual encoding here.
 * @see npdu_encode_npdu_data for a simpler version to use when sending an
 *           APDU instead of a Network Layer Message.
 * 
 * @param npdu_data [out] Returns a filled-out structure with information
 * 					 provided by the other arguments and good defaults.
 * @param network_message_type [in] The type of Network Layer Message.
 * @param data_expecting_reply [in] True if message should have a reply.
 * @param priority [in] One of the 4 priorities defined in section 6.2.2,
 *                      like B'11' = Life Safety message
 */
static void npdu_encode_npdu_network(
    BACNET_NPDU_DATA * npdu_data,
    BACNET_NETWORK_MESSAGE_TYPE network_message_type,
    bool data_expecting_reply,
    BACNET_MESSAGE_PRIORITY priority)
{
    if (npdu_data) {
        npdu_data->data_expecting_reply = data_expecting_reply;
        npdu_data->protocol_version = BACNET_PROTOCOL_VERSION;
        npdu_data->network_layer_message = true;        /* false if APDU */
        npdu_data->network_message_type = network_message_type; /* optional */
        npdu_data->vendor_id = 0;       /* optional, if net message type is > 0x80 */
        npdu_data->priority = priority;
        npdu_data->hop_count = DFLT_HOP_COUNT;		
    }
}


/** Function to encode and send any supported Network Layer Message.
 * The payload for the message is encoded from information in the iArgs[] array.
 * The contents of iArgs are are, per message type:
 * - NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK: Single int for DNET requested
 * - NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK: Array of DNET(s) to send,
 * 		terminated with -1
 * 
 * @param network_message_type [in] The type of message to be sent.
 * @param dst [in/out] If not NULL, contains the destination for the message.
 * @param iArgs [in] An optional array of values whose meaning depends on 
 *                   the type of message.
 * @return Number of bytes sent, or <=0 if no message was sent.
 */
int Send_Network_Layer_Message( 
	    BACNET_NETWORK_MESSAGE_TYPE network_message_type,
	    BACNET_ADDRESS * dst,
	    int * iArgs )
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    int *pVal = iArgs;		/* Start with first value */
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS bcastDest;
    
    /* If dst was NULL, get our (local net) broadcast MAC address. */
    if ( dst == NULL ) {
		datalink_get_broadcast_address(&bcastDest);
    	dst = &bcastDest;
    }

    npdu_encode_npdu_network(&npdu_data,
    		network_message_type, false,
    		MESSAGE_PRIORITY_NORMAL);
    
    /* We don't need src information, since a message can't originate from
     * our downstream BACnet network.
     */
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], dst, NULL, &npdu_data);
    
    /* Now encode the optional payload bytes, per message type */
    switch ( network_message_type )
    {
    case NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK:
        if (*pVal >= 0) {
            len =
                encode_unsigned16(&Handler_Transmit_Buffer[pdu_len],
                (uint16_t) *pVal);
            pdu_len += len;
        } 
        /* else, don't encode a DNET */
        break;
        
    case NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK:
    	while ( *pVal >= 0 ) {
            len =
                encode_unsigned16(&Handler_Transmit_Buffer[pdu_len],
                (uint16_t) *pVal);
            pdu_len += len;
    		pVal++;
    	}
    	break;
    	
    default:
#if PRINT_ENABLED
    	fprintf(stderr, "Not sent: %s message unsupported \n", 
         		bactext_network_layer_msg_name( network_message_type ) );
  #endif
    	return 0;
    	break;		/* Will never reach this line */
    }
    
#if PRINT_ENABLED
    if ( dst != NULL )
        fprintf(stderr, "Sending %s message to BACnet network %u \n", 
        		bactext_network_layer_msg_name( network_message_type ),
        	     dst->net );
    else
        fprintf(stderr, "Sending %s message to local BACnet network \n", 
         		bactext_network_layer_msg_name( network_message_type ) );
#endif

    /* Now send the message */
    bytes_sent =
        datalink_send_pdu(dst, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0) {
    	int wasErrno = errno;	/* preserve the errno */
        fprintf(stderr,
            "Failed to send %s message (%s)!\n",
            bactext_network_layer_msg_name( network_message_type ),
            strerror(wasErrno));
    }
#endif
	return bytes_sent;
}


/** Finds a specific router, or all reachable BACnet networks.
 * The response(s) will come in I-am-router-to-network message(s).
 * 
 * @param dst [in] If NULL, request will be broadcast to the local BACnet
 *                 network.  Optionally may designate a particular router
 *                 destination to respond.
 * @param dnet [in] Which BACnet network to request for; if -1, no DNET
 *                 will be sent and the receiving router(s) will send
 *                 their full list of reachable BACnet networks.
 */
void Send_Who_Is_Router_To_Network(
    BACNET_ADDRESS * dst,
    int dnet)
{
	Send_Network_Layer_Message( NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK,
		    					dst, &dnet );
}

/** Broadcast an I-am-router-to-network message, giving the list of networks we can reach.
 * The message will be sent to our normal DataLink Layer interface, not the routed backend.
 * @param DNET_list [in] list of BACnet network numbers for which I am a router, terminated with -1
 */
void Send_I_Am_Router_To_Network(
    const int DNET_list[])
{
    /* Use a NULL dst here since we want a broadcast MAC address. */
	Send_Network_Layer_Message( NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK,
		    					NULL, (int *) DNET_list );
}

/* */
void Send_Initialize_Routing_Table(
    BACNET_ADDRESS * dst,
    BACNET_ROUTER_PORT * router_port_list)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    uint8_t number_of_ports = 0;
    BACNET_ROUTER_PORT *router_port;
    uint8_t i = 0;      /* counter */

    npdu_encode_npdu_network(&npdu_data, NETWORK_MESSAGE_INIT_RT_TABLE, true,
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
            len =
                encode_unsigned16(&Handler_Transmit_Buffer[pdu_len],
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
#if PRINT_ENABLED
    fprintf(stderr, "Send Initialize-Routing-Table message\n");
#endif
    bytes_sent =
        datalink_send_pdu(dst, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr,
            "Failed to send Initialize-Routing-Table message (%s)!\n",
            strerror(errno));
#endif
}

/* */
void Send_Initialize_Routing_Table_Ack(
		const int DNET_list[] )
{
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    // BACNET_ROUTER_PORT * router_port_list;

    /* FIXME: is this parameter needed? */
//    router_port_list = router_port_list;
    /* setup packet for sending */
    npdu_encode_npdu_network(&npdu_data, NETWORK_MESSAGE_INIT_RT_TABLE_ACK,
        false, MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], NULL, NULL, &npdu_data);
    /* encode the optional DNET list portion of the packet */
    datalink_get_broadcast_address(&dest);
    bytes_sent =
        datalink_send_pdu(&dest, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr,
            "Failed to Send Initialize-Routing-Table-Ack message (%s)!\n",
            strerror(errno));
#endif
}
