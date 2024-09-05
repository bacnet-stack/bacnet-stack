/**
 * @file
 * @brief BACnetAccessRule service encode and decode
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include "bacnet/access_rule.h"
#include "bacnet/bacdcode.h"

/**
 * @brief Encode the BACnetAccessRule
 *
 *  BACnetAccessRule ::= SEQUENCE {
 *      time-range-specifier [0] ENUMERATED {
 *          specified (0),
 *          always (1)
 *      },
 *      time-range[1] BACnetDeviceObjectPropertyReference OPTIONAL,
 *      -- to be present if time-range-specifier has the value "specified"
 *      location-specifier [2] ENUMERATED {
 *          specified (0),
 *          all (1)
 *      },
 *      location [3] BACnetDeviceObjectReference OPTIONAL,
 *      -- to be present if location-specifier has the value "specified"
 *      enable [4] BOOLEAN
 *   }
 *
 * @param apdu  Pointer to the buffer for encoding, or NULL for length
 * @param rule  Pointer to the data to be encoded
 * @return number of bytes encoded
 */
int bacapp_encode_access_rule(uint8_t *apdu, const BACNET_ACCESS_RULE *rule)
{
    int len;
    int apdu_len = 0;

    len = encode_context_enumerated(apdu, 0, rule->time_range_specifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (rule->time_range_specifier == TIME_RANGE_SPECIFIER_SPECIFIED) {
        len = bacapp_encode_context_device_obj_property_ref(
            apdu, 1, &rule->time_range);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    len = encode_context_enumerated(apdu, 2, rule->location_specifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (rule->location_specifier == LOCATION_SPECIFIER_SPECIFIED) {
        len = bacapp_encode_context_device_obj_ref(apdu, 3, &rule->location);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    len = encode_context_boolean(apdu, 4, rule->enable);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the BACnetAccessRule as Context Tagged
 * @param apdu  Pointer to the buffer for encoding, or NULL for length
 * @param tag_number  Tag number
 * @param rule  Pointer to the data to be encoded
 * @return number of bytes encoded
 */
int bacapp_encode_context_access_rule(
    uint8_t *apdu, uint8_t tag_number, const BACNET_ACCESS_RULE *rule)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(apdu, tag_number);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacapp_encode_access_rule(apdu, rule);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, tag_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the BACnetAccessRule
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  The size of the buffer for decoding.
 * @param data  Pointer to the data to be stored
 * @return number of bytes decoded or BACNET_STATUS_ERROR on error
 */
int bacnet_access_rule_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_ACCESS_RULE *data)
{
    int len;
    int apdu_len = 0;
    uint32_t enumerated_value = 0, time_range_specifier = 0,
             location_specifier = 0;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *reference = NULL;
    BACNET_DEVICE_OBJECT_REFERENCE *location_reference = NULL;
    bool boolean_value = false;

    /* time-range-specifier [0] ENUMERATED {
           specified (0),
           always (1)
       }
    */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &enumerated_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    } else if (enumerated_value < TIME_RANGE_SPECIFIER_MAX) {
        time_range_specifier = enumerated_value;
        if (data) {
            data->time_range_specifier =
                (BACNET_ACCESS_RULE_TIME_RANGE_SPECIFIER)time_range_specifier;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    /* time-range[1] BACnetDeviceObjectPropertyReference OPTIONAL,
       -- to be present if time-range-specifier has the value "specified" */
    if (time_range_specifier == TIME_RANGE_SPECIFIER_SPECIFIED) {
        if (data) {
            reference = &data->time_range;
        }
        len = bacnet_device_object_property_reference_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 1, reference);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    }
    /* time-range-specifier [0] ENUMERATED {
           specified (0),
           always (1)
       }
    */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &enumerated_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    } else if (enumerated_value < LOCATION_SPECIFIER_MAX) {
        location_specifier = enumerated_value;
        if (data) {
            data->location_specifier =
                (BACNET_ACCESS_RULE_LOCATION_SPECIFIER)location_specifier;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    /* location [3] BACnetDeviceObjectReference OPTIONAL,
       -- to be present if location-specifier has the value "specified" */
    if (location_specifier == LOCATION_SPECIFIER_SPECIFIED) {
        if (data) {
            location_reference = &data->location;
        }
        len = bacnet_device_object_reference_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 3, location_reference);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    }
    /* enable [4] BOOLEAN */
    len = bacnet_boolean_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 4, &boolean_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        data->enable = boolean_value;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the BACnetAccessRule
 * @param apdu  Pointer to the buffer for decoding.
 * @param data  Pointer to the data to be stored
 * @return number of bytes decoded
 * @deprecated Use bacapp_access_rule_decode() instead
 */
int bacapp_decode_access_rule(const uint8_t *apdu, BACNET_ACCESS_RULE *data)
{
    return bacnet_access_rule_decode(apdu, MAX_APDU, data);
}

/**
 * @brief Decode the BACnetAccessRule as Context Tagged
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size The size of the buffer for decoding.
 * @param tag_number  Tag number
 * @param data  Pointer to the data to be stored
 * @return number of bytes decoded or BACNET_STATUS_ERROR on error
 */
int bacnet_access_rule_context_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_number,
    BACNET_ACCESS_RULE *data)
{
    int len = 0;
    int apdu_len = 0;

    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    len =
        bacnet_access_rule_decode(&apdu[apdu_len], apdu_size - apdu_len, data);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the BACnetAccessRule as Context Tagged
 * @param apdu  Pointer to the buffer for decoding.
 * @param tag_number  Tag number
 * @param data  Pointer to the data to be stored
 * @return number of bytes decoded or BACNET_STATUS_ERROR on error
 * @deprecated Use bacnet_access_rule_context_decode() instead
 */
int bacapp_decode_context_access_rule(
    const uint8_t *apdu, uint8_t tag_number, BACNET_ACCESS_RULE *data)
{
    return bacnet_access_rule_context_decode(apdu, MAX_APDU, tag_number, data);
}
