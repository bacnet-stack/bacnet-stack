/**
 * @file
 * @brief test BACnetDate and BACnetTime encoding and decoding API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/ztest.h>
#include "bacnet/datetime.h"
#include "bacnet/basic/sys/days.h"
#include "bacnet/bacdcode.h"

/* define our epic beginnings */
#define BACNET_EPOCH_YEAR 1900

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API for unsigned 16b integers
 */
static void datetime_print(const char *title, BACNET_DATE_TIME *bdatetime)
{
    printf(
        "%s: %04u/%02u/%02u %02u:%02u:%02u.%03u\n", title,
        (unsigned int)bdatetime->date.year, (unsigned int)bdatetime->date.month,
        (unsigned int)bdatetime->date.wday, (unsigned int)bdatetime->time.hour,
        (unsigned int)bdatetime->time.min, (unsigned int)bdatetime->time.sec,
        (unsigned int)bdatetime->time.hundredths);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_datetime, testBACnetDateTimeWildcard)
#else
static void testBACnetDateTimeWildcard(void)
#endif
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_datetime, testBACnetDateTimeAdd)
#else
static void testBACnetDateTimeAdd(void)
#endif
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

    datetime_set_values(&bdatetime, 2013, 6, 6, 0, 59, 59, 0);
    datetime_add_seconds(&bdatetime, 1);
    datetime_set_values(&test_bdatetime, 2013, 6, 6, 1, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    zassert_equal(diff, 0, "diff=%d", diff);

    datetime_set_values(&bdatetime, 2013, 6, 6, 0, 0, 0, 0);
    datetime_add_seconds(&bdatetime, -1);
    datetime_set_values(&test_bdatetime, 2013, 6, 5, 23, 59, 59, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    zassert_equal(diff, 0, "diff=%d", diff);

    datetime_set_values(&bdatetime, 2013, 6, 6, 0, 59, 59, 99);
    datetime_add_milliseconds(&bdatetime, 10);
    datetime_set_values(&test_bdatetime, 2013, 6, 6, 1, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    zassert_equal(diff, 0, "diff=%d", diff);
}

static void testBACnetDateTimeSeconds(void)
{
    uint8_t hour = 0, minute = 0, second = 0;
    uint8_t test_hour = 0, test_minute = 0, test_second = 0;
    uint32_t seconds = 0, test_seconds;

    for (hour = 0; hour < 24; hour++) {
        for (minute = 0; minute < 60; minute += 3) {
            for (second = 0; second < 60; second += 17) {
                seconds = datetime_hms_to_seconds_since_midnight(
                    hour, minute, second);
                datetime_hms_from_seconds_since_midnight(
                    seconds, &test_hour, &test_minute, &test_second);
                test_seconds = datetime_hms_to_seconds_since_midnight(
                    test_hour, test_minute, test_second);
                zassert_equal(seconds, test_seconds, NULL);
            }
        }
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_datetime, testBACnetDate)
#else
static void testBACnetDate(void)
#endif
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_datetime, testBACnetTime)
#else
static void testBACnetTime(void)
#endif
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_datetime, testBACnetDateTime)
#else
static void testBACnetDateTime(void)
#endif
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_datetime, testWildcardDateTime)
#else
static void testWildcardDateTime(void)
#endif
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

static void testDayOfYear(void)
{
    uint32_t days = 0;
    uint8_t month = 0, test_month = 0;
    uint8_t day = 0, test_day = 0;
    uint16_t year = 0;
    BACNET_DATE bdate;
    BACNET_DATE test_bdate;

    days = days_of_year(1900, 1, 1);
    zassert_equal(days, 1, NULL);
    days_of_year_to_month_day(days, 1900, &month, &day);
    zassert_equal(month, 1, NULL);
    zassert_equal(day, 1, NULL);

    for (year = 1900; year <= 2154; year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= days_per_month(year, month); day++) {
                days = days_of_year(year, month, day);
                days_of_year_to_month_day(days, year, &test_month, &test_day);
                zassert_equal(month, test_month, NULL);
                zassert_equal(day, test_day, NULL);
            }
        }
    }
    for (year = 1900; year <= 2154; year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= days_per_month(year, month); day++) {
                datetime_set_date(&bdate, year, month, day);
                days = datetime_day_of_year(&bdate);
                datetime_day_of_year_into_date(days, year, &test_bdate);
                zassert_equal(
                    datetime_compare_date(&bdate, &test_bdate), 0,
                    "year=%u month=%u day=%u", year, month, day);
            }
        }
    }
}

static void testDateEpochConversionCompare(
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t second,
    uint8_t hundredth)
{
    uint64_t epoch_seconds = 0;
    BACNET_DATE_TIME bdatetime = { 0 };
    BACNET_DATE_TIME test_bdatetime = { 0 };
    int compare = 0;

    datetime_set_date(&bdatetime.date, year, month, day);
    datetime_set_time(&bdatetime.time, hour, minute, second, hundredth);
    epoch_seconds = datetime_seconds_since_epoch(&bdatetime);
    datetime_since_epoch_seconds(&test_bdatetime, epoch_seconds);
    compare = datetime_compare(&bdatetime, &test_bdatetime);
    zassert_equal(compare, 0, NULL);
    if (compare != 0) {
        datetime_print("bdatetime", &bdatetime);
        datetime_print("test_bdatetime", &test_bdatetime);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_datetime, testDateEpochConversion)
#else
static void testDateEpochConversion(void)
#endif
{
    /* min */
    testDateEpochConversionCompare(BACNET_EPOCH_YEAR, 1, 1, 0, 0, 0, 0);
    /* middle */
    testDateEpochConversionCompare(2020, 6, 26, 12, 30, 30, 0);
    /* max */
    testDateEpochConversionCompare(
        BACNET_EPOCH_YEAR + 0xFF - 1, 12, 31, 23, 59, 59, 0);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_datetime, testBACnetDayOfWeek)
#else
static void testBACnetDayOfWeek(void)
#endif
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_datetime, testDatetimeCodec)
#else
static void testDatetimeCodec(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    uint8_t tag_number = 10;
    BACNET_DATE_TIME datetimeIn;
    BACNET_DATE_TIME datetimeOut;
    int apdu_len;
    int test_len;
    int null_len;
    int str_len;
    char str[64];
    int diff;
    bool status;

    status = datetime_date_init_ascii(&datetimeIn.date, "1904/2/1");
    zassert_true(status, NULL);
    status = datetime_time_init_ascii(&datetimeIn.time, "5:06:07.8");
    zassert_true(status, NULL);
    /* application */
    apdu_len = bacapp_encode_datetime(NULL, &datetimeIn);
    zassert_true(apdu_len <= sizeof(apdu), NULL);
    null_len = bacapp_encode_datetime(NULL, &datetimeIn);
    apdu_len = bacapp_encode_datetime(apdu, &datetimeIn);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len > 0, NULL);
    null_len = bacnet_datetime_decode(apdu, apdu_len, NULL);
    test_len = bacnet_datetime_decode(apdu, apdu_len, &datetimeOut);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(apdu_len > 0, NULL);
    diff = datetime_compare(&datetimeOut, &datetimeIn);
    zassert_equal(diff, 0, NULL);
    /* test time stringify */
    str_len = datetime_time_to_ascii(&datetimeIn.time, str, sizeof(str));
    zassert_true(str_len > 0, NULL);
    status = datetime_time_init_ascii(&datetimeIn.time, str);
    zassert_true(status, NULL);
    diff = datetime_compare(&datetimeOut, &datetimeIn);
    zassert_equal(diff, 0, NULL);
    /* test date stringify */
    str_len = datetime_date_to_ascii(&datetimeIn.date, str, sizeof(str));
    zassert_true(str_len > 0, NULL);
    status = datetime_date_init_ascii(&datetimeIn.date, str);
    zassert_true(status, NULL);
    diff = datetime_compare(&datetimeOut, &datetimeIn);
    zassert_equal(diff, 0, NULL);
    /* test for invalid date tag */
    apdu_len = bacapp_encode_datetime(apdu, &datetimeIn);
    encode_tag(apdu, BACNET_APPLICATION_TAG_REAL, false, 4);
    test_len = bacnet_datetime_decode(&apdu[0], apdu_len, &datetimeOut);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    /* test for invalid time tag */
    apdu_len = bacapp_encode_datetime(apdu, &datetimeIn);
    encode_tag(&apdu[5], BACNET_APPLICATION_TAG_REAL, false, 4);
    test_len = bacnet_datetime_decode(apdu, apdu_len, &datetimeOut);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    /* ERROR too short APDU */
    apdu_len = bacapp_encode_datetime(apdu, &datetimeIn);
    while (apdu_len) {
        apdu_len--;
        test_len = bacnet_datetime_decode(apdu, apdu_len, &datetimeOut);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    }
    /* context */
    apdu_len = bacapp_encode_context_datetime(NULL, tag_number, &datetimeIn);
    zassert_true(apdu_len <= sizeof(apdu), NULL);
    apdu_len = bacapp_encode_context_datetime(apdu, tag_number, &datetimeIn);
    test_len = bacnet_datetime_context_decode(
        apdu, apdu_len, tag_number, &datetimeOut);
    zassert_equal(apdu_len, test_len, NULL);
    /* ERROR too short APDU */
    while (apdu_len) {
        apdu_len--;
        test_len = bacnet_datetime_context_decode(
            apdu, apdu_len, tag_number, &datetimeOut);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    }
    diff = datetime_compare(&datetimeOut, &datetimeIn);
    zassert_equal(diff, 0, NULL);
    /* test datetime stringify */
    status = datetime_init_ascii(&datetimeIn, "1904/2/1-5:06:07.8");
    zassert_true(status, NULL);
    str_len = datetime_to_ascii(&datetimeIn, str, sizeof(str));
    zassert_true(str_len > 0, NULL);
    status = datetime_init_ascii(&datetimeOut, str);
    zassert_true(status, NULL);
    diff = datetime_compare(&datetimeOut, &datetimeIn);
    zassert_equal(diff, 0, NULL);
}

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

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_datetime, testDatetimeConvertUTC)
#else
static void testDatetimeConvertUTC(void)
#endif
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
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacnet_datetime, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacnet_datetime, ztest_unit_test(testBACnetDate),
        ztest_unit_test(testBACnetTime), ztest_unit_test(testBACnetDateTime),
        ztest_unit_test(testBACnetDayOfWeek),
        ztest_unit_test(testDateEpochConversion),
        ztest_unit_test(testBACnetDateTimeAdd),
        ztest_unit_test(testBACnetDateTimeWildcard),
        ztest_unit_test(testDatetimeCodec),
        ztest_unit_test(testWildcardDateTime),
        ztest_unit_test(testBACnetDateTimeSeconds),
        ztest_unit_test(testDayOfYear),
        ztest_unit_test(testDatetimeConvertUTC));

    ztest_run_test_suite(bacnet_datetime);
}
#endif
