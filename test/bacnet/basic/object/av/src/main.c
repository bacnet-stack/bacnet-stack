/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @section LICENSE
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <float.h>
#include <math.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/object/av.h>
#include <bacnet/proplist.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(av_tests, testAnalog_Value)
#else
static void testAnalog_Value(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };

    Analog_Value_Init();
    object_instance = Analog_Value_Create(object_instance);
    count = Analog_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Analog_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_ANALOG_VALUE, object_instance, Analog_Value_Property_Lists,
        Analog_Value_Read_Property, Analog_Value_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Analog_Value_Name_Set, Analog_Value_Name_ASCII);
    status = Analog_Value_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test direct Analog Value property APIs
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(av_tests, testAnalog_Value_APIs)
#else
static void testAnalog_Value_APIs(void)
#endif
{
    const uint32_t instance = 123;
    const uint32_t invalid_instance = instance + 1;
    const char *sample_description = "Test Analog Value";
    char sample_context[] = "context";
    bool status = false;
    uint32_t test_instance = BACNET_MAX_INSTANCE;

    Analog_Value_Init();
    test_instance = Analog_Value_Create(instance);
    zassert_equal(test_instance, instance, NULL);
    zassert_true(Analog_Value_Valid_Instance(instance), NULL);
    zassert_true(Analog_Value_Write_Enabled(instance), NULL);
    zassert_false(Analog_Value_Out_Of_Service(instance), NULL);
    zassert_equal(Analog_Value_Event_State(instance), EVENT_STATE_NORMAL, NULL);
    zassert_true(
        fabsf(Analog_Value_Present_Value(instance)) < FLT_EPSILON, NULL);
    zassert_true(
        fabsf(Analog_Value_COV_Increment(instance) - 1.0f) < FLT_EPSILON, NULL);
    zassert_equal(Analog_Value_Units(instance), UNITS_PERCENT, NULL);
    zassert_equal(
        Analog_Value_Reliability(instance), RELIABILITY_NO_FAULT_DETECTED,
        NULL);
    zassert_true(Analog_Value_Min_Pres_Value(instance) < -3.0e38f, NULL);
    zassert_true(Analog_Value_Max_Pres_Value(instance) > 3.0e38f, NULL);
    zassert_is_null(Analog_Value_Description(instance), NULL);
    zassert_is_null(Analog_Value_Context_Get(instance), NULL);

    status = Analog_Value_Description_Set(instance, sample_description);
    zassert_true(status, NULL);
    zassert_equal(
        strcmp(Analog_Value_Description(instance), sample_description), 0,
        NULL);
    status = Analog_Value_Description_Set(invalid_instance, sample_description);
    zassert_false(status, NULL);

    status = Analog_Value_Reliability_Set(instance, RELIABILITY_PROCESS_ERROR);
    zassert_true(status, NULL);
    zassert_equal(
        Analog_Value_Reliability(instance), RELIABILITY_PROCESS_ERROR, NULL);
    zassert_true(Analog_Value_Change_Of_Value(instance), NULL);
    Analog_Value_Change_Of_Value_Clear(instance);
    status = Analog_Value_Reliability_Set(instance, RELIABILITY_PROCESS_ERROR);
    zassert_true(status, NULL);
    zassert_false(Analog_Value_Change_Of_Value(instance), NULL);
    status =
        Analog_Value_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);
    zassert_true(status, NULL);
    zassert_true(Analog_Value_Change_Of_Value(instance), NULL);
    status = Analog_Value_Reliability_Set(
        invalid_instance, RELIABILITY_PROCESS_ERROR);
    zassert_false(status, NULL);

    status = Analog_Value_Min_Pres_Value_Set(instance, -5.0f);
    zassert_true(status, NULL);
    zassert_true(
        fabsf(Analog_Value_Min_Pres_Value(instance) - (-5.0f)) < FLT_EPSILON,
        NULL);
    status = Analog_Value_Max_Pres_Value_Set(instance, 55.0f);
    zassert_true(status, NULL);
    zassert_true(
        fabsf(Analog_Value_Max_Pres_Value(instance) - 55.0f) < FLT_EPSILON,
        NULL);
    status = Analog_Value_Present_Value_Set(instance, 42.5f, 8);
    zassert_true(status, NULL);
    zassert_true(
        fabsf(Analog_Value_Present_Value(instance) - 42.5f) < FLT_EPSILON,
        NULL);

    Analog_Value_COV_Increment_Set(instance, 2.5f);
    zassert_true(
        fabsf(Analog_Value_COV_Increment(instance) - 2.5f) < FLT_EPSILON, NULL);
    status = Analog_Value_Units_Set(instance, UNITS_VOLTS);
    zassert_true(status, NULL);
    zassert_equal(Analog_Value_Units(instance), UNITS_VOLTS, NULL);
    status = Analog_Value_Event_State_Set(instance, EVENT_STATE_HIGH_LIMIT);
    zassert_true(status, NULL);
    zassert_equal(
        Analog_Value_Event_State(instance), EVENT_STATE_HIGH_LIMIT, NULL);

    Analog_Value_Write_Disable(instance);
    zassert_false(Analog_Value_Write_Enabled(instance), NULL);
    Analog_Value_Write_Enable(instance);
    zassert_true(Analog_Value_Write_Enabled(instance), NULL);

    Analog_Value_Change_Of_Value_Clear(instance);
    Analog_Value_Out_Of_Service_Set(instance, true);
    zassert_true(Analog_Value_Out_Of_Service(instance), NULL);
    zassert_true(Analog_Value_Change_Of_Value(instance), NULL);
    Analog_Value_Context_Set(instance, (void *)sample_context);
    zassert_equal(Analog_Value_Context_Get(instance), sample_context, NULL);
    zassert_is_null(Analog_Value_Context_Get(invalid_instance), NULL);

#if defined(INTRINSIC_REPORTING)
    status = Analog_Value_Time_Delay_Set(instance, 5);
    zassert_true(status, NULL);
    zassert_equal(Analog_Value_Time_Delay(instance), 5, NULL);
    status = Analog_Value_Notification_Class_Set(instance, 7);
    zassert_true(status, NULL);
    zassert_equal(Analog_Value_Notification_Class(instance), 7, NULL);
    status = Analog_Value_High_Limit_Set(instance, 90.0f);
    zassert_true(status, NULL);
    zassert_true(
        fabsf(Analog_Value_High_Limit(instance) - 90.0f) < FLT_EPSILON, NULL);
    status = Analog_Value_Low_Limit_Set(instance, 10.0f);
    zassert_true(status, NULL);
    zassert_true(
        fabsf(Analog_Value_Low_Limit(instance) - 10.0f) < FLT_EPSILON, NULL);
    status = Analog_Value_Deadband_Set(instance, 1.5f);
    zassert_true(status, NULL);
    zassert_true(
        fabsf(Analog_Value_Deadband(instance) - 1.5f) < FLT_EPSILON, NULL);
    status = Analog_Value_Limit_Enable_Set(
        instance, EVENT_LOW_LIMIT_ENABLE | EVENT_HIGH_LIMIT_ENABLE);
    zassert_true(status, NULL);
    zassert_equal(
        Analog_Value_Limit_Enable(instance),
        EVENT_LOW_LIMIT_ENABLE | EVENT_HIGH_LIMIT_ENABLE, NULL);
    status = Analog_Value_Limit_Enable_Set(instance, (BACNET_LIMIT_ENABLE)0x04);
    zassert_false(status, NULL);
    status = Analog_Value_Event_Enable_Set(
        instance, EVENT_ENABLE_TO_OFFNORMAL | EVENT_ENABLE_TO_NORMAL);
    zassert_true(status, NULL);
    zassert_equal(
        Analog_Value_Event_Enable(instance),
        EVENT_ENABLE_TO_OFFNORMAL | EVENT_ENABLE_TO_NORMAL, NULL);
    status = Analog_Value_Event_Enable_Set(instance, (BACNET_EVENT_ENABLE)0x08);
    zassert_false(status, NULL);
    status = Analog_Value_Event_Detection_Enable_Set(instance, false);
    zassert_true(status, NULL);
    zassert_false(Analog_Value_Event_Detection_Enable(instance), NULL);
    status = Analog_Value_Notify_Type_Set(instance, NOTIFY_ALARM);
    zassert_true(status, NULL);
    zassert_equal(Analog_Value_Notify_Type(instance), NOTIFY_ALARM, NULL);
    status = Analog_Value_Notify_Type_Set(instance, (BACNET_NOTIFY_TYPE)42);
    zassert_false(status, NULL);
#endif

    status = Analog_Value_Delete(instance);
    zassert_true(status, NULL);
    zassert_false(Analog_Value_Valid_Instance(instance), NULL);
    Analog_Value_Cleanup();
}

/**
 * @brief Test Analog Value Writable_Property_List API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(av_tests, testAnalog_Value_Writable_Properties)
#else
static void testAnalog_Value_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Analog_Value_Init();
    zassert_not_equal(Analog_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* write-enabled (default): list starts with PROP_PRESENT_VALUE */
    zassert_true(Analog_Value_Write_Enabled(instance), NULL);
    Analog_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write-disabled: list skips PROP_PRESENT_VALUE */
    Analog_Value_Write_Disable(instance);
    zassert_false(Analog_Value_Write_Enabled(instance), NULL);
    Analog_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_not_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    Analog_Value_Write_Enable(instance);
    zassert_true(Analog_Value_Write_Enabled(instance), NULL);
    Analog_Value_Writable_Property_List(instance, &properties);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* unknown instance: must return a valid list, not NULL/garbage */
    properties = NULL;
    Analog_Value_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* NULL properties pointer: must not crash */
    Analog_Value_Writable_Property_List(instance, NULL);

    Analog_Value_Delete(instance);
    Analog_Value_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(av_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        av_tests, ztest_unit_test(testAnalog_Value),
        ztest_unit_test(testAnalog_Value_APIs),
        ztest_unit_test(testAnalog_Value_Writable_Properties));

    ztest_run_test_suite(av_tests);
}
#endif
