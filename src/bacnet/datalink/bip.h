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
#ifndef BIP_H
#define BIP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/bvlc.h"

/* specific defines for BACnet/IP over Ethernet */
#define BIP_HEADER_MAX (1 + 1 + 2)
#define BIP_MPDU_MAX (BIP_HEADER_MAX + MAX_PDU)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* note: define init, set_interface, and cleanup in your port */
    /* on Linux, ifname is eth0, ath0, arc0, and others.
       on Windows, ifname is the dotted ip address of the interface */
    BACNET_STACK_EXPORT
    bool bip_init(char *ifname);

    BACNET_STACK_EXPORT
    void bip_set_interface(char *ifname);

    BACNET_STACK_EXPORT
    void bip_cleanup(void);

    /* common BACnet/IP functions */
    BACNET_STACK_EXPORT
    bool bip_valid(void);

    BACNET_STACK_EXPORT
    void bip_get_broadcast_address(BACNET_ADDRESS *dest);

    BACNET_STACK_EXPORT
    void bip_get_my_address(BACNET_ADDRESS *my_address);

    BACNET_STACK_EXPORT
    int bip_send_pdu(BACNET_ADDRESS *dest,
        BACNET_NPDU_DATA *npdu_data,
        uint8_t *pdu,
        unsigned pdu_len);

    /* implement in ports module */
    BACNET_STACK_EXPORT
    int bip_send_mpdu(BACNET_IP_ADDRESS *dest, uint8_t *mtu, uint16_t mtu_len);

    BACNET_STACK_EXPORT
    uint16_t bip_receive(BACNET_ADDRESS *src,
        uint8_t *pdu,
        uint16_t max_pdu,
        unsigned timeout);

    /* use host byte order for setting UDP port */
    BACNET_STACK_EXPORT
    void bip_set_port(uint16_t port);

    BACNET_STACK_EXPORT
    bool bip_port_changed(void);

    /* returns host byte order of UDP port */
    BACNET_STACK_EXPORT
    uint16_t bip_get_port(void);

    BACNET_STACK_EXPORT
    bool bip_set_addr(BACNET_IP_ADDRESS *addr);

    BACNET_STACK_EXPORT
    bool bip_get_addr(BACNET_IP_ADDRESS *addr);

    BACNET_STACK_EXPORT
    bool bip_get_addr_by_name(const char *host_name, BACNET_IP_ADDRESS *addr);

    BACNET_STACK_EXPORT
    bool bip_set_broadcast_addr(BACNET_IP_ADDRESS *addr);

    BACNET_STACK_EXPORT
    bool bip_get_broadcast_addr(BACNET_IP_ADDRESS *addr);

    BACNET_STACK_EXPORT
    bool bip_set_subnet_prefix(uint8_t prefix);

    BACNET_STACK_EXPORT
    uint8_t bip_get_subnet_prefix(void);

    BACNET_STACK_EXPORT
    void bip_debug_enable(void);

    BACNET_STACK_EXPORT
    void bip_debug_disable(void);

    BACNET_STACK_EXPORT
    int bip_get_socket(void);

    BACNET_STACK_EXPORT
    int bip_get_broadcast_socket(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DLBIP BACnet/IP DataLink Network Layer
 * @ingroup DataLink
 * Implementation of the Network Layer using BACnet/IP as the transport, as
 * described in Annex J.
 * The functions described here fulfill the roles defined generically at the
 * DataLink level by serving as the implementation of the function templates.
 *
 */
#endif
