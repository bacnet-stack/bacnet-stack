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
int bacnet_dailyschedule_context_decode(const uint8_t *apdu,
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
    len = bacnet_time_values_context_decode(apdu, apdu_size,
        tag_number, &day->Time_Values[0], ARRAY_SIZE(day->Time_Values),
        &tv_count);
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
