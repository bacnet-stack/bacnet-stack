/**
 * @file
 * @brief Unit test for BACnetSpecialEvent. This test also indirectly tests
 *  BACnetCalendarEntry
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @date Aug 2023
 *
 * SPDX-License-Identifier: MIT
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

    BACNET_SPECIAL_EVENT in = {
        .periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE,
        .period = {
            .calendarReference = {
                .instance = 5,
                .type = OBJECT_CALENDAR,
            }
        },
        .timeValues = {
            .TV_Count = 2,
            .Time_Values = {
                {
                    .Time = {
                        .hour = 12,
                        .min = 30,
                        .sec = 15,
                        .hundredths = 5
                    },
                    .Value = {
                        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
                        .type = {
                            .Unsigned_Int = 15,
                        },
                    }
                },
                {
                    .Time = {
                        .hour = 16,
                        .min = 1,
                        .sec = 2,
                        .hundredths = 3
                    },
                    .Value = {
                        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
                        .type = {
                            .Unsigned_Int = 0,
                        },
                    }
                },
            }
        },
        .priority = 5,
    };
    BACNET_SPECIAL_EVENT out = { 0 };

    len = bacnet_special_event_encode(apdu, &in);
    null_len = bacnet_special_event_encode(NULL, &in);
    zassert_equal(len, null_len, NULL);

    apdu_len = bacnet_special_event_decode(apdu, len, &out);
    zassert_equal(len, apdu_len, NULL);

    zassert_equal(in.periodTag, out.periodTag, NULL);
    zassert_equal(in.period.calendarReference.instance,
        out.period.calendarReference.instance, NULL);
    zassert_equal(in.period.calendarReference.type,
        out.period.calendarReference.type, NULL);
    zassert_equal(in.timeValues.TV_Count, out.timeValues.TV_Count, NULL);

    zassert_equal(in.timeValues.Time_Values[0].Time.hour,
        out.timeValues.Time_Values[0].Time.hour, NULL);
    zassert_equal(in.timeValues.Time_Values[0].Time.min,
        out.timeValues.Time_Values[0].Time.min, NULL);
    zassert_equal(in.timeValues.Time_Values[0].Time.sec,
        out.timeValues.Time_Values[0].Time.sec, NULL);
    zassert_equal(in.timeValues.Time_Values[0].Time.hundredths,
        out.timeValues.Time_Values[0].Time.hundredths, NULL);

    zassert_equal(in.timeValues.Time_Values[0].Value.tag,
        out.timeValues.Time_Values[0].Value.tag, NULL);
    zassert_equal(in.timeValues.Time_Values[0].Value.type.Unsigned_Int,
        out.timeValues.Time_Values[0].Value.type.Unsigned_Int, NULL);

    zassert_equal(in.timeValues.Time_Values[1].Time.hour,
        out.timeValues.Time_Values[1].Time.hour, NULL);
    zassert_equal(in.timeValues.Time_Values[1].Time.min,
        out.timeValues.Time_Values[1].Time.min, NULL);
    zassert_equal(in.timeValues.Time_Values[1].Time.sec,
        out.timeValues.Time_Values[1].Time.sec, NULL);
    zassert_equal(in.timeValues.Time_Values[1].Time.hundredths,
        out.timeValues.Time_Values[1].Time.hundredths, NULL);

    zassert_equal(in.timeValues.Time_Values[1].Value.tag,
        out.timeValues.Time_Values[1].Value.tag, NULL);
    zassert_equal(in.timeValues.Time_Values[1].Value.type.Unsigned_Int,
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

    BACNET_SPECIAL_EVENT in = {
        .periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY,
        .period = {
            .calendarEntry = {
                .tag = BACNET_CALENDAR_DATE,
                .type = {
                    .Date = {
                        .year = 2155,
                        .month = 10,
                        .day = 0xff,
                        .wday = 0xff,
                    }
                }
            }
        },
        .timeValues = {
            .TV_Count = 0,
            .Time_Values = { 0 }
        },
        .priority = 16,
    };
    BACNET_SPECIAL_EVENT out = { 0 };

    len = bacnet_special_event_encode(apdu, &in);
    null_len = bacnet_special_event_encode(NULL, &in);
    zassert_equal(len, null_len, NULL);

    apdu_len = bacnet_special_event_decode(apdu, len, &out);
    zassert_equal(len, apdu_len, NULL);
    zassert_equal(in.periodTag, out.periodTag, NULL);
    zassert_equal(
        in.period.calendarEntry.tag, out.period.calendarEntry.tag, NULL);
    zassert_equal(in.period.calendarEntry.type.Date.wday,
        out.period.calendarEntry.type.Date.wday, NULL);
    zassert_equal(in.period.calendarEntry.type.Date.year,
        out.period.calendarEntry.type.Date.year, NULL);
    zassert_equal(in.period.calendarEntry.type.Date.month,
        out.period.calendarEntry.type.Date.month, NULL);
    zassert_equal(in.period.calendarEntry.type.Date.day,
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

    BACNET_SPECIAL_EVENT in = {
        .periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY,
        .period = {
            .calendarEntry = {
                .tag = BACNET_CALENDAR_DATE_RANGE,
                .type = {
                    .DateRange = {
                        .startdate = {
                            .day = 1,
                            .month = 12,
                            .year = 2155,
                            .wday = 0xff,
                        },
                        .enddate = {
                            .day = 31,
                            .month = 12,
                            .year = 2155,
                            .wday = 0xff,
                        },
                    }
                }
            }
        },
        .timeValues = {
            .TV_Count = 0,
            .Time_Values = { 0 }
        },
        .priority = 0,
    };
    BACNET_SPECIAL_EVENT out = { 0 };

    len = bacnet_special_event_encode(apdu, &in);
    null_len = bacnet_special_event_encode(NULL, &in);
    zassert_equal(len, null_len, NULL);

    apdu_len = bacnet_special_event_decode(apdu, len, &out);
    zassert_equal(len, apdu_len, "apdu_len %d != len %d", apdu_len, len);
    zassert_equal(in.periodTag, out.periodTag, NULL);

    zassert_equal(
        in.period.calendarEntry.tag, out.period.calendarEntry.tag, NULL);

    zassert_equal(in.period.calendarEntry.type.DateRange.startdate.wday,
        out.period.calendarEntry.type.DateRange.startdate.wday, NULL);
    zassert_equal(in.period.calendarEntry.type.DateRange.startdate.year,
        out.period.calendarEntry.type.DateRange.startdate.year, NULL);
    zassert_equal(in.period.calendarEntry.type.DateRange.startdate.month,
        out.period.calendarEntry.type.DateRange.startdate.month, NULL);
    zassert_equal(in.period.calendarEntry.type.DateRange.startdate.day,
        out.period.calendarEntry.type.DateRange.startdate.day, NULL);

    zassert_equal(in.period.calendarEntry.type.DateRange.enddate.wday,
        out.period.calendarEntry.type.DateRange.enddate.wday, NULL);
    zassert_equal(in.period.calendarEntry.type.DateRange.enddate.year,
        out.period.calendarEntry.type.DateRange.enddate.year, NULL);
    zassert_equal(in.period.calendarEntry.type.DateRange.enddate.month,
        out.period.calendarEntry.type.DateRange.enddate.month, NULL);
    zassert_equal(in.period.calendarEntry.type.DateRange.enddate.day,
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

    BACNET_SPECIAL_EVENT in = {
        .periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY,
        .period = {
            .calendarEntry = {
                .tag = BACNET_CALENDAR_WEEK_N_DAY,
                .type = {
                    .WeekNDay = {
                        .month = 0xff,
                        .dayofweek = 1, /* mondays */
                        .weekofmonth = 0xff,
                    }
                }
            }
        },
        .timeValues = {
            .TV_Count = 0,
            .Time_Values = { 0 }
        },
        .priority = 16,
    };
    BACNET_SPECIAL_EVENT out = { 0 };

    len = bacnet_special_event_encode(apdu, &in);
    null_len = bacnet_special_event_encode(NULL, &in);
    zassert_equal(len, null_len, NULL);

    apdu_len = bacnet_special_event_decode(apdu, len, &out);
    zassert_equal(len, apdu_len, NULL);
    zassert_equal(in.periodTag, out.periodTag, NULL);

    zassert_equal(
        in.period.calendarEntry.tag, out.period.calendarEntry.tag, NULL);
    zassert_equal(in.period.calendarEntry.type.WeekNDay.month,
        out.period.calendarEntry.type.WeekNDay.month, NULL);
    zassert_equal(in.period.calendarEntry.type.WeekNDay.dayofweek,
        out.period.calendarEntry.type.WeekNDay.dayofweek, NULL);
    zassert_equal(in.period.calendarEntry.type.WeekNDay.weekofmonth,
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
    int len, apdu_len;
    BACNET_SPECIAL_EVENT out = { 0 };
    uint8_t sample[18] = { 0x0e, 0x0c, 0xff, 0x0a, 0x1c, 0xff, 0x0f, 0x2e, 0xb4,
        0x00, 0x00, 0x00, 0x00, 0x91, 0x00, 0x2f, 0x39, 0x10 };

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

    zassert_equal(out.timeValues.Time_Values[0].Value.tag,
        BACNET_APPLICATION_TAG_ENUMERATED, NULL);
    zassert_equal(
        out.timeValues.Time_Values[0].Value.type.Unsigned_Int, 0, NULL);

    zassert_equal(out.priority, 16, NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnetSpecialEvent_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(BACnetSpecialEvent_tests,
        ztest_unit_test(test_BACnetSpecialEvent_CalendarRef),
        ztest_unit_test(test_BACnetSpecialEvent_Date),
        ztest_unit_test(test_BACnetSpecialEvent_DateRange),
        ztest_unit_test(test_BACnetSpecialEvent_WeekNDate),
        ztest_unit_test(test_BACnetSpecialEvent_DecodeRealAPDU));

    ztest_run_test_suite(BACnetSpecialEvent_tests);
}
#endif
