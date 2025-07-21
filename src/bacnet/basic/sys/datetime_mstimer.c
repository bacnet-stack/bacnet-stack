/**
 * @file
 * @brief API for Milliseconds Timer based Time-of-Day Clock
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/dst.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datetime.h"

/* local time */
static BACNET_DATE_TIME BACnet_Date_Time;
static int16_t UTC_Offset_Minutes;
/* starting and stopping dates/times to determine DST */
static struct daylight_savings_data DST_Range;
static bool DST_Enabled;
/* local time based on mstimer */
static struct mstimer Date_Timer;

/**
 * @brief Synchronize the local time from the millisecond timer
 */
static void datetime_sync(void)
{
    bacnet_time_t seconds, elapsed_seconds;
    unsigned long milliseconds;

    milliseconds = mstimer_elapsed(&Date_Timer);
    elapsed_seconds = milliseconds / 1000UL;
    if (elapsed_seconds) {
        mstimer_restart(&Date_Timer);
        seconds = datetime_seconds_since_epoch(&BACnet_Date_Time);
        seconds += elapsed_seconds;
        datetime_since_epoch_seconds(&BACnet_Date_Time, seconds);
        /* generate a hundredths value */
        milliseconds -= (elapsed_seconds * 1000UL);
        BACnet_Date_Time.time.hundredths = milliseconds / 10;
    }
}

/**
 * @brief Get the local determination of daylight savings time being active
 * @return true if DST is active, false otherwise
 */
static bool datetime_dst_active(
    struct daylight_savings_data *dst,
    BACNET_DATE_TIME *bdatetime,
    bool enabled)
{
    bool active = false;

    if (enabled) {
        active = dst_active(
            dst, bdatetime->date.year, bdatetime->date.month,
            bdatetime->date.day, bdatetime->time.hour, bdatetime->time.min,
            bdatetime->time.sec);
    }

    return active;
}

/**
 * @brief Get the local date and time
 * @param bdate [out] The date to get
 * @param btime [out] The time to get
 * @param utc_offset_minutes [out] The UTC offset in minutes
 * @param dst_active [out] The DST flag
 * @return true if successful, false on error
 */
bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    datetime_sync();
    if (bdate) {
        datetime_copy_date(bdate, &BACnet_Date_Time.date);
    }
    if (btime) {
        datetime_copy_time(btime, &BACnet_Date_Time.time);
    }
    if (utc_offset_minutes) {
        *utc_offset_minutes = UTC_Offset_Minutes;
    }
    if (dst_active) {
        *dst_active =
            datetime_dst_active(&DST_Range, &BACnet_Date_Time, DST_Enabled);
    }

    return true;
}

/**
 * @brief Get the UTC offset in minutes
 * @return The UTC offset in minutes
 */
int16_t datetime_utc_offset_minutes(void)
{
    return UTC_Offset_Minutes;
}

/**
 * @brief Set the UTC offset in minutes
 * @param minutes [in] The UTC offset in minutes
 * @return true if successful, false on error
 */
bool datetime_utc_offset_minutes_set(int16_t minutes)
{
    UTC_Offset_Minutes = minutes;

    return true;
}

/**
 * @brief Get the Daylight Savings Enabled flag
 * @return Daylight Savings Enabled flag
 */
bool datetime_dst_enabled(void)
{
    return DST_Enabled;
}

/**
 * @brief Set the Daylight Savings Enabled flag
 * @param flag [in] The Daylight Savings Enabled flag
 */
void datetime_dst_enabled_set(bool flag)
{
    DST_Enabled = flag;
}

/**
 * @brief Get the local DST start and end date range
 * @param dst_range [out] The DST range to get
 * @return true if successful, false on error
 */
bool datetime_dst_ordinal_range(
    uint8_t *start_month,
    uint8_t *start_week,
    uint8_t *start_day,
    uint8_t *end_month,
    uint8_t *end_week,
    uint8_t *end_day)
{
    if (!DST_Range.Ordinal) {
        return false;
    }
    *start_month = DST_Range.Begin_Month;
    *start_week = DST_Range.Begin_Week;
    *start_day = DST_Range.Begin_Day;
    *end_month = DST_Range.End_Month;
    *end_week = DST_Range.End_Week;
    *end_day = DST_Range.End_Day;

    return true;
}

bool datetime_dst_ordinal_range_set(
    uint8_t start_month,
    uint8_t start_week,
    BACNET_WEEKDAY start_day,
    uint8_t end_month,
    uint8_t end_week,
    BACNET_WEEKDAY end_day)
{
    DST_Range.Ordinal = true;
    DST_Range.Begin_Month = start_month;
    DST_Range.Begin_Week = start_week;
    DST_Range.Begin_Day = start_day;
    DST_Range.End_Month = end_month;
    DST_Range.End_Week = end_week;
    DST_Range.End_Day = end_day;

    return true;
}

/**
 * @brief Get the local DST start and end date range for specific month day
 * @param dst_range [in] The DST range
 * @return true if the start and end month day values are returned
 */
bool datetime_dst_date_range(
    uint8_t *start_month,
    uint8_t *start_day,
    uint8_t *end_month,
    uint8_t *end_day)
{
    if (DST_Range.Ordinal) {
        return false;
    }
    *start_month = DST_Range.Begin_Month;
    *start_day = DST_Range.Begin_Day;
    *end_month = DST_Range.End_Month;
    *end_day = DST_Range.End_Day;

    return true;
}

/**
 * @brief Set the local DST start and end date range for specific month day
 * @param dst_range [in] The DST range to set
 * @return true if successful, false on error
 */
bool datetime_dst_date_range_set(
    uint8_t start_month, uint8_t start_day, uint8_t end_month, uint8_t end_day)
{
    DST_Range.Ordinal = false;
    DST_Range.Begin_Month = start_month;
    DST_Range.Begin_Day = start_day;
    DST_Range.End_Month = end_month;
    DST_Range.End_Day = end_day;

    return true;
}

/**
 * @brief Set the local date and time from a BACnet TimeSynchronization request
 * @param bdate [in] The date to set
 * @param btime [in] The time to set
 * @param utc [in] true if originating from an UTCTimeSynchronization request
 */
void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
{
    BACNET_DATE_TIME local_time = { 0 };
    const int32_t dst_adjust_minutes = 60L;

    if (utc) {
        if (bdate && btime) {
            datetime_copy_date(&local_time.date, bdate);
            datetime_copy_time(&local_time.time, btime);
            datetime_add_minutes(&local_time, UTC_Offset_Minutes);
            if (datetime_dst_active(&DST_Range, &local_time, DST_Enabled)) {
                datetime_add_minutes(&local_time, dst_adjust_minutes);
            }
            datetime_copy(&BACnet_Date_Time, &local_time);
            mstimer_restart(&Date_Timer);
        }
    } else {
        datetime_copy_date(&BACnet_Date_Time.date, bdate);
        datetime_copy_time(&BACnet_Date_Time.time, btime);
        mstimer_restart(&Date_Timer);
    }
}

/**
 * @brief Initialize the local date and time timer
 */
void datetime_init(void)
{
    dst_init_defaults(&DST_Range);
    mstimer_set(&Date_Timer, 0);
}
