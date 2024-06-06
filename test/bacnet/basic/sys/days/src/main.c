/*
 * Copyright (c) 2021 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/sys/days.h>

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
    zassert_equal(year, test_year, NULL);
    zassert_equal(month, test_month, NULL);
    zassert_equal(day, test_day, NULL);
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
    const uint16_t epoch_year = 2000;

    test_epoch_conversion_date(epoch_year, 2000, 1, 1);
    test_epoch_conversion_date(epoch_year, 2048, 2, 28);
    test_epoch_conversion_date(epoch_year, 2048, 2, 29);
    test_epoch_conversion_date(epoch_year, 2038, 6, 15);
    test_epoch_conversion_date(epoch_year, 9999, 12, 31);
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
        ztest_unit_test(test_days_of_year_to_md),
        ztest_unit_test(test_days_date_is_valid),
        ztest_unit_test(test_days_apart));

    ztest_run_test_suite(days_tests);
}
#endif
