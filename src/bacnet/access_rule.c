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

int bacapp_encode_access_rule(uint8_t *apdu, const BACNET_ACCESS_RULE *rule)
{
    int len;
    int apdu_len = 0;

    len = encode_context_enumerated(&apdu[0], 0, rule->time_range_specifier);
    apdu_len += len;

    if (rule->time_range_specifier == TIME_RANGE_SPECIFIER_SPECIFIED) {
        len = bacapp_encode_context_device_obj_property_ref(
            &apdu[apdu_len], 1, &rule->time_range);
        if (len > 0) {
            apdu_len += len;
        } else {
            return -1;
        }
    }

    len =
        encode_context_enumerated(&apdu[apdu_len], 2, rule->location_specifier);
    apdu_len += len;

    if (rule->location_specifier == LOCATION_SPECIFIER_SPECIFIED) {
        len = bacapp_encode_context_device_obj_property_ref(
            &apdu[apdu_len], 3, &rule->location);
        if (len > 0) {
            apdu_len += len;
        } else {
            return -1;
        }
    }

    len = encode_context_boolean(&apdu[apdu_len], 4, rule->enable);
    apdu_len += len;

    return apdu_len;
}

int bacapp_encode_context_access_rule(
    uint8_t *apdu, uint8_t tag_number, const BACNET_ACCESS_RULE *rule)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(&apdu[apdu_len], tag_number);
    apdu_len += len;

    len = bacapp_encode_access_rule(&apdu[apdu_len], rule);
    apdu_len += len;

    len = encode_closing_tag(&apdu[apdu_len], tag_number);
    apdu_len += len;

    return apdu_len;
}

int bacapp_decode_access_rule(const uint8_t *apdu, BACNET_ACCESS_RULE *rule)
{
    int len;
    int apdu_len = 0;
    uint32_t time_range_specifier = rule->time_range_specifier;
    uint32_t location_specifier = rule->location_specifier;

    if (decode_is_context_tag(&apdu[apdu_len], 0)) {
        len = decode_context_enumerated(
            &apdu[apdu_len], 0, &time_range_specifier);
        if (len < 0) {
            return -1;
        } else if (time_range_specifier < TIME_RANGE_SPECIFIER_MAX) {
            apdu_len += len;
            rule->time_range_specifier =
                (BACNET_ACCESS_RULE_TIME_RANGE_SPECIFIER)time_range_specifier;
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    if (rule->time_range_specifier == TIME_RANGE_SPECIFIER_SPECIFIED) {
        if (decode_is_context_tag(&apdu[apdu_len], 1)) {
            len = bacapp_decode_context_device_obj_property_ref(
                &apdu[apdu_len], 1, &rule->time_range);
            if (len < 0) {
                return -1;
            } else {
                apdu_len += len;
            }
        } else {
            return -1;
        }
    }

    if (decode_is_context_tag(&apdu[apdu_len], 2)) {
        len =
            decode_context_enumerated(&apdu[apdu_len], 2, &location_specifier);
        if (len < 0) {
            return -1;
        } else if (location_specifier < LOCATION_SPECIFIER_MAX) {
            apdu_len += len;
            rule->location_specifier =
                (BACNET_ACCESS_RULE_LOCATION_SPECIFIER)location_specifier;
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    if (rule->location_specifier == LOCATION_SPECIFIER_SPECIFIED) {
        if (decode_is_context_tag(&apdu[apdu_len], 3)) {
            len = bacapp_decode_context_device_obj_property_ref(
                &apdu[apdu_len], 3, &rule->location);
            if (len < 0) {
                return -1;
            } else {
                apdu_len += len;
            }
        } else {
            return -1;
        }
    }

    if (decode_is_context_tag(&apdu[apdu_len], 4)) {
        len = decode_context_boolean2(&apdu[apdu_len], 4, &rule->enable);
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

int bacapp_decode_context_access_rule(
    const uint8_t *apdu, uint8_t tag_number, BACNET_ACCESS_RULE *rule)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_length = bacapp_decode_access_rule(&apdu[len], rule);

        if (section_length == -1) {
            len = -1;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
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
