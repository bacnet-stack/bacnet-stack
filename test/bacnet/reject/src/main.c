/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/reject.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
/* decode the whole APDU - mainly used for unit testing */
static int reject_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint8_t *reject_reason)
{
    int len = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu_len) {
        if (apdu[0] != PDU_TYPE_REJECT)
            return -1;
        if (apdu_len > 1) {
            len = reject_decode_service_request(
                &apdu[1], apdu_len - 1, invoke_id, reject_reason);
        }
    }

    return len;
}

static void testRejectEncodeDecode(void)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 0;
    uint8_t reject_reason = 0;
    uint8_t test_invoke_id = 0;
    uint8_t test_reject_reason = 0;

    len = reject_encode_apdu(&apdu[0], invoke_id, reject_reason);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = reject_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_reject_reason);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_reject_reason, reject_reason, NULL);

    /* change type to get negative response */
    apdu[0] = PDU_TYPE_ABORT;
    len = reject_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_reject_reason);
    zassert_equal(len, -1, NULL);

    /* test NULL APDU */
    len = reject_decode_apdu(
        NULL, apdu_len, &test_invoke_id, &test_reject_reason);
    zassert_equal(len, -1, NULL);

    /* force a zero length */
    len = reject_decode_apdu(&apdu[0], 0, &test_invoke_id, &test_reject_reason);
    zassert_equal(len, 0, NULL);

    /* check them all...   */
    for (invoke_id = 0; invoke_id < 255; invoke_id++) {
        for (reject_reason = 0; reject_reason < 255; reject_reason++) {
            len = reject_encode_apdu(&apdu[0], invoke_id, reject_reason);
            apdu_len = len;
            zassert_not_equal(len, 0, NULL);
            len = reject_decode_apdu(
                &apdu[0], apdu_len, &test_invoke_id, &test_reject_reason);
            zassert_not_equal(len, -1, NULL);
            zassert_equal(test_invoke_id, invoke_id, NULL);
            zassert_equal(test_reject_reason, reject_reason, NULL);
        }
    }
}

static void testRejectErrorCode(void)
{
    int i;
    BACNET_ERROR_CODE error_code;
    BACNET_REJECT_REASON reject_reason;
    BACNET_REJECT_REASON test_reject_reason;

    for (i = 0; i < MAX_BACNET_REJECT_REASON; i++) {
        reject_reason = (BACNET_REJECT_REASON)i;
        error_code = reject_convert_to_error_code(reject_reason);
        test_reject_reason = reject_convert_error_code(error_code);
        if (test_reject_reason != reject_reason) {
            printf("Reject: result=%u reject-code=%u\n",
                test_reject_reason,
                reject_reason);
        }
        zassert_equal(test_reject_reason, reject_reason, NULL);
    }
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(reject_tests,
     ztest_unit_test(testRejectEncodeDecode),
     ztest_unit_test(testRejectErrorCode)
     );

    ztest_run_test_suite(reject_tests);
}
