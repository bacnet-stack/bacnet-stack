/**************************************************************************
 *
 * Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#ifndef BIP_H
#define BIP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"

/* specific defines for BACnet/IP over Ethernet */
#define BIP_HEADER_MAX (1 + 1 + 2)
#define BIP_MPDU_MAX (BIP_HEADER_MAX + MAX_PDU)

#define BVLL_TYPE_BACNET_IP (0x81)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* note: define init, set_interface, and cleanup in your port */
/* on Linux, ifname is eth0, ath0, arc0, and others.
   on Windows, ifname is the dotted ip address of the interface */
bool bip_init(char *ifname);
void bip_set_interface(const char *ifname);
void bip_cleanup(void);

/* Convert uint8_t IPv4 to uint32 */
uint32_t convertBIP_Address2uint32(const uint8_t *bip_address);
void convertUint32Address_2_uint8Address(uint32_t ip, uint8_t *address);
/* common BACnet/IP functions */
void bip_set_socket(uint8_t sock_fd);
uint8_t bip_socket(void);
bool bip_valid(void);
void bip_get_broadcast_address(BACNET_ADDRESS *dest); /* destination address */
void bip_get_my_address(BACNET_ADDRESS *my_address);

/* function to send a packet out the BACnet/IP socket */
/* returns zero on success, non-zero on failure */
int bip_send_pdu(
    BACNET_ADDRESS *dest, /* destination address */

    BACNET_NPDU_DATA *npdu_data, /* network information */

    uint8_t *pdu, /* any data to be sent - may be null */

    unsigned pdu_len); /* number of bytes of data */

/* receives a BACnet/IP packet */
/* returns the number of octets in the PDU, or zero on failure */
uint16_t bip_receive(
    BACNET_ADDRESS *src, /* source address */

    uint8_t *pdu, /* PDU data */

    uint16_t max_pdu, /* amount of space available in the PDU  */

    unsigned timeout); /* milliseconds to wait for a packet */

/* use host byte order for setting */
void bip_set_port(uint16_t port);
/* returns host byte order */
uint16_t bip_get_port(void);

/* use network byte order for setting */
void bip_set_addr(const uint8_t *net_address);
/* returns network byte order */
uint8_t *bip_get_addr(void);

/* use network byte order for setting */
void bip_set_broadcast_addr(const uint8_t *net_address);
/* returns network byte order */
uint8_t *bip_get_broadcast_addr(void);

/* gets an IP address by name, where name can be a
   string that is an IP address in dotted form, or
   a name that is a domain name
   returns 0 if not found, or
   an IP address in network byte order */
long bip_getaddrbyname(const char *host_name);

void bip_debug_enable(void);

void bip_debug_disable(void);

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
