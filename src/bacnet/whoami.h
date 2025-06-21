/**
 * @file
 * @brief API for BACnet Who-Am-I service encoder and decoder
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_WHO_AM_I_H
#define BACNET_WHO_AM_I_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/bacstr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int who_am_i_request_encode(
    uint8_t *apdu,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *Model_Name,
    const BACNET_CHARACTER_STRING *Serial_Number);

BACNET_STACK_EXPORT
int who_am_i_request_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint16_t *vendor_id,
    BACNET_CHARACTER_STRING *Model_Name,
    BACNET_CHARACTER_STRING *Serial_Number);

BACNET_STACK_EXPORT
int who_am_i_request_service_encode(
    uint8_t *apdu,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
