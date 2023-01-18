/**
 * @file
 * @brief Unit test for BACnetWeeklySchedule
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @date Aug 2022
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
#include "bacnet/weeklyschedule.h"
#include "bacnet/dailyschedule.h"
#include "bacnet/datetime.h"
#include "bacnet/bacapp.h"


/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API
 */
static void test_BACnetWeeklySchedule()
{
    int len, apdu_len;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_WEEKLY_SCHEDULE empty_value = { 0 };
    BACNET_WEEKLY_SCHEDULE value = { 0 };
    BACNET_WEEKLY_SCHEDULE decoded = { 0 };
    int diff = 0;
    bool status = false;
    uint8_t tag_number = 0;

    value.weeklySchedule[0].TV_Count = 2;

    value.weeklySchedule[0].Time_Values[0].Time = (BACNET_TIME) {
        .hour = 5,
        .min = 30
    };
    value.weeklySchedule[0].Time_Values[0].Value = (BACNET_PRIMITIVE_DATA_VALUE) {
        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
        .type.Unsigned_Int = 123,
    };

    value.weeklySchedule[0].Time_Values[1].Time = (BACNET_TIME) {
        .hour = 15,
        .min = 0
    };
    value.weeklySchedule[0].Time_Values[1].Value = (BACNET_PRIMITIVE_DATA_VALUE) {
        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
        .type.Unsigned_Int = 456,
    };

    value.weeklySchedule[6].TV_Count = 1;
    value.weeklySchedule[6].Time_Values[0].Value = (BACNET_PRIMITIVE_DATA_VALUE) {
        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
        .type.Unsigned_Int = 777,
    };


    len = bacnet_weeklyschedule_encode(apdu, &value);
    apdu_len = bacnet_weeklyschedule_decode(apdu, len, &decoded);
    zassert_true(len > 0, NULL);
    zassert_true(apdu_len > 0, NULL);
    status = bacnet_weeklyschedule_same(&value, &decoded);
    zassert_true(status, NULL);
    // Check that the compare function really compares:
    status = bacnet_weeklyschedule_same(&empty_value, &decoded);
    zassert_false(status, NULL);

    len = bacnet_weeklyschedule_context_encode(apdu, tag_number, &value);
    apdu_len = bacnet_weeklyschedule_context_decode(apdu, len, tag_number, &decoded);
    zassert_true(len > 0, NULL);
    zassert_true(apdu_len > 0, NULL);
    status = bacnet_weeklyschedule_same(&value, &decoded);
    zassert_true(status, NULL);

    /* negative testing - the tag differs */
    tag_number++;
    apdu_len = bacnet_weeklyschedule_context_decode(apdu, len, tag_number, &decoded);
    zassert_true(apdu_len < 0, NULL);
}

/**
 * @}
 */
void test_main(void)
{
    ztest_test_suite(BACnetWeeklySchedule_tests,
     ztest_unit_test(test_BACnetWeeklySchedule)
     );

    ztest_run_test_suite(BACnetWeeklySchedule_tests);
}
