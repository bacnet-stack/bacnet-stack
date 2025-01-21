/* @file
 * @brief test BACnet integer encode/decode APIs
 * @date June 2022
 * @brief tests sRGB to and from from CIE xy and brightness API
 *
 * @section LICENSE
 * Copyright (c) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/lighting_command.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static float Tracking_Value;

/**
 * @brief Callback for tracking value updates
 * @param  old_value - value prior to write
 * @param  value - value of the write
 */
static void dimmer_tracking_value(uint32_t key, float old_value, float value)
{
    (void)key;
    (void)old_value;
    Tracking_Value = value;
}

/**
 * @brief compare two floating point values to 3 decimal places
 *
 * @param x1 - first comparison value
 * @param x2 - second comparison value
 * @return true if the value is the same to 3 decimal points
 */
static bool is_float_equal(float x1, float x2)
{
    return fabs(x1 - x2) < 0.001;
}

/**
 * @brief test dimmer blink handler
 * @param data - dimmer data
 * @param blink - blink data
 * @param milliseconds - number of milliseconds per timer call
 */
static void test_lighting_command_blink_unit(BACNET_LIGHTING_COMMAND_DATA *data)
{
    uint16_t milliseconds = 10;
    uint32_t duration = 0;
    BACNET_LIGHTING_COMMAND_WARN_DATA *blink = &data->Blink;

    lighting_command_fade_to(data, data->Max_Actual_Value, 0);
    lighting_command_timer(data, milliseconds);
    zassert_true(data->In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, data->Max_Actual_Value), NULL);
    if (blink->Duration == 0) {
        /* immediate */
        lighting_command_blink_warn(data, BACNET_LIGHTS_WARN, blink);
        lighting_command_timer(data, milliseconds);
        zassert_true(data->In_Progress == BACNET_LIGHTING_IDLE, NULL);
        zassert_true(is_float_equal(Tracking_Value, blink->End_Value), NULL);
    } else if (blink->Interval == 0) {
        /* no blink, just egress timing */
        lighting_command_blink_warn(data, BACNET_LIGHTS_WARN, blink);
        lighting_command_timer(data, milliseconds);
        zassert_true(data->In_Progress == BACNET_LIGHTING_OTHER, NULL);
        zassert_true(is_float_equal(Tracking_Value, blink->On_Value), NULL);
        milliseconds = blink->Duration;
        lighting_command_blink_warn(data, BACNET_LIGHTS_WARN, blink);
        lighting_command_timer(data, milliseconds);
        zassert_true(data->In_Progress == BACNET_LIGHTING_IDLE, NULL);
        zassert_true(is_float_equal(Tracking_Value, blink->End_Value), NULL);
    } else {
        /* blinking and egress timing */
        if ((blink->Count > 0) && (blink->Count < UINT16_MAX)) {
            duration = blink->Count * blink->Interval * 2UL;
            if (duration > blink->Duration) {
                duration = blink->Duration;
            }
        } else {
            duration = blink->Duration;
        }
        milliseconds = blink->Interval;
        do {
            lighting_command_blink_warn(data, BACNET_LIGHTS_WARN, blink);
            lighting_command_timer(data, milliseconds);
            if (blink->Duration) {
                zassert_true(
                    data->In_Progress == BACNET_LIGHTING_OTHER,
                    "In_Progress=%d", data->In_Progress);
                if (data->Blink.State) {
                    zassert_true(
                        is_float_equal(Tracking_Value, blink->Off_Value),
                        "Tracking_Value=%f", Tracking_Value);
                } else {
                    zassert_true(
                        is_float_equal(Tracking_Value, blink->On_Value),
                        "Tracking_Value=%f", Tracking_Value);
                }
            } else {
                zassert_true(
                    data->In_Progress == BACNET_LIGHTING_IDLE, "In_Progress=%d",
                    data->In_Progress);
                zassert_true(
                    is_float_equal(Tracking_Value, blink->End_Value),
                    "Tracking_Value=%f", Tracking_Value);
            }
        } while (blink->Duration);
    }
}

/**
 * Tests for Dimmer Command
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lighting_command_tests, test_lighting_command_command_unit)
#else
static void test_lighting_command_command_unit(void)
#endif
{
    BACNET_LIGHTING_COMMAND_DATA data = { 0 };
    uint16_t milliseconds = 10;
    uint32_t fade_time = 1000;
    float target_level = 100.0f;
    float target_step = 1.0f;

    lighting_command_init(&data);
    data.Notification_Head.callback = dimmer_tracking_value;
    lighting_command_stop(&data);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    lighting_command_none(&data);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    /* fade up */
    lighting_command_fade_to(&data, 100.0f, fade_time);
    milliseconds = fade_time / 2;
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_FADE_ACTIVE, NULL);
    zassert_true(
        is_float_equal(Tracking_Value, 50.5f), "Tracking_Value=%f",
        Tracking_Value);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    /* fade down */
    target_level = 0.0f;
    lighting_command_fade_to(&data, target_level, fade_time);
    milliseconds = fade_time / 2;
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_FADE_ACTIVE, NULL);
    zassert_true(
        is_float_equal(Tracking_Value, 50.5f), "Tracking_Value=%f",
        Tracking_Value);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, 0.0f), NULL);
    /* low trim */
    data.Low_Trim_Value = 10.0f;
    target_level = 1.0f;
    milliseconds = 10;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, data.Low_Trim_Value), NULL);
    target_level = 0.0f;
    milliseconds = 10;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    data.Low_Trim_Value = data.Min_Actual_Value;
    /* high trim */
    data.High_Trim_Value = 90.0f;
    target_level = 100.0f;
    milliseconds = 10;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, data.High_Trim_Value), NULL);
    data.High_Trim_Value = data.Max_Actual_Value;

    /* step UP - inhibit ON */
    target_step = 1.0f;
    target_level = 0.0f;
    milliseconds = 10;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    lighting_command_step(&data, BACNET_LIGHTS_STEP_UP, target_step);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, 0.0f), NULL);
    /* step UP while ON */
    target_step = 1.0f;
    target_level = 1.0f;
    milliseconds = 10;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    lighting_command_step(&data, BACNET_LIGHTS_STEP_UP, target_step);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(
        is_float_equal(Tracking_Value, target_step + target_level), NULL);
    /* clamp to max */
    target_step = 100.0f;
    lighting_command_step(&data, BACNET_LIGHTS_STEP_UP, target_step);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, data.Max_Actual_Value), NULL);
    /* turn ON, then step UP */
    target_step = 1.0f;
    target_level = 0.0f;
    milliseconds = 10;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    lighting_command_step(&data, BACNET_LIGHTS_STEP_ON, target_step);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, 1.0f), NULL);
    /* clamp to max */
    target_step = 100.0f;
    lighting_command_step(&data, BACNET_LIGHTS_STEP_ON, target_step);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, data.Max_Actual_Value), NULL);
    /* step DOWN, not off */
    target_step = 1.0f;
    target_level = data.Min_Actual_Value + target_step;
    milliseconds = 10;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    lighting_command_step(&data, BACNET_LIGHTS_STEP_DOWN, target_step);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, data.Min_Actual_Value), NULL);
    /* clamp to min */
    target_step = 100.0f;
    lighting_command_step(&data, BACNET_LIGHTS_STEP_DOWN, target_step);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, data.Min_Actual_Value), NULL);
    /* step DOWN and off */
    target_step = 100.0f;
    target_level = data.Min_Actual_Value;
    milliseconds = 10;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    lighting_command_step(&data, BACNET_LIGHTS_STEP_OFF, target_step);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, 0.0f), NULL);
    /* blink warn - immediate off */
    data.Blink.Interval = 0;
    data.Blink.Duration = 0;
    data.Blink.State = false;
    data.Blink.On_Value = 100.0f;
    data.Blink.Off_Value = 0.0f;
    data.Blink.End_Value = 0.0f;
    data.Blink.Count = UINT16_MAX;
    test_lighting_command_blink_unit(&data);
    /* blink warn - off after duration */
    data.Blink.Interval = 0;
    data.Blink.Duration = 1000;
    data.Blink.State = false;
    data.Blink.On_Value = 100.0f;
    data.Blink.Off_Value = 0.0f;
    data.Blink.End_Value = 0.0f;
    data.Blink.Count = UINT16_MAX;
    test_lighting_command_blink_unit(&data);
    /* blink warn - on/off for duration */
    data.Blink.Interval = 500;
    data.Blink.Duration = 2000;
    data.Blink.State = false;
    data.Blink.On_Value = 100.0f;
    data.Blink.Off_Value = 0.0f;
    data.Blink.End_Value = 0.0f;
    data.Blink.Count = UINT16_MAX;
    test_lighting_command_blink_unit(&data);

    /* quick ramp */
    target_level = 0.0f;
    milliseconds = 1000;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    target_level = 100.0f;
    lighting_command_ramp_to(&data, target_level, 100.0f);
    lighting_command_timer(&data, milliseconds);
    zassert_true(
        data.In_Progress == BACNET_LIGHTING_RAMP_ACTIVE, "In_Progress=%d",
        data.In_Progress);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    lighting_command_timer(&data, milliseconds);
    zassert_true(
        data.In_Progress == BACNET_LIGHTING_IDLE, "In_Progress=%d",
        data.In_Progress);
    /* slower ramp up */
    target_level = 100.0f;
    milliseconds = 100;
    do {
        lighting_command_ramp_to(&data, target_level, 1.0f);
        lighting_command_timer(&data, milliseconds);
        if (data.Lighting_Operation == BACNET_LIGHTS_RAMP_TO) {
            zassert_true(
                data.In_Progress == BACNET_LIGHTING_RAMP_ACTIVE,
                "In_Progress=%d", data.In_Progress);
            zassert_true(
                isgreater(data.Tracking_Value, data.Min_Actual_Value),
                "Tracking_Value=%f", Tracking_Value);
            zassert_true(
                isless(data.Tracking_Value, data.Max_Actual_Value),
                "Tracking_Value=%f", Tracking_Value);
        }
    } while (data.Lighting_Operation != BACNET_LIGHTS_STOP);

    /* slower ramp down */
    target_level = data.Min_Actual_Value;
    milliseconds = 33;
    do {
        lighting_command_ramp_to(&data, target_level, 0.1f);
        lighting_command_timer(&data, milliseconds);
        if (data.Lighting_Operation == BACNET_LIGHTS_RAMP_TO) {
            zassert_true(
                data.In_Progress == BACNET_LIGHTING_RAMP_ACTIVE,
                "In_Progress=%d", data.In_Progress);
            zassert_true(
                isgreater(data.Tracking_Value, data.Min_Actual_Value),
                "Tracking_Value=%f", Tracking_Value);
            zassert_true(
                isless(data.Tracking_Value, data.Max_Actual_Value),
                "Tracking_Value=%f", Tracking_Value);
        }
    } while (data.Lighting_Operation != BACNET_LIGHTS_STOP);

    /* null check code coverage */
    lighting_command_fade_to(NULL, 0.0f, 0);
    lighting_command_ramp_to(NULL, 0.0f, 0.0f);
    lighting_command_step(NULL, BACNET_LIGHTS_STEP_OFF, 0.0f);
    lighting_command_blink_warn(NULL, BACNET_LIGHTS_WARN, NULL);
    lighting_command_stop(NULL);
    lighting_command_none(NULL);
    lighting_command_timer(NULL, 0);
    lighting_command_init(NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(lighting_command_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        lighting_command_tests,
        ztest_unit_test(test_lighting_command_command_unit));

    ztest_run_test_suite(lighting_command_tests);
}
#endif
