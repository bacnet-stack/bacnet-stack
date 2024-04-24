/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/sys/platform.h>
#include <bacnet/lighting.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testBACnetLightingCommand(BACNET_LIGHTING_COMMAND *data)
{
    bool status = false;
    BACNET_LIGHTING_COMMAND test_data;
    int len, apdu_len;
    uint8_t apdu[MAX_APDU] = { 0 };
    char command_text[80] = "";

    /* copy */
    status = lighting_command_copy(&test_data, NULL);
    zassert_false(status, NULL);
    status = lighting_command_copy(NULL, data);
    zassert_false(status, NULL);
    status = lighting_command_copy(&test_data, data);
    zassert_true(status, NULL);
    status = lighting_command_same(&test_data, data);
    zassert_true(status, NULL);
    /* encode/decode */
    len = lighting_command_encode(apdu, data);
    apdu_len = lighting_command_decode(apdu, len, &test_data);
    zassert_true(
        len > 0, "lighting-command[%s] failed to encode!",
        bactext_lighting_operation_name(data->operation));
    zassert_true(
        apdu_len > 0, "lighting-command[%s] failed to decode!",
        bactext_lighting_operation_name(data->operation));
    status = lighting_command_same(&test_data, data);
    while (len) {
        len--;
        apdu_len = lighting_command_decode(apdu, len, NULL);
    }
    /* to/from ASCII */
    len = lighting_command_to_ascii(NULL, NULL, 0);
    zassert_equal(len, 0, NULL);
    len = lighting_command_to_ascii(data, NULL, 0);
    zassert_true(len > 0, NULL);
    len = lighting_command_to_ascii(data, command_text, 0);
    zassert_true(len > 0, NULL);
    len = lighting_command_to_ascii(data, command_text, sizeof(command_text));
    zassert_true(len > 0, NULL);
    status = lighting_command_from_ascii(NULL, command_text);
    zassert_false(status, NULL);
    status = lighting_command_from_ascii(&test_data, NULL);
    zassert_false(status, NULL);
    status = lighting_command_from_ascii(NULL, NULL);
    zassert_false(status, NULL);
    status = lighting_command_from_ascii(&test_data, command_text);
    zassert_true(status, NULL);
    status = lighting_command_same(&test_data, data);
    zassert_true(
        status, "lighting-command[%s] \"%s\" is different!",
        bactext_lighting_operation_name(data->operation), command_text);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lighting_tests, testBACnetLightingCommandAll)
#else
static void testBACnetLightingCommandAll(void)
#endif
{
    /*
    BACNET_LIGHTING_OPERATION operation;
    bool use_target_level:1;
    bool use_ramp_rate:1;
    bool use_step_increment:1;
    bool use_fade_time:1;
    bool use_priority:1;
    float target_level;
    float ramp_rate;
    float step_increment;
    uint32_t fade_time;
    uint8_t priority;
    */
    BACNET_LIGHTING_COMMAND test_data[] = {
        { BACNET_LIGHTS_NONE, false, false, false, false, false, 0.0, 100.0,
          1.0, 100, 1 },
        { BACNET_LIGHTS_FADE_TO, true, false, false, true, true, 100.0, 100.0,
          1.0, 100, 1 },
        { BACNET_LIGHTS_FADE_TO, true, false, false, false, false, 0.0, 100.0,
          1.0, 100, 1 },
        { BACNET_LIGHTS_RAMP_TO, true, true, false, false, true, 0.0, 100.0,
          1.0, 100, 1 },
        { BACNET_LIGHTS_RAMP_TO, true, false, false, false, false, 100.0, 100.0,
          1.0, 100, 1 },
        { BACNET_LIGHTS_STEP_UP, false, false, true, false, true, 100.0, 100.0,
          1.0, 100, 1 },
        { BACNET_LIGHTS_STEP_UP, false, false, true, false, false, 100.0, 100.0,
          2.0, 100, 1 },
        { BACNET_LIGHTS_STEP_DOWN, false, false, true, false, true, 100.0,
          100.0, 1.0, 100, 1 },
        { BACNET_LIGHTS_STEP_DOWN, false, false, true, false, false, 100.0,
          100.0, 2.0, 100, 1 },
        { BACNET_LIGHTS_STEP_ON, false, false, true, false, true, 100.0, 100.0,
          1.0, 100, 1 },
        { BACNET_LIGHTS_STEP_ON, false, false, true, false, false, 100.0, 100.0,
          2.0, 100, 1 },
        { BACNET_LIGHTS_STEP_OFF, false, false, true, false, true, 100.0, 100.0,
          1.0, 100, 1 },
        { BACNET_LIGHTS_STEP_OFF, false, false, true, false, false, 100.0,
          100.0, 2.0, 100, 1 },
        { BACNET_LIGHTS_STOP, false, false, false, false, true, 100.0, 100.0,
          1.0, 100, 1 },
        { BACNET_LIGHTS_STOP, false, false, false, false, false, 100.0, 100.0,
          2.0, 100, 1 },
    };
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(test_data); i++) {
        printf(
            "test-lighting-command[%s]\n",
            bactext_lighting_operation_name(test_data[i].operation));
        testBACnetLightingCommand(&test_data[i]);
    }
}
/**
 * @}
 */

/**
 * @brief Test
 */
static void testBACnetColorCommand(BACNET_COLOR_COMMAND *data)
{
    bool status = false;
    BACNET_COLOR_COMMAND test_data = { 0 };
    int len = 0, apdu_len = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_ERROR_CODE error_code;

    status = color_command_copy(&test_data, NULL);
    zassert_false(status, NULL);
    status = color_command_copy(NULL, data);
    zassert_false(status, NULL);
    status = color_command_copy(&test_data, data);
    zassert_true(status, NULL);
    status = color_command_same(&test_data, data);
    zassert_true(status, NULL);
    len = color_command_encode(apdu, data);
    apdu_len = color_command_decode(apdu, len, &error_code, &test_data);
    zassert_true(
        len > 0, "color-command[%s] failed to encode!",
        bactext_color_operation_name(data->operation));
    zassert_true(
        apdu_len > 0, "color-command[%s] failed to decode!",
        bactext_color_operation_name(data->operation));
    status = color_command_same(&test_data, data);
    while (len) {
        len--;
        apdu_len = color_command_decode(apdu, len, NULL, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lighting_tests, testBACnetColorCommandAll)
#else
static void testBACnetColorCommandAll(void)
#endif
{
    BACNET_COLOR_COMMAND test_data[] = {
        { .operation = BACNET_COLOR_OPERATION_NONE,
          .target.color_temperature = 0,
          .transit.fade_time = 0 },
        { .operation = BACNET_COLOR_OPERATION_STOP,
          .target.color_temperature = 0,
          .transit.fade_time = 0 },
        { .operation = BACNET_COLOR_OPERATION_FADE_TO_COLOR,
          .target.color.x_coordinate = 0.0,
          .target.color.y_coordinate = 0.0,
          .transit.fade_time = 0 },
        { .operation = BACNET_COLOR_OPERATION_FADE_TO_COLOR,
          .target.color.x_coordinate = 0.0,
          .target.color.y_coordinate = 0.0,
          .transit.fade_time = 2000 },
        { .operation = BACNET_COLOR_OPERATION_FADE_TO_CCT,
          .target.color_temperature = 1800,
          .transit.fade_time = 0 },
        { .operation = BACNET_COLOR_OPERATION_FADE_TO_CCT,
          .target.color_temperature = 1800,
          .transit.fade_time = 2000 },
        { .operation = BACNET_COLOR_OPERATION_RAMP_TO_CCT,
          .target.color_temperature = 1800,
          .transit.ramp_rate = 0 },
        { .operation = BACNET_COLOR_OPERATION_RAMP_TO_CCT,
          .target.color_temperature = 1800,
          .transit.ramp_rate = 20 },
        { .operation = BACNET_COLOR_OPERATION_STEP_UP_CCT,
          .target.color_temperature = 1800,
          .transit.step_increment = 0 },
        { .operation = BACNET_COLOR_OPERATION_STEP_UP_CCT,
          .target.color_temperature = 1800,
          .transit.step_increment = 1 },
        { .operation = BACNET_COLOR_OPERATION_STEP_DOWN_CCT,
          .target.color_temperature = 5000,
          .transit.step_increment = 0 },
        { .operation = BACNET_COLOR_OPERATION_STEP_DOWN_CCT,
          .target.color_temperature = 5000,
          .transit.step_increment = 1 },
    };
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(test_data); i++) {
        testBACnetColorCommand(&test_data[i]);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lighting_tests, testBACnetXYColor)
#else
static void testBACnetXYColor(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_XY_COLOR value = { 0 };
    BACNET_XY_COLOR test_value = { 0 };
    int len = 0, test_len = 0, null_len = 0;
    uint8_t tag_number = 1;
    bool status = false;

    value.x_coordinate = 1.0;
    value.y_coordinate = 1.0;

    null_len = xy_color_encode(NULL, &value);
    len = xy_color_encode(apdu, &value);
    zassert_equal(null_len, len, NULL);
    test_len = xy_color_decode(apdu, sizeof(apdu), &test_value);
    zassert_equal(test_len, len, NULL);
    status = xy_color_same(&value, &test_value);
    zassert_true(status, NULL);

    null_len = xy_color_context_encode(NULL, tag_number, &value);
    len = xy_color_context_encode(apdu, tag_number, &value);
    zassert_equal(null_len, len, NULL);
    test_len = xy_color_context_decode(apdu, len, tag_number, &test_value);
    zassert_equal(test_len, len, NULL);
    status = xy_color_same(&value, &test_value);
    zassert_true(status, NULL);
    while (len) {
        len--;
        test_len = xy_color_context_decode(apdu, len, tag_number, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(lighting_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        lighting_tests, ztest_unit_test(testBACnetLightingCommandAll),
        ztest_unit_test(testBACnetColorCommandAll),
        ztest_unit_test(testBACnetXYColor));

    ztest_run_test_suite(lighting_tests);
}
#endif
