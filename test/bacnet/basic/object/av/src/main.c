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
#include <bacnet/bacapp.h>
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
    const int32_t skip_fail_property_list[] = { PROP_PRIORITY_ARRAY,
                                                PROP_CURRENT_COMMAND_PRIORITY,
                                                -1 };

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
    zassert_equal(strcmp(Analog_Value_Description(instance), ""), 0, NULL);
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
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* write-disabled: list skips PROP_PRESENT_VALUE */
    Analog_Value_Write_Disable(instance);
    zassert_false(Analog_Value_Write_Enabled(instance), NULL);
    Analog_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_false(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    Analog_Value_Write_Enable(instance);
    zassert_true(Analog_Value_Write_Enabled(instance), NULL);
    Analog_Value_Writable_Property_List(instance, &properties);
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

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
#if defined(BACNET_OBJECT_ANALOG_VALUE_COMMANDABLE)
static uint32_t Test_Callback_Count;
static float Test_Callback_Value;

static void
test_av_write_callback(uint32_t object_instance, float old_value, float value)
{
    (void)object_instance;
    (void)old_value;
    Test_Callback_Count++;
    Test_Callback_Value = value;
}

static bool test_av_write(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    uint32_t instance,
    BACNET_PROPERTY_ID property,
    const BACNET_APPLICATION_DATA_VALUE *value,
    uint8_t priority)
{
    memset(wp_data, 0, sizeof(*wp_data));
    wp_data->object_type = OBJECT_ANALOG_VALUE;
    wp_data->object_instance = instance;
    wp_data->object_property = property;
    wp_data->array_index = BACNET_ARRAY_ALL;
    wp_data->priority = priority;
    wp_data->application_data_len =
        bacapp_encode_application_data(wp_data->application_data, value);
    return Analog_Value_Write_Property(wp_data);
}

static int test_av_read(
    uint32_t instance,
    BACNET_PROPERTY_ID property,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *apdu,
    size_t apdu_size,
    BACNET_READ_PROPERTY_DATA *rpdata)
{
    memset(rpdata, 0, sizeof(*rpdata));
    rpdata->object_type = OBJECT_ANALOG_VALUE;
    rpdata->object_instance = instance;
    rpdata->object_property = property;
    rpdata->array_index = array_index;
    rpdata->application_data = apdu;
    rpdata->application_data_len = apdu_size;
    return Analog_Value_Read_Property(rpdata);
}

/**
 * @brief Test the commandable (priority array) Analog Value
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(av_tests, testAnalog_Value_Commandable)
#else
static void testAnalog_Value_Commandable(void)
#endif
{
    BACNET_WRITE_PROPERTY_DATA wp_data;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    uint32_t instance;
    int len;
    bool status;

    Analog_Value_Init();
    instance = Analog_Value_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(instance, BACNET_MAX_INSTANCE, NULL);
    Analog_Value_Write_Present_Value_Callback_Set(test_av_write_callback);
    Test_Callback_Count = 0;
    zassert_equal(Analog_Value_Present_Value_Priority(instance), 0, NULL);

    /* relinquish default drives the present value */
    zassert_true(Analog_Value_Relinquish_Default_Set(instance, 21.0f), NULL);
    zassert_false(
        islessgreater(Analog_Value_Present_Value(instance), 21.0f), NULL);
    zassert_equal(Test_Callback_Count, 1, NULL);

    /* the highest priority command wins */
    zassert_true(
        Analog_Value_Present_Value_Priority_Set(instance, 30.0f, 8), NULL);
    zassert_true(
        Analog_Value_Present_Value_Priority_Set(instance, 10.0f, 5), NULL);
    zassert_false(
        islessgreater(Analog_Value_Present_Value(instance), 10.0f), NULL);
    zassert_equal(Analog_Value_Present_Value_Priority(instance), 5, NULL);
    zassert_false(Analog_Value_Priority_Array_Relinquished(instance, 8), NULL);
    zassert_true(Analog_Value_Priority_Array_Relinquished(instance, 9), NULL);
    zassert_false(
        islessgreater(Analog_Value_Priority_Array_Value(instance, 8), 30.0f),
        NULL);
    /* a lower priority command does not change the present value */
    zassert_true(
        Analog_Value_Present_Value_Priority_Set(instance, 50.0f, 16), NULL);
    zassert_false(
        islessgreater(Analog_Value_Present_Value(instance), 10.0f), NULL);
    /* invalid priorities */
    zassert_false(
        Analog_Value_Present_Value_Priority_Set(instance, 1.0f, 0), NULL);
    zassert_false(
        Analog_Value_Present_Value_Priority_Set(instance, 1.0f, 6), NULL);
    zassert_false(
        Analog_Value_Present_Value_Priority_Set(instance, 1.0f, 17), NULL);
    /* relinquish falls back through the array */
    zassert_true(Analog_Value_Present_Value_Relinquish(instance, 5), NULL);
    zassert_false(
        islessgreater(Analog_Value_Present_Value(instance), 30.0f), NULL);
    zassert_true(Analog_Value_Present_Value_Relinquish(instance, 8), NULL);
    zassert_true(Analog_Value_Present_Value_Relinquish(instance, 16), NULL);
    zassert_false(
        islessgreater(Analog_Value_Present_Value(instance), 21.0f), NULL);
    zassert_false(islessgreater(Test_Callback_Value, 21.0f), NULL);

    /* WriteProperty: REAL at priority 9 */
    value.tag = BACNET_APPLICATION_TAG_REAL;
    value.type.Real = 42.0f;
    status = test_av_write(&wp_data, instance, PROP_PRESENT_VALUE, &value, 9);
    zassert_true(status, NULL);
    zassert_equal(Analog_Value_Present_Value_Priority(instance), 9, NULL);
    zassert_false(
        islessgreater(Analog_Value_Present_Value(instance), 42.0f), NULL);
    /* WriteProperty: priority 6 is reserved */
    status = test_av_write(&wp_data, instance, PROP_PRESENT_VALUE, &value, 6);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);
    /* WriteProperty: priority 16 (also what an absent priority decodes to) */
    value.type.Real = 7.0f;
    status = test_av_write(
        &wp_data, instance, PROP_PRESENT_VALUE, &value, BACNET_MAX_PRIORITY);
    zassert_true(status, NULL);
    zassert_false(Analog_Value_Priority_Array_Relinquished(instance, 16), NULL);

    /* ReadProperty: Priority_Array[0] is the size */
    len = test_av_read(
        instance, PROP_PRIORITY_ARRAY, 0, apdu, sizeof(apdu), &rpdata);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_UNSIGNED_INT, NULL);
    zassert_equal(value.type.Unsigned_Int, BACNET_MAX_PRIORITY, NULL);
    /* ReadProperty: Priority_Array[9] is the REAL, [1] is NULL */
    len = test_av_read(
        instance, PROP_PRIORITY_ARRAY, 9, apdu, sizeof(apdu), &rpdata);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_REAL, NULL);
    zassert_false(islessgreater(value.type.Real, 42.0f), NULL);
    len = test_av_read(
        instance, PROP_PRIORITY_ARRAY, 1, apdu, sizeof(apdu), &rpdata);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_NULL, NULL);
    /* ReadProperty: Priority_Array[17] is an error */
    len = test_av_read(
        instance, PROP_PRIORITY_ARRAY, 17, apdu, sizeof(apdu), &rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(rpdata.error_code, ERROR_CODE_INVALID_ARRAY_INDEX, NULL);
#if (BACNET_PROTOCOL_REVISION >= 17)
    len = test_av_read(
        instance, PROP_CURRENT_COMMAND_PRIORITY, BACNET_ARRAY_ALL, apdu,
        sizeof(apdu), &rpdata);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_UNSIGNED_INT, NULL);
    zassert_equal(value.type.Unsigned_Int, 9, NULL);
#endif

    /* WriteProperty: NULL relinquishes */
    value.tag = BACNET_APPLICATION_TAG_NULL;
    status = test_av_write(&wp_data, instance, PROP_PRESENT_VALUE, &value, 9);
    zassert_true(status, NULL);
    zassert_true(Analog_Value_Priority_Array_Relinquished(instance, 9), NULL);
    status = test_av_write(
        &wp_data, instance, PROP_PRESENT_VALUE, &value, BACNET_MAX_PRIORITY);
    zassert_true(status, NULL);
    zassert_equal(Analog_Value_Present_Value_Priority(instance), 0, NULL);
    /* WriteProperty: priority 0 is out of range */
    status = test_av_write(&wp_data, instance, PROP_PRESENT_VALUE, &value, 0);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);

    /* WriteProperty: Relinquish_Default */
    value.tag = BACNET_APPLICATION_TAG_REAL;
    value.type.Real = 18.5f;
    status = test_av_write(
        &wp_data, instance, PROP_RELINQUISH_DEFAULT, &value,
        BACNET_NO_PRIORITY);
    zassert_true(status, NULL);
    zassert_false(
        islessgreater(Analog_Value_Present_Value(instance), 18.5f), NULL);

    /* legacy setter without a priority writes where the value resolves */
    zassert_true(
        Analog_Value_Present_Value_Set(instance, 19.0f, BACNET_NO_PRIORITY),
        NULL);
    zassert_false(
        islessgreater(Analog_Value_Relinquish_Default(instance), 19.0f), NULL);

    Analog_Value_Write_Present_Value_Callback_Set(NULL);
    zassert_true(Analog_Value_Delete(instance), NULL);
}
#endif

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(av_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
#if defined(BACNET_OBJECT_ANALOG_VALUE_COMMANDABLE)
    ztest_test_suite(
        av_tests, ztest_unit_test(testAnalog_Value),
        ztest_unit_test(testAnalog_Value_APIs),
        ztest_unit_test(testAnalog_Value_Writable_Properties),
        ztest_unit_test(testAnalog_Value_Commandable));
#else
    ztest_test_suite(
        av_tests, ztest_unit_test(testAnalog_Value),
        ztest_unit_test(testAnalog_Value_APIs),
        ztest_unit_test(testAnalog_Value_Writable_Properties));
#endif

    ztest_run_test_suite(av_tests);
}
#endif
