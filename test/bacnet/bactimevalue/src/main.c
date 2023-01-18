/**
 * @file
 * @brief Unit test for BACnetTimeValue
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2022
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
#include "bacnet/datetime.h"
#include "bacnet/bacapp.h"


/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API
 */
static void test_BACnetTimeValue(BACNET_TIME_VALUE *value)
{
    int len, apdu_len;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_TIME_VALUE test_value = { 0 };
    int diff = 0;
    bool status = false;
    uint8_t tag_number = 0;

    len = bacnet_time_value_encode(apdu, value);
    apdu_len = bacnet_time_value_decode(apdu, len, &test_value);
    zassert_true(len > 0, NULL);
    zassert_true(apdu_len > 0, NULL);
    diff = datetime_compare_time(&test_value.Time, &value->Time);
    zassert_true(diff == 0, NULL);
    status = bacapp_same_value(
        (BACNET_APPLICATION_DATA_VALUE *) &test_value.Value,
        (BACNET_APPLICATION_DATA_VALUE *) &value->Value);
    zassert_true(status, NULL);

    len = bacnet_time_value_context_encode(apdu, tag_number, value);
    apdu_len = bacnet_time_value_context_decode(apdu, len, tag_number, &test_value);
    zassert_true(len > 0, NULL);
    zassert_true(apdu_len > 0, NULL);
    diff = datetime_compare_time(&test_value.Time, &value->Time);
    zassert_true(diff == 0, NULL);
    status = bacapp_same_value(
        (BACNET_APPLICATION_DATA_VALUE *) &test_value.Value,
        (BACNET_APPLICATION_DATA_VALUE *) &value->Value);
    zassert_true(status, NULL);
    /* negative testing */
    tag_number++;
    apdu_len = bacnet_time_value_context_decode(apdu, len, tag_number, &test_value);
    zassert_true(apdu_len < 0, NULL);
}

/**
 * @brief Test encode/decode API
 */
static void test_BACnetTimeValues(void)
{
    BACNET_TIME_VALUE time_value = { 0 };

    test_BACnetTimeValue(&time_value);
    bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, "4.2",
        &time_value.Value);
    datetime_time_init_ascii(&time_value.Time, "12:00");
    test_BACnetTimeValue(&time_value);
    bacapp_parse_application_data(BACNET_APPLICATION_TAG_UNSIGNED_INT, "99999",
        &time_value.Value);
    datetime_time_init_ascii(&time_value.Time, "23:59:59");
    test_BACnetTimeValue(&time_value);
}
/**
 * @}
 */
void test_main(void)
{
    ztest_test_suite(BACnetTimeValue_tests,
     ztest_unit_test(test_BACnetTimeValues)
     );

    ztest_run_test_suite(BACnetTimeValue_tests);
}
