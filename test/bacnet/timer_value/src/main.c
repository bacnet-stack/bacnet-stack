/**
 * @file
 * @brief Unit test for BACnetChannelValue.
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
#include "bacnet/timer_value.h"

/**
 * @addtogroup bacnet_tests
 * @{
 */

/** @brief try decoding a real sample from Siemens, captured with wireshark */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetTimerValue_Tests, test_BACnetTimerValue)
#else
static void test_BACnetTimerValue(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;
    bool status;
    BACNET_TIMER_STATE_CHANGE_VALUE *value;
    BACNET_TIMER_STATE_CHANGE_VALUE case_value[] = {
        { .tag = BACNET_APPLICATION_TAG_NULL, .type.Real = 0.0f, .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_NO_VALUE,
          .type.Unsigned_Int = 0,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_BOOLEAN,
          .type.Boolean = false,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
          .type.Unsigned_Int = 0xDEADBEEF,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_SIGNED_INT,
          .type.Signed_Int = 0x00C0FFEE,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_REAL,
          .type.Real = 3.141592654f,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_DOUBLE,
          .type.Double = 2.32323232323,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_ENUMERATED,
          .type.Enumerated = 0x0BADF00D,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_LIGHTING_COMMAND,
          .type.Lighting_Command.operation = BACNET_LIGHTS_NONE,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_COLOR_COMMAND,
          .type.Color_Command.operation = BACNET_COLOR_OPERATION_NONE,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_XY_COLOR,
          .type.XY_Color.x_coordinate = 0.0f,
          .type.XY_Color.y_coordinate = 0.0f,
          .next = NULL },
    };
    struct to_ascii_test_value {
        const char *string;
        BACNET_TIMER_STATE_CHANGE_VALUE value;
    } to_ascii_values[] = {
        { "NULL",
          { .tag = BACNET_APPLICATION_TAG_NULL,
            .type.Real = 0.0f,
            .next = NULL } },
        { "FALSE",
          { .tag = BACNET_APPLICATION_TAG_BOOLEAN,
            .type.Boolean = false,
            .next = NULL } },
        { "1234567890",
          { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 1234567890,
            .next = NULL } },
        { "-1234567890",
          { .tag = BACNET_APPLICATION_TAG_SIGNED_INT,
            .type.Signed_Int = -1234567890,
            .next = NULL } },
        { "3.141593",
          { .tag = BACNET_APPLICATION_TAG_REAL,
            .type.Real = 3.141592654f,
            .next = NULL } },
        { "-3.141593",
          { .tag = BACNET_APPLICATION_TAG_REAL,
            .type.Real = -3.141592654f,
            .next = NULL } },
        { "0",
          { .tag = BACNET_APPLICATION_TAG_LIGHTING_COMMAND,
            .type.Lighting_Command = { .operation = BACNET_LIGHTS_NONE },
            .next = NULL } },
        { "1,75.000000,5,8",
          { .tag = BACNET_APPLICATION_TAG_LIGHTING_COMMAND,
            .type.Lighting_Command = { .operation = BACNET_LIGHTS_FADE_TO,
                                       .use_fade_time = true,
                                       .fade_time = 5,
                                       .use_target_level = true,
                                       .target_level = 75.0f,
                                       .use_priority = true,
                                       .priority = 8 },
            .next = NULL } },
    };
    struct from_ascii_test_value {
        const char *string;
        BACNET_APPLICATION_TAG tag;
    } from_ascii_values[] = {
        { "NULL", BACNET_APPLICATION_TAG_NULL },
        { "FALSE", BACNET_APPLICATION_TAG_BOOLEAN },
        { "1234567890", BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { "-1234567890", BACNET_APPLICATION_TAG_SIGNED_INT },
        { "3.141592654", BACNET_APPLICATION_TAG_REAL },
        { "-3.141592654", BACNET_APPLICATION_TAG_REAL },
        { "F1.21", BACNET_APPLICATION_TAG_REAL },
        { "f1.21", BACNET_APPLICATION_TAG_REAL },
        { "D1.21", BACNET_APPLICATION_TAG_DOUBLE },
        { "d1.21", BACNET_APPLICATION_TAG_DOUBLE },
        { "L0", BACNET_APPLICATION_TAG_LIGHTING_COMMAND },
        { "l0", BACNET_APPLICATION_TAG_LIGHTING_COMMAND },
        { "C0", BACNET_APPLICATION_TAG_COLOR_COMMAND },
        { "c0", BACNET_APPLICATION_TAG_COLOR_COMMAND },
        { "X0.0,0.0", BACNET_APPLICATION_TAG_XY_COLOR },
        { "x0.0,0.0", BACNET_APPLICATION_TAG_XY_COLOR },
    };
    size_t i;
    BACNET_TIMER_STATE_CHANGE_VALUE test_value = { 0 };
    char test_string[64];

    bacnet_timer_value_link_array(case_value, ARRAY_SIZE(case_value));
    value = &case_value[0];
    while (value) {
        null_len = bacnet_timer_value_encode(NULL, sizeof(apdu), value);
        if (value->tag != BACNET_APPLICATION_TAG_NULL) {
            zassert_not_equal(null_len, 0, NULL);
        }
        apdu_len = bacnet_timer_value_encode(apdu, sizeof(apdu), value);
        zassert_equal(
            apdu_len, null_len, "value->tag: %s len=%d null_len=%d",
            bactext_application_tag_name(value->tag), apdu_len, null_len);
        null_len = bacnet_timer_value_decode(NULL, apdu_len, &test_value);
        zassert_equal(null_len, BACNET_STATUS_ERROR, NULL);
        null_len = bacnet_timer_value_decode(apdu, apdu_len, NULL);
        zassert_equal(null_len, BACNET_STATUS_ERROR, NULL);
        test_len = bacnet_timer_value_decode(apdu, apdu_len, &test_value);
        zassert_not_equal(
            test_len, BACNET_STATUS_ERROR, "value->tag: %s test_len=%d",
            bactext_application_tag_name(value->tag), test_len);
        zassert_equal(test_len, apdu_len, NULL);
        zassert_equal(
            value->tag, test_value.tag, "value->tag: %s test_tag=%s",
            bactext_application_tag_name(value->tag),
            bactext_application_tag_name(test_value.tag));
        status = bacnet_timer_value_same(value, &test_value);
        zassert_true(
            status, "decode: different: %s",
            bactext_application_tag_name(value->tag));
        status = bacnet_timer_value_copy(&test_value, value);
        zassert_true(
            status, "copy: failed: %s",
            bactext_application_tag_name(value->tag));
        status = bacnet_timer_value_same(value, &test_value);
        zassert_true(
            status, "copy: different: %s",
            bactext_application_tag_name(value->tag));
        value = value->next;
    }
    for (i = 0; i < ARRAY_SIZE(from_ascii_values); i++) {
        status = bacnet_timer_value_from_ascii(
            &test_value, from_ascii_values[i].string);
        zassert_true(
            status, "from_ascii: failed: %s", from_ascii_values[i].string);
        zassert_equal(
            test_value.tag, from_ascii_values[i].tag, "from_ascii: %s",
            from_ascii_values[i].string);
    }
    for (i = 0; i < ARRAY_SIZE(to_ascii_values); i++) {
        status = bacnet_timer_value_to_ascii(
            &to_ascii_values[i].value, test_string, sizeof(test_string));
        zassert_true(status, "to_ascii: failed: %s", to_ascii_values[i].string);
        if (status) {
            zassert_true(
                bacnet_stricmp(test_string, to_ascii_values[i].string) == 0,
                "to_ascii: failed: got %s expected: %s", test_string,
                to_ascii_values[i].string);
        }
    }
    /* no-value API */
    null_len = bacnet_timer_value_no_value_encode(NULL, 0);
    zassert_not_equal(null_len, 0, NULL);
    apdu_len = bacnet_timer_value_no_value_encode(apdu, 0);
    zassert_not_equal(apdu_len, 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_timer_value_no_value_decode(NULL, 0, 0);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    test_len = bacnet_timer_value_no_value_decode(apdu, apdu_len, 0);
    zassert_equal(apdu_len, test_len, NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnetTimerValue_Tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        BACnetTimerValue_Tests, ztest_unit_test(test_BACnetTimerValue));

    ztest_run_test_suite(BACnetTimerValue_Tests);
}
#endif
