/**
 * @file
 * @brief BACnetDailySchedule complex data type encode and decode
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2024
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include "bacnet/dailyschedule.h"
#include "bacnet/bactimevalue.h"

/**
 * @brief Encode a BACnetDailySchedule value to a buffer
 * @param apdu [out] Buffer to encode to
 * @param apdu_size [in] Size of the buffer
 * @param tag_number [in] Tag number to use
 * @param day [in] Value to encode
 * @return Number of bytes encoded, or  BACNET_STATUS_ERROR if an error occurs
 */
int bacnet_dailyschedule_context_decode(
    const uint8_t *apdu,
    int apdu_size,
    uint8_t tag_number,
    BACNET_DAILY_SCHEDULE *day)
{
    unsigned int tv_count = 0;
    int len = 0;

    if (day == NULL) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu == NULL) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_time_values_context_decode(
        apdu, apdu_size, tag_number, &day->Time_Values[0],
        ARRAY_SIZE(day->Time_Values), &tv_count);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    day->TV_Count = (uint16_t)tv_count;

    return len;
}

/**
 * @brief Encode a BACnetDailySchedule value to a buffer
 * @param apdu [out] Buffer to encode to
 * @param tag_number [in] Tag number to use
 * @param day [in] Value to encode
 * @return Number of bytes encoded, or BACNET_STATUS_ERROR if an error occurs
 */
int bacnet_dailyschedule_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_DAILY_SCHEDULE *day)
{
    if (day == NULL) {
        return BACNET_STATUS_ERROR;
    }
    if (day->TV_Count > ARRAY_SIZE(day->Time_Values)) {
        return BACNET_STATUS_ERROR;
    }

    return bacnet_time_values_context_encode(
        apdu, tag_number, &day->Time_Values[0], day->TV_Count);
}

/**
 * @brief Compare two BACnetDailySchedule values
 * @param a [in] First value to compare
 * @param b [in] Second value to compare
 * @return true if the values are the same, false otherwise
 * @note If either value is NULL, false is returned.
 */
bool bacnet_dailyschedule_same(
    const BACNET_DAILY_SCHEDULE *a, const BACNET_DAILY_SCHEDULE *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }
    if (a->TV_Count != b->TV_Count) {
        return false;
    }
    for (unsigned i = 0; i < a->TV_Count; i++) {
        if (!bacnet_time_value_same(&a->Time_Values[i], &b->Time_Values[i])) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Copy a BACnetDailySchedule value
 * @param dest [out] Destination to copy to
 * @param src [in] Source to copy from
 */
void bacnet_dailyschedule_copy(
    BACNET_DAILY_SCHEDULE *dest, const BACNET_DAILY_SCHEDULE *src)
{
    if (dest == NULL || src == NULL) {
        return;
    }
    if (src->TV_Count > ARRAY_SIZE(dest->Time_Values)) {
        return;
    }
    dest->TV_Count = src->TV_Count;
    for (unsigned i = 0; i < dest->TV_Count; i++) {
        bacnet_time_value_copy(&dest->Time_Values[i], &src->Time_Values[i]);
    }
}
