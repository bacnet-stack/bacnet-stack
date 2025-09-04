/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * @copyright SPDX-License-Identifier: MIT
 */

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
    return fabs(x1 - x2) < 0.001;
}

static float Test_Tracking_Value;
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
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testLightingOutput)
#else
static void testLightingOutput(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    const uint32_t instance = 123;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
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

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_LIGHTING_OUTPUT;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Lighting_Output_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) >= 0) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Lighting_Output_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            if (rpdata.object_property != PROP_PRIORITY_ARRAY) {
                zassert_equal(
                    len, test_len, "property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Lighting_Output_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
            if (property_list_commandable_member(
                    wpdata.object_type, wpdata.object_property)) {
                wpdata.priority = 16;
                status = Lighting_Output_Write_Property(&wpdata);
                zassert_true(status, NULL);
                wpdata.application_data_len =
                    encode_application_null(wpdata.application_data);
                wpdata.priority = 16;
                status = Lighting_Output_Write_Property(&wpdata);
                zassert_true(status, NULL);
                wpdata.priority = 6;
                status = Lighting_Output_Write_Property(&wpdata);
                zassert_false(status, NULL);
                zassert_equal(
                    wpdata.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);
                wpdata.priority = 0;
                status = Lighting_Output_Write_Property(&wpdata);
                zassert_false(status, NULL);
                zassert_equal(
                    wpdata.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
            }
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Lighting_Output_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            zassert_equal(
                len, test_len, "property '%s': failed to decode!\n",
                bactext_property_name(rpdata.object_property));
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Lighting_Output_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        }
        pOptional++;
    }
    /* check for unsupported property - use ALL */
    rpdata.object_property = PROP_ALL;
    len = Lighting_Output_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    wpdata.object_property = PROP_ALL;
    status = Lighting_Output_Write_Property(&wpdata);
    zassert_false(status, NULL);
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
    real_value = -1.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    real_value = -2.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    real_value = -3.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    real_value = -4.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    real_value = -5.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    real_value = -6.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
    real_value = -7.0;
    priority = 8;
    Lighting_Output_Present_Value_Set(instance, real_value, priority);
    Lighting_Output_Timer(instance, 10);
    in_progress = Lighting_Output_In_Progress(instance);
    zassert_equal(
        in_progress, BACNET_LIGHTING_IDLE, "in_progress=%s",
        bactext_lighting_in_progress(in_progress));
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
        real_value, test_real);
    status = Lighting_Output_Present_Value_Set(instance, 99.0f, priority);
    zassert_true(status, NULL);
    Lighting_Output_Timer(instance, 10);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(
        is_float_equal(test_real, real_value), "value=%f test_value=%f",
        real_value, test_real);
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
        real_value, test_real);
    status = Lighting_Output_Present_Value_Set(instance, 98.0f, priority);
    Lighting_Output_Timer(instance, 10);
    real_value = Lighting_Output_Present_Value(instance);
    test_real = Lighting_Output_Tracking_Value(instance);
    zassert_true(
        is_float_equal(test_real, real_value), "value=%f test_value=%f",
        real_value, test_real);
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
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(lo_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(lo_tests, ztest_unit_test(testLightingOutput));

    ztest_run_test_suite(lo_tests);
}
#endif
