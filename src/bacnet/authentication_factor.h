/**
 * @file
 * @brief BACnet BACnetAuthenticationFactor structure and codecs
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_AUTHENTICATION_FACTOR_H
#define BACNET_AUTHENTICATION_FACTOR_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"

typedef struct BACnetAuthenticationFactor {
    BACNET_AUTHENTICATION_FACTOR_TYPE format_type;
    uint32_t format_class;
    BACNET_OCTET_STRING value;
} BACNET_AUTHENTICATION_FACTOR;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int bacapp_encode_authentication_factor(
        uint8_t * apdu,
        const BACNET_AUTHENTICATION_FACTOR * af);
    BACNET_STACK_EXPORT
    int bacapp_encode_context_authentication_factor(
        uint8_t * apdu,
        uint8_t tag,
        const BACNET_AUTHENTICATION_FACTOR * af);
    BACNET_STACK_EXPORT
    int bacapp_decode_authentication_factor(
        const uint8_t * apdu,
        BACNET_AUTHENTICATION_FACTOR * af);
    BACNET_STACK_EXPORT
    int bacapp_decode_context_authentication_factor(
        const uint8_t * apdu,
        uint8_t tag,
        BACNET_AUTHENTICATION_FACTOR * af);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
