/**
 * @file
 * @brief Unit tests for Linux operating-system datetime behavior
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <zephyr/ztest.h>
#include "bacnet/datetime.h"

static unsigned Time_Change_Count;
static unsigned Subsecond_Change_Count;
static bool Observer_Read_Succeeded;

static void test_datetime_change_callback(
    BACNET_DATETIME_CHANGE change, const BACNET_DATE_TIME *standard_time)
{
    BACNET_DATE_TIME local_time = { 0 };

    (void)change;
    (void)standard_time;
    Time_Change_Count++;
    Observer_Read_Succeeded =
        datetime_local(&local_time.date, &local_time.time, NULL, NULL);
}

static void test_subsecond_change_callback(
    BACNET_DATETIME_CHANGE change, const BACNET_DATE_TIME *standard_time)
{
    (void)change;
    (void)standard_time;
    Subsecond_Change_Count++;
}

/**
 * @brief Absolute hundredths difference, wrapping across the second boundary.
 */
static int hundredths_delta(uint8_t actual, uint8_t expected)
{
    int delta = (int)actual - (int)expected;

    if (delta < 0) {
        delta = -delta;
    }
    if (delta > 50) {
        delta = 100 - delta;
    }

    return delta;
}

static void check_local_time(
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    bool expected_dst)
{
    BACNET_DATE date = { 0 };
    BACNET_TIME time = { 0 };
    int16_t utc_offset = 0;
    bool dst_active = false;

    zassert_true(datetime_local(&date, &time, &utc_offset, &dst_active), NULL);
    zassert_equal(date.year, year, NULL);
    zassert_equal(date.month, month, NULL);
    zassert_equal(date.day, day, NULL);
    zassert_equal(time.hour, hour, NULL);
    zassert_equal(time.min, minute, NULL);
    zassert_equal(utc_offset, 300, NULL);
    zassert_equal(dst_active, expected_dst, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_linux, test_host_timezone_sync)
#else
static void test_host_timezone_sync(void)
#endif
{
    static DATETIME_CHANGE_NOTIFICATION notification = {
        NULL, test_datetime_change_callback
    };
    BACNET_DATE_TIME standard_time = { 0 };
    BACNET_DATE_TIME utc_time = { 0 };
    BACNET_DATE date = { 0 };
    BACNET_TIME time = { 0 };

    zassert_equal(setenv("TZ", "America/New_York", 1), 0, NULL);
    tzset();
    datetime_reset();
    datetime_init();
    datetime_timesync_deadband_set(0);
    datetime_change_notification_add(&notification);
    Time_Change_Count = 0;

    datetime_set_date(&date, 2024, 7, 1);
    datetime_set_time(&time, 14, 0, 0, 0);
    datetime_timesync(&date, &time, false);
    check_local_time(2024, 7, 1, 14, 0, true);
    zassert_equal(Time_Change_Count, 1, NULL);
    zassert_true(Observer_Read_Succeeded, NULL);
    zassert_true(datetime_standard(&standard_time), NULL);
    zassert_equal(standard_time.time.hour, 13, NULL);

    datetime_set_values(&utc_time, 2024, 3, 10, 6, 30, 0, 0);
    datetime_timesync(&utc_time.date, &utc_time.time, true);
    check_local_time(2024, 3, 10, 1, 30, false);
    datetime_set_values(&utc_time, 2024, 3, 10, 7, 30, 0, 0);
    datetime_timesync(&utc_time.date, &utc_time.time, true);
    check_local_time(2024, 3, 10, 3, 30, true);

    datetime_set_values(&utc_time, 2024, 11, 3, 5, 30, 0, 0);
    datetime_timesync(&utc_time.date, &utc_time.time, true);
    check_local_time(2024, 11, 3, 1, 30, true);
    datetime_set_values(&utc_time, 2024, 11, 3, 6, 30, 0, 0);
    datetime_timesync(&utc_time.date, &utc_time.time, true);
    check_local_time(2024, 11, 3, 1, 30, false);

    datetime_set_date(&date, 2024, 11, 3);
    datetime_set_time(&time, 1, 30, 0, 0);
    datetime_timesync(&date, &time, false);
    check_local_time(2024, 11, 3, 1, 30, false);
    zassert_false(datetime_utc_offset_minutes_set(0), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_linux, test_host_timezone_partial_utc)
#else
static void test_host_timezone_partial_utc(void)
#endif
{
    BACNET_DATE_TIME utc_time = { 0 };
    BACNET_DATE date = { 0 };
    BACNET_TIME time = { 0 };

    zassert_equal(setenv("TZ", "America/New_York", 1), 0, NULL);
    tzset();
    datetime_reset();
    datetime_init();
    datetime_timesync_deadband_set(0);

    /* 02:30 UTC during DST is 22:30 local on the previous day */
    datetime_set_values(&utc_time, 2024, 7, 1, 2, 30, 0, 0);
    datetime_timesync(&utc_time.date, &utc_time.time, true);
    check_local_time(2024, 6, 30, 22, 30, true);

    /* date-only UTC request preserves the UTC time */
    datetime_set_date(&date, 2024, 7, 5);
    datetime_timesync(&date, NULL, true);
    check_local_time(2024, 7, 4, 22, 30, true);

    /* time-only UTC request keeps the UTC date, not the local date */
    datetime_set_time(&time, 3, 15, 0, 0);
    datetime_timesync(NULL, &time, true);
    check_local_time(2024, 7, 4, 23, 15, true);

    /* the same requests outside DST */
    datetime_set_values(&utc_time, 2024, 1, 10, 3, 30, 0, 0);
    datetime_timesync(&utc_time.date, &utc_time.time, true);
    check_local_time(2024, 1, 9, 22, 30, false);
    datetime_set_date(&date, 2024, 1, 20);
    datetime_timesync(&date, NULL, true);
    check_local_time(2024, 1, 19, 22, 30, false);
    datetime_set_time(&time, 4, 45, 0, 0);
    datetime_timesync(NULL, &time, true);
    check_local_time(2024, 1, 19, 23, 45, false);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_linux, test_host_subsecond_offset)
#else
static void test_host_subsecond_offset(void)
#endif
{
    static DATETIME_CHANGE_NOTIFICATION notification = {
        NULL, test_subsecond_change_callback
    };
    BACNET_DATE_TIME standard_time = { 0 };
    BACNET_DATE_TIME readback = { 0 };
    BACNET_DATE date = { 0 };
    BACNET_DATE read_date = { 0 };
    BACNET_TIME time = { 0 };
    BACNET_TIME read_time = { 0 };
    struct timeval tv = { 0 };
    uint8_t preserved = 0;
    int delta = 0;

    zassert_equal(setenv("TZ", "America/New_York", 1), 0, NULL);
    tzset();
    datetime_reset();
    datetime_init();
    datetime_timesync_deadband_set(0);
    datetime_change_notification_add(&notification);

    zassert_true(datetime_standard(&standard_time), NULL);
    standard_time.time.hundredths = 80;
    datetime_standard_set(&standard_time);
    zassert_true(datetime_standard(&readback), NULL);
    zassert_equal(readback.date.year, standard_time.date.year, NULL);
    zassert_equal(readback.date.month, standard_time.date.month, NULL);
    zassert_equal(readback.date.day, standard_time.date.day, NULL);
    zassert_equal(readback.time.hour, standard_time.time.hour, NULL);
    zassert_equal(readback.time.min, standard_time.time.min, NULL);
    zassert_equal(readback.time.sec, standard_time.time.sec, NULL);
    zassert_true(
        hundredths_delta(readback.time.hundredths, 80) <= 1, "hundredths %u",
        readback.time.hundredths);

    standard_time.time.hundredths = 20;
    datetime_standard_set(&standard_time);
    zassert_true(datetime_standard(&readback), NULL);
    zassert_equal(readback.time.sec, standard_time.time.sec, NULL);
    zassert_true(
        hundredths_delta(readback.time.hundredths, 20) <= 1, "hundredths %u",
        readback.time.hundredths);

    datetime_set_date(&date, 2024, 1, 15);
    datetime_set_time(&time, 10, 0, 0, 80);
    datetime_timesync(&date, &time, false);
    zassert_true(datetime_local(&read_date, &read_time, NULL, NULL), NULL);
    zassert_equal(read_date.year, 2024, NULL);
    zassert_equal(read_date.month, 1, NULL);
    zassert_equal(read_date.day, 15, NULL);
    zassert_equal(read_time.hour, 10, NULL);
    zassert_equal(read_time.min, 0, NULL);
    zassert_equal(read_time.sec, 0, NULL);
    zassert_true(
        hundredths_delta(read_time.hundredths, 80) <= 1, "hundredths %u",
        read_time.hundredths);

    datetime_timesync_deadband_set(200);
    Subsecond_Change_Count = 0;
    datetime_set_time(&time, 10, 0, 0, 80);
    datetime_timesync(&date, &time, false);
    zassert_equal(Subsecond_Change_Count, 0, NULL);

    datetime_set_time(&time, 10, 0, 0, 20);
    datetime_timesync(&date, &time, false);
    zassert_equal(Subsecond_Change_Count, 1, NULL);
    zassert_true(datetime_local(&read_date, &read_time, NULL, NULL), NULL);
    zassert_equal(read_time.sec, 0, NULL);
    zassert_true(
        hundredths_delta(read_time.hundredths, 20) <= 1, "hundredths %u",
        read_time.hundredths);
    preserved = read_time.hundredths;

    datetime_timesync_deadband_set(0);
    datetime_set_date(&date, 2024, 1, 16);
    datetime_timesync(&date, NULL, false);
    zassert_true(datetime_local(&read_date, &read_time, NULL, NULL), NULL);
    zassert_equal(read_date.day, 16, NULL);
    zassert_equal(read_time.hour, 10, NULL);
    zassert_equal(read_time.min, 0, NULL);
    zassert_equal(read_time.sec, 0, NULL);
    zassert_true(
        hundredths_delta(read_time.hundredths, preserved) <= 1, "hundredths %u",
        read_time.hundredths);

    datetime_reset();
    zassert_equal(gettimeofday(&tv, NULL), 0, NULL);
    zassert_true(datetime_local(&read_date, &read_time, NULL, NULL), NULL);
    delta =
        hundredths_delta(read_time.hundredths, (uint8_t)(tv.tv_usec / 10000));
    if (delta > 1) {
        zassert_equal(gettimeofday(&tv, NULL), 0, NULL);
        delta = hundredths_delta(
            read_time.hundredths, (uint8_t)(tv.tv_usec / 10000));
    }
    zassert_true(delta <= 1, "hundredths %u", read_time.hundredths);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(datetime_linux, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        datetime_linux, ztest_unit_test(test_host_timezone_sync),
        ztest_unit_test(test_host_timezone_partial_utc),
        ztest_unit_test(test_host_subsecond_offset));
    ztest_run_test_suite(datetime_linux);
}
#endif
