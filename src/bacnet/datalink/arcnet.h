/**
 * @file
 * @brief BACnet ARCNET datalink interface and defines
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLARCNET BACnet ARCNET DataLink Network Layer
 * @ingroup DataLink
 */
#ifndef BACNET_ARCNET_H
#define BACNET_ARCNET_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"

/* specific defines for ARCNET */
#define ARCNET_HEADER_MAX (1 + 1 + 2 + 2 + 1 + 1 + 1 + 1)
#define ARCNET_MPDU_MAX (ARCNET_HEADER_MAX + MAX_PDU)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
bool arcnet_valid(void);
BACNET_STACK_EXPORT
void arcnet_cleanup(void);
BACNET_STACK_EXPORT
bool arcnet_init(char *interface_name);

/* function to send a packet out the 802.2 socket */
/* returns zero on success, non-zero on failure */
BACNET_STACK_EXPORT
int arcnet_send_pdu(
    BACNET_ADDRESS *dest, /* destination address */
    BACNET_NPDU_DATA *npdu_data, /* network information */
    uint8_t *pdu, /* any data to be sent - may be null */
    unsigned pdu_len); /* number of bytes of data */

/* receives an framed packet */
/* returns the number of octets in the PDU, or zero on failure */
BACNET_STACK_EXPORT
uint16_t arcnet_receive(
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* PDU data */
    uint16_t max_pdu, /* amount of space available in the PDU  */
    unsigned timeout); /* milliseconds to wait for a packet */

BACNET_STACK_EXPORT
void arcnet_get_my_address(BACNET_ADDRESS *my_address);
BACNET_STACK_EXPORT
void arcnet_get_broadcast_address(
    BACNET_ADDRESS *dest); /* destination address */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
