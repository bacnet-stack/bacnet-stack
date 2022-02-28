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
#include "bacnet/basic/sys/days.h"
#include "bacnet/datetime.h"
#include "bacnet/bacdcode.h"

/** @file datetime.c  Manipulate BACnet Date and Time values */

/* BACnet Date */
/* year = years since 1900 through 2155 */
/* month 1=Jan */
/* day = day of month 1..31 */
/* wday 1=Monday...7=Sunday */

/* Wildcards:
  A value of X'FF' in any of the four octets
  shall indicate that the value is unspecified.
  If all four octets = X'FF', the corresponding
  time or date may be interpreted as "any" or "don't care"
*/

/**
 * Determines if a given date is valid
 *
 * @param year - years after Christ birth (1900..2155 AD)
 * @param month - months (1=Jan...12=Dec)
 * @param day - day of month (1-31)
 * @return true if the date is valid
 */
bool datetime_ymd_is_valid(uint16_t year, uint8_t month, uint8_t day)
{
    bool status = false; /* true if value date */
    uint8_t monthdays = 0; /* days in a month */

    monthdays = days_per_month(year, month);
    if ((year >= BACNET_DATE_YEAR_EPOCH) && (monthdays > 0) && (day >= 1) &&
        (day <= monthdays)) {
        status = true;
    }

    return status;
}

/**
 * Determines if a given date is valid
 *
 * @param bdate - BACnet date structure
 * @return true if the date is valid
 */
bool datetime_date_is_valid(BACNET_DATE *bdate)
{
    bool status = false; /* true if value date */

    if (bdate) {
        status = datetime_ymd_is_valid(bdate->year, bdate->month, bdate->day);
    }

    return status;
}

/**
 * Converts date to day of the year
 *
 * @param year - years after Christ birth (1900..2155 AD)
 * @param month - months (1=Jan...12=Dec)
 * @param day - day of month (1-31)
 * @return number of days since Jan 1 (inclusive) of the given year
 */
uint32_t datetime_ymd_day_of_year(uint16_t year, uint8_t month, uint8_t day)
{
    uint32_t days = 0; /* return value */
    uint8_t months = 0; /* loop counter for months */

    if (datetime_ymd_is_valid(year, month, day)) {
        for (months = 1; months < month; months++) {
            days += days_per_month(year, months);
        }
        days += day;
    }

    return (days);
}

/**
 * Converts date to day of the year
 *
 * @param days - number of days since Jan 1 (inclusive) of the given year
 * @param year - years after Christ birth (1900..2155 AD)
 * @param bdate - BACnet date structure
 */
void datetime_day_of_year_into_date(
    uint32_t days, uint16_t year, BACNET_DATE *bdate)
{
    uint8_t month = 0;
    uint8_t day = 0;

    days_of_year_to_month_day(days, year, &month, &day);
    datetime_set_date(bdate, year, month, day);
}

/**
 * Converts date to day of the year
 *
 * @param bdate - BACnet date structure
 * @return number of days since Jan 1 (inclusive) of the given year
 */
uint32_t datetime_day_of_year(BACNET_DATE *bdate)
{
    uint32_t days = 0;

    if (bdate) {
        days = datetime_ymd_day_of_year(bdate->year, bdate->month, bdate->day);
    }

    return days;
}

/**
 * Converts days since BACnet epoch
 *
 * @param year - years after Christ birth (1900..2155 AD)
 * @param month - months (1=Jan...12=Dec)
 * @param day - day of month (1-31)
 * @return number of days since epoch, or 0 if out of range
 */
uint32_t datetime_ymd_to_days_since_epoch(
    uint16_t year, uint8_t month, uint8_t day)
{
    uint32_t days = 0; /* return value */
    uint16_t years = 0; /* loop counter for years */

    if (datetime_ymd_is_valid(year, month, day)) {
        for (years = BACNET_DATE_YEAR_EPOCH; years < year; years++) {
            days += 365;
            if (days_is_leap_year(years)) {
                days++;
            }
        }
        days += datetime_ymd_day_of_year(year, month, day);
        /* 'days since' is one less */
        days -= 1;
    }

    return (days);
}

/**
 * Converts date to days since BACnet epoch
 *
 * @param bdate - BACnet date structure
 * @return number of days since epoch, or 0 if out of range
 */
uint32_t datetime_days_since_epoch(BACNET_DATE *bdate)
{
    uint32_t days = 0;

    if (bdate) {
        days = datetime_ymd_to_days_since_epoch(
            bdate->year, bdate->month, bdate->day);
    }

    return days;
}

/**
 * Converts days since BACnet epoch to YMD date
 *
 * @param days - number of days since epoch, or 0 if out of range
 * @param pYear - [out] years after Christ birth (1900..2155 AD)
 * @param pMonth - [out] months (1=Jan...12=Dec)
 * @param pDay - [out] day of month (1-31)
 */
void datetime_ymd_from_days_since_epoch(
    uint32_t days, uint16_t *pYear, uint8_t *pMonth, uint8_t *pDay)
{
    uint16_t year = BACNET_DATE_YEAR_EPOCH;
    uint8_t month = 1;
    uint8_t day = 1;

    while (days >= 365) {
        if ((days_is_leap_year(year)) && (days == 365)) {
            break;
        }
        days -= 365;
        if (days_is_leap_year(year)) {
            --days;
        }
        year++;
    }

    while (days >= (uint32_t)days_per_month(year, month)) {
        days -= days_per_month(year, month);
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

/**
 * Converts days since BACnet epoch to date
 *
 * @param days - number of days since epoch, or 0 if out of range
 * @param bdate - BACnet date structure
 */
void datetime_days_since_epoch_into_date(uint32_t days, BACNET_DATE *bdate)
{
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;

    datetime_ymd_from_days_since_epoch(days, &year, &month, &day);
    datetime_set_date(bdate, year, month, day);
}

/**
 * Determines the day of week based on BACnet epoch: Jan 1, 1900 was a Monday
 *
 * @param year - years after Christ birth (1900..2155 AD)
 * @param month - months (1=Jan...12=Dec)
 * @param day - day of month (1-31)
 * @return BACnet day of week where 1=Monday...7=Sunday
 */
uint8_t datetime_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    uint8_t dow = (uint8_t)BACNET_DAY_OF_WEEK_EPOCH;

    dow += (datetime_ymd_to_days_since_epoch(year, month, day) % 7);

    return dow;
}

/**
 * Determines if a given time is valid
 *
 * @param btime - pointer to a BACNET_TIME structure
 * @return true if the time is valid
 */
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

uint32_t datetime_hms_to_seconds_since_midnight(
    uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    return ((hours * 60 * 60) + (minutes * 60) + seconds);
}

uint16_t datetime_hm_to_minutes_since_midnight(uint8_t hours, uint8_t minutes)
{
    return ((hours * 60) + minutes);
}

void datetime_hms_from_seconds_since_midnight(
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
    uint32_t seconds, BACNET_TIME *btime)
{
    if (btime) {
        datetime_hms_from_seconds_since_midnight(
            seconds, &btime->hour, &btime->min, &btime->sec);
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
        seconds = datetime_hms_to_seconds_since_midnight(
            btime->hour, btime->min, btime->sec);
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
        minutes =
            datetime_hm_to_minutes_since_midnight(btime->hour, btime->min);
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
    bdatetime_minutes =
        datetime_hms_to_seconds_since_midnight(
            bdatetime->time.hour, bdatetime->time.min, bdatetime->time.sec) /
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
    datetime_hms_from_seconds_since_midnight(bdatetime_minutes * 60,
        &bdatetime->time.hour, &bdatetime->time.min, NULL);
    datetime_days_since_epoch_into_date(bdatetime_days, &bdatetime->date);
}

#ifdef UINT64_MAX
/**
 * @brief Calculates the number of seconds since epoch
 * @param bdatetime [in] the starting date and time
 * @return seconds since midnight
 */
uint64_t datetime_seconds_since_epoch(BACNET_DATE_TIME *bdatetime)
{
    uint64_t seconds = 0;
    uint32_t days = 0;

    if (bdatetime) {
        days = datetime_days_since_epoch(&bdatetime->date);
        seconds = datetime_hms_to_seconds_since_midnight(24, 0, 0);
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
void datetime_since_epoch_seconds(BACNET_DATE_TIME *bdatetime, uint64_t seconds)
{
    uint32_t seconds_after_midnight = 0;
    uint32_t days = 0;
    uint32_t day_seconds = 0;

    if (bdatetime) {
        day_seconds = datetime_hms_to_seconds_since_midnight(24, 0, 0);
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
        if (bdate->year == (BACNET_DATE_YEAR_EPOCH + 0xFF)) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

/* Sets the year as a wildcard */
void datetime_wildcard_year_set(BACNET_DATE *bdate)
{
    if (bdate) {
        bdate->year = BACNET_DATE_YEAR_EPOCH + 0xFF;
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
        bdate->year = BACNET_DATE_YEAR_EPOCH + 0xFF;
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

/**
 * @brief Parse an ascii string for the date 2021/12/31 or 2021/12/31:1
 * @param bdate - #BACNET_DATE structure
 * @param argv - C string with date formatted 2021/12/31 or 2021/12/31:1
 *  or year/month/day or year/month/day:weekday
 * @return true if parsed successfully
 */
bool datetime_date_init_ascii(BACNET_DATE *bdate, const char *ascii)
{
    bool status = false;
    int year, month, day, wday;
    int count = 0;

    count = sscanf(ascii, "%4d/%3d/%3d:%3d", &year, &month, &day, &wday);
    if (count == 3) {
        datetime_set_date(bdate, (uint16_t)year, (uint8_t)month, (uint8_t)day);
        status = true;
    } else if (count == 4) {
        bdate->year = (uint16_t)year;
        bdate->month = (uint8_t)month;
        bdate->day = (uint8_t)day;
        bdate->wday = (uint8_t)wday;
        status = true;
    }

    return status;
}

/**
 * @brief Parse an ascii string for the time formatted 23:59:59.99
 * @param btime - #BACNET_TIME structure
 * @param ascii - C string with time formatted 23:59:59.99 or 23:59:59 or
 *  or 23:59 or hours:minutes:seconds.hundredths
 * @return true if parsed successfully
 */
bool datetime_time_init_ascii(BACNET_TIME *btime, const char *ascii)
{
    bool status = false;
    int hour, min, sec, hundredths;
    int count = 0;

    count = sscanf(ascii, "%3d:%3d:%3d.%3d", &hour, &min, &sec, &hundredths);
    if (count == 4) {
        btime->hour = (uint8_t)hour;
        btime->min = (uint8_t)min;
        btime->sec = (uint8_t)sec;
        btime->hundredths = (uint8_t)hundredths;
        status = true;
    } else if (count == 3) {
        btime->hour = (uint8_t)hour;
        btime->min = (uint8_t)min;
        btime->sec = (uint8_t)sec;
        btime->hundredths = 0;
        status = true;
    } else if (count == 2) {
        btime->hour = (uint8_t)hour;
        btime->min = (uint8_t)min;
        btime->sec = 0;
        btime->hundredths = 0;
        status = true;
    }

    return status;
}
