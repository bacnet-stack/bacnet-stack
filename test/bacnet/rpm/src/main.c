/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacerror.h>  /* For bacerror_decode_error_class_and_code() */
#include <bacnet/bacdcode.h>
#include <bacnet/rpm.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static int rpm_ack_decode_apdu(uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    uint8_t *invoke_id,
    uint8_t **service_request,
    unsigned *service_request_len)
{
    int offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK)
        return -1;
    *invoke_id = apdu[1];
    if (apdu[2] != SERVICE_CONFIRMED_READ_PROP_MULTIPLE)
        return -1;
    offset = 3;
    if (apdu_len > offset) {
        if (service_request)
            *service_request = &apdu[offset];
        if (service_request_len)
            *service_request_len = apdu_len - offset;
    }

    return offset;
}

static int rpm_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint8_t **service_request,
    unsigned *service_request_len)
{
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -1;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_READ_PROP_MULTIPLE)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        if (service_request)
            *service_request = &apdu[offset];
        if (service_request_len)
            *service_request_len = apdu_len - offset;
    }

    return offset;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(rpm_tests, testReadPropertyMultiple)
#else
static void testReadPropertyMultiple(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int test_len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 12;
    uint8_t test_invoke_id = 0;
    uint8_t *service_request = NULL;
    unsigned service_request_len = 0;
    BACNET_RPM_DATA rpmdata;

    rpmdata.object_type = OBJECT_DEVICE;
    rpmdata.object_instance = 0;
    rpmdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpmdata.array_index = 0;

    /* build the RPM - try to make it easy for the Application Layer development
     */
    /* IDEA: similar construction, but pass apdu, apdu_len pointer, size of apdu
       to let the called function handle the out of space problem that these get
       into by returning a boolean of success/failure. It almost needs to use
       the keylist library or something similar. Also check case of storing a
       backoff point (i.e. save enough room for object_end) */
    apdu_len = rpm_encode_apdu_init(&apdu[0], invoke_id);
    /* each object has a beginning and an end */
    apdu_len +=
        rpm_encode_apdu_object_begin(&apdu[apdu_len], OBJECT_DEVICE, 123);
    /* then stuff as many properties into it as APDU length will allow */
    apdu_len += rpm_encode_apdu_object_property(
        &apdu[apdu_len], PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL);
    apdu_len += rpm_encode_apdu_object_property(
        &apdu[apdu_len], PROP_OBJECT_NAME, BACNET_ARRAY_ALL);
    apdu_len += rpm_encode_apdu_object_end(&apdu[apdu_len]);
    /* each object has a beginning and an end */
    apdu_len +=
        rpm_encode_apdu_object_begin(&apdu[apdu_len], OBJECT_ANALOG_INPUT, 33);
    apdu_len += rpm_encode_apdu_object_property(
        &apdu[apdu_len], PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL);
    apdu_len += rpm_encode_apdu_object_property(
        &apdu[apdu_len], PROP_ALL, BACNET_ARRAY_ALL);
    apdu_len += rpm_encode_apdu_object_end(&apdu[apdu_len]);

    zassert_not_equal(apdu_len, 0, NULL);

    test_len = rpm_decode_apdu(&apdu[0], apdu_len, &test_invoke_id,
        &service_request, /* will point to the service request in the apdu */
        &service_request_len);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_not_null(service_request, NULL);
    zassert_true(service_request_len > 0, NULL);

    test_len =
        rpm_decode_object_id(service_request, service_request_len, &rpmdata);
    zassert_true(test_len > 0, NULL);
    zassert_equal(rpmdata.object_type, OBJECT_DEVICE, NULL);
    zassert_equal(rpmdata.object_instance, 123, NULL);
    len = test_len;
    /* decode the object property portion of the service request */
    test_len = rpm_decode_object_property(
        &service_request[len], service_request_len - len, &rpmdata);
    zassert_true(test_len > 0, NULL);
    zassert_equal(rpmdata.object_property, PROP_OBJECT_IDENTIFIER, NULL);
    zassert_equal(rpmdata.array_index, BACNET_ARRAY_ALL, NULL);
    len += test_len;
    test_len = rpm_decode_object_property(
        &service_request[len], service_request_len - len, &rpmdata);
    zassert_true(test_len > 0, NULL);
    zassert_equal(rpmdata.object_property, PROP_OBJECT_NAME, NULL);
    zassert_equal(rpmdata.array_index, BACNET_ARRAY_ALL, NULL);
    len += test_len;
    /* try again - we should fail */
    test_len = rpm_decode_object_property(
        &service_request[len], service_request_len - len, &rpmdata);
    zassert_true(test_len < 0, NULL);
    /* is it the end of this object? */
    test_len =
        rpm_decode_object_end(&service_request[len], service_request_len - len);
    zassert_equal(test_len, 1, NULL);
    len += test_len;
    /* try to decode an object id */
    test_len = rpm_decode_object_id(
        &service_request[len], service_request_len - len, &rpmdata);
    zassert_true(test_len > 0, NULL);
    zassert_equal(rpmdata.object_type, OBJECT_ANALOG_INPUT, NULL);
    zassert_equal(rpmdata.object_instance, 33, NULL);
    len += test_len;
    /* decode the object property portion of the service request only */
    test_len = rpm_decode_object_property(
        &service_request[len], service_request_len - len, &rpmdata);
    zassert_true(test_len > 0, NULL);
    zassert_equal(rpmdata.object_property, PROP_OBJECT_IDENTIFIER, NULL);
    zassert_equal(rpmdata.array_index, BACNET_ARRAY_ALL, NULL);
    len += test_len;
    test_len = rpm_decode_object_property(
        &service_request[len], service_request_len - len, &rpmdata);
    zassert_true(test_len > 0, NULL);
    zassert_equal(rpmdata.object_property, PROP_ALL, NULL);
    zassert_equal(rpmdata.array_index, BACNET_ARRAY_ALL, NULL);
    len += test_len;
    test_len = rpm_decode_object_property(
        &service_request[len], service_request_len - len, &rpmdata);
    zassert_true(test_len < 0, NULL);
    /* got an error -1, is it the end of this object? */
    test_len =
        rpm_decode_object_end(&service_request[len], service_request_len - len);
    zassert_equal(test_len, 1, NULL);
    len += test_len;
    zassert_equal(len, service_request_len, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(rpm_tests, testReadPropertyMultipleAck)
#else
static void testReadPropertyMultipleAck(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int test_len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 12;
    uint8_t test_invoke_id = 0;
    uint8_t *service_request = NULL;
    unsigned service_request_len = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_DEVICE;
    uint32_t object_instance = 0;
    BACNET_PROPERTY_ID object_property = PROP_OBJECT_IDENTIFIER;
    uint32_t array_index = 0;
    BACNET_APPLICATION_DATA_VALUE application_data[4] = { { 0 } };
    BACNET_APPLICATION_DATA_VALUE test_application_data = { 0 };
    uint8_t application_data_buffer[MAX_APDU] = { 0 };
    int application_data_buffer_len = 0;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
    BACNET_RPM_DATA rpmdata;

    /* build the RPM - try to make it easy for the
       Application Layer development */
    /* IDEA: similar construction, but pass apdu, apdu_len pointer,
       size of apdu to let the called function handle the out of
       space problem that these get into  by returning a boolean
       of success/failure.
       It almost needs to use the keylist library or something similar.
       Also check case of storing a backoff point
       (i.e. save enough room for object_end) */
    apdu_len = rpm_ack_encode_apdu_init(&apdu[0], invoke_id);
    /* object beginning */
    rpmdata.object_type = OBJECT_DEVICE;
    rpmdata.object_instance = 123;
    apdu_len += rpm_ack_encode_apdu_object_begin(&apdu[apdu_len], &rpmdata);
    /* reply property */
    apdu_len += rpm_ack_encode_apdu_object_property(
        &apdu[apdu_len], PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL);
    /* reply value */
    application_data[0].tag = BACNET_APPLICATION_TAG_OBJECT_ID;
    application_data[0].type.Object_Id.type = OBJECT_DEVICE;
    application_data[0].type.Object_Id.instance = 123;
    application_data_buffer_len = bacapp_encode_application_data(
        &application_data_buffer[0], &application_data[0]);
    apdu_len += rpm_ack_encode_apdu_object_property_value(&apdu[apdu_len],
        &application_data_buffer[0], application_data_buffer_len);
    /* reply property */
    apdu_len += rpm_ack_encode_apdu_object_property(
        &apdu[apdu_len], PROP_OBJECT_TYPE, BACNET_ARRAY_ALL);
    /* reply value */
    application_data[1].tag = BACNET_APPLICATION_TAG_ENUMERATED;
    application_data[1].type.Enumerated = OBJECT_DEVICE;
    application_data_buffer_len = bacapp_encode_application_data(
        &application_data_buffer[0], &application_data[1]);
    apdu_len += rpm_ack_encode_apdu_object_property_value(&apdu[apdu_len],
        &application_data_buffer[0], application_data_buffer_len);
    /* object end */
    apdu_len += rpm_ack_encode_apdu_object_end(&apdu[apdu_len]);

    /* object beginning */
    rpmdata.object_type = OBJECT_ANALOG_INPUT;
    rpmdata.object_instance = 33;
    apdu_len += rpm_ack_encode_apdu_object_begin(&apdu[apdu_len], &rpmdata);
    /* reply property */
    apdu_len += rpm_ack_encode_apdu_object_property(
        &apdu[apdu_len], PROP_PRESENT_VALUE, BACNET_ARRAY_ALL);
    /* reply value */
    application_data[2].tag = BACNET_APPLICATION_TAG_REAL;
    application_data[2].type.Real = 0.0;
    application_data_buffer_len = bacapp_encode_application_data(
        &application_data_buffer[0], &application_data[2]);
    apdu_len += rpm_ack_encode_apdu_object_property_value(&apdu[apdu_len],
        &application_data_buffer[0], application_data_buffer_len);
    /* reply property */
    apdu_len += rpm_ack_encode_apdu_object_property(
        &apdu[apdu_len], PROP_DEADBAND, BACNET_ARRAY_ALL);
    /* reply error */
    apdu_len += rpm_ack_encode_apdu_object_property_error(
        &apdu[apdu_len], ERROR_CLASS_PROPERTY, ERROR_CODE_UNKNOWN_PROPERTY);
    /* object end */
    apdu_len += rpm_ack_encode_apdu_object_end(&apdu[apdu_len]);
    zassert_not_equal(apdu_len, 0, NULL);

    /****** decode the packet ******/
    test_len = rpm_ack_decode_apdu(&apdu[0], apdu_len, &test_invoke_id,
        &service_request, /* will point to the service request in the apdu */
        &service_request_len);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_not_null(service_request, NULL);
    zassert_true(service_request_len > 0, NULL);
    /* the first part should be the first object id */
    test_len = rpm_ack_decode_object_id(
        service_request, service_request_len, &object_type, &object_instance);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(object_type, OBJECT_DEVICE, NULL);
    zassert_equal(object_instance, 123, NULL);
    len = test_len;
    /* extract the property */
    test_len = rpm_ack_decode_object_property(&service_request[len],
        service_request_len - len, &object_property, &array_index);
    zassert_equal(object_property, PROP_OBJECT_IDENTIFIER, NULL);
    zassert_equal(array_index, BACNET_ARRAY_ALL, NULL);
    len += test_len;
    /* what is the result? An error or a value? */
    zassert_true(decode_is_opening_tag_number(&service_request[len], 4), NULL);
    len++;
    /* decode the object property portion of the service request */
    /* note: if this was an array, there could have been
       more than one element to decode */
    test_len = bacapp_decode_application_data(&service_request[len],
        service_request_len - len, &test_application_data);
    zassert_true(test_len > 0, NULL);
    zassert_true(bacapp_same_value(&application_data[0], &test_application_data), NULL);
    len += test_len;
    zassert_true(decode_is_closing_tag_number(&service_request[len], 4), NULL);
    len++;
    /* see if there is another property */
    test_len = rpm_ack_decode_object_property(&service_request[len],
        service_request_len - len, &object_property, &array_index);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(object_property, PROP_OBJECT_TYPE, NULL);
    zassert_equal(array_index, BACNET_ARRAY_ALL, NULL);
    len += test_len;
    /* what is the result value? */
    zassert_true(decode_is_opening_tag_number(&service_request[len], 4), NULL);
    len++;
    /* decode the object property portion of the service request */
    test_len = bacapp_decode_application_data(&service_request[len],
        service_request_len - len, &test_application_data);
    zassert_true(test_len > 0, NULL);
    zassert_true(bacapp_same_value(&application_data[1], &test_application_data), NULL);
    len += test_len;
    zassert_true(decode_is_closing_tag_number(&service_request[len], 4), NULL);
    len++;
    /* see if there is another property */
    /* this time we should fail */
    test_len = rpm_ack_decode_object_property(&service_request[len],
        service_request_len - len, &object_property, &array_index);
    zassert_equal(test_len, -1, NULL);
    /* see if it is the end of this object */
    test_len = rpm_ack_decode_object_end(
        &service_request[len], service_request_len - len);
    zassert_equal(test_len, 1, NULL);
    len += test_len;
    /* try to decode another object id */
    test_len = rpm_ack_decode_object_id(&service_request[len],
        service_request_len - len, &object_type, &object_instance);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(object_type, OBJECT_ANALOG_INPUT, NULL);
    zassert_equal(object_instance, 33, NULL);
    len += test_len;
    /* decode the object property portion of the service request only */
    test_len = rpm_ack_decode_object_property(&service_request[len],
        service_request_len - len, &object_property, &array_index);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(object_property, PROP_PRESENT_VALUE, NULL);
    zassert_equal(array_index, BACNET_ARRAY_ALL, NULL);
    len += test_len;
    /* what is the result value? */
    zassert_true(decode_is_opening_tag_number(&service_request[len], 4), NULL);
    len++;
    /* decode the object property portion of the service request */
    test_len = bacapp_decode_application_data(&service_request[len],
        service_request_len - len, &test_application_data);
    zassert_true(test_len > 0, NULL);
    zassert_true(bacapp_same_value(&application_data[2], &test_application_data), NULL);
    len += test_len;
    zassert_true(decode_is_closing_tag_number(&service_request[len], 4), NULL);
    len++;
    /* see if there is another property */
    test_len = rpm_ack_decode_object_property(&service_request[len],
        service_request_len - len, &object_property, &array_index);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(object_property, PROP_DEADBAND, NULL);
    zassert_equal(array_index, BACNET_ARRAY_ALL, NULL);
    len += test_len;
    /* what is the result value? */
    zassert_true(decode_is_opening_tag_number(&service_request[len], 5), NULL);
    len++;
    /* it was an error reply */
    test_len = bacerror_decode_error_class_and_code(&service_request[len],
        service_request_len - len, &error_class, &error_code);
    zassert_not_equal(test_len, 0, NULL);
    zassert_equal(error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(error_code, ERROR_CODE_UNKNOWN_PROPERTY, NULL);
    len += test_len;
    zassert_true(decode_is_closing_tag_number(&service_request[len], 5), NULL);
    len++;
    /* is there another property? */
    test_len = rpm_ack_decode_object_property(&service_request[len],
        service_request_len - len, &object_property, &array_index);
    zassert_equal(test_len, -1, NULL);
    /* got an error -1, is it the end of this object? */
    test_len = rpm_ack_decode_object_end(
        &service_request[len], service_request_len - len);
    zassert_equal(test_len, 1, NULL);
    len += test_len;
    /* check for another object */
    test_len = rpm_ack_decode_object_id(&service_request[len],
        service_request_len - len, &object_type, &object_instance);
    zassert_equal(test_len, 0, NULL);
    zassert_equal(len, service_request_len, NULL);
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(rpm_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(rpm_tests,
     ztest_unit_test(testReadPropertyMultiple),
     ztest_unit_test(testReadPropertyMultipleAck)
     );

    ztest_run_test_suite(rpm_tests);
}
#endif
