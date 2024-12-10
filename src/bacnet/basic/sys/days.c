/**
 * @file
 * @brief computes days from date, days of the week, days in a month,
 *  days in a year
 * @note Public domain algorithms from ACM
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 1997
 * @copyright SPDX-License-Identifier: CC-PDDC
 */
#include <stdint.h>
#include <stdbool.h>
#include "bacnet/basic/sys/days.h"

/**
 * Determines if a year is a leap year using Gregorian algorithm
 *
 * @param year - years after Christ birth (0..9999 AD)
 * @return true if leap year
 */
bool days_is_leap_year(uint16_t year)
{
    if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0)) {
        return true;
    }

    return (false);
}

/**
 * Determines the number of days in given month and year
 *
 * @param year - years after Christ birth (0..9999 AD)
 * @param month - months (1=Jan...12=Dec)
 * @return number of days in a given month and year, or 0 on error
 */
uint8_t days_per_month(uint16_t year, uint8_t month)
{
    /* note: start with a zero in the first element to save us from a
       month - 1 calculation in the lookup */
    const uint8_t month_days[13] = { 0,  31, 28, 31, 30, 31, 30,
                                     31, 31, 30, 31, 30, 31 };

    if ((month == 2) && days_is_leap_year(year)) {
        return (29);
    } else if (month >= 1 && month <= 12) {
        return (month_days[month]);
    }

    return 0;
}

/**
 * Determines the number of days in given year
 *
 * @param year - years after Christ birth (0..9999 AD)
 * @return number of days in a given year
 */
uint32_t days_per_year(uint16_t year)
{
    uint32_t days = 365;

    if (days_is_leap_year(year)) {
        days++;
    }

    return days;
}

/**
 * Determines the number of days (1..365) for a given date
 *
 * @param year - years after Christ birth (0..9999 AD)
 * @param month - months (1=Jan...12=Dec)
 * @param day - day of month (1-31)
 * @param days - number of days of the year (1..365)
 */
uint16_t days_of_year(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t days = 0; /* return value */
    uint8_t mm = 0; /* months counter */

    for (mm = 1; mm < month; mm++) {
        days += days_per_month(year, mm);
    }
    days += day;

    return days;
}

/**
 * Determines the number of days remaining in the year for a given date
 *
 * @param year - years after Christ birth (0..9999 AD)
 * @param month - months (1=Jan...12=Dec)
 * @param day - day of month (1-31)
 * @param days - number of days of the year (1..365)
 */
uint16_t days_of_year_remaining(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t days = 0; /* return value */
    uint8_t mm = 0; /* months counter */

    days = days_per_month(year, month) - day;
    for (mm = 12; mm > month; mm--) {
        days += days_per_month(year, mm);
    }

    return days;
}

/**
 * Converts days of year into date (year, month, day)
 *
 * @param days - number of days of the year (1..365)
 * @param year - years after Christ birth (0..9999 AD)
 * @param pMonth - months (1=Jan...12=Dec)
 * @param pDay - day of month (1-31)
 */
void days_of_year_to_month_day(
    uint32_t days, uint16_t year, uint8_t *pMonth, uint8_t *pDay)
{
    uint8_t month = 1;
    uint8_t day = 0;

    while (days > (uint32_t)days_per_month(year, month)) {
        days -= days_per_month(year, month);
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

/**
 * Counts days apart between two dates
 *
 * @param year1 - years after Christ birth (0..9999 AD)
 * @param month1 - months (1=Jan...12=Dec)
 * @param day1 - day of month (1-31)
 * @return number of days apart, or 0 if same dates
 */
uint32_t days_apart(
    uint16_t year1,
    uint8_t month1,
    uint8_t day1,
    uint16_t year2,
    uint8_t month2,
    uint8_t day2)
{
    uint32_t days = 0; /* return value */
    uint32_t days1 = 0;
    uint32_t days2 = 0;
    uint16_t yy = 0; /* year */

    if (year2 > year1) {
        days = days_of_year_remaining(year1, month1, day1);
        for (yy = year1 + 1; yy < year2; yy++) {
            days += 365;
            if (days_is_leap_year(yy)) {
                days++;
            }
        }
        days += days_of_year(year2, month2, day2);
    } else if (year2 < year1) {
        days = days_of_year_remaining(year2, month2, day2);
        for (yy = year2 + 1; yy < year1; yy++) {
            days += 365;
            if (days_is_leap_year(yy)) {
                days++;
            }
        }
        days += days_of_year(year1, month1, day1);
    } else {
        days1 = days_of_year(year1, month1, day1);
        days2 = days_of_year(year2, month2, day2);
        if (days2 > days1) {
            days = days2 - days1;
        } else {
            days = days1 - days2;
        }
    }

    return days;
}

/**
 * Converts date to days since epoch (Jan 0, epoch year)
 *
 * @param year - years after Christ birth (0..9999 AD)
 * @param month - months (1=Jan...12=Dec)
 * @param day - day of month (1-31)
 * @return number of days since epoch, or 0 if out of range
 */
uint32_t
days_since_epoch(uint16_t epoch_year, uint16_t year, uint8_t month, uint8_t day)
{
    uint32_t days = 0; /* return value */
    uint16_t yy = 0; /* year */
    uint8_t mm = 0; /* months counter */
    uint8_t monthdays = 0; /* days in a month */

    /* validate the date conforms to our range */
    monthdays = days_per_month(year, month);
    if ((year >= epoch_year) && (year <= 9999) && (monthdays > 0) &&
        (day >= 1) && (day <= monthdays)) {
        for (yy = epoch_year; yy < year; yy++) {
            days += 365;
            if (days_is_leap_year(yy)) {
                days++;
            }
        }
        for (mm = 1; mm < month; mm++) {
            days += days_per_month(year, mm);
        }
        days += day;
        /* 'days since' is one less */
        days -= 1;
    }

    return (days);
}

/**
 * Converts epoch days into date (year, month, day)
 *
 * @param days - number of days since epoch
 * @param pYear - years after Christ birth (2000..9999 AD)
 * @param pMonth - months (1=Jan...12=Dec)
 * @param pDay - day of month (1-31)
 * @return nothing
 */
void days_since_epoch_to_date(
    uint16_t epoch_year,
    uint32_t days,
    uint16_t *pYear,
    uint8_t *pMonth,
    uint8_t *pDay)
{
    uint8_t month = 1;
    uint8_t day = 1;
    uint16_t year;

    year = epoch_year;
    while (days >= days_per_year(year)) {
        days -= days_per_year(year);
        year++;
    }
    while (days >= days_per_month(year, month)) {
        days -= days_per_month(year, month);
        month++;
    }
    day += days;
    /* load values into the pointers */
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
 * Determines if a given date is valid
 *
 * @param year - years after Christ birth (0..9999 AD)
 * @param month - months (1=Jan...12=Dec)
 * @param day - day of month (1-31)
 * @return true if the date is valid
 */
bool days_date_is_valid(uint16_t year, uint8_t month, uint8_t day)
{
    uint8_t month_days = 0;
    bool valid = false; /* return value */

    /* get the number of days in the month, and check for valid month too */
    month_days = days_per_month(year, month);
    if ((month_days > 0) && (day > 0) && (day <= month_days)) {
        valid = true;
    }

    return (valid);
}

/**
 * Returns the day of the week value
 *
 * @param epoch_day - day of week for epoch day
 * @param days - number of days since epoch
 * @return day of week 1..7 offset by epoch day
 */
uint8_t days_of_week(uint8_t epoch_day, uint32_t days)
{
    uint8_t dow = epoch_day;

    dow += (days % 7);

    return dow;
}
