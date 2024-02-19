/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
// #include <bacnet/bacapp.h>
#include <bacnet/cov.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API for unsigned 16b integers
 */
int ccov_notify_decode_apdu(
    uint8_t *apdu, unsigned apdu_len, uint8_t *invoke_id, BACNET_COV_DATA *data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu) {
        return -1;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -2;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_COV_NOTIFICATION)
        return -3;
    offset = 4;

    /* optional limits - must be used as a pair */
    if (apdu_len > offset) {
        len = cov_notify_decode_service_request(
            &apdu[offset], apdu_len - offset, data);
    }

    return len;
}

int ucov_notify_decode_apdu(
    uint8_t *apdu, unsigned apdu_len, BACNET_COV_DATA *data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST)
        return -2;
    if (apdu[1] != SERVICE_UNCONFIRMED_COV_NOTIFICATION)
        return -3;
    /* optional limits - must be used as a pair */
    offset = 2;
    if (apdu_len > offset) {
        len = cov_notify_decode_service_request(
            &apdu[offset], apdu_len - offset, data);
    }

    return len;
}

static int cov_subscribe_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -2;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_SUBSCRIBE_COV)
        return -3;
    offset = 4;

    /* optional limits - must be used as a pair */
    if (apdu_len > offset) {
        len = cov_subscribe_decode_service_request(
            &apdu[offset], apdu_len - offset, data);
    }

    return len;
}

static int cov_subscribe_property_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -2;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY)
        return -3;
    offset = 4;

    /* optional limits - must be used as a pair */
    if (apdu_len > offset) {
        len = cov_subscribe_property_decode_service_request(
            &apdu[offset], apdu_len - offset, data);
    }

    return len;
}

/* dummy function stubs */
static void testCOVNotifyData(BACNET_COV_DATA *data, BACNET_COV_DATA *test_data)
{
    BACNET_PROPERTY_VALUE *value = NULL;
    BACNET_PROPERTY_VALUE *test_value = NULL;

    zassert_equal(test_data->subscriberProcessIdentifier,
        data->subscriberProcessIdentifier, NULL);
    zassert_equal(test_data->initiatingDeviceIdentifier,
        data->initiatingDeviceIdentifier, NULL);
    zassert_equal(test_data->monitoredObjectIdentifier.type,
        data->monitoredObjectIdentifier.type, NULL);
    zassert_equal(test_data->monitoredObjectIdentifier.instance,
        data->monitoredObjectIdentifier.instance, NULL);
    zassert_equal(test_data->timeRemaining, data->timeRemaining, NULL);
    /* test the listOfValues in some clever manner */
    value = data->listOfValues;
    test_value = test_data->listOfValues;
    while (value) {
        zassert_not_null(test_value, NULL);
        if (test_value) {
            zassert_equal(test_value->propertyIdentifier,
                value->propertyIdentifier, "property=%u test_property=%u",
                (unsigned)value->propertyIdentifier,
                (unsigned)test_value->propertyIdentifier);
            zassert_equal(test_value->propertyArrayIndex,
                value->propertyArrayIndex, NULL);
            zassert_equal(test_value->priority, value->priority, NULL);
            zassert_true(
                bacapp_same_value(&test_value->value, &value->value), NULL);
            test_value = test_value->next;
        }
        value = value->next;
    }
}

static void testUCOVNotifyData(BACNET_COV_DATA *data)
{
    uint8_t apdu[480] = { 0 };
    int len = 0, null_len = 0, apdu_len = 0;
    BACNET_COV_DATA test_data = { 0 };
    BACNET_PROPERTY_VALUE value_list[5] = { { 0 } };

    null_len = ucov_notify_encode_apdu(NULL, sizeof(apdu), data);
    len = ucov_notify_encode_apdu(&apdu[0], sizeof(apdu), data);
    zassert_true(len > 0, NULL);
    zassert_equal(len, null_len, NULL);
    apdu_len = len;

    cov_data_value_list_link(
        &test_data, &value_list[0], ARRAY_SIZE(value_list));
    len = ucov_notify_decode_apdu(&apdu[0], apdu_len, &test_data);
    zassert_not_equal(len, -1, NULL);
    testCOVNotifyData(data, &test_data);
}

static void testCCOVNotifyData(uint8_t invoke_id, BACNET_COV_DATA *data)
{
    uint8_t apdu[480] = { 0 };
    int len = 0, null_len = 0, apdu_len = 0;
    BACNET_COV_DATA test_data = { 0 };
    BACNET_PROPERTY_VALUE value_list[2] = { { 0 } };
    uint8_t test_invoke_id = 0;

    null_len = ccov_notify_encode_apdu(NULL, sizeof(apdu), invoke_id, data);
    len = ccov_notify_encode_apdu(&apdu[0], sizeof(apdu), invoke_id, data);
    zassert_not_equal(len, 0, NULL);
    zassert_equal(len, null_len, NULL);
    apdu_len = len;

    cov_data_value_list_link(&test_data, &value_list[0], 2);
    len = ccov_notify_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_true(len > 0, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    testCOVNotifyData(data, &test_data);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cov_tests, testCOVNotify)
#else
static void testCOVNotify(void)
#endif
{
    uint8_t invoke_id = 12;
    BACNET_COV_DATA data;
    BACNET_PROPERTY_VALUE value_list[2] = { { 0 } };

    data.subscriberProcessIdentifier = 1;
    data.initiatingDeviceIdentifier = 123;
    data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.monitoredObjectIdentifier.instance = 321;
    data.timeRemaining = 456;

    cov_data_value_list_link(&data, &value_list[0], 2);
    /* first value */
    value_list[0].propertyIdentifier = PROP_PRESENT_VALUE;
    value_list[0].propertyArrayIndex = BACNET_ARRAY_ALL;
    bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_REAL, "21.0", &value_list[0].value);
    value_list[0].priority = 0;
    /* second value */
    value_list[1].propertyIdentifier = PROP_STATUS_FLAGS;
    value_list[1].propertyArrayIndex = BACNET_ARRAY_ALL;
    bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_BIT_STRING, "0000", &value_list[1].value);
    value_list[1].priority = 0;

    testUCOVNotifyData(&data);
    testCCOVNotifyData(invoke_id, &data);
}

static void testCOVSubscribeData(
    BACNET_SUBSCRIBE_COV_DATA *data, BACNET_SUBSCRIBE_COV_DATA *test_data)
{
    zassert_equal(test_data->subscriberProcessIdentifier,
        data->subscriberProcessIdentifier, NULL);
    zassert_equal(test_data->monitoredObjectIdentifier.type,
        data->monitoredObjectIdentifier.type, NULL);
    zassert_equal(test_data->monitoredObjectIdentifier.instance,
        data->monitoredObjectIdentifier.instance, NULL);
    zassert_equal(
        test_data->cancellationRequest, data->cancellationRequest, NULL);
    if (test_data->cancellationRequest != data->cancellationRequest) {
        printf("cancellation request failed!\n");
    }
    if (!test_data->cancellationRequest) {
        zassert_equal(test_data->issueConfirmedNotifications,
            data->issueConfirmedNotifications, NULL);
        zassert_equal(test_data->lifetime, data->lifetime, NULL);
    }
}

static void testCOVSubscribePropertyData(
    BACNET_SUBSCRIBE_COV_DATA *data, BACNET_SUBSCRIBE_COV_DATA *test_data)
{
    testCOVSubscribeData(data, test_data);
    zassert_equal(test_data->monitoredProperty.propertyIdentifier,
        data->monitoredProperty.propertyIdentifier, NULL);
    zassert_equal(test_data->monitoredProperty.propertyArrayIndex,
        data->monitoredProperty.propertyArrayIndex, NULL);
    zassert_equal(
        test_data->covIncrementPresent, data->covIncrementPresent, NULL);
    if (test_data->covIncrementPresent) {
        zassert_false(islessgreater(test_data->covIncrement, 
            data->covIncrement), NULL);
    }
}

static void testCOVSubscribeEncoding(
    uint8_t invoke_id, BACNET_SUBSCRIBE_COV_DATA *data)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_SUBSCRIBE_COV_DATA test_data = { 0 };
    uint8_t test_invoke_id = 0;

    len = cov_subscribe_encode_apdu(&apdu[0], sizeof(apdu), invoke_id, data);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = cov_subscribe_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_true(len > 0, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    testCOVSubscribeData(data, &test_data);
}

static void testCOVSubscribePropertyEncoding(
    uint8_t invoke_id, BACNET_SUBSCRIBE_COV_DATA *data)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_SUBSCRIBE_COV_DATA test_data;
    uint8_t test_invoke_id = 0;

    len = cov_subscribe_property_encode_apdu(
        &apdu[0], sizeof(apdu), invoke_id, data);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = cov_subscribe_property_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_true(len > 0, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    testCOVSubscribePropertyData(data, &test_data);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cov_tests, testCOVSubscribe)
#else
static void testCOVSubscribe(void)
#endif
{
    uint8_t invoke_id = 12;
    BACNET_SUBSCRIBE_COV_DATA data = { 0 };

    data.subscriberProcessIdentifier = 1;
    data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.monitoredObjectIdentifier.instance = 321;
    data.cancellationRequest = false;
    data.issueConfirmedNotifications = true;
    data.lifetime = 456;

    testCOVSubscribeEncoding(invoke_id, &data);
    data.cancellationRequest = true;
    testCOVSubscribeEncoding(invoke_id, &data);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cov_tests, testCOVSubscribeProperty)
#else
static void testCOVSubscribeProperty(void)
#endif
{
    uint8_t invoke_id = 12;
    BACNET_SUBSCRIBE_COV_DATA data;

    data.subscriberProcessIdentifier = 1;
    data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.monitoredObjectIdentifier.instance = 321;
    data.cancellationRequest = false;
    data.issueConfirmedNotifications = true;
    data.lifetime = 456;
    data.monitoredProperty.propertyIdentifier = PROP_FILE_SIZE;
    data.monitoredProperty.propertyArrayIndex = BACNET_ARRAY_ALL;
    data.covIncrementPresent = true;
    data.covIncrement = 1.0;

    testCOVSubscribePropertyEncoding(invoke_id, &data);

    data.cancellationRequest = true;
    testCOVSubscribePropertyEncoding(invoke_id, &data);

    data.cancellationRequest = false;
    data.covIncrementPresent = false;
    testCOVSubscribePropertyEncoding(invoke_id, &data);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(cov_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(cov_tests, ztest_unit_test(testCOVNotify),
        ztest_unit_test(testCOVSubscribe),
        ztest_unit_test(testCOVSubscribeProperty));

    ztest_run_test_suite(cov_tests);
}
#endif
