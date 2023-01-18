/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacstr.h>
#include <bacnet/rd.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static int rd_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_REINITIALIZED_STATE *state,
    BACNET_CHARACTER_STRING *password)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -1;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_REINITIALIZE_DEVICE)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len = rd_decode_service_request(
            &apdu[offset], apdu_len - offset, state, password);
    }

    return len;
}

static void test_ReinitializeDevice(void)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    BACNET_REINITIALIZED_STATE state;
    BACNET_REINITIALIZED_STATE test_state;
    BACNET_CHARACTER_STRING password;
    BACNET_CHARACTER_STRING test_password;

    state = BACNET_REINIT_WARMSTART;
    characterstring_init_ansi(&password, "John 3:16");
    len = rd_encode_apdu(&apdu[0], invoke_id, state, &password);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = rd_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_state, &test_password);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_state, state, NULL);
    zassert_true(characterstring_same(&test_password, &password), NULL);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(rd_tests,
     ztest_unit_test(test_ReinitializeDevice)
     );

    ztest_run_test_suite(rd_tests);
}
