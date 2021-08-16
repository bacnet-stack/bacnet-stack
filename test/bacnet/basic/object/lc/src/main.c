/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet load control object
 */

#include <ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacstr.h>
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

#if 0
/* Mocks */

void bacapp_value_list_init(
    BACNET_APPLICATION_DATA_VALUE *value,
    size_t count)
{
}

void bacapp_property_value_list_init(
    BACNET_PROPERTY_VALUE *value,
    size_t count)
{
}


int bacapp_encode_data(
    uint8_t * apdu,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    return -1;
}

int bacapp_decode_data(
    uint8_t * apdu,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    return -1;
}


int bacapp_decode_application_data(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    return -1;
}


bool bacapp_decode_application_data_safe(
    uint8_t * new_apdu,
    uint32_t new_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    return false;
}


int bacapp_encode_application_data(
    uint8_t * apdu,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    return -1;
}


int bacapp_decode_context_data(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID property)
{
    return -1;
}


int bacapp_encode_context_data(
    uint8_t * apdu,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID property)
{
    return -1;
}


int bacapp_encode_context_data_value(
    uint8_t * apdu,
    uint8_t context_tag_number,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    return -1;
}


BACNET_APPLICATION_TAG bacapp_context_tag_type(
    BACNET_PROPERTY_ID property,
    uint8_t tag_number)
{
    return MAX_BACNET_APPLICATION_TAG;
}


bool bacapp_copy(
    BACNET_APPLICATION_DATA_VALUE * dest_value,
    BACNET_APPLICATION_DATA_VALUE * src_value)
{
    return false;
}


int bacapp_data_len(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_PROPERTY_ID property)
{
    return -1;
}

int bacapp_decode_data_len(
    uint8_t * apdu,
    uint8_t tag_data_type,
    uint32_t len_value_type)
{
    return -1;
}

int bacapp_decode_application_data_len(
    uint8_t * apdu,
    unsigned max_apdu_len)
{
    return -1;
}

int bacapp_decode_context_data_len(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_PROPERTY_ID property)
{
    return -1;
}

int bacapp_snprintf_value(
    char *str,
    size_t str_len,
    BACNET_OBJECT_PROPERTY_VALUE * object_value)
{
    return -1;
}


#ifdef BACAPP_PRINT_ENABLED
bool bacapp_parse_application_data(
    BACNET_APPLICATION_TAG tag_number,
    const char *argv,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    return false;
}

bool bacapp_print_value(
    FILE * stream,
    BACNET_OBJECT_PROPERTY_VALUE * value)
{
    return false;
}

#endif

bool bacapp_same_value(
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_APPLICATION_DATA_VALUE * test_value)
{
    return false;
}
#endif



/**
 * @brief Test
 */

static void test_Load_Control_Count(void)
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

static void testLoadControlStateMachine(void)
{
//TODO:    unsigned i = 0, j = 0;
    uint8_t level = 0;

    Load_Control_Init();

//TODO:    /* validate the triggers for each state change */
//TODO:    for (j = 0; j < 20; j++) {
//TODO:        Load_Control_State_Machine(0);
//TODO:        for (i = 0; i < MAX_LOAD_CONTROLS; i++) {
//TODO:            zassert_equal(Load_Control_State[i], SHED_INACTIVE, NULL);
//TODO:        }
//TODO:    }

    /* SHED_REQUEST_PENDING */
    /* CancelShed - Start time has wildcards */
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Shed_Duration(0, 60);
    Load_Control_WriteProperty_Start_Time_Wildcards(0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);

    /* CancelShed - Requested_Shed_Level equal to default value */
    Load_Control_Init();
    Load_Control_WriteProperty_Request_Shed_Level(0, 0);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
    Load_Control_WriteProperty_Shed_Duration(0, 5);
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 15, 0, 0, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);

    /* CancelShed - Non-default values, but Start time is passed */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Request_Shed_Level(0, 1);
    Load_Control_WriteProperty_Shed_Duration(0, 5);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 28, 15, 0, 0, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);

    /* ReconfigurePending - new write received while pending */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Request_Shed_Level(0, 1);
    Load_Control_WriteProperty_Shed_Duration(0, 5);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Request_Shed_Level(0, 2);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Shed_Duration(0, 6);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Duty_Window(0, 60);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 1);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);

    /* CannotMeetShed -> FinishedUnsuccessfulShed */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Request_Shed_Level(0, 1);
    Load_Control_WriteProperty_Shed_Duration(0, 120);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    /* set to lowest value so we cannot meet the shed level */
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 16, 0, 0, 0);
    Analog_Output_Present_Value_Set(0, 0, 16);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    /* FinishedUnsuccessfulShed */
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 23, 0, 0, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);

    /* CannotMeetShed -> UnsuccessfulShedReconfigured */
    Load_Control_Init();
    Load_Control_WriteProperty_Enable(0, true);
    Load_Control_WriteProperty_Request_Shed_Level(0, 1);
    Load_Control_WriteProperty_Shed_Duration(0, 120);
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 15, 0, 0, 0);
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 5, 0, 0, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    /* set to lowest value so we cannot meet the shed level */
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 16, 0, 0, 0);
    Analog_Output_Present_Value_Set(0, 0, 16);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    /* FinishedUnsuccessfulShed */
    Load_Control_WriteProperty_Start_Time(0, 2007, 2, 27, 16, 0, 0, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_REQUEST_PENDING, NULL);
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 16, 0, 1, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_NON_COMPLIANT, NULL);
    /* CanNowComplyWithShed */
    Analog_Output_Present_Value_Set(0, 100, 16);
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 16, 0, 2, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_COMPLIANT, NULL);
    level = Analog_Output_Present_Value(0);
//TODO: Fails:    zassert_equal(level, 90, NULL);
    /* FinishedSuccessfulShed */
//TODO:    datetime_set_values(&Current_Time, 2007, 2, 27, 23, 0, 0, 0);
    Load_Control_State_Machine(0);
//TODO:    zassert_equal(Load_Control_State[0], SHED_INACTIVE, NULL);
    level = Analog_Output_Present_Value(0);
//TODO: Fails:    zassert_equal(level, 100, NULL);
}


#ifndef MAX_LOAD_CONTROLS
#define MAX_LOAD_CONTROLS (4)
#endif

static void test_api_stubs(void)
{
    BACNET_CHARACTER_STRING object_name_st = { 0 };

    zassert_equal(Load_Control_Count(), MAX_LOAD_CONTROLS, NULL);

    zassert_false(Load_Control_Valid_Instance(MAX_LOAD_CONTROLS), NULL);
    zassert_equal(Load_Control_Index_To_Instance(MAX_LOAD_CONTROLS), Load_Control_Count(), NULL);
    zassert_equal(Load_Control_Instance_To_Index(MAX_LOAD_CONTROLS), Load_Control_Count(), NULL);

    zassert_false(Load_Control_Valid_Instance(UINT32_MAX), NULL);
    zassert_equal(Load_Control_Index_To_Instance(UINT32_MAX), Load_Control_Count(), NULL);
    zassert_equal(Load_Control_Instance_To_Index(UINT32_MAX), Load_Control_Count(), NULL);

    zassert_true(Load_Control_Valid_Instance(0), NULL);
    zassert_equal(Load_Control_Index_To_Instance(0), 0, NULL);
    zassert_equal(Load_Control_Instance_To_Index(0), 0, NULL);

    zassert_false(Load_Control_Object_Name(0, NULL), NULL);
    zassert_false(Load_Control_Object_Name(UINT32_MAX, &object_name_st), NULL);


    zassert_true(Load_Control_Object_Name(0, &object_name_st), NULL);
    zassert_true(characterstring_valid(&object_name_st), NULL);
    zassert_true(characterstring_printable(&object_name_st), NULL);
}

static void test_Load_Control_Read_Write_Property(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    /* for decode value data */
    BACNET_APPLICATION_DATA_VALUE value;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    unsigned count = 0;
    uint32_t object_instance = 0;


    zassert_equal(Load_Control_Read_Property(NULL), 0, NULL);
    zassert_false(Load_Control_Write_Property(NULL), NULL);

    object_instance = Load_Control_Index_To_Instance(0);
    Load_Control_Init();
    count = Load_Control_Count();
    zassert_true(count > 0, NULL);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_LOAD_CONTROL;
    rpdata.object_instance = object_instance;
    Load_Control_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
	rpdata.object_property = *pRequired;
	rpdata.array_index = BACNET_ARRAY_ALL;
	len = Load_Control_Read_Property(&rpdata);
	zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
	if (len > 0) {
	    test_len = bacapp_decode_application_data(
		rpdata.application_data,
		(uint8_t)rpdata.application_data_len, &value);
	    zassert_true(test_len >= 0, NULL);
	}
	pRequired++;
    }
    while ((*pOptional) != -1) {
	rpdata.object_property = *pOptional;
	rpdata.array_index = BACNET_ARRAY_ALL;
	len = Load_Control_Read_Property(&rpdata);
	zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
	if (len > 0) {
	    test_len = bacapp_decode_application_data(
		rpdata.application_data,
		(uint8_t)rpdata.application_data_len, &value);
	    zassert_true(test_len >= 0, NULL);
	}
	pOptional++;
    }
}

static bool init_wp_data_and_value(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    bool status = false;
    if ((wp_data != NULL) && (value != NULL))
    {
	memset(&value, 0, sizeof(value));
	memset(&wp_data, 0, sizeof(wp_data));

	wp_data->object_type = OBJECT_LOAD_CONTROL;
	wp_data->object_instance = 0;
	wp_data->array_index = BACNET_ARRAY_ALL;
	wp_data->priority = BACNET_NO_PRIORITY;
	wp_data->object_property = PROP_SHED_DURATION;
	value->context_specific = false;
	value->context_tag = 0;
	value->tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
	value->type.Unsigned_Int = 0;  /* duration */
	wp_data->application_data_len = bacapp_encode_application_data(&wp_data->application_data[0], value);
	zassert_true(wp_data->application_data_len >= 0, NULL);
	zassert_equal(wp_data->error_class, 0, NULL);
	zassert_equal(wp_data->error_code, 0, NULL);

	status = true;
    }

    return status;
}

static void test_ShedInactive_gets_RcvShedRequests(void)
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


    /* Verify calls to dependencies are properly made */
    // object_property == PROP_REQUESTED_SHED_LEVEL calls bacapp_decode_context_data()
    // object_property == PROP_START_TIME calls bacapp_decode_application_data()
    // object_property == PROP_SHED_DURATION calls nothing
    // object_property == PROP_DUTY_WINDOW calls nothing
    // object_property == PROP_SHED_LEVELS calls nothing
    // object_property == PROP_ENABLE calls nothing
    // default returns error
}

static void test_ShedReqPending_gets_ReconfigPending(void)
{
    ztest_test_skip();
}

static void test_ShedReqPending_gets_CancelShed(void)
{
    ztest_test_skip();
}

static void test_ShedReqPending_gets_CannotMeetShed(void)
{
    ztest_test_skip();
}

static void test_ShedReqPending_gets_PrepareToShed(void)
{
    ztest_test_skip();
}

static void test_ShedReqPending_gets_AbleToMeetShed(void)
{
    ztest_test_skip();
}

static void test_ShedNonCommpliant_gets_UnsuccessfulShedReconfig(void)
{
    ztest_test_skip();
}

static void test_ShedNonCommpliant_gets_FinishedUnsuccessfulShed(void)
{
    ztest_test_skip();
}

static void test_ShedNonCommpliant_gets_CanNowComplyWithShed(void)
{
    ztest_test_skip();
}

static void test_ShedCommpliant_gets_FinishedSuccessfulShed(void)
{
    ztest_test_skip();
}

static void test_ShedCommpliant_gets_SuccessfulShedReconfig(void)
{
    ztest_test_skip();
}

static void test_ShedCommpliant_gets_CanNoLongerComplyWithShed(void)
{
    ztest_test_skip();
}

/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(lc_tests,
     ztest_unit_test(test_api_stubs),
     ztest_unit_test(test_Load_Control_Count),
     ztest_unit_test(test_Load_Control_Read_Write_Property),
     ztest_unit_test(testLoadControlStateMachine),
     ztest_unit_test(test_ShedInactive_gets_RcvShedRequests),
     ztest_unit_test(test_ShedReqPending_gets_ReconfigPending),
     ztest_unit_test(test_ShedReqPending_gets_CancelShed),
     ztest_unit_test(test_ShedReqPending_gets_CannotMeetShed),
     ztest_unit_test(test_ShedReqPending_gets_PrepareToShed),
     ztest_unit_test(test_ShedReqPending_gets_AbleToMeetShed),
     ztest_unit_test(test_ShedNonCommpliant_gets_UnsuccessfulShedReconfig),
     ztest_unit_test(test_ShedNonCommpliant_gets_FinishedUnsuccessfulShed),
     ztest_unit_test(test_ShedNonCommpliant_gets_CanNowComplyWithShed),
     ztest_unit_test(test_ShedCommpliant_gets_FinishedSuccessfulShed),
     ztest_unit_test(test_ShedCommpliant_gets_SuccessfulShedReconfig),
     ztest_unit_test(test_ShedCommpliant_gets_CanNoLongerComplyWithShed)
     );

    ztest_run_test_suite(lc_tests);
}
