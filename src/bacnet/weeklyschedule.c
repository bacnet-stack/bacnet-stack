/*####COPYRIGHTBEGIN####
-------------------------------------------
Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>

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

#include <stdint.h>
#include "bacnet/weeklyschedule.h"
#include "bacnet/bacdcode.h"
#include "bacapp.h"

int bacnet_weeklyschedule_decode(
    uint8_t *apdu, int max_apdu_len, BACNET_WEEKLY_SCHEDULE *value)
{
    int len = 0;
    int apdu_len = 0;
    int wi;

    value->singleDay = false;
    for (wi = 0; wi < 7; wi++) {
        len = bacnet_dailyschedule_decode(&apdu[apdu_len],
            max_apdu_len - apdu_len, &value->weeklySchedule[wi]);
        if (len < 0) {
            if (wi == 1) {
                value->singleDay = true;
                break;
            }
            return -1;
        }
        apdu_len += len;
    }
    return apdu_len;
}

int bacnet_weeklyschedule_encode(uint8_t *apdu, BACNET_WEEKLY_SCHEDULE *value)
{
    int apdu_len = 0;
    int len = 0;
    int wi;
    uint8_t *apdu_offset = NULL;

    for (wi = 0; wi < (value->singleDay ? 1 : 7); wi++) {
        /* not enough room to write data */
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = bacnet_dailyschedule_encode(
            apdu_offset, &value->weeklySchedule[wi]);
        if (len < 0)
            return -1;
        apdu_len += len;
    }
    return apdu_len;
}

/**
 * @brief Encode a context tagged WeeklySchedule complex data type
 * @param apdu - the APDU buffer
 * @param tag_number - the APDU buffer size
 * @param value - WeeklySchedule structure
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int bacnet_weeklyschedule_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_WEEKLY_SCHEDULE *value)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    if (value) {
        apdu_offset = apdu;
        len = encode_opening_tag(apdu_offset, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = bacnet_weeklyschedule_encode(apdu_offset, value);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_closing_tag(apdu_offset, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

int bacnet_weeklyschedule_context_decode(uint8_t *apdu,
    int max_apdu_len,
    uint8_t tag_number,
    BACNET_WEEKLY_SCHEDULE *value)
{
    int apdu_len = 0;
    int len;

    if ((max_apdu_len - apdu_len) >= 1 &&
        decode_is_opening_tag_number(&apdu[apdu_len], tag_number)) {
        apdu_len += 1;
    } else {
        return -1;
    }

    if (-1 ==
        (len = bacnet_weeklyschedule_decode(
             &apdu[apdu_len], max_apdu_len - apdu_len, value))) {
        return -1;
    } else {
        apdu_len += len;
    }

    if ((max_apdu_len - apdu_len) >= 1 &&
        decode_is_closing_tag_number(&apdu[apdu_len], tag_number)) {
        apdu_len += 1;
    } else {
        return -1;
    }
    return apdu_len;
}

/**
 * @brief Compare the BACnetWeeklySchedule complex data
 * @param value1 - BACNET_COLOR_COMMAND structure
 * @param value2 - BACNET_COLOR_COMMAND structure
 * @return true if the same
 */
bool bacnet_weeklyschedule_same(
    BACNET_WEEKLY_SCHEDULE *value1, BACNET_WEEKLY_SCHEDULE *value2)
{
    BACNET_APPLICATION_DATA_VALUE adv1, adv2;
    BACNET_DAILY_SCHEDULE *ds1, *ds2;
    BACNET_TIME_VALUE *tv1, *tv2;
    int wi, ti;

    for (wi = 0; wi < 7; wi++) {
        ds1 = &value1->weeklySchedule[wi];
        ds2 = &value2->weeklySchedule[wi];
        if (ds1->TV_Count != ds2->TV_Count) {
            return false;
        }
        for (ti = 0; ti < ds1->TV_Count; ti++) {
            tv1 = &ds1->Time_Values[ti];
            tv2 = &ds2->Time_Values[ti];
            if (0 != datetime_compare_time(&tv1->Time, &tv2->Time)) {
                return false;
            }

            /* TODO the conversion can be avoided by adding a "primitive"
               variant of bacapp_same_value(),
              at the cost of some code duplication */
            bacnet_primitive_to_application_data_value(&adv1, &tv1->Value);
            bacnet_primitive_to_application_data_value(&adv2, &tv2->Value);

            if (!bacapp_same_value(&adv1, &adv2)) {
                return false;
            }
        }
    }

    return true;
}
