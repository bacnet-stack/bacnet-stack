/**
 * @file
 * @brief BACnet single precision REAL encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_REAL_H
#define BACNET_REAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int decode_real_safe(
        uint8_t * apdu,
        uint32_t len_value,
        float *real_value);

    BACNET_STACK_EXPORT
    int decode_real(
        uint8_t * apdu,
        float *real_value);

    BACNET_STACK_EXPORT
    int encode_bacnet_real(
        float value,
        uint8_t * apdu);
    BACNET_STACK_EXPORT
    int decode_double(
        uint8_t * apdu,
        double *real_value);
    BACNET_STACK_EXPORT
    int decode_double_safe(
        uint8_t * apdu,
        uint32_t len_value,
        double *double_value);

    BACNET_STACK_EXPORT
    int encode_bacnet_double(
        double value,
        uint8_t * apdu);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
