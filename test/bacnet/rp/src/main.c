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
static int rp_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_READ_PROPERTY_DATA *rpdata)
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
    if (apdu[3] != SERVICE_CONFIRMED_READ_PROPERTY)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len =
            rp_decode_service_request(&apdu[offset], apdu_len - offset, rpdata);
    }

    return len;
}

static int rp_ack_decode_apdu(uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    uint8_t *invoke_id,
    BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    int offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK)
        return -1;
    *invoke_id = apdu[1];
    if (apdu[2] != SERVICE_CONFIRMED_READ_PROPERTY)
        return -1;
    offset = 3;
    if (apdu_len > offset) {
        len = rp_ack_decode_service_request(
            &apdu[offset], apdu_len - offset, rpdata);
    }

    return len;
}

static void testReadPropertyAck(void)
{
    uint8_t apdu[480] = { 0 };
    uint8_t apdu2[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 1;
    uint8_t test_invoke_id = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_READ_PROPERTY_DATA test_data;
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

    len = rp_ack_encode_apdu(&apdu[0], invoke_id, &rpdata);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len = len;
    len = rp_ack_decode_apdu(&apdu[0], apdu_len, /* total length of the apdu */
        &test_invoke_id, &test_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);

    zassert_equal(test_data.object_type, rpdata.object_type, NULL);
    zassert_equal(test_data.object_instance, rpdata.object_instance, NULL);
    zassert_equal(test_data.object_property, rpdata.object_property, NULL);
    zassert_equal(test_data.array_index, rpdata.array_index, NULL);
    zassert_equal(
        test_data.application_data_len, rpdata.application_data_len, NULL);

    /* since object property == object_id, decode the application data using
       the appropriate decode function */
    len =
        decode_object_id(test_data.application_data, &object, &object_instance);
    object_type = object;
    zassert_equal(object_type, rpdata.object_type, NULL);
    zassert_equal(object_instance, rpdata.object_instance, NULL);
}

static void testReadProperty(void)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_READ_PROPERTY_DATA test_data;

    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = rp_encode_apdu(&apdu[0], invoke_id, &rpdata);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = rp_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_data.object_type, rpdata.object_type, NULL);
    zassert_equal(test_data.object_instance, rpdata.object_instance, NULL);
    zassert_equal(test_data.object_property, rpdata.object_property, NULL);
    zassert_equal(test_data.array_index, rpdata.array_index, NULL);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(rp_tests,
     ztest_unit_test(testReadProperty),
     ztest_unit_test(testReadPropertyAck)
     );

    ztest_run_test_suite(rp_tests);
}
