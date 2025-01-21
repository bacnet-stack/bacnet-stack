/**
 * @file
 * @brief BACnetCredentialAuthenticationFactor encode and decode functions
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include "bacnet/credential_authentication_factor.h"
#include "bacnet/bacdcode.h"

int bacapp_encode_credential_authentication_factor(
    uint8_t *apdu, const BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor)
{
    int len;
    int apdu_len = 0;

    len = encode_context_enumerated(&apdu[apdu_len], 0, factor->disable);
    if (len < 0) {
        return -1;
    } else {
        apdu_len += len;
    }

    len = bacapp_encode_context_authentication_factor(
        &apdu[apdu_len], 1, &factor->authentication_factor);
    if (len < 0) {
        return -1;
    } else {
        apdu_len += len;
    }

    return apdu_len;
}

int bacapp_encode_context_credential_authentication_factor(
    uint8_t *apdu,
    uint8_t tag,
    const BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(&apdu[apdu_len], tag);
    apdu_len += len;

    len =
        bacapp_encode_credential_authentication_factor(&apdu[apdu_len], factor);
    apdu_len += len;

    len = encode_closing_tag(&apdu[apdu_len], tag);
    apdu_len += len;

    return apdu_len;
}

int bacapp_decode_credential_authentication_factor(
    const uint8_t *apdu, BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor)
{
    int len;
    int apdu_len = 0;
    uint32_t disable = factor->disable;

    if (decode_is_context_tag(&apdu[apdu_len], 0)) {
        len = decode_context_enumerated(&apdu[apdu_len], 0, &disable);
        if (len < 0) {
            return -1;
        } else if (disable < ACCESS_AUTHENTICATION_FACTOR_DISABLE_MAX) {
            apdu_len += len;
            factor->disable =
                (BACNET_ACCESS_AUTHENTICATION_FACTOR_DISABLE)disable;
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    if (decode_is_context_tag(&apdu[apdu_len], 1)) {
        len = bacapp_decode_context_authentication_factor(
            &apdu[apdu_len], 1, &factor->authentication_factor);
        if (len < 0) {
            return -1;
        } else {
            apdu_len += len;
        }
    } else {
        return -1;
    }

    return apdu_len;
}

int bacapp_decode_context_credential_authentication_factor(
    const uint8_t *apdu,
    uint8_t tag,
    BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag)) {
        len++;
        section_length =
            bacapp_decode_credential_authentication_factor(&apdu[len], factor);

        if (section_length == -1) {
            len = -1;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag)) {
                len++;
            } else {
                len = -1;
            }
        }
    } else {
        len = -1;
    }
    return len;
}
