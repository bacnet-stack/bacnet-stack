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
        ztest_unit_test(test_BACnetCalendarEntry_ContextDecode));

    ztest_run_test_suite(BACnetCalendarEntry_tests);
}
#endif
