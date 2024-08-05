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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lc_tests, test_Load_Control_Count)
#else
static void test_Load_Control_Count(void)
#endif
{
    /* Verify the same value is returned on successive calls without init */
    zassert_equal(Load_Control_Count(), MAX_LOAD_CONTROLS, NULL);
    zassert_equal(Load_Control_Count(), MAX_LOAD_CONTROLS, NULL);

    /* Verify the same value is returned on successive calls with init */
    Load_Control_Init();
    zassert_equal(Load_Control_Count(), MAX_LOAD_CONTROLS, NULL);
    zassert_equal(Load_Control_Count(), MAX_LOAD_CONTROLS, NULL);

    /* Verify the same value is returned on successive calls with re-init */
    Load_Control_Init();
    zassert_equal(Load_Control_Count(), MAX_LOAD_CONTROLS, NULL);
    zassert_equal(Load_Control_Count(), MAX_LOAD_CONTROLS, NULL);
}

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
    zassert_true(status, NULL);
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lc_tests, testLoadControlStateMachine)
#else
static void testLoadControlStateMachine(void)
#endif
{
    unsigned i = 0, j = 0;
    float level = 0;
    unsigned count = 0;
    BACNET_DATE_TIME bdatetime = { 0 };
    uint32_t object_instance = 0;
    BACNET_OBJECT_PROPERTY_REFERENCE object_property_reference;
    bool status;

    Analog_Output_Init();
    Load_Control_Init();
    object_instance = Load_Control_Index_To_Instance(0);
    /* validate the triggers for each state change */
    for (j = 0; j < 20; j++) {
        Load_Control_State_Machine(0, &bdatetime);
        count = Load_Control_Count();
        for (i = 0; i < count; i++) {
            zassert_equal(Load_Control_State(i), SHED_INACTIVE, NULL);
        }
    }
    /* SHED_REQUEST_PENDING */
    /* CancelShed - Start time has wildcards */
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Shed_Duration(0, 60);
    Load_Control_WriteProperty_Start_Time_Wildcards(0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_INACTIVE, NULL);

    /* CancelShed - Requested_Shed_Level equal to default value */
    Load_Control_Init();
    Load_Control_WriteProperty_Request_Shed_Level(0, 0);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
    Load_Control_WriteProperty_Shed_Duration(0, 5);
    datetime_set_values(&bdatetime, 2007, 2, 27, 15, 0, 0, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_INACTIVE, NULL);

    /* CancelShed - Non-default values, but Start time is passed */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Request_Shed_Level(0, 1);
    Load_Control_WriteProperty_Shed_Duration(0, 5);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&bdatetime, 2007, 2, 28, 15, 0, 0, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_INACTIVE, NULL);

    /* ReconfigurePending - new write received while pending */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Request_Shed_Level(0, 1);
    Load_Control_WriteProperty_Shed_Duration(0, 5);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&bdatetime, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Request_Shed_Level(0, 2);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Shed_Duration(0, 6);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Duty_Window(0, 60);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 1);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);

    /* CannotMeetShed -> FinishedUnsuccessfulShed */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Request_Shed_Level(0, 1);
    Load_Control_WriteProperty_Shed_Duration(0, 120);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&bdatetime, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    /* set to lowest value so we cannot meet the shed level */
    datetime_set_values(&bdatetime, 2007, 2, 27, 16, 0, 0, 0);

    Load_Control_Manipulated_Variable_Reference(
        object_instance, &object_property_reference);
    if ((object_property_reference.object_identifier.type ==
         OBJECT_ANALOG_OUTPUT) &&
        (object_property_reference.property_identifier == PROP_PRESENT_VALUE)) {
        status = Analog_Output_Present_Value_Set(
            object_property_reference.object_identifier.instance, 0.0f,
            BACNET_MAX_PRIORITY);
        zassert_true(status, NULL);
    } else {
        zassert_true(false, NULL);
    }

    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_NON_COMPLIANT, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_NON_COMPLIANT, NULL);
    /* FinishedUnsuccessfulShed */
    datetime_set_values(&bdatetime, 2007, 2, 27, 23, 0, 0, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_INACTIVE, NULL);

    /* CannotMeetShed -> UnsuccessfulShedReconfigured */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Request_Shed_Level(0, 1);
    Load_Control_WriteProperty_Shed_Duration(0, 120);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&bdatetime, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    /* set to lowest value so we cannot meet the shed level */
    datetime_set_values(&bdatetime, 2007, 2, 27, 16, 0, 0, 0);
    Load_Control_Manipulated_Variable_Reference(
        object_instance, &object_property_reference);
    if ((object_property_reference.object_identifier.type ==
         OBJECT_ANALOG_OUTPUT) &&
        (object_property_reference.property_identifier == PROP_PRESENT_VALUE)) {
        status = Analog_Output_Present_Value_Set(
            object_property_reference.object_identifier.instance, 0.0f,
            BACNET_MAX_PRIORITY);
        zassert_true(status, NULL);
    } else {
        zassert_true(false, NULL);
    }
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_NON_COMPLIANT, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_NON_COMPLIANT, NULL);
    /* FinishedUnsuccessfulShed */
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 16, 0, 0, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_REQUEST_PENDING, NULL);
    datetime_set_values(&bdatetime, 2007, 2, 27, 16, 0, 1, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_NON_COMPLIANT, NULL);
    /* CanNowComplyWithShed */
    Load_Control_Manipulated_Variable_Reference(
        object_instance, &object_property_reference);
    if ((object_property_reference.object_identifier.type ==
         OBJECT_ANALOG_OUTPUT) &&
        (object_property_reference.property_identifier == PROP_PRESENT_VALUE)) {
        status = Analog_Output_Present_Value_Set(
            object_property_reference.object_identifier.instance, 100.0f,
            BACNET_MAX_PRIORITY);
        zassert_true(status, NULL);
    } else {
        zassert_true(false, NULL);
    }
    datetime_set_values(&bdatetime, 2007, 2, 27, 16, 0, 2, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_COMPLIANT, NULL);
    Load_Control_Manipulated_Variable_Reference(
        object_instance, &object_property_reference);
    if ((object_property_reference.object_identifier.type ==
         OBJECT_ANALOG_OUTPUT) &&
        (object_property_reference.property_identifier == PROP_PRESENT_VALUE)) {
        level = Analog_Output_Present_Value(
            object_property_reference.object_identifier.instance);
    } else {
        zassert_true(false, NULL);
    }
    zassert_false(islessgreater(90.0f, level), "AO Present Value = %f", level);
    /* FinishedSuccessfulShed */
    datetime_set_values(&bdatetime, 2007, 2, 27, 23, 0, 0, 0);
    Load_Control_State_Machine(0, &bdatetime);
    zassert_equal(Load_Control_State(0), SHED_INACTIVE, NULL);
    if ((object_property_reference.object_identifier.type ==
         OBJECT_ANALOG_OUTPUT) &&
        (object_property_reference.property_identifier == PROP_PRESENT_VALUE)) {
        level = Analog_Output_Present_Value(
            object_property_reference.object_identifier.instance);
    } else {
        zassert_true(false, NULL);
    }
    zassert_false(islessgreater(100.0f, level), "AO Present Value = %f", level);
}

#ifndef MAX_LOAD_CONTROLS
#define MAX_LOAD_CONTROLS (4)
#endif

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lc_tests, test_api_stubs)
#else
static void test_api_stubs(void)
#endif
{
    BACNET_CHARACTER_STRING object_name_st = { 0 };

    zassert_equal(Load_Control_Count(), MAX_LOAD_CONTROLS, NULL);

    zassert_false(Load_Control_Valid_Instance(MAX_LOAD_CONTROLS), NULL);
    zassert_equal(
        Load_Control_Index_To_Instance(MAX_LOAD_CONTROLS), Load_Control_Count(),
        NULL);
    zassert_equal(
        Load_Control_Instance_To_Index(MAX_LOAD_CONTROLS), Load_Control_Count(),
        NULL);

    zassert_false(Load_Control_Valid_Instance(UINT32_MAX), NULL);
    zassert_equal(
        Load_Control_Index_To_Instance(UINT32_MAX), Load_Control_Count(), NULL);
    zassert_equal(
        Load_Control_Instance_To_Index(UINT32_MAX), Load_Control_Count(), NULL);

    zassert_true(Load_Control_Valid_Instance(0), NULL);
    zassert_equal(Load_Control_Index_To_Instance(0), 0, NULL);
    zassert_equal(Load_Control_Instance_To_Index(0), 0, NULL);

    zassert_false(Load_Control_Object_Name(0, NULL), NULL);
    zassert_false(Load_Control_Object_Name(UINT32_MAX, &object_name_st), NULL);

    zassert_true(Load_Control_Object_Name(0, &object_name_st), NULL);
    zassert_true(characterstring_valid(&object_name_st), NULL);
    zassert_true(characterstring_printable(&object_name_st), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lc_tests, test_Load_Control_Read_Write_Property)
#else
static void test_Load_Control_Read_Write_Property(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    object_instance = Load_Control_Index_To_Instance(0);
    Load_Control_Init();
    count = Load_Control_Count();
    zassert_true(count > 0, NULL);
    test_object_instance = Load_Control_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_LOAD_CONTROL, object_instance, Load_Control_Property_Lists,
        Load_Control_Read_Property, Load_Control_Write_Property,
        skip_fail_property_list);
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
        lc_tests, ztest_unit_test(test_api_stubs),
        ztest_unit_test(test_Load_Control_Count),
        ztest_unit_test(test_Load_Control_Read_Write_Property),
        ztest_unit_test(testLoadControlStateMachine),
        ztest_unit_test(test_ShedInactive_gets_RcvShedRequests));

    ztest_run_test_suite(lc_tests);
}
#endif
