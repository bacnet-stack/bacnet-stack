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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "bacport.h"
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
struct timezone {
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime; /* type of dst correction */
};

/*************************************************************************
 * Description: simulate the gettimeofday Linux function
 * Returns: zero
 * Note: The resolution of GetSystemTimeAsFileTime() is 15625 microseconds.
 * The resolution of _ftime() is about 16 milliseconds.
 * To get microseconds accuracy we need to use QueryPerformanceCounter or
 * timeGetTime for the elapsed time.
 *************************************************************************/

int gettimeofday(struct timeval *tp, void *tzp)
{
    static int tzflag = 0;
    struct timezone *tz;
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
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}
#endif

/**
 * @brief Set offset from the system clock.
 * @param bdate BACnet Date structure to hold local time
 * @param btime BACnet Time structure to hold local time
 * @param utc - True for UTC sync, False for Local time
 * @return True if time is set
 */
void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
{
    (void)bdate;
    (void)btime;
    (void)utc;
    return;
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
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    bool status = false;
    struct tm *tblock = NULL;
#if defined(_MSC_VER)
    time_t tTemp;
#else
    struct timeval tv;
#endif
#if defined(_MSC_VER)
    time(&tTemp);
    tblock = (struct tm *)localtime(&tTemp);
#else
    if (gettimeofday(&tv, NULL) == 0) {
        tblock = (struct tm *)localtime((const time_t *)&tv.tv_sec);
    }
#endif

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
        datetime_set_date(
            bdate, (uint16_t)tblock->tm_year + 1900,
            (uint8_t)tblock->tm_mon + 1, (uint8_t)tblock->tm_mday);
#if !defined(_MSC_VER)
        datetime_set_time(
            btime, (uint8_t)tblock->tm_hour, (uint8_t)tblock->tm_min,
            (uint8_t)tblock->tm_sec, (uint8_t)(tv.tv_usec / 10000));
#else
        datetime_set_time(
            btime, (uint8_t)tblock->tm_hour, (uint8_t)tblock->tm_min,
            (uint8_t)tblock->tm_sec, 0);
#endif
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
