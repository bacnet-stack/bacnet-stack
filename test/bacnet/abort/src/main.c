/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/abort.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief decode the whole APDU - mainly used for unit testing
 */
static int abort_decode_apdu(
    uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint8_t *abort_reason,
    bool *server)
{
    int len = 0;

    if (!apdu) {
        return -1;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu_len > 0) {
        if ((apdu[0] & 0xF0) != PDU_TYPE_ABORT) {
            return -1;
        }
        if (apdu[0] & 1) {
            *server = true;
        } else {
            *server = false;
        }
        if (apdu_len > 1) {
            len = abort_decode_service_request(
                &apdu[1], apdu_len - 1, invoke_id, abort_reason);
        }
    }

    return len;
}

/**
 * @brief Test Abort ADPU
 */
static void testAbortAPDU(uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t test_invoke_id = 0;
    uint8_t test_abort_reason = 0;
    bool test_server = false;

    len = abort_encode_apdu(&apdu[0], invoke_id, abort_reason, server);
    apdu_len = len;
    zassert_not_equal(len, 0, NULL);
    len = abort_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_abort_reason, &test_server);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_abort_reason, abort_reason, NULL);
    zassert_equal(test_server, server, NULL);

    return;
}

/**
 * @brief Test encode/decode API for strings
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(abort_tests, testAbortEncodeDecode)
#else
static void testAbortEncodeDecode(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 0;
    uint8_t test_invoke_id = 0;
    uint8_t abort_reason = 0;
    uint8_t test_abort_reason = 0;
    bool server = false;
    bool test_server = false;

    len = abort_encode_apdu(&apdu[0], invoke_id, abort_reason, server);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;
    len = abort_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_abort_reason, &test_server);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_abort_reason, abort_reason, NULL);
    zassert_equal(test_server, server, NULL);

    /* change type to get negative response */
    apdu[0] = PDU_TYPE_REJECT;
    len = abort_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_abort_reason, &test_server);
    zassert_equal(len, -1, NULL);

    /* test NULL APDU */
    len = abort_decode_apdu(
        NULL, apdu_len, &test_invoke_id, &test_abort_reason, &test_server);
    zassert_equal(len, -1, NULL);

    /* force a zero length */
    len = abort_decode_apdu(
        &apdu[0], 0, &test_invoke_id, &test_abort_reason, &test_server);
    zassert_equal(len, 0, NULL);

    /* check them all...   */
    for (invoke_id = 0; invoke_id < 255; invoke_id++) {
        for (abort_reason = 0; abort_reason < 255; abort_reason++) {
            testAbortAPDU(invoke_id, abort_reason, false);
            testAbortAPDU(invoke_id, abort_reason, true);
        }
    }
}

/**
 * @brief Test Abort Error
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(abort_tests, testAbortError)
#else
static void testAbortError(void)
#endif
{
    int i;
    BACNET_ERROR_CODE error_code;
    BACNET_ABORT_REASON abort_code;
    BACNET_ABORT_REASON test_abort_code;

    for (i = 0; i < ABORT_REASON_RESERVED_MIN; i++) {
        abort_code = (BACNET_ABORT_REASON)i;
        error_code = abort_convert_to_error_code(abort_code);
        test_abort_code = abort_convert_error_code(error_code);
        if (test_abort_code != abort_code) {
            printf(
                "Abort: result=%u abort-code=%u\n", test_abort_code,
                abort_code);
        }
        zassert_equal(test_abort_code, abort_code, NULL);
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(abort_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        abort_tests, ztest_unit_test(testAbortEncodeDecode),
        ztest_unit_test(testAbortError));

    ztest_run_test_suite(abort_tests);
}
#endif
