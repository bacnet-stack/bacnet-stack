/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet load control object
 */

#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacstr.h>
#include <bacnet/basic/object/ao.h>
#include <bacnet/basic/object/lc.h>
#include <property_test.h>

/* TODO: Refactor basic/object/lc.c to avoid duplication of the following:
 */
/* number of demo objects */
#ifndef MAX_LOAD_CONTROLS
#define MAX_LOAD_CONTROLS 4
#endif

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */

static void
Load_Control_WriteProperty_Request_Shed_Level(int instance, unsigned level)
{
    bool status = false;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_WRITE_PROPERTY_DATA wp_data;

    wp_data.object_type = OBJECT_LOAD_CONTROL;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.object_property = PROP_REQUESTED_SHED_LEVEL;
    value.type.Shed_Level.type = BACNET_SHED_TYPE_LEVEL;
    value.type.Shed_Level.value.level = level;
    wp_data.application_data_len = bacapp_encode_known_property(
        &wp_data.application_data[0], &value, wp_data.object_type,
        wp_data.object_property);
    zassert_true(wp_data.application_data_len > 0, NULL);
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, "LC=%lu level=%u", instance, level);
}

static void Load_Control_WriteProperty_Enable(int instance, bool enable)
{
    bool status = false;
    BACNET_APPLICATION_DATA_VALUE value;
    BACNET_WRITE_PROPERTY_DATA wp_data;

    wp_data.object_type = OBJECT_LOAD_CONTROL;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    /* Set Enable=TRUE */
    wp_data.object_property = PROP_ENABLE;
    value.context_specific = false;
    value.context_tag = 0;
    value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.type.Boolean = enable;
    wp_data.application_data_len =
        bacapp_encode_application_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}

static void
Load_Control_WriteProperty_Shed_Duration(int instance, unsigned duration)
{
    bool status = false;
    BACNET_APPLICATION_DATA_VALUE value;
    BACNET_WRITE_PROPERTY_DATA wp_data;

    wp_data.object_type = OBJECT_LOAD_CONTROL;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.object_property = PROP_SHED_DURATION;
    value.context_specific = false;
    value.context_tag = 0;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = duration;
    wp_data.application_data_len =
        bacapp_encode_application_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}

static void
Load_Control_WriteProperty_Duty_Window(int instance, unsigned duration)
{
    bool status = false;
    BACNET_APPLICATION_DATA_VALUE value;
    BACNET_WRITE_PROPERTY_DATA wp_data;

    wp_data.object_type = OBJECT_LOAD_CONTROL;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.object_property = PROP_DUTY_WINDOW;
    value.context_specific = false;
    value.context_tag = 0;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = duration;
    wp_data.application_data_len =
        bacapp_encode_application_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}

static void Load_Control_WriteProperty_Start_Time_Wildcards(int instance)
{
    int len = 0;
    bool status = false;
    BACNET_APPLICATION_DATA_VALUE value;
    BACNET_WRITE_PROPERTY_DATA wp_data;

    wp_data.object_type = OBJECT_LOAD_CONTROL;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.object_property = PROP_START_TIME;
    value.tag = BACNET_APPLICATION_TAG_DATETIME;
    datetime_date_wildcard_set(&value.type.Date_Time.date);
    datetime_time_wildcard_set(&value.type.Date_Time.time);
    wp_data.application_data_len = bacapp_encode_known_property(
        &wp_data.application_data[0], &value, wp_data.object_type,
        wp_data.object_property);
    zassert_true(wp_data.application_data_len > 0, NULL);
    wp_data.application_data_len += len;
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}

static void Load_Control_WriteProperty_Start_Time(
    int instance,
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t seconds,
    uint8_t hundredths)
{
    int len = 0;
    bool status = false;
    BACNET_APPLICATION_DATA_VALUE value;
    BACNET_WRITE_PROPERTY_DATA wp_data;

    wp_data.object_type = OBJECT_LOAD_CONTROL;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.object_property = PROP_START_TIME;
    value.tag = BACNET_APPLICATION_TAG_DATETIME;
    datetime_set_date(&value.type.Date_Time.date, year, month, day);
    datetime_set_time(
        &value.type.Date_Time.time, hour, minute, seconds, hundredths);
    wp_data.application_data_len = bacapp_encode_known_property(
        &wp_data.application_data[0], &value, wp_data.object_type,
        wp_data.object_property);
    zassert_true(wp_data.application_data_len > 0, NULL);
    wp_data.application_data_len += len;
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}

/**
 * Test object for the Load Control object manipulation
 */
struct object_data {
    bool Relinquished[BACNET_MAX_PRIORITY];
    float Priority_Array[BACNET_MAX_PRIORITY];
    float Relinquish_Default;
} Test_Object_Data;

/**
 * @brief For a given object instance-number, determines the present-value
 * @param  object_instance - object-instance number of the object
 * @return  present-value of the object
 */
static float Test_Present_Value(void)
{
    float value;
    uint8_t priority;

    value = Test_Object_Data.Relinquish_Default;
    for (priority = 0; priority < BACNET_MAX_PRIORITY; priority++) {
        if (!Test_Object_Data.Relinquished[priority]) {
            value = Test_Object_Data.Priority_Array[priority];
            break;
        }
    }

    return value;
}

/**
 * @brief For a given object instance-number, determines the priority
 * @param  object_instance - object-instance number of the object
 * @return  active priority 1..16, or 0 if no priority is active
 */
static unsigned Test_Present_Value_Priority(void)
{
    unsigned p = 0; /* loop counter */
    unsigned priority = 0; /* return value */

    for (p = 0; p < BACNET_MAX_PRIORITY; p++) {
        if (!Test_Object_Data.Relinquished[p]) {
            priority = p + 1;
            break;
        }
    }

    return priority;
}

/**
 * @brief For a given object instance-number, sets the present-value
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point analog value
 * @param  priority - priority-array index value 1..16
 * @return  true if values are within range and present-value is set.
 */
static bool Test_Present_Value_Priority_Set(float value, unsigned priority)
{
    bool status = false;

    if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY)) {
        Test_Object_Data.Relinquished[priority - 1] = false;
        Test_Object_Data.Priority_Array[priority - 1] = value;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, relinquishes the present-value
 * @param  object_instance - object-instance number of the object
 * @param  priority - priority-array index value 1..16
 * @return  true if values are within range and present-value is relinquished.
 */
static bool Test_Present_Value_Priority_Relinquish(unsigned priority)
{
    bool status = false;

    if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY)) {
        Test_Object_Data.Relinquished[priority - 1] = true;
        Test_Object_Data.Priority_Array[priority - 1] = 0.0;
        status = true;
    }

    return status;
}

static BACNET_OBJECT_PROPERTY_REFERENCE test_object_property_reference;

static void test_load_control_manipulated_object_read(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property_id,
    uint8_t *priority,
    float *value)
{
    zassert_equal(
        test_object_property_reference.object_identifier.type, object_type,
        NULL);
    zassert_equal(
        test_object_property_reference.object_identifier.instance,
        object_instance, NULL);
    zassert_equal(
        test_object_property_reference.property_identifier, property_id, NULL);
    if (priority) {
        *priority = Test_Present_Value_Priority();
    }
    if (value) {
        *value = Test_Present_Value();
    }
}

/**
 * @brief Callback for manipulated object controlled value write
 * @param  object_type - object type of the manipulated object
 * @param  object_instance - object-instance number of the object
 * @param  property_id - property identifier of the manipulated object
 * @param  priority - priority of the write
 * @param  value - value of the write
 */
static void test_load_control_manipulated_object_write(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property_id,
    uint8_t priority,
    float value)
{
    zassert_equal(
        test_object_property_reference.object_identifier.type, object_type,
        NULL);
    zassert_equal(
        test_object_property_reference.object_identifier.instance,
        object_instance, NULL);
    zassert_equal(
        test_object_property_reference.property_identifier, property_id, NULL);
    Test_Present_Value_Priority_Set(value, priority);
}

/**
 * @brief Callback for manipulated object controlled value relinquish
 * @param  object_type - object type of the manipulated object
 * @param  object_instance - object-instance number of the object
 * @param  property_id - property identifier of the manipulated object
 * @param  priority - priority of the relinquish
 */
static void test_load_control_manipulated_object_relinquish(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property_id,
    uint8_t priority)
{
    zassert_equal(
        test_object_property_reference.object_identifier.type, object_type,
        NULL);
    zassert_equal(
        test_object_property_reference.object_identifier.instance,
        object_instance, NULL);
    zassert_equal(
        test_object_property_reference.property_identifier, property_id, NULL);
    Test_Present_Value_Priority_Relinquish(priority);
}

static void test_setup(uint32_t object_instance)
{
    uint32_t test_object_instance = 0;
    unsigned priority = 0;

    Load_Control_Init();
    test_object_instance = Load_Control_Create(object_instance);
    zassert_equal(test_object_instance, object_instance, NULL);
    test_object_instance = Load_Control_Index_To_Instance(0);
    zassert_equal(test_object_instance, object_instance, NULL);
    /* manipulated object */
    test_object_property_reference.object_identifier.type =
        OBJECT_ANALOG_OUTPUT;
    test_object_property_reference.object_identifier.instance = 1;
    test_object_property_reference.property_identifier = PROP_PRESENT_VALUE;
    Load_Control_Manipulated_Variable_Reference_Set(
        object_instance, &test_object_property_reference);
    Load_Control_Manipulated_Object_Write_Callback_Set(
        object_instance, test_load_control_manipulated_object_write);
    Load_Control_Manipulated_Object_Relinquish_Callback_Set(
        object_instance, test_load_control_manipulated_object_relinquish);
    Load_Control_Manipulated_Object_Read_Callback_Set(
        object_instance, test_load_control_manipulated_object_read);
    /* target object */
    for (priority = 1; priority <= BACNET_MAX_PRIORITY; priority++) {
        Test_Present_Value_Priority_Relinquish(priority);
    }
    Test_Present_Value_Priority_Set(0.0f, 16);
    Load_Control_Priority_For_Writing_Set(object_instance, 4);
}

static void test_teardown(uint32_t object_instance)
{
    bool status = false;

    status = Load_Control_Delete(object_instance);
    zassert_true(status, NULL);
    Load_Control_Cleanup();
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lc_tests, testLoadControlStateMachine)
#else
static void testLoadControlStateMachine(void)
#endif
{
    unsigned j = 0, priority = 0;
    float level = 0;
    unsigned count = 0;
    BACNET_DATE_TIME bdatetime = { 0 };
    uint32_t object_index = 0;
    uint32_t object_instance = 1234;
    BACNET_OBJECT_PROPERTY_REFERENCE object_property_reference;
    bool status;

    test_setup(object_instance);
    status = Load_Control_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    Load_Control_Manipulated_Variable_Reference(
        object_instance, &object_property_reference);
    zassert_equal(
        object_property_reference.object_identifier.type,
        test_object_property_reference.object_identifier.type, NULL);
    zassert_equal(
        object_property_reference.object_identifier.instance,
        test_object_property_reference.object_identifier.instance, NULL);
    zassert_equal(
        object_property_reference.property_identifier,
        test_object_property_reference.property_identifier, NULL);
    /* validate the state does not change - without any triggers */
    for (j = 0; j < 20; j++) {
        Load_Control_State_Machine(object_index, &bdatetime);
        count = Load_Control_Count();
        zassert_equal(count, 1, NULL);
        zassert_equal(
            Load_Control_Present_Value(object_instance), BACNET_SHED_INACTIVE,
            NULL);
    }
    /* BACNET_SHED_REQUEST_PENDING */
    /* CancelShed - Start time has wildcards */
    Load_Control_WriteProperty_Enable(object_instance, true);
    Load_Control_WriteProperty_Shed_Duration(object_instance, 60);
    Load_Control_WriteProperty_Start_Time_Wildcards(object_instance);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_INACTIVE,
        NULL);
    test_teardown(object_instance);

    /* CancelShed - Requested_Shed_Level equal to default value */
    test_setup(object_instance);
    status = Load_Control_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    Load_Control_WriteProperty_Request_Shed_Level(object_instance, 0);
    Load_Control_WriteProperty_Start_Time(
        object_instance, 2007, 2, 27, 15, 0, 0, 0);
    Load_Control_WriteProperty_Shed_Duration(object_instance, 5);
    datetime_set_values(&bdatetime, 2007, 2, 27, 15, 0, 0, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_INACTIVE,
        NULL);
    test_teardown(object_instance);

    /* CancelShed - Non-default values, but Start time is passed */
    test_setup(object_instance);
    status = Load_Control_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    Load_Control_WriteProperty_Enable(object_instance, true);
    Load_Control_WriteProperty_Request_Shed_Level(object_instance, 1);
    Load_Control_WriteProperty_Shed_Duration(object_instance, 5);
    Load_Control_WriteProperty_Start_Time(
        object_instance, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&bdatetime, 2007, 2, 28, 15, 0, 0, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_INACTIVE,
        NULL);
    test_teardown(object_instance);

    /* ReconfigurePending - new write received while pending */
    test_setup(object_instance);
    status = Load_Control_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    Load_Control_WriteProperty_Enable(object_instance, true);
    Load_Control_WriteProperty_Request_Shed_Level(object_instance, 1);
    Load_Control_WriteProperty_Shed_Duration(object_instance, 5);
    Load_Control_WriteProperty_Start_Time(
        object_instance, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&bdatetime, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Request_Shed_Level(object_instance, 2);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Shed_Duration(object_instance, 6);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Duty_Window(object_instance, 60);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Start_Time(
        object_instance, 2007, 2, 27, 15, 0, 0, 1);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    test_teardown(object_instance);

    /* CannotMeetShed -> FinishedUnsuccessfulShed */
    test_setup(object_instance);
    status = Load_Control_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    Load_Control_WriteProperty_Enable(object_instance, true);
    Load_Control_WriteProperty_Request_Shed_Level(object_instance, 1);
    Load_Control_WriteProperty_Shed_Duration(object_instance, 120);
    Load_Control_WriteProperty_Start_Time(
        object_instance, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&bdatetime, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    /* set to lowest value so we cannot meet the shed level */
    datetime_set_values(&bdatetime, 2007, 2, 27, 16, 0, 0, 0);
    Test_Present_Value_Priority_Set(0.0f, 16);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_NON_COMPLIANT,
        NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_NON_COMPLIANT,
        NULL);
    /* FinishedUnsuccessfulShed */
    datetime_set_values(&bdatetime, 2007, 2, 27, 23, 0, 0, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_INACTIVE,
        NULL);
    test_teardown(object_instance);

    /* CannotMeetShed -> UnsuccessfulShedReconfigured */
    test_setup(object_instance);
    status = Load_Control_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    Load_Control_WriteProperty_Enable(object_instance, true);
    Load_Control_WriteProperty_Request_Shed_Level(object_instance, 1);
    Load_Control_WriteProperty_Shed_Duration(object_instance, 120);
    Load_Control_WriteProperty_Start_Time(
        object_instance, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&bdatetime, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    /* set to lowest value so we cannot meet the shed level */
    datetime_set_values(&bdatetime, 2007, 2, 27, 16, 0, 0, 0);
    Test_Present_Value_Priority_Set(0.0f, 16);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_NON_COMPLIANT,
        NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_NON_COMPLIANT,
        NULL);
    /* FinishedUnsuccessfulShed */
    Load_Control_WriteProperty_Start_Time(
        object_instance, 2007, 2, 27, 16, 0, 0, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance),
        BACNET_SHED_REQUEST_PENDING, NULL);
    datetime_set_values(&bdatetime, 2007, 2, 27, 16, 0, 1, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_NON_COMPLIANT,
        NULL);
    /* CanNowComplyWithShed */
    Test_Present_Value_Priority_Set(100.0f, 16);
    datetime_set_values(&bdatetime, 2007, 2, 27, 16, 0, 2, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_COMPLIANT,
        NULL);
    level = Test_Present_Value();
    zassert_true(
        islessgreater(100.0f, level), "Present Value = %f", (double)level);
    priority = Test_Present_Value_Priority();
    zassert_equal(
        Load_Control_Priority_For_Writing(object_instance), priority, NULL);
    /* FinishedSuccessfulShed */
    datetime_set_values(&bdatetime, 2007, 2, 27, 23, 0, 0, 0);
    Load_Control_State_Machine(object_index, &bdatetime);
    zassert_equal(
        Load_Control_Present_Value(object_instance), BACNET_SHED_INACTIVE,
        NULL);
    level = Test_Present_Value();
    zassert_false(
        islessgreater(100.0f, level), "Present Value = %f", (double)level);
    priority = Test_Present_Value_Priority();
    zassert_equal(16, priority, NULL);
    test_teardown(object_instance);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lc_tests, test_Load_Control_Read_Write_Property)
#else
static void test_Load_Control_Read_Write_Property(void)
#endif
{
    unsigned count = 0;
    uint32_t object_instance = 123;
    unsigned index;
    bool status = false;
    const int skip_fail_property_list[] = { -1 };

    test_setup(object_instance);
    status = Load_Control_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    index = Load_Control_Instance_To_Index(object_instance);
    zassert_equal(index, 0, NULL);
    count = Load_Control_Count();
    zassert_true(count == 1, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_LOAD_CONTROL, object_instance, Load_Control_Property_Lists,
        Load_Control_Read_Property, Load_Control_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Load_Control_Name_Set, Load_Control_Name_ASCII);
    test_teardown(object_instance);
}

static bool init_wp_data_and_value(
    BACNET_WRITE_PROPERTY_DATA *wp_data, BACNET_APPLICATION_DATA_VALUE *value)
{
    bool status = false;
    if ((wp_data != NULL) && (value != NULL)) {
        memset(value, 0, sizeof(*value));
        memset(wp_data, 0, sizeof(*wp_data));

        wp_data->object_type = OBJECT_LOAD_CONTROL;
        wp_data->object_instance = 0;
        wp_data->array_index = BACNET_ARRAY_ALL;
        wp_data->priority = BACNET_NO_PRIORITY;
        wp_data->object_property = PROP_SHED_DURATION;
        value->context_specific = false;
        value->context_tag = 0;
        value->tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
        value->type.Unsigned_Int = 0; /* duration */
        wp_data->application_data_len = bacapp_encode_application_data(
            &wp_data->application_data[0], value);
        zassert_true(wp_data->application_data_len >= 0, NULL);
        zassert_equal(wp_data->error_class, 0, NULL);
        zassert_equal(wp_data->error_code, 0, NULL);

        status = true;
    }

    return status;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lc_tests, test_ShedInactive_gets_RcvShedRequests)
#else
static void test_ShedInactive_gets_RcvShedRequests(void)
#endif
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    /* Verify invalid parameters cause failure */
    zassert_false(Load_Control_Write_Property(NULL), NULL);

    /* Verify invalid parameter value of application_data_len cause failure */
    zassert_true(init_wp_data_and_value(&wp_data, &value), NULL);
    wp_data.application_data_len = -1;
    zassert_false(Load_Control_Write_Property(&wp_data), NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);

    /* Verify invalid parameter value of application_data_len cause failure */
    zassert_true(init_wp_data_and_value(&wp_data, &value), NULL);
    wp_data.application_data_len = -1;
    zassert_false(Load_Control_Write_Property(&wp_data), NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(lc_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        lc_tests, ztest_unit_test(test_Load_Control_Read_Write_Property),
        ztest_unit_test(testLoadControlStateMachine),
        ztest_unit_test(test_ShedInactive_gets_RcvShedRequests));

    ztest_run_test_suite(lc_tests);
}
#endif
