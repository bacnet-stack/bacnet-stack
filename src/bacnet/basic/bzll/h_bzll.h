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
/* Callback to advertise when protocol address change */
typedef int (*bzll_advertise_address_callback)(
    const uint8_t *protocol_addr, const uint16_t address_size);
/* Callback to request the read property */
typedef int (*bzll_request_read_property_address_callback)(
    const struct bzll_vmac_data *dest);

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bzll_send_mpdu_callback_set(bzll_send_mpdu_callback cb);
BACNET_STACK_EXPORT
void bzll_get_my_address_callback_set(bzll_get_address_callback cb);
BACNET_STACK_EXPORT
void bzll_advertise_address_callback_set(bzll_advertise_address_callback cb);
BACNET_STACK_EXPORT
void bzll_request_read_property_address_callback_set(
    bzll_request_read_property_address_callback cb);

BACNET_STACK_EXPORT
void bzll_get_maximum_incoming_transfer_size(uint32_t *transfer_size);
BACNET_STACK_EXPORT
void bzll_get_maximum_outgoing_transfer_size(uint32_t *transfer_size);
BACNET_STACK_EXPORT
void bzll_get_my_protocol_address(
    uint8_t *protocol_address, uint8_t *protocol_address_size);
BACNET_STACK_EXPORT
bool bzll_match_protocol_address(
    const uint8_t *protocol_addr,
    const uint8_t address_size);
BACNET_STACK_EXPORT
bool bzll_update_object_protocol_address(
    uint8_t *protocol_addr,
    uint16_t address_size);
BACNET_STACK_EXPORT
bool bzll_update_node_protocol_address(
    struct bzll_vmac_data *vmac_data,
    uint8_t *protocol_addr,
    uint16_t address_size);
BACNET_STACK_EXPORT
void bzll_set_broadcast_group_id(uint16_t group_id);
BACNET_STACK_EXPORT
uint16_t bzll_get_broadcast_group_id(void);

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
BACNET_STACK_EXPORT
bool bzll_debug_enabled(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
