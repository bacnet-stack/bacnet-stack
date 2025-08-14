/**
 * @file
 * @brief BACnet/IP datalink API and defines
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @ingroup DataLink
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BIP_H
#define BACNET_BIP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
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
void bip_set_interface(const char *ifname);

BACNET_STACK_EXPORT
const char *bip_get_interface(void);

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
int bip_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len);

/* implement in ports module */
BACNET_STACK_EXPORT
int bip_send_mpdu(
    const BACNET_IP_ADDRESS *dest, const uint8_t *mtu, uint16_t mtu_len);

BACNET_STACK_EXPORT
uint16_t bip_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout);

/* use host byte order for setting UDP port */
BACNET_STACK_EXPORT
void bip_set_port(uint16_t port);

BACNET_STACK_EXPORT
bool bip_port_changed(void);

/* returns host byte order of UDP port */
BACNET_STACK_EXPORT
uint16_t bip_get_port(void);

BACNET_STACK_EXPORT
bool bip_set_addr(const BACNET_IP_ADDRESS *addr);

BACNET_STACK_EXPORT
bool bip_get_addr(BACNET_IP_ADDRESS *addr);

BACNET_STACK_EXPORT
bool bip_get_addr_by_name(const char *host_name, BACNET_IP_ADDRESS *addr);

BACNET_STACK_EXPORT
bool bip_set_broadcast_addr(const BACNET_IP_ADDRESS *addr);

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

BACNET_STACK_EXPORT
int bip_set_broadcast_binding(const char *ip4_broadcast);

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
