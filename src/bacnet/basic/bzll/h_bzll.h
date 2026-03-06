/**
 * @file
 * @brief Header file for a basic BACnet Zigbee Link Layer handler
 * @author Steve Karg
 * @date March 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_BZLL_HANDLER_H
#define BACNET_BASIC_BZLL_HANDLER_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/datalink/bzll.h"
#include "bacnet/basic/bzll/bzllvmac.h"

/* Callback for sending MPDU */
typedef int (*bzll_send_mpdu_callback)(
    const struct bzll_vmac_data *dest, const uint8_t *mtu, int mtu_len);
/* Callback for getting my address */
typedef int (*bzll_get_address_callback)(struct bzll_vmac_data *addr);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bzll_send_mpdu_callback_set(bzll_send_mpdu_callback cb);
BACNET_STACK_EXPORT
void bzll_get_my_address_callback_set(bzll_get_address_callback cb);
BACNET_STACK_EXPORT
void bzll_get_broadcast_address_callback_set(bzll_get_address_callback cb);

BACNET_STACK_EXPORT
bool bzll_bacnet_address_from_device_id(
    BACNET_ADDRESS *addr, uint32_t device_id);

BACNET_STACK_EXPORT
bool bzll_bacnet_address_to_device_id(
    const BACNET_ADDRESS *addr, uint32_t *device_id);

BACNET_STACK_EXPORT
void bzll_receive_pdu(
    struct bzll_vmac_data *vmac_src, uint8_t *npdu, uint16_t npdu_len);

BACNET_STACK_EXPORT
void bzll_debug_enable(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
