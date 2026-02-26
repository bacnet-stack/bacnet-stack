/**
 * @file
 * @brief BACnet BACnetAuthenticationFactor structure and codecs
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include "bacnet/authentication_factor.h"
#include "bacnet/bacdcode.h"

/**
 * @brief Encode the BACnetAuthenticationFactor complex data
 * @param apdu  Pointer to the buffer for encoding into, or NULL for length
 * @param data  Pointer used for encoding the value
 * @return number of bytes encoded, or zero if unable to encode
 */
int bacapp_encode_authentication_factor(
    uint8_t *apdu, const BACNET_AUTHENTICATION_FACTOR *af)
{
    int len;
    int apdu_len = 0;

    len = encode_context_enumerated(apdu, 0, af->format_type);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_unsigned(apdu, 1, af->format_class);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_octet_string(apdu, 2, &af->value);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the BACnetAuthenticationFactor context tagged complex data
 * @param apdu  Pointer to the buffer for encoding into, or NULL for length
 * @param data  Pointer used for encoding the value
 * @return number of bytes encoded, or zero if unable to encode
 */
int bacapp_encode_context_authentication_factor(
    uint8_t *apdu, uint8_t tag, const BACNET_AUTHENTICATION_FACTOR *af)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(apdu, tag);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacapp_encode_authentication_factor(apdu, af);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, tag);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the BACnetAuthenticationFactor complex data
 * @details
 *  BACnetAuthenticationFactor ::= SEQUENCE {
 *      format-type[0] BACnetAuthenticationFactorType,
 *      format-class[1] Unsigned,
 *      value[2] OctetString
 *      -- for encoding of values into this octet string see Annex P.
 *  }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Number of valid bytes in the buffer.
 * @param af  Pointer to the property decoded data to be stored, or NULL for
 *  length of the decoded bytes.
 * @return Number of bytes decoded or BACNET_STATUS_ERROR on error.
 */
int bacnet_authentication_factor_decode(
    const uint8_t *apdu, unsigned apdu_size, BACNET_AUTHENTICATION_FACTOR *af)
{
    int len = 0, apdu_len = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OCTET_STRING *octet_string_value = NULL;

    /* format-type[0] BACnetAuthenticationFactorType */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &enum_value);
    if (len > 0) {
        if (af) {
            if (enum_value > AUTHENTICATION_FACTOR_MAX) {
                enum_value = AUTHENTICATION_FACTOR_MAX;
            }
            af->format_type = (BACNET_AUTHENTICATION_FACTOR_TYPE)enum_value;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* format-class[1] Unsigned */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
    if (len > 0) {
        if (af) {
            af->format_class = unsigned_value;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* value[2] OctetString */
    if (af) {
        octet_string_value = &af->value;
    } else {
        octet_string_value = NULL;
    }
    len = bacnet_octet_string_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, octet_string_value);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

#if defined(BACNET_STACK_DEPRECATED_DISABLE)
/**
 * @brief Decode the BACnetAuthenticationFactor complex data
 * @param apdu  Pointer to the buffer for decoding.
 * @param af  Pointer to the property decoded data to be stored
 * @return Bytes decoded or BACNET_STATUS_REJECT on error.
 * @deprecated Use bacnet_authentication_factor_decode() instead
 */
int bacapp_decode_authentication_factor(
    const uint8_t *apdu, BACNET_AUTHENTICATION_FACTOR *af)
{
    return bacnet_authentication_factor_decode(apdu, MAX_APDU, af);
}
#endif

/**
 * @brief Decode the context tagged BACnetAuthenticationFactor complex data
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Number of valid bytes in the buffer.
 * @param tag  context tag number wrapping the complex data
 * @param af  Pointer to the property decoded data to be stored, or NULL
 *  for the length in bytes decoded
 * @return Number of bytes decoded,
 *  0 if opening tag doesn't match (use to detect OPTIONAL),
 *  or BACNET_STATUS_ERROR on error.
 */
int bacnet_authentication_factor_context_decode(
    const uint8_t *apdu,
    unsigned apdu_size,
    uint8_t tag,
    BACNET_AUTHENTICATION_FACTOR *af)
{
    int len = 0, apdu_len = 0;

    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag, &len)) {
        apdu_len += len;
        len = bacnet_authentication_factor_decode(
            &apdu[apdu_len], apdu_size - apdu_len, af);
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
    } else {
        return 0;
    }

    return apdu_len;
}

#if defined(BACNET_STACK_DEPRECATED_DISABLE)
/**
 * @brief Decode the context tagged BACnetAuthenticationFactor complex data
 * @param apdu  Pointer to the buffer for decoding.
 * @param tag  context tag number wrapping the complex data
 * @param af  Pointer to the property decoded data to be stored
 * @return Number of bytes decoded,
 *  0 if opening tag doesn't match (use to detect OPTIONAL),
 *  or BACNET_STATUS_ERROR on error.
 * @deprecated Use bacnet_authentication_factor_context_decode() instead
 */
int bacapp_decode_context_authentication_factor(
    const uint8_t *apdu, uint8_t tag, BACNET_AUTHENTICATION_FACTOR *af)
{
    return bacnet_authentication_factor_context_decode(apdu, MAX_APDU, tag, af);
}
#endif
