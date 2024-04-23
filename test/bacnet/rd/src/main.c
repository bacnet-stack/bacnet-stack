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
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_REINITIALIZED_STATE *state,
    BACNET_CHARACTER_STRING *password)
{
    int len = 0;
    int apdu_len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size <= 4) {
        return BACNET_STATUS_ERROR;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST) {
        return BACNET_STATUS_ERROR;
    }
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    if (invoke_id) {
        *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    }
    if (apdu[3] != SERVICE_CONFIRMED_REINITIALIZE_DEVICE) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len = 4;
    if (apdu_len < apdu_size) {
        len = rd_decode_service_request(
            &apdu[apdu_len], apdu_size - apdu_len, state, password);
        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = len;
        }
    }

    return apdu_len;
}

static void Test_ReinitializeDevice_Service(
    BACNET_REINITIALIZED_STATE state, char *password_string)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    int null_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    BACNET_REINITIALIZED_STATE test_state;
    BACNET_CHARACTER_STRING password;
    BACNET_CHARACTER_STRING test_password;

    characterstring_init_ansi(&password, password_string);
    null_len = rd_encode_apdu(NULL, invoke_id, state, &password);
    len = rd_encode_apdu(&apdu[0], invoke_id, state, &password);
    zassert_equal(null_len, len, "len=%d null_len=%d", len, null_len);
    zassert_true(len > 0, NULL);
    apdu_len = len;

    null_len = rd_decode_apdu(&apdu[0], apdu_len, NULL, NULL, NULL);
    len = rd_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_state, &test_password);
    zassert_equal(null_len, len, "len=%d null_len=%d", len, null_len);
    zassert_true(len > 0, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_state, state, NULL);
    zassert_true(characterstring_same(&test_password, &password), NULL);

    while (apdu_len) {
        apdu_len--;
        if (apdu_len == 6) {
            /* boundary of optional password, so becomes valid */
            continue;
        }
        len = rd_decode_apdu(apdu, apdu_len, NULL, NULL, NULL);
        zassert_true(len < 0, "len=%d apdu_len=%d password='%s'", len, apdu_len,
            password_string);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(rd_tests, test_ReinitializeDevice)
#else
static void test_ReinitializeDevice(void)
#endif
{
    Test_ReinitializeDevice_Service(BACNET_REINIT_COLDSTART, "John 3:16");
    Test_ReinitializeDevice_Service(BACNET_REINIT_COLDSTART, "");
    Test_ReinitializeDevice_Service(BACNET_REINIT_WARMSTART, "Joshua95");
    Test_ReinitializeDevice_Service(BACNET_REINIT_WARMSTART, "");
    Test_ReinitializeDevice_Service(BACNET_REINIT_STARTBACKUP, "Mary98");
    Test_ReinitializeDevice_Service(BACNET_REINIT_STARTBACKUP, "");
    Test_ReinitializeDevice_Service(BACNET_REINIT_ENDBACKUP, "Anna99");
    Test_ReinitializeDevice_Service(BACNET_REINIT_ENDBACKUP, "");
    Test_ReinitializeDevice_Service(BACNET_REINIT_STARTRESTORE, "Chris04");
    Test_ReinitializeDevice_Service(BACNET_REINIT_STARTRESTORE, "");
    Test_ReinitializeDevice_Service(BACNET_REINIT_ENDRESTORE, "Steve66");
    Test_ReinitializeDevice_Service(BACNET_REINIT_ENDRESTORE, "");
    Test_ReinitializeDevice_Service(BACNET_REINIT_ABORTRESTORE, "Patricia66");
    Test_ReinitializeDevice_Service(BACNET_REINIT_ABORTRESTORE, "");

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(rd_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(rd_tests, ztest_unit_test(test_ReinitializeDevice));

    ztest_run_test_suite(rd_tests);
}
#endif
