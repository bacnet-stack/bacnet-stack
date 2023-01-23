/**
 * @file
 * @author Steve Karg
 * @date 2009
 * @brief System time library header file.
 *
 * @section DESCRIPTION
 *
 * This library provides functions for getting and setting the system time.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if CONFIG_NATIVE_APPLICATION
#include <sys/time.h>
#elif defined(__ZEPHYR__)
#include <zephyr/posix/sys/time.h>
#else
#include <posix/sys/time.h>
#endif
#include <time.h>
#include "bacnet/datetime.h"


/* HACK:
 * - Zephyr does not declare timezone in any header file.
 * - The gcc-arm-none-eabi-7-2018-q2-update/bin/arm-none-eabi/lib/thumb/v7e-m/
 *   libc_nano.a 'time' symbol does not resolve '_gettimeofday'
 *
 * TODO: figure out how to link in the real time() and timezone;
 */
long timezone;

time_t time(time_t *tloc)
{
    time_t time = { 0 };

    return time;
}

/**
 * @brief Get the date, time, timezone, and UTC offset from system
 * @param utc_time - the BACnet Date and Time structure to hold UTC time
 * @param local_time - the BACnet Date and Time structure to hold local time
 * @param utc_offset_minutes - number of minutes offset from UTC
 * For example, -6*60 represents 6.00 hours behind UTC/GMT
 * @param true if DST is enabled and active
 * @return true if local time was retrieved
 */
bool datetime_local(
    BACNET_DATE * bdate,
    BACNET_TIME * btime,
    int16_t * utc_offset_minutes,
    bool * dst_active)
{
    bool status = false;
    struct tm tblock_st = { 0 };
    struct tm *tblock = &tblock_st;
    struct timeval tv;

    if (gettimeofday(&tv, NULL) == 0)
    {
        tblock = (struct tm *)localtime((const time_t *)&tv.tv_sec);
    }
    if (tblock) {
        status = true;
        /** struct tm
         *   int    tm_sec   Seconds [0,60].
         *   int    tm_min   Minutes [0,59].
         *   int    tm_hour  Hour [0,23].
         *   int    tm_mday  Day of month [1,31].
         *   int    tm_mon   Month of year [0,11].
         *   int    tm_year  Years since 1900.
         *   int    tm_wday  Day of week [0,6] (Sunday =0).
         *   int    tm_yday  Day of year [0,365].
         *   int    tm_isdst Daylight Savings flag.
         */
        datetime_set_date(bdate, (uint16_t)tblock->tm_year + 1900,
                          (uint8_t)tblock->tm_mon + 1,
                          (uint8_t)tblock->tm_mday);
        datetime_set_time(btime, (uint8_t)tblock->tm_hour,
                          (uint8_t)tblock->tm_min, (uint8_t)tblock->tm_sec,
                          (uint8_t)(tv.tv_usec / 10000));
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
            *utc_offset_minutes = timezone / 60;
        }
    }

    return status;
}

/**
 * initialize the date time
 */
void datetime_init(void)
{
    /* nothing to do */
}
