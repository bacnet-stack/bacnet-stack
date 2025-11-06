/**
 * @file
 * @brief BACnet BACnetAuthenticationFactorFormat codecs
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include "bacnet/bacdcode.h"
#include "bacnet/authentication_factor_format.h"

/**
 * @brief Encode BACnetAuthenticationFactorFormat data
 * @param apdu - buffer to store the encoding, or NULL for length
 * @param data - data to use for the encoding values
 * @return number of bytes in the buffer
 */
int bacapp_encode_authentication_factor_format(
    uint8_t *apdu, const BACNET_AUTHENTICATION_FACTOR_FORMAT *data)
{
    int len;
    int apdu_len = 0;

    len = encode_context_enumerated(apdu, 0, data->format_type);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (data->format_type == AUTHENTICATION_FACTOR_CUSTOM) {
        /*  optional fields are required when Format-Type
            field has a value of CUSTOM. */
        len = encode_context_unsigned(apdu, 1, data->vendor_id);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_context_unsigned(apdu, 2, data->vendor_format);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Context encode BACnetAuthenticationFactorFormat data
 * @param apdu - buffer to store the encoding, or NULL for length
 * @param tag - tag number used to encapsulate the data within open/close tags
 * @param data - data to use for the encoding values
 * @return number of bytes in the buffer
 */
int bacapp_encode_context_authentication_factor_format(
    uint8_t *apdu, uint8_t tag, const BACNET_AUTHENTICATION_FACTOR_FORMAT *data)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(apdu, tag);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacapp_encode_authentication_factor_format(apdu, data);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, tag);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a BACnetAuthenticationFactorFormat property value
 * @details BACnetAuthenticationFactorFormat ::= SEQUENCE {
 *      format-type[0] BACnetAuthenticationFactorType,
 *      vendor-id[1] Unsigned16 OPTIONAL,
 *      vendor-format[2] Unsigned16 OPTIONAL
 *  }
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Count of valid bytes in the buffer.
 * @param data  Pointer to the property decoded data to be stored
 *  or NULL for length
 * @return number of bytes decoded or BACNET_STATUS_ERROR on error.
 */
int bacnet_authentication_factor_format_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_AUTHENTICATION_FACTOR_FORMAT *data)
{
    int len;
    int apdu_len = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_AUTHENTICATION_FACTOR_TYPE format_type =
        AUTHENTICATION_FACTOR_UNDEFINED;

    /* format-type[0] BACnetAuthenticationFactorType */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &enum_value);
    if (len > 0) {
        apdu_len += len;
        if (enum_value > AUTHENTICATION_FACTOR_MAX) {
            enum_value = AUTHENTICATION_FACTOR_MAX;
        }
        format_type = (BACNET_AUTHENTICATION_FACTOR_TYPE)enum_value;
        if (data) {
            data->format_type = format_type;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (format_type == AUTHENTICATION_FACTOR_CUSTOM) {
        /*  optional fields are required when Format-Type
            field has a value of CUSTOM. */
        /* vendor-id[1] Unsigned16 OPTIONAL */
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (unsigned_value > UINT16_MAX) {
                return BACNET_STATUS_ERROR;
            }
            if (data) {
                data->vendor_id = (uint16_t)unsigned_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* vendor-format[2] Unsigned16 OPTIONAL */
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (unsigned_value > UINT16_MAX) {
                return BACNET_STATUS_ERROR;
            }
            if (data) {
                data->vendor_format = (uint16_t)unsigned_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}

/**
 * @brief Decode a BACnetAuthenticationFactorFormat property value
 * @param apdu Pointer to the buffer for decoding.
 * @param apdu_size Count of valid bytes in the buffer.
 * @param data Pointer to the property decoded data to be stored
 *  or NULL for length
 * @return Number of bytes decoded or BACNET_STATUS_ERROR on error.
 * @deprecated Use bacnet_authentication_factor_format_decode() instead
 */
int bacapp_decode_authentication_factor_format(
    const uint8_t *apdu, BACNET_AUTHENTICATION_FACTOR_FORMAT *data)
{
    return bacnet_authentication_factor_format_decode(apdu, MAX_APDU, data);
}

/**
 * @brief Decode a context tagged BACnetAuthenticationFactorFormat property
 *  value
 * @param apdu  Pointer to the buffer for decoding.
 * @param tag - tag number used to encapsulate the data within open/close tags
 * @param data  Pointer to the property decoded data to be stored
 *  or NULL for length
 * @return number of bytes decoded or BACNET_STATUS_ERROR on error.
 */
int bacnet_authentication_factor_format_context_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag,
    BACNET_AUTHENTICATION_FACTOR_FORMAT *data)
{
    int len = 0, apdu_len = 0;

    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag, &len)) {
        apdu_len += len;
        len = bacnet_authentication_factor_format_decode(
            &apdu[apdu_len], apdu_size - apdu_len, data);
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
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Decode a context tagged BACnetAuthenticationFactorFormat property
 *  value
 * @param apdu  Pointer to the buffer for decoding.
 * @param tag - tag number used to encapsulate the data within open/close
 * tags
 * @param data  Pointer to the property decoded data to be stored
 *  or NULL for length
 * @return number of bytes decoded or BACNET_STATUS_ERROR on error.
 * @deprecated use bacnet_authentication_factor_format_context_decode()
 * instead
 */
int bacapp_decode_context_authentication_factor_format(
    const uint8_t *apdu, uint8_t tag, BACNET_AUTHENTICATION_FACTOR_FORMAT *data)
{
    return bacnet_authentication_factor_format_context_decode(
        apdu, MAX_APDU, tag, data);
}
