/**
 * @file
 * @brief API for BACnet You-Are service encoder and decoder
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_YOU_ARE_H
#define BACNET_YOU_ARE_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/bacstr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int you_are_request_encode(
    uint8_t *apdu,
    uint32_t device_id,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number,
    const BACNET_OCTET_STRING *mac_address);

BACNET_STACK_EXPORT
int you_are_request_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint32_t *device_id,
    uint16_t *vendor_id,
    BACNET_CHARACTER_STRING *model_name,
    BACNET_CHARACTER_STRING *serial_number,
    BACNET_OCTET_STRING *mac_address);

BACNET_STACK_EXPORT
int you_are_request_service_encode(
    uint8_t *apdu,
    uint32_t device_id,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number,
    const BACNET_OCTET_STRING *mac_address);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
