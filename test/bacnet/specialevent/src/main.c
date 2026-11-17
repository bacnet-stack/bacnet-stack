/**
 * @file
 * @brief Unit test for BACnetSpecialEvent and its linked-list variant
 *  BACnetSpecialEventEntry. This test also indirectly tests
 *  BACnetCalendarEntry, BACnetDailySchedule, and BACnetTimeValue.
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @date Aug 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/ztest.h>
#include "bacnet/bactimevalue.h"
#include "bacnet/special_event.h"
#include "bacnet/calendar_entry.h"
#include "bacnet/datetime.h"
#include "bacnet/bacapp.h"

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetSpecialEvent_tests, test_BACnetSpecialEvent_CalendarRef)
#else
static void test_BACnetSpecialEvent_CalendarRef(void)
#endif
{
    int len, apdu_len, null_len;
    uint8_t apdu[MAX_APDU] = { 0 };

    BACNET_SPECIAL_EVENT in = { 0 };
    BACNET_SPECIAL_EVENT out = { 0 };

    in.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE;
    in.period.calendarReference.instance = 5;
    in.period.calendarReference.type = OBJECT_CALENDAR;
    in.timeValues.TV_Count = 2;
    in.timeValues.Time_Values[0].Time.hour = 12;
    in.timeValues.Time_Values[0].Time.min = 30;
    in.timeValues.Time_Values[0].Time.sec = 15;
    in.timeValues.Time_Values[0].Time.hundredths = 5;
    in.timeValues.Time_Values[0].Value.tag =
        BACNET_APPLICATION_TAG_UNSIGNED_INT;
    in.timeValues.Time_Values[0].Value.type.Unsigned_Int = 15;
    in.timeValues.Time_Values[1].Time.hour = 16;
    in.timeValues.Time_Values[1].Time.min = 1;
    in.timeValues.Time_Values[1].Time.sec = 2;
    in.timeValues.Time_Values[1].Time.hundredths = 3;
    in.timeValues.Time_Values[1].Value.tag =
        BACNET_APPLICATION_TAG_UNSIGNED_INT;
    in.timeValues.Time_Values[1].Value.type.Unsigned_Int = 0;
    in.priority = 5;

    len = bacnet_special_event_encode(apdu, &in);
    null_len = bacnet_special_event_encode(NULL, &in);
    zassert_equal(len, null_len, NULL);

    null_len = bacnet_special_event_decode(apdu, len, NULL);
    zassert_equal(len, null_len, NULL);
    apdu_len = bacnet_special_event_decode(apdu, len, &out);
    zassert_equal(len, apdu_len, NULL);

    zassert_equal(in.periodTag, out.periodTag, NULL);
    zassert_equal(
        in.period.calendarReference.instance,
        out.period.calendarReference.instance, NULL);
    zassert_equal(
        in.period.calendarReference.type, out.period.calendarReference.type,
        NULL);
    zassert_equal(in.timeValues.TV_Count, out.timeValues.TV_Count, NULL);

    zassert_equal(
        in.timeValues.Time_Values[0].Time.hour,
        out.timeValues.Time_Values[0].Time.hour, NULL);
    zassert_equal(
        in.timeValues.Time_Values[0].Time.min,
        out.timeValues.Time_Values[0].Time.min, NULL);
    zassert_equal(
        in.timeValues.Time_Values[0].Time.sec,
        out.timeValues.Time_Values[0].Time.sec, NULL);
    zassert_equal(
        in.timeValues.Time_Values[0].Time.hundredths,
        out.timeValues.Time_Values[0].Time.hundredths, NULL);

    zassert_equal(
        in.timeValues.Time_Values[0].Value.tag,
        out.timeValues.Time_Values[0].Value.tag, NULL);
    zassert_equal(
        in.timeValues.Time_Values[0].Value.type.Unsigned_Int,
        out.timeValues.Time_Values[0].Value.type.Unsigned_Int, NULL);

    zassert_equal(
        in.timeValues.Time_Values[1].Time.hour,
        out.timeValues.Time_Values[1].Time.hour, NULL);
    zassert_equal(
        in.timeValues.Time_Values[1].Time.min,
        out.timeValues.Time_Values[1].Time.min, NULL);
    zassert_equal(
        in.timeValues.Time_Values[1].Time.sec,
        out.timeValues.Time_Values[1].Time.sec, NULL);
    zassert_equal(
        in.timeValues.Time_Values[1].Time.hundredths,
        out.timeValues.Time_Values[1].Time.hundredths, NULL);

    zassert_equal(
        in.timeValues.Time_Values[1].Value.tag,
        out.timeValues.Time_Values[1].Value.tag, NULL);
    zassert_equal(
        in.timeValues.Time_Values[1].Value.type.Unsigned_Int,
        out.timeValues.Time_Values[1].Value.type.Unsigned_Int, NULL);

    zassert_equal(in.priority, out.priority, NULL);
}

/**
 * @brief Test encode/decode API with Calendar Entry (Date variant)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetSpecialEvent_tests, test_BACnetSpecialEvent_Date)
#else
static void test_BACnetSpecialEvent_Date(void)
#endif
{
    int len, apdu_len, null_len;
    uint8_t apdu[MAX_APDU] = { 0 };

    BACNET_SPECIAL_EVENT in = { 0 };
    BACNET_SPECIAL_EVENT out = { 0 };

    in.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
    in.period.calendarEntry.tag = BACNET_CALENDAR_DATE;
    in.period.calendarEntry.type.Date.year = 2155;
    in.period.calendarEntry.type.Date.month = 10;
    in.period.calendarEntry.type.Date.day = 0xff;
    in.period.calendarEntry.type.Date.wday = 0xff;
    in.timeValues.TV_Count = 0;
    in.priority = 16;

    len = bacnet_special_event_encode(apdu, &in);
    null_len = bacnet_special_event_encode(NULL, &in);
    zassert_equal(len, null_len, NULL);

    null_len = bacnet_special_event_decode(apdu, len, NULL);
    zassert_equal(len, null_len, NULL);
    apdu_len = bacnet_special_event_decode(apdu, len, &out);
    zassert_equal(len, apdu_len, NULL);
    zassert_equal(in.periodTag, out.periodTag, NULL);
    zassert_equal(
        in.period.calendarEntry.tag, out.period.calendarEntry.tag, NULL);
    zassert_equal(
        in.period.calendarEntry.type.Date.wday,
        out.period.calendarEntry.type.Date.wday, NULL);
    zassert_equal(
        in.period.calendarEntry.type.Date.year,
        out.period.calendarEntry.type.Date.year, NULL);
    zassert_equal(
        in.period.calendarEntry.type.Date.month,
        out.period.calendarEntry.type.Date.month, NULL);
    zassert_equal(
        in.period.calendarEntry.type.Date.day,
        out.period.calendarEntry.type.Date.day, NULL);

    zassert_equal(in.timeValues.TV_Count, out.timeValues.TV_Count, NULL);
    zassert_equal(in.priority, out.priority, NULL);
}

/**
 * @brief Test encode/decode API with Calendar Entry (DateRange variant)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetSpecialEvent_tests, test_BACnetSpecialEvent_DateRange)
#else
static void test_BACnetSpecialEvent_DateRange(void)
#endif
{
    int len, apdu_len, null_len;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_SPECIAL_EVENT in = { 0 };
    BACNET_SPECIAL_EVENT out = { 0 };

    in.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
    in.period.calendarEntry.tag = BACNET_CALENDAR_DATE_RANGE;
    in.period.calendarEntry.type.DateRange.startdate.year = 2155;
    in.period.calendarEntry.type.DateRange.startdate.month = 12;
    in.period.calendarEntry.type.DateRange.startdate.day = 1;
    in.period.calendarEntry.type.DateRange.startdate.wday = 0xff;
    in.period.calendarEntry.type.DateRange.enddate.year = 2155;
    in.period.calendarEntry.type.DateRange.enddate.month = 12;
    in.period.calendarEntry.type.DateRange.enddate.day = 31;
    in.period.calendarEntry.type.DateRange.enddate.wday = 0xff;
    in.timeValues.TV_Count = 0;
    in.priority = 1;

    len = bacnet_special_event_encode(apdu, &in);
    null_len = bacnet_special_event_encode(NULL, &in);
    zassert_equal(len, null_len, NULL);

    null_len = bacnet_special_event_decode(apdu, len, NULL);
    zassert_equal(len, null_len, NULL);
    apdu_len = bacnet_special_event_decode(apdu, len, &out);
    zassert_equal(len, apdu_len, "apdu_len %d != len %d", apdu_len, len);
    zassert_equal(in.periodTag, out.periodTag, NULL);

    zassert_equal(
        in.period.calendarEntry.tag, out.period.calendarEntry.tag, NULL);

    zassert_equal(
        in.period.calendarEntry.type.DateRange.startdate.wday,
        out.period.calendarEntry.type.DateRange.startdate.wday, NULL);
    zassert_equal(
        in.period.calendarEntry.type.DateRange.startdate.year,
        out.period.calendarEntry.type.DateRange.startdate.year, NULL);
    zassert_equal(
        in.period.calendarEntry.type.DateRange.startdate.month,
        out.period.calendarEntry.type.DateRange.startdate.month, NULL);
    zassert_equal(
        in.period.calendarEntry.type.DateRange.startdate.day,
        out.period.calendarEntry.type.DateRange.startdate.day, NULL);

    zassert_equal(
        in.period.calendarEntry.type.DateRange.enddate.wday,
        out.period.calendarEntry.type.DateRange.enddate.wday, NULL);
    zassert_equal(
        in.period.calendarEntry.type.DateRange.enddate.year,
        out.period.calendarEntry.type.DateRange.enddate.year, NULL);
    zassert_equal(
        in.period.calendarEntry.type.DateRange.enddate.month,
        out.period.calendarEntry.type.DateRange.enddate.month, NULL);
    zassert_equal(
        in.period.calendarEntry.type.DateRange.enddate.day,
        out.period.calendarEntry.type.DateRange.enddate.day, NULL);

    zassert_equal(in.timeValues.TV_Count, out.timeValues.TV_Count, NULL);
    zassert_equal(in.priority, out.priority, NULL);
}

/**
 * @brief Test encode/decode API with Calendar Entry (Date variant)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetSpecialEvent_tests, test_BACnetSpecialEvent_WeekNDate)
#else
static void test_BACnetSpecialEvent_WeekNDate(void)
#endif
{
    int len, apdu_len, null_len;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_SPECIAL_EVENT in = { 0 };
    BACNET_SPECIAL_EVENT out = { 0 };

    in.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
    in.period.calendarEntry.tag = BACNET_CALENDAR_WEEK_N_DAY;
    in.period.calendarEntry.type.WeekNDay.month = 0xff;
    in.period.calendarEntry.type.WeekNDay.dayofweek = 1; /* mondays */
    in.period.calendarEntry.type.WeekNDay.weekofmonth = 0xff;
    in.timeValues.TV_Count = 0;
    in.priority = 16;

    len = bacnet_special_event_encode(apdu, &in);
    null_len = bacnet_special_event_encode(NULL, &in);
    zassert_equal(len, null_len, NULL);

    null_len = bacnet_special_event_decode(apdu, len, NULL);
    zassert_equal(len, null_len, NULL);
    apdu_len = bacnet_special_event_decode(apdu, len, &out);
    zassert_equal(len, apdu_len, NULL);
    zassert_equal(in.periodTag, out.periodTag, NULL);

    zassert_equal(
        in.period.calendarEntry.tag, out.period.calendarEntry.tag, NULL);
    zassert_equal(
        in.period.calendarEntry.type.WeekNDay.month,
        out.period.calendarEntry.type.WeekNDay.month, NULL);
    zassert_equal(
        in.period.calendarEntry.type.WeekNDay.dayofweek,
        out.period.calendarEntry.type.WeekNDay.dayofweek, NULL);
    zassert_equal(
        in.period.calendarEntry.type.WeekNDay.weekofmonth,
        out.period.calendarEntry.type.WeekNDay.weekofmonth, NULL);

    zassert_equal(in.timeValues.TV_Count, out.timeValues.TV_Count, NULL);
    zassert_equal(in.priority, out.priority, NULL);
}

/** @brief try decoding a real sample from Siemens, captured with wireshark */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetSpecialEvent_tests, test_BACnetSpecialEvent_DecodeRealAPDU)
#else
static void test_BACnetSpecialEvent_DecodeRealAPDU(void)
#endif
{
    int apdu_len, null_len;
    BACNET_SPECIAL_EVENT out = { 0 };
    uint8_t sample[18] = {
        0x0e, 0x0c, 0xff, 0x0a, 0x1c, 0xff, 0x0f, 0x2e, 0xb4,
        0x00, 0x00, 0x00, 0x00, 0x91, 0x00, 0x2f, 0x39, 0x10
    };

    null_len = bacnet_special_event_decode(sample, sizeof(sample), NULL);
    zassert_equal(sizeof(sample), null_len, NULL);
    apdu_len = bacnet_special_event_decode(sample, sizeof(sample), &out);
    zassert_equal(sizeof(sample), apdu_len, NULL);

    zassert_equal(
        out.periodTag, BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY, NULL);
    zassert_equal(out.period.calendarEntry.tag, BACNET_CALENDAR_DATE, NULL);
    zassert_equal(out.period.calendarEntry.type.Date.day, 28, NULL);
    zassert_equal(out.period.calendarEntry.type.Date.month, 10, NULL);
    zassert_equal(
        out.period.calendarEntry.type.Date.year, 2155 /* any */, NULL);
    zassert_equal(
        out.period.calendarEntry.type.Date.wday, 0xff /* any */, NULL);

    zassert_equal(out.timeValues.TV_Count, 1, NULL);

    zassert_equal(out.timeValues.Time_Values[0].Time.hour, 0, NULL);
    zassert_equal(out.timeValues.Time_Values[0].Time.min, 0, NULL);
    zassert_equal(out.timeValues.Time_Values[0].Time.sec, 0, NULL);
    zassert_equal(out.timeValues.Time_Values[0].Time.hundredths, 0, NULL);

    zassert_equal(
        out.timeValues.Time_Values[0].Value.tag,
        BACNET_APPLICATION_TAG_ENUMERATED, NULL);
    zassert_equal(
        out.timeValues.Time_Values[0].Value.type.Unsigned_Int, 0, NULL);

    zassert_equal(out.priority, 16, NULL);
}

/**
 * @brief Test the context tagged, same, and copy API of BACnetSpecialEvent
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetSpecialEvent_tests, test_BACnetSpecialEvent_ContextSameCopy)
#else
static void test_BACnetSpecialEvent_ContextSameCopy(void)
#endif
{
    int len, apdu_len, null_len;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_SPECIAL_EVENT in = { 0 };
    BACNET_SPECIAL_EVENT out = { 0 };
    BACNET_SPECIAL_EVENT copy = { 0 };
    const uint8_t tag_number = 4;

    in.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE;
    in.period.calendarReference.type = OBJECT_CALENDAR;
    in.period.calendarReference.instance = 7;
    in.timeValues.TV_Count = 1;
    in.timeValues.Time_Values[0].Time.hour = 8;
    in.timeValues.Time_Values[0].Value.tag =
        BACNET_APPLICATION_TAG_UNSIGNED_INT;
    in.timeValues.Time_Values[0].Value.type.Unsigned_Int = 42;
    in.priority = 8;

    len = bacnet_special_event_context_encode(apdu, tag_number, &in);
    null_len = bacnet_special_event_context_encode(NULL, tag_number, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(len, null_len, NULL);
    zassert_equal(
        bacnet_special_event_context_encode(apdu, tag_number, NULL), 0, NULL);

    apdu_len = bacnet_special_event_context_decode(apdu, len, tag_number, &out);
    zassert_equal(apdu_len, len, "apdu_len=%d len=%d", apdu_len, len);
    zassert_true(bacnet_special_event_same(&in, &out), NULL);

    /* negative testing */
    apdu_len =
        bacnet_special_event_context_decode(apdu, len, tag_number + 1, &out);
    zassert_equal(apdu_len, BACNET_STATUS_ERROR, NULL);
    apdu_len =
        bacnet_special_event_context_decode(apdu, len - 1, tag_number, &out);
    zassert_equal(apdu_len, BACNET_STATUS_ERROR, NULL);

    /* same */
    zassert_true(bacnet_special_event_same(&in, &in), NULL);
    out = in;
    out.priority = 9;
    zassert_false(bacnet_special_event_same(&in, &out), NULL);
    out = in;
    out.period.calendarReference.instance++;
    zassert_false(bacnet_special_event_same(&in, &out), NULL);
    out = in;
    out.timeValues.Time_Values[0].Value.type.Unsigned_Int++;
    zassert_false(bacnet_special_event_same(&in, &out), NULL);
    out = in;
    out.timeValues.TV_Count = 0;
    zassert_false(bacnet_special_event_same(&in, &out), NULL);
    out = in;
    out.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
    zassert_false(bacnet_special_event_same(&in, &out), NULL);

    /* copy */
    zassert_true(bacnet_special_event_copy(&copy, &in), NULL);
    zassert_true(bacnet_special_event_same(&in, &copy), NULL);
    zassert_false(bacnet_special_event_copy(NULL, &in), NULL);
    zassert_false(bacnet_special_event_copy(&copy, NULL), NULL);
}

#define STORE_MAX 4
typedef struct {
    BACNET_TIME_VALUE entries[STORE_MAX];
    unsigned count;
    unsigned limit;
} STORE_CONTEXT;

static bool store_entry(const BACNET_TIME_VALUE *time_value, void *ctx)
{
    STORE_CONTEXT *store = ctx;

    if (!time_value || !store || (store->count >= store->limit)) {
        return false;
    }
    store->entries[store->count] = *time_value;
    store->count++;

    return true;
}

/**
 * @brief Test the linked-list BACnetSpecialEvent entry encode/decode API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetSpecialEvent_tests, test_BACnetSpecialEventEntry)
#else
static void test_BACnetSpecialEventEntry(void)
#endif
{
    int len, apdu_len, null_len, test_len;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_DAILY_SCHEDULE_ENTRY tv[2] = { 0 };
    BACNET_SPECIAL_EVENT_ENTRY in = { 0 };
    BACNET_SPECIAL_EVENT_ENTRY out = { 0 };
    BACNET_SPECIAL_EVENT_ENTRY other = { 0 };
    STORE_CONTEXT store = { 0 };
    const uint8_t tag_number = 4;
    unsigned i;

    tv[0].Time_Value.Time.hour = 12;
    tv[0].Time_Value.Time.min = 30;
    tv[0].Time_Value.Value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    tv[0].Time_Value.Value.type.Unsigned_Int = 15;
    tv[0].next = &tv[1];
    tv[1].Time_Value.Time.hour = 16;
    tv[1].Time_Value.Time.min = 1;
    tv[1].Time_Value.Value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    tv[1].Time_Value.Value.type.Unsigned_Int = 0;

    /* calendar reference period */
    in.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE;
    in.period.calendarReference.type = OBJECT_CALENDAR;
    in.period.calendarReference.instance = 5;
    in.timeValues = &tv[0];
    in.priority = 5;
    len = bacnet_special_event_entry_encode(apdu, &in);
    null_len = bacnet_special_event_entry_encode(NULL, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(len, null_len, NULL);

    /* NULL value only measures the length */
    test_len = bacnet_special_event_entry_decode(apdu, len, NULL, NULL, NULL);
    zassert_equal(test_len, len, NULL);
    out.timeValues = &tv[1];
    store.limit = STORE_MAX;
    apdu_len =
        bacnet_special_event_entry_decode(apdu, len, &out, store_entry, &store);
    zassert_equal(apdu_len, len, NULL);
    zassert_equal(out.periodTag, in.periodTag, NULL);
    zassert_equal(out.period.calendarReference.type, OBJECT_CALENDAR, NULL);
    zassert_equal(out.period.calendarReference.instance, 5, NULL);
    zassert_equal(out.priority, 5, NULL);
    zassert_equal(store.count, 2, NULL);
    for (i = 0; i < store.count; i++) {
        zassert_true(
            bacnet_time_value_same(&tv[i].Time_Value, &store.entries[i]), NULL);
    }
    /* the caller owns the list, so decode leaves it empty */
    zassert_equal(out.timeValues, NULL, NULL);

    /* calendar entry period with an empty list */
    memset(&store, 0, sizeof(store));
    store.limit = STORE_MAX;
    memset(&in, 0, sizeof(in));
    in.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
    in.period.calendarEntry.tag = BACNET_CALENDAR_WEEK_N_DAY;
    in.period.calendarEntry.type.WeekNDay.month = 0xff;
    in.period.calendarEntry.type.WeekNDay.dayofweek = 1;
    in.period.calendarEntry.type.WeekNDay.weekofmonth = 0xff;
    in.priority = 16;
    len = bacnet_special_event_entry_encode(apdu, &in);
    null_len = bacnet_special_event_entry_encode(NULL, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(len, null_len, NULL);
    memset(&out, 0, sizeof(out));
    apdu_len =
        bacnet_special_event_entry_decode(apdu, len, &out, store_entry, &store);
    zassert_equal(apdu_len, len, NULL);
    zassert_equal(store.count, 0, NULL);
    zassert_true(bacnet_special_event_entry_same(&in, &out), NULL);

    /* context tagged */
    len = bacnet_special_event_entry_context_encode(apdu, tag_number, &in);
    null_len = bacnet_special_event_entry_context_encode(NULL, tag_number, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(len, null_len, NULL);
    zassert_equal(
        bacnet_special_event_entry_context_encode(apdu, tag_number, NULL),
        BACNET_STATUS_ERROR, NULL);
    memset(&out, 0, sizeof(out));
    apdu_len = bacnet_special_event_entry_context_decode(
        apdu, len, tag_number, &out, store_entry, &store);
    zassert_equal(apdu_len, len, NULL);
    zassert_true(bacnet_special_event_entry_same(&in, &out), NULL);
    apdu_len = bacnet_special_event_entry_context_decode(
        apdu, len, tag_number, NULL, NULL, NULL);
    zassert_equal(apdu_len, len, NULL);

    /* negative testing - encode */
    zassert_equal(
        bacnet_special_event_entry_encode(apdu, NULL), BACNET_STATUS_ERROR,
        NULL);

    /* negative testing - context decode */
    apdu_len = bacnet_special_event_entry_context_decode(
        apdu, len, tag_number + 1, &out, store_entry, &store);
    zassert_equal(apdu_len, BACNET_STATUS_ERROR, NULL);
    apdu_len = bacnet_special_event_entry_context_decode(
        apdu, len - 1, tag_number, &out, store_entry, &store);
    zassert_equal(apdu_len, BACNET_STATUS_ERROR, NULL);

    /* negative testing - decode */
    zassert_equal(
        bacnet_special_event_entry_decode(NULL, len, &out, store_entry, &store),
        BACNET_STATUS_ERROR, NULL);
    zassert_equal(
        bacnet_special_event_entry_decode(apdu, 0, &out, store_entry, &store),
        BACNET_STATUS_ERROR, NULL);
    /* every truncation of a valid encoding is malformed */
    len = bacnet_special_event_entry_encode(apdu, &in);
    for (test_len = 1; test_len < len; test_len++) {
        apdu_len = bacnet_special_event_entry_decode(
            apdu, test_len, &out, store_entry, &store);
        zassert_true(apdu_len < 0, "test_len=%d", test_len);
    }
    /* a tag that is not a period returns zero */
    apdu[0] = 0x2C; /* context tag 2, length 4 */
    zassert_equal(
        bacnet_special_event_entry_decode(apdu, len, &out, NULL, NULL), 0,
        NULL);
    /* a calendar reference must name a Calendar object */
    other = in;
    other.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE;
    other.period.calendarReference.type = OBJECT_CHARACTERSTRING_VALUE;
    other.period.calendarReference.instance = 1;
    len = bacnet_special_event_entry_encode(apdu, &other);
    memset(&out, 0, sizeof(out));
    zassert_equal(
        bacnet_special_event_entry_decode(apdu, len, &out, NULL, NULL),
        BACNET_STATUS_ERROR, NULL);
    zassert_equal(out.periodTag, 0, NULL);
    len = bacnet_special_event_entry_encode(apdu, &in);
    /* priority out of range */
    in.priority = 0;
    len = bacnet_special_event_entry_encode(apdu, &in);
    zassert_equal(
        bacnet_special_event_entry_decode(apdu, len, &out, NULL, NULL),
        BACNET_STATUS_ERANGE, NULL);
    in.priority = BACNET_MAX_PRIORITY + 1;
    len = bacnet_special_event_entry_encode(apdu, &in);
    zassert_equal(
        bacnet_special_event_entry_decode(apdu, len, &out, NULL, NULL),
        BACNET_STATUS_ERANGE, NULL);
    /* context decode passes the range error through */
    len = bacnet_special_event_entry_context_encode(apdu, tag_number, &in);
    zassert_equal(
        bacnet_special_event_entry_context_decode(
            apdu, len, tag_number, &out, NULL, NULL),
        BACNET_STATUS_ERANGE, NULL);
    /* store_fn aborts the decode */
    memset(&in, 0, sizeof(in));
    in.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE;
    in.period.calendarReference.type = OBJECT_CALENDAR;
    in.timeValues = &tv[0];
    in.priority = 1;
    len = bacnet_special_event_entry_encode(apdu, &in);
    memset(&store, 0, sizeof(store));
    store.limit = 1;
    zassert_equal(
        bacnet_special_event_entry_decode(apdu, len, &out, store_entry, &store),
        BACNET_STATUS_ERROR, NULL);

    /* same */
    memset(&other, 0, sizeof(other));
    zassert_false(bacnet_special_event_entry_same(NULL, &in), NULL);
    zassert_false(bacnet_special_event_entry_same(&in, NULL), NULL);
    zassert_true(bacnet_special_event_entry_same(&in, &in), NULL);
    other = in;
    other.priority = 2;
    zassert_false(bacnet_special_event_entry_same(&in, &other), NULL);
    other = in;
    other.period.calendarReference.instance++;
    zassert_false(bacnet_special_event_entry_same(&in, &other), NULL);
    other = in;
    other.period.calendarReference.type = OBJECT_SCHEDULE;
    zassert_false(bacnet_special_event_entry_same(&in, &other), NULL);
    other = in;
    other.timeValues = &tv[1];
    zassert_false(bacnet_special_event_entry_same(&in, &other), NULL);
    other = in;
    other.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
    zassert_false(bacnet_special_event_entry_same(&in, &other), NULL);
    in.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
    in.period.calendarEntry.tag = BACNET_CALENDAR_DATE;
    in.period.calendarEntry.type.Date.year = 2155;
    in.period.calendarEntry.type.Date.month = 10;
    in.period.calendarEntry.type.Date.day = 28;
    in.period.calendarEntry.type.Date.wday = 0xff;
    other = in;
    zassert_true(bacnet_special_event_entry_same(&in, &other), NULL);
    other.period.calendarEntry.type.Date.day = 29;
    zassert_false(bacnet_special_event_entry_same(&in, &other), NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnetSpecialEvent_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        BACnetSpecialEvent_tests,
        ztest_unit_test(test_BACnetSpecialEvent_CalendarRef),
        ztest_unit_test(test_BACnetSpecialEvent_Date),
        ztest_unit_test(test_BACnetSpecialEvent_DateRange),
        ztest_unit_test(test_BACnetSpecialEvent_WeekNDate),
        ztest_unit_test(test_BACnetSpecialEvent_DecodeRealAPDU),
        ztest_unit_test(test_BACnetSpecialEvent_ContextSameCopy),
        ztest_unit_test(test_BACnetSpecialEventEntry));

    ztest_run_test_suite(BACnetSpecialEvent_tests);
}
#endif
