/**************************************************************************
*
* Copyright (C) 2015 Nikola Jelic <nikola.jelic@euroicc.com>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "access_rule.h"
#include "bacdcode.h"

int bacapp_encode_access_rule(
    uint8_t * apdu,
    BACNET_ACCESS_RULE * rule)
{
    int len;
    int apdu_len = 0;

    len = encode_context_enumerated(&apdu[0], 0, rule->time_range_specifier);
    apdu_len += len;

    if (rule->time_range_specifier == TIME_RANGE_SPECIFIER_SPECIFIED) {
        len =
            bacapp_encode_context_device_obj_property_ref(&apdu[apdu_len], 1,
            &rule->time_range);
        if (len > 0)
            apdu_len += len;
        else
            return -1;
    }

    len =
        encode_context_enumerated(&apdu[apdu_len], 2,
        rule->location_specifier);
    apdu_len += len;

    if (rule->location_specifier == LOCATION_SPECIFIER_SPECIFIED) {
        len =
            bacapp_encode_context_device_obj_property_ref(&apdu[apdu_len], 3,
            &rule->location);
        if (len > 0)
            apdu_len += len;
        else
            return -1;
    }

    len = encode_context_boolean(&apdu[apdu_len], 4, rule->enable);
    apdu_len += len;

    return apdu_len;
}

int bacapp_encode_context_access_rule(
    uint8_t * apdu,
    uint8_t tag_number,
    BACNET_ACCESS_RULE * rule)
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

int bacapp_decode_access_rule(
    uint8_t * apdu,
    BACNET_ACCESS_RULE * rule)
{
    int len;
    int apdu_len = 0;

    if (decode_is_context_tag(&apdu[apdu_len], 0)) {
        len =
            decode_context_enumerated(&apdu[apdu_len], 0,
            &rule->time_range_specifier);
        if (len < 0)
            return -1;
        else
            apdu_len += len;
    } else
        return -1;

    if (rule->time_range_specifier == TIME_RANGE_SPECIFIER_SPECIFIED) {
        if (decode_is_context_tag(&apdu[apdu_len], 1)) {
            len =
                bacapp_decode_context_device_obj_property_ref(&apdu[apdu_len],
                1, &rule->time_range);
            if (len < 0)
                return -1;
            else
                apdu_len += len;
        } else
            return -1;
    }

    if (decode_is_context_tag(&apdu[apdu_len], 2)) {
        len =
            decode_context_enumerated(&apdu[apdu_len], 2,
            &rule->location_specifier);
        if (len < 0)
            return -1;
        else
            apdu_len += len;
    } else
        return -1;

    if (rule->location_specifier == LOCATION_SPECIFIER_SPECIFIED) {
        if (decode_is_context_tag(&apdu[apdu_len], 3)) {
            len =
                bacapp_decode_context_device_obj_property_ref(&apdu[apdu_len],
                3, &rule->location);
            if (len < 0)
                return -1;
            else
                apdu_len += len;
        } else
            return -1;
    }

    if (decode_is_context_tag(&apdu[apdu_len], 4)) {
        len = decode_context_boolean2(&apdu[apdu_len], 4, &rule->enable);
        if (len < 0)
            return -1;
        else
            apdu_len += len;
    } else
        return -1;

    return apdu_len;
}

int bacapp_decode_context_access_rule(
    uint8_t * apdu,
    uint8_t tag_number,
    BACNET_ACCESS_RULE * rule)
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
