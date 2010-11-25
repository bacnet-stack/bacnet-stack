/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef DATALINK_H
#define DATALINK_H

#include "config.h"
#include "bacdef.h"

#if defined(BACDL_ETHERNET)
#include "ethernet.h"

#define datalink_init ethernet_init
#define datalink_send_pdu ethernet_send_pdu
#define datalink_receive ethernet_receive
#define datalink_cleanup ethernet_cleanup
#define datalink_get_broadcast_address ethernet_get_broadcast_address
#define datalink_get_my_address ethernet_get_my_address

#elif defined(BACDL_ARCNET)
#include "arcnet.h"

#define datalink_init arcnet_init
#define datalink_send_pdu arcnet_send_pdu
#define datalink_receive arcnet_receive
#define datalink_cleanup arcnet_cleanup
#define datalink_get_broadcast_address arcnet_get_broadcast_address
#define datalink_get_my_address arcnet_get_my_address

#elif defined(BACDL_MSTP)
#include "dlmstp.h"

#define datalink_init dlmstp_init
#define datalink_send_pdu dlmstp_send_pdu
#define datalink_receive dlmstp_receive
#define datalink_cleanup dlmstp_cleanup
#define datalink_get_broadcast_address dlmstp_get_broadcast_address
#define datalink_get_my_address dlmstp_get_my_address

#elif defined(BACDL_BIP)
#include "bip.h"
#include "bvlc.h"

#define datalink_init bip_init
#if defined(BBMD_ENABLED) && BBMD_ENABLED
#define datalink_send_pdu bvlc_send_pdu
#define datalink_receive bvlc_receive
#else
#define datalink_send_pdu bip_send_pdu
#define datalink_receive bip_receive
#endif
#define datalink_cleanup bip_cleanup
#define datalink_get_broadcast_address bip_get_broadcast_address
#ifdef BAC_ROUTING
#define datalink_get_my_address routed_get_my_address
#else
#define datalink_get_my_address bip_get_my_address
#endif

#else /* Ie, BACDL_ALL */
#include "npdu.h"

#define MAX_HEADER (8)
#define MAX_MPDU (MAX_HEADER+MAX_PDU)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int datalink_send_pdu(
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * npdu_data,
        uint8_t * pdu,
        unsigned pdu_len);
    extern uint16_t datalink_receive(
        BACNET_ADDRESS * src,
        uint8_t * pdu,
        uint16_t max_pdu,
        unsigned timeout);
    extern void datalink_cleanup(
        void);
    extern void datalink_get_broadcast_address(
        BACNET_ADDRESS * dest);
    extern void datalink_get_my_address(
        BACNET_ADDRESS * my_address);
    extern void datalink_set_interface(
        char *ifname);
    extern void datalink_set(
        char *datalink_string);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
/** @defgroup DataLink The BACnet Network (DataLink) Layer
 * <b>6 THE NETWORK LAYER </b><br>
 * The purpose of the BACnet network layer is to provide the means by which 
 * messages can be relayed from one BACnet network to another, regardless of 
 * the BACnet data link technology in use on that network. Whereas the data 
 * link layer provides the capability to address messages to a single device 
 * or broadcast them to all devices on the local network, the network layer 
 * allows messages to be directed to a single remote device, broadcast on a 
 * remote network, or broadcast globally to all devices on all networks. 
 * A BACnet Device is uniquely located by a network number and a MAC address.
 * 
 * Each client or server application must define exactly one of these
 * DataLink settings, which will control which parts of the code will be built:
 * - BACDL_ETHERNET -- for Clause 7 ISO 8802-3 ("Ethernet") LAN
 * - BACDL_ARCNET   -- for Clause 8 ARCNET LAN
 * - BACDL_MSTP     -- for Clause 9 MASTER-SLAVE/TOKEN PASSING (MS/TP) LAN
 * - BACDL_BIP      -- for ANNEX J - BACnet/IP 
 * - BACDL_ALL      -- Unspecified for the build, so the transport can be 
 *                     chosen at runtime from among these choices.
 * - Clause 10 POINT-TO-POINT (PTP) and Clause 11 EIA/CEA-709.1 ("LonTalk") LAN
 *   are not currently supported by this project.
 *//** @defgroup DLTemplates DataLink Template Functions
 * @ingroup DataLink
 * Most of the functions in this group are function templates which are assigned
 * to a specific DataLink network layer implementation either at compile time or
 * at runtime.
 */
#endif
