/* @file
 * @brief tests day of year calculations API
 * @date August 2021
 * @author Steve Karg <Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/days.h>

/* define our epic beginnings */
#define BACNET_EPOCH_YEAR 1900
/* 1/1/1900 is a Monday */
/* Monday=1..Sunday=7 */
#define BACNET_EPOCH_DOW 1

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * Unit Test for the days, checking the epoch conversion
 */
static void test_epoch_conversion_date(
    uint16_t epoch_year, uint16_t year, uint8_t month, uint8_t day)
{
    uint32_t days;
    uint16_t test_year;
    uint8_t test_month;
    uint8_t test_day;

    /* conversions of day and date */
    days = days_since_epoch(epoch_year, year, month, day);
    days_since_epoch_to_date(
        epoch_year, days, &test_year, &test_month, &test_day);
    zassert_equal(
        year, test_year, "date=%u/%u/%u year=%u test_year=%u", year, month, day,
        year, test_year);
    zassert_equal(
        month, test_month, "date=%u/%u/%u month=%u test_month=%u", year, month,
        day, month, test_month);
    zassert_equal(
        day, test_day, "date=%u/%u/%u day=%u test_day=%u", year, month, day,
        day, test_day);
}

/**
 * Unit Test for the day of week based on epoch year and epoch day of week
 * @param epoch_year - years after Christ birth (0..9999 AD)
 * @param epoch_dow - day of week (1=Monday...7=Sunday)
 * @param year - years after Christ birth (0..9999 AD)
 * @param month - months (1=Jan...12=Dec)
 * @param day - day of month (1-31)
 * @param dow - day of week (1=Monday...7=Sunday)
 */
static void test_epoch_conversion_day(
    uint16_t epoch_year,
    uint8_t epoch_dow,
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t dow)
{
    uint32_t days;
    uint16_t test_dow;

    /* conversions of day and date */
    days = days_since_epoch(epoch_year, year, month, day);
    test_dow = days_of_week(epoch_dow, days);
    zassert_equal(
        dow, test_dow, "date=%u/%u/%u dow=%u test_dow=%u", year, month, day,
        dow, test_dow);
}

/**
 * Unit Test for the epoch
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(days_tests, test_days_epoch_conversion)
#else
static void test_days_epoch_conversion(void)
#endif
{
    const uint16_t epoch_year = BACNET_EPOCH_YEAR;
    const uint8_t epoch_day_of_week = BACNET_EPOCH_DOW;

    test_epoch_conversion_date(epoch_year, 2000, 1, 1);
    test_epoch_conversion_date(epoch_year, 2048, 2, 28);
    test_epoch_conversion_date(epoch_year, 2048, 2, 29);
    test_epoch_conversion_date(epoch_year, 2038, 6, 15);
    test_epoch_conversion_date(epoch_year, 9999, 12, 31);

    test_epoch_conversion_day(
        epoch_year, epoch_day_of_week, epoch_year, 1, 1, epoch_day_of_week);
    /* some known day of week (1=Monday...7=Sunday) */
    test_epoch_conversion_day(epoch_year, epoch_day_of_week, 2003, 1, 6, 1);
    test_epoch_conversion_day(epoch_year, epoch_day_of_week, 2003, 1, 7, 2);
    test_epoch_conversion_day(epoch_year, epoch_day_of_week, 2003, 1, 8, 3);
    test_epoch_conversion_day(epoch_year, epoch_day_of_week, 2003, 1, 9, 4);
    test_epoch_conversion_day(epoch_year, epoch_day_of_week, 2003, 1, 10, 5);
    test_epoch_conversion_day(epoch_year, epoch_day_of_week, 2003, 1, 11, 6);
    test_epoch_conversion_day(epoch_year, epoch_day_of_week, 2003, 1, 12, 7);
    test_epoch_conversion_day(epoch_year, epoch_day_of_week, 2003, 1, 13, 1);
    /* 50th wedding anniversary */
    test_epoch_conversion_day(epoch_year, epoch_day_of_week, 2043, 6, 26, 5);
}

/**
 * Unit Test for the days and year to month date year
 */
static void test_days_of_year_to_month_day_date(
    uint16_t year, uint16_t days, uint8_t month, uint8_t day)
{
    uint8_t test_month = 0;
    uint8_t test_day = 0;
    /* conversions of days and year */
    days_of_year_to_month_day(days, year, &test_month, &test_day);
    zassert_equal(month, test_month, NULL);
    zassert_equal(day, test_day, NULL);
}

/**
 * Unit Test for the days and year to month date year
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(days_tests, test_days_of_year_to_md)
#else
static void test_days_of_year_to_md(void)
#endif
{
    test_days_of_year_to_month_day_date(2029, 145, 5, 25);
    test_days_of_year_to_month_day_date(2000, 260, 9, 16);
    test_days_of_year_to_month_day_date(1995, 67, 3, 8);
    test_days_of_year_to_month_day_date(2092, 366, 12, 31);
    test_days_of_year_to_month_day_date(2070, 105, 4, 15);
}

/**
 * Unit Test for the days, checking the date to see if it is a valid day
 */
static void test_date_is_valid_day(uint16_t year, uint8_t month)
{
    uint8_t last_day = days_per_month(year, month);

    zassert_equal(days_date_is_valid(year, month, 0), false, NULL);
    zassert_equal(days_date_is_valid(year, month, 1), true, NULL);
    zassert_equal(days_date_is_valid(year, month, 15), true, NULL);
    zassert_equal(days_date_is_valid(year, month, last_day), true, NULL);
    zassert_equal(days_date_is_valid(year, month, 32), false, NULL);
}

/**
 * Unit Test for the days, checking the date to see if it is a valid date
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(days_tests, test_days_date_is_valid)
#else
static void test_days_date_is_valid(void)
#endif
{
    /* first month */
    test_date_is_valid_day(0, 1);
    test_date_is_valid_day(2012, 1);
    test_date_is_valid_day(9999, 1);
    /* middle month */
    test_date_is_valid_day(0, 6);
    test_date_is_valid_day(2012, 6);
    test_date_is_valid_day(9999, 6);
    /* last month */
    test_date_is_valid_day(0, 12);
    test_date_is_valid_day(2012, 12);
    test_date_is_valid_day(9999, 12);
    /* february */
    test_date_is_valid_day(0, 2);
    test_date_is_valid_day(2000, 2);
    test_date_is_valid_day(2001, 2);
    test_date_is_valid_day(2002, 2);
    test_date_is_valid_day(2003, 2);
    test_date_is_valid_day(2004, 2);
    test_date_is_valid_day(9999, 2);
    /* invalid months */
    zassert_equal(days_per_month(0, 0), 0, NULL);
    zassert_equal(days_per_month(0, 13), 0, NULL);
    zassert_equal(days_per_month(0, 99), 0, NULL);
    zassert_equal(days_per_month(0, 0), 0, NULL);
}

/**
 * Unit Test for the days, checking the date to see if it is a valid date
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(days_tests, test_days_since_epoch)
#else
static void test_days_since_epoch(void)
#endif
{
    uint32_t days = 0;
    uint16_t year = 0, test_year = 0;
    uint8_t month = 0, test_month = 0;
    uint8_t day = 0, test_day = 0;

    days = days_since_epoch(BACNET_EPOCH_YEAR, BACNET_EPOCH_YEAR, 1, 1);
    zassert_equal(days, 0, "days=%lu", (unsigned long)days);
    days_since_epoch_to_date(BACNET_EPOCH_YEAR, days, &year, &month, &day);
    zassert_equal(year, BACNET_EPOCH_YEAR, NULL);
    zassert_equal(month, 1, NULL);
    zassert_equal(day, 1, NULL);

    for (year = BACNET_EPOCH_YEAR; year < (BACNET_EPOCH_YEAR + 0xFF); year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= days_per_month(year, month); day++) {
                days = days_since_epoch(BACNET_EPOCH_YEAR, year, month, day);
                days_since_epoch_to_date(
                    BACNET_EPOCH_YEAR, days, &test_year, &test_month,
                    &test_day);
                zassert_equal(year, test_year, NULL);
                zassert_equal(month, test_month, NULL);
                zassert_equal(day, test_day, NULL);
            }
        }
    }
}

/**
 * Unit Test for days apart, checking the dates to see how many days apart
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(days_tests, test_days_apart)
#else
static void test_days_apart(void)
#endif
{
    zassert_equal(days_apart(2000, 1, 1, 2000, 1, 1), 0, NULL);
    zassert_equal(days_apart(2000, 1, 1, 2000, 1, 2), 1, NULL);
    zassert_equal(days_apart(2000, 1, 1, 2000, 2, 1), 31, NULL);
    zassert_equal(days_apart(2000, 1, 1, 2000, 12, 31), 365, NULL);
    zassert_equal(days_apart(2000, 1, 1, 2001, 1, 1), 366, NULL);
    zassert_equal(days_apart(2001, 1, 1, 2000, 1, 1), 366, NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(days_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        days_tests, ztest_unit_test(test_days_epoch_conversion),
        ztest_unit_test(test_days_since_epoch),
        ztest_unit_test(test_days_of_year_to_md),
        ztest_unit_test(test_days_date_is_valid),
        ztest_unit_test(test_days_apart));

    ztest_run_test_suite(days_tests);
}
#endif
