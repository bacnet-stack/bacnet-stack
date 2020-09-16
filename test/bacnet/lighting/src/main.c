/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/bacdef.h>
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

    status = lighting_command_copy(&test_data, NULL);
    zassert_false(status, NULL);
    status = lighting_command_copy(NULL, data);
    zassert_false(status, NULL);
    status = lighting_command_copy(&test_data, data);
    zassert_true(status, NULL);
    status = lighting_command_same(&test_data, data);
    zassert_true(status, NULL);
    len = lighting_command_encode(apdu, data);
    apdu_len = lighting_command_decode(apdu, sizeof(apdu), &test_data);
    zassert_true(len > 0, NULL);
    zassert_true(apdu_len > 0, NULL);
    status = lighting_command_same(&test_data, data);
}

static void testBACnetLightingCommandAll(void)
{
    BACNET_LIGHTING_COMMAND data;

    data.operation = BACNET_LIGHTS_NONE;
    data.use_target_level = false;
    data.use_ramp_rate = false;
    data.use_step_increment = false;
    data.use_fade_time = false;
    data.use_priority = false;
    data.target_level = 0.0;
    data.ramp_rate = 100.0;
    data.step_increment = 1.0;
    data.fade_time = 100;
    data.priority = 1;
    testBACnetLightingCommand(&data);
    data.operation = BACNET_LIGHTS_STOP;
    data.use_target_level = true;
    data.use_ramp_rate = true;
    data.use_step_increment = true;
    data.use_fade_time = true;
    data.use_priority = true;
    testBACnetLightingCommand(&data);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(lighting_tests,
     ztest_unit_test(testBACnetLightingCommandAll)
     );

    ztest_run_test_suite(lighting_tests);
}
