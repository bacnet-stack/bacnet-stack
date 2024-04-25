/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/rp.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static int rp_decode_apdu(
    uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    unsigned apdu_len = 0;

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
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_READ_PROPERTY) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len = 4;

    if (apdu_size > apdu_len) {
        len = rp_decode_service_request(
            &apdu[apdu_len], apdu_size - apdu_len, rpdata);
        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = len;
        }
    }

    return apdu_len;
}

static int rp_ack_decode_apdu(
    uint8_t *apdu,
    int apdu_size,
    uint8_t *invoke_id,
    BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    int apdu_len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size <= 3) {
        return BACNET_STATUS_ERROR;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK) {
        return BACNET_STATUS_ERROR;
    }
    *invoke_id = apdu[1];
    if (apdu[2] != SERVICE_CONFIRMED_READ_PROPERTY) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len = 3;
    if (apdu_size > apdu_len) {
        len = rp_ack_decode_service_request(
            &apdu[apdu_len], apdu_size - apdu_len, rpdata);
        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = len;
        }
    }

    return apdu_len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(rp_tests, testReadPropertyAck)
#else
static void testReadPropertyAck(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    uint8_t apdu2[480] = { 0 };
    int apdu_len = 0, test_len = 0, null_len = 0;
    uint8_t invoke_id = 1;
    uint8_t test_invoke_id = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_READ_PROPERTY_DATA test_data = { 0 };
    BACNET_OBJECT_TYPE object_type = OBJECT_DEVICE;
    uint32_t object_instance = 0;
    BACNET_OBJECT_TYPE object = 0;

    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;

    rpdata.application_data_len = encode_bacnet_object_id(
        &apdu2[0], rpdata.object_type, rpdata.object_instance);
    rpdata.application_data = &apdu2[0];

    null_len = rp_ack_encode_apdu(NULL, invoke_id, &rpdata);
    apdu_len = rp_ack_encode_apdu(&apdu[0], invoke_id, &rpdata);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    zassert_not_equal(apdu_len, 0, NULL);
    zassert_not_equal(apdu_len, BACNET_STATUS_ERROR, NULL);
    test_len =
        rp_ack_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);

    zassert_equal(test_data.object_type, rpdata.object_type, NULL);
    zassert_equal(test_data.object_instance, rpdata.object_instance, NULL);
    zassert_equal(test_data.object_property, rpdata.object_property, NULL);
    zassert_equal(test_data.array_index, rpdata.array_index, NULL);
    zassert_equal(
        test_data.application_data_len, rpdata.application_data_len, NULL);

    /* since object property == object_id, decode the application data using
       the appropriate decode function */
    test_len =
        decode_object_id(test_data.application_data, &object, &object_instance);
    object_type = object;
    zassert_equal(object_type, rpdata.object_type, NULL);
    zassert_equal(object_instance, rpdata.object_instance, NULL);
    while (apdu_len) {
        apdu_len--;
        if ((apdu_len <= 15) && (apdu_len >= 11)) {
            /* boundary of optional parameters, so becomes valid */
            continue;
        }
        test_len =
            rp_ack_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
        zassert_true(
            test_len < 0, "test_len=%d apdu_len=%d", test_len, apdu_len);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(rp_tests, testReadProperty)
#else
static void testReadProperty(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int apdu_len = 0, test_len = 0, null_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_READ_PROPERTY_DATA test_data;

    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    null_len = rp_encode_apdu(NULL, invoke_id, &rpdata);
    apdu_len = rp_encode_apdu(&apdu[0], invoke_id, &rpdata);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    zassert_not_equal(apdu_len, 0, NULL);

    test_len = rp_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(test_data.object_type, rpdata.object_type, NULL);
    zassert_equal(test_data.object_instance, rpdata.object_instance, NULL);
    zassert_equal(test_data.object_property, rpdata.object_property, NULL);
    zassert_equal(test_data.array_index, rpdata.array_index, NULL);
    while (apdu_len) {
        apdu_len--;
        test_len =
            rp_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
        zassert_true(
            test_len < 0, "test_len=%d apdu_len=%d", test_len, apdu_len);
    }

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(rp_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        rp_tests, ztest_unit_test(testReadProperty),
        ztest_unit_test(testReadPropertyAck));

    ztest_run_test_suite(rp_tests);
}
#endif
