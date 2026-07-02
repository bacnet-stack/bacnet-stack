/**
 * @file
 * @brief Unit tests for handler_read_property_multiple_ack decoder paths
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/rpm.h>
#include <bacnet/basic/service/h_rpm_a.h>

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rpm_a_tests, testMalformedTag4PayloadReturnsError)
#else
static void testMalformedTag4PayloadReturnsError(void)
#endif
{
    uint8_t service_request[480] = { 0 };
    uint8_t value_buffer[16] = { 0 };
    BACNET_RPM_DATA rpmdata = { 0 };
    BACNET_READ_ACCESS_DATA *read_access_data = NULL;
    int len = 0;
    int value_len = 0;
    int test_len = 0;

    rpmdata.object_type = OBJECT_DEVICE;
    rpmdata.object_instance = 123;
    len += rpm_ack_encode_apdu_object_begin(&service_request[len], &rpmdata);
    len += rpm_ack_encode_apdu_object_property(
        &service_request[len], PROP_OBJECT_LIST, BACNET_ARRAY_ALL);

    len += encode_opening_tag(&service_request[len], 4);

    value_len = encode_application_object_id(
        &value_buffer[0], OBJECT_DEVICE, rpmdata.object_instance);
    zassert_true(value_len > 0, NULL);
    memcpy(&service_request[len], &value_buffer[0], (size_t)value_len);
    len += value_len;

    /* Truncated second value in same tag-4 payload to force partial decode. */
    value_len = encode_application_real(&value_buffer[0], 1.0f);
    zassert_true(value_len >= 5, NULL);
    memcpy(&service_request[len], &value_buffer[0], 2);
    len += 2;

    len += encode_closing_tag(&service_request[len], 4);
    len += rpm_ack_encode_apdu_object_end(&service_request[len]);

    read_access_data = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
    zassert_not_null(read_access_data, NULL);

    test_len =
        rpm_ack_decode_service_request(service_request, len, read_access_data);
    zassert_equal(
        test_len, BACNET_STATUS_ERROR,
        "rpm_ack_decode_service_request returned %d, expected %d", test_len,
        BACNET_STATUS_ERROR);

    while (read_access_data) {
        read_access_data = rpm_data_free(read_access_data);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_rpm_a_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_rpm_a_tests, ztest_unit_test(testMalformedTag4PayloadReturnsError));
    ztest_run_test_suite(h_rpm_a_tests);
}
#endif
