/* @file
 * @brief test BACnet Lighting Output object APIs
 * @date 2013
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/proplist.h>
#include <bacnet/basic/object/lo.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief compare two floating point values to 3 decimal places
 *
 * @param x1 - first comparison value
 * @param x2 - second comparison value
 * @return true if the value is the same to 3 decimal points
 */
static bool is_float_equal(float x1, float x2)
{
    return fabsf(x1 - x2) < 0.001f;
}

/**
 * @brief Search for a property identifier in a terminated property list
 * @param list pointer to list terminated by -1
 * @param property property identifier to find
 * @return true when found in list
 */
static bool property_list_contains(const int32_t *list, int32_t property)
{
    unsigned i;

    if (!list) {
        return false;
    }
    for (i = 0; list[i] != -1; i++) {
        if (list[i] == property) {
            return true;
        }
    }

    return false;
}

static float Test_Tracking_Value;
static uint32_t Test_Lighting_Command_Instance;
static BACNET_LIGHTING_OPERATION Test_Lighting_Command_Operation;
static float Test_Lighting_Command_Target_Value;
static float Test_Lighting_Command_Modifier_Value;
static bool Test_Lighting_Command_Called;

/**
 * @brief Callback for tracking value updates
 * @param  key - key used to link to specific light
 * @param  old_value - value prior to write
 * @param  value - value of the write
 */
static void lighting_command_tracking_value_observer(
    uint32_t key, float old_value, float value)
{
    (void)key;
    (void)old_value;
    Test_Tracking_Value = value;
}

/**
 * @brief Callback for lighting command events
 * @param  object_instance - object instance
 * @param  operation - lighting operation
 * @param  target_value - operation target value
 * @param  modifier_value - operation modifier value
 */
static void lighting_command_event_observer(
    uint32_t object_instance,
    BACNET_LIGHTING_OPERATION operation,
    float target_value,
    float modifier_value)
{
    Test_Lighting_Command_Called = true;
    Test_Lighting_Command_Instance = object_instance;
    Test_Lighting_Command_Operation = operation;
    Test_Lighting_Command_Target_Value = target_value;
    Test_Lighting_Command_Modifier_Value = modifier_value;
}

/**
 * @brief Test writable property list API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testLightingOutputWritablePropertyList)
#else
static void testLightingOutputWritablePropertyList(void)
#endif
{
    const int32_t *properties = NULL;

    Lighting_Output_Writable_Property_List(1, NULL);
    Lighting_Output_Writable_Property_List(1, &properties);
    zassert_not_null(properties, NULL);
    zassert_true(
        property_list_contains(properties, PROP_PRESENT_VALUE),
        "missing PROP_PRESENT_VALUE");
    zassert_true(
        property_list_contains(properties, PROP_LIGHTING_COMMAND),
        "missing PROP_LIGHTING_COMMAND");
    zassert_true(
        property_list_contains(properties, PROP_EGRESS_TIME),
        "missing PROP_EGRESS_TIME");
    zassert_true(
        property_list_contains(properties, PROP_TRIM_FADE_TIME),
        "missing PROP_TRIM_FADE_TIME");
    zassert_true(
        property_list_contains(
            properties, PROP_LIGHTING_COMMAND_DEFAULT_PRIORITY),
        "missing PROP_LIGHTING_COMMAND_DEFAULT_PRIORITY");
}

/**
 * @brief Test delayed WARN_OFF path that triggers blink-stop callback
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testLightingOutputBlinkStop)
#else
static void testLightingOutputBlinkStop(void)
#endif
{
    const uint32_t instance = 321;
    const unsigned priority = 8;
    BACNET_LIGHTING_IN_PROGRESS in_progress;
    float test_real;
    bool status;

    Lighting_Output_Init();
    Lighting_Output_Create(instance);
    status = Lighting_Output_Blink_Warn_Enable_Set(instance, true);
    zassert_true(status, NULL);
    status = Lighting_Output_Egress_Time_Set(instance, 1);
    zassert_true(status, NULL);
    status =
        Lighting_Output_Blink_Warn_Feature_Set(instance, 0.0f, 0, UINT16_MAX);
    zassert_true(status, NULL);
    status = Lighting_Output_Present_Value_Set(instance, 75.0f, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    test_real = Lighting_Output_Present_Value(instance);
    zassert_true(is_float_equal(test_real, 75.0f), NULL);

    status = Lighting_Output_Present_Value_Set(
        instance, BACNET_LIGHTING_SPECIAL_VALUE_WARN_OFF, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 500);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(in_progress, BACNET_LIGHTING_OTHER, NULL);
    test_real = Lighting_Output_Present_Value(instance);
    zassert_true(is_float_equal(test_real, 75.0f), NULL);

    Lighting_Output_Timer(instance, 500);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(in_progress, BACNET_LIGHTING_IDLE, NULL);
    test_real = Lighting_Output_Present_Value(instance);
    zassert_true(is_float_equal(test_real, 0.0f), NULL);
    test_real = Lighting_Output_Priority_Array_Value(instance, priority);
    zassert_true(is_float_equal(test_real, 0.0f), NULL);
    status = Lighting_Output_Priority_Array_Relinquished(instance, priority);
    zassert_false(status, NULL);

    status = Lighting_Output_Delete(instance);
    zassert_true(status, NULL);
    Lighting_Output_Cleanup();
}

/**
 * @brief Test WARN_RELINQUISH behavior with egress-time configured
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testLightingOutputWarnRelinquishEgress)
#else
static void testLightingOutputWarnRelinquishEgress(void)
#endif
{
    const uint32_t instance = 322;
    const unsigned priority = 8;
    BACNET_LIGHTING_IN_PROGRESS in_progress;
    float test_real;
    bool status;

    Lighting_Output_Init();
    Lighting_Output_Create(instance);
    status = Lighting_Output_Blink_Warn_Enable_Set(instance, true);
    zassert_true(status, NULL);
    status = Lighting_Output_Egress_Time_Set(instance, 1);
    zassert_true(status, NULL);
    status =
        Lighting_Output_Blink_Warn_Feature_Set(instance, 0.0f, 0, UINT16_MAX);
    zassert_true(status, NULL);

    status = Lighting_Output_Present_Value_Set(instance, 60.0f, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    test_real = Lighting_Output_Present_Value(instance);
    zassert_true(is_float_equal(test_real, 60.0f), NULL);
    status = Lighting_Output_Priority_Array_Relinquished(instance, priority);
    zassert_false(status, NULL);

    status = Lighting_Output_Present_Value_Set(
        instance, BACNET_LIGHTING_SPECIAL_VALUE_WARN_RELINQUISH, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 500);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(in_progress, BACNET_LIGHTING_IDLE, NULL);
    status = Lighting_Output_Priority_Array_Relinquished(instance, priority);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Present_Value(instance);
    zassert_true(is_float_equal(test_real, 0.0f), NULL);

    status = Lighting_Output_Delete(instance);
    zassert_true(status, NULL);
    Lighting_Output_Cleanup();
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testLightingOutput)
#else
static void testLightingOutput(void)
#endif
{
    const uint32_t instance = 123;
    bool status = false;
    unsigned index, count;
    uint16_t milliseconds = 10;
    uint32_t test_instance;
    const char *test_name = NULL;
    char *sample_name = "sample";
    BACNET_LIGHTING_COMMAND lighting_command = { 0 };
    BACNET_LIGHTING_IN_PROGRESS in_progress;
    float real_value, test_real;
    uint32_t unsigned_value, test_unsigned;
    unsigned priority, test_priority;
    BACNET_OBJECT_ID object_id, test_object_id;
    const int32_t skip_fail_property_list[] = { -1 };

    Lighting_Output_Init();
    Lighting_Output_Create(instance);
    status = Lighting_Output_Valid_Instance(instance);
    zassert_true(status, NULL);
    status = Lighting_Output_Valid_Instance(BACNET_MAX_INSTANCE);
    zassert_false(status, NULL);
    index = Lighting_Output_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    count = Lighting_Output_Count();
    zassert_equal(count, 1, NULL);
    test_instance = Lighting_Output_Index_To_Instance(0);
    zassert_equal(test_instance, instance, NULL);
    /* set the present-value */
    priority = BACNET_MAX_PRIORITY;
    status = Lighting_Output_Present_Value_Set(instance, 100.0f, priority);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Present_Value(instance);
    zassert_true(is_float_equal(test_real, 100.0f), NULL);
    test_priority = Lighting_Output_Present_Value_Priority(instance);
    zassert_equal(priority, test_priority, NULL);
    test_real = Lighting_Output_Priority_Array_Value(instance, priority);
    zassert_true(is_float_equal(test_real, 100.0f), NULL);
    status = Lighting_Output_Priority_Array_Relinquished(instance, priority);
    zassert_false(status, NULL);
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_LIGHTING_OUTPUT, instance, Lighting_Output_Property_Lists,
        Lighting_Output_Read_Property, Lighting_Output_Write_Property,
        skip_fail_property_list);
    /* Relinquish the present-value */
    status = Lighting_Output_Present_Value_Relinquish(instance, priority);
    zassert_true(status, NULL);
    status = Lighting_Output_Priority_Array_Relinquished(instance, priority);
    zassert_true(status, NULL);
    /* check the dimming/ramping/stepping engine*/
    Lighting_Output_Timer(instance, milliseconds);
    /* test the ASCII name get/set */
    status = Lighting_Output_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Lighting_Output_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Lighting_Output_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Lighting_Output_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);
    /* test the ASCII description get/set */
    status = Lighting_Output_Description_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Lighting_Output_Description(instance);
    zassert_equal(test_name, sample_name, NULL);
    /* test local control API */
    lighting_command.operation = BACNET_LIGHTS_NONE;
    /* configure all the lighting command parameters */
    lighting_command.use_fade_time = true;
    lighting_command.fade_time = 1;
    lighting_command.use_ramp_rate = true;
    lighting_command.ramp_rate = 10.0f;
    lighting_command.use_step_increment = true;
    lighting_command.step_increment = 10.0f;
    lighting_command.use_target_level = true;
    lighting_command.target_level = 50.0f;
    lighting_command.use_priority = true;
    lighting_command.priority = 8;
    do {
        status =
            Lighting_Output_Lighting_Command_Set(instance, &lighting_command);
        if (status) {
            zassert_true(status, NULL);
        } else {
            printf(
                "lighting-command operation[%d] %s not supported.\n",
                lighting_command.operation,
                bactext_lighting_operation_name(lighting_command.operation));
        }
        if (lighting_command.operation == BACNET_LIGHTS_PROPRIETARY_MIN) {
            /* skip to make testing faster */
            lighting_command.operation = BACNET_LIGHTS_PROPRIETARY_MAX;
        } else if (lighting_command.operation == BACNET_LIGHTS_RESERVED_MIN) {
            /* skip to make testing faster */
            lighting_command.operation = BACNET_LIGHTS_RESERVED_MAX;
        } else {
            lighting_command.operation++;
        }
    } while (lighting_command.operation <= BACNET_LIGHTS_PROPRIETARY_MAX);
    /* test the present-value APIs */
    Lighting_Output_Present_Value_Relinquish_All(instance);
    real_value = 1.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    test_real = Lighting_Output_Present_Value(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    test_priority = Lighting_Output_Present_Value_Priority(instance);
    zassert_equal(
        priority, test_priority, "priority=%u test_priority=%u", priority,
        test_priority);
    Lighting_Output_Present_Value_Relinquish(instance, priority);
    real_value = Lighting_Output_Relinquish_Default(instance);
    test_real = Lighting_Output_Present_Value(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    /* test the present-value special values */
    status = Lighting_Output_Default_Fade_Time_Set(instance, 100);
    zassert_true(status, NULL);
    status = Lighting_Output_Default_Ramp_Rate_Set(instance, 100.0f);
    zassert_true(status, NULL);
    status = Lighting_Output_Egress_Time_Set(instance, 0);
    zassert_true(status, NULL);
    status = Lighting_Output_Default_Step_Increment_Set(instance, 1.0f);
    zassert_true(status, NULL);
    status = Lighting_Output_Transition_Set(
        instance, BACNET_LIGHTING_TRANSITION_NONE);
    zassert_true(status, NULL);
    /* WARN - start with lights on  */
    real_value = -1.0;
    priority = 8;
    status = Lighting_Output_Blink_Warn_Enable_Set(instance, true);
    zassert_true(status, NULL);
    status = Lighting_Output_Present_Value_Set(instance, 100.0f, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    status = Lighting_Output_Present_Value_Set(instance, real_value, priority);
    zassert_true(status, NULL);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_OTHER, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* WARN RELINQUISH */
    real_value = -2.0;
    priority = 8;
    status = Lighting_Output_Blink_Warn_Enable(instance);
    zassert_true(status, NULL);
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* WARN_OFF */
    real_value = -3.0;
    priority = 8;
    status = Lighting_Output_Blink_Warn_Enable(instance);
    zassert_true(status, NULL);
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* RESTORE_ON */
    real_value = -4.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* DEFAULT_ON */
    real_value = -5.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* TOGGLE_RESTORE */
    real_value = -6.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* twice for toggle */
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* TOGGLE_DEFAULT */
    real_value = -7.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* twice for toggle */
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* test the in-progress set API */
    status = Lighting_Output_In_Progress_Set(
        instance, BACNET_LIGHTING_NOT_CONTROLLED);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_NOT_CONTROLLED, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* tracking-value */
    real_value = 100.0f;
    status = Lighting_Output_Tracking_Value_Set(instance, real_value);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    /* blink-warn features */
    status = Lighting_Output_Blink_Warn_Feature_Set(instance, 0.0f, 0, 65535);
    zassert_true(status, NULL);
    status = Lighting_Output_Blink_Warn_Feature_Set(instance, -1.0f, 0, 65535);
    zassert_true(status, NULL);
    status = Lighting_Output_Blink_Warn_Feature_Set(instance, 101.0f, 0, 65535);
    zassert_true(status, NULL);
    /* egress-time */
    unsigned_value = 5 * 60;
    status = Lighting_Output_Egress_Time_Set(instance, unsigned_value);
    zassert_true(status, NULL);
    test_unsigned = Lighting_Output_Egress_Time(instance);
    zassert_equal(unsigned_value, test_unsigned, NULL);
    /* default-fade-time */
    unsigned_value = 5 * 60 * 1000;
    status = Lighting_Output_Default_Fade_Time_Set(instance, unsigned_value);
    zassert_true(status, NULL);
    test_unsigned = Lighting_Output_Default_Fade_Time(instance);
    zassert_equal(unsigned_value, test_unsigned, NULL);
    /* default-ramp-rate */
    real_value = 1.0f;
    status = Lighting_Output_Default_Ramp_Rate_Set(instance, real_value);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Default_Ramp_Rate(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    /* default-step-increment */
    real_value = 2.0f;
    status = Lighting_Output_Default_Step_Increment_Set(instance, real_value);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Default_Step_Increment(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    /* relinquish-default */
    real_value = 0.0f;
    status = Lighting_Output_Relinquish_Default_Set(instance, real_value);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Relinquish_Default(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    /* overridden */
    real_value = 88.0f;
    status = Lighting_Output_Overridden_Set(instance, real_value);
    zassert_true(status, NULL);
    status = Lighting_Output_Overridden_Status(instance);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(
        is_float_equal(test_real, real_value), "value=%f test_value=%f",
        (double)real_value, (double)test_real);
    status = Lighting_Output_Present_Value_Set(instance, 99.0f, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(
        is_float_equal(test_real, real_value), "value=%f test_value=%f",
        (double)real_value, (double)test_real);
    real_value = Lighting_Output_Present_Value(instance);
    status = Lighting_Output_Overridden_Clear(instance);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    /* overridden-momentary */
    real_value = 77.0f;
    status = Lighting_Output_Overridden_Momentary(instance, real_value);
    zassert_true(status, NULL);
    status = Lighting_Output_Overridden_Status(instance);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(
        is_float_equal(test_real, real_value), "value=%f test_value=%f",
        (double)real_value, (double)test_real);
    status = Lighting_Output_Present_Value_Set(instance, 98.0f, priority);
    Lighting_Output_Timer(instance, 10);
    real_value = Lighting_Output_Present_Value(instance);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(
        is_float_equal(test_real, real_value), "value=%f test_value=%f",
        (double)real_value, (double)test_real);
    /* refresh */
    Lighting_Output_Lighting_Command_Refresh(instance);
    /* color-override */
    status = Lighting_Output_Color_Override_Set(instance, true);
    zassert_true(status, NULL);
    status = Lighting_Output_Color_Override(instance);
    zassert_true(status, NULL);
    status = Lighting_Output_Color_Override_Set(instance, false);
    zassert_true(status, NULL);
    status = Lighting_Output_Color_Override(instance);
    zassert_false(status, NULL);
    /* color-reference */
    object_id.instance = 1;
    object_id.type = OBJECT_COLOR;
    status = Lighting_Output_Color_Reference_Set(instance, &object_id);
    zassert_true(status, NULL);
    status = Lighting_Output_Color_Reference(instance, &test_object_id);
    zassert_true(status, NULL);
    zassert_equal(object_id.instance, test_object_id.instance, NULL);
    zassert_equal(object_id.type, test_object_id.type, NULL);
    /* override-color-reference */
    object_id.instance = 2;
    object_id.type = OBJECT_COLOR;
    status = Lighting_Output_Override_Color_Reference_Set(instance, &object_id);
    zassert_true(status, NULL);
    status =
        Lighting_Output_Override_Color_Reference(instance, &test_object_id);
    zassert_true(status, NULL);
    zassert_equal(object_id.instance, test_object_id.instance, NULL);
    zassert_equal(object_id.type, test_object_id.type, NULL);
    /* tracking-value observer */
    real_value = 95.0f;
    Lighting_Output_Write_Present_Value_Callback_Set(
        lighting_command_tracking_value_observer);
    status = Lighting_Output_Present_Value_Set(instance, real_value, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    zassert_true(is_float_equal(Test_Tracking_Value, real_value), NULL);
    /* high-end-trim, low-end-trim, and trim-fade-time */
    Lighting_Output_Present_Value_Relinquish_All(instance);
    status = Lighting_Output_Transition_Set(
        instance, BACNET_LIGHTING_TRANSITION_NONE);
    zassert_true(status, NULL);
    status = Lighting_Output_Trim_Fade_Time_Set(instance, 0);
    zassert_true(status, NULL);
    status = Lighting_Output_High_End_Trim_Set(instance, 90.0f);
    zassert_true(status, NULL);
    test_real = Lighting_Output_High_End_Trim(instance);
    zassert_true(is_float_equal(test_real, 90.0f), NULL);
    status = Lighting_Output_Low_End_Trim_Set(instance, 10.0f);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Low_End_Trim(instance);
    zassert_true(is_float_equal(test_real, 10.0f), NULL);
    priority = 8;
    status = Lighting_Output_Present_Value_Set(instance, 100.0f, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_TRIM_ACTIVE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(is_float_equal(test_real, 90.0f), NULL);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    status = Lighting_Output_Present_Value_Set(instance, 1.0f, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_TRIM_ACTIVE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(
        is_float_equal(test_real, 10.0f), "tracking=%f", (double)test_real);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    Lighting_Output_Present_Value_Relinquish_All(instance);
    status = Lighting_Output_Present_Value_Set(instance, 100.0f, 1);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(
        is_float_equal(test_real, 100.0f), "tracking=%f", (double)test_real);
    Lighting_Output_Present_Value_Relinquish_All(instance);
    status = Lighting_Output_Present_Value_Set(instance, 80.0f, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(is_float_equal(test_real, 80.0f), NULL);
    unsigned_value = 1000;
    status = Lighting_Output_Trim_Fade_Time_Set(instance, unsigned_value);
    zassert_true(status, NULL);
    test_unsigned = Lighting_Output_Trim_Fade_Time(instance);
    zassert_equal(test_unsigned, unsigned_value, NULL);
    status = Lighting_Output_Default_Fade_Time_Set(instance, 2000);
    zassert_true(status, NULL);
    status = Lighting_Output_Transition_Set(
        instance, BACNET_LIGHTING_TRANSITION_FADE);
    zassert_true(status, NULL);
    status = Lighting_Output_Present_Value_Set(instance, 100.0f, priority);
    zassert_true(status, NULL);
    milliseconds = 500;
    Lighting_Output_Timer(instance, milliseconds);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_FADE_ACTIVE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(is_float_equal(test_real, 85.0f), NULL);
    Lighting_Output_Timer(instance, milliseconds);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_FADE_ACTIVE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(is_float_equal(test_real, 90.0f), NULL);
    Lighting_Output_Timer(instance, milliseconds);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_TRIM_ACTIVE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(is_float_equal(test_real, 92.5f), NULL);
    Lighting_Output_Timer(instance, milliseconds);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_TRIM_ACTIVE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(is_float_equal(test_real, 90.0f), NULL);
    Lighting_Output_Timer(instance, milliseconds);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    /* feedback value */
    status = Lighting_Output_Feedback_Value_Set(instance, 55.5f);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Feedback_Value(instance);
    zassert_true(is_float_equal(test_real, 55.5f), NULL);
    /* power*/
    status = Lighting_Output_Power_Set(instance, 12345.67f);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Power(instance);
    zassert_true(is_float_equal(test_real, 12345.67f), NULL);
    /* instantenous power*/
    status = Lighting_Output_Instantaneous_Power_Set(instance, 76543.21f);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Instantaneous_Power(instance);
    zassert_true(is_float_equal(test_real, 76543.21f), NULL);
    /* context get/set */
    Lighting_Output_Context_Set(
        instance, Lighting_Output_Context_Get(instance));
    /* min-actual-value get/set */
    real_value = 5.0f;
    status = Lighting_Output_Min_Actual_Value_Set(instance, real_value);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Min_Actual_Value(instance);
    zassert_true(
        is_float_equal(test_real, real_value), "value=%f test=%f",
        (double)real_value, (double)test_real);
    real_value = 1.0f;
    status = Lighting_Output_Min_Actual_Value_Set(instance, real_value);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Min_Actual_Value(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    /* max-actual-value get/set */
    real_value = 95.0f;
    status = Lighting_Output_Max_Actual_Value_Set(instance, real_value);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Max_Actual_Value(instance);
    zassert_true(
        is_float_equal(test_real, real_value), "value=%f test=%f",
        (double)real_value, (double)test_real);
    real_value = 100.0f;
    status = Lighting_Output_Max_Actual_Value_Set(instance, real_value);
    zassert_true(status, NULL);
    test_real = Lighting_Output_Max_Actual_Value(instance);
    zassert_true(is_float_equal(test_real, real_value), NULL);
    /* out-of-bounds */
    test_instance = Lighting_Output_Create(BACNET_MAX_INSTANCE + 1);
    zassert_equal(test_instance, BACNET_MAX_INSTANCE, NULL);
    /* check the delete function */
    status = Lighting_Output_Delete(instance);
    zassert_true(status, NULL);
    test_instance = Lighting_Output_Create(BACNET_MAX_INSTANCE);
    zassert_equal(test_instance, 1, NULL);
    Lighting_Output_Cleanup();

    return;
}
/**
 * @brief Test boundary and relationship behavior for Min/Max Actual Value
 *
 * Verifies:
 *  - values outside 1.0..100.0 are rejected
 *  - exact boundary values 1.0 and 100.0 are accepted
 *  - setting Min above Max clamps Min down to the current Max value
 *  - setting Max below Min forces Min down to the new Max value
 *  - calls on a non-existent instance fail gracefully
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testLightingOutputMinMaxActualValue)
#else
static void testLightingOutputMinMaxActualValue(void)
#endif
{
    const uint32_t instance = 400;
    bool status;
    float test_real;

    Lighting_Output_Init();
    Lighting_Output_Create(instance);

    /* --- out-of-range rejection for Min_Actual_Value --- */
    status = Lighting_Output_Min_Actual_Value_Set(instance, 0.0f);
    zassert_false(status, "Min=0.0 should be rejected (below 1.0)");
    status = Lighting_Output_Min_Actual_Value_Set(instance, -1.0f);
    zassert_false(status, "Min=-1.0 should be rejected");
    status = Lighting_Output_Min_Actual_Value_Set(instance, 100.1f);
    zassert_false(status, "Min=100.1 should be rejected (above 100.0)");
    status = Lighting_Output_Min_Actual_Value_Set(instance, 200.0f);
    zassert_false(status, "Min=200.0 should be rejected");

    /* --- out-of-range rejection for Max_Actual_Value --- */
    status = Lighting_Output_Max_Actual_Value_Set(instance, 0.0f);
    zassert_false(status, "Max=0.0 should be rejected (below 1.0)");
    status = Lighting_Output_Max_Actual_Value_Set(instance, -1.0f);
    zassert_false(status, "Max=-1.0 should be rejected");
    status = Lighting_Output_Max_Actual_Value_Set(instance, 100.1f);
    zassert_false(status, "Max=100.1 should be rejected (above 100.0)");
    status = Lighting_Output_Max_Actual_Value_Set(instance, 200.0f);
    zassert_false(status, "Max=200.0 should be rejected");

    /* --- exact boundary values must be accepted --- */
    status = Lighting_Output_Min_Actual_Value_Set(instance, 1.0f);
    zassert_true(status, "Min=1.0 (lower bound) should be accepted");
    status = Lighting_Output_Max_Actual_Value_Set(instance, 100.0f);
    zassert_true(status, "Max=100.0 (upper bound) should be accepted");

    /* --- relationship invariant: Min > Max clamps Min down to Max ---
     * Per the implementation: when the requested Min exceeds the current Max,
     * Min is clamped to the current Max value (Max is unchanged). */
    status = Lighting_Output_Max_Actual_Value_Set(instance, 50.0f);
    zassert_true(status, NULL);
    status = Lighting_Output_Min_Actual_Value_Set(instance, 1.0f);
    zassert_true(status, NULL);
    /* request Min=80 with Max=50: call succeeds, Min stored as 50 */
    status = Lighting_Output_Min_Actual_Value_Set(instance, 80.0f);
    zassert_true(status, "Min=80 > Max=50 should succeed (clamped)");
    test_real = Lighting_Output_Min_Actual_Value(instance);
    zassert_true(
        is_float_equal(test_real, 50.0f),
        "Min should be clamped to Max(50.0), got %f", (double)test_real);
    test_real = Lighting_Output_Max_Actual_Value(instance);
    zassert_true(
        is_float_equal(test_real, 50.0f), "Max should remain 50.0, got %f",
        (double)test_real);

    /* --- relationship invariant: Max < Min forces Min down to new Max ---
     * Per the implementation: when the requested Max is below the current Min,
     * Min is set to the new Max and then Max is set to the new Max. */
    status = Lighting_Output_Max_Actual_Value_Set(instance, 80.0f);
    zassert_true(status, NULL);
    status = Lighting_Output_Min_Actual_Value_Set(instance, 60.0f);
    zassert_true(status, NULL);
    /* request Max=30 with Min=60: call succeeds, both become 30 */
    status = Lighting_Output_Max_Actual_Value_Set(instance, 30.0f);
    zassert_true(status, "Max=30 < Min=60 should succeed (forces Min down)");
    test_real = Lighting_Output_Max_Actual_Value(instance);
    zassert_true(
        is_float_equal(test_real, 30.0f), "Max should be 30.0, got %f",
        (double)test_real);
    test_real = Lighting_Output_Min_Actual_Value(instance);
    zassert_true(
        is_float_equal(test_real, 30.0f),
        "Min should be forced to new Max(30.0), got %f", (double)test_real);

    /* --- non-existent instance fails gracefully --- */
    status = Lighting_Output_Min_Actual_Value_Set(instance + 1, 50.0f);
    zassert_false(status, "Min set on non-existent instance should fail");
    status = Lighting_Output_Max_Actual_Value_Set(instance + 1, 50.0f);
    zassert_false(status, "Max set on non-existent instance should fail");
    test_real = Lighting_Output_Min_Actual_Value(instance + 1);
    zassert_true(
        is_float_equal(test_real, 0.0f),
        "Min get on non-existent instance should return 0.0");
    test_real = Lighting_Output_Max_Actual_Value(instance + 1);
    zassert_true(
        is_float_equal(test_real, 0.0f),
        "Max get on non-existent instance should return 0.0");

    status = Lighting_Output_Delete(instance);
    zassert_true(status, NULL);
    Lighting_Output_Cleanup();
}

/**
 * @brief Test lighting command callback API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testLightingOutputCommandCallback)
#else
static void testLightingOutputCommandCallback(void)
#endif
{
    const uint32_t instance = 500;
    BACNET_LIGHTING_COMMAND lighting_command = { 0 };
    bool status = false;
    unsigned priority = 8;

    Lighting_Output_Init();
    Lighting_Output_Create(instance);

    /* Register callback */
    Lighting_Output_Write_Lighting_Command_Callback_Set(
        lighting_command_event_observer);

    /* Initialize test tracking */
    Test_Lighting_Command_Called = false;
    Test_Lighting_Command_Instance = 0;
    Test_Lighting_Command_Operation = BACNET_LIGHTS_NONE;
    Test_Lighting_Command_Target_Value = 0.0f;
    Test_Lighting_Command_Modifier_Value = 0.0f;

    /* Test fade command */
    lighting_command.operation = BACNET_LIGHTS_FADE_TO;
    lighting_command.use_fade_time = true;
    lighting_command.fade_time = 100;
    lighting_command.use_target_level = true;
    lighting_command.target_level = 50.0f;
    lighting_command.use_priority = true;
    lighting_command.priority = priority;

    status = Lighting_Output_Lighting_Command_Set(instance, &lighting_command);
    zassert_true(status, "Fade command should succeed");
    zassert_true(Test_Lighting_Command_Called, "Callback should be called");
    zassert_equal(Test_Lighting_Command_Instance, instance, NULL);
    zassert_equal(Test_Lighting_Command_Operation, BACNET_LIGHTS_FADE_TO, NULL);
    zassert_true(
        is_float_equal(Test_Lighting_Command_Target_Value, 50.0f),
        "Target value should be 50.0");
    zassert_true(
        is_float_equal(Test_Lighting_Command_Modifier_Value, 100.0f),
        "Modifier (fade time) should be 100.0");

    /* Test ramp command */
    Test_Lighting_Command_Called = false;
    lighting_command.operation = BACNET_LIGHTS_RAMP_TO;
    lighting_command.use_ramp_rate = true;
    lighting_command.ramp_rate = 10.0f;
    lighting_command.target_level = 75.0f;

    status = Lighting_Output_Lighting_Command_Set(instance, &lighting_command);
    zassert_true(status, "Ramp command should succeed");
    zassert_true(Test_Lighting_Command_Called, "Callback should be called");
    zassert_equal(Test_Lighting_Command_Operation, BACNET_LIGHTS_RAMP_TO, NULL);
    zassert_true(
        is_float_equal(Test_Lighting_Command_Target_Value, 75.0f),
        "Target value should be 75.0");
    zassert_true(
        is_float_equal(Test_Lighting_Command_Modifier_Value, 10.0f),
        "Modifier (ramp rate) should be 10.0");

    /* Test step command */
    Test_Lighting_Command_Called = false;
    lighting_command.operation = BACNET_LIGHTS_STEP_ON;
    lighting_command.use_step_increment = true;
    lighting_command.step_increment = 5.0f;

    status = Lighting_Output_Lighting_Command_Set(instance, &lighting_command);
    zassert_true(status, "Step command should succeed");
    zassert_true(Test_Lighting_Command_Called, "Callback should be called");
    zassert_equal(Test_Lighting_Command_Operation, BACNET_LIGHTS_STEP_ON, NULL);
    zassert_true(
        is_float_equal(Test_Lighting_Command_Target_Value, 5.0f),
        "Target (step increment) should be 5.0");
    zassert_true(
        is_float_equal(Test_Lighting_Command_Modifier_Value, 0.0f),
        "Modifier (fade time) should be 0.0 for step commands");

    /* Cleanup */
    status = Lighting_Output_Delete(instance);
    zassert_true(status, NULL);
    Lighting_Output_Cleanup();
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(lo_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        lo_tests, ztest_unit_test(testLightingOutput),
        ztest_unit_test(testLightingOutputWritablePropertyList),
        ztest_unit_test(testLightingOutputBlinkStop),
        ztest_unit_test(testLightingOutputWarnRelinquishEgress),
        ztest_unit_test(testLightingOutputCommandCallback),
        ztest_unit_test(testLightingOutputMinMaxActualValue));

    ztest_run_test_suite(lo_tests);
}
#endif
