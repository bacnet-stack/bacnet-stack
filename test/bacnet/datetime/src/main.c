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

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/ztest.h>
#include "bacnet/datetime.h"
#include "bacnet/bacdcode.h"


/* define our epic beginnings */
#define BACNET_EPOCH_YEAR 1900
/* 1/1/1900 is a Monday */
#define BACNET_EPOCH_DOW BACNET_WEEKDAY_MONDAY


/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API for unsigned 16b integers
 */
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

static void testBACnetDateTimeWildcard(void)
{
    BACNET_DATE_TIME bdatetime;
    bool status = false;

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    status = datetime_wildcard(&bdatetime);
    zassert_false(status, NULL);

    datetime_wildcard_set(&bdatetime);
    status = datetime_wildcard(&bdatetime);
    zassert_true(status, NULL);
}

static void testBACnetDateTimeAdd(void)
{
    BACNET_DATE_TIME bdatetime, test_bdatetime;
    uint32_t minutes = 0;
    int diff = 0;

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_copy(&test_bdatetime, &bdatetime);
    datetime_add_minutes(&bdatetime, minutes);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    zassert_equal(diff, 0, NULL);

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_add_minutes(&bdatetime, 60);
    datetime_set_values(&test_bdatetime, BACNET_EPOCH_YEAR, 1, 1, 1, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    zassert_equal(diff, 0, NULL);

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_add_minutes(&bdatetime, (24 * 60));
    datetime_set_values(&test_bdatetime, BACNET_EPOCH_YEAR, 1, 2, 0, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    zassert_equal(diff, 0, NULL);

    datetime_set_values(&bdatetime, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_add_minutes(&bdatetime, (31 * 24 * 60));
    datetime_set_values(&test_bdatetime, BACNET_EPOCH_YEAR, 2, 1, 0, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    zassert_equal(diff, 0, NULL);

    datetime_set_values(&bdatetime, 2013, 6, 6, 23, 59, 59, 0);
    datetime_add_minutes(&bdatetime, 60);
    datetime_set_values(&test_bdatetime, 2013, 6, 7, 0, 59, 59, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    zassert_equal(diff, 0, NULL);

    datetime_set_values(&bdatetime, 2013, 6, 6, 0, 59, 59, 0);
    datetime_add_minutes(&bdatetime, -60);
    datetime_set_values(&test_bdatetime, 2013, 6, 5, 23, 59, 59, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    zassert_equal(diff, 0, NULL);
}

#if 0 /*TODO: Change to use external methods */
static void testBACnetDateTimeSeconds(void)
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
                zassert_equal(seconds, test_seconds, NULL);
            }
        }
    }
}
#endif

static void testBACnetDate(void)
{
    BACNET_DATE bdate1, bdate2;
    int diff = 0;

    datetime_set_date(&bdate1, BACNET_EPOCH_YEAR, 1, 1);
    datetime_copy_date(&bdate2, &bdate1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_equal(diff, 0, NULL);
    datetime_set_date(&bdate2, BACNET_EPOCH_YEAR, 1, 2);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff < 0, NULL);
    datetime_set_date(&bdate2, BACNET_EPOCH_YEAR, 2, 1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff < 0, NULL);
    datetime_set_date(&bdate2, 1901, 1, 1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff < 0, NULL);

    /* midpoint */
    datetime_set_date(&bdate1, 2007, 7, 15);
    datetime_copy_date(&bdate2, &bdate1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_equal(diff, 0, NULL);
    datetime_set_date(&bdate2, 2007, 7, 14);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff > 0, NULL);
    datetime_set_date(&bdate2, 2007, 7, 1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff > 0, NULL);
    datetime_set_date(&bdate2, 2007, 7, 31);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff < 0, NULL);
    datetime_set_date(&bdate2, 2007, 8, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff < 0, NULL);
    datetime_set_date(&bdate2, 2007, 12, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff < 0, NULL);
    datetime_set_date(&bdate2, 2007, 6, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff > 0, NULL);
    datetime_set_date(&bdate2, 2007, 1, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff > 0, NULL);
    datetime_set_date(&bdate2, 2006, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff > 0, NULL);
    datetime_set_date(&bdate2, BACNET_EPOCH_YEAR, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff > 0, NULL);
    datetime_set_date(&bdate2, 2008, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff < 0, NULL);
    datetime_set_date(&bdate2, 2154, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    zassert_true(diff < 0, NULL);

    return;
}

static void testBACnetTime(void)
{
    BACNET_TIME btime1, btime2;
    int diff = 0;

    datetime_set_time(&btime1, 0, 0, 0, 0);
    datetime_copy_time(&btime2, &btime1);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_equal(diff, 0, NULL);

    datetime_set_time(&btime1, 23, 59, 59, 99);
    datetime_copy_time(&btime2, &btime1);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_equal(diff, 0, NULL);

    /* midpoint */
    datetime_set_time(&btime1, 12, 30, 30, 50);
    datetime_copy_time(&btime2, &btime1);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_equal(diff, 0, NULL);
    datetime_set_time(&btime2, 12, 30, 30, 51);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_true(diff < 0, NULL);
    datetime_set_time(&btime2, 12, 30, 31, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_true(diff < 0, NULL);
    datetime_set_time(&btime2, 12, 31, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_true(diff < 0, NULL);
    datetime_set_time(&btime2, 13, 30, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_true(diff < 0, NULL);

    datetime_set_time(&btime2, 12, 30, 30, 49);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_true(diff > 0, NULL);
    datetime_set_time(&btime2, 12, 30, 29, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_true(diff > 0, NULL);
    datetime_set_time(&btime2, 12, 29, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_true(diff > 0, NULL);
    datetime_set_time(&btime2, 11, 30, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    zassert_true(diff > 0, NULL);

    return;
}

static void testBACnetDateTime(void)
{
    BACNET_DATE_TIME bdatetime1, bdatetime2;
    BACNET_DATE bdate;
    BACNET_TIME btime;
    int diff = 0;

    datetime_set_values(&bdatetime1, BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    datetime_copy(&bdatetime2, &bdatetime1);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_equal(diff, 0, NULL);
    datetime_set_time(&btime, 0, 0, 0, 0);
    datetime_set_date(&bdate, BACNET_EPOCH_YEAR, 1, 1);
    datetime_set(&bdatetime1, &bdate, &btime);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_equal(diff, 0, NULL);

    /* midpoint */
    /* if datetime1 is before datetime2, returns negative */
    datetime_set_values(&bdatetime1, 2000, 7, 15, 12, 30, 30, 50);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 30, 51);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff < 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 31, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff < 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 31, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff < 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 13, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff < 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 7, 16, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff < 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 8, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff < 0, NULL);
    datetime_set_values(&bdatetime2, 2001, 7, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff < 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 30, 49);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff > 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 29, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff > 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 29, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff > 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 11, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff > 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 7, 14, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff > 0, NULL);
    datetime_set_values(&bdatetime2, 2000, 6, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff > 0, NULL);
    datetime_set_values(&bdatetime2, 1999, 7, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    zassert_true(diff > 0, NULL);

    return;
}

static void testWildcardDateTime(void)
{
    BACNET_DATE_TIME bdatetime1, bdatetime2;
    BACNET_DATE bdate;
    BACNET_TIME btime;
    int diff = 0;

    datetime_wildcard_set(&bdatetime1);
    zassert_true(datetime_wildcard(&bdatetime1), NULL);
    zassert_true(datetime_wildcard_present(&bdatetime1), NULL);
    datetime_copy(&bdatetime2, &bdatetime1);
    diff = datetime_wildcard_compare(&bdatetime1, &bdatetime2);
    zassert_equal(diff, 0, NULL);
    datetime_time_wildcard_set(&btime);
    datetime_date_wildcard_set(&bdate);
    datetime_set(&bdatetime1, &bdate, &btime);
    diff = datetime_wildcard_compare(&bdatetime1, &bdatetime2);
    zassert_equal(diff, 0, NULL);

    return;
}

#if 0 /*TODO: Change to use external methods */
static void testDayOfYear(void)
{
    uint32_t days = 0;
    uint8_t month = 0, test_month = 0;
    uint8_t day = 0, test_day = 0;
    uint16_t year = 0;
    BACNET_DATE bdate;
    BACNET_DATE test_bdate;

    days = day_of_year(1900, 1, 1);
    zassert_equal(days, 1, NULL);
    day_of_year_into_md(days, 1900, &month, &day);
    zassert_equal(month, 1, NULL);
    zassert_equal(day, 1, NULL);

    for (year = 1900; year <= 2154; year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= datetime_month_days(year, month); day++) {
                days = day_of_year(year, month, day);
                day_of_year_into_md(days, year, &test_month, &test_day);
                zassert_equal(month, test_month, NULL);
                zassert_equal(day, test_day, NULL);
            }
        }
    }
    for (year = 1900; year <= 2154; year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= datetime_month_days(year, month); day++) {
                datetime_set_date(&bdate, year, month, day);
                days = datetime_day_of_year(&bdate);
                datetime_day_of_year_into_date(days, year, &test_bdate);
                zassert_true(datetime_compare_date(&bdate, &test_bdate), 0, NULL);
            }
        }
    }
}
#endif

static void testDateEpochConversionCompare(
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
    zassert_equal(compare, 0, NULL);
    if (compare != 0) {
        datetime_print("bdatetime", &bdatetime);
        datetime_print("test_bdatetime", &test_bdatetime);
    }
}

static void testDateEpochConversion(void)
{
    /* min */
    testDateEpochConversionCompare(
        BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    /* middle */
    testDateEpochConversionCompare(
        2020, 6, 26, 12, 30, 30, 0);
    /* max */
    testDateEpochConversionCompare(
        BACNET_EPOCH_YEAR + 0xFF - 1, 12, 31, 23, 59, 59, 0);
}

#if 0 /*TODO: Change to use external methods */
static void testDateEpoch(void)
{
    uint32_t days = 0;
    uint16_t year = 0, test_year = 0;
    uint8_t month = 0, test_month = 0;
    uint8_t day = 0, test_day = 0;

    days = days_since_epoch(BACNET_EPOCH_YEAR, 1, 1);
    zassert_equal(days, 0, NULL);
    days_since_epoch_into_ymd(days, &year, &month, &day);
    zassert_equal(year, BACNET_EPOCH_YEAR, NULL);
    zassert_equal(month, 1, NULL);
    zassert_equal(day, 1, NULL);

    for (year = BACNET_EPOCH_YEAR; year < (BACNET_EPOCH_YEAR + 0xFF); year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= datetime_month_days(year, month); day++) {
                days = days_since_epoch(year, month, day);
                days_since_epoch_into_ymd(
                    days, &test_year, &test_month, &test_day);
                zassert_equal(year, test_year, NULL);
                zassert_equal(month, test_month, NULL);
                zassert_equal(day, test_day, NULL);
            }
        }
    }
}
#endif

static void testBACnetDayOfWeek(void)
{
    uint8_t dow = 0;

    /* 1/1/1900 is a Monday */
    dow = datetime_day_of_week(1900, 1, 1);
    zassert_equal(dow, BACNET_WEEKDAY_MONDAY, NULL);

    /* 1/1/2007 is a Monday */
    dow = datetime_day_of_week(2007, 1, 1);
    zassert_equal(dow, BACNET_WEEKDAY_MONDAY, NULL);
    dow = datetime_day_of_week(2007, 1, 2);
    zassert_equal(dow, BACNET_WEEKDAY_TUESDAY, NULL);
    dow = datetime_day_of_week(2007, 1, 3);
    zassert_equal(dow, BACNET_WEEKDAY_WEDNESDAY, NULL);
    dow = datetime_day_of_week(2007, 1, 4);
    zassert_equal(dow, BACNET_WEEKDAY_THURSDAY, NULL);
    dow = datetime_day_of_week(2007, 1, 5);
    zassert_equal(dow, BACNET_WEEKDAY_FRIDAY, NULL);
    dow = datetime_day_of_week(2007, 1, 6);
    zassert_equal(dow, BACNET_WEEKDAY_SATURDAY, NULL);
    dow = datetime_day_of_week(2007, 1, 7);
    zassert_equal(dow, BACNET_WEEKDAY_SUNDAY, NULL);

    dow = datetime_day_of_week(2007, 1, 31);
    zassert_equal(dow, 3, NULL);
}

static void testDatetimeCodec(void)
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

    zassert_equal(inLen, outLen, NULL);

    zassert_equal(datetimeIn.date.day, datetimeOut.date.day, NULL);
    zassert_equal(datetimeIn.date.month, datetimeOut.date.month, NULL);
    zassert_equal(datetimeIn.date.wday, datetimeOut.date.wday, NULL);
    zassert_equal(datetimeIn.date.year, datetimeOut.date.year, NULL);

    zassert_equal(datetimeIn.time.hour, datetimeOut.time.hour, NULL);
    zassert_equal(datetimeIn.time.min, datetimeOut.time.min, NULL);
    zassert_equal(datetimeIn.time.sec, datetimeOut.time.sec, NULL);
    zassert_equal(datetimeIn.time.hundredths, datetimeOut.time.hundredths, NULL);
}

#if 0 /*TODO: Change to use external methods */
static void testDatetimeConvertUTCSpecific(
    BACNET_DATE_TIME *utc_time,
    BACNET_DATE_TIME *local_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes)
{
    bool status = false;
    BACNET_DATE_TIME test_local_time;

    status = datetime_local_to_utc(
        utc_time, local_time, utc_offset_minutes, dst_adjust_minutes);
    zassert_true(status, NULL);
    status = datetime_utc_to_local(
        &test_local_time, utc_time, utc_offset_minutes, dst_adjust_minutes);
    zassert_true(status, NULL);
    /* validate the conversion */
    zassert_equal(local_time->date.day, test_local_time.date.day, NULL);
    zassert_equal(local_time->date.month, test_local_time.date.month, NULL);
    zassert_equal(local_time->date.wday, test_local_time.date.wday, NULL);
    zassert_equal(local_time->date.year, test_local_time.date.year, NULL);
    zassert_equal(local_time->time.hour, test_local_time.time.hour, NULL);
    zassert_equal(local_time->time.min, test_local_time.time.min, NULL);
    zassert_equal(local_time->time.sec, test_local_time.time.sec, NULL);
    zassert_equal(
        local_time->time.hundredths, test_local_time.time.hundredths, NULL);
}
#endif

#if 0 /*TODO: Change to use external methods */
static void testDatetimeConvertUTC(void)
{
    BACNET_DATE_TIME local_time;
    BACNET_DATE_TIME utc_time;
    /* values are positive east of UTC and negative west of UTC */
    int16_t utc_offset_minutes = 0;
    int8_t dst_adjust_minutes = 0;

    datetime_set_date(&local_time.date, 1999, 12, 23);
    datetime_set_time(&local_time.time, 8, 30, 0, 0);
    testDatetimeConvertUTCSpecific(
        &utc_time, &local_time, utc_offset_minutes, dst_adjust_minutes);
    /* check a timezone West of UTC */
    utc_offset_minutes = -6 * 60;
    dst_adjust_minutes = -60;
    testDatetimeConvertUTCSpecific(
        &utc_time, &local_time, utc_offset_minutes, dst_adjust_minutes);
    /* check a timezone East of UTC */
    utc_offset_minutes = 6 * 60;
    dst_adjust_minutes = 60;
    testDatetimeConvertUTCSpecific(
        &utc_time, &local_time, utc_offset_minutes, dst_adjust_minutes);
}
#endif
/**
 * @}
 */


void test_main(void)
{
#if 0
     ztest_unit_test(testDateEpoch),
     ztest_unit_test(testBACnetDateTimeSeconds),
     ztest_unit_test(testDayOfYear),
#endif
    ztest_test_suite(wp_tests,
     ztest_unit_test(testBACnetDate),
     ztest_unit_test(testBACnetTime),
     ztest_unit_test(testBACnetDateTime),
     ztest_unit_test(testBACnetDayOfWeek),
     ztest_unit_test(testDateEpochConversion),
     ztest_unit_test(testBACnetDateTimeAdd),
     ztest_unit_test(testBACnetDateTimeWildcard),
     ztest_unit_test(testDatetimeCodec),
     ztest_unit_test(testWildcardDateTime)
     );

    ztest_run_test_suite(wp_tests);
}
