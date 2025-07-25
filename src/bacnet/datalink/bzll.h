/**
 * @file
 * @brief A BACnet/ZigBee Data Link Layer (BZLL)
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2025
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DL_ZIGBEE BACnet/ZigBee Data Link Layer (BZLL)
 * @ingroup DataLink
 */
#ifndef BACNET_ZIGBEE_LINK_LAYER_H
#define BACNET_ZIGBEE_LINK_LAYER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"

/* ZigBee Cluster Library (ZCL) client to server frame */
#define BZLL_HEADER_MAX (1 + 1)
#define BZLL_MPDU_MAX (BZLL_HEADER_MAX + MAX_PDU)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
bool bzll_valid(void);
BACNET_STACK_EXPORT
void bzll_cleanup(void);
BACNET_STACK_EXPORT
bool bzll_init(char *interface_name);

BACNET_STACK_EXPORT
int bzll_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len);

BACNET_STACK_EXPORT
uint16_t bzll_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout);
BACNET_STACK_EXPORT
int bzll_send(uint8_t *mtu, int mtu_len);

BACNET_STACK_EXPORT
void bzll_set_my_address(const BACNET_ADDRESS *my_address);
BACNET_STACK_EXPORT
void bzll_get_my_address(BACNET_ADDRESS *my_address);
BACNET_STACK_EXPORT
void bzll_get_broadcast_address(BACNET_ADDRESS *dest);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
