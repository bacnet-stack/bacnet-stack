/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacapp.h>
#include <bacnet/bacenum.h>
#include <bacnet/ptransfer.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static int ptransfer_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_PRIVATE_TRANSFER_DATA *private_data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -1;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    /* invoke id - filled in by net layer */
    *invoke_id = apdu[2];
    if (apdu[3] != SERVICE_CONFIRMED_PRIVATE_TRANSFER)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len = ptransfer_decode_service_request(
            &apdu[offset], apdu_len - offset, private_data);
    }

    return len;
}

static int uptransfer_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    BACNET_PRIVATE_TRANSFER_DATA *private_data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu) {
        return -1;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) {
        return -1;
    }
    if (apdu[1] != SERVICE_UNCONFIRMED_PRIVATE_TRANSFER) {
        return -1;
    }
    offset = 2;
    if (apdu_len > offset) {
        len = ptransfer_decode_service_request(
            &apdu[offset], apdu_len - offset, private_data);
    }

    return len;
}

static int ptransfer_ack_decode_apdu(uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    uint8_t *invoke_id,
    BACNET_PRIVATE_TRANSFER_DATA *private_data)
{
    int len = 0;
    int offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK)
        return -1;
    *invoke_id = apdu[1];
    if (apdu[2] != SERVICE_CONFIRMED_PRIVATE_TRANSFER)
        return -1;
    offset = 3;
    if (apdu_len > offset) {
        len = ptransfer_decode_service_request(
            &apdu[offset], apdu_len - offset, private_data);
    }

    return len;
}

static int ptransfer_error_decode_apdu(uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    uint8_t *invoke_id,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code,
    BACNET_PRIVATE_TRANSFER_DATA *private_data)
{
    int len = 0;
    int offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_ERROR)
        return -1;
    *invoke_id = apdu[1];
    if (apdu[2] != SERVICE_CONFIRMED_PRIVATE_TRANSFER)
        return -1;
    offset = 3;
    if (apdu_len > offset) {
        len = ptransfer_error_decode_service_request(&apdu[offset],
            apdu_len - offset, error_class, error_code, private_data);
    }

    return len;
}

static void test_Private_Transfer_Ack(void)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    BACNET_PRIVATE_TRANSFER_DATA private_data;
    BACNET_PRIVATE_TRANSFER_DATA test_data;
    uint8_t test_value[480] = { 0 };
    int private_data_len = 0;
    char private_data_chunk[33] = { "00112233445566778899AABBCCDDEEFF" };
    BACNET_APPLICATION_DATA_VALUE data_value;
    BACNET_APPLICATION_DATA_VALUE test_data_value;
    bool status = false;

    private_data.vendorID = BACNET_VENDOR_ID;
    private_data.serviceNumber = 1;

    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,
        &private_data_chunk[0], &data_value);
    zassert_true(status, NULL);
    private_data_len =
        bacapp_encode_application_data(&test_value[0], &data_value);

    private_data.serviceParameters = &test_value[0];
    private_data.serviceParametersLen = private_data_len;

    len = ptransfer_ack_encode_apdu(&apdu[0], invoke_id, &private_data);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len = len;
    len = ptransfer_ack_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_data.vendorID, private_data.vendorID, NULL);
    zassert_equal(test_data.serviceNumber, private_data.serviceNumber, NULL);
    zassert_equal(
        test_data.serviceParametersLen, private_data.serviceParametersLen, NULL);
    len = bacapp_decode_application_data(test_data.serviceParameters,
        test_data.serviceParametersLen, &test_data_value);
    zassert_true(bacapp_same_value(&data_value, &test_data_value), NULL);
}

static void test_Private_Transfer_Error(void)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_RESOURCES;
    BACNET_ERROR_CODE error_code = ERROR_CODE_OPERATIONAL_PROBLEM;
    BACNET_ERROR_CLASS test_error_class = 0;
    BACNET_ERROR_CODE test_error_code = 0;
    BACNET_PRIVATE_TRANSFER_DATA private_data;
    BACNET_PRIVATE_TRANSFER_DATA test_data;
    uint8_t test_value[480] = { 0 };
    int private_data_len = 0;
    char private_data_chunk[33] = { "00112233445566778899AABBCCDDEEFF" };
    BACNET_APPLICATION_DATA_VALUE data_value;
    BACNET_APPLICATION_DATA_VALUE test_data_value;
    bool status = false;

    private_data.vendorID = BACNET_VENDOR_ID;
    private_data.serviceNumber = 1;

    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,
        &private_data_chunk[0], &data_value);
    zassert_true(status, NULL);
    private_data_len =
        bacapp_encode_application_data(&test_value[0], &data_value);
    private_data.serviceParameters = &test_value[0];
    private_data.serviceParametersLen = private_data_len;

    len = ptransfer_error_encode_apdu(
        &apdu[0], invoke_id, error_class, error_code, &private_data);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len = len;
    len = ptransfer_error_decode_apdu(&apdu[0], apdu_len, &test_invoke_id,
        &test_error_class, &test_error_code, &test_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(test_data.vendorID, private_data.vendorID, NULL);
    zassert_equal(test_data.serviceNumber, private_data.serviceNumber, NULL);
    zassert_equal(test_error_class, error_class, NULL);
    zassert_equal(test_error_code, error_code, NULL);
    zassert_equal(
        test_data.serviceParametersLen, private_data.serviceParametersLen, NULL);
    len = bacapp_decode_application_data(test_data.serviceParameters,
        test_data.serviceParametersLen, &test_data_value);
    zassert_true(bacapp_same_value(&data_value, &test_data_value), NULL);
}

static void test_Private_Transfer_Request(void)
{
    uint8_t apdu[480] = { 0 };
    uint8_t test_value[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    int private_data_len = 0;
    char private_data_chunk[33] = { "00112233445566778899AABBCCDDEEFF" };
    BACNET_APPLICATION_DATA_VALUE data_value = { 0 };
    BACNET_APPLICATION_DATA_VALUE test_data_value = { 0 };
    BACNET_PRIVATE_TRANSFER_DATA private_data = { 0 };
    BACNET_PRIVATE_TRANSFER_DATA test_data = { 0 };
    bool status = false;

    private_data.vendorID = BACNET_VENDOR_ID;
    private_data.serviceNumber = 1;

    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,
        &private_data_chunk[0], &data_value);
    zassert_true(status, NULL);
    private_data_len =
        bacapp_encode_application_data(&test_value[0], &data_value);
    private_data.serviceParameters = &test_value[0];
    private_data.serviceParametersLen = private_data_len;

    len = ptransfer_encode_apdu(&apdu[0], invoke_id, &private_data);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;
    len =
        ptransfer_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_data.vendorID, private_data.vendorID, NULL);
    zassert_equal(test_data.serviceNumber, private_data.serviceNumber, NULL);
    zassert_equal(
        test_data.serviceParametersLen, private_data.serviceParametersLen, NULL);
    len = bacapp_decode_application_data(test_data.serviceParameters,
        test_data.serviceParametersLen, &test_data_value);
    zassert_true(bacapp_same_value(&data_value, &test_data_value), NULL);

    return;
}

static void test_Unconfirmed_Private_Transfer_Request(void)
{
    uint8_t apdu[480] = { 0 };
    uint8_t test_value[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    int private_data_len = 0;
    char private_data_chunk[32] = { "I Love You, Patricia!" };
    BACNET_APPLICATION_DATA_VALUE data_value;
    BACNET_APPLICATION_DATA_VALUE test_data_value;
    BACNET_PRIVATE_TRANSFER_DATA private_data;
    BACNET_PRIVATE_TRANSFER_DATA test_data;
    bool status = false;

    private_data.vendorID = BACNET_VENDOR_ID;
    private_data.serviceNumber = 1;

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_CHARACTER_STRING,
            &private_data_chunk[0], &data_value);
    zassert_true(status, NULL);
    private_data_len =
        bacapp_encode_application_data(&test_value[0], &data_value);
    private_data.serviceParameters = &test_value[0];
    private_data.serviceParametersLen = private_data_len;

    len = uptransfer_encode_apdu(&apdu[0], &private_data);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;
    len = uptransfer_decode_apdu(&apdu[0], apdu_len, &test_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_data.vendorID, private_data.vendorID, NULL);
    zassert_equal(test_data.serviceNumber, private_data.serviceNumber, NULL);
    zassert_equal(
        test_data.serviceParametersLen, private_data.serviceParametersLen, NULL);
    len = bacapp_decode_application_data(test_data.serviceParameters,
        test_data.serviceParametersLen, &test_data_value);
    zassert_true(bacapp_same_value(&data_value, &test_data_value), NULL);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(ptransfer_tests,
     ztest_unit_test(test_Private_Transfer_Request),
     ztest_unit_test(test_Private_Transfer_Ack),
     ztest_unit_test(test_Private_Transfer_Error),
     ztest_unit_test(test_Unconfirmed_Private_Transfer_Request)
     );

    ztest_run_test_suite(ptransfer_tests);
}
