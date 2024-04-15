/**
 * @file
 * @brief Unit test for DeleteObject service encode and decode API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdest.h>
#include <bacnet/delete_object.h>

static void test_DeleteObjectCodec(BACNET_DELETE_OBJECT_DATA *data)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_DELETE_OBJECT_DATA test_data = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;

    null_len = delete_object_encode_service_request(NULL, data);
    apdu_len = delete_object_encode_service_request(apdu, data);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len != BACNET_STATUS_ERROR, NULL);
    null_len = delete_object_decode_service_request(apdu, apdu_len, NULL);
    test_len = delete_object_decode_service_request(apdu, apdu_len, &test_data);
    zassert_equal(test_len, null_len, NULL);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    while (test_len) {
        test_len--;
        len = delete_object_decode_service_request(apdu, test_len, &test_data);
        zassert_equal(
            len, BACNET_STATUS_REJECT, "len=%d test_len=%d", len, test_len);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(delete_object_tests, test_DeleteObject)
#else
static void test_DeleteObject(void)
#endif
{
    BACNET_DELETE_OBJECT_DATA data = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;

    test_DeleteObjectCodec(&data);
    data.object_instance = BACNET_MAX_INSTANCE;
    test_DeleteObjectCodec(&data);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(delete_object_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(delete_object_tests, ztest_unit_test(test_DeleteObject));

    ztest_run_test_suite(delete_object_tests);
}
#endif
