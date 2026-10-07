/* @file
 * @brief tests daylight savings time validity API
 * @date August 2021
 * @author Steve Karg <Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/dst.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * Unit Test for daylight savings time
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(dst_tests, dst_test_valid)
#else
static void dst_test_valid(void)
#endif
{
    struct daylight_savings_data data = { 0 };
    uint8_t epoch_day;
    uint16_t epoch_year;
    /* test at 3am */
    uint8_t hour = 3;
    uint8_t minute = 0;
    uint8_t second = 0;
    bool active;

    unsigned i;
    struct dst_test_data {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        bool active;
    } test_ordinal_data[] = {
        /* start date boundary checking for several years */
        { 2007, 3, 10, false },
        { 2007, 3, 11, true },
        { 2008, 3, 8, false },
        { 2008, 3, 9, true },
        { 2009, 3, 7, false },
        { 2009, 3, 8, true },
        { 2010, 3, 13, false },
        { 2010, 3, 14, true },
        { 2011, 3, 12, false },
        { 2011, 3, 13, true },
        { 2012, 3, 10, false },
        { 2012, 3, 11, true },
        { 2013, 3, 9, false },
        { 2013, 3, 10, true },
        { 2014, 3, 8, false },
        { 2014, 3, 9, true },
        { 2015, 3, 7, false },
        { 2015, 3, 8, true },
        /* end date boundary checking for several years */
        { 2007, 11, 3, true },
        { 2007, 11, 4, false },
        { 2008, 11, 1, true },
        { 2008, 11, 2, false },
        { 2009, 10, 31, true },
        { 2009, 11, 1, false },
        { 2010, 11, 6, true },
        { 2010, 11, 7, false },
        { 2011, 11, 5, true },
        { 2011, 11, 6, false },
        { 2012, 11, 3, true },
        { 2012, 11, 4, false },
        { 2013, 11, 2, true },
        { 2013, 11, 3, false },
        { 2014, 11, 1, true },
        { 2014, 11, 2, false },
        { 2015, 10, 31, true },
        { 2015, 11, 1, false },
        /* year long check boundaries over a year */
        { 2013, 1, 1, false },
        { 2013, 3, 3, false },
        { 2013, 3, 7, false },
        { 2013, 3, 8, false },
        { 2013, 3, 9, false },
        { 2013, 3, 10, true },
        { 2013, 3, 11, true },
        { 2013, 3, 12, true },
        { 2013, 7, 10, true },
        { 2013, 11, 2, true },
        { 2013, 11, 3, false },
        { 2013, 11, 4, false },
        { 2013, 11, 7, false },
        { 2013, 11, 8, false },
        { 2013, 11, 30, false },
        { 2013, 12, 31, false },
    };
    struct dst_test_data *td;

    dst_init_defaults(&data);
    for (i = 0; i < ARRAY_SIZE(test_ordinal_data); i++) {
        td = &test_ordinal_data[i];
        active = dst_active(
            &data, td->year, td->month, td->day, hour, minute, second);
        zassert_true(active == td->active, NULL);
    }
    /* test the fixed dates */
    epoch_day = data.Epoch_Day;
    epoch_year = data.Epoch_Year;
    dst_init(&data, false, 4, 1, 0, 9, 30, 0, epoch_day, epoch_year);
    /* check the boundaries */
    active = dst_active(&data, 2013, 3, 31, hour, minute, second);
    zassert_true(active == false, NULL);
    active = dst_active(&data, 2013, 4, 1, hour, minute, second);
    zassert_true(active == true, NULL);
    active = dst_active(&data, 2013, 4, 2, hour, minute, second);
    zassert_true(active == true, NULL);
    active = dst_active(&data, 2013, 9, 29, hour, minute, second);
    zassert_true(active == true, NULL);
    active = dst_active(&data, 2013, 9, 30, hour, minute, second);
    zassert_true(active == false, NULL);
    active = dst_active(&data, 2013, 10, 1, hour, minute, second);
    zassert_true(active == false, NULL);
}

/**
 * Unit Test for Southern Hemisphere style DST ranges that wrap
 * across the end of the year (Begin_Month > End_Month)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(dst_tests, dst_test_southern_hemisphere)
#else
static void dst_test_southern_hemisphere(void)
#endif
{
    struct daylight_savings_data data = { 0 };
    uint8_t epoch_day;
    uint16_t epoch_year;
    uint8_t hour = 3;
    uint8_t minute = 0;
    uint8_t second = 0;

    dst_init_defaults(&data);
    epoch_day = data.Epoch_Day;
    epoch_year = data.Epoch_Year;
    /* ordinal: begins 1st Sunday in October, ends 1st Sunday in April */
    dst_init(&data, true, 10, 7, 1, 4, 7, 1, epoch_day, epoch_year);
    /* within the wrap (Jan, before the April end) */
    zassert_true(
        dst_active(&data, 2013, 1, 15, hour, minute, second), NULL);
    /* within the wrap (Dec, after the October begin) */
    zassert_true(
        dst_active(&data, 2013, 12, 15, hour, minute, second), NULL);
    /* outside the wrap, in the middle of the year */
    zassert_false(
        dst_active(&data, 2013, 7, 15, hour, minute, second), NULL);

    /* date-based: begins October 1st, ends March 31st */
    dst_init(&data, false, 10, 1, 0, 3, 31, 0, epoch_day, epoch_year);
    zassert_true(
        dst_active(&data, 2013, 1, 15, hour, minute, second), NULL);
    zassert_true(
        dst_active(&data, 2013, 12, 15, hour, minute, second), NULL);
    zassert_false(
        dst_active(&data, 2013, 7, 15, hour, minute, second), NULL);
}

/**
 * Unit Test for calendar dates that do not exist (e.g. February 31st)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(dst_tests, dst_test_invalid_day)
#else
static void dst_test_invalid_day(void)
#endif
{
    struct daylight_savings_data data = { 0 };
    uint8_t hour = 3;
    uint8_t minute = 0;
    uint8_t second = 0;

    dst_init_defaults(&data);
    /* February 31st does not exist */
    zassert_false(
        dst_active(&data, 2013, 2, 31, hour, minute, second), NULL);
    /* April 31st does not exist */
    zassert_false(
        dst_active(&data, 2013, 4, 31, hour, minute, second), NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(dst_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        dst_tests, ztest_unit_test(dst_test_valid),
        ztest_unit_test(dst_test_southern_hemisphere),
        ztest_unit_test(dst_test_invalid_day));

    ztest_run_test_suite(dst_tests);
}
#endif
