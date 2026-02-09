/**
 * @file
 * @brief Unit test for CreateObject service encode and decode API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @section LICENSE
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdest.h>
#include <bacnet/create_object.h>
#include <bacnet/bactext.h>

static uint32_t Test_Create_Object_Returned_Instance = BACNET_MAX_INSTANCE;
static uint32_t Test_Delete_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_WRITE_PROPERTY_DATA Test_Write_Property_Data;
static bool Test_Write_Property_Return_Status;
/**
 * @brief CreateObject service handler for an object
 * @ingroup ObjHelpers
 * @param object_instance [in] instance number of the object to create,
 *  or BACNET_MAX_INSTANCE to create the next free object instance
 * @return object instance number created, or BACNET_MAX_INSTANCE if not
 */
static uint32_t test_create_object_function(uint32_t object_instance)
{
    (void)object_instance;
    return Test_Create_Object_Returned_Instance;
}

/**
 * @brief DeleteObject service handler for an object
 * @ingroup ObjHelpers
 * @param object_instance [in] instance number of the object to delete
 * @return true if the object instance is deleted
 */
static bool test_delete_object_function(uint32_t object_instance)
{
    Test_Delete_Object_Instance = object_instance;
    return true;
}

/** Attempts to write a new value to one property for this object type
 *  of a given instance.
 * A function template; @see device.c for assignment to object types.
 * @ingroup ObjHelpers
 *
 * @param wp_data [in] Pointer to the BACnet_Write_Property_Data structure,
 *                     which is packed with the information from the WP request.
 * @return The length of the apdu encoded or -1 for error or
 *         -2 for abort message.
 */
static bool test_write_property_function(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    Test_Write_Property_Data = *wp_data;
    return Test_Write_Property_Return_Status;
}

static void test_CreateObjectCodec(BACNET_CREATE_OBJECT_DATA *data)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_CREATE_OBJECT_DATA test_data = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;
    bool object_supported = false, object_exists = false, status = false;

    test_len = create_object_service_request_encode(NULL, 0, data);
    zassert_equal(test_len, 0, NULL);
    null_len = create_object_service_request_encode(NULL, sizeof(apdu), data);
    apdu_len = create_object_service_request_encode(apdu, sizeof(apdu), data);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len != BACNET_STATUS_ERROR, NULL);
    null_len = create_object_decode_service_request(apdu, apdu_len, NULL);
    test_len = create_object_decode_service_request(apdu, apdu_len, &test_data);
    zassert_equal(test_len, null_len, NULL);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    while (test_len) {
        test_len--;
        len = create_object_decode_service_request(apdu, test_len, &test_data);
        if (data->object_instance == BACNET_MAX_INSTANCE) {
            if (test_len == 4) {
                /* optional list-of-values */
                continue;
            }
        } else {
            if (test_len == 7) {
                /* optional list-of-values */
                continue;
            }
        }
        zassert_equal(
            len, BACNET_STATUS_REJECT, "len=%d test_len=%d", len, test_len);
    }
    /* test service processing */
    apdu_len = create_object_service_request_encode(apdu, sizeof(apdu), data);
    test_len = create_object_decode_service_request(apdu, apdu_len, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    /* processing - error case */
    status = create_object_process(
        NULL, object_supported, object_exists, NULL, NULL, NULL);
    zassert_equal(status, false, NULL);
    /* processing - error case */
    status = create_object_process(
        data, object_supported, object_exists, NULL, NULL, NULL);
    zassert_equal(status, false, NULL);
    zassert_equal(data->error_class, ERROR_CLASS_OBJECT, NULL);
    zassert_equal(data->error_code, ERROR_CODE_UNSUPPORTED_OBJECT_TYPE, NULL);
    /* processing - error case */
    object_supported = true;
    object_exists = true;
    status = create_object_process(
        data, object_supported, object_exists, test_create_object_function,
        test_delete_object_function, test_write_property_function);
    zassert_equal(status, false, NULL);
    zassert_equal(data->error_class, ERROR_CLASS_OBJECT, NULL);
    zassert_equal(
        data->error_code, ERROR_CODE_OBJECT_IDENTIFIER_ALREADY_EXISTS, NULL);
    /* processing - error case */
    object_supported = true;
    object_exists = false;
    status = create_object_process(
        data, object_supported, object_exists, NULL,
        test_delete_object_function, test_write_property_function);
    zassert_equal(status, false, NULL);
    zassert_equal(data->error_class, ERROR_CLASS_OBJECT, NULL);
    zassert_equal(
        data->error_code, ERROR_CODE_DYNAMIC_CREATION_NOT_SUPPORTED, NULL);
    /* processing - error case */
    object_supported = true;
    object_exists = false;
    Test_Create_Object_Returned_Instance = BACNET_MAX_INSTANCE;
    status = create_object_process(
        data, object_supported, object_exists, test_create_object_function,
        test_delete_object_function, test_write_property_function);
    zassert_equal(status, false, NULL);
    zassert_equal(data->error_class, ERROR_CLASS_RESOURCES, NULL);
    zassert_equal(data->error_code, ERROR_CODE_NO_SPACE_FOR_OBJECT, NULL);
    /* processing - error case */
    object_supported = true;
    object_exists = false;
    Test_Create_Object_Returned_Instance = 12345;
    Test_Write_Property_Return_Status = false;
    status = create_object_process(
        data, object_supported, object_exists, test_create_object_function,
        test_delete_object_function, test_write_property_function);
    if (data->application_data_len) {
        zassert_equal(status, false, NULL);
        zassert_equal(
            data->error_class, ERROR_CLASS_PROPERTY, "error_class=%s",
            bactext_error_class_name(data->error_class));
        zassert_equal(
            data->error_code, ERROR_CODE_WRITE_ACCESS_DENIED, "error_code=%s",
            bactext_error_code_name(data->error_code));
    } else {
        zassert_equal(status, true, NULL);
    }
    /* processing - successful case */
    object_supported = true;
    object_exists = false;
    if (data->object_instance != BACNET_MAX_INSTANCE) {
        Test_Create_Object_Returned_Instance = data->object_instance;
    } else {
        Test_Create_Object_Returned_Instance = 12345;
    }
    Test_Write_Property_Return_Status = true;
    status = create_object_process(
        data, object_supported, object_exists, test_create_object_function,
        test_delete_object_function, test_write_property_function);
    zassert_equal(status, true, NULL);
    zassert_equal(
        data->object_instance, Test_Create_Object_Returned_Instance, NULL);
    zassert_equal(
        data->first_failed_element_number, 0, "first_failed_element_number=%u",
        data->first_failed_element_number);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(create_object_tests, test_CreateObject)
#else
static void test_CreateObject(void)
#endif
{
    BACNET_CREATE_OBJECT_DATA data = {
        .object_instance = 1,
        .object_type = OBJECT_ANALOG_OUTPUT,
        .application_data_len = 0,
        .application_data = { 0 },
        .error_class = ERROR_CLASS_PROPERTY,
        .error_code = ERROR_CODE_SUCCESS,
    };
    BACNET_PROPERTY_VALUE value[2] = {
        { .next = NULL,
          .priority = BACNET_NO_PRIORITY,
          .propertyArrayIndex = BACNET_ARRAY_ALL,
          .propertyIdentifier = PROP_OBJECT_NAME,
          .value = { .tag = BACNET_APPLICATION_TAG_CHARACTER_STRING,
                     .type.Character_String = { .encoding = CHARACTER_UTF8,
                                                .length = 4,
                                                .value = "Test" } } },
        { .next = NULL,
          .priority = 1,
          .propertyArrayIndex = BACNET_ARRAY_ALL,
          .propertyIdentifier = PROP_PRESENT_VALUE,
          .value = { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 42.0 } },
    };
    int len = 0;

    /* encode two initial values */
    len = create_object_encode_initial_value(
        data.application_data, len, &value[0]);
    data.application_data_len += len;
    len = create_object_encode_initial_value(
        data.application_data, len, &value[1]);
    data.application_data_len += len;
    /* test encoding and decoding of CreateObject service */
    test_CreateObjectCodec(&data);
    data.object_instance = BACNET_MAX_INSTANCE;
    test_CreateObjectCodec(&data);
    /* validate the last write */
    zassert_equal(
        Test_Write_Property_Data.object_instance, data.object_instance, NULL);
    zassert_equal(Test_Write_Property_Data.object_type, data.object_type, NULL);
    zassert_equal(
        Test_Write_Property_Data.array_index, value[1].propertyArrayIndex,
        NULL);
    zassert_equal(
        Test_Write_Property_Data.object_property, value[1].propertyIdentifier,
        NULL);
    /* test with no initial values */
    data.object_instance = 1;
    data.application_data_len = 0;
    test_CreateObjectCodec(&data);
    data.object_instance = BACNET_MAX_INSTANCE;
    test_CreateObjectCodec(&data);
}

static void test_CreateObjectAckCodec(BACNET_CREATE_OBJECT_DATA *data)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_CREATE_OBJECT_DATA test_data = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;
    uint8_t invoke_id = 0;

    null_len = create_object_ack_service_encode(NULL, data);
    apdu_len = create_object_ack_service_encode(apdu, data);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len != BACNET_STATUS_ERROR, NULL);
    null_len = create_object_ack_service_decode(apdu, apdu_len, NULL);
    test_len = create_object_ack_service_decode(apdu, apdu_len, &test_data);
    zassert_equal(test_len, null_len, NULL);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    while (test_len) {
        test_len--;
        len = create_object_ack_service_decode(apdu, test_len, &test_data);
        zassert_equal(
            len, BACNET_STATUS_ERROR, "len=%d test_len=%d", len, test_len);
    }
    null_len = create_object_ack_encode(NULL, invoke_id, data);
    apdu_len = create_object_ack_encode(apdu, invoke_id, data);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len > 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(create_object_tests, test_CreateObject)
#else
static void test_CreateObjectACK(void)
#endif
{
    BACNET_CREATE_OBJECT_DATA data = { 0 };

    test_CreateObjectAckCodec(&data);
    data.object_instance = BACNET_MAX_INSTANCE;
    test_CreateObjectAckCodec(&data);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(create_object_tests, test_CreateObjectError)
#else
static void test_CreateObjectError(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_CREATE_OBJECT_DATA data = { 0 }, test_data = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;
    uint8_t invoke_id = 0;

    data.error_class = ERROR_CLASS_SERVICES;
    data.error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
    data.first_failed_element_number = 0;
    null_len = create_object_error_ack_service_encode(NULL, &data);
    apdu_len = create_object_error_ack_service_encode(apdu, &data);
    zassert_equal(apdu_len, null_len, NULL);
    null_len =
        create_object_error_ack_service_decode(NULL, apdu_len, &test_data);
    zassert_equal(null_len, BACNET_STATUS_REJECT, NULL);
    null_len = create_object_error_ack_service_decode(apdu, apdu_len, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    test_len =
        create_object_error_ack_service_decode(apdu, apdu_len, &test_data);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_equal(test_data.error_class, data.error_class, NULL);
    zassert_equal(test_data.error_code, data.error_code, NULL);
    zassert_equal(
        test_data.first_failed_element_number, data.first_failed_element_number,
        NULL);
    while (test_len) {
        test_len--;
        len =
            create_object_error_ack_service_decode(apdu, test_len, &test_data);
        zassert_equal(
            len, BACNET_STATUS_REJECT, "len=%d test_len=%d", len, test_len);
    }
    null_len = create_object_error_ack_encode(NULL, invoke_id, &data);
    apdu_len = create_object_error_ack_encode(apdu, invoke_id, &data);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len > 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(create_object_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        create_object_tests, ztest_unit_test(test_CreateObject),
        ztest_unit_test(test_CreateObjectACK),
        ztest_unit_test(test_CreateObjectError));

    ztest_run_test_suite(create_object_tests);
}
#endif
