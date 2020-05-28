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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "bacnet/datetime.h"
#include "bacnet/bacdcode.h"

/** @file datetime.c  Manipulate BACnet Date and Time values */

/* define our epic beginnings */
#define BACNET_EPOCH_YEAR 1900
/* 1/1/1900 is a Monday */
#define BACNET_EPOCH_DOW BACNET_WEEKDAY_MONDAY

/* BACnet Date */
/* year = years since 1900 */
/* month 1=Jan */
/* day = day of month 1..31 */
/* wday 1=Monday...7=Sunday */

/* Wildcards:
  A value of X'FF' in any of the four octets
  shall indicate that the value is unspecified.
  If all four octets = X'FF', the corresponding
  time or date may be interpreted as "any" or "don't care"
*/

bool datetime_is_leap_year(uint16_t year)
{
    if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0)) {
        return (true);
    } else {
        return (false);
    }
}

uint8_t datetime_month_days(uint16_t year, uint8_t month)
{
    /* note: start with a zero in the first element to save us from a
       month - 1 calculation in the lookup */
    int month_days[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    /* return value */
    uint8_t days = 0;

    /* February */
    if ((month == 2) && datetime_is_leap_year(year)) {
        days = 29;
    } else if (month >= 1 && month <= 12) {
        days = (uint8_t)month_days[month];
    }

    return days;
}

bool datetime_ymd_is_valid(uint16_t year, uint8_t month, uint8_t day)
{
    bool status = false; /* true if value date */
    uint8_t monthdays = 0; /* days in a month */

    monthdays = datetime_month_days(year, month);
    if ((year >= BACNET_EPOCH_YEAR) && (monthdays > 0) && (day >= 1) &&
        (day <= monthdays)) {
        status = true;
    }

    return status;
}

bool datetime_date_is_valid(BACNET_DATE *bdate)
{
    bool status = false; /* true if value date */

    if (bdate) {
        status = datetime_ymd_is_valid(bdate->year, bdate->month, bdate->day);
    }

    return status;
}

static uint32_t day_of_year(uint16_t year, uint8_t month, uint8_t day)
{
    uint32_t days = 0; /* return value */
    uint8_t months = 0; /* loop counter for months */

    if (datetime_ymd_is_valid(year, month, day)) {
        for (months = 1; months < month; months++) {
            days += datetime_month_days(year, months);
        }
        days += day;
    }

    return (days);
}

static void day_of_year_into_md(
    uint32_t days, uint16_t year, uint8_t *pMonth, uint8_t *pDay)
{
    uint8_t month = 1;
    uint8_t day = 0;

    while (days > (uint32_t)datetime_month_days(year, month)) {
        days -= datetime_month_days(year, month);
        month++;
    }

    day = (uint8_t)(day + days);

    if (pMonth) {
        *pMonth = month;
    }
    if (pDay) {
        *pDay = day;
    }

    return;
}

void datetime_day_of_year_into_date(
    uint32_t days, uint16_t year, BACNET_DATE *bdate)
{
    uint8_t month = 0;
    uint8_t day = 0;

    day_of_year_into_md(days, year, &month, &day);
    datetime_set_date(bdate, year, month, day);
}

uint32_t datetime_day_of_year(BACNET_DATE *bdate)
{
    uint32_t days = 0;

    if (bdate) {
        days = day_of_year(bdate->year, bdate->month, bdate->day);
    }

    return days;
}

static uint32_t days_since_epoch(uint16_t year, uint8_t month, uint8_t day)
{
    uint32_t days = 0; /* return value */
    uint16_t years = 0; /* loop counter for years */

    if (datetime_ymd_is_valid(year, month, day)) {
        for (years = BACNET_EPOCH_YEAR; years < year; years++) {
            days += 365;
            if (datetime_is_leap_year(years)) {
                days++;
            }
        }
        days += day_of_year(year, month, day);
        /* 'days since' is one less */
        days -= 1;
    }

    return (days);
}

uint32_t datetime_days_since_epoch(BACNET_DATE *bdate)
{
    uint32_t days = 0;

    if (bdate) {
        days = days_since_epoch(bdate->year, bdate->month, bdate->day);
    }

    return days;
}

static void days_since_epoch_into_ymd(
    uint32_t days, uint16_t *pYear, uint8_t *pMonth, uint8_t *pDay)
{
    uint16_t year = BACNET_EPOCH_YEAR;
    uint8_t month = 1;
    uint8_t day = 1;

    while (days >= 365) {
        if ((datetime_is_leap_year(year)) && (days == 365)) {
            break;
        }
        days -= 365;
        if (datetime_is_leap_year(year)) {
            --days;
        }
        year++;
    }

    while (days >= (uint32_t)datetime_month_days(year, month)) {
        days -= datetime_month_days(year, month);
        month++;
    }

    day = (uint8_t)(day + days);

    if (pYear) {
        *pYear = year;
    }
    if (pMonth) {
        *pMonth = month;
    }
    if (pDay) {
        *pDay = day;
    }

    return;
}

void datetime_days_since_epoch_into_date(uint32_t days, BACNET_DATE *bdate)
{
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;

    days_since_epoch_into_ymd(days, &year, &month, &day);
    datetime_set_date(bdate, year, month, day);
}

/* Jan 1, 1900 is a Monday */
/* wday 1=Monday...7=Sunday */
uint8_t datetime_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    uint8_t dow = (uint8_t)BACNET_EPOCH_DOW;

    dow += (days_since_epoch(year, month, day) % 7);

    return dow;
}

bool datetime_time_is_valid(BACNET_TIME *btime)
{
    bool status = false;

    if (btime) {
        if ((btime->hour < 24) && (btime->min < 60) && (btime->sec < 60) &&
            (btime->hundredths < 100)) {
            status = true;
        }
    }

    return status;
}

/**
 * Determines if a given date and time is valid for calendar
 *
 * @param bdate - pointer to a BACNET_DATE structure
 * @param btime - pointer to a BACNET_TIME structure
 *
 * @return true if the date and time are valid
 */
bool datetime_is_valid(BACNET_DATE *bdate, BACNET_TIME *btime)
{
    return datetime_date_is_valid(bdate) && datetime_time_is_valid(btime);
}

/**
 * If the date1 is the same as date2, return is 0.
 * If date1 is after date2, returns positive.
 * if date1 is before date2, returns negative.
 *
 * @param date1 - Pointer to a BACNET_DATE structure
 * @param date2 - Pointer to a BACNET_DATE structure
 *
 * @return -/0/+
 */
int datetime_compare_date(BACNET_DATE *date1, BACNET_DATE *date2)
{
    int diff = 0;

    if (date1 && date2) {
        diff = (int)date1->year - (int)date2->year;
        if (diff == 0) {
            diff = (int)date1->month - (int)date2->month;
            if (diff == 0) {
                diff = (int)date1->day - (int)date2->day;
            }
        }
    }

    return diff;
}

/**
 * If the time1 is the same as time2, return is 0.
 * If time1 is after time2, returns positive.
 * if time1 is before time2, returns negative.
 *
 * @param time1 - Pointer to a BACNET_TIME structure
 * @param time2 - Pointer to a BACNET_TIME structure
 *
 * @return -/0/+
 */
int datetime_compare_time(BACNET_TIME *time1, BACNET_TIME *time2)
{
    int diff = 0;

    if (time1 && time2) {
        diff = (int)time1->hour - (int)time2->hour;
        if (diff == 0) {
            diff = (int)time1->min - (int)time2->min;
            if (diff == 0) {
                diff = (int)time1->sec - (int)time2->sec;
                if (diff == 0) {
                    diff = (int)time1->hundredths - (int)time2->hundredths;
                }
            }
        }
    }

    return diff;
}

/**
 * If the datetime1 is the same datetime2, return is 0.
 * If datetime1 is after datetime2, returns positive.
 * if datetime1 is before datetime2, returns negative.
 *
 * @param datetime1 - Pointer to a BACNET_DATE_TIME structure
 * @param datetime2 - Pointer to a BACNET_DATE_TIME structure
 *
 * @return -/0/+
 */
int datetime_compare(BACNET_DATE_TIME *datetime1, BACNET_DATE_TIME *datetime2)
{
    int diff = 0;

    diff = datetime_compare_date(&datetime1->date, &datetime2->date);
    if (diff == 0) {
        diff = datetime_compare_time(&datetime1->time, &datetime2->time);
    }

    return diff;
}

int datetime_wildcard_compare_date(BACNET_DATE *date1, BACNET_DATE *date2)
{
    int diff = 0;

    if (date1 && date2) {
        if ((date1->year != 1900 + 0xFF) && (date2->year != 1900 + 0xFF)) {
            diff = (int)date1->year - (int)date2->year;
        }
        if (diff == 0) {
            if ((date1->month != 0xFF) && (date2->month != 0xFF)) {
                diff = (int)date1->month - (int)date2->month;
            }
            if (diff == 0) {
                if ((date1->day != 0xFF) && (date2->day != 0xFF)) {
                    diff = (int)date1->day - (int)date2->day;
                }
                /* we ignore weekday in comparison */
            }
        }
    }

    return diff;
}

int datetime_wildcard_compare_time(BACNET_TIME *time1, BACNET_TIME *time2)
{
    int diff = 0;

    if (time1 && time2) {
        if ((time1->hour != 0xFF) && (time2->hour != 0xFF)) {
            diff = (int)time1->hour - (int)time2->hour;
        }
        if (diff == 0) {
            if ((time1->min != 0xFF) && (time2->min != 0xFF)) {
                diff = (int)time1->min - (int)time2->min;
            }
            if (diff == 0) {
                if ((time1->sec != 0xFF) && (time2->sec != 0xFF)) {
                    diff = (int)time1->sec - (int)time2->sec;
                }
                if (diff == 0) {
                    if ((time1->hundredths != 0xFF) &&
                        (time2->hundredths != 0xFF)) {
                        diff = (int)time1->hundredths - (int)time2->hundredths;
                    }
                }
            }
        }
    }

    return diff;
}

int datetime_wildcard_compare(
    BACNET_DATE_TIME *datetime1, BACNET_DATE_TIME *datetime2)
{
    int diff = 0;

    diff = datetime_wildcard_compare_date(&datetime1->date, &datetime2->date);
    if (diff == 0) {
        diff =
            datetime_wildcard_compare_time(&datetime1->time, &datetime2->time);
    }

    return diff;
}

void datetime_copy_date(BACNET_DATE *dest_date, BACNET_DATE *src_date)
{
    if (dest_date && src_date) {
        dest_date->year = src_date->year;
        dest_date->month = src_date->month;
        dest_date->day = src_date->day;
        dest_date->wday = src_date->wday;
    }
}

void datetime_copy_time(BACNET_TIME *dest_time, BACNET_TIME *src_time)
{
    if (dest_time && src_time) {
        dest_time->hour = src_time->hour;
        dest_time->min = src_time->min;
        dest_time->sec = src_time->sec;
        dest_time->hundredths = src_time->hundredths;
    }
}

void datetime_copy(
    BACNET_DATE_TIME *dest_datetime, BACNET_DATE_TIME *src_datetime)
{
    datetime_copy_time(&dest_datetime->time, &src_datetime->time);
    datetime_copy_date(&dest_datetime->date, &src_datetime->date);
}

void datetime_set_date(
    BACNET_DATE *bdate, uint16_t year, uint8_t month, uint8_t day)
{
    if (bdate) {
        bdate->year = year;
        bdate->month = month;
        bdate->day = day;
        bdate->wday = datetime_day_of_week(year, month, day);
    }
}

void datetime_set_time(BACNET_TIME *btime,
    uint8_t hour,
    uint8_t minute,
    uint8_t seconds,
    uint8_t hundredths)
{
    if (btime) {
        btime->hour = hour;
        btime->min = minute;
        btime->sec = seconds;
        btime->hundredths = hundredths;
    }
}

void datetime_set(
    BACNET_DATE_TIME *bdatetime, BACNET_DATE *bdate, BACNET_TIME *btime)
{
    if (bdate && btime && bdatetime) {
        bdatetime->time.hour = btime->hour;
        bdatetime->time.min = btime->min;
        bdatetime->time.sec = btime->sec;
        bdatetime->time.hundredths = btime->hundredths;
        bdatetime->date.year = bdate->year;
        bdatetime->date.month = bdate->month;
        bdatetime->date.day = bdate->day;
        bdatetime->date.wday = bdate->wday;
    }
}

void datetime_set_values(BACNET_DATE_TIME *bdatetime,
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t seconds,
    uint8_t hundredths)
{
    if (bdatetime) {
        bdatetime->date.year = year;
        bdatetime->date.month = month;
        bdatetime->date.day = day;
        bdatetime->date.wday = datetime_day_of_week(year, month, day);
        bdatetime->time.hour = hour;
        bdatetime->time.min = minute;
        bdatetime->time.sec = seconds;
        bdatetime->time.hundredths = hundredths;
    }
}

static uint32_t seconds_since_midnight(
    uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    return ((hours * 60 * 60) + (minutes * 60) + seconds);
}

static uint16_t minutes_since_midnight(uint8_t hours, uint8_t minutes)
{
    return ((hours * 60) + minutes);
}

static void seconds_since_midnight_into_hms(
    uint32_t seconds, uint8_t *pHours, uint8_t *pMinutes, uint8_t *pSeconds)
{
    uint8_t hour = 0;
    uint8_t minute = 0;

    hour = (uint8_t)(seconds / (60 * 60));
    seconds -= (hour * 60 * 60);
    minute = (uint8_t)(seconds / 60);
    seconds -= (minute * 60);

    if (pHours) {
        *pHours = hour;
    }
    if (pMinutes) {
        *pMinutes = minute;
    }
    if (pSeconds) {
        *pSeconds = (uint8_t)seconds;
    }
}

/**
 * @brief Converts the number of seconds since midnight into BACnet Time
 * @param seconds since midnight
 * @param btime [in] BACNET_TIME containing the time to convert
 */
void datetime_seconds_since_midnight_into_time(
    uint32_t seconds,
    BACNET_TIME *btime)
{
    if (btime) {
        seconds_since_midnight_into_hms(seconds,
            &btime->hour, &btime->min, &btime->sec);
        btime->hundredths = 0;
    }
}

/** Calculates the number of seconds since midnight
 *
 * @param btime [in] BACNET_TIME containing the time to convert
 *
 * @return seconds since midnight
 */
uint32_t datetime_seconds_since_midnight(BACNET_TIME *btime)
{
    uint32_t seconds = 0;

    if (btime) {
        seconds = seconds_since_midnight(btime->hour, btime->min, btime->sec);
    }

    return seconds;
}

/** Calculates the number of minutes since midnight
 *
 * @param btime [in] BACNET_TIME containing the time to convert
 *
 * @return minutes since midnight
 */
uint16_t datetime_minutes_since_midnight(BACNET_TIME *btime)
{
    uint32_t minutes = 0;

    if (btime) {
        minutes = minutes_since_midnight(btime->hour, btime->min);
    }

    return minutes;
}

/** Utility to add or subtract minutes to a BACnet DateTime structure
 *
 * @param bdatetime [in] the starting date and time
 * @param minutes [in] number of minutes to add or subtract from the time
 */
void datetime_add_minutes(BACNET_DATE_TIME *bdatetime, int32_t minutes)
{
    uint32_t bdatetime_minutes = 0;
    uint32_t bdatetime_days = 0;
    int32_t days = 0;

    /* convert bdatetime to seconds and days */
    bdatetime_minutes = seconds_since_midnight(bdatetime->time.hour,
                            bdatetime->time.min, bdatetime->time.sec) /
        60;
    bdatetime_days = datetime_days_since_epoch(&bdatetime->date);

    /* more minutes than in a day? */
    days = minutes / (24 * 60);
    bdatetime_days += days;
    minutes -= (days * 24 * 60);
    /* less minutes - previous day? */
    if (minutes < 0) {
        /* convert to positive for easier math */
        minutes *= -1;
        if ((uint32_t)minutes > bdatetime_minutes) {
            /* previous day */
            bdatetime_days -= 1;
            bdatetime_minutes += ((24 * 60) - minutes);
        } else {
            bdatetime_minutes -= minutes;
            days = bdatetime_minutes / (24 * 60);
            bdatetime_days += days;
            bdatetime_minutes -= (days * 24 * 60);
        }
    } else {
        /* more days than current datetime? */
        bdatetime_minutes += minutes;
        days = bdatetime_minutes / (24 * 60);
        bdatetime_days += days;
        bdatetime_minutes -= (days * 24 * 60);
    }

    /* convert bdatetime from seconds and days */
    seconds_since_midnight_into_hms(bdatetime_minutes * 60,
        &bdatetime->time.hour, &bdatetime->time.min, NULL);
    datetime_days_since_epoch_into_date(bdatetime_days, &bdatetime->date);
}

#ifdef UINT64_MAX
/**
 * @brief Calculates the number of seconds since epoch
 * @param bdatetime [in] the starting date and time
 * @return seconds since midnight
 */
uint64_t datetime_seconds_since_epoch(
    BACNET_DATE_TIME * bdatetime)
{
    uint64_t seconds = 0;
    uint32_t days = 0;

    if (bdatetime) {
        days = datetime_days_since_epoch(&bdatetime->date);
        seconds = seconds_since_midnight(24, 0, 0);
        seconds *= days;
        seconds += datetime_seconds_since_midnight(&bdatetime->time);
    }

    return seconds;
}
#endif

#ifdef UINT64_MAX
/**
 * @brief Calculates the number of seconds since epoch
 * @param bdatetime [in] the starting date and time
 * @return seconds since midnight
 */
void datetime_since_epoch_seconds(
    BACNET_DATE_TIME * bdatetime,
    uint64_t seconds)
{
    uint32_t seconds_after_midnight = 0;
    uint32_t days = 0;
    uint32_t day_seconds = 0;

    if (bdatetime) {
        day_seconds = seconds_since_midnight(24, 0, 0);
        days = seconds / day_seconds;
        seconds_after_midnight = seconds - (days * day_seconds);
        datetime_seconds_since_midnight_into_time(
            seconds_after_midnight, &bdatetime->time);
        datetime_days_since_epoch_into_date(days, &bdatetime->date);
    }
}
#endif

/* Returns true if year is a wildcard */
bool datetime_wildcard_year(BACNET_DATE *bdate)
{
    bool wildcard_present = false;

    if (bdate) {
        if (bdate->year == (BACNET_EPOCH_YEAR + 0xFF)) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Sets the year as a wildcard */
void datetime_wildcard_year_set(BACNET_DATE *bdate)
{
    if (bdate) {
        bdate->year = BACNET_EPOCH_YEAR + 0xFF;
    }
}

/* Returns true if month is a wildcard */
bool datetime_wildcard_month(BACNET_DATE *bdate)
{
    bool wildcard_present = false;

    if (bdate) {
        if (bdate->month == 0xFF) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Sets the month as a wildcard */
void datetime_wildcard_month_set(BACNET_DATE *bdate)
{
    if (bdate) {
        bdate->month = 0xFF;
    }
}

/* Returns true if day is a wildcard */
bool datetime_wildcard_day(BACNET_DATE *bdate)
{
    bool wildcard_present = false;

    if (bdate) {
        if (bdate->day == 0xFF) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Sets the day as a wildcard */
void datetime_wildcard_day_set(BACNET_DATE *bdate)
{
    if (bdate) {
        bdate->day = 0xFF;
    }
}

/* Returns true if weekday is a wildcard */
bool datetime_wildcard_weekday(BACNET_DATE *bdate)
{
    bool wildcard_present = false;

    if (bdate) {
        if (bdate->wday == 0xFF) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Sets the weekday as a wildcard */
void datetime_wildcard_weekday_set(BACNET_DATE *bdate)
{
    if (bdate) {
        bdate->wday = 0xFF;
    }
}

/* Returns true if hour is a wildcard */
bool datetime_wildcard_hour(BACNET_TIME *btime)
{
    bool wildcard_present = false;

    if (btime) {
        if (btime->hour == 0xFF) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Sets the hour as a wildcard */
void datetime_wildcard_hour_set(BACNET_TIME *btime)
{
    if (btime) {
        btime->hour = 0xFF;
    }
}

/* Returns true if minute is a wildcard */
bool datetime_wildcard_minute(BACNET_TIME *btime)
{
    bool wildcard_present = false;

    if (btime) {
        if (btime->min == 0xFF) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Sets the minute as a wildcard */
void datetime_wildcard_minute_set(BACNET_TIME *btime)
{
    if (btime) {
        btime->min = 0xFF;
    }
}

/* Returns true if seconds is wildcard */
bool datetime_wildcard_second(BACNET_TIME *btime)
{
    bool wildcard_present = false;

    if (btime) {
        if (btime->sec == 0xFF) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Sets the seconds as a wildcard */
void datetime_wildcard_second_set(BACNET_TIME *btime)
{
    if (btime) {
        btime->sec = 0xFF;
    }
}

/* Returns true if hundredths is a wildcard */
bool datetime_wildcard_hundredths(BACNET_TIME *btime)
{
    bool wildcard_present = false;

    if (btime) {
        if (btime->hundredths == 0xFF) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Sets the hundredths as a wildcard */
void datetime_wildcard_hundredths_set(BACNET_TIME *btime)
{
    if (btime) {
        btime->hundredths = 0xFF;
    }
}

bool datetime_wildcard(BACNET_DATE_TIME *bdatetime)
{
    bool wildcard_present = false;

    if (bdatetime) {
        if (datetime_wildcard_year(&bdatetime->date) &&
            datetime_wildcard_month(&bdatetime->date) &&
            datetime_wildcard_day(&bdatetime->date) &&
            datetime_wildcard_weekday(&bdatetime->date) &&
            datetime_wildcard_hour(&bdatetime->time) &&
            datetime_wildcard_minute(&bdatetime->time) &&
            datetime_wildcard_second(&bdatetime->time) &&
            datetime_wildcard_hundredths(&bdatetime->time)) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Returns true if any type of wildcard is present except for day of week
 * on it's own.  Also checks for special day and month values.  Used in
 * trendlog object.
 */
bool datetime_wildcard_present(BACNET_DATE_TIME *bdatetime)
{
    bool wildcard_present = false;

    if (bdatetime) {
        if (datetime_wildcard_year(&bdatetime->date) ||
            (bdatetime->date.month > 12) || (bdatetime->date.day > 31) ||
            datetime_wildcard_hour(&bdatetime->time) ||
            datetime_wildcard_minute(&bdatetime->time) ||
            datetime_wildcard_second(&bdatetime->time) ||
            datetime_wildcard_hundredths(&bdatetime->time)) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

void datetime_date_wildcard_set(BACNET_DATE *bdate)
{
    if (bdate) {
        bdate->year = BACNET_EPOCH_YEAR + 0xFF;
        bdate->month = 0xFF;
        bdate->day = 0xFF;
        bdate->wday = 0xFF;
    }
}

void datetime_time_wildcard_set(BACNET_TIME *btime)
{
    if (btime) {
        btime->hour = 0xFF;
        btime->min = 0xFF;
        btime->sec = 0xFF;
        btime->hundredths = 0xFF;
    }
}

void datetime_wildcard_set(BACNET_DATE_TIME *bdatetime)
{
    if (bdatetime) {
        datetime_date_wildcard_set(&bdatetime->date);
        datetime_time_wildcard_set(&bdatetime->time);
    }
}

/**
 * @brief Converts UTC to local time
 * @param local_time - the BACnet Date and Time structure to hold local time
 * @param utc_time - the BACnet Date and Time structure to hold UTC time
 * @param utc_offset_minutes - number of minutes offset from UTC
 *  Values are positive East of UTC and negative West of UTC
 *  For example, -6*60 represents 6.00 hours West of UTC
 * @param dst_adjust_minutes - number of minutes to adjust local time
 *  Values are positive East of UTC and negative West of UTC
 * @return true if the time is converted
 */
bool datetime_utc_to_local(BACNET_DATE_TIME *local_time,
    BACNET_DATE_TIME *utc_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes)
{
    bool status = false;

    if (local_time && utc_time) {
        datetime_copy(local_time, utc_time);
        utc_offset_minutes *= -1;
        datetime_add_minutes(local_time, utc_offset_minutes);
        if (dst_adjust_minutes != 0) {
            dst_adjust_minutes *= -1;
            datetime_add_minutes(local_time, dst_adjust_minutes);
        }
        status = true;
    }

    return status;
}

/**
 * @brief Converts UTC to local time
 * @param utc_time - the BACnet Date and Time structure to hold UTC time
 * @param local_time - the BACnet Date and Time structure to hold local time
 * @param utc_offset_minutes - number of minutes offset from UTC
 *  Values are positive East of UTC and negative West of UTC
 *  For example, -6*60 represents 6.00 hours West of UTC
 * @param dst_adjust_minutes - number of minutes to adjust local time
 *  Values are positive East of UTC and negative West of UTC
 * @return true if the time is converted
 */
bool datetime_local_to_utc(BACNET_DATE_TIME *utc_time,
    BACNET_DATE_TIME *local_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes)
{
    bool status = false;

    if (local_time && utc_time) {
        datetime_copy(utc_time, local_time);
        datetime_add_minutes(utc_time, utc_offset_minutes);
        if (dst_adjust_minutes != 0) {
            datetime_add_minutes(utc_time, dst_adjust_minutes);
        }
        status = true;
    }

    return status;
}

int bacapp_encode_datetime(uint8_t *apdu, BACNET_DATE_TIME *value)
{
    int len = 0;
    int apdu_len = 0;

    if (apdu && value) {
        len = encode_application_date(&apdu[0], &value->date);
        apdu_len += len;

        len = encode_application_time(&apdu[apdu_len], &value->time);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_encode_context_datetime(
    uint8_t *apdu, uint8_t tag_number, BACNET_DATE_TIME *value)
{
    int len = 0;
    int apdu_len = 0;

    if (apdu && value) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;

        len = bacapp_encode_datetime(&apdu[apdu_len], value);
        apdu_len += len;

        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_datetime(uint8_t *apdu, BACNET_DATE_TIME *value)
{
    int len = 0;
    int section_len;

    if (-1 ==
        (section_len = decode_application_date(&apdu[len], &value->date))) {
        return -1;
    }
    len += section_len;

    if (-1 ==
        (section_len = decode_application_time(&apdu[len], &value->time))) {
        return -1;
    }

    len += section_len;

    return len;
}

int bacapp_decode_context_datetime(
    uint8_t *apdu, uint8_t tag_number, BACNET_DATE_TIME *value)
{
    int apdu_len = 0;
    int len;

    if (decode_is_opening_tag_number(&apdu[apdu_len], tag_number)) {
        apdu_len++;
    } else {
        return -1;
    }

    if (-1 == (len = bacapp_decode_datetime(&apdu[apdu_len], value))) {
        return -1;
    } else {
        apdu_len += len;
    }

    if (decode_is_closing_tag_number(&apdu[apdu_len], tag_number)) {
        apdu_len++;
    } else {
        return -1;
    }
    return apdu_len;
}

#ifdef BAC_TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

static void datetime_print(const char *title,
    BACNET_DATE_TIME *bdatetime)
{
    printf("%s: %04u/%02u/%02u %02u:%02u:%02u.%03u\n",
        title,
        (unsigned int)bdatetime->date.year,
        (unsigned int)bdatetime->date.month,
        (unsigned int)bdatetime->date.wday,
        (unsigned int)bdatetime->time.hour,
        (unsigned int)bdatetime->time.min,
        (unsigned int)bdatetime->time.sec,
        (unsigned int)bdatetime->time.hundredths);
}

static void testBACnetDateTimeWildcard(Test *pTest)
{
    BACNET_DATE_TIME bdatetime;
    bool status = false;

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    status = datetime_wildcard(&bdatetime);
    ct_test(pTest, status == false);

    datetime_wildcard_set(&bdatetime);
    status = datetime_wildcard(&bdatetime);
    ct_test(pTest, status == true);
}

static void testBACnetDateTimeAdd(Test *pTest)
{
    BACNET_DATE_TIME bdatetime, test_bdatetime;
    uint32_t minutes = 0;
    int diff = 0;

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_copy(&test_bdatetime, &bdatetime);
    datetime_add_minutes(&bdatetime, minutes);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_add_minutes(&bdatetime, 60);
    datetime_set_values(&test_bdatetime, BACNET_EPOCH_YEAR, 1, 1, 1, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_add_minutes(&bdatetime, (24 * 60));
    datetime_set_values(&test_bdatetime, BACNET_EPOCH_YEAR, 1, 2, 0, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_add_minutes(&bdatetime, (31 * 24 * 60));
    datetime_set_values(&test_bdatetime, BACNET_EPOCH_YEAR, 2, 1, 0, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);

    datetime_set_values(&bdatetime, 2013, 6, 6, 23, 59, 59, 0);
    datetime_add_minutes(&bdatetime, 60);
    datetime_set_values(&test_bdatetime, 2013, 6, 7, 0, 59, 59, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);

    datetime_set_values(&bdatetime, 2013, 6, 6, 0, 59, 59, 0);
    datetime_add_minutes(&bdatetime, -60);
    datetime_set_values(&test_bdatetime, 2013, 6, 5, 23, 59, 59, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);
}

static void testBACnetDateTimeSeconds(Test *pTest)
{
    uint8_t hour = 0, minute = 0, second = 0;
    uint8_t test_hour = 0, test_minute = 0, test_second = 0;
    uint32_t seconds = 0, test_seconds;

    for (hour = 0; hour < 24; hour++) {
        for (minute = 0; minute < 60; minute += 3) {
            for (second = 0; second < 60; second += 17) {
                seconds = seconds_since_midnight(hour, minute, second);
                seconds_since_midnight_into_hms(
                    seconds, &test_hour, &test_minute, &test_second);
                test_seconds =
                    seconds_since_midnight(test_hour, test_minute, test_second);
                ct_test(pTest, seconds == test_seconds);
            }
        }
    }
}

static void testBACnetDate(Test *pTest)
{
    BACNET_DATE bdate1, bdate2;
    int diff = 0;

    datetime_set_date(&bdate1, BACNET_EPOCH_YEAR, 1, 1);
    datetime_copy_date(&bdate2, &bdate1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff == 0);
    datetime_set_date(&bdate2, BACNET_EPOCH_YEAR, 1, 2);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, BACNET_EPOCH_YEAR, 2, 1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 1901, 1, 1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);

    /* midpoint */
    datetime_set_date(&bdate1, 2007, 7, 15);
    datetime_copy_date(&bdate2, &bdate1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff == 0);
    datetime_set_date(&bdate2, 2007, 7, 14);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2007, 7, 1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2007, 7, 31);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 2007, 8, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 2007, 12, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 2007, 6, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2007, 1, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2006, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, BACNET_EPOCH_YEAR, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2008, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 2154, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);

    return;
}

static void testBACnetTime(Test *pTest)
{
    BACNET_TIME btime1, btime2;
    int diff = 0;

    datetime_set_time(&btime1, 0, 0, 0, 0);
    datetime_copy_time(&btime2, &btime1);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff == 0);

    datetime_set_time(&btime1, 23, 59, 59, 99);
    datetime_copy_time(&btime2, &btime1);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff == 0);

    /* midpoint */
    datetime_set_time(&btime1, 12, 30, 30, 50);
    datetime_copy_time(&btime2, &btime1);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff == 0);
    datetime_set_time(&btime2, 12, 30, 30, 51);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff < 0);
    datetime_set_time(&btime2, 12, 30, 31, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff < 0);
    datetime_set_time(&btime2, 12, 31, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff < 0);
    datetime_set_time(&btime2, 13, 30, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff < 0);

    datetime_set_time(&btime2, 12, 30, 30, 49);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff > 0);
    datetime_set_time(&btime2, 12, 30, 29, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff > 0);
    datetime_set_time(&btime2, 12, 29, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff > 0);
    datetime_set_time(&btime2, 11, 30, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff > 0);

    return;
}

static void testBACnetDateTime(Test *pTest)
{
    BACNET_DATE_TIME bdatetime1, bdatetime2;
    BACNET_DATE bdate;
    BACNET_TIME btime;
    int diff = 0;

    datetime_set_values(&bdatetime1, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_copy(&bdatetime2, &bdatetime1);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff == 0);
    datetime_set_time(&btime, 0, 0, 0, 0);
    datetime_set_date(&bdate, BACNET_EPOCH_YEAR, 1, 1);
    datetime_set(&bdatetime1, &bdate, &btime);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff == 0);

    /* midpoint */
    /* if datetime1 is before datetime2, returns negative */
    datetime_set_values(&bdatetime1, 2000, 7, 15, 12, 30, 30, 50);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 30, 51);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 31, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 31, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 13, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 16, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 8, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2001, 7, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 30, 49);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 29, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 29, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 11, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 7, 14, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 6, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 1999, 7, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);

    return;
}

static void testWildcardDateTime(Test *pTest)
{
    BACNET_DATE_TIME bdatetime1, bdatetime2;
    BACNET_DATE bdate;
    BACNET_TIME btime;
    int diff = 0;

    datetime_wildcard_set(&bdatetime1);
    ct_test(pTest, datetime_wildcard(&bdatetime1));
    ct_test(pTest, datetime_wildcard_present(&bdatetime1));
    datetime_copy(&bdatetime2, &bdatetime1);
    diff = datetime_wildcard_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff == 0);
    datetime_time_wildcard_set(&btime);
    datetime_date_wildcard_set(&bdate);
    datetime_set(&bdatetime1, &bdate, &btime);
    diff = datetime_wildcard_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff == 0);

    return;
}

static void testDayOfYear(Test *pTest)
{
    uint32_t days = 0;
    uint8_t month = 0, test_month = 0;
    uint8_t day = 0, test_day = 0;
    uint16_t year = 0;
    BACNET_DATE bdate;
    BACNET_DATE test_bdate;

    days = day_of_year(1900, 1, 1);
    ct_test(pTest, days == 1);
    day_of_year_into_md(days, 1900, &month, &day);
    ct_test(pTest, month == 1);
    ct_test(pTest, day == 1);

    for (year = 1900; year <= 2154; year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= datetime_month_days(year, month); day++) {
                days = day_of_year(year, month, day);
                day_of_year_into_md(days, year, &test_month, &test_day);
                ct_test(pTest, month == test_month);
                ct_test(pTest, day == test_day);
            }
        }
    }
    for (year = 1900; year <= 2154; year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= datetime_month_days(year, month); day++) {
                datetime_set_date(&bdate, year, month, day);
                days = datetime_day_of_year(&bdate);
                datetime_day_of_year_into_date(days, year, &test_bdate);
                ct_test(pTest, datetime_compare_date(&bdate, &test_bdate) == 0);
            }
        }
    }
}

static void testDateEpochConversionCompare(Test *pTest,
    uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second, uint8_t hundredth)
{
    uint64_t epoch_seconds = 0;
    BACNET_DATE_TIME bdatetime = {0};
    BACNET_DATE_TIME test_bdatetime = {0};
    int compare = 0;

    datetime_set_date(&bdatetime.date, year, month, day);
    datetime_set_time(&bdatetime.time, hour, minute, second,
        hundredth);
    epoch_seconds = datetime_seconds_since_epoch(&bdatetime);
    datetime_since_epoch_seconds(&test_bdatetime,
        epoch_seconds);
    compare = datetime_compare(&bdatetime,&test_bdatetime);
    ct_test(pTest,compare == 0);
    if (compare != 0) {
        datetime_print("bdatetime", &bdatetime);
        datetime_print("test_bdatetime", &test_bdatetime);
    }
}

static void testDateEpochConversion(Test *pTest)
{
    /* min */
    testDateEpochConversionCompare(pTest,
        BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    /* middle */
    testDateEpochConversionCompare(pTest,
        2020, 6, 26, 12, 30, 30, 0);
    /* max */
    testDateEpochConversionCompare(pTest,
        BACNET_EPOCH_YEAR + 0xFF - 1, 12, 31, 23, 59, 59, 0);
}

static void testDateEpoch(Test *pTest)
{
    uint32_t days = 0;
    uint16_t year = 0, test_year = 0;
    uint8_t month = 0, test_month = 0;
    uint8_t day = 0, test_day = 0;

    days = days_since_epoch(BACNET_EPOCH_YEAR, 1, 1);
    ct_test(pTest, days == 0);
    days_since_epoch_into_ymd(days, &year, &month, &day);
    ct_test(pTest, year == BACNET_EPOCH_YEAR);
    ct_test(pTest, month == 1);
    ct_test(pTest, day == 1);

    for (year = BACNET_EPOCH_YEAR; year < (BACNET_EPOCH_YEAR + 0xFF); year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= datetime_month_days(year, month); day++) {
                days = days_since_epoch(year, month, day);
                days_since_epoch_into_ymd(
                    days, &test_year, &test_month, &test_day);
                ct_test(pTest, year == test_year);
                ct_test(pTest, month == test_month);
                ct_test(pTest, day == test_day);
            }
        }
    }
}

static void testBACnetDayOfWeek(Test *pTest)
{
    uint8_t dow = 0;

    /* 1/1/1900 is a Monday */
    dow = datetime_day_of_week(1900, 1, 1);
    ct_test(pTest, dow == BACNET_WEEKDAY_MONDAY);

    /* 1/1/2007 is a Monday */
    dow = datetime_day_of_week(2007, 1, 1);
    ct_test(pTest, dow == BACNET_WEEKDAY_MONDAY);
    dow = datetime_day_of_week(2007, 1, 2);
    ct_test(pTest, dow == BACNET_WEEKDAY_TUESDAY);
    dow = datetime_day_of_week(2007, 1, 3);
    ct_test(pTest, dow == BACNET_WEEKDAY_WEDNESDAY);
    dow = datetime_day_of_week(2007, 1, 4);
    ct_test(pTest, dow == BACNET_WEEKDAY_THURSDAY);
    dow = datetime_day_of_week(2007, 1, 5);
    ct_test(pTest, dow == BACNET_WEEKDAY_FRIDAY);
    dow = datetime_day_of_week(2007, 1, 6);
    ct_test(pTest, dow == BACNET_WEEKDAY_SATURDAY);
    dow = datetime_day_of_week(2007, 1, 7);
    ct_test(pTest, dow == BACNET_WEEKDAY_SUNDAY);

    dow = datetime_day_of_week(2007, 1, 31);
    ct_test(pTest, dow == 3);
}

static void testDatetimeCodec(Test *pTest)
{
    uint8_t apdu[MAX_APDU];
    BACNET_DATE_TIME datetimeIn;
    BACNET_DATE_TIME datetimeOut;
    int inLen;
    int outLen;

    datetimeIn.date.day = 1;
    datetimeIn.date.month = 2;
    datetimeIn.date.wday = 3;
    datetimeIn.date.year = 1904;

    datetimeIn.time.hour = 5;
    datetimeIn.time.min = 6;
    datetimeIn.time.sec = 7;
    datetimeIn.time.hundredths = 8;

    inLen = bacapp_encode_context_datetime(apdu, 10, &datetimeIn);
    outLen = bacapp_decode_context_datetime(apdu, 10, &datetimeOut);

    ct_test(pTest, inLen == outLen);

    ct_test(pTest, datetimeIn.date.day == datetimeOut.date.day);
    ct_test(pTest, datetimeIn.date.month == datetimeOut.date.month);
    ct_test(pTest, datetimeIn.date.wday == datetimeOut.date.wday);
    ct_test(pTest, datetimeIn.date.year == datetimeOut.date.year);

    ct_test(pTest, datetimeIn.time.hour == datetimeOut.time.hour);
    ct_test(pTest, datetimeIn.time.min == datetimeOut.time.min);
    ct_test(pTest, datetimeIn.time.sec == datetimeOut.time.sec);
    ct_test(pTest, datetimeIn.time.hundredths == datetimeOut.time.hundredths);
}

static void testDatetimeConvertUTCSpecific(Test *pTest,
    BACNET_DATE_TIME *utc_time,
    BACNET_DATE_TIME *local_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes)
{
    bool status = false;
    BACNET_DATE_TIME test_local_time;

    status = datetime_local_to_utc(
        utc_time, local_time, utc_offset_minutes, dst_adjust_minutes);
    ct_test(pTest, status);
    status = datetime_utc_to_local(
        &test_local_time, utc_time, utc_offset_minutes, dst_adjust_minutes);
    ct_test(pTest, status);
    /* validate the conversion */
    ct_test(pTest, local_time->date.day == test_local_time.date.day);
    ct_test(pTest, local_time->date.month == test_local_time.date.month);
    ct_test(pTest, local_time->date.wday == test_local_time.date.wday);
    ct_test(pTest, local_time->date.year == test_local_time.date.year);
    ct_test(pTest, local_time->time.hour == test_local_time.time.hour);
    ct_test(pTest, local_time->time.min == test_local_time.time.min);
    ct_test(pTest, local_time->time.sec == test_local_time.time.sec);
    ct_test(
        pTest, local_time->time.hundredths == test_local_time.time.hundredths);
}

static void testDatetimeConvertUTC(Test *pTest)
{
    BACNET_DATE_TIME local_time;
    BACNET_DATE_TIME utc_time;
    /* values are positive east of UTC and negative west of UTC */
    int16_t utc_offset_minutes = 0;
    int8_t dst_adjust_minutes = 0;

    datetime_set_date(&local_time.date, 1999, 12, 23);
    datetime_set_time(&local_time.time, 8, 30, 0, 0);
    testDatetimeConvertUTCSpecific(
        pTest, &utc_time, &local_time, utc_offset_minutes, dst_adjust_minutes);
    /* check a timezone West of UTC */
    utc_offset_minutes = -6 * 60;
    dst_adjust_minutes = -60;
    testDatetimeConvertUTCSpecific(
        pTest, &utc_time, &local_time, utc_offset_minutes, dst_adjust_minutes);
    /* check a timezone East of UTC */
    utc_offset_minutes = 6 * 60;
    dst_adjust_minutes = 60;
    testDatetimeConvertUTCSpecific(
        pTest, &utc_time, &local_time, utc_offset_minutes, dst_adjust_minutes);
}

void testDateTime(Test *pTest)
{
    bool rc;

    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACnetDate);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetTime);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDateTime);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDayOfWeek);
    assert(rc);
    rc = ct_addTestFunction(pTest, testDateEpoch);
    assert(rc);
    rc = ct_addTestFunction(pTest, testDateEpochConversion);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDateTimeSeconds);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDateTimeAdd);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDateTimeWildcard);
    assert(rc);
    rc = ct_addTestFunction(pTest, testDatetimeCodec);
    assert(rc);
    rc = ct_addTestFunction(pTest, testDayOfYear);
    assert(rc);
    rc = ct_addTestFunction(pTest, testWildcardDateTime);
    assert(rc);
    rc = ct_addTestFunction(pTest, testDatetimeConvertUTC);
    assert(rc);
}

#ifdef TEST_DATE_TIME
int main(void)
{
    Test *pTest;

    pTest = ct_create("BACnet Date Time", NULL);
    testDateTime(pTest);
    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}

#endif /* TEST_DATE_TIME */
#endif /* BAC_TEST */
