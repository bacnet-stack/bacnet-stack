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

/**
 * @brief Encode a BACnetCredentialAuthenticationFactor structure into an APDU
 * buffer.
 *
 * BACnetCredentialAuthenticationFactor ::= SEQUENCE {
 *   disable[0] BACnetAccessAuthenticationFactorDisable,
 *   authentication-factor[1] BACnetAuthenticationFactor
 * }
 *
 * @param apdu [in] The APDU buffer, or NULL for length
 * @param factor [in] The BACnetCredentialAuthenticationFactor structure to
 * encode
 * @return number of bytes encoded, or negative on error
 */
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

/**
 * @brief Encode a BACnetCredentialAuthenticationFactor structure into an APDU
 * buffer, with context tag.
 *
 * @param apdu [in] The APDU buffer, or NULL for length
 * @param tag [in] The context tag number
 * @param factor [in] The BACnetCredentialAuthenticationFactor structure to
 * encode
 * @return number of bytes encoded, or negative on error
 */
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

/**
 * @brief Decode a BACnetCredentialAuthenticationFactor structure from an APDU
 * buffer.
 *
 * BACnetCredentialAuthenticationFactor ::= SEQUENCE {
 *   disable[0] BACnetAccessAuthenticationFactorDisable,
 *   authentication-factor[1] BACnetAuthenticationFactor
 * }
 *
 * @param apdu [in] The APDU buffer
 * @param apdu_size [in] The size of the APDU buffer
 * @param factor [out] The BACnetCredentialAuthenticationFactor structure to
 * decode
 * @return number of bytes decoded, or negative on error
 */
int bacapp_decode_credential_authentication_factor(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor)
{
    int len;
    int apdu_len = 0;
    uint32_t disable = factor->disable;

    /* disable[0] BACnetAccessAuthenticationFactorDisable */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &disable);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    } else if (disable < UINT16_MAX) {
        apdu_len += len;
        factor->disable = (BACNET_ACCESS_AUTHENTICATION_FACTOR_DISABLE)disable;
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_authentication_factor_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1,
        &factor->authentication_factor);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    } else {
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Decode a BACnetCredentialAuthenticationFactor structure from an APDU
 * buffer, with context tag.
 * @param apdu [in] The APDU buffer
 * @param apdu_size [in] The size of the APDU buffer
 * @param tag [in] The context tag number
 * @param factor [out] The BACnetCredentialAuthenticationFactor structure to
 * decode
 * @return number of bytes decoded, or negative on error
 */
int bacapp_decode_context_credential_authentication_factor(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag,
    BACNET_CREDENTIAL_AUTHENTICATION_FACTOR *factor)
{
    int len = 0, apdu_len = 0;

    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacapp_decode_credential_authentication_factor(
        &apdu[apdu_len], apdu_size - apdu_len, factor);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
