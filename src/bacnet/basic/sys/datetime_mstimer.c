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

/* local standard time (never includes the DST adjustment) */
static BACNET_DATE_TIME BACnet_Date_Time;
static int16_t UTC_Offset_Minutes;
/* starting and stopping dates/times to determine DST */
static struct daylight_savings_data DST_Range;
static bool DST_Enabled;
/* local time based on mstimer */
static struct mstimer Date_Timer;
/* true once the defaults were loaded */
static bool Initialized;

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

/* number of minutes that DST adds to local standard time */
#define DST_ADJUST_MINUTES (60L)

/**
 * @brief Get the local determination of daylight savings time being active
 * @param dst [in] The DST rule
 * @param bdatetime [in] The local standard time
 * @param enabled [in] The Daylight Savings Enabled flag
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
    BACNET_DATE_TIME local_time = { 0 };
    bool dst;

    datetime_sync();
    datetime_copy(&local_time, &BACnet_Date_Time);
    dst = datetime_dst_active(&DST_Range, &local_time, DST_Enabled);
    if (dst) {
        datetime_add_minutes(&local_time, DST_ADJUST_MINUTES);
    }
    if (bdate) {
        datetime_copy_date(bdate, &local_time.date);
    }
    if (btime) {
        datetime_copy_time(btime, &local_time.time);
    }
    if (utc_offset_minutes) {
        *utc_offset_minutes = UTC_Offset_Minutes;
    }
    if (dst_active) {
        *dst_active = dst;
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
 * @note The clock stores local standard time. UTC is converted using the
 *  UTC offset. Local wall-clock time is converted by removing the DST
 *  adjustment when DST is active. A wall-clock time that is repeated at the
 *  end of DST is stored as standard time.
 */
void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
{
    BACNET_DATE_TIME local_time = { 0 };
    BACNET_DATE_TIME standard_time = { 0 };

    if (!bdate || !btime) {
        return;
    }
    datetime_copy_date(&local_time.date, bdate);
    datetime_copy_time(&local_time.time, btime);
    if (utc) {
        datetime_utc_to_local(
            &standard_time, &local_time, UTC_Offset_Minutes, 0);
    } else {
        datetime_copy(&standard_time, &local_time);
        if (datetime_dst_active(&DST_Range, &standard_time, DST_Enabled)) {
            /* not standard time, unless the wall-clock time is
               a daylight time that was one hour earlier in standard time */
            datetime_add_minutes(&standard_time, -DST_ADJUST_MINUTES);
            if (!datetime_dst_active(&DST_Range, &standard_time, DST_Enabled)) {
                /* time that does not exist (spring forward gap) */
                datetime_copy(&standard_time, &local_time);
            }
        }
    }
    datetime_copy(&BACnet_Date_Time, &standard_time);
    mstimer_restart(&Date_Timer);
}

/**
 * @brief Get the local standard time, which never includes the DST adjustment
 * @param standard_time [out] The local standard date and time
 * @return true if successful, false on error
 * @note The clock stores local standard time, so it is returned as is.
 *  Use datetime_local() to get the wall-clock time with DST applied.
 */
bool datetime_standard(BACNET_DATE_TIME *standard_time)
{
    if (!standard_time) {
        return false;
    }
    datetime_sync();
    datetime_copy(standard_time, &BACnet_Date_Time);

    return true;
}

/**
 * @brief Set the local standard time, without a DST adjustment
 * @param standard_time [in] The local standard date and time
 * @note This is not a time synchronization notification.
 *  Use it to load a hardware RTC that holds standard time at power up
 *  or after a time synchronization.
 */
void datetime_standard_set(const BACNET_DATE_TIME *standard_time)
{
    if (!standard_time) {
        return;
    }
    datetime_copy(&BACnet_Date_Time, standard_time);
    mstimer_restart(&Date_Timer);
}

/**
 * @brief Reset the local date and time, UTC offset, and DST rule to defaults
 * @note Explicit return to defaults, intended for unit tests.
 */
void datetime_init_defaults(void)
{
    Initialized = true;
    memset(&BACnet_Date_Time, 0, sizeof(BACnet_Date_Time));
    UTC_Offset_Minutes = 0;
    DST_Enabled = false;
    dst_init_defaults(&DST_Range);
    mstimer_set(&Date_Timer, 0);
}

/**
 * @brief Initialize the local date and time timer once.
 *  Later calls do not reset the DST rule, UTC offset, or time.
 */
void datetime_init(void)
{
    if (!Initialized) {
        datetime_init_defaults();
    }
}
