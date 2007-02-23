/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef DATE_TIME_H
#define DATE_TIME_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BACNET_WEEKDAY_MONDAY = 1,
    BACNET_WEEKDAY_TUESDAY = 2,
    BACNET_WEEKDAY_WEDNESDAY = 3,
    BACNET_WEEKDAY_THURSDAY = 4,
    BACNET_WEEKDAY_FRIDAY = 5,
    BACNET_WEEKDAY_SATURDAY = 6,
    BACNET_WEEKDAY_SUNDAY = 7
} BACNET_WEEKDAY;

/* date */
typedef struct BACnet_Date {
    uint16_t year;              /* AD */
    uint8_t month;              /* 1=Jan */
    uint8_t day;                /* 1..31 */
    uint8_t wday;               /* 1=Monday-7=Sunday */
} BACNET_DATE;

/* time */
typedef struct BACnet_Time {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t hundredths;
} BACNET_TIME;

typedef struct BACnet_DateTime {
    BACNET_DATE date;
    BACNET_TIME time;
} BACNET_DATE_TIME;

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

    /* utility initialization functions */
    void datetime_set_date(BACNET_DATE * bdate,
        uint16_t year, uint8_t month, uint8_t day);
    void datetime_set_time(BACNET_TIME * btime,
        uint8_t hour, uint8_t minute, uint8_t seconds, uint8_t hundredths);
    void datetime_set(BACNET_DATE_TIME * bdatetime,
        BACNET_DATE * bdate, BACNET_TIME * btime);
    void datetime_set_values(BACNET_DATE_TIME * bdatetime,
        uint16_t year, uint8_t month, uint8_t day,
        uint8_t hour, uint8_t minute, uint8_t seconds, uint8_t hundredths);

    /* utility comparison functions:
       if the date/times are the same, return is 0
       if date1 is before date2, returns negative
       if date1 is after date2, returns positive */
    int datetime_compare_date(BACNET_DATE * date1, BACNET_DATE * date2);
    int datetime_compare_time(BACNET_TIME * time1, BACNET_TIME * time2);
    int datetime_compare(BACNET_DATE_TIME * datetime1,
        BACNET_DATE_TIME * datetime2);

    /* utility copy functions */
    void datetime_copy_date(BACNET_DATE * date1, BACNET_DATE * date2);
    void datetime_copy_time(BACNET_TIME * time1, BACNET_TIME * time2);
    void datetime_copy(BACNET_DATE_TIME * datetime1,
        BACNET_DATE_TIME * datetime2);

    /* utility add function */
    void datetime_add_minutes(BACNET_DATE_TIME * bdatetime,
        uint32_t minutes);

    /* date and time wildcards */
    bool datetime_wildcard(BACNET_DATE_TIME * bdatetime);
    void datetime_wildcard_set(BACNET_DATE_TIME * bdatetime);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* DATE_TIME_H */
