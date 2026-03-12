/**
 * @file
 * @brief Unit test for BACnetShedLevel.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/ztest.h>
#include "bacnet/bactext.h"
#include "bacnet/bacstr.h"
#include "bacnet/shed_level.h"

/**
 * @addtogroup bacnet_tests
 * @{
 */

/** @brief Test the BACnetShedLevel complex data encode/decode */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetShedLevel_Tests, test_BACnetShedLevel)
#else
static void test_BACnetShedLevel(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;
    bool status;
    BACNET_SHED_LEVEL *value;
    BACNET_SHED_LEVEL case_value[] = {
        { .type = BACNET_SHED_TYPE_PERCENT, .value.percent = 75 },
        { .type = BACNET_SHED_TYPE_LEVEL, .value.level = 12345 },
        { .type = BACNET_SHED_TYPE_AMOUNT, .value.amount = 3.141592654f },
    };
    size_t i;
    BACNET_SHED_LEVEL test_value = { 0 };
    char buffer[64];

    for (i = 0; i < ARRAY_SIZE(case_value); i++) {
        value = &case_value[i];
        null_len = bacnet_shed_level_encode(apdu, NULL);
        zassert_equal(
            null_len, 0, "value->type: %s null_len=%d",
            bactext_shed_level_type_name(value->type), null_len);
        null_len = bacnet_shed_level_encode(NULL, value);
        apdu_len = bacnet_shed_level_encode(apdu, value);
        zassert_equal(
            apdu_len, null_len, "value->type: %s len=%d null_len=%d",
            bactext_shed_level_type_name(value->type), apdu_len, null_len);
        null_len = bacnet_shed_level_decode(NULL, apdu_len, &test_value);
        zassert_equal(
            null_len, BACNET_STATUS_ERROR, "value->type: %s null_len=%d",
            bactext_shed_level_type_name(value->type), null_len);
        null_len = bacnet_shed_level_decode(apdu, apdu_len, NULL);
        zassert_equal(
            null_len, apdu_len, "value->type: %s null_len=%d apdu_len=%d",
            bactext_shed_level_type_name(value->type), null_len, apdu_len);
        test_len = bacnet_shed_level_decode(apdu, apdu_len, &test_value);
        zassert_not_equal(
            test_len, BACNET_STATUS_ERROR, "value->type: %s test_len=%d",
            bactext_shed_level_type_name(value->type), test_len);
        zassert_equal(
            test_len, apdu_len, "value->type: %s test_len=%d apdu_len=%d",
            bactext_shed_level_type_name(value->type), test_len, apdu_len);
        zassert_equal(
            value->type, test_value.type, "value->type: %s test_type=%s",
            bactext_shed_level_type_name(value->type),
            bactext_shed_level_type_name(test_value.type));
        status = bacnet_shed_level_same(value, &test_value);
        zassert_true(
            status, "decode: different: %s",
            bactext_shed_level_type_name(value->type));
        status = bacnet_shed_level_copy(&test_value, value);
        zassert_true(
            status, "copy: failed: %s",
            bactext_shed_level_type_name(value->type));
        status = bacnet_shed_level_same(value, &test_value);
        zassert_true(
            status, "copy: different: %s",
            bactext_shed_level_type_name(value->type));
        test_len = bacapp_snprintf_shed_level(buffer, sizeof(buffer), value);
        zassert_true(
            test_len > 0, "snprintf: failed: %s",
            bactext_shed_level_type_name(value->type));
        test_len = bacapp_snprintf_shed_level(buffer, 0, value);
        zassert_true(
            test_len > 0, "snprintf length only: failed: %s",
            bactext_shed_level_type_name(value->type));
    }
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnetShedLevel_Tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        BACnetShedLevel_Tests, ztest_unit_test(test_BACnetShedLevel));

    ztest_run_test_suite(BACnetShedLevel_Tests);
}
#endif
