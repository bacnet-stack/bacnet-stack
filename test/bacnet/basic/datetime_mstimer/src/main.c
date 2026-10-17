/**
 * @file
 * @brief Unit tests for the millisecond-timer datetime backend
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
#include "bacnet/datetime.h"

static unsigned long Fake_Milliseconds;
/* Added to Fake_Milliseconds on every mstimer_now() sample. */
static unsigned long Now_Advance;
static unsigned Lock_Count;
static unsigned Unlock_Count;
static unsigned Observer_One_Count;
static unsigned Observer_Two_Count;
static unsigned Observer_Local_Read_Count;
static BACNET_DATETIME_CHANGE Last_Change;

unsigned long mstimer_now(void)
{
    Fake_Milliseconds += Now_Advance;

    return Fake_Milliseconds;
}

static void test_datetime_lock(void)
{
    Lock_Count++;
}

static void test_datetime_unlock(void)
{
    Unlock_Count++;
}

static void test_datetime_observer_one(
    BACNET_DATETIME_CHANGE change, const BACNET_DATE_TIME *standard_time)
{
    BACNET_DATE_TIME local_time = { 0 };

    (void)standard_time;
    Last_Change = change;
    Observer_One_Count++;
    if (datetime_local(&local_time.date, &local_time.time, NULL, NULL)) {
        Observer_Local_Read_Count++;
    }
}

static void test_datetime_observer_two(
    BACNET_DATETIME_CHANGE change, const BACNET_DATE_TIME *standard_time)
{
    (void)change;
    (void)standard_time;
    Observer_Two_Count++;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer, test_datetime_backend)
#else
static void test_datetime_backend(void)
#endif
{
    static DATETIME_CHANGE_NOTIFICATION observer_one = {
        NULL, test_datetime_observer_one
    };
    static DATETIME_CHANGE_NOTIFICATION observer_two = {
        NULL, test_datetime_observer_two
    };
    BACNET_DATE_TIME standard_time = { 0 };
    BACNET_DATE_TIME read_standard = { 0 };
    BACNET_DATE date = { 0 };
    BACNET_TIME time = { 0 };
    bool dst_active = false;
    unsigned notifications = 0;

    Fake_Milliseconds = 0;
    Now_Advance = 0;
    Lock_Count = 0;
    Unlock_Count = 0;
    Observer_One_Count = 0;
    Observer_Two_Count = 0;
    Observer_Local_Read_Count = 0;
    datetime_lock_callback_set(test_datetime_lock, test_datetime_unlock);
    datetime_reset();
    zassert_false(datetime_valid(), NULL);
    datetime_init();
    datetime_dst_date_range_set(3, 10, 11, 3);
    datetime_dst_enabled_set(true);
    datetime_utc_offset_minutes_set(300);

    datetime_set_values(&standard_time, 2024, 1, 15, 17, 0, 0, 0);
    datetime_timesync(&standard_time.date, &standard_time.time, true);
    zassert_true(datetime_valid(), NULL);
    zassert_true(datetime_standard(&read_standard), NULL);
    zassert_equal(read_standard.time.hour, 12, NULL);
    Fake_Milliseconds += 500;
    zassert_true(datetime_local(&date, &time, NULL, NULL), NULL);
    zassert_equal(time.hundredths, 50, NULL);

    datetime_utc_offset_minutes_set(-60);
    datetime_set_values(&standard_time, 2024, 1, 15, 17, 0, 0, 0);
    datetime_timesync(&standard_time.date, &standard_time.time, true);
    zassert_true(datetime_local(&date, &time, NULL, &dst_active), NULL);
    zassert_equal(time.hour, 18, NULL);
    zassert_false(dst_active, NULL);

    datetime_utc_offset_minutes_set(0);
    datetime_set_values(&standard_time, 2024, 3, 10, 1, 59, 59, 0);
    datetime_standard_set(&standard_time);
    datetime_init();
    zassert_true(datetime_dst_enabled(), NULL);
    Fake_Milliseconds += 1000;
    zassert_true(datetime_local(&date, &time, NULL, &dst_active), NULL);
    zassert_equal(time.hour, 3, NULL);
    zassert_equal(time.min, 0, NULL);
    zassert_true(dst_active, NULL);

    datetime_set_values(&standard_time, 2024, 11, 3, 0, 59, 59, 0);
    datetime_standard_set(&standard_time);
    Fake_Milliseconds += 1000;
    zassert_true(datetime_local(&date, &time, NULL, &dst_active), NULL);
    zassert_equal(time.hour, 1, NULL);
    zassert_false(dst_active, NULL);
    Fake_Milliseconds += 1000;
    zassert_true(datetime_local(&date, &time, NULL, &dst_active), NULL);
    zassert_equal(time.hour, 1, NULL);
    zassert_false(dst_active, NULL);

    datetime_set_values(&standard_time, 2024, 7, 1, 14, 0, 0, 0);
    datetime_timesync(&standard_time.date, &standard_time.time, false);
    zassert_true(datetime_standard(&read_standard), NULL);
    zassert_equal(read_standard.time.hour, 13, NULL);
    zassert_true(datetime_local(&date, &time, NULL, &dst_active), NULL);
    zassert_equal(time.hour, 14, NULL);
    zassert_true(dst_active, NULL);

    datetime_set_values(&standard_time, 2024, 11, 3, 1, 30, 0, 0);
    datetime_timesync(&standard_time.date, &standard_time.time, false);
    zassert_true(datetime_standard(&read_standard), NULL);
    zassert_equal(read_standard.time.hour, 1, NULL);
    zassert_true(datetime_local(&date, &time, NULL, &dst_active), NULL);
    zassert_equal(time.hour, 1, NULL);
    zassert_false(dst_active, NULL);

    datetime_change_notification_add(&observer_one);
    datetime_change_notification_add(&observer_two);
    datetime_utc_offset_minutes_set(60);
    zassert_equal(Observer_One_Count, 1, NULL);
    zassert_equal(Last_Change, DATETIME_CHANGE_UTC_OFFSET, NULL);
    datetime_utc_offset_minutes_set(60);
    zassert_equal(Observer_One_Count, 1, NULL);
    datetime_dst_enabled_set(false);
    zassert_equal(Observer_One_Count, 2, NULL);
    zassert_equal(Last_Change, DATETIME_CHANGE_DST, NULL);
    datetime_dst_enabled_set(true);
    zassert_equal(Observer_One_Count, 3, NULL);
    zassert_true(datetime_standard(&standard_time), NULL);
    datetime_standard_set(&standard_time);
    zassert_equal(Observer_One_Count, 3, NULL);
    datetime_timesync_deadband_set(0);
    datetime_set_values(&standard_time, 2024, 7, 1, 14, 0, 0, 0);
    datetime_timesync(&standard_time.date, &standard_time.time, false);
    zassert_equal(Observer_One_Count, 4, NULL);
    zassert_equal(Observer_Two_Count, 4, NULL);
    zassert_equal(Last_Change, DATETIME_CHANGE_TIME, NULL);
    zassert_equal(Observer_Local_Read_Count, 4, NULL);

    datetime_timesync_deadband_set(1000);
    notifications = Observer_One_Count;
    zassert_true(datetime_local(&date, &time, NULL, NULL), NULL);
    datetime_set(&standard_time, &date, &time);
    datetime_timesync(&standard_time.date, &standard_time.time, false);
    zassert_equal(Observer_One_Count, notifications, NULL);

    zassert_equal(Lock_Count, Unlock_Count, NULL);
    notifications = Lock_Count;
    datetime_lock_callback_set(test_datetime_lock, NULL);
    datetime_lock();
    datetime_unlock();
    zassert_equal(Lock_Count, notifications, NULL);
    zassert_equal(Unlock_Count, notifications, NULL);
    datetime_lock_callback_set(NULL, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer, test_datetime_timesync_partial)
#else
static void test_datetime_timesync_partial(void)
#endif
{
    BACNET_DATE_TIME standard_time = { 0 };
    BACNET_DATE_TIME read_standard = { 0 };
    BACNET_DATE date = { 0 };
    BACNET_TIME time = { 0 };
    bool dst_active = false;

    Now_Advance = 0;
    Fake_Milliseconds = 0;
    datetime_lock_callback_set(NULL, NULL);
    datetime_reset();
    datetime_dst_date_range_set(3, 10, 11, 3);
    datetime_dst_enabled_set(true);
    datetime_utc_offset_minutes_set(300);
    datetime_timesync_deadband_set(0);

    /* date-only local request during DST preserves the local time */
    datetime_set_values(&standard_time, 2024, 7, 1, 13, 0, 0, 0);
    datetime_standard_set(&standard_time);
    datetime_set_date(&date, 2024, 7, 2);
    datetime_timesync(&date, NULL, false);
    zassert_true(datetime_standard(&read_standard), NULL);
    zassert_equal(read_standard.date.day, 2, NULL);
    zassert_equal(read_standard.time.hour, 13, NULL);
    zassert_true(datetime_local(&date, &time, NULL, &dst_active), NULL);
    zassert_equal(date.day, 2, NULL);
    zassert_equal(time.hour, 14, NULL);
    zassert_true(dst_active, NULL);

    /* time-only local request keeps the local date across midnight */
    datetime_set_values(&standard_time, 2024, 7, 1, 23, 30, 0, 0);
    datetime_standard_set(&standard_time);
    datetime_set_time(&time, 0, 45, 0, 0);
    datetime_timesync(NULL, &time, false);
    zassert_true(datetime_standard(&read_standard), NULL);
    zassert_equal(read_standard.date.day, 1, NULL);
    zassert_equal(read_standard.time.hour, 23, NULL);
    zassert_equal(read_standard.time.min, 45, NULL);
    zassert_true(datetime_local(&date, &time, NULL, NULL), NULL);
    zassert_equal(date.day, 2, NULL);
    zassert_equal(time.hour, 0, NULL);
    zassert_equal(time.min, 45, NULL);

    /* date-only UTC request preserves the UTC time */
    datetime_set_values(&standard_time, 2024, 7, 1, 13, 0, 0, 0);
    datetime_standard_set(&standard_time);
    datetime_set_date(&date, 2024, 7, 3);
    datetime_timesync(&date, NULL, true);
    zassert_true(datetime_standard(&read_standard), NULL);
    zassert_equal(read_standard.date.day, 3, NULL);
    zassert_equal(read_standard.time.hour, 13, NULL);

    /* time-only UTC request keeps the UTC date across midnight */
    datetime_set_values(&standard_time, 2024, 7, 1, 20, 0, 0, 0);
    datetime_standard_set(&standard_time);
    datetime_set_time(&time, 1, 30, 0, 0);
    datetime_timesync(NULL, &time, true);
    zassert_true(datetime_standard(&read_standard), NULL);
    zassert_equal(read_standard.date.day, 1, NULL);
    zassert_equal(read_standard.time.hour, 20, NULL);
    zassert_equal(read_standard.time.min, 30, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer, test_datetime_sync_keeps_sampled_ticks)
#else
static void test_datetime_sync_keeps_sampled_ticks(void)
#endif
{
    BACNET_DATE_TIME standard_time = { 0 };
    BACNET_DATE_TIME read_standard = { 0 };
    unsigned i = 0;

    Now_Advance = 0;
    Fake_Milliseconds = 0;
    datetime_lock_callback_set(NULL, NULL);
    datetime_reset();
    datetime_set_values(&standard_time, 2024, 1, 1, 0, 0, 0, 0);
    datetime_standard_set(&standard_time);
    /* Each clock read samples mstimer_now(), and that sample moves
     * the monotonic clock by 1 ms. The origin must advance by the
     * sampled amount so those ticks stay on the BACnet clock. */
    Now_Advance = 1;
    for (i = 0; i < 1000U; i++) {
        zassert_true(datetime_standard(&read_standard), NULL);
    }
    Now_Advance = 0;
    zassert_equal(Fake_Milliseconds, 1000, NULL);
    zassert_equal(read_standard.time.hour, 0, NULL);
    zassert_equal(read_standard.time.min, 0, NULL);
    zassert_equal(read_standard.time.sec, 1, NULL);
    zassert_equal(read_standard.time.hundredths, 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(datetime_mstimer, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        datetime_mstimer, ztest_unit_test(test_datetime_backend),
        ztest_unit_test(test_datetime_timesync_partial),
        ztest_unit_test(test_datetime_sync_keeps_sampled_ticks));
    ztest_run_test_suite(datetime_mstimer);
}
#endif
