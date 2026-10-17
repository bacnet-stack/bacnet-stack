/**
 * @file
 * @brief System time library using the host operating system clock
 * @author Steve Karg
 * @date 2009
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacport.h"
#include "bacnet/datetime.h"

/* Difference between the BACnet clock and the host clock. Kept so time
 * can be set without changing the operating-system clock. */
static struct timeval Time_Offset;

/**
 * @brief Calculate the normalized difference between two clock readings.
 * @param t0 [in] Target time
 * @param t1 [in] Current system time
 * @param difference [out] Whole seconds and a microsecond remainder in
 * the range [0, 999999]
 */
static void time_difference(
    struct timeval t0, struct timeval t1, struct timeval *difference)
{
    time_t sec = 0;
    long usec = 0;

    sec = (time_t)t0.tv_sec - (time_t)t1.tv_sec;
    usec = (long)t0.tv_usec - (long)t1.tv_usec;
    while (usec >= 1000000L) {
        usec -= 1000000L;
        sec += 1;
    }
    while (usec < 0) {
        usec += 1000000L;
        sec -= 1;
    }
    difference->tv_sec = sec;
    difference->tv_usec = usec;
}

/**
 * @brief Add the stored offset to a system-clock reading.
 * @param seconds [in,out] Seconds since the epoch
 * @param microseconds [in,out] Microseconds, normalized to [0, 999999]
 *
 * Time_Offset.tv_usec is in [0, 999999], so the microsecond sum carries at
 * most one second.
 */
static void time_offset_apply(time_t *seconds, long *microseconds)
{
    *seconds += Time_Offset.tv_sec;
    *microseconds += Time_Offset.tv_usec;
    if (*microseconds >= 1000000L) {
        *microseconds -= 1000000L;
        *seconds += 1;
    }
}

/**
 * @brief Determine whether two system-clock readings are within the
 * TimeSynchronization deadband.
 * @param t0 [in] Requested time
 * @param t1 [in] Current time
 * @return true if the absolute difference is at most the configured
 * deadband; false if the deadband is zero
 *
 * Timestamps are compared instead of local wall-clock values because
 * wall-clock values repeat during the DST fall-back hour.
 */
static bool timestamp_in_deadband(struct timeval t0, struct timeval t1)
{
    int64_t difference = 0;
    int64_t deadband = (int64_t)datetime_timesync_deadband() * 1000;

    difference = (((int64_t)t0.tv_sec - (int64_t)t1.tv_sec) * 1000000) +
        ((int64_t)t0.tv_usec - (int64_t)t1.tv_usec);
    if (difference < 0) {
        difference = -difference;
    }

    return (deadband > 0) && (difference <= deadband);
}

/**
 * @brief Convert a local struct tm value to UTC calendar fields using the
 * host timezone offset and daylight-saving state, without gmtime().
 * @param local_tm [in] Local time, including tm_isdst
 * @param utc_tm [in,out] Receives the UTC year, month, day, hour, minute,
 * and second; other members are unchanged
 */
static void datetime_tm_utc_fields(const struct tm *local_tm, struct tm *utc_tm)
{
    BACNET_DATE_TIME local_time = { 0 };
    BACNET_DATE_TIME utc_time = { 0 };

    tzset();
    datetime_set_values(
        &local_time, (uint16_t)(local_tm->tm_year + 1900),
        (uint8_t)(local_tm->tm_mon + 1), (uint8_t)local_tm->tm_mday,
        (uint8_t)local_tm->tm_hour, (uint8_t)local_tm->tm_min,
        (uint8_t)local_tm->tm_sec, 0);
    (void)datetime_local_to_utc(
        &utc_time, &local_time, (int16_t)(timezone / 60),
        (local_tm->tm_isdst > 0) ? -60 : 0);
    utc_tm->tm_year = (int)utc_time.date.year - 1900;
    utc_tm->tm_mon = (int)utc_time.date.month - 1;
    utc_tm->tm_mday = utc_time.date.day;
    utc_tm->tm_hour = utc_time.time.hour;
    utc_tm->tm_min = utc_time.time.min;
    utc_tm->tm_sec = utc_time.time.sec;
}

/**
 * @brief Apply a BACnet TimeSynchronization request to the host clock offset.
 * @param bdate [in] Date to set, or NULL to retain the current date
 * @param btime [in] Time to set, or NULL to retain the current time
 * @param utc [in] true if the request carries UTC time
 *
 * The host clock itself is not changed; its offset from system time is updated.
 */
void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
{
    struct timeval tv_system = { 0 };
    struct timeval tv_target = { 0 };
    struct timeval tv_current = { 0 };
    struct tm input_tm = { 0 };
    struct tm target_tm = { 0 };
    struct tm standard_tm = { 0 };
    struct tm check_tm = { 0 };
    struct tm *timeinfo = NULL;
    struct tm *roundtrip = NULL;
    time_t rawtime = 0;
    time_t target_seconds = 0;
    time_t standard_seconds = 0;
    long current_usec = 0;
    BACNET_DATE_TIME standard_time = { 0 };

    datetime_lock();
    if (gettimeofday(&tv_system, NULL) != 0) {
        datetime_unlock();
        return;
    }
    rawtime = tv_system.tv_sec;
    current_usec = (long)tv_system.tv_usec;
    time_offset_apply(&rawtime, &current_usec);
    timeinfo = localtime(&rawtime);
    if (!timeinfo) {
        datetime_unlock();
        return;
    }
    input_tm = *timeinfo;
    if (utc) {
        /* omitted fields are seeded in the same basis as the request */
        datetime_tm_utc_fields(timeinfo, &input_tm);
    }
    if (bdate) {
        input_tm.tm_year = (int)bdate->year - 1900;
        input_tm.tm_mon = (int)bdate->month - 1;
        input_tm.tm_mday = bdate->day;
    }
    if (btime) {
        input_tm.tm_hour = btime->hour;
        input_tm.tm_min = btime->min;
        input_tm.tm_sec = btime->sec;
    }
    input_tm.tm_isdst = utc ? 0 : -1;
    target_tm = input_tm;
    tzset();
    target_seconds = mktime(&target_tm);
    if (!utc && (target_tm.tm_isdst > 0)) {
        standard_tm = input_tm;
        standard_tm.tm_isdst = 0;
        standard_seconds = mktime(&standard_tm);
        check_tm = standard_tm;
        roundtrip = localtime(&standard_seconds);
        if (roundtrip) {
            check_tm = *roundtrip;
        }
        if (!roundtrip || (check_tm.tm_isdst != 0) ||
            (check_tm.tm_year != input_tm.tm_year) ||
            (check_tm.tm_mon != input_tm.tm_mon) ||
            (check_tm.tm_mday != input_tm.tm_mday) ||
            (check_tm.tm_hour != input_tm.tm_hour) ||
            (check_tm.tm_min != input_tm.tm_min) ||
            (check_tm.tm_sec != input_tm.tm_sec)) {
            target_tm = input_tm;
            target_tm.tm_isdst = -1;
            target_seconds = mktime(&target_tm);
        } else {
            target_seconds = standard_seconds;
        }
    }
    if (utc) {
        target_seconds -= timezone;
    }
    tv_target.tv_sec = target_seconds;
    if (btime) {
        tv_target.tv_usec = (suseconds_t)btime->hundredths * 10000;
    } else {
        tv_target.tv_usec = (suseconds_t)current_usec;
    }
    tv_current.tv_sec = rawtime;
    tv_current.tv_usec = (suseconds_t)current_usec;
    if (timestamp_in_deadband(tv_target, tv_current)) {
        datetime_unlock();
        return;
    }
    if (gettimeofday(&tv_system, NULL) != 0) {
        datetime_unlock();
        return;
    }
    time_difference(tv_target, tv_system, &Time_Offset);
    datetime_unlock();
    if (datetime_standard(&standard_time)) {
        datetime_change_notify(DATETIME_CHANGE_TIME, &standard_time);
    }
}

/**
 * @brief Get the host's local date, time, UTC offset, and DST state.
 * @param bdate [out] Receives the local date, or NULL
 * @param btime [out] Receives the local time, or NULL
 * @param utc_offset_minutes [out] Receives the BACnet UTC offset, or NULL
 * @param dst_active [out] Receives the DST state, or NULL
 * @return true if the system time was retrieved successfully
 */
bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    bool status = false;
    struct tm *timeinfo = NULL;
    struct timeval tv = { 0 };
    time_t seconds = 0;
    long microseconds = 0;

    datetime_lock();
    if (gettimeofday(&tv, NULL) == 0) {
        seconds = tv.tv_sec;
        microseconds = (long)tv.tv_usec;
        time_offset_apply(&seconds, &microseconds);
        timeinfo = localtime(&seconds);
        if (timeinfo) {
            status = true;
            if (bdate) {
                datetime_set_date(
                    bdate, (uint16_t)(timeinfo->tm_year + 1900),
                    (uint8_t)(timeinfo->tm_mon + 1),
                    (uint8_t)timeinfo->tm_mday);
            }
            if (btime) {
                datetime_set_time(
                    btime, (uint8_t)timeinfo->tm_hour,
                    (uint8_t)timeinfo->tm_min, (uint8_t)timeinfo->tm_sec,
                    (uint8_t)(microseconds / 10000));
            }
            if (dst_active) {
                *dst_active = (timeinfo->tm_isdst > 0);
            }
            if (utc_offset_minutes) {
                *utc_offset_minutes =
                    (int16_t)(-(timeinfo->tm_gmtoff -
                                ((timeinfo->tm_isdst > 0) ? 3600 : 0)) /
                              60);
            }
        }
    }
    datetime_unlock();

    return status;
}

/**
 * @brief Get the host's BACnet UTC offset in minutes.
 * @return UTC offset in minutes; positive values are west of UTC
 */
int16_t datetime_utc_offset_minutes(void)
{
    int16_t offset = 0;

    (void)datetime_local(NULL, NULL, &offset, NULL);

    return offset;
}

/**
 * @brief Attempt to set the host's BACnet UTC offset.
 * @param minutes [in] Requested UTC offset in minutes
 * @return false because the host operating system owns its timezone
 */
bool datetime_utc_offset_minutes_set(int16_t minutes)
{
    (void)minutes;
    return false;
}

/**
 * @brief Set the host clock offset to a local standard-time value.
 * @param standard_time [in] Standard-time value to set
 *
 * The operating system clock and timezone are not modified.
 */
void datetime_standard_set(const BACNET_DATE_TIME *standard_time)
{
    struct tm standard_tm = { 0 };
    struct timeval tv_system = { 0 };
    struct timeval tv_target = { 0 };
    time_t target_seconds = 0;

    if (!standard_time || (gettimeofday(&tv_system, NULL) != 0)) {
        return;
    }
    standard_tm.tm_year = (int)standard_time->date.year - 1900;
    standard_tm.tm_mon = (int)standard_time->date.month - 1;
    standard_tm.tm_mday = standard_time->date.day;
    standard_tm.tm_hour = standard_time->time.hour;
    standard_tm.tm_min = standard_time->time.min;
    standard_tm.tm_sec = standard_time->time.sec;
    standard_tm.tm_isdst = 0;
    tzset();
    target_seconds = mktime(&standard_tm);
    tv_target.tv_sec = target_seconds;
    tv_target.tv_usec = (suseconds_t)standard_time->time.hundredths * 10000;
    datetime_lock();
    time_difference(tv_target, tv_system, &Time_Offset);
    datetime_unlock();
}

/**
 * @brief Get the host's local time expressed as standard time.
 * @param standard_time [out] Receives the standard-time value
 * @return true if standard_time is not NULL and the local time is available
 */
bool datetime_standard(BACNET_DATE_TIME *standard_time)
{
    BACNET_DATE date = { 0 };
    BACNET_TIME time = { 0 };
    bool dst = false;

    if (!standard_time || !datetime_local(&date, &time, NULL, &dst)) {
        return false;
    }
    datetime_set(standard_time, &date, &time);
    if (dst) {
        datetime_add_minutes(standard_time, -60);
    }

    return true;
}

/**
 * @brief Check whether the host clock is considered valid.
 * @return true because the host operating system owns clock validity
 */
bool datetime_valid(void)
{
    return true;
}

/**
 * @brief Reset the host clock offset to the operating-system clock.
 */
void datetime_reset(void)
{
    datetime_lock();
    Time_Offset.tv_sec = 0;
    Time_Offset.tv_usec = 0;
    datetime_unlock();
}

/**
 * @brief Initialize the host date and time backend.
 *
 * The operating system owns the local clock and timezone, so no initialization
 * is required.
 */
void datetime_init(void)
{
    /* The operating system owns the local clock and timezone. */
}
