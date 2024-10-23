/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/whois.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static int whois_decode_apdu(
    const uint8_t *apdu,
    unsigned apdu_len,
    int32_t *pLow_limit,
    int32_t *pHigh_limit)
{
    int len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* optional limits - must be used as a pair */
    if (apdu_len >= 2) {
        /* optional checking - most likely was already done prior to this call
         */
        if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) {
            return BACNET_STATUS_ERROR;
        }
        if (apdu[1] != SERVICE_UNCONFIRMED_WHO_IS) {
            return BACNET_STATUS_ERROR;
        }
        len = whois_decode_service_request(
            &apdu[2], apdu_len - 2, pLow_limit, pHigh_limit);
    }

    return len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(whois_tests, testWhoIs)
#else
static void testWhoIs(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    int32_t low_limit = -1;
    int32_t high_limit = -1;
    int32_t test_low_limit = 0;
    int32_t test_high_limit = 0;

    /* normal who-is without limits */
    len = whois_encode_apdu(&apdu[0], low_limit, high_limit);
    zassert_true(len > 0, NULL);
    apdu_len = len;

    len = whois_decode_apdu(
        &apdu[0], apdu_len, &test_low_limit, &test_high_limit);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(test_low_limit, low_limit, NULL);
    zassert_equal(test_high_limit, high_limit, NULL);

    /* normal who-is with limits - complete range */
    for (low_limit = 0; low_limit <= BACNET_MAX_INSTANCE;
         low_limit += (BACNET_MAX_INSTANCE / 4)) {
        for (high_limit = 0; high_limit <= BACNET_MAX_INSTANCE;
             high_limit += (BACNET_MAX_INSTANCE / 4)) {
            len = whois_encode_apdu(&apdu[0], low_limit, high_limit);
            apdu_len = len;
            zassert_true(len > 0, NULL);
            len = whois_decode_apdu(
                &apdu[0], apdu_len, &test_low_limit, &test_high_limit);
            zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
            zassert_equal(test_low_limit, low_limit, NULL);
            zassert_equal(test_high_limit, high_limit, NULL);
        }
    }
    /* abnormal case:
       who-is with no limits, but with APDU containing 2 limits */
    low_limit = 0;
    high_limit = 0;
    len = whois_encode_apdu(&apdu[0], low_limit, high_limit);
    zassert_true(len > 0, NULL);
    apdu_len = len;
    low_limit = -1;
    high_limit = -1;
    len = whois_encode_apdu(&apdu[0], low_limit, high_limit);
    zassert_true(len > 0, NULL);
    apdu_len = len;
    len = whois_decode_apdu(
        &apdu[0], apdu_len, &test_low_limit, &test_high_limit);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(test_low_limit, low_limit, NULL);
    zassert_equal(test_high_limit, high_limit, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(whois_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(whois_tests, ztest_unit_test(testWhoIs));

    ztest_run_test_suite(whois_tests);
}
#endif
