/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
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
*********************************************************************/
#ifndef DATALINK_H
#define DATALINK_H

#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/config.h"
#include "bacnet/bacdef.h"

#if defined(BACDL_ETHERNET)
#include "bacnet/datalink/ethernet.h"
#define MAX_MPDU ETHERNET_MPDU_MAX

#define datalink_init ethernet_init
#define datalink_send_pdu ethernet_send_pdu
#define datalink_receive ethernet_receive
#define datalink_cleanup ethernet_cleanup
#define datalink_get_broadcast_address ethernet_get_broadcast_address
#define datalink_get_my_address ethernet_get_my_address
#define datalink_maintenance_timer(s)

#elif defined(BACDL_ARCNET)
#include "bacnet/datalink/arcnet.h"
#define MAX_MPDU ARCNET_MPDU_MAX

#define datalink_init arcnet_init
#define datalink_send_pdu arcnet_send_pdu
#define datalink_receive arcnet_receive
#define datalink_cleanup arcnet_cleanup
#define datalink_get_broadcast_address arcnet_get_broadcast_address
#define datalink_get_my_address arcnet_get_my_address
#define datalink_maintenance_timer(s)

#elif defined(BACDL_MSTP)
#include "bacnet/datalink/dlmstp.h"
#define MAX_MPDU DLMSTP_MPDU_MAX

#define datalink_init dlmstp_init
#define datalink_send_pdu dlmstp_send_pdu
#define datalink_receive dlmstp_receive
#define datalink_cleanup dlmstp_cleanup
#define datalink_get_broadcast_address dlmstp_get_broadcast_address
#define datalink_get_my_address dlmstp_get_my_address
#define datalink_maintenance_timer(s)

#elif defined(BACDL_BIP)
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#define MAX_MPDU BIP_MPDU_MAX

#define datalink_init bip_init
#define datalink_send_pdu bip_send_pdu
#define datalink_receive bip_receive
#define datalink_cleanup bip_cleanup
#define datalink_get_broadcast_address bip_get_broadcast_address
#ifdef BAC_ROUTING
BACNET_STACK_EXPORT
void routed_get_my_address(
    BACNET_ADDRESS * my_address);
#define datalink_get_my_address routed_get_my_address
#else
#define datalink_get_my_address bip_get_my_address
#endif
#define datalink_maintenance_timer(s) bvlc_maintenance_timer(s)

#elif defined(BACDL_BIP6)
#include "bacnet/datalink/bip6.h"
#include "bacnet/datalink/bvlc6.h"
#include "bacnet/basic/bbmd6/h_bbmd6.h"
#define MAX_MPDU BIP6_MPDU_MAX

#define datalink_init bip6_init
#define datalink_send_pdu bip6_send_pdu
#define datalink_receive bip6_receive
#define datalink_cleanup bip6_cleanup
#define datalink_get_broadcast_address bip6_get_broadcast_address
#define datalink_get_my_address bip6_get_my_address
#define datalink_maintenance_timer(s) bvlc6_maintenance_timer(s)

#elif defined(BACDL_ALL) || defined(BACDL_NONE) || defined(BACDL_CUSTOM)
#include "bacnet/npdu.h"

#define MAX_HEADER (8)
#define MAX_MPDU (MAX_HEADER+MAX_PDU)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    bool datalink_init(char *ifname);

    BACNET_STACK_EXPORT
    int datalink_send_pdu(
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * npdu_data,
        uint8_t * pdu,
        unsigned pdu_len);

    BACNET_STACK_EXPORT
    uint16_t datalink_receive(
        BACNET_ADDRESS * src,
        uint8_t * pdu,
        uint16_t max_pdu,
        unsigned timeout);

    BACNET_STACK_EXPORT
    void datalink_cleanup(
        void);

    BACNET_STACK_EXPORT
    void datalink_get_broadcast_address(
        BACNET_ADDRESS * dest);

    BACNET_STACK_EXPORT
    void datalink_get_my_address(
        BACNET_ADDRESS * my_address);

    BACNET_STACK_EXPORT
    void datalink_set_interface(
        char *ifname);

    BACNET_STACK_EXPORT
    void datalink_set(
        char *datalink_string);

    BACNET_STACK_EXPORT
    void datalink_maintenance_timer(uint16_t seconds);

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
 * - BACDL_BIP      -- for ANNEX J - BACnet/IPv4
 * - BACDL_BIP6     -- for ANNEX U - BACnet/IPv6
 * - BACDL_ALL      -- Unspecified for the build, so the transport can be
 *                     chosen at runtime from among these choices.
 * - BACDL_NONE      -- Unspecified for the build for unit testing
 * - BACDL_CUSTOM    -- For externally linked datalink_xxx functions
 * - Clause 10 POINT-TO-POINT (PTP) and Clause 11 EIA/CEA-709.1 ("LonTalk") LAN
 *   are not currently supported by this project.
                                                                                                                                                                                              *//** @defgroup DLTemplates DataLink Template Functions
 * @ingroup DataLink
 * Most of the functions in this group are function templates which are assigned
 * to a specific DataLink network layer implementation either at compile time or
 * at runtime.
 */
#endif
