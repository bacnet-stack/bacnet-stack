/**
 * @file
 * @brief Unit test for CreateObject service encode and decode API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdest.h>
#include <bacnet/create_object.h>

static void test_CreateObjectCodec(BACNET_CREATE_OBJECT_DATA *data)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_CREATE_OBJECT_DATA test_data = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;

    null_len = create_object_encode_service_request(NULL, data);
    apdu_len = create_object_encode_service_request(apdu, data);
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
        zassert_equal(
            len, BACNET_STATUS_REJECT, "len=%d test_len=%d", len, test_len);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(create_object_tests, test_CreateObject)
#else
static void test_CreateObject(void)
#endif
{
    BACNET_CREATE_OBJECT_DATA data = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;

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
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;

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
