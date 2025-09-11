/**
 * @file
 * @brief BACnet Error message encoding and decoding API testing
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/bacerror.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static int bacerror_decode_apdu(
    const uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_CONFIRMED_SERVICE *service,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    int apdu_len = BACNET_STATUS_ERROR;

    if (apdu && (apdu_size > 0)) {
        if (apdu[0] != PDU_TYPE_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len = 1;
        apdu_len = bacerror_decode_service_request(
            &apdu[apdu_len], apdu_size - apdu_len, invoke_id, service,
            error_class, error_code);
    }

    return apdu_len;
}

/**
 * @brief Test BACnet error type encode/decode
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacerror_tests, testBACError)
#else
static void testBACError(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0, apdu_len = 0, null_len, test_len;
    uint8_t invoke_id = 0;
    BACNET_CONFIRMED_SERVICE service = 0;
    BACNET_ERROR_CLASS error_class = 0;
    BACNET_ERROR_CODE error_code = 0;
    uint8_t test_invoke_id = 0;
    BACNET_CONFIRMED_SERVICE test_service = 0;
    BACNET_ERROR_CLASS test_error_class = 0;
    BACNET_ERROR_CODE test_error_code = 0;

    null_len =
        bacerror_encode_apdu(NULL, invoke_id, service, error_class, error_code);
    len = bacerror_encode_apdu(
        &apdu[0], invoke_id, service, error_class, error_code);
    zassert_equal(len, null_len, NULL);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    null_len = bacerror_decode_apdu(&apdu[0], apdu_len, NULL, NULL, NULL, NULL);
    len = bacerror_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_service, &test_error_class,
        &test_error_code);
    zassert_not_equal(len, BACNET_STATUS_ERROR, "len=%d", len);
    zassert_equal(len, null_len, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_service, service, NULL);
    zassert_equal(test_error_class, error_class, NULL);
    zassert_equal(test_error_code, error_code, NULL);

    /* test too short lengths */
    while (len) {
        len--;
        test_len = bacerror_decode_apdu(
            &apdu[0], len, &test_invoke_id, &test_service, &test_error_class,
            &test_error_code);
        zassert_equal(
            test_len, BACNET_STATUS_ERROR, "len=%d test_len=%d", len, test_len);
    }

    /* change type to get negative response */
    apdu[0] = PDU_TYPE_ABORT;
    len = bacerror_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_service, &test_error_class,
        &test_error_code);
    zassert_true(len <= 0, NULL);

    /* test NULL APDU */
    len = bacerror_decode_apdu(
        NULL, apdu_len, &test_invoke_id, &test_service, &test_error_class,
        &test_error_code);
    zassert_true(len <= 0, NULL);

    /* check them all...   */
    for (service = 0; service < MAX_BACNET_CONFIRMED_SERVICE; service++) {
        for (error_class = 0; error_class < ERROR_CLASS_PROPRIETARY_FIRST;
             error_class++) {
            for (error_code = 0; error_code < ERROR_CODE_PROPRIETARY_FIRST;
                 error_code++) {
                len = bacerror_encode_apdu(
                    &apdu[0], invoke_id, service, error_class, error_code);
                apdu_len = len;
                zassert_not_equal(len, 0, NULL);
                len = bacerror_decode_apdu(
                    &apdu[0], apdu_len, &test_invoke_id, &test_service,
                    &test_error_class, &test_error_code);
                zassert_not_equal(len, -1, NULL);
                zassert_equal(test_invoke_id, invoke_id, NULL);
                zassert_equal(test_service, service, NULL);
                zassert_equal(test_error_class, error_class, NULL);
                zassert_equal(test_error_code, error_code, NULL);
            }
        }
    }

    /* max boundaries */
    service = 255;
    error_class = ERROR_CLASS_PROPRIETARY_LAST;
    error_code = ERROR_CODE_PROPRIETARY_LAST;
    len = bacerror_encode_apdu(
        &apdu[0], invoke_id, service, error_class, error_code);
    apdu_len = len;
    zassert_not_equal(len, 0, NULL);
    len = bacerror_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_service, &test_error_class,
        &test_error_code);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_service, service, NULL);
    zassert_equal(test_error_class, error_class, NULL);
    zassert_equal(test_error_code, error_code, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacerror_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bacerror_tests, ztest_unit_test(testBACError));

    ztest_run_test_suite(bacerror_tests);
}
#endif
