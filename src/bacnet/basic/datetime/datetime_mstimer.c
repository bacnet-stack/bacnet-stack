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
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/dst.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datetime.h"

static BACNET_DATE_TIME BACnet_Date_Time;
static int16_t UTC_Offset_Minutes;
static struct daylight_savings_data DST_Range;
static bool DST_Enabled;
static struct mstimer Date_Timer;
static unsigned long DateTime_Millisecond_Remainder;
static bool DateTime_Initialized;
static bool DateTime_Valid;

/**
 * @brief Initialize the clock state once while the caller holds the lock.
 */
static void datetime_initialize_locked(void)
{
    if (!DateTime_Initialized) {
        datetime_set_values(&BACnet_Date_Time, 1990, 1, 1, 0, 0, 0, 0);
        UTC_Offset_Minutes = 0;
        dst_init_defaults(&DST_Range);
        DST_Enabled = false;
        DateTime_Valid = false;
        DateTime_Millisecond_Remainder = 0;
        mstimer_set(&Date_Timer, 0);
        DateTime_Initialized = true;
    }
}

/**
 * @brief Advance the stored standard time by the elapsed milliseconds.
 */
static void datetime_sync_locked(void)
{
    bacnet_time_t seconds = 0;
    bacnet_time_t elapsed_seconds = 0;
    unsigned long milliseconds = 0;
    unsigned long elapsed_milliseconds = 0;
    unsigned long hundredths = 0;

    datetime_initialize_locked();
    /* One sample measures the elapsed time and moves the origin. */
    milliseconds = mstimer_restart(&Date_Timer);
    milliseconds += DateTime_Millisecond_Remainder;
    DateTime_Millisecond_Remainder = milliseconds % 10UL;
    elapsed_milliseconds = milliseconds - DateTime_Millisecond_Remainder;
    if (elapsed_milliseconds) {
        elapsed_seconds = elapsed_milliseconds / 1000UL;
        milliseconds = elapsed_milliseconds % 1000UL;
        hundredths = (unsigned long)BACnet_Date_Time.time.hundredths +
            (milliseconds / 10UL);
        elapsed_seconds += hundredths / 100UL;
        seconds = datetime_seconds_since_epoch(&BACnet_Date_Time);
        seconds += elapsed_seconds;
        datetime_since_epoch_seconds(&BACnet_Date_Time, seconds);
        BACnet_Date_Time.time.hundredths = (uint8_t)(hundredths % 100UL);
    }
}

/**
 * @brief Determine whether daylight saving time is active for a time value.
 * @param dst [in] Daylight-saving date range configuration
 * @param bdatetime [in] Standard-time date and time to evaluate
 * @param enabled [in] Whether daylight-saving time is enabled
 * @return true if daylight-saving time is active
 */
static bool datetime_dst_active(
    struct daylight_savings_data *dst,
    const BACNET_DATE_TIME *bdatetime,
    bool enabled)
{
    if (enabled) {
        return dst_active(
            dst, bdatetime->date.year, bdatetime->date.month,
            bdatetime->date.day, bdatetime->time.hour, bdatetime->time.min,
            bdatetime->time.sec);
    }

    return false;
}

/**
 * @brief Get the local date and time.
 * @param bdate [out] Date to receive the local date, or NULL
 * @param btime [out] Time to receive the local time, or NULL
 * @param utc_offset_minutes [out] BACnet UTC offset in minutes, or NULL
 * @param dst_active [out] Whether daylight-saving time is active, or NULL
 * @return true
 */
bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    BACNET_DATE_TIME local_time = { 0 };
    bool daylight_savings = false;

    datetime_lock();
    datetime_sync_locked();
    datetime_copy(&local_time, &BACnet_Date_Time);
    daylight_savings =
        datetime_dst_active(&DST_Range, &BACnet_Date_Time, DST_Enabled);
    if (daylight_savings) {
        datetime_add_minutes(&local_time, 60);
    }
    if (utc_offset_minutes) {
        *utc_offset_minutes = UTC_Offset_Minutes;
    }
    datetime_unlock();

    if (bdate) {
        datetime_copy_date(bdate, &local_time.date);
    }
    if (btime) {
        datetime_copy_time(btime, &local_time.time);
    }
    if (dst_active) {
        *dst_active = daylight_savings;
    }

    return true;
}

/**
 * @brief Set the clock to a local standard-time date and time.
 * @param standard_time [in] Standard-time value to set
 *
 * Marks the clock valid and restarts the millisecond timer.
 */
void datetime_standard_set(const BACNET_DATE_TIME *standard_time)
{
    if (!standard_time) {
        return;
    }
    datetime_lock();
    datetime_initialize_locked();
    datetime_sync_locked();
    datetime_copy(&BACnet_Date_Time, standard_time);
    DateTime_Valid = true;
    DateTime_Millisecond_Remainder = 0;
    mstimer_restart(&Date_Timer);
    datetime_unlock();
}

/**
 * @brief Get the clock value in local standard time.
 * @param standard_time [out] Receives the standard-time value
 * @return true if standard_time is not NULL
 */
bool datetime_standard(BACNET_DATE_TIME *standard_time)
{
    if (!standard_time) {
        return false;
    }
    datetime_lock();
    datetime_sync_locked();
    datetime_copy(standard_time, &BACnet_Date_Time);
    datetime_unlock();

    return true;
}

/**
 * @brief Check whether the clock has been set to a valid time.
 * @return true if a time has been set
 */
bool datetime_valid(void)
{
    bool valid = false;

    datetime_lock();
    datetime_initialize_locked();
    valid = DateTime_Valid;
    datetime_unlock();

    return valid;
}

/**
 * @brief Reset the clock to its default value and mark it invalid.
 */
void datetime_reset(void)
{
    datetime_lock();
    DateTime_Initialized = false;
    datetime_initialize_locked();
    datetime_unlock();
}

/**
 * @brief Get the BACnet UTC offset in minutes.
 * @return UTC offset in minutes; positive values are west of UTC
 */
int16_t datetime_utc_offset_minutes(void)
{
    int16_t offset = 0;

    datetime_lock();
    datetime_initialize_locked();
    offset = UTC_Offset_Minutes;
    datetime_unlock();

    return offset;
}

/**
 * @brief Set the BACnet UTC offset in minutes.
 * @param minutes [in] UTC offset; positive values are west of UTC
 * @return true
 *
 * Notifies registered observers when the value changes.
 */
bool datetime_utc_offset_minutes_set(int16_t minutes)
{
    BACNET_DATE_TIME standard_time = { 0 };
    bool changed = false;

    datetime_lock();
    datetime_sync_locked();
    changed = (UTC_Offset_Minutes != minutes);
    UTC_Offset_Minutes = minutes;
    datetime_copy(&standard_time, &BACnet_Date_Time);
    datetime_unlock();
    if (changed) {
        datetime_change_notify(DATETIME_CHANGE_UTC_OFFSET, &standard_time);
    }

    return true;
}

/**
 * @brief Get the daylight-saving-time enabled flag.
 * @return true if daylight-saving time is enabled
 */
bool datetime_dst_enabled(void)
{
    bool enabled = false;

    datetime_lock();
    datetime_initialize_locked();
    enabled = DST_Enabled;
    datetime_unlock();

    return enabled;
}

/**
 * @brief Enable or disable daylight-saving time.
 * @param flag [in] true to enable daylight-saving time
 *
 * Notifies registered observers when the value changes.
 */
void datetime_dst_enabled_set(bool flag)
{
    BACNET_DATE_TIME standard_time = { 0 };
    bool changed = false;

    datetime_lock();
    datetime_sync_locked();
    changed = (DST_Enabled != flag);
    DST_Enabled = flag;
    datetime_copy(&standard_time, &BACnet_Date_Time);
    datetime_unlock();
    if (changed) {
        datetime_change_notify(DATETIME_CHANGE_DST, &standard_time);
    }
}

/**
 * @brief Get the ordinal-weekday daylight-saving date range.
 * @param start_month [out] Start month
 * @param start_week [out] Start week of month
 * @param start_day [out] Start weekday
 * @param end_month [out] End month
 * @param end_week [out] End week of month
 * @param end_day [out] End weekday
 * @return true if the configured range uses ordinal weekdays
 */
bool datetime_dst_ordinal_range(
    uint8_t *start_month,
    uint8_t *start_week,
    uint8_t *start_day,
    uint8_t *end_month,
    uint8_t *end_week,
    uint8_t *end_day)
{
    bool ordinal = false;

    datetime_lock();
    datetime_initialize_locked();
    ordinal = DST_Range.Ordinal;
    if (ordinal && start_month && start_week && start_day && end_month &&
        end_week && end_day) {
        *start_month = DST_Range.Begin_Month;
        *start_week = DST_Range.Begin_Week;
        *start_day = DST_Range.Begin_Day;
        *end_month = DST_Range.End_Month;
        *end_week = DST_Range.End_Week;
        *end_day = DST_Range.End_Day;
    }
    datetime_unlock();

    return ordinal;
}

/**
 * @brief Set the ordinal-weekday daylight-saving date range.
 * @param start_month [in] Start month
 * @param start_week [in] Start week of month
 * @param start_day [in] Start weekday
 * @param end_month [in] End month
 * @param end_week [in] End week of month
 * @param end_day [in] End weekday
 * @return true
 *
 * Notifies registered observers when the range changes.
 */
bool datetime_dst_ordinal_range_set(
    uint8_t start_month,
    uint8_t start_week,
    BACNET_WEEKDAY start_day,
    uint8_t end_month,
    uint8_t end_week,
    BACNET_WEEKDAY end_day)
{
    BACNET_DATE_TIME standard_time = { 0 };
    bool changed = false;

    datetime_lock();
    datetime_sync_locked();
    changed = !DST_Range.Ordinal || (DST_Range.Begin_Month != start_month) ||
        (DST_Range.Begin_Week != start_week) ||
        (DST_Range.Begin_Day != (uint8_t)start_day) ||
        (DST_Range.End_Month != end_month) ||
        (DST_Range.End_Week != end_week) ||
        (DST_Range.End_Day != (uint8_t)end_day);
    DST_Range.Ordinal = true;
    DST_Range.Begin_Month = start_month;
    DST_Range.Begin_Week = start_week;
    DST_Range.Begin_Day = (uint8_t)start_day;
    DST_Range.End_Month = end_month;
    DST_Range.End_Week = end_week;
    DST_Range.End_Day = (uint8_t)end_day;
    datetime_copy(&standard_time, &BACnet_Date_Time);
    datetime_unlock();
    if (changed) {
        datetime_change_notify(DATETIME_CHANGE_DST, &standard_time);
    }

    return true;
}

/**
 * @brief Get the calendar-date daylight-saving date range.
 * @param start_month [out] Start month
 * @param start_day [out] Start day of month
 * @param end_month [out] End month
 * @param end_day [out] End day of month
 * @return true if the configured range uses calendar dates
 */
bool datetime_dst_date_range(
    uint8_t *start_month,
    uint8_t *start_day,
    uint8_t *end_month,
    uint8_t *end_day)
{
    bool ordinal = false;

    datetime_lock();
    datetime_initialize_locked();
    ordinal = DST_Range.Ordinal;
    if (!ordinal && start_month && start_day && end_month && end_day) {
        *start_month = DST_Range.Begin_Month;
        *start_day = DST_Range.Begin_Day;
        *end_month = DST_Range.End_Month;
        *end_day = DST_Range.End_Day;
    }
    datetime_unlock();

    return !ordinal;
}

/**
 * @brief Set the calendar-date daylight-saving date range.
 * @param start_month [in] Start month
 * @param start_day [in] Start day of month
 * @param end_month [in] End month
 * @param end_day [in] End day of month
 * @return true
 *
 * Notifies registered observers when the range changes.
 */
bool datetime_dst_date_range_set(
    uint8_t start_month, uint8_t start_day, uint8_t end_month, uint8_t end_day)
{
    BACNET_DATE_TIME standard_time = { 0 };
    bool changed = false;

    datetime_lock();
    datetime_sync_locked();
    changed = DST_Range.Ordinal || (DST_Range.Begin_Month != start_month) ||
        (DST_Range.Begin_Day != start_day) ||
        (DST_Range.End_Month != end_month) || (DST_Range.End_Day != end_day);
    DST_Range.Ordinal = false;
    DST_Range.Begin_Month = start_month;
    DST_Range.Begin_Day = start_day;
    DST_Range.End_Month = end_month;
    DST_Range.End_Day = end_day;
    datetime_copy(&standard_time, &BACnet_Date_Time);
    datetime_unlock();
    if (changed) {
        datetime_change_notify(DATETIME_CHANGE_DST, &standard_time);
    }

    return true;
}

/**
 * @brief Apply a BACnet TimeSynchronization request to the clock.
 * @param bdate [in] Date to set, or NULL to retain the current date
 * @param btime [in] Time to set, or NULL to retain the current time
 * @param utc [in] true if the request carries UTC time
 *
 * Local requests are converted to standard time using the configured DST
 * range. UTC requests are converted using the BACnet UTC offset.
 */
void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
{
    BACNET_DATE_TIME requested = { 0 };
    BACNET_DATE_TIME standard_time = { 0 };
    BACNET_DATE_TIME current = { 0 };
    bool changed = false;

    datetime_lock();
    datetime_sync_locked();
    datetime_copy(&current, &BACnet_Date_Time);
    /* seed omitted fields in the same time basis as the request */
    if (utc) {
        datetime_local_to_utc(&requested, &current, UTC_Offset_Minutes, 0);
    } else {
        datetime_copy(&requested, &current);
        if (datetime_dst_active(&DST_Range, &current, DST_Enabled)) {
            datetime_add_minutes(&requested, 60);
        }
    }
    if (bdate) {
        datetime_copy_date(&requested.date, bdate);
    }
    if (btime) {
        datetime_copy_time(&requested.time, btime);
    }
    if (utc) {
        datetime_utc_to_local(
            &standard_time, &requested, UTC_Offset_Minutes, 0);
    } else {
        datetime_copy(&standard_time, &requested);
        if (datetime_dst_active(&DST_Range, &requested, DST_Enabled)) {
            datetime_add_minutes(&standard_time, -60);
        }
    }
    if (!datetime_timesync_in_deadband(&current, &standard_time)) {
        datetime_copy(&BACnet_Date_Time, &standard_time);
        mstimer_restart(&Date_Timer);
        DateTime_Millisecond_Remainder = 0;
        DateTime_Valid = true;
        changed = true;
    }
    datetime_copy(&standard_time, &BACnet_Date_Time);
    datetime_unlock();
    if (changed) {
        datetime_change_notify(DATETIME_CHANGE_TIME, &standard_time);
    }
}

/**
 * @brief Initialize the millisecond-timer clock backend.
 *
 * Initialization is performed once; call datetime_reset() to reset it.
 */
void datetime_init(void)
{
    datetime_lock();
    datetime_initialize_locked();
    datetime_unlock();
}
