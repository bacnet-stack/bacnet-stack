/**
 * @file
 * @brief Unit test for BACnetSpecialEvent
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
ZTEST(BACnetSpecialEvent_tests, test_BACnetSpecialEvent)
#else
static void test_BACnetSpecialEvent(void)
#endif
{
    int len, apdu_len;
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t sample[18] = { 0x0e, 0x0c, 0xff, 0x0a, 0x1c, 0xff, 0x0f, 0x2e, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x91, 0x00, 0x2f, 0x39, 0x10 };

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
    apdu_len = bacnet_special_event_decode(apdu, len, &out);

    zassert_equal(len, apdu_len, NULL);

    zassert_equal(in.periodTag, out.periodTag, NULL);
    zassert_equal(in.period.calendarReference.instance, out.period.calendarReference.instance, NULL);
    zassert_equal(in.period.calendarReference.type, out.period.calendarReference.type, NULL);
    zassert_equal(in.timeValues.TV_Count, out.timeValues.TV_Count, NULL);

    zassert_equal(in.timeValues.Time_Values[0].Time.hour, out.timeValues.Time_Values[0].Time.hour, NULL);
    zassert_equal(in.timeValues.Time_Values[0].Time.min, out.timeValues.Time_Values[0].Time.min, NULL);
    zassert_equal(in.timeValues.Time_Values[0].Time.sec, out.timeValues.Time_Values[0].Time.sec, NULL);
    zassert_equal(in.timeValues.Time_Values[0].Time.hundredths, out.timeValues.Time_Values[0].Time.hundredths, NULL);

    zassert_equal(in.timeValues.Time_Values[0].Value.tag, out.timeValues.Time_Values[0].Value.tag, NULL);
    zassert_equal(in.timeValues.Time_Values[0].Value.type.Unsigned_Int, out.timeValues.Time_Values[0].Value.type.Unsigned_Int, NULL);

    zassert_equal(in.timeValues.Time_Values[1].Time.hour, out.timeValues.Time_Values[1].Time.hour, NULL);
    zassert_equal(in.timeValues.Time_Values[1].Time.min, out.timeValues.Time_Values[1].Time.min, NULL);
    zassert_equal(in.timeValues.Time_Values[1].Time.sec, out.timeValues.Time_Values[1].Time.sec, NULL);
    zassert_equal(in.timeValues.Time_Values[1].Time.hundredths, out.timeValues.Time_Values[1].Time.hundredths, NULL);

    zassert_equal(in.timeValues.Time_Values[1].Value.tag, out.timeValues.Time_Values[1].Value.tag, NULL);
    zassert_equal(in.timeValues.Time_Values[1].Value.type.Unsigned_Int, out.timeValues.Time_Values[1].Value.type.Unsigned_Int, NULL);

    zassert_equal(in.priority, out.priority, NULL);


    /* try decoding a real sample */

    memset(&out, 0, sizeof(out));

    apdu_len = bacnet_special_event_decode(sample, sizeof(sample), &out);
    zassert_equal(sizeof(sample), apdu_len, NULL);

    zassert_equal(out.periodTag, BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY, NULL);
    zassert_equal(out.period.calendarEntry.tag, BACNET_CALENDAR_DATE, NULL);
    zassert_equal(out.period.calendarEntry.type.Date.day, 28, NULL);
    zassert_equal(out.period.calendarEntry.type.Date.month, 10, NULL);
    zassert_equal(out.period.calendarEntry.type.Date.year, 2155 /* any */, NULL);
    zassert_equal(out.period.calendarEntry.type.Date.wday, 0xff /* any */, NULL);

    zassert_equal(out.timeValues.TV_Count, 1, NULL);

    zassert_equal(out.timeValues.Time_Values[0].Time.hour, 0, NULL);
    zassert_equal(out.timeValues.Time_Values[0].Time.min, 0, NULL);
    zassert_equal(out.timeValues.Time_Values[0].Time.sec, 0, NULL);
    zassert_equal(out.timeValues.Time_Values[0].Time.hundredths, 0, NULL);

    zassert_equal(out.timeValues.Time_Values[0].Value.tag, BACNET_APPLICATION_TAG_ENUMERATED, NULL);
    zassert_equal(out.timeValues.Time_Values[0].Value.type.Unsigned_Int, 0, NULL);

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
     ztest_unit_test(test_BACnetSpecialEvent)
     );

    ztest_run_test_suite(BACnetSpecialEvent_tests);
}
#endif
