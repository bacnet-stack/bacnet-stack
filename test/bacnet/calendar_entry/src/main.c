/**
 * @file
 * @brief Unit test for BACnetCalendarEntry complex data type encode/decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/ztest.h>
#include "bacnet/calendar_entry.h"
#include "bacnet/bacdcode.h"
#include "bacnet/datetime.h"

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API for the Date choice, including a NULL
 *  destination which shall return the same length as a non-NULL decode.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetCalendarEntry_tests, test_BACnetCalendarEntry_Date)
#else
static void test_BACnetCalendarEntry_Date(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len, null_len, test_len;
    BACNET_CALENDAR_ENTRY entry = { 0 };
    BACNET_CALENDAR_ENTRY test_entry = { 0 };

    entry.tag = BACNET_CALENDAR_DATE;
    entry.type.Date.year = 2155;
    entry.type.Date.month = 10;
    entry.type.Date.day = 0xff;
    entry.type.Date.wday = 0xff;

    len = bacnet_calendar_entry_encode(apdu, &entry);
    zassert_true(len > 0, NULL);

    null_len = bacnet_calendar_entry_decode(apdu, len, NULL);
    zassert_equal(len, null_len, NULL);
    test_len = bacnet_calendar_entry_decode(apdu, len, &test_entry);
    zassert_equal(len, test_len, NULL);
    zassert_equal(entry.tag, test_entry.tag, NULL);
    zassert_equal(entry.type.Date.year, test_entry.type.Date.year, NULL);
    zassert_equal(entry.type.Date.month, test_entry.type.Date.month, NULL);
    zassert_equal(entry.type.Date.day, test_entry.type.Date.day, NULL);
    zassert_equal(entry.type.Date.wday, test_entry.type.Date.wday, NULL);

    /* zero length APDU is an empty list */
    zassert_equal(bacnet_calendar_entry_decode(apdu, 0, &test_entry), 0, NULL);
    /* NULL apdu is rejected */
    zassert_equal(
        bacnet_calendar_entry_decode(NULL, len, &test_entry),
        BACNET_STATUS_REJECT, NULL);
}

/**
 * @brief Test encode/decode API for the DateRange choice, including a NULL
 *  destination which shall return the same length as a non-NULL decode.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetCalendarEntry_tests, test_BACnetCalendarEntry_DateRange)
#else
static void test_BACnetCalendarEntry_DateRange(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len, null_len, test_len;
    BACNET_CALENDAR_ENTRY entry = { 0 };
    BACNET_CALENDAR_ENTRY test_entry = { 0 };

    entry.tag = BACNET_CALENDAR_DATE_RANGE;
    entry.type.DateRange.startdate.year = 2155;
    entry.type.DateRange.startdate.month = 12;
    entry.type.DateRange.startdate.day = 1;
    entry.type.DateRange.startdate.wday = 0xff;
    entry.type.DateRange.enddate.year = 2155;
    entry.type.DateRange.enddate.month = 12;
    entry.type.DateRange.enddate.day = 31;
    entry.type.DateRange.enddate.wday = 0xff;

    len = bacnet_calendar_entry_encode(apdu, &entry);
    zassert_true(len > 0, NULL);

    null_len = bacnet_calendar_entry_decode(apdu, len, NULL);
    zassert_equal(len, null_len, NULL);
    test_len = bacnet_calendar_entry_decode(apdu, len, &test_entry);
    zassert_equal(len, test_len, NULL);
    zassert_equal(entry.tag, test_entry.tag, NULL);
    zassert_equal(
        entry.type.DateRange.startdate.year,
        test_entry.type.DateRange.startdate.year, NULL);
    zassert_equal(
        entry.type.DateRange.startdate.month,
        test_entry.type.DateRange.startdate.month, NULL);
    zassert_equal(
        entry.type.DateRange.startdate.day,
        test_entry.type.DateRange.startdate.day, NULL);
    zassert_equal(
        entry.type.DateRange.enddate.year,
        test_entry.type.DateRange.enddate.year, NULL);
    zassert_equal(
        entry.type.DateRange.enddate.month,
        test_entry.type.DateRange.enddate.month, NULL);
    zassert_equal(
        entry.type.DateRange.enddate.day, test_entry.type.DateRange.enddate.day,
        NULL);
}

/**
 * @brief Test encode/decode API for the WeekNDay choice, including a NULL
 *  destination which shall return the same length as a non-NULL decode.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetCalendarEntry_tests, test_BACnetCalendarEntry_WeekNDay)
#else
static void test_BACnetCalendarEntry_WeekNDay(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len, null_len, test_len;
    BACNET_CALENDAR_ENTRY entry = { 0 };
    BACNET_CALENDAR_ENTRY test_entry = { 0 };

    entry.tag = BACNET_CALENDAR_WEEK_N_DAY;
    entry.type.WeekNDay.month = 0xff;
    entry.type.WeekNDay.weekofmonth = 0xff;
    entry.type.WeekNDay.dayofweek = 1;

    len = bacnet_calendar_entry_encode(apdu, &entry);
    zassert_true(len > 0, NULL);

    null_len = bacnet_calendar_entry_decode(apdu, len, NULL);
    zassert_equal(len, null_len, NULL);
    test_len = bacnet_calendar_entry_decode(apdu, len, &test_entry);
    zassert_equal(len, test_len, NULL);
    zassert_equal(entry.tag, test_entry.tag, NULL);
    zassert_equal(
        entry.type.WeekNDay.month, test_entry.type.WeekNDay.month, NULL);
    zassert_equal(
        entry.type.WeekNDay.weekofmonth, test_entry.type.WeekNDay.weekofmonth,
        NULL);
    zassert_equal(
        entry.type.WeekNDay.dayofweek, test_entry.type.WeekNDay.dayofweek,
        NULL);
}

/**
 * @brief Test the context tagged encode/decode API, including a NULL
 *  destination which shall return the same length as a non-NULL decode.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetCalendarEntry_tests, test_BACnetCalendarEntry_ContextDecode)
#else
static void test_BACnetCalendarEntry_ContextDecode(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t tag_number = 1;
    int len, null_len, test_len;
    BACNET_CALENDAR_ENTRY entry = { 0 };
    BACNET_CALENDAR_ENTRY test_entry = { 0 };

    entry.tag = BACNET_CALENDAR_DATE;
    entry.type.Date.year = 2024;
    entry.type.Date.month = 1;
    entry.type.Date.day = 31;
    entry.type.Date.wday = 3;

    len = bacnet_calendar_entry_context_encode(apdu, tag_number, &entry);
    zassert_true(len > 0, NULL);

    null_len =
        bacnet_calendar_entry_context_decode(apdu, len, tag_number, NULL);
    zassert_equal(len, null_len, NULL);
    test_len = bacnet_calendar_entry_context_decode(
        apdu, len, tag_number, &test_entry);
    zassert_equal(len, test_len, NULL);
    zassert_equal(entry.tag, test_entry.tag, NULL);
    zassert_equal(entry.type.Date.year, test_entry.type.Date.year, NULL);
    zassert_equal(entry.type.Date.month, test_entry.type.Date.month, NULL);
    zassert_equal(entry.type.Date.day, test_entry.type.Date.day, NULL);
    zassert_equal(entry.type.Date.wday, test_entry.type.Date.wday, NULL);

    /* incorrect tag number is rejected */
    test_len = bacnet_calendar_entry_context_decode(
        apdu, len, tag_number + 1, &test_entry);
    zassert_equal(test_len, BACNET_STATUS_REJECT, NULL);
}

/**
 * @brief Test bacapp_date_in_calendar_entry() for the Date choice, including
 *  date patterns with unspecified (wildcard) year, month, day, and
 *  day-of-week octets.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetCalendarEntry_tests, test_BACnetCalendarEntry_Date_Match)
#else
static void test_BACnetCalendarEntry_Date_Match(void)
#endif
{
    BACNET_CALENDAR_ENTRY entry = { 0 };
    BACNET_DATE date = { 0 };

    /* specific date match */
    entry.tag = BACNET_CALENDAR_DATE;
    entry.type.Date.year = 2024;
    entry.type.Date.month = 1;
    entry.type.Date.day = 31;
    entry.type.Date.wday = 3;
    datetime_set_date(&date, 2024, 1, 31);
    date.wday = 3;
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    /* different day does not match */
    datetime_set_date(&date, 2024, 1, 30);
    date.wday = 2;
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* day-of-week contradicting an otherwise fully specified date
       is a non-match per IC135-2012-6 */
    datetime_set_date(&date, 2024, 1, 31);
    date.wday = 3;
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    date.wday = 4;
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* date pattern: any year, specific month/day, e.g. 1-Jan every year */
    entry.type.Date.year = 2155;
    entry.type.Date.month = 1;
    entry.type.Date.day = 1;
    entry.type.Date.wday = 0xff;
    datetime_set_date(&date, 2000, 1, 1);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2030, 1, 1);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2030, 1, 2);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* odd(13)/even(14) month values are only meaningful for WeekNDay;
       for a Date choice, month is exact (1-12) or 'any' (0xff), so 13/14
       never match a real date */
    entry.type.Date.year = 2155;
    entry.type.Date.month = 13;
    entry.type.Date.day = 0xff;
    entry.type.Date.wday = 0xff;
    datetime_set_date(&date, 2024, 3, 15);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    entry.type.Date.month = 14;
    datetime_set_date(&date, 2024, 4, 15);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* NULL entry is rejected */
    zassert_false(bacapp_date_in_calendar_entry(&date, NULL), NULL);
}

/**
 * @brief Test bacapp_date_in_calendar_entry() for the DateRange choice.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetCalendarEntry_tests, test_BACnetCalendarEntry_DateRange_Match)
#else
static void test_BACnetCalendarEntry_DateRange_Match(void)
#endif
{
    BACNET_CALENDAR_ENTRY entry = { 0 };
    BACNET_DATE date = { 0 };

    entry.tag = BACNET_CALENDAR_DATE_RANGE;
    datetime_set_date(&entry.type.DateRange.startdate, 2024, 6, 1);
    datetime_set_date(&entry.type.DateRange.enddate, 2024, 6, 30);

    datetime_set_date(&date, 2024, 6, 1);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 6, 15);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 6, 30);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 5, 31);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 7, 1);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);
}

/**
 * @brief Test bacapp_date_in_calendar_entry() for the WeekNDay choice,
 *  including unspecified (wildcard) octets, each supported week-of-month
 *  value (1-6), and unsupported week-of-month values (7-9).
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetCalendarEntry_tests, test_BACnetCalendarEntry_WeekNDay_Match)
#else
static void test_BACnetCalendarEntry_WeekNDay_Match(void)
#endif
{
    BACNET_CALENDAR_ENTRY entry = { 0 };
    BACNET_DATE date = { 0 };

    /* any month, any week-of-month, every Monday */
    entry.tag = BACNET_CALENDAR_WEEK_N_DAY;
    entry.type.WeekNDay.month = 0xff;
    entry.type.WeekNDay.weekofmonth = 0xff;
    entry.type.WeekNDay.dayofweek = BACNET_WEEKDAY_MONDAY;
    datetime_set_date(&date, 2024, 1, 1);
    date.wday = BACNET_WEEKDAY_MONDAY;
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    date.wday = BACNET_WEEKDAY_TUESDAY;
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* week-of-month 1: days numbered 1-7 */
    entry.type.WeekNDay.weekofmonth = 1;
    entry.type.WeekNDay.dayofweek = 0xff;
    datetime_set_date(&date, 2024, 2, 7);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 2, 8);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* week-of-month 2: days numbered 8-14 */
    entry.type.WeekNDay.weekofmonth = 2;
    datetime_set_date(&date, 2024, 2, 8);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 2, 14);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 2, 15);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* week-of-month 3: days numbered 15-21 */
    entry.type.WeekNDay.weekofmonth = 3;
    datetime_set_date(&date, 2024, 2, 15);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 2, 21);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 2, 22);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* week-of-month 4: days numbered 22-28 */
    entry.type.WeekNDay.weekofmonth = 4;
    datetime_set_date(&date, 2024, 2, 22);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 2, 28);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 2, 29);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* week-of-month 5: days numbered 29-31 (January 2024 has 31 days) */
    entry.type.WeekNDay.weekofmonth = 5;
    datetime_set_date(&date, 2024, 1, 29);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 1, 31);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 1, 28);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* week-of-month 6: last 7 days of the month (Feb 2024 has 29 days) */
    entry.type.WeekNDay.weekofmonth = 6;
    datetime_set_date(&date, 2024, 2, 23);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 2, 29);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 2, 22);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* week-of-month 7-9: unsupported values never match, even a date
       that would otherwise fall within days 1-7 */
    datetime_set_date(&date, 2024, 2, 1);
    entry.type.WeekNDay.weekofmonth = 7;
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    entry.type.WeekNDay.weekofmonth = 8;
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    entry.type.WeekNDay.weekofmonth = 9;
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* month match: specific month */
    entry.type.WeekNDay.month = 2;
    entry.type.WeekNDay.weekofmonth = 0xff;
    datetime_set_date(&date, 2024, 2, 1);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 3, 1);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* month match: odd(13) months */
    entry.type.WeekNDay.month = 13;
    datetime_set_date(&date, 2024, 3, 15);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 4, 15);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);

    /* month match: even(14) months */
    entry.type.WeekNDay.month = 14;
    datetime_set_date(&date, 2024, 4, 15);
    zassert_true(bacapp_date_in_calendar_entry(&date, &entry), NULL);
    datetime_set_date(&date, 2024, 3, 15);
    zassert_false(bacapp_date_in_calendar_entry(&date, &entry), NULL);
}

/**
 * @brief Test bacnet_calendar_entry_same() for all choices.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetCalendarEntry_tests, test_BACnetCalendarEntry_Same)
#else
static void test_BACnetCalendarEntry_Same(void)
#endif
{
    BACNET_CALENDAR_ENTRY value1 = { 0 };
    BACNET_CALENDAR_ENTRY value2 = { 0 };

    value1.tag = BACNET_CALENDAR_DATE_RANGE;
    datetime_set_date(&value1.type.DateRange.startdate, 2024, 6, 1);
    datetime_set_date(&value1.type.DateRange.enddate, 2024, 6, 30);
    value2.tag = BACNET_CALENDAR_DATE_RANGE;
    datetime_set_date(&value2.type.DateRange.startdate, 2024, 6, 1);
    datetime_set_date(&value2.type.DateRange.enddate, 2024, 6, 30);
    zassert_true(bacnet_calendar_entry_same(&value1, &value2), NULL);

    /* different end dates are not the same */
    datetime_set_date(&value2.type.DateRange.enddate, 2024, 7, 1);
    zassert_false(bacnet_calendar_entry_same(&value1, &value2), NULL);

    /* NULL values are rejected */
    zassert_false(bacnet_calendar_entry_same(NULL, &value2), NULL);
    zassert_false(bacnet_calendar_entry_same(&value1, NULL), NULL);

    /* mismatched tags are not the same */
    value2.tag = BACNET_CALENDAR_DATE;
    zassert_false(bacnet_calendar_entry_same(&value1, &value2), NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnetCalendarEntry_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        BACnetCalendarEntry_tests,
        ztest_unit_test(test_BACnetCalendarEntry_Date),
        ztest_unit_test(test_BACnetCalendarEntry_DateRange),
        ztest_unit_test(test_BACnetCalendarEntry_WeekNDay),
        ztest_unit_test(test_BACnetCalendarEntry_ContextDecode),
        ztest_unit_test(test_BACnetCalendarEntry_Date_Match),
        ztest_unit_test(test_BACnetCalendarEntry_DateRange_Match),
        ztest_unit_test(test_BACnetCalendarEntry_WeekNDay_Match),
        ztest_unit_test(test_BACnetCalendarEntry_Same));

    ztest_run_test_suite(BACnetCalendarEntry_tests);
}
#endif
