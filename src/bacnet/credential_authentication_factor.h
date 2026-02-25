/**
 * @file
 * @brief BACnetCredentialAuthenticationFactor encode and decode functions
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_CREDENTIAL_AUTHENTICATION_FACTOR_H
#define BACNET_CREDENTIAL_AUTHENTICATION_FACTOR_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/authentication_factor.h"

typedef struct {
    BACNET_ACCESS_AUTHENTICATION_FACTOR_DISABLE disable;
    BACNET_AUTHENTICATION_FACTOR authentication_factor;
} BACNET_CREDENTIAL_AUTHENTICATION_FACTOR;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacapp_encode_credential_authentication_factor(
    uint8_t *apdu, const BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor);
BACNET_STACK_EXPORT
int bacapp_encode_context_credential_authentication_factor(
    uint8_t *apdu,
    uint8_t tag,
    const BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor);
BACNET_STACK_EXPORT
int bacapp_decode_credential_authentication_factor(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor);
BACNET_STACK_EXPORT
int bacapp_decode_context_credential_authentication_factor(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag,
    BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
