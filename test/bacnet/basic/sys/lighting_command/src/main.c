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
static uint16_t Tracking_Elapsed_Milliseconds;

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
 * @brief Callback for non-standard Lighting_Operation timer values
 * @param  data - Lighting Command data structure
 * @param  milliseconds - elapsed time in milliseconds
 */
static void dimmer_timer_task(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    Tracking_Elapsed_Milliseconds = milliseconds;
    switch (data->Lighting_Operation) {
        case BACNET_LIGHTS_PROPRIETARY_MIN:
            break;
        case BACNET_LIGHTS_PROPRIETARY_MAX:
            break;
        default:
            break;
    }
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
    struct lighting_command_notification observer1 = { 0 };
    struct lighting_command_notification observer2 = { 0 };
    struct lighting_command_timer_notification timer_observer1 = { 0 };
    struct lighting_command_timer_notification timer_observer2 = { 0 };
    uint16_t milliseconds = 10;
    uint32_t fade_time = 1000;
    float target_level = 100.0f;
    float target_step = 1.0f;
    float ramp_rate = 1000.0f;
    float override_level;

    lighting_command_init(&data);
    /* lighting command subscribe */
    observer1.callback = dimmer_tracking_value;
    lighting_command_notification_add(&data, &observer1);
    /* add again to verify skipping */
    lighting_command_notification_add(&data, &observer1);
    /* add second tracker */
    lighting_command_notification_add(&data, &observer2);

    /* lighting command timer subscribe */
    timer_observer1.callback = dimmer_timer_task;
    lighting_command_timer_notfication_add(&data, &timer_observer1);
    /* add again to verify skipping */
    lighting_command_timer_notfication_add(&data, &timer_observer1);
    /* add second tracker */
    lighting_command_timer_notfication_add(&data, &timer_observer2);

    /* basic STOP and NONE states */
    lighting_command_stop(&data);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    lighting_command_none(&data);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);

    /* normalized range clamp testing */
    data.Max_Actual_Value = 95.0f;
    data.Min_Actual_Value = 5.0f;
    target_level = lighting_command_normalized_range_clamp(&data, 0.1f);
    zassert_true(is_float_equal(target_level, 0.0f), NULL);
    target_level = lighting_command_normalized_range_clamp(&data, 100.0f);
    zassert_true(is_float_equal(target_level, data.Max_Actual_Value), NULL);
    target_level = lighting_command_normalized_range_clamp(&data, 1.0f);
    zassert_true(is_float_equal(target_level, data.Min_Actual_Value), NULL);
    data.Max_Actual_Value = 100.0f;
    data.Min_Actual_Value = 1.0f;

    /* fade up */
    target_level = 100.0f;
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
    /* override */
    override_level = 42.0f;
    target_level = 100.0f;
    milliseconds = 10;
    data.Overridden = true;
    data.Overridden_Momentary = false;
    lighting_command_override(&data, override_level);
    lighting_command_timer(&data, milliseconds);
    zassert_true(is_float_equal(Tracking_Value, override_level), NULL);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(is_float_equal(Tracking_Value, override_level), NULL);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    data.Overridden = false;
    lighting_command_override(&data, target_level);
    lighting_command_timer(&data, milliseconds);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    /* momentary override - self clearing flags */
    override_level = 42.0f;
    target_level = 100.0f;
    milliseconds = 10;
    data.Overridden = true;
    data.Overridden_Momentary = true;
    lighting_command_override(&data, override_level);
    lighting_command_timer(&data, milliseconds);
    zassert_true(is_float_equal(Tracking_Value, override_level), NULL);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_false(data.Overridden, NULL);
    zassert_true(data.Overridden_Momentary, NULL);
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_false(data.Overridden, NULL);
    zassert_false(data.Overridden_Momentary, NULL);

    /* step clamping */
    target_step = lighting_command_step_increment_clamp(0.0f);
    zassert_true(is_float_equal(target_step, 0.1f), NULL);
    target_step = lighting_command_step_increment_clamp(100.1f);
    zassert_true(is_float_equal(target_step, 100.0f), NULL);

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
    ramp_rate = 100.0f;
    lighting_command_ramp_to(&data, target_level, ramp_rate);
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
    ramp_rate = 1.0f;
    do {
        lighting_command_ramp_to(&data, target_level, ramp_rate);
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
    ramp_rate = 0.1f;
    do {
        lighting_command_ramp_to(&data, target_level, ramp_rate);
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
    /* large elapsed timer - ramp up */
    target_level = data.Max_Actual_Value;
    milliseconds = 2000;
    ramp_rate = 0.1f;
    do {
        lighting_command_ramp_to(&data, target_level, ramp_rate);
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

    /* out-of-service */
    target_level = 100.0f;
    milliseconds = 10;
    data.Out_Of_Service = false;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    data.Out_Of_Service = true;
    lighting_command_fade_to(&data, 0.0f, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    /* previous target level - unchanged */
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);
    target_level = 0.0f;
    data.Out_Of_Service = false;
    lighting_command_fade_to(&data, target_level, 0);
    lighting_command_timer(&data, milliseconds);
    zassert_true(data.In_Progress == BACNET_LIGHTING_IDLE, NULL);
    zassert_true(is_float_equal(Tracking_Value, target_level), NULL);

    /* non-standard lighting operation */
    milliseconds = 10;
    data.Lighting_Operation = BACNET_LIGHTS_PROPRIETARY_MIN;
    lighting_command_timer(&data, milliseconds);
    zassert_equal(data.Lighting_Operation, BACNET_LIGHTS_PROPRIETARY_MIN, NULL);
    data.Lighting_Operation = BACNET_LIGHTS_PROPRIETARY_MAX;
    lighting_command_timer(&data, milliseconds);
    zassert_equal(data.Lighting_Operation, BACNET_LIGHTS_PROPRIETARY_MAX, NULL);

    /* null check code coverage */
    lighting_command_override(NULL, override_level);
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
