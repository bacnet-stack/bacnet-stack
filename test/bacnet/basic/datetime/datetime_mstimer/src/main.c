/**
 * @file
 * @brief test the millisecond timer based time-of-day clock
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
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
    zassert_equal(bdate.year, BACNET_DATE_YEAR_EPOCH, NULL);
    zassert_equal(bdate.month, 1, NULL);
    zassert_equal(bdate.day, 1, NULL);
    zassert_equal(bdate.wday, BACNET_WEEKDAY_MONDAY, NULL);
    zassert_equal(btime.hour, 0, NULL);
    zassert_equal(btime.min, 0, NULL);
    zassert_equal(btime.sec, 0, NULL);
    zassert_equal(btime.hundredths, 0, NULL);
    zassert_equal(utc_offset, 0, NULL);
    zassert_false(dst_active, NULL);
}

/**
 * @brief Test the clock starts at a valid date and time and keeps running
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer_tests, test_datetime_start_value)
#else
static void test_datetime_start_value(void)
#endif
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };
    BACNET_DATE_TIME bdatetime = { 0 };
    uint8_t apdu[32] = { 0 };
    BACNET_DATE test_date = { 0 };
    int len;

    Fake_Milliseconds = 0;
    datetime_init_defaults();
    zassert_true(datetime_local(&bdate, &btime, NULL, NULL), NULL);
    zassert_equal(bdate.year, BACNET_DATE_YEAR_EPOCH, NULL);
    zassert_equal(bdate.month, 1, NULL);
    zassert_equal(bdate.day, 1, NULL);
    zassert_equal(bdate.wday, BACNET_WEEKDAY_MONDAY, NULL);
    zassert_equal(btime.sec, 0, NULL);
    zassert_equal(btime.hundredths, 0, NULL);
    /* an encoded Local_Date has valid month, day, and weekday */
    len = bacnet_date_application_encode(apdu, sizeof(apdu), &bdate);
    zassert_true(len > 0, NULL);
    len = bacnet_date_application_decode(apdu, sizeof(apdu), &test_date);
    zassert_true(len > 0, NULL);
    zassert_true(
        datetime_ymd_is_valid(test_date.year, test_date.month, test_date.day),
        NULL);
    zassert_true(test_date.wday >= 1 && test_date.wday <= 7, NULL);

    /* the clock keeps running from the start value */
    Fake_Milliseconds = 90 * 1000UL + 250;
    zassert_true(datetime_standard(&bdatetime), NULL);
    zassert_equal(bdatetime.date.year, BACNET_DATE_YEAR_EPOCH, NULL);
    zassert_equal(bdatetime.date.month, 1, NULL);
    zassert_equal(bdatetime.date.day, 1, NULL);
    zassert_equal(bdatetime.time.min, 1, NULL);
    zassert_equal(bdatetime.time.sec, 30, NULL);
    zassert_equal(bdatetime.time.hundredths, 25, NULL);
    Fake_Milliseconds = 86400UL * 1000UL;
    zassert_true(datetime_standard(&bdatetime), NULL);
    zassert_equal(bdatetime.date.day, 2, NULL);
    zassert_equal(bdatetime.date.wday, BACNET_WEEKDAY_TUESDAY, NULL);
    zassert_equal(bdatetime.time.hour, 0, NULL);
}

static void set_time(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t second)
{
    datetime_set_date(bdate, year, month, day);
    datetime_set_time(btime, hour, minute, second, 0);
}

static void check_local(
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t second,
    bool dst_expected)
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };
    int16_t utc_offset = 0;
    bool dst_active = false;

    zassert_true(
        datetime_local(&bdate, &btime, &utc_offset, &dst_active), NULL);
    zassert_equal(bdate.year, year, "year=%u", bdate.year);
    zassert_equal(bdate.month, month, "month=%u", bdate.month);
    zassert_equal(bdate.day, day, "day=%u", bdate.day);
    zassert_equal(btime.hour, hour, "hour=%u", btime.hour);
    zassert_equal(btime.min, minute, "min=%u", btime.min);
    zassert_equal(btime.sec, second, "sec=%u", btime.sec);
    zassert_equal(dst_active, dst_expected, NULL);
}

/**
 * @brief Test local time sync with and without DST
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer_tests, test_datetime_timesync_local)
#else
static void test_datetime_timesync_local(void)
#endif
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };

    Fake_Milliseconds = 0;
    datetime_init_defaults();
    datetime_dst_enabled_set(true);

    /* summer wall-clock time is stored as standard time */
    set_time(&bdate, &btime, 2026, 6, 1, 12, 0, 0);
    datetime_timesync(&bdate, &btime, false);
    check_local(2026, 6, 1, 12, 0, 0, true);
    datetime_dst_enabled_set(false);
    check_local(2026, 6, 1, 11, 0, 0, false);
    datetime_dst_enabled_set(true);

    /* wall-clock just after midnight in DST crosses back a day */
    set_time(&bdate, &btime, 2026, 6, 1, 0, 30, 15);
    datetime_timesync(&bdate, &btime, false);
    check_local(2026, 6, 1, 0, 30, 15, true);

    /* winter time is unchanged */
    set_time(&bdate, &btime, 2026, 1, 15, 12, 0, 0);
    datetime_timesync(&bdate, &btime, false);
    check_local(2026, 1, 15, 12, 0, 0, false);
    datetime_dst_enabled_set(false);
    check_local(2026, 1, 15, 12, 0, 0, false);
}

/**
 * @brief Test the clock follows the DST changeover while running
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer_tests, test_datetime_dst_changeover)
#else
static void test_datetime_dst_changeover(void)
#endif
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };

    Fake_Milliseconds = 0;
    datetime_init_defaults();
    datetime_dst_enabled_set(true);

    /* spring forward: 2026-03-08 01:59:58 -> 03:00:00 */
    set_time(&bdate, &btime, 2026, 3, 8, 1, 59, 58);
    datetime_timesync(&bdate, &btime, false);
    check_local(2026, 3, 8, 1, 59, 58, false);
    Fake_Milliseconds += 1000;
    check_local(2026, 3, 8, 1, 59, 59, false);
    Fake_Milliseconds += 1000;
    check_local(2026, 3, 8, 3, 0, 0, true);
    Fake_Milliseconds += 1000;
    check_local(2026, 3, 8, 3, 0, 1, true);

    /* wall-clock time after the changeover */
    set_time(&bdate, &btime, 2026, 3, 8, 3, 30, 0);
    datetime_timesync(&bdate, &btime, false);
    check_local(2026, 3, 8, 3, 30, 0, true);
    /* time that does not exist is stored as standard time */
    set_time(&bdate, &btime, 2026, 3, 8, 2, 30, 0);
    datetime_timesync(&bdate, &btime, false);
    check_local(2026, 3, 8, 3, 30, 0, true);

    /* fall back: 2026-11-01 01:59:58 DST -> 01:00:00 standard */
    set_time(&bdate, &btime, 2026, 11, 1, 1, 59, 58);
    datetime_timesync(&bdate, &btime, false);
    /* repeated hour is standard time */
    check_local(2026, 11, 1, 1, 59, 58, false);
    set_time(&bdate, &btime, 2026, 11, 1, 0, 59, 58);
    datetime_timesync(&bdate, &btime, false);
    check_local(2026, 11, 1, 0, 59, 58, true);
    Fake_Milliseconds += 1000;
    check_local(2026, 11, 1, 0, 59, 59, true);
    Fake_Milliseconds += 1000;
    check_local(2026, 11, 1, 1, 0, 0, true);
    Fake_Milliseconds += 3599000;
    check_local(2026, 11, 1, 1, 59, 59, true);
    Fake_Milliseconds += 1000;
    check_local(2026, 11, 1, 1, 0, 0, false);
    Fake_Milliseconds += 3600000;
    check_local(2026, 11, 1, 2, 0, 0, false);
}

/**
 * @brief Test UTC time sync stores standard time without a DST step
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer_tests, test_datetime_timesync_utc)
#else
static void test_datetime_timesync_utc(void)
#endif
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };

    Fake_Milliseconds = 0;
    datetime_init_defaults();
    datetime_dst_enabled_set(true);
    /* BACnet UTC offset is positive West of UTC: US Central */
    datetime_utc_offset_minutes_set(360);

    /* 17:00 UTC in summer is 11:00 standard and 12:00 daylight */
    set_time(&bdate, &btime, 2026, 6, 1, 17, 0, 0);
    datetime_timesync(&bdate, &btime, true);
    check_local(2026, 6, 1, 12, 0, 0, true);
    datetime_dst_enabled_set(false);
    check_local(2026, 6, 1, 11, 0, 0, false);

    /* 17:00 UTC in winter is 11:00 standard */
    datetime_dst_enabled_set(true);
    set_time(&bdate, &btime, 2026, 1, 15, 17, 0, 0);
    datetime_timesync(&bdate, &btime, true);
    check_local(2026, 1, 15, 11, 0, 0, false);

    /* BACnet UTC offset is positive west of UTC: US Eastern is +300 */
    datetime_utc_offset_minutes_set(300);
    set_time(&bdate, &btime, 2026, 1, 15, 17, 0, 0);
    datetime_timesync(&bdate, &btime, true);
    check_local(2026, 1, 15, 12, 0, 0, false);
    /* a UTC time that crosses midnight */
    set_time(&bdate, &btime, 2026, 1, 15, 3, 0, 0);
    datetime_timesync(&bdate, &btime, true);
    check_local(2026, 1, 14, 22, 0, 0, false);
}

/**
 * @brief Test configuration changes apply on the next read without
 *  changing the stored time
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer_tests, test_datetime_config_change)
#else
static void test_datetime_config_change(void)
#endif
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };

    Fake_Milliseconds = 0;
    datetime_init_defaults();
    datetime_dst_enabled_set(true);
    set_time(&bdate, &btime, 2026, 7, 1, 12, 0, 0);
    datetime_timesync(&bdate, &btime, false);
    check_local(2026, 7, 1, 12, 0, 0, true);

    /* disable DST */
    datetime_dst_enabled_set(false);
    check_local(2026, 7, 1, 11, 0, 0, false);
    datetime_dst_enabled_set(true);
    check_local(2026, 7, 1, 12, 0, 0, true);

    /* change the DST rule so that July is outside of DST */
    zassert_true(datetime_dst_date_range_set(1, 1, 3, 31), NULL);
    check_local(2026, 7, 1, 11, 0, 0, false);
    zassert_true(datetime_dst_date_range_set(4, 1, 10, 31), NULL);
    check_local(2026, 7, 1, 12, 0, 0, true);

    /* UTC offset does not alter the stored time */
    datetime_utc_offset_minutes_set(360);
    check_local(2026, 7, 1, 12, 0, 0, true);
}

/**
 * @brief Test the standard time API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer_tests, test_datetime_standard)
#else
static void test_datetime_standard(void)
#endif
{
    BACNET_DATE_TIME standard = { 0 };
    BACNET_DATE_TIME rtc = { 0 };
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };

    Fake_Milliseconds = 0;
    datetime_init_defaults();
    datetime_dst_enabled_set(true);
    datetime_dst_date_range_set(4, 1, 10, 31);

    /* NULL handling */
    zassert_false(datetime_standard(NULL), NULL);
    datetime_standard_set(NULL);

    /* load standard time as if from a hardware RTC at power up */
    datetime_set_values(&rtc, 2026, 7, 1, 11, 0, 0, 0);
    datetime_standard_set(&rtc);
    zassert_true(datetime_standard(&standard), NULL);
    zassert_equal(datetime_compare(&standard, &rtc), 0, NULL);
    /* local time has the DST adjustment applied */
    check_local(2026, 7, 1, 12, 0, 0, true);

    /* the standard time runs with the millisecond timer */
    Fake_Milliseconds = 5000;
    zassert_true(datetime_standard(&standard), NULL);
    zassert_equal(standard.time.hour, 11, NULL);
    zassert_equal(standard.time.sec, 5, NULL);
    check_local(2026, 7, 1, 12, 0, 5, true);

    /* standard time is unchanged when DST is disabled or not in range */
    datetime_dst_enabled_set(false);
    zassert_true(datetime_standard(&standard), NULL);
    zassert_equal(standard.time.hour, 11, NULL);
    zassert_equal(standard.time.sec, 5, NULL);
    check_local(2026, 7, 1, 11, 0, 5, false);
    datetime_dst_enabled_set(true);

    /* standard time after a wall-clock time sync with DST active */
    set_time(&bdate, &btime, 2026, 7, 1, 12, 0, 0);
    datetime_timesync(&bdate, &btime, false);
    zassert_true(datetime_standard(&standard), NULL);
    zassert_equal(standard.date.month, 7, NULL);
    zassert_equal(standard.time.hour, 11, NULL);
    zassert_equal(standard.time.min, 0, NULL);

    /* standard time across midnight when DST is active */
    datetime_set_values(&rtc, 2026, 7, 1, 23, 30, 0, 0);
    datetime_standard_set(&rtc);
    check_local(2026, 7, 2, 0, 30, 0, true);
    zassert_true(datetime_standard(&standard), NULL);
    zassert_equal(standard.date.day, 1, NULL);
    zassert_equal(standard.time.hour, 23, NULL);

    /* set restarts the timer: the elapsed time is not added after the set */
    Fake_Milliseconds = 20000;
    datetime_standard_set(&rtc);
    zassert_true(datetime_standard(&standard), NULL);
    zassert_equal(standard.time.min, 30, NULL);
    zassert_equal(standard.time.sec, 0, NULL);
}

/**
 * @brief Test the standard-time changed flag
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer_tests, test_datetime_standard_changed)
#else
static void test_datetime_standard_changed(void)
#endif
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };
    BACNET_DATE_TIME rtc = { 0 };

    Fake_Milliseconds = 0;
    datetime_init_defaults();
    zassert_false(datetime_standard_changed(), NULL);

    /* local timesync sets the latch when the time changes.
       The read clears it. A repeat of the same time does not. */
    set_time(&bdate, &btime, 2026, 1, 15, 10, 0, 0);
    datetime_timesync(&bdate, &btime, false);
    zassert_true(datetime_standard_changed(), NULL);
    zassert_false(datetime_standard_changed(), NULL);
    datetime_timesync(&bdate, &btime, false);
    zassert_false(datetime_standard_changed(), NULL);

    /* a later sync keeps a latch that the application has not read */
    set_time(&bdate, &btime, 2026, 1, 15, 10, 30, 0);
    datetime_timesync(&bdate, &btime, false);
    datetime_timesync(&bdate, &btime, false);
    zassert_true(datetime_standard_changed(), NULL);

    /* a date change sets the latch */
    set_time(&bdate, &btime, 2026, 1, 16, 10, 30, 0);
    datetime_timesync(&bdate, &btime, false);
    zassert_true(datetime_standard_changed(), NULL);

    /* UTC timesync sets the latch only when standard time changes.
       The offset set does not. 16:30 UTC is 10:30 standard. */
    datetime_utc_offset_minutes_set(360);
    zassert_false(datetime_standard_changed(), NULL);
    set_time(&bdate, &btime, 2026, 1, 16, 16, 30, 0);
    datetime_timesync(&bdate, &btime, true);
    zassert_false(datetime_standard_changed(), NULL);
    /* 18:00 UTC is 12:00 standard */
    set_time(&bdate, &btime, 2026, 1, 16, 18, 0, 0);
    datetime_timesync(&bdate, &btime, true);
    zassert_true(datetime_standard_changed(), NULL);

    /* standard set stores time and does not set the latch */
    datetime_set_values(&rtc, 2026, 7, 1, 23, 30, 0, 0);
    datetime_standard_set(NULL);
    zassert_false(datetime_standard_changed(), NULL);
    datetime_standard_set(&rtc);
    zassert_false(datetime_standard_changed(), NULL);
    datetime_standard_set(&rtc);
    zassert_false(datetime_standard_changed(), NULL);

    /* DST and offset changes do not store standard time.
       Enabling DST moves the wall-clock date across midnight. */
    zassert_true(datetime_dst_date_range_set(4, 1, 10, 31), NULL);
    zassert_false(datetime_standard_changed(), NULL);
    datetime_dst_enabled_set(true);
    zassert_false(datetime_standard_changed(), NULL);
    check_local(2026, 7, 2, 0, 30, 0, true);
    /* wall-clock 00:30 is standard 23:30. The stored time is the same. */
    set_time(&bdate, &btime, 2026, 7, 2, 0, 30, 0);
    datetime_timesync(&bdate, &btime, false);
    zassert_false(datetime_standard_changed(), NULL);
    zassert_true(
        datetime_dst_ordinal_range_set(
            3, 2, BACNET_WEEKDAY_SUNDAY, 11, 1, BACNET_WEEKDAY_SUNDAY),
        NULL);
    zassert_false(datetime_standard_changed(), NULL);
    zassert_true(datetime_utc_offset_minutes_set(300), NULL);
    zassert_false(datetime_standard_changed(), NULL);

    /* the free-running timer updates standard time without a set */
    Fake_Milliseconds = 5000;
    check_local(2026, 7, 2, 0, 30, 5, true);
    zassert_false(datetime_standard_changed(), NULL);

    /* init defaults clears a latch that the read did not consume */
    set_time(&bdate, &btime, 2026, 1, 15, 10, 0, 0);
    datetime_timesync(&bdate, &btime, false);
    datetime_init_defaults();
    zassert_false(datetime_standard_changed(), NULL);
}

/**
 * @brief Test the clock keeps the sub-second remainder between reads
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datetime_mstimer_tests, test_datetime_remainder)
#else
static void test_datetime_remainder(void)
#endif
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };
    BACNET_DATE_TIME start_time = { 0 }, now_time = { 0 };
    uint32_t elapsed_ms;
    unsigned long ms;

    Fake_Milliseconds = 0;
    datetime_init_defaults();
    set_time(&bdate, &btime, 2026, 1, 15, 10, 0, 0);
    datetime_timesync(&bdate, &btime, false);
    datetime_standard(&start_time);
    /* read every 1.5 seconds for 60 seconds */
    for (ms = 1500; ms <= 60000; ms += 1500) {
        Fake_Milliseconds = ms;
        zassert_true(datetime_standard(&now_time), NULL);
    }
    elapsed_ms = (datetime_seconds_since_epoch(&now_time) -
                  datetime_seconds_since_epoch(&start_time)) *
        1000UL;
    elapsed_ms += now_time.time.hundredths * 10UL;
    zassert_true(
        (elapsed_ms >= 59990UL) && (elapsed_ms <= 60010UL), "elapsed=%u",
        (unsigned)elapsed_ms);

    /* TimeSync at .50 seconds, read after 600 ms, expect +1.10 seconds */
    Fake_Milliseconds = 100000;
    datetime_set_time(&btime, 10, 0, 0, 50);
    datetime_timesync(&bdate, &btime, false);
    Fake_Milliseconds = 100600;
    zassert_true(datetime_standard(&now_time), NULL);
    zassert_equal(now_time.time.sec, 1, "sec=%u", now_time.time.sec);
    zassert_equal(
        now_time.time.hundredths, 10, "hundredths=%u",
        now_time.time.hundredths);

    /* a remainder under 10 ms is kept in the timer */
    Fake_Milliseconds = 100605;
    datetime_standard(&now_time);
    Fake_Milliseconds = 100610;
    datetime_standard(&now_time);
    zassert_equal(
        now_time.time.hundredths, 11, "hundredths=%u",
        now_time.time.hundredths);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(datetime_mstimer_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        datetime_mstimer_tests, ztest_unit_test(test_datetime_init_defaults),
        ztest_unit_test(test_datetime_start_value),
        ztest_unit_test(test_datetime_timesync_local),
        ztest_unit_test(test_datetime_dst_changeover),
        ztest_unit_test(test_datetime_timesync_utc),
        ztest_unit_test(test_datetime_config_change),
        ztest_unit_test(test_datetime_standard),
        ztest_unit_test(test_datetime_standard_changed),
        ztest_unit_test(test_datetime_remainder));

    ztest_run_test_suite(datetime_mstimer_tests);
}
#endif
