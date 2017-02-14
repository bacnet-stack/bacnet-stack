/**
* @file
* @author Steve Karg
* @date 2015
* @defgroup DLBIP6 BACnet/IPv6 DataLink Network Layer
* @ingroup DataLink
*
* Implementation of the Network Layer using BACnet/IPv6 as the transport, as
* described in Annex J.
* The functions described here fulfill the roles defined generically at the
* DataLink level by serving as the implementation of the function templates.
*/
#ifndef BIP6_H
#define BIP6_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bacdef.h"
#include "npdu.h"
#include "bvlc6.h"

/* specific defines for BACnet/IP over Ethernet */
#define BIP6_HEADER_MAX (1 + 1 + 2)
#define BIP6_MPDU_MAX (BIP6_HEADER_MAX+MAX_PDU)
/* for legacy demo applications */
#define MAX_MPDU BIP6_MPDU_MAX

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* 6 datalink functions used by demo handlers and applications:
       init, send, receive, cleanup, unicast/broadcast address.
       Note: the addresses used here are VMAC addresses. */
    bool bip6_init(
        char *ifname);
    void bip6_cleanup(
        void);
    void bip6_get_broadcast_address(
        BACNET_ADDRESS * my_address);
    void bip6_get_my_address(
        BACNET_ADDRESS * my_address);
    int bip6_send_pdu(
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * npdu_data,
        uint8_t * pdu,
        unsigned pdu_len);
    uint16_t bip6_receive(
        BACNET_ADDRESS * src,
        uint8_t * pdu,
        uint16_t max_pdu,
        unsigned timeout);

    /* functions that are custom per port */
    bool bip6_address_match_self(
        BACNET_IP6_ADDRESS *addr);

    void bip6_set_interface(
        char *ifname);

    bool bip6_set_addr(
        BACNET_IP6_ADDRESS *addr);
    bool bip6_get_addr(
        BACNET_IP6_ADDRESS *addr);

    void bip6_set_port(
        uint16_t port);
    uint16_t bip6_get_port(
        void);

    bool bip6_set_broadcast_addr(
        BACNET_IP6_ADDRESS *addr);
    /* returns network byte order */
    bool bip6_get_broadcast_addr(
        BACNET_IP6_ADDRESS *addr);

    int bip6_send_mpdu(
        BACNET_IP6_ADDRESS *addr,
        uint8_t * mtu,
        uint16_t mtu_len);

    bool bip6_send_success(void);
    bool bip6_send_failed(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
