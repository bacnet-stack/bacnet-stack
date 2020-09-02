/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief test BACnet load control object
 */

#include <ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/basic/object/ao.h>
#include <bacnet/basic/object/lc.h>

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
#if 0 /* TODO: How should this get exposed? */
static void Load_Control_WriteProperty_Request_Shed_Level(
    int instance, unsigned level)
{
    bool status = false;
    BACNET_APPLICATION_DATA_VALUE value;
    BACNET_WRITE_PROPERTY_DATA wp_data;

    wp_data.object_type = OBJECT_LOAD_CONTROL;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.object_property = PROP_REQUESTED_SHED_LEVEL;
    value.context_specific = true;
    value.context_tag = 1;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = level;
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}
#endif

#if 0 /* TODO: How should this get exposed? */
static void Load_Control_WriteProperty_Enable(
    int instance, bool enable)
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
        bacapp_encode_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}
#endif

#if 0 /* TODO: How should this get exposed? */
static void Load_Control_WriteProperty_Shed_Duration(
    int instance, unsigned duration)
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
        bacapp_encode_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}
#endif

#if 0 /* TODO: How should this get exposed? */
static void Load_Control_WriteProperty_Duty_Window(
    int instance, unsigned duration)
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
        bacapp_encode_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}
#endif

#if 0 /* TODO: How should this get exposed? */
static void Load_Control_WriteProperty_Start_Time_Wildcards(
    int instance)
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
    value.context_specific = false;
    value.context_tag = 0;
    value.tag = BACNET_APPLICATION_TAG_DATE;
    datetime_date_wildcard_set(&value.type.Date);
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    len = wp_data.application_data_len;
    value.tag = BACNET_APPLICATION_TAG_TIME;
    datetime_time_wildcard_set(&value.type.Time);
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[len], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    wp_data.application_data_len += len;
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}
#endif

#if 0 /* TODO: How should this get exposed? */
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
    value.context_specific = false;
    value.context_tag = 0;
    value.tag = BACNET_APPLICATION_TAG_DATE;
    datetime_set_date(&value.type.Date, year, month, day);
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    len = wp_data.application_data_len;
    value.tag = BACNET_APPLICATION_TAG_TIME;
    datetime_set_time(&value.type.Time, hour, minute, seconds, hundredths);
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[len], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    wp_data.application_data_len += len;
    status = Load_Control_Write_Property(&wp_data);
    zassert_true(status, NULL);
}
#endif

static void testLoadControlStateMachine(void)
{
#if 0 /*TODO: Need visiblity inside LoadControlStateMachine */
    unsigned i = 0, j = 0;
    uint8_t level = 0;

    Load_Control_Init();
    /* validate the triggers for each state change */
    for (j = 0; j < 20; j++) {
        Load_Control_State_Machine(0);
        for (i = 0; i < MAX_LOAD_CONTROLS; i++) {
            zassert_equal(Load_Control_State[i], SHED_INACTIVE, NULL);
        }
    }
    /* SHED_REQUEST_PENDING */
    /* CancelShed - Start time has wildcards */
    Load_Control_WriteProperty_Enable(pTest, 0, true);
    Load_Control_WriteProperty_Shed_Duration(pTest, 0, 60);
    Load_Control_WriteProperty_Start_Time_Wildcards(pTest, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);

    /* CancelShed - Requested_Shed_Level equal to default value */
    Load_Control_Init();
    Load_Control_WriteProperty_Request_Shed_Level(pTest, 0, 0);
    Load_Control_WriteProperty_Start_Time(pTest, 0, 2007, 2, 27, 15, 0, 0, 0);
    Load_Control_WriteProperty_Shed_Duration(pTest, 0, 5);
    datetime_set_values(&Current_Time, 2007, 2, 27, 15, 0, 0, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);

    /* CancelShed - Non-default values, but Start time is passed */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(pTest, 0, true);
    Load_Control_WriteProperty_Request_Shed_Level(pTest, 0, 1);
    Load_Control_WriteProperty_Shed_Duration(pTest, 0, 5);
    Load_Control_WriteProperty_Start_Time(pTest, 0, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&Current_Time, 2007, 2, 28, 15, 0, 0, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);

    /* ReconfigurePending - new write received while pending */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(pTest, 0, true);
    Load_Control_WriteProperty_Request_Shed_Level(pTest, 0, 1);
    Load_Control_WriteProperty_Shed_Duration(pTest, 0, 5);
    Load_Control_WriteProperty_Start_Time(pTest, 0, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&Current_Time, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Request_Shed_Level(pTest, 0, 2);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Shed_Duration(pTest, 0, 6);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Duty_Window(pTest, 0, 60);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Start_Time(pTest, 0, 2007, 2, 27, 15, 0, 0, 1);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);

    /* CannotMeetShed -> FinishedUnsuccessfulShed */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(pTest, 0, true);
    Load_Control_WriteProperty_Request_Shed_Level(pTest, 0, 1);
    Load_Control_WriteProperty_Shed_Duration(pTest, 0, 120);
    Load_Control_WriteProperty_Start_Time(pTest, 0, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&Current_Time, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    /* set to lowest value so we cannot meet the shed level */
    datetime_set_values(&Current_Time, 2007, 2, 27, 16, 0, 0, 0);
    Analog_Output_Present_Value_Set(0, 0, 16);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    /* FinishedUnsuccessfulShed */
    datetime_set_values(&Current_Time, 2007, 2, 27, 23, 0, 0, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);

    /* CannotMeetShed -> UnsuccessfulShedReconfigured */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(pTest, 0, true);
    Load_Control_WriteProperty_Request_Shed_Level(pTest, 0, 1);
    Load_Control_WriteProperty_Shed_Duration(pTest, 0, 120);
    Load_Control_WriteProperty_Start_Time(pTest, 0, 2007, 2, 27, 15, 0, 0, 0);
    datetime_set_values(&Current_Time, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    /* set to lowest value so we cannot meet the shed level */
    datetime_set_values(&Current_Time, 2007, 2, 27, 16, 0, 0, 0);
    Analog_Output_Present_Value_Set(0, 0, 16);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    /* FinishedUnsuccessfulShed */
    Load_Control_WriteProperty_Start_Time(pTest, 0, 2007, 2, 27, 16, 0, 0, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    datetime_set_values(&Current_Time, 2007, 2, 27, 16, 0, 1, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    /* CanNowComplyWithShed */
    Analog_Output_Present_Value_Set(0, 100, 16);
    datetime_set_values(&Current_Time, 2007, 2, 27, 16, 0, 2, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_COMPLIANT, NULL);
    level = Analog_Output_Present_Value(0);
    zassert_equal(level, 90, NULL);
    /* FinishedSuccessfulShed */
    datetime_set_values(&Current_Time, 2007, 2, 27, 23, 0, 0, 0);
    Load_Control_State_Machine(0);
    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);
    level = Analog_Output_Present_Value(0);
    zassert_equal(level, 100, NULL);
#else
    ztest_test_skip();
#endif
}

static void testLoadControl(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = 0;
    uint32_t decoded_instance = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Analog_Output_Init();
    Load_Control_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_LOAD_CONTROL;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Load_Control_Read_Property(&rpdata);
    zassert_true(len != 0, NULL);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_OBJECT_ID, NULL);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    zassert_equal(decoded_type, rpdata.object_type, NULL);
    zassert_equal(decoded_instance, rpdata.object_instance, NULL);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(lc_tests,
     ztest_unit_test(testLoadControl),
     ztest_unit_test(testLoadControlStateMachine)
     );

    ztest_run_test_suite(lc_tests);
}
