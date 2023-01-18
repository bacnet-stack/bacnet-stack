/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacerror.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief decode the whole APDU - mainly used for unit testing
 */
static int bacerror_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_CONFIRMED_SERVICE *service,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    int len = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu_len) {
        if (apdu[0] != PDU_TYPE_ERROR)
            return -1;
        if (apdu_len > 1) {
            len = bacerror_decode_service_request(&apdu[1], apdu_len - 1,
                invoke_id, service, error_class, error_code);
        }
    }

    return len;
}

/**
 * @brief Test BACnet error type encode/decode
 */
static void testBACError(void)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 0;
    BACNET_CONFIRMED_SERVICE service = 0;
    BACNET_ERROR_CLASS error_class = 0;
    BACNET_ERROR_CODE error_code = 0;
    uint8_t test_invoke_id = 0;
    BACNET_CONFIRMED_SERVICE test_service = 0;
    BACNET_ERROR_CLASS test_error_class = 0;
    BACNET_ERROR_CODE test_error_code = 0;

    len = bacerror_encode_apdu(
        NULL, invoke_id, service, error_class, error_code);
    zassert_equal(len, 0, NULL);
    len = bacerror_encode_apdu(
        &apdu[0], invoke_id, service, error_class, error_code);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = bacerror_decode_apdu(&apdu[0], apdu_len, &test_invoke_id,
        &test_service, &test_error_class, &test_error_code);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_service, service, NULL);
    zassert_equal(test_error_class, error_class, NULL);
    zassert_equal(test_error_code, error_code, NULL);

    /* change type to get negative response */
    apdu[0] = PDU_TYPE_ABORT;
    len = bacerror_decode_apdu(&apdu[0], apdu_len, &test_invoke_id,
        &test_service, &test_error_class, &test_error_code);
    zassert_equal(len, -1, NULL);

    /* test NULL APDU */
    len = bacerror_decode_apdu(NULL, apdu_len, &test_invoke_id, &test_service,
        &test_error_class, &test_error_code);
    zassert_equal(len, -1, NULL);

    /* force a zero length */
    len = bacerror_decode_apdu(&apdu[0], 0, &test_invoke_id, &test_service,
        &test_error_class, &test_error_code);
    zassert_equal(len, 0, NULL);

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
                len = bacerror_decode_apdu(&apdu[0], apdu_len, &test_invoke_id,
                    &test_service, &test_error_class, &test_error_code);
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
    len = bacerror_decode_apdu(&apdu[0], apdu_len, &test_invoke_id,
        &test_service, &test_error_class, &test_error_code);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_service, service, NULL);
    zassert_equal(test_error_class, error_class, NULL);
    zassert_equal(test_error_code, error_code, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(bacerror_tests,
     ztest_unit_test(testBACError)
     );

    ztest_run_test_suite(bacerror_tests);
}
