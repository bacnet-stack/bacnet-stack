/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @brief Implementation of the Network Layer using BACnet Ethernet transport
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DL_ETHERNET BACnet Ethernet DataLink Network Layer
 * @ingroup DataLink
 */
#ifndef BACNET_ETHERNET_H
#define BACNET_ETHERNET_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"

/* specific defines for Ethernet */
#define ETHERNET_HEADER_MAX (6 + 6 + 2 + 1 + 1 + 1)
#define ETHERNET_MPDU_MAX (ETHERNET_HEADER_MAX + MAX_PDU)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
bool ethernet_valid(void);
BACNET_STACK_EXPORT
void ethernet_cleanup(void);
BACNET_STACK_EXPORT
bool ethernet_init(char *interface_name);

/* function to send a packet out the 802.2 socket */
/* returns number of bytes sent on success, negative on failure */
BACNET_STACK_EXPORT
int ethernet_send_pdu(
    BACNET_ADDRESS *dest, /* destination address */
    BACNET_NPDU_DATA *npdu_data, /* network information */
    uint8_t *pdu, /* any data to be sent - may be null */
    unsigned pdu_len); /* number of bytes of data */

/* receives an 802.2 framed packet */
/* returns the number of octets in the PDU, or zero on failure */
BACNET_STACK_EXPORT
uint16_t ethernet_receive(
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* PDU data */
    uint16_t max_pdu, /* amount of space available in the PDU  */
    unsigned timeout); /* milliseconds to wait for a packet */

BACNET_STACK_EXPORT
void ethernet_set_my_address(const BACNET_ADDRESS *my_address);
BACNET_STACK_EXPORT
void ethernet_get_my_address(BACNET_ADDRESS *my_address);
BACNET_STACK_EXPORT
void ethernet_get_broadcast_address(
    BACNET_ADDRESS *dest); /* destination address */

/* some functions from Linux driver */
BACNET_STACK_EXPORT
void ethernet_debug_address(const char *info, const BACNET_ADDRESS *dest);
BACNET_STACK_EXPORT
int ethernet_send(uint8_t *mtu, int mtu_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
