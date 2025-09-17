/**
 * @file
 * @brief BACnetWeeklySchedule complex data type encode and decode
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date September 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include "bacnet/weeklyschedule.h"
#include "bacnet/bacdcode.h"
#include "bacapp.h"

/**
 * @brief decode a BACnetWeeklySchedule
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - stores the decoded property value
 * @return  number of bytes decoded, or BACNET_STATUS_ERROR if errors occur
 */
int bacnet_weeklyschedule_decode(
    const uint8_t *apdu, int apdu_size, BACNET_WEEKLY_SCHEDULE *value)
{
    int len = 0;
    int apdu_len = 0;
    int wi;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size <= 0) {
        return BACNET_STATUS_ERROR;
    }
    value->singleDay = false;
    for (wi = 0; wi < BACNET_WEEKLY_SCHEDULE_SIZE; wi++) {
        len = bacnet_dailyschedule_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 0,
            &value->weeklySchedule[wi]);
        if (len < 0) {
            if (wi == 1) {
                value->singleDay = true;
                break;
            }
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the Weekly_Schedule property
 *  from clause 12 Schedule Object Type
 *  BACnetARRAY[7] of BACnetDailySchedule
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param value - value to be encoded
 * @return the number of apdu bytes encoded, or BACNET_STATUS_ERROR if value
 * was inconsistent.
 */
int bacnet_weeklyschedule_encode(
    uint8_t *apdu, const BACNET_WEEKLY_SCHEDULE *value)
{
    int apdu_len = 0;
    int len = 0;
    int wi;

    for (wi = 0; wi < (value->singleDay ? 1 : BACNET_WEEKLY_SCHEDULE_SIZE);
         wi++) {
        len = bacnet_dailyschedule_context_encode(
            apdu, 0, &value->weeklySchedule[wi]);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }

    return apdu_len;
}

/**
 * @brief Encode a context tagged Weekly_Schedule complex data type
 * @param apdu - the APDU buffer, or NULL for buffer length
 * @param tag_number - the APDU buffer size
 * @param value - WeeklySchedule structure
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int bacnet_weeklyschedule_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_WEEKLY_SCHEDULE *value)
{
    int len = 0;
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = bacnet_weeklyschedule_encode(apdu, value);
        if (len == BACNET_STATUS_ERROR) {
            return 0;
        }
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief decode a context encoded Weekly_Schedule property
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param tag_number - context tag number to match
 * @param value - stores the decoded property value
 * @return number of bytes decoded, or BACNET_STATUS_ERROR if an error occurs
 */
int bacnet_weeklyschedule_context_decode(
    const uint8_t *apdu,
    int apdu_size,
    uint8_t tag_number,
    BACNET_WEEKLY_SCHEDULE *value)
{
    int apdu_len = 0;
    int len;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_weeklyschedule_decode(
        &apdu[apdu_len], apdu_size - apdu_len, value);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare the BACnetWeeklySchedule complex data
 * @param value1 - BACNET_WEEKLY_SCHEDULE structure
 * @param value2 - BACNET_WEEKLY_SCHEDULE structure
 * @return true if the same
 */
bool bacnet_weeklyschedule_same(
    const BACNET_WEEKLY_SCHEDULE *value1, const BACNET_WEEKLY_SCHEDULE *value2)
{
    BACNET_APPLICATION_DATA_VALUE adv1 = { 0 }, adv2 = { 0 };
    const BACNET_DAILY_SCHEDULE *ds1, *ds2;
    const BACNET_TIME_VALUE *tv1, *tv2;
    int wi, ti;

    for (wi = 0; wi < BACNET_WEEKLY_SCHEDULE_SIZE; wi++) {
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
