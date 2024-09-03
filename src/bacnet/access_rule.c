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
 * @brief Parse a string into a BACnet Shed Level value
 * @param value [out] The BACnet Shed Level value
 * @param argv [in] The string to parse
 * @return True on success, else False
 */
bool bacnet_shed_level_from_ascii(BACNET_ACCESS_RULE *value, const char *argv)
{
    bool status = false;
    int count;
    unsigned percent, level;
    float amount;
    const char *percentage;
    const char *decimal_point;

    if (!status) {
        percentage = strchr(argv, '%');
        if (percentage) {
            count = sscanf(argv, "%u", &percent);
            if (count == 1) {
                value->type = BACNET_SHED_TYPE_PERCENT;
                value->value.percent = percent;
                status = true;
            }
        }
    }
    if (!status) {
        decimal_point = strchr(argv, '.');
        if (decimal_point) {
            count = sscanf(argv, "%f", &amount);
            if (count == 1) {
                value->type = BACNET_SHED_TYPE_AMOUNT;
                value->value.amount = amount;
                status = true;
            }
        }
    }
    if (!status) {
        count = sscanf(argv, "%u", &level);
        if (count == 1) {
            value->type = BACNET_SHED_TYPE_LEVEL;
            value->value.level = level;
            status = true;
        }
    }

    return status;
}

bool bacnet_shed_level_same(
    const BACNET_ACCESS_RULE *value1, const BACNET_ACCESS_RULE *value2)
{
    return false;
}
