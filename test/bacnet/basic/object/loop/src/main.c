/**
 * @file
 * @brief Unit test for Loop object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/loop.h>
#include <bacnet/bactext.h>
#include <bacnet/list_element.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */
static BACNET_WRITE_PROPERTY_DATA Write_Property_Internal_Data;
static bool Write_Property_Internal(BACNET_WRITE_PROPERTY_DATA *data)
{
    memcpy(
        &Write_Property_Internal_Data, data,
        sizeof(BACNET_WRITE_PROPERTY_DATA));

    return true;
}

static BACNET_READ_PROPERTY_DATA Read_Property_Internal_Data;
static int Read_Property_Internal_Length;
static int Read_Property_Internal(BACNET_READ_PROPERTY_DATA *data)
{
    memcpy(
        &Read_Property_Internal_Data, data, sizeof(BACNET_READ_PROPERTY_DATA));

    return Read_Property_Internal_Length;
}

static struct loop_write_property_notification Write_Property_Notification;
static BACNET_WRITE_PROPERTY_DATA Write_Property_Notification_Data;
static uint32_t Write_Property_Notification_Instance;
static bool Write_Property_Notification_Status;
static void Loop_Write_Property_Notification_Callback(
    uint32_t instance, bool status, BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    Write_Property_Notification_Instance = instance;
    Write_Property_Notification_Status = status;
    memcpy(
        &Write_Property_Notification_Data, wp_data,
        sizeof(BACNET_WRITE_PROPERTY_DATA));
}

static int Proprietary_Properties[] = { 512, 513, -1 };
static uint8_t Proprietary_Serial_Number[16];

/**
 * @brief WriteProperty handler for this objects proprietary properties.
 * For the given WriteProperty data, the application_data is loaded
 * or the error code and class are set and the return value is false.
 * @param  data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return false if an error is loaded, true if no errors
 */
static bool Write_Property_Proprietary(BACNET_WRITE_PROPERTY_DATA *data)
{
    bool status = false;
    int apdu_len = 0;
    uint8_t *apdu = NULL;
    size_t apdu_size = 0;
    BACNET_OCTET_STRING octet_value = { 0 };

    switch ((int)data->object_property) {
        case 512:
            apdu_len = bacnet_octet_string_application_decode(
                apdu, apdu_size, &octet_value);
            if (apdu_len > 0) {
                octetstring_copy_value(
                    Proprietary_Serial_Number,
                    sizeof(Proprietary_Serial_Number), &octet_value);
                status = true;
            } else if (apdu_len == 0) {
                status = false;
                data->error_class = ERROR_CLASS_PROPERTY;
                data->error_code = ERROR_CODE_INVALID_DATA_TYPE;

            } else {
                status = false;
                data->error_class = ERROR_CLASS_PROPERTY;
                data->error_code = ERROR_CODE_INVALID_DATA_ENCODING;
            }
            break;
        case 513:
        default:
            status = false;
            data->error_class = ERROR_CLASS_PROPERTY;
            data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return false;
}

/**
 * @brief ReadProperty handler for this objects proprietary properties.
 * For the given ReadProperty data, the application_data is loaded
 * or the error code and class are set and the return value is
 * BACNET_STATUS_ERROR.
 * @param  data - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return false if an error is loaded, true if no errors
 */
static int Read_Property_Proprietary(BACNET_READ_PROPERTY_DATA *data)
{
    int apdu_len = 0;
    uint8_t *apdu = NULL;
    size_t apdu_size = 0;
    BACNET_OCTET_STRING octet_value = { 0 };
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    if ((data == NULL) || (data->application_data == NULL) ||
        (data->application_data_len == 0)) {
        return 0;
    }
    /* none of our proprietary properties are arrays */
    if (data->array_index != BACNET_ARRAY_ALL) {
        data->error_class = ERROR_CLASS_PROPERTY;
        data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return BACNET_STATUS_ERROR;
    }
    apdu = data->application_data;
    apdu_size = data->application_data_len;
    switch ((int)data->object_property) {
        case 512:
            octetstring_init(
                &octet_value, Proprietary_Serial_Number,
                sizeof(Proprietary_Serial_Number));
            apdu_len = bacnet_octet_string_application_encode(
                apdu, apdu_size, &octet_value);
            break;
        case 513:
            unsigned_value = Loop_Size();
            apdu_len = bacnet_unsigned_application_encode(
                apdu, apdu_size, unsigned_value);
            break;
        default:
            data->error_class = ERROR_CLASS_PROPERTY;
            data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/**
 * @brief Test
 */
static void test_Loop_Read_Write(void)
{
    const uint32_t instance = 123;
    unsigned count = 0;
    unsigned index = 0;
    const char *sample_name = "Loop:0";
    char *sample_context = "context";
    const char *sample_description = "Loop Description";
    const char *test_name = NULL;
    uint32_t test_instance = 0;
    bool status = false;
    const int32_t skip_fail_property_list[] = { -1 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_CHARACTER_STRING cstring = { 0 };
    int32_t i = 0,
            units_properties[] = { PROP_OUTPUT_UNITS,
                                   PROP_CONTROLLED_VARIABLE_UNITS,
                                   PROP_PROPORTIONAL_CONSTANT_UNITS,
                                   PROP_INTEGRAL_CONSTANT_UNITS,
                                   PROP_DERIVATIVE_CONSTANT_UNITS,
                                   -1 },
            real_properties[] = { PROP_PRESENT_VALUE,
                                  PROP_CONTROLLED_VARIABLE_VALUE,
                                  PROP_SETPOINT,
                                  PROP_PROPORTIONAL_CONSTANT,
                                  PROP_INTEGRAL_CONSTANT,
                                  PROP_DERIVATIVE_CONSTANT,
                                  PROP_BIAS,
                                  PROP_MAXIMUM_OUTPUT,
                                  PROP_MINIMUM_OUTPUT,
                                  PROP_COV_INCREMENT,
                                  -1 };

    Loop_Init();
    Loop_Create(instance);
    status = Loop_Valid_Instance(instance);
    zassert_true(status, NULL);
    status = Loop_Valid_Instance(instance - 1);
    zassert_false(status, NULL);
    index = Loop_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    test_instance = Loop_Index_To_Instance(index);
    zassert_equal(instance, test_instance, NULL);
    count = Loop_Count();
    zassert_true(count > 0, NULL);
    /* reliability and status flags */
    status = Loop_Reliability_Set(instance, RELIABILITY_PROCESS_ERROR);
    zassert_true(status, NULL);
    /* add some proprietary properties */
    Loop_Proprietary_Property_List_Set(Proprietary_Properties);
    Loop_Read_Property_Proprietary_Callback_Set(Read_Property_Proprietary);
    Loop_Write_Property_Proprietary_Callback_Set(Write_Property_Proprietary);
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_LOOP, instance, Loop_Property_Lists, Loop_Read_Property,
        Loop_Write_Property, skip_fail_property_list);
    /* test the ASCII name get/set */
    status = Loop_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Loop_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Loop_Object_Name(instance, &cstring);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&cstring, sample_name);
    zassert_true(status, NULL);
    status = Loop_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Loop_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);
    /* test specific WriteProperty values - common configuration */
    wp_data.object_type = OBJECT_LOOP;
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
    status = Loop_Write_Property(&wp_data);
    zassert_true(status, NULL);
    value.type.Boolean = false;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_true(status, NULL);
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 123;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_false(status, NULL);
    /* write present-value */
    wp_data.object_property = PROP_PRESENT_VALUE;
    value.tag = BACNET_APPLICATION_TAG_REAL;
    value.type.Real = 1.0f;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_true(status, NULL);
    /* write minimum-output and maximum-output */
    wp_data.object_property = PROP_MINIMUM_OUTPUT;
    value.tag = BACNET_APPLICATION_TAG_REAL;
    value.type.Real = 1.0f;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_MAXIMUM_OUTPUT;
    value.tag = BACNET_APPLICATION_TAG_REAL;
    value.type.Real = 100.0f;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_true(status, NULL);
    /* action - out of range error */
    wp_data.object_property = PROP_ACTION;
    value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value.type.Enumerated = BACNET_ACTION_MAX;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
    /* units - out of range error */
    i = 0;
    while (units_properties[i] != -1) {
        wp_data.object_property = units_properties[i];
        value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
        value.type.Enumerated = UINT16_MAX + 1UL;
        wp_data.application_data_len =
            bacapp_encode_application_data(wp_data.application_data, &value);
        status = Loop_Write_Property(&wp_data);
        zassert_false(status, NULL);
        zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
        zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
        i++;
    }
    /* REAL - out of range error */
    i = 0;
    while (real_properties[i] != -1) {
        wp_data.object_property = real_properties[i];
        value.tag = BACNET_APPLICATION_TAG_REAL;
        value.type.Real = NAN;
        wp_data.application_data_len =
            bacapp_encode_application_data(wp_data.application_data, &value);
        status = Loop_Write_Property(&wp_data);
        zassert_false(status, NULL);
        zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
        zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
        i++;
    }
    /* priority-for-writing - out of range error */
    wp_data.object_property = PROP_PRIORITY_FOR_WRITING;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = BACNET_MIN_PRIORITY;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_true(status, NULL);
    value.type.Unsigned_Int = BACNET_MAX_PRIORITY + 1ULL;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
    value.type.Unsigned_Int = UINT8_MAX + 1ULL;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
    /* read-only property */
    wp_data.object_property = PROP_OBJECT_TYPE;
    value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value.type.Enumerated = OBJECT_ANALOG_INPUT;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_MAX_PRIORITY;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Loop_Write_Property(&wp_data);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);
    zassert_false(status, NULL);
    /* == API testing where not already tested by read or write property == */
    /* reliability and status flags API */
    status = Loop_Reliability_Set(instance, RELIABILITY_PROCESS_ERROR);
    zassert_true(status, NULL);
    /* context API */
    Loop_Context_Set(instance, sample_context);
    zassert_true(sample_context == Loop_Context_Get(instance), NULL);
    zassert_true(NULL == Loop_Context_Get(instance + 1), NULL);
    /* description API */
    status = Loop_Description_Set(instance, sample_description);
    zassert_true(status, NULL);
    zassert_equal(sample_description, Loop_Description_ANSI(instance), NULL);
    status = Loop_Description(instance, &cstring);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&cstring, sample_description);
    zassert_true(status, NULL);
    status = Loop_Description_Set(instance, NULL);
    zassert_true(status, NULL);
    status = characterstring_init_ansi(&cstring, "");
    zassert_true(status, NULL);
    status =
        characterstring_ansi_same(&cstring, Loop_Description_ANSI(instance));
    zassert_true(status, NULL);
    /* cleanup */
    status = Loop_Delete(instance);
    zassert_true(status, NULL);
    Loop_Cleanup();
}

/**
 * @brief Test
 */
static void test_Loop_Operation(void)
{
    const uint32_t instance = 123;
    uint32_t test_instance = 0;
    bool status = false;
    uint32_t elapsed_time = 0;
    BACNET_OBJECT_PROPERTY_REFERENCE reference = { 0 };
    BACNET_APPLICATION_DATA_VALUE test_value = { 0 };
    int len = 0;

    /* init */
    Loop_Init();
    Loop_Create(instance);
    status = Loop_Valid_Instance(instance);
    zassert_true(status, NULL);
    /* set Kp, Ki, Kd, setpoint, etc */
    /* connect the read and write property callbacks */
    Loop_Write_Property_Internal_Callback_Set(Write_Property_Internal);
    Loop_Read_Property_Internal_Callback_Set(Read_Property_Internal);
    Write_Property_Notification.callback =
        Loop_Write_Property_Notification_Callback;
    Write_Property_Notification.next = NULL;
    Loop_Write_Property_Notification_Add(&Write_Property_Notification);
    /* run the PID loop */
    Loop_Timer(instance, elapsed_time);
    elapsed_time += 1000;
    Loop_Timer(instance, elapsed_time);
    Loop_Update_Interval_Set(instance, 100);
    elapsed_time += 100;
    Loop_Timer(instance, elapsed_time);
    elapsed_time += 100;
    Loop_Timer(instance, elapsed_time);
    elapsed_time += 100;
    Loop_Timer(instance, elapsed_time);
    /* references - test by referencing self */
    reference.object_identifier.instance = instance;
    reference.object_identifier.type = OBJECT_LOOP;
    reference.property_array_index = BACNET_ARRAY_ALL;
    reference.property_identifier = PROP_CONTROLLED_VARIABLE_VALUE;
    Loop_Controlled_Variable_Reference_Set(instance, &reference);
    reference.property_identifier = PROP_SETPOINT;
    Loop_Setpoint_Reference_Set(instance, &reference);
    reference.property_identifier = PROP_PRESENT_VALUE;
    Loop_Manipulated_Variable_Reference_Set(instance, &reference);
    elapsed_time += 100;
    Loop_Timer(instance, elapsed_time);
    /* references - test by referencing another internal object */
    reference.object_identifier.type = OBJECT_ANALOG_OUTPUT;
    reference.property_identifier = PROP_PRESENT_VALUE;
    Loop_Manipulated_Variable_Reference_Set(instance, &reference);
    elapsed_time += 100;
    Loop_Timer(instance, elapsed_time);
    /* verify that the internal read/write property callbacks were used */
    zassert_equal(
        Write_Property_Internal_Data.object_type, OBJECT_ANALOG_OUTPUT,
        "WriteProperty=%s:%d",
        bactext_object_type_name(Write_Property_Internal_Data.object_type),
        Write_Property_Internal_Data.object_instance);
    zassert_equal(
        Write_Property_Internal_Data.object_instance, instance,
        "WriteProperty=%s:%d",
        bactext_object_type_name(Write_Property_Internal_Data.object_type),
        Write_Property_Internal_Data.object_instance);
    zassert_equal(
        Write_Property_Internal_Data.object_property, PROP_PRESENT_VALUE,
        "WriteProperty=%s:%d %s",
        bactext_object_type_name(Write_Property_Internal_Data.object_type),
        Write_Property_Internal_Data.object_instance,
        bactext_property_name(Write_Property_Internal_Data.object_property));
    len = bacapp_decode_application_data(
        Write_Property_Internal_Data.application_data,
        Write_Property_Internal_Data.application_data_len, &test_value);
    zassert_true(len > 0, "len=%d", len);
    zassert_equal(Write_Property_Notification_Instance, instance, NULL);
    zassert_equal(Write_Property_Notification_Status, true, NULL);
    zassert_equal(
        Write_Property_Notification_Data.object_property, PROP_PRESENT_VALUE,
        NULL);
    len = bacapp_decode_application_data(
        Write_Property_Notification_Data.application_data,
        Write_Property_Notification_Data.application_data_len, &test_value);
    zassert_true(len > 0, "len=%d", len);
    /* cleanup instance */
    status = Loop_Delete(instance);
    zassert_true(status, NULL);
    /* test create of next instance */
    test_instance = Loop_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(test_instance, BACNET_MAX_INSTANCE, NULL);
    test_instance = Loop_Create(test_instance);
    zassert_not_equal(test_instance, BACNET_MAX_INSTANCE, NULL);
    test_instance = Loop_Create(BACNET_MAX_INSTANCE + 1);
    zassert_equal(test_instance, BACNET_MAX_INSTANCE, NULL);
    /* cleanup all */
    Loop_Cleanup();
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(
        loop_tests, ztest_unit_test(test_Loop_Read_Write),
        ztest_unit_test(test_Loop_Operation));

    ztest_run_test_suite(loop_tests);
}
