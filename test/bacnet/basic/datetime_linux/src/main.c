/**
 * @file
 * @brief Unit tests for Linux operating-system datetime behavior
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <zephyr/ztest.h>
#include "bacnet/datetime.h"

static unsigned Time_Change_Count;
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
ZTEST_SUITE(datetime_linux, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(datetime_linux, ztest_unit_test(test_host_timezone_sync));
    ztest_run_test_suite(datetime_linux);
}
#endif
