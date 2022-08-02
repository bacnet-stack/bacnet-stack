/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2015 Nikola Jelic

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdcode.h"
#include "bacnet/bactimevalue.h"

int bacapp_encode_time_value(uint8_t *apdu, BACNET_TIME_VALUE *value)
{
    int len;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    len = encode_application_time(apdu_offset, &value->Time);
    apdu_len += len;

    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    len = bacapp_encode_application_data(apdu_offset, &value->Value);
    apdu_len += len;

    return apdu_len;
}

int bacapp_encode_context_time_value(
    uint8_t *apdu, uint8_t tag_number, BACNET_TIME_VALUE *value)
{
    int len;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    len = encode_opening_tag(apdu_offset, tag_number);
    apdu_len += len;

    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    len = bacapp_encode_time_value(apdu_offset, value);
    apdu_len += len;

    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    len = encode_closing_tag(apdu_offset, tag_number);
    apdu_len += len;

    return apdu_len;
}

// TODO max_apdu_len
int bacapp_decode_time_value(uint8_t *apdu, BACNET_TIME_VALUE *value)
{
    int len;
    int apdu_len = 0;

    len = decode_application_time(&apdu[apdu_len], &value->Time);
    if (len <= 0) {
        return -1;
    }
    apdu_len += len;

    len = bacapp_decode_application_data(&apdu[apdu_len], 2048, &value->Value);
    if (len <= 0) {
        return -1;
    }
    apdu_len += len;

    return apdu_len;
}

// TODO max_apdu_len
int bacapp_decode_context_time_value(
    uint8_t *apdu, uint8_t tag_number, BACNET_TIME_VALUE *value)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
    } else {
        return -1;
    }

    section_length = bacapp_decode_time_value(&apdu[len], value);
    if (section_length > 0) {
        len += section_length;
    } else {
        return -1;
    }

    if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
        len++;
    } else {
        return -1;
    }

    return len;
}

int bacapp_decode_context_time_values(
    uint8_t * apdu,
    int max_apdu_len,
    uint8_t tag_number,
    BACNET_TIME_VALUE *time_values,
    unsigned int max_time_values)
{
    unsigned int j;
    int len = 0;
    int apdu_len = 0;
    unsigned int count_values = 0;
    BACNET_TIME_VALUE dummy;

    /* day-schedule [0] SEQUENCE OF BACnetTimeValue */
    if (decode_is_opening_tag_number(&apdu[apdu_len], tag_number)) {
        apdu_len++;
        while ((apdu_len < max_apdu_len) &&
            !decode_is_closing_tag_number(&apdu[apdu_len], tag_number)) {
            if (count_values < max_time_values)
                len =
                    bacapp_decode_time_value(&apdu[apdu_len], /*max_apdu_len - apdu_len,*/
                        &time_values[count_values++]);
            else
                len =
                    bacapp_decode_time_value(&apdu[apdu_len], /*max_apdu_len - apdu_len,*/
                        &dummy);
            if (len < 0)
                return -1;
            apdu_len += len;
        }
        /* Zeroing other values */
        for (j = count_values; j < max_time_values; j++) {
            time_values[j].Value.tag = BACNET_APPLICATION_TAG_NULL;
            time_values[j].Value.type.Unsigned_Int = 0;
            time_values[j].Time.hour = 0;
            time_values[j].Time.min = 0;
            time_values[j].Time.sec = 0;
            time_values[j].Time.hundredths = 0;
        }
        /* overflow ! */
        if (apdu_len >= max_apdu_len)
            return -1;
        apdu_len++;     /* closing tag */
        return apdu_len;
    }
    return -1;
}

/* Encodes a : [x] SEQUENCE OF BACnetTimeValue into a fixed-size buffer */
int bacapp_encode_context_time_values(
    uint8_t * apdu,
    uint8_t tag_number,
    BACNET_TIME_VALUE *time_values,
    unsigned int max_time_values)
{
    unsigned int j;
    int apdu_len = 0;
    int len = 0;
    BACNET_TIME t0 = { 0 };
    uint8_t *apdu_offset = NULL;

    /* day-schedule [x] SEQUENCE OF BACnetTimeValue */
    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    apdu_len += encode_opening_tag(apdu_offset, tag_number);

    for (j = 0; j < max_time_values; j++)
        /* Encode only non-null values (NULL,00:00:00.00) */
        if (time_values[j].Value.tag != BACNET_APPLICATION_TAG_NULL ||
            datetime_compare_time(&t0, &time_values[j].Time) != 0) {
            if (apdu) {
                apdu_offset = &apdu[apdu_len];
            }
            len =
                bacapp_encode_time_value(apdu_offset,
                &time_values[j]);
            if (len < 0)
                return -1;
            apdu_len += len;
        }
    /* close tag */
    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    apdu_len += encode_closing_tag(apdu_offset, tag_number);
    return apdu_len;
}
