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
static bool DateTime_Valid;
static bool UTC_Offset_Set_Allowed = true;

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

bool datetime_zone(
    BACNET_DATE_TIME *local_time,
    int16_t *utc_offset_minutes,
    int8_t *dst_adjust_minutes)
{
    BACNET_DATE_TIME bdatetime = { 0 };

    bdatetime.date = BACnet_Date;
    bdatetime.time = BACnet_Time;
    if (datetime_wildcard_present(&bdatetime)) {
        return false;
    }
    if (local_time) {
        datetime_copy(local_time, &bdatetime);
    }
    if (utc_offset_minutes) {
        *utc_offset_minutes = UTC_Offset_Minutes;
    }
    if (dst_adjust_minutes) {
        if (DST_Active) {
            *dst_adjust_minutes = -60;
        } else {
            *dst_adjust_minutes = 0;
        }
    }

    return true;
}

void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
{
    BACNET_DATE_TIME current = { 0 };
    BACNET_DATE_TIME standard_time = { 0 };

    datetime_set(&current, &BACnet_Date, &BACnet_Time);
    datetime_copy_date(&BACnet_Date, bdate);
    datetime_copy_time(&BACnet_Time, btime);
    if (!bdate && !btime) {
        datetime_date_wildcard_set(&BACnet_Date);
        datetime_time_wildcard_set(&BACnet_Time);
    }
    (void)utc;
    datetime_set(&standard_time, &BACnet_Date, &BACnet_Time);
    if (!datetime_timesync_in_deadband(&current, &standard_time)) {
        DateTime_Valid = true;
        datetime_change_notify(DATETIME_CHANGE_TIME, &standard_time);
    }
}

void datetime_standard_set(const BACNET_DATE_TIME *standard_time)
{
    if (standard_time) {
        datetime_copy_date(&BACnet_Date, &standard_time->date);
        datetime_copy_time(&BACnet_Time, &standard_time->time);
        DateTime_Valid = true;
    }
}

bool datetime_standard(BACNET_DATE_TIME *standard_time)
{
    if (!standard_time) {
        return false;
    }
    datetime_set(standard_time, &BACnet_Date, &BACnet_Time);

    return true;
}

bool datetime_valid(void)
{
    return DateTime_Valid;
}

void datetime_reset(void)
{
    datetime_date_wildcard_set(&BACnet_Date);
    datetime_time_wildcard_set(&BACnet_Time);
    UTC_Offset_Minutes = 0;
    DST_Active = false;
    DateTime_Valid = false;
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
    if (!UTC_Offset_Set_Allowed) {
        return false;
    }
    UTC_Offset_Minutes = minutes;

    return true;
}

void datetime_utc_offset_minutes_set_accept(bool accept)
{
    UTC_Offset_Set_Allowed = accept;
}

void datetime_init(void)
{
    /* nothing to do */
}
