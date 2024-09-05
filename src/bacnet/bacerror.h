/**
 * @file
 * @brief BACnet error encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_ERROR_H
#define BACNET_ERROR_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacerror_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    BACNET_CONFIRMED_SERVICE service,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code);

BACNET_STACK_EXPORT
int bacerror_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_CONFIRMED_SERVICE *service,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code);

BACNET_STACK_EXPORT
int bacerror_decode_error_class_and_code(
    const uint8_t *apdu,
    unsigned apdu_size,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
