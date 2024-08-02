/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/dcc.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static int dcc_decode_apdu(
    uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    uint16_t *timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE *enable_disable,
    BACNET_CHARACTER_STRING *password)
{
    int len = 0;
    unsigned apdu_len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size > 4) {
        if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST) {
            return BACNET_STATUS_ERROR;
        }
        /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
        *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
        if (apdu[3] != SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len = 4;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size > apdu_len) {
        len = dcc_decode_service_request(
            &apdu[apdu_len], apdu_size - apdu_len, timeDuration, enable_disable,
            password);
        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = len;
        }
    }

    return apdu_len;
}

static void test_DeviceCommunicationControlData(
    uint8_t invoke_id,
    uint16_t timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
    BACNET_CHARACTER_STRING *password)
{
    uint8_t apdu[480] = { 0 };
    int apdu_size = 0, null_len = 0, test_len = 0;
    uint8_t test_invoke_id = 0;
    uint16_t test_timeDuration = 0;
    BACNET_COMMUNICATION_ENABLE_DISABLE test_enable_disable;
    BACNET_CHARACTER_STRING test_password;

    null_len = dcc_encode_apdu(
        NULL, invoke_id, timeDuration, enable_disable, password);
    apdu_size = dcc_encode_apdu(
        &apdu[0], invoke_id, timeDuration, enable_disable, password);
    zassert_equal(apdu_size, null_len, NULL);
    zassert_not_equal(apdu_size, 0, NULL);

    test_len = dcc_decode_apdu(
        &apdu[0], apdu_size, &test_invoke_id, &test_timeDuration,
        &test_enable_disable, &test_password);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_timeDuration, timeDuration, NULL);
    zassert_equal(test_enable_disable, enable_disable, NULL);
    zassert_true(characterstring_same(&test_password, password), NULL);
    test_len = dcc_decode_apdu(
        apdu, 4, &test_invoke_id, &test_timeDuration, &test_enable_disable,
        &test_password);
    zassert_true(test_len < 0, "apdu_size=%d test_len=%d", apdu_size, test_len);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(dcc_tests, test_DeviceCommunicationControl)
#else
static void test_DeviceCommunicationControl(void)
#endif
{
    uint8_t invoke_id = 128;
    uint16_t timeDuration = 0;
    BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable;
    BACNET_CHARACTER_STRING password;

    timeDuration = 0;
    enable_disable = COMMUNICATION_DISABLE_INITIATION;
    characterstring_init_ansi(&password, "John 3:16");
    test_DeviceCommunicationControlData(
        invoke_id, timeDuration, enable_disable, &password);

    timeDuration = 12345;
    enable_disable = COMMUNICATION_DISABLE;
    test_DeviceCommunicationControlData(
        invoke_id, timeDuration, enable_disable, NULL);

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(dcc_tests, test_DeviceCommunicationControlMalformedData)
#else
static void test_DeviceCommunicationControlMalformedData(void)
#endif
{
    /* payload with enable-disable, and password with wrong characterstring
     * length */
    uint8_t payload_1[] = { 0x19, 0x00, 0x2a, 0x00, 0x41 };
    /* payload with enable-disable, and password with wrong characterstring
     * length */
    uint8_t payload_2[] = { 0x19, 0x00, 0x2d, 0x55, 0x00, 0x66,
                            0x69, 0x73, 0x74, 0x65, 0x72 };
    /* payload with enable-disable - wrong context tag number for password */
    uint8_t payload_3[] = { 0x19, 0x01, 0x3d, 0x09, 0x00, 0x66,
                            0x69, 0x73, 0x74, 0x65, 0x72 };
    /* payload with duration, enable-disable, and password */
    uint8_t payload_4[] = { 0x00, 0x05, 0xf1, 0x11, 0x0a, 0x00,
                            0x19, 0x00, 0x2d, 0x09, 0x00, 0x66,
                            0x69, 0x73, 0x74, 0x65, 0x72 };
    /* payload submitted with bug report */
    uint8_t payload_5[] = { 0x0d, 0xff, 0x80, 0x00, 0x03, 0x1a,
                            0x0a, 0x19, 0x00, 0x2a, 0x00, 0x41 };
    int len = 0;
    uint8_t test_invoke_id = 0;
    uint16_t test_timeDuration = 0;
    BACNET_COMMUNICATION_ENABLE_DISABLE test_enable_disable;
    BACNET_CHARACTER_STRING test_password;

    len = dcc_decode_apdu(
        &payload_1[0], sizeof(payload_1), &test_invoke_id, &test_timeDuration,
        &test_enable_disable, &test_password);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    len = dcc_decode_apdu(
        &payload_2[0], sizeof(payload_2), &test_invoke_id, &test_timeDuration,
        &test_enable_disable, &test_password);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    len = dcc_decode_apdu(
        &payload_3[0], sizeof(payload_3), &test_invoke_id, &test_timeDuration,
        &test_enable_disable, &test_password);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    len = dcc_decode_apdu(
        &payload_4[0], sizeof(payload_4), &test_invoke_id, &test_timeDuration,
        &test_enable_disable, &test_password);
    zassert_equal(len, BACNET_STATUS_ABORT, NULL);
    len = dcc_decode_apdu(
        &payload_5[0], sizeof(payload_5), &test_invoke_id, &test_timeDuration,
        &test_enable_disable, &test_password);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(dcc_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        dcc_tests, ztest_unit_test(test_DeviceCommunicationControl),
        ztest_unit_test(test_DeviceCommunicationControlMalformedData));

    ztest_run_test_suite(dcc_tests);
}
#endif
