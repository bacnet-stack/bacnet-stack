/**
 * @file
 * @brief Unit test for list-element services encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdest.h>
#include <bacnet/list_element.h>

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(list_element_tests, test_ListElement)
#else
static void test_ListElement(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_LIST_ELEMENT_DATA list_element = { 0 }, test_list_element = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;
    uint8_t application_data[MAX_APDU] = { 0 }, *test_application_data = NULL;
    int application_data_len = 0, test_application_data_len = 0;
    unsigned i = 0;
    BACNET_DESTINATION destination[5] = { 0 }, test_destination[5] = { 0 };
    const int destination_size = sizeof(destination) / sizeof(destination[0]);

    list_element.application_data = NULL;
    list_element.application_data_len = 0;
    /* NULL test - number of bytes which would have been written to the APDU */
    null_len = list_element_encode_service_request(NULL, &list_element);
    apdu_len = list_element_encode_service_request(apdu, &list_element);
    zassert_equal(apdu_len, null_len, NULL);
    apdu_len =
        list_element_service_request_encode(apdu, null_len, &list_element);
    zassert_equal(apdu_len, null_len, NULL);
    /* negative test - too short */
    null_len = list_element_service_request_encode(NULL, 0, &list_element);
    zassert_equal(0, null_len, NULL);
    /* decoder test */
    test_len =
        list_element_decode_service_request(apdu, apdu_len, &test_list_element);
    zassert_equal(apdu_len, test_len, NULL);
    /* fill application_data with RecipientList default data */
    list_element.array_index = BACNET_ARRAY_ALL;
    for (i = 0; i < destination_size; i++) {
        bacnet_destination_default_init(&destination[i]);
        application_data_len += bacnet_destination_encode(
            &application_data[application_data_len], &destination[i]);
    }
    list_element.application_data = application_data;
    list_element.application_data_len = application_data_len;
    apdu_len = list_element_encode_service_request(apdu, &list_element);
    test_len =
        list_element_decode_service_request(apdu, apdu_len, &test_list_element);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_equal(test_list_element.array_index, BACNET_ARRAY_ALL, NULL);
    test_application_data = list_element.application_data;
    test_application_data_len = list_element.application_data_len;
    for (i = 0; i < destination_size; i++) {
        test_len = bacnet_destination_decode(
            test_application_data, test_application_data_len,
            &test_destination[i]);
        zassert_true(
            bacnet_destination_same(&destination[i], &test_destination[i]),
            NULL);
        zassert_not_equal(test_len, BACNET_STATUS_REJECT, NULL);
        test_application_data_len -= test_len;
        test_application_data += test_len;
    }
    zassert_equal(test_application_data_len, 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(list_element_tests, test_ListElementError)
#else
static void test_ListElementError(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_LIST_ELEMENT_DATA list_element = { 0 }, test_list_element = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;

    list_element.error_class = ERROR_CLASS_SERVICES;
    list_element.error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
    list_element.first_failed_element_number = 0;
    null_len = list_element_error_ack_encode(NULL, &list_element);
    apdu_len = list_element_error_ack_encode(apdu, &list_element);
    zassert_equal(apdu_len, null_len, NULL);
    test_len =
        list_element_error_ack_decode(apdu, apdu_len, &test_list_element);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_equal(
        test_list_element.error_class, list_element.error_class, NULL);
    zassert_equal(test_list_element.error_code, list_element.error_code, NULL);
    zassert_equal(
        test_list_element.first_failed_element_number,
        list_element.first_failed_element_number, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(list_element_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        list_element_tests, ztest_unit_test(test_ListElement),
        ztest_unit_test(test_ListElementError));

    ztest_run_test_suite(list_element_tests);
}
#endif
