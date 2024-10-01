/**
 * @file
 * @brief BACnet/IPv6 datalink API and defines
 * @author Steve Karg
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 * @details Implementation of the Network Layer using BACnet/IPv6 as the
 * transport, as described in ASHRAE 135 Annex J. The functions described here
 * fulfill the roles defined generically at the DataLink level by serving as the
 * implementation of the function templates.
 * @defgroup DLBIP6 BACnet/IPv6 DataLink Network Layer
 * @ingroup DataLink
 */
#ifndef BACNET_BIP6_H
#define BACNET_BIP6_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"
#include "bacnet/datalink/bvlc6.h"

/* specific defines for BACnet/IP over Ethernet */
#define BIP6_HEADER_MAX (1 + 1 + 2)
#define BIP6_MPDU_MAX (BIP6_HEADER_MAX + MAX_PDU)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 6 datalink functions used by demo handlers and applications:
   init, send, receive, cleanup, unicast/broadcast address.
   Note: the addresses used here are VMAC addresses. */
BACNET_STACK_EXPORT
bool bip6_init(char *ifname);
BACNET_STACK_EXPORT
void bip6_cleanup(void);
BACNET_STACK_EXPORT
void bip6_get_broadcast_address(BACNET_ADDRESS *my_address);
BACNET_STACK_EXPORT
void bip6_get_my_address(BACNET_ADDRESS *my_address);
BACNET_STACK_EXPORT
int bip6_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len);
BACNET_STACK_EXPORT
uint16_t bip6_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout);

/* functions that are custom per port */
BACNET_STACK_EXPORT
int bip6_set_interface(char *ifname);

BACNET_STACK_EXPORT
bool bip6_address_match_self(const BACNET_IP6_ADDRESS *addr);

BACNET_STACK_EXPORT
bool bip6_set_addr(const BACNET_IP6_ADDRESS *addr);
BACNET_STACK_EXPORT
bool bip6_get_addr(BACNET_IP6_ADDRESS *addr);

BACNET_STACK_EXPORT
void bip6_set_port(uint16_t port);
BACNET_STACK_EXPORT
uint16_t bip6_get_port(void);

BACNET_STACK_EXPORT
bool bip6_set_broadcast_addr(const BACNET_IP6_ADDRESS *addr);
/* returns network byte order */
BACNET_STACK_EXPORT
bool bip6_get_broadcast_addr(BACNET_IP6_ADDRESS *addr);

BACNET_STACK_EXPORT
int bip6_send_mpdu(
    const BACNET_IP6_ADDRESS *addr, const uint8_t *mtu, uint16_t mtu_len);
BACNET_STACK_EXPORT
bool bip6_send_pdu_queue_empty(void);
BACNET_STACK_EXPORT
void bip6_receive_callback(void);

BACNET_STACK_EXPORT
void bip6_debug_enable(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
