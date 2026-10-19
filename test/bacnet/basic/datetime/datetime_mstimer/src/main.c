/**
 * @file
 * @brief test the millisecond timer based time-of-day clock
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/datetime.h>
#include <bacnet/basic/sys/mstimer.h>

static unsigned long Fake_Milliseconds;

/**
 * @brief Get the current time in milliseconds
 * @return milliseconds
 */
unsigned long mstimer_now(void)
{
    return Fake_Milliseconds;
}

/**
 * @brief Test datetime_init() initializes once and
 *  datetime_init_defaults() returns to defaults
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer_tests, test_datetime_init_defaults)
#else
static void test_datetime_init_defaults(void)
#endif
{
    uint8_t start_month = 0, start_week = 0, start_day = 0;
    uint8_t end_month = 0, end_week = 0, end_day = 0;
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };
    int16_t utc_offset = 0;
    bool dst_active = false;
    bool status;

    Fake_Milliseconds = 0;
    datetime_init_defaults();
    status = datetime_local(&bdate, &btime, &utc_offset, &dst_active);
    zassert_true(status, NULL);
    zassert_equal(utc_offset, 0, NULL);
    zassert_false(dst_active, NULL);
    zassert_false(datetime_dst_enabled(), NULL);
    status = datetime_dst_ordinal_range(
        &start_month, &start_week, &start_day, &end_month, &end_week, &end_day);
    zassert_true(status, NULL);
    zassert_equal(start_month, 3, NULL);
    zassert_equal(start_week, 2, NULL);
    zassert_equal(end_month, 11, NULL);
    zassert_equal(end_week, 1, NULL);

    /* change the configuration */
    datetime_utc_offset_minutes_set(300);
    datetime_dst_enabled_set(true);
    status = datetime_dst_date_range_set(4, 1, 10, 31);
    zassert_true(status, NULL);
    bdate.year = 2026;
    bdate.month = 6;
    bdate.day = 1;
    bdate.wday = 1;
    btime.hour = 12;
    datetime_timesync(&bdate, &btime, false);

    /* datetime_init() has no effect after the first initialization */
    datetime_init();
    datetime_init();
    zassert_equal(datetime_utc_offset_minutes(), 300, NULL);
    zassert_true(datetime_dst_enabled(), NULL);
    zassert_false(
        datetime_dst_ordinal_range(
            &start_month, &start_week, &start_day, &end_month, &end_week,
            &end_day),
        NULL);
    status = datetime_local(&bdate, &btime, &utc_offset, &dst_active);
    zassert_true(status, NULL);
    zassert_equal(bdate.year, 2026, NULL);
    zassert_equal(btime.hour, 12, NULL);
    zassert_equal(utc_offset, 300, NULL);
    zassert_true(dst_active, NULL);

    /* explicit return to defaults */
    datetime_init_defaults();
    zassert_equal(datetime_utc_offset_minutes(), 0, NULL);
    zassert_false(datetime_dst_enabled(), NULL);
    status = datetime_dst_ordinal_range(
        &start_month, &start_week, &start_day, &end_month, &end_week, &end_day);
    zassert_true(status, NULL);
    zassert_equal(start_month, 3, NULL);
    status = datetime_local(&bdate, &btime, &utc_offset, &dst_active);
    zassert_true(status, NULL);
    zassert_equal(bdate.year, 0, NULL);
    zassert_equal(btime.hour, 0, NULL);
    zassert_equal(utc_offset, 0, NULL);
    zassert_false(dst_active, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(datetime_mstimer_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        datetime_mstimer_tests, ztest_unit_test(test_datetime_init_defaults));

    ztest_run_test_suite(datetime_mstimer_tests);
}
#endif
