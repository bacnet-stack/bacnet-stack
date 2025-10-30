/**
 * @file
 * @brief Unit test for Timer object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/timer.h>
#include <bacnet/timer_value.h>
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */
static BACNET_WRITE_PROPERTY_DATA Write_Property_Internal_Data;
static bool Write_Property_Internal(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    memcpy(
        &Write_Property_Internal_Data, wp_data,
        sizeof(BACNET_WRITE_PROPERTY_DATA));

    return true;
}

/**
 * @brief Test
 */
static void test_Timer_Read_Write(void)
{
    const uint32_t instance = 123;
    unsigned count = 0, test_count = 0;
    unsigned index = 0;
    const char *sample_name = "Timer:0";
    char *sample_context = "context";
    const char *test_name = NULL;
    uint32_t test_instance = 0;
    bool status = false;
    const int skip_fail_property_list[] = { -1 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member = { 0 }, *test_member = NULL;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    Timer_Init();
    Timer_Create(instance);
    status = Timer_Valid_Instance(instance);
    zassert_true(status, NULL);
    status = Timer_Valid_Instance(instance - 1);
    zassert_false(status, NULL);
    index = Timer_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    test_instance = Timer_Index_To_Instance(index);
    zassert_equal(instance, test_instance, NULL);
    count = Timer_Count();
    zassert_true(count > 0, NULL);
    /* configure the instance property values and test API for lists */
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_ANALOG_OUTPUT;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    status = Timer_Reference_List_Member_Element_Set(instance, index, &member);
    zassert_true(status, NULL);
    count = Timer_Reference_List_Member_Element_Count(instance);
    zassert_equal(count, 1, NULL);
    /* add the same element - should be success without actually adding */
    status = Timer_Reference_List_Member_Element_Add(instance, &member);
    zassert_true(status, NULL);
    count = Timer_Reference_List_Member_Element_Count(instance);
    zassert_equal(count, 1, NULL);
    /* next */
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_BINARY_OUTPUT;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    status = Timer_Reference_List_Member_Element_Add(instance, &member);
    zassert_true(status, NULL);
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_MULTI_STATE_OUTPUT;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    status = Timer_Reference_List_Member_Element_Add(instance, &member);
    zassert_true(status, NULL);
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_LIGHTING_OUTPUT;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    status = Timer_Reference_List_Member_Element_Add(instance, &member);
    zassert_true(status, NULL);
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_COLOR;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    status = Timer_Reference_List_Member_Element_Add(instance, &member);
    zassert_true(status, NULL);
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_COLOR_TEMPERATURE;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    status = Timer_Reference_List_Member_Element_Add(instance, &member);
    zassert_true(status, NULL);
    count = Timer_Reference_List_Member_Element_Count(instance);
    status = Timer_Reference_List_Member_Element_Remove(instance, &member);
    test_count = Timer_Reference_List_Member_Element_Count(instance);
    zassert_true(
        count > test_count, "count=%u test_count=%u", count, test_count);
    /* reliability and status flags */
    status = Timer_Reliability_Set(instance, RELIABILITY_PROCESS_ERROR);
    zassert_true(status, NULL);
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_TIMER, instance, Timer_Property_Lists, Timer_Read_Property,
        Timer_Write_Property, skip_fail_property_list);
    /* test the ASCII name get/set */
    status = Timer_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Timer_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Timer_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Timer_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);

    /* test specific WriteProperty values - common configuration */
    wp_data.object_type = OBJECT_TIMER;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_MAX_PRIORITY;
    zassert_true(status, NULL);
    /* out-of-service */
    wp_data.object_property = PROP_OUT_OF_SERVICE;
    value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.type.Boolean = true;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_true(status, NULL);
    value.type.Boolean = false;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_true(status, NULL);
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 123;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_false(status, NULL);
    /* write present-value */
    wp_data.object_property = PROP_PRESENT_VALUE;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 0;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_true(status, NULL);
    /* configure min-pres-value and max-pres-value to max limits */
    wp_data.object_property = PROP_MIN_PRES_VALUE;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 1;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_MAX_PRES_VALUE;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = UINT32_MAX;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_PRESENT_VALUE;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 1;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_PRESENT_VALUE;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = UINT32_MAX;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_true(status, NULL);
    /* configure min-pres-value and max-pres-value to elicit out-of-range */
    wp_data.object_property = PROP_MIN_PRES_VALUE;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 100;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_MAX_PRES_VALUE;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = UINT32_MAX - 100;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_PRESENT_VALUE;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 1;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
    wp_data.object_property = PROP_PRESENT_VALUE;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = UINT32_MAX;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
    /* read-only property */
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_MAX_PRIORITY;
    wp_data.object_property = PROP_OBJECT_TYPE;
    value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value.type.Enumerated = OBJECT_ANALOG_INPUT;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Timer_Write_Property(&wp_data);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);
    zassert_false(status, NULL);
    /* present-value API */
    status = Timer_Present_Value_Set(instance, 0);
    zassert_true(status, NULL);
    /* negative testing of API */
    test_member = Timer_Reference_List_Member_Element(instance + 1, 0);
    zassert_true(test_member == NULL, NULL);
    /* reliability and status flags */
    status = Timer_Reliability_Set(instance, RELIABILITY_PROCESS_ERROR);
    zassert_true(status, NULL);
    /* context API */
    Timer_Context_Set(instance, sample_context);
    zassert_true(sample_context == Timer_Context_Get(instance), NULL);
    zassert_true(NULL == Timer_Context_Get(instance + 1), NULL);
    /* cleanup */
    status = Timer_Delete(instance);
    zassert_true(status, NULL);
    Timer_Cleanup();
}

static void test_Timer_Operation_Transition_Default(
    const uint32_t instance,
    BACNET_TIMER_STATE test_state,
    BACNET_TIMER_TRANSITION test_transition)
{
    bool status = false;
    BACNET_TIMER_STATE_CHANGE_VALUE *value = NULL;
    BACNET_TIMER_STATE state = TIMER_STATE_IDLE;
    BACNET_TIMER_TRANSITION transition = TIMER_TRANSITION_NONE;
    uint32_t timeout = 0, test_timeout = 0;
    BACNET_DATE_TIME bdatetime = { 0 }, test_bdatetime = { 0 };
    BACNET_APPLICATION_DATA_VALUE test_value = { 0 };
    int diff = 0, len = 0;

    state = Timer_State(instance);
    zassert_equal(state, test_state, NULL);
    transition = Timer_Last_State_Change(instance);
    zassert_equal(transition, test_transition, NULL);
    if (Timer_Running(instance)) {
        timeout = Timer_Default_Timeout(instance);
        test_timeout = Timer_Initial_Timeout(instance);
        zassert_equal(timeout, test_timeout, NULL);
        status = Timer_Initial_Timeout_Set(instance, timeout);
        zassert_true(status, NULL);
        test_timeout = Timer_Initial_Timeout(instance);
        zassert_equal(timeout, test_timeout, NULL);
        test_timeout = Timer_Present_Value(instance);
        zassert_equal(timeout, test_timeout, NULL);
    }
    datetime_local(&bdatetime.date, &bdatetime.time, NULL, NULL);
    status = Timer_Update_Time(instance, &test_bdatetime);
    zassert_true(status, NULL);
    diff = datetime_compare(&bdatetime, &test_bdatetime);
    zassert_equal(diff, 0, "diff=%d", diff);
    status = Timer_Update_Time_Set(instance, &bdatetime);
    zassert_true(status, NULL);
    status = Timer_Update_Time(instance, &test_bdatetime);
    zassert_true(status, NULL);
    diff = datetime_compare(&bdatetime, &test_bdatetime);
    zassert_equal(diff, 0, "diff=%d", diff);
    status = Timer_Expiration_Time(instance, &test_bdatetime);
    zassert_true(status, NULL);
    diff = datetime_compare(&bdatetime, &test_bdatetime);
    zassert_true(diff < 0, "diff=%d", diff);
    zassert_equal(
        Write_Property_Internal_Data.object_property, PROP_PRESENT_VALUE, NULL);
    len = bacapp_decode_application_data(
        Write_Property_Internal_Data.application_data,
        Write_Property_Internal_Data.application_data_len, &test_value);
    zassert_true(len > 0, "len=%d", len);
    value = Timer_State_Change_Value(instance, test_transition);
    zassert_equal(test_value.tag, value->tag, NULL);
    zassert_equal(test_value.type.Enumerated, value->type.Enumerated, NULL);
}

/**
 * @brief Test
 */
static void test_Timer_Operation(void)
{
    const uint32_t instance = 123;
    bool status = false;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member = { 0 };
    BACNET_TIMER_STATE_CHANGE_VALUE *value = NULL;
    BACNET_TIMER_STATE test_state = TIMER_STATE_IDLE;
    BACNET_DATE_TIME bdatetime = { 0 };
    uint32_t elapsed_time = 0;
    unsigned members = 0, i = 0;

    /* init */
    Timer_Init();
    Timer_Create(instance);
    status = Timer_Valid_Instance(instance);
    zassert_true(status, NULL);
    /* set the local time */
    datetime_set_values(&bdatetime, 2025, 10, 24, 10, 50, 42, 42);
    datetime_timesync(&bdatetime.date, &bdatetime.time, false);
    /* configure the reference members and the write property values */
    Timer_Write_Property_Internal_Callback_Set(Write_Property_Internal);
    members = Timer_Reference_List_Member_Capacity(instance);
    for (i = 0; i < members; i++) {
        member.deviceIdentifier.type = OBJECT_DEVICE;
        member.deviceIdentifier.instance = 0;
        member.objectIdentifier.type = OBJECT_BINARY_VALUE;
        member.objectIdentifier.instance = 1 + i;
        member.propertyIdentifier = PROP_PRESENT_VALUE;
        member.arrayIndex = BACNET_ARRAY_ALL;
        status = Timer_Reference_List_Member_Element_Set(instance, i, &member);
    }
    /* check the transitions boundaries */
    value = Timer_State_Change_Value(instance, TIMER_TRANSITION_NONE);
    zassert_true(value == NULL, NULL);
    value = Timer_State_Change_Value(instance, TIMER_TRANSITION_MAX);
    zassert_true(value == NULL, NULL);
    /* configure the transitions */
    value =
        Timer_State_Change_Value(instance, TIMER_TRANSITION_IDLE_TO_RUNNING);
    value->tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value->type.Enumerated = BINARY_ACTIVE;
    value =
        Timer_State_Change_Value(instance, TIMER_TRANSITION_RUNNING_TO_IDLE);
    value->tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value->type.Enumerated = BINARY_INACTIVE;
    value =
        Timer_State_Change_Value(instance, TIMER_TRANSITION_EXPIRED_TO_IDLE);
    value->tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value->type.Enumerated = BINARY_INACTIVE;
    value =
        Timer_State_Change_Value(instance, TIMER_TRANSITION_RUNNING_TO_EXPIRED);
    value->tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value->type.Enumerated = BINARY_INACTIVE;
    value =
        Timer_State_Change_Value(instance, TIMER_TRANSITION_FORCED_TO_EXPIRED);
    value->tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value->type.Enumerated = BINARY_INACTIVE;
    value =
        Timer_State_Change_Value(instance, TIMER_TRANSITION_EXPIRED_TO_RUNNING);
    value->tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value->type.Enumerated = BINARY_ACTIVE;
    /* alternate API */
    status = Timer_State_Change_Value_Set(
        instance, TIMER_TRANSITION_EXPIRED_TO_RUNNING, value);
    zassert_true(status, NULL);
    /* start timer using a write to timer-running property to use defaults */
    /* IDLE_TO_RUNNING */
    test_state = Timer_State(instance);
    zassert_equal(test_state, TIMER_STATE_IDLE, NULL);
    status = Timer_Running_Set(instance, true);
    zassert_true(status, NULL);
    status = Timer_Running(instance);
    zassert_true(status, NULL);
    test_Timer_Operation_Transition_Default(
        instance, TIMER_STATE_RUNNING, TIMER_TRANSITION_IDLE_TO_RUNNING);
    /* RUNNING_TO_RUNNING */
    test_state = Timer_State(instance);
    zassert_equal(test_state, TIMER_STATE_RUNNING, NULL);
    status = Timer_Running_Set(instance, true);
    zassert_true(status, NULL);
    status = Timer_Running(instance);
    zassert_true(status, NULL);
    test_Timer_Operation_Transition_Default(
        instance, TIMER_STATE_RUNNING, TIMER_TRANSITION_RUNNING_TO_RUNNING);
    /* EXPIRED_TO_RUNNING */
    test_state = Timer_State(instance);
    zassert_equal(test_state, TIMER_STATE_RUNNING, NULL);
    elapsed_time = Timer_Present_Value(instance);
    Timer_Task(instance, elapsed_time);
    test_state = Timer_State(instance);
    zassert_equal(test_state, TIMER_STATE_EXPIRED, NULL);
    status = Timer_Running_Set(instance, true);
    zassert_true(status, NULL);
    status = Timer_Running(instance);
    zassert_true(status, NULL);
    test_Timer_Operation_Transition_Default(
        instance, TIMER_STATE_RUNNING, TIMER_TRANSITION_EXPIRED_TO_RUNNING);
    /* start timer using a write to timer-state property to use defaults */
    /* RUNNING_TO_IDLE */
    test_state = Timer_State(instance);
    zassert_equal(test_state, TIMER_STATE_RUNNING, NULL);
    status = Timer_State_Set(instance, TIMER_STATE_IDLE);
    zassert_true(status, NULL);
    test_state = Timer_State(instance);
    zassert_equal(test_state, TIMER_STATE_IDLE, NULL);
    test_Timer_Operation_Transition_Default(
        instance, TIMER_STATE_IDLE, TIMER_TRANSITION_RUNNING_TO_IDLE);
    Timer_Task(instance, elapsed_time);
    /* RUNNING_TO_EXPIRED */
    status = Timer_Running_Set(instance, true);
    zassert_true(status, NULL);
    test_state = Timer_State(instance);
    zassert_equal(test_state, TIMER_STATE_RUNNING, NULL);
    status = Timer_Running_Set(instance, false);
    zassert_true(status, NULL);
    test_state = Timer_State(instance);
    zassert_equal(test_state, TIMER_STATE_EXPIRED, NULL);
    test_Timer_Operation_Transition_Default(
        instance, TIMER_STATE_EXPIRED, TIMER_TRANSITION_FORCED_TO_EXPIRED);
    Timer_Task(instance, elapsed_time);
    /* cleanup instance */
    status = Timer_Delete(instance);
    zassert_true(status, NULL);
    /* cleanup all */
    zassert_equal(instance, Timer_Create(instance), NULL);
    Timer_Cleanup();
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(
        timer_tests, ztest_unit_test(test_Timer_Read_Write),
        ztest_unit_test(test_Timer_Operation));

    ztest_run_test_suite(timer_tests);
}
