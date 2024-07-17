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
#include "bacnet/basic/service/h_ts.h"
#include "bacport.h"
#include "bacnet/datetime.h"


/**
 * @brief Set offset from the system clock.
 * @param bdate BACnet Date structure to hold local time
 * @param btime BACnet Time structure to hold local time
 * @param utc - True for UTC sync, False for Local time
 * @return True if time is set
 */
void datetime_timesync(
    BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
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
bool datetime_local(BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    bool status = false;
    struct tm *tblock = NULL;
    struct timeval tv;

    if (gettimeofday(&tv, NULL) == 0) {
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
	 *   long   tm_gmtoff offset from UTC in seconds
         */
        datetime_set_date(bdate, (uint16_t)tblock->tm_year + 1900,
            (uint8_t)tblock->tm_mon + 1, (uint8_t)tblock->tm_mday);
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
            /* tm_gmtoff is set to the difference, in seconds,
                between Coordinated Universal Time (UTC) and
                local standard time */
            *utc_offset_minutes = tblock->tm_gmtoff / 60;
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
