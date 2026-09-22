/**
 * @file
 * @brief Stub functions for unit test of a BACnet object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/datetime.h"

static BACNET_DATE BACnet_Date;
static BACNET_TIME BACnet_Time;
static int16_t UTC_Offset_Minutes;
static bool DST_Active;

bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    BACNET_DATE_TIME bdatetime = { 0 };

    bdatetime.date = BACnet_Date;
    bdatetime.time = BACnet_Time;
    if (datetime_wildcard_present(&bdatetime)) {
        return false;
    }
    if (bdate) {
        datetime_copy_date(bdate, &BACnet_Date);
    }
    if (btime) {
        datetime_copy_time(btime, &BACnet_Time);
    }
    if (utc_offset_minutes) {
        *utc_offset_minutes = UTC_Offset_Minutes;
    }
    if (dst_active) {
        *dst_active = DST_Active;
    }

    return true;
}

void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
{
    if (bdate) {
        datetime_copy_date(&BACnet_Date, bdate);
    } else {
        datetime_date_wildcard_set(&BACnet_Date);
    }
    if (btime) {
        datetime_copy_time(&BACnet_Time, btime);
    } else {
        datetime_time_wildcard_set(&BACnet_Time);
    }
    (void)utc;
}

/**
 * @brief Set the UTC offset in minutes
 * @param minutes [in] The UTC offset in minutes
 * @return true if successful, false on error
 * @note BACnet UTC Offset is inverse of common practice.
 * If your UTC offset is -5hours of GMT,
 * then BACnet UTC offset is +5hours.
 * BACnet UTC offset is expressed in minutes.
 */
bool datetime_utc_offset_minutes_set(int16_t minutes)
{
    UTC_Offset_Minutes = minutes;

    return true;
}

void datetime_init(void)
{
    /* nothing to do */
}
