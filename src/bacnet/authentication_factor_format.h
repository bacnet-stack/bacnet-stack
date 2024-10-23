/**
 * @file
 * @brief BACnet BACnetAuthenticationFactorFormat structure and codecs
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_AUTHENTICATION_FACTOR_FORMAT_H
#define BACNET_AUTHENTICATION_FACTOR_FORMAT_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

typedef struct BACnetAuthenticationFactorFormat {
    BACNET_AUTHENTICATION_FACTOR_TYPE format_type;
    uint32_t vendor_id, vendor_format;
} BACNET_AUTHENTICATION_FACTOR_FORMAT;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacapp_encode_authentication_factor_format(
    uint8_t *apdu, const BACNET_AUTHENTICATION_FACTOR_FORMAT *aff);
BACNET_STACK_EXPORT
int bacapp_encode_context_authentication_factor_format(
    uint8_t *apdu,
    uint8_t tag_number,
    const BACNET_AUTHENTICATION_FACTOR_FORMAT *aff);
BACNET_STACK_EXPORT
int bacapp_decode_authentication_factor_format(
    const uint8_t *apdu, BACNET_AUTHENTICATION_FACTOR_FORMAT *aff);
BACNET_STACK_EXPORT
int bacapp_decode_context_authentication_factor_format(
    const uint8_t *apdu,
    uint8_t tag_number,
    BACNET_AUTHENTICATION_FACTOR_FORMAT *aff);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
