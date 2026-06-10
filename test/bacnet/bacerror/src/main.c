/**
 * @file
 * @brief BACnet Error message encoding and decoding API testing
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/bacerror.h>
#include <bacnet/abort.h>
#include <bacnet/reject.h>

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
 * @brief Test bacnet_error_encode_apdu dispatch - abort, reject, and error
 * paths
 *
 * The new function bacnet_error_encode_apdu() dispatches to
 * abort_encode_apdu(), reject_encode_apdu(), or bacerror_encode_apdu() based on
 * the error code. Verify all three paths produce the correct PDU type byte.
 * Note: abort_encode_apdu() and reject_encode_apdu() return 0 for NULL apdu
 * (no length-only mode); only bacerror_encode_apdu() supports NULL apdu.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacerror_tests, testBACNetErrorEncodeAPDU)
#else
static void testBACNetErrorEncodeAPDU(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int null_len = 0;
    uint8_t invoke_id = 5;
    BACNET_CONFIRMED_SERVICE service = SERVICE_CONFIRMED_ATOMIC_READ_FILE;

    /* --- abort path --- */
    /* ERROR_CODE_ABORT_APDU_TOO_LONG is an abort-valid error code.
     * abort_encode_apdu() returns 0 for NULL apdu (no length-only mode). */
    null_len = bacnet_error_encode_apdu(
        NULL, invoke_id, service, ERROR_CODE_ABORT_APDU_TOO_LONG);
    zassert_equal(null_len, 0, "abort null_len=%d", null_len);
    len = bacnet_error_encode_apdu(
        &apdu[0], invoke_id, service, ERROR_CODE_ABORT_APDU_TOO_LONG);
    zassert_true(len > 0, "abort len=%d", len);
    zassert_equal(apdu[0] & 0xFEU, PDU_TYPE_ABORT, "apdu[0]=0x%02x", apdu[0]);
    zassert_equal(apdu[1], invoke_id, NULL);

    /* --- reject path --- */
    /* ERROR_CODE_REJECT_BUFFER_OVERFLOW is a reject-valid error code.
     * reject_encode_apdu() returns 0 for NULL apdu (no length-only mode). */
    memset(apdu, 0, sizeof(apdu));
    null_len = bacnet_error_encode_apdu(
        NULL, invoke_id, service, ERROR_CODE_REJECT_BUFFER_OVERFLOW);
    zassert_equal(null_len, 0, "reject null_len=%d", null_len);
    len = bacnet_error_encode_apdu(
        &apdu[0], invoke_id, service, ERROR_CODE_REJECT_BUFFER_OVERFLOW);
    zassert_true(len > 0, "reject len=%d", len);
    zassert_equal(apdu[0], PDU_TYPE_REJECT, "apdu[0]=0x%02x", apdu[0]);
    zassert_equal(apdu[1], invoke_id, NULL);

    /* --- regular error path --- */
    /* ERROR_CODE_UNKNOWN_OBJECT maps to a normal BACnet error.
     * bacerror_encode_apdu() supports NULL apdu (length-only mode). */
    memset(apdu, 0, sizeof(apdu));
    null_len = bacnet_error_encode_apdu(
        NULL, invoke_id, service, ERROR_CODE_UNKNOWN_OBJECT);
    zassert_true(null_len > 0, "error null_len=%d", null_len);
    len = bacnet_error_encode_apdu(
        &apdu[0], invoke_id, service, ERROR_CODE_UNKNOWN_OBJECT);
    zassert_equal(len, null_len, NULL);
    zassert_true(len > 0, "error len=%d", len);
    zassert_equal(apdu[0], PDU_TYPE_ERROR, "apdu[0]=0x%02x", apdu[0]);
    zassert_equal(apdu[1], invoke_id, NULL);
    zassert_equal(apdu[2], (uint8_t)service, NULL);

    /* --- reject path from h_arf.c bounds checks --- */
    /* ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE triggers reject */
    memset(apdu, 0, sizeof(apdu));
    len = bacnet_error_encode_apdu(
        &apdu[0], invoke_id, service, ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE);
    zassert_true(len > 0, "len=%d", len);
    zassert_equal(apdu[0], PDU_TYPE_REJECT, "apdu[0]=0x%02x", apdu[0]);

    /* --- reject path for missing required parameter --- */
    memset(apdu, 0, sizeof(apdu));
    len = bacnet_error_encode_apdu(
        &apdu[0], invoke_id, service,
        ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER);
    zassert_true(len > 0, "len=%d", len);
    zassert_equal(apdu[0], PDU_TYPE_REJECT, "apdu[0]=0x%02x", apdu[0]);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacerror_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacerror_tests, ztest_unit_test(testBACError),
        ztest_unit_test(testBACNetErrorEncodeAPDU));

    ztest_run_test_suite(bacerror_tests);
}
#endif
