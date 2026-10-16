/**
 * @file
 * @brief System time library header file.
 * @details This library provides functions for getting/setting the system time.
 * @author Steve Karg
 * @date 2009
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "bacport.h"
#include "bacnet/bacdef.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datetime.h"

/* Offset between Windows epoch 1/1/1601 and
   Unix epoch 1/1/1970 in 100 nanosec units */
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS) || defined(__BORLANDC__)
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL
#endif

#if defined(__BORLANDC__) || defined(_WIN32)
/* seems to not be defined in time.h as specified by The Open Group */
/* difference from UTC and local standard time  */
long int timezone;
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
int gettimeofday(struct timeval *tp, void *tzp);
#endif

static time_t Time_Offset; /* Time offset in seconds */

/**
 * @brief Wrapper for localtime that works across different compilers.
 * @param newtime Pointer to a struct tm to hold the local time.
 * @param seconds Pointer to a time_t value representing the time in seconds
 * since the epoch.
 * @param tblock Pointer to a struct tm pointer that will point to the result.
 * @return True if successful, False otherwise.
 */
static bool
datetime_localtime(struct tm *newtime, time_t *seconds, struct tm **tblock)
{
#if defined(_MSC_VER)
    errno_t err = 0;

    err = localtime_s(newtime, seconds);
    if (err == 0) {
        *tblock = newtime;
        return true;
    }
#else
    struct tm *tmp = NULL;

    tmp = localtime(seconds);
    if (tmp) {
        *newtime = *tmp;
        *tblock = newtime;
        return true;
    }
#endif

    *tblock = NULL;
    return false;
}

/**
 * @brief Calculate the time offset from the system clock.
 * @param t0 [in] Target time
 * @param t1 [in] Current system time
 * @return Time offset in seconds
 */
static time_t time_difference(struct timeval t0, struct timeval t1)
{
    return (t0.tv_sec - t1.tv_sec) + (t0.tv_usec - t1.tv_usec) / 1000000;
}

/**
 * @brief Get the host timezone offset from UTC in seconds west.
 * @return Standard-time offset in seconds west of UTC
 */
static long datetime_timezone_seconds_west(void)
{
#if defined(_MSC_VER) || defined(__BORLANDC__)
    long seconds_west = 0;

    (void)_get_timezone(&seconds_west);
    return seconds_west;
#else
    _tzset();
    return timezone;
#endif
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

    datetime_set_values(
        &local_time, (uint16_t)(local_tm->tm_year + 1900),
        (uint8_t)(local_tm->tm_mon + 1), (uint8_t)local_tm->tm_mday,
        (uint8_t)local_tm->tm_hour, (uint8_t)local_tm->tm_min,
        (uint8_t)local_tm->tm_sec, 0);
    (void)datetime_local_to_utc(
        &utc_time, &local_time,
        (int16_t)(datetime_timezone_seconds_west() / 60),
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
    struct tm current_tm = { 0 };
    struct tm input_tm = { 0 };
    struct tm target_tm = { 0 };
    struct tm standard_tm = { 0 };
    struct tm check_tm = { 0 };
    struct tm *timeinfo = NULL;
    struct tm *roundtrip = NULL;
    time_t seconds = 0;
    time_t target_seconds = 0;
    time_t standard_seconds = 0;
    BACNET_DATE_TIME standard_time = { 0 };

    datetime_lock();
    if (gettimeofday(&tv_system, NULL) != 0) {
        datetime_unlock();
        return;
    }
    seconds = tv_system.tv_sec + Time_Offset;
    if (!datetime_localtime(&current_tm, &seconds, &timeinfo)) {
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
    (void)datetime_timezone_seconds_west();
    target_seconds = mktime(&target_tm);
    if (!utc && (target_tm.tm_isdst > 0)) {
        standard_tm = input_tm;
        standard_tm.tm_isdst = 0;
        standard_seconds = mktime(&standard_tm);
        if (datetime_localtime(&check_tm, &standard_seconds, &roundtrip)) {
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
        target_seconds -= datetime_timezone_seconds_west();
    }
    tv_target.tv_sec = target_seconds;
    tv_target.tv_usec = btime ? (long)btime->hundredths * 10000 : 0;
    tv_current = tv_system;
    tv_current.tv_sec += Time_Offset;
    if (timestamp_in_deadband(tv_target, tv_current)) {
        datetime_unlock();
        return;
    }
    if (gettimeofday(&tv_system, NULL) != 0) {
        datetime_unlock();
        return;
    }
    Time_Offset = time_difference(tv_target, tv_system);
    datetime_unlock();
    if (datetime_standard(&standard_time)) {
        datetime_change_notify(DATETIME_CHANGE_TIME, &standard_time);
    }
}

#if defined(_MSC_VER) || defined(__BORLANDC__)
struct timezone {
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime; /* type of dst correction */
};

/**
 * @brief Simulate the POSIX gettimeofday function on Windows.
 * @param tp [out] Receives seconds and microseconds since the Unix epoch
 * @param tzp [out] Optional timezone information
 * @return 0
 *
 * The elapsed-time resolution is limited by timeGetTime().
 */
int gettimeofday(struct timeval *tp, void *tzp)
{
    static int tzflag = 0;
    struct timezone *tz;
    long tz_seconds = 0;
    int tz_hours = 0;
    /* start calendar time in microseconds */
    static LONGLONG usec_timer = 0;
    LONGLONG usec_elapsed = 0;
    /* elapsed time in milliseconds */
    static uint32_t time_start = 0;
    /* semi-accurate time from File Timer */
    FILETIME ft;
    uint32_t elapsed_milliseconds = 0;

    tzp = tzp;
    if (usec_timer == 0) {
        /* a 64-bit value representing the number of
           100-nanosecond intervals since January 1, 1601 (UTC). */
        GetSystemTimeAsFileTime(&ft);
        usec_timer = ft.dwHighDateTime;
        usec_timer <<= 32;
        usec_timer |= ft.dwLowDateTime;
        /*converting file time to unix epoch 1970 */
        usec_timer /= 10; /*convert into microseconds */
        usec_timer -= DELTA_EPOCH_IN_MICROSECS;
        tp->tv_sec = (long)(usec_timer / 1000000UL);
        tp->tv_usec = (long)(usec_timer % 1000000UL);
        time_start = timeGetTime();
    } else {
        elapsed_milliseconds = timeGetTime() - time_start;
        usec_elapsed = usec_timer + ((LONGLONG)elapsed_milliseconds * 1000UL);
        tp->tv_sec = (long)(usec_elapsed / 1000000UL);
        tp->tv_usec = (long)(usec_elapsed % 1000000UL);
    }
    if (tzp) {
        if (!tzflag) {
            _tzset();
            tzflag++;
        }
        tz = tzp;
        (void)_get_timezone(&tz_seconds);
        tz->tz_minuteswest = tz_seconds / 60;
        (void)_get_daylight(&tz_hours);
        tz->tz_dsttime = tz_hours;
    }

    return 0;
}
#endif

/**
 * @brief Convert a struct tm to BACnet date and time
 * @param bdate - the BACnet Date structure to hold date
 * @param btime - the BACnet Time structure to hold time
 * @param hundredths - hundredths of a second [0..99]
 * @param utc_offset_minutes - BACnet UTC_Offset, in minutes west of UTC.
 * For example, +300 represents UTC-5.
 * @param dst_active - true if DST is enabled and active
 * @param tblock - the struct tm to convert
 *
 *  struct tm :
 *    int    tm_sec   Seconds [0,60].
 *    int    tm_min   Minutes [0,59].
 *    int    tm_hour  Hour [0,23].
 *    int    tm_mday  Day of month [1,31].
 *    int    tm_mon   Month of year [0,11].
 *    int    tm_year  Years since 1900.
 *    int    tm_wday  Day of week [0,6] (Sunday =0).
 *    int    tm_yday  Day of year [0,365].
 *    int    tm_isdst Daylight Savings flag.
 */
static void datetime_from_tm(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    uint8_t hundredths,
    int16_t *utc_offset_minutes,
    bool *dst_active,
    struct tm *tblock)
{
    if (bdate) {
        datetime_set_date(
            bdate, (uint16_t)(tblock->tm_year + 1900),
            (uint8_t)(tblock->tm_mon + 1), (uint8_t)tblock->tm_mday);
    }
    if (btime) {
        datetime_set_time(
            btime, (uint8_t)tblock->tm_hour, (uint8_t)tblock->tm_min,
            (uint8_t)tblock->tm_sec, hundredths);
    }
    if (dst_active) {
        /* The value of tm_isdst is:
           - positive if Daylight Saving Time is in effect,
           - 0 if Daylight Saving Time is not in effect, and
           - negative if the information is not available. */
        if (tblock->tm_isdst > 0) {
            *dst_active = true;
        } else {
            *dst_active = false;
        }
    }
    /* note: timezone is declared in <time.h> stdlib. */
    if (utc_offset_minutes) {
        /* timezone is set to the difference, in seconds,
            between Coordinated Universal Time (UTC) and
            local standard time */
        *utc_offset_minutes = (int16_t)(datetime_timezone_seconds_west() / 60);
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
    struct tm *tblock = NULL;
    struct tm newtime = { 0 };
    struct timeval tv = { 0 };
    time_t seconds = 0;

    datetime_lock();
    if (gettimeofday(&tv, NULL) == 0) {
        seconds = tv.tv_sec + Time_Offset;
        if (datetime_localtime(&newtime, &seconds, &tblock)) {
            datetime_from_tm(
                bdate, btime, (uint8_t)(tv.tv_usec / 10000), utc_offset_minutes,
                dst_active, tblock);
            status = true;
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
    (void)datetime_timezone_seconds_west();
    target_seconds = mktime(&standard_tm);
    tv_target.tv_sec = target_seconds;
    tv_target.tv_usec = (long)standard_time->time.hundredths * 10000;
    datetime_lock();
    Time_Offset = time_difference(tv_target, tv_system);
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
    Time_Offset = 0;
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
    /* nothing to do */
}
