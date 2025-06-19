/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/getevent.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static int getevent_decode_apdu(
    const uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_OBJECT_ID *lastReceivedObjectIdentifier)
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
        if (apdu[3] != SERVICE_CONFIRMED_GET_EVENT_INFORMATION) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len = 4;
    }

    if (apdu_size > apdu_len) {
        len = getevent_decode_service_request(
            &apdu[apdu_len], apdu_size - apdu_len,
            lastReceivedObjectIdentifier);
        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = len;
        }
    }

    return apdu_len;
}

static int getevent_ack_decode_apdu(
    const uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    uint8_t *invoke_id,
    BACNET_GET_EVENT_INFORMATION_DATA *get_event_data,
    bool *moreEvents)
{
    int len = 0;
    int offset = 0;

    if (!apdu) {
        return -1;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK) {
        return -1;
    }
    *invoke_id = apdu[1];
    if (apdu[2] != SERVICE_CONFIRMED_GET_EVENT_INFORMATION) {
        return -1;
    }
    offset = 3;
    if (apdu_len > offset) {
        len = getevent_ack_decode_service_request(
            &apdu[offset], apdu_len - offset, get_event_data, moreEvents);
    }

    return len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(getevent_tests, testGetEventInformationAck)
#else
static void testGetEventInformationAck(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 1;
    uint8_t test_invoke_id = 0;
    BACNET_GET_EVENT_INFORMATION_DATA event_data[1] = { 0 };
    BACNET_GET_EVENT_INFORMATION_DATA test_event_data[1] = { 0 };
    bool moreEvents = false;
    bool test_moreEvents = false;
    unsigned i = 0;

    getevent_information_link_array(
        &test_event_data[0], ARRAY_SIZE(test_event_data));
    getevent_information_link_array(&event_data[0], ARRAY_SIZE(event_data));

    event_data[0].objectIdentifier.type = OBJECT_BINARY_INPUT;
    event_data[0].objectIdentifier.instance = 1;
    event_data[0].eventState = EVENT_STATE_NORMAL;
    bitstring_init(&event_data[0].acknowledgedTransitions);
    bitstring_set_bit(
        &event_data[0].acknowledgedTransitions, TRANSITION_TO_OFFNORMAL, false);
    bitstring_set_bit(
        &event_data[0].acknowledgedTransitions, TRANSITION_TO_FAULT, false);
    bitstring_set_bit(
        &event_data[0].acknowledgedTransitions, TRANSITION_TO_NORMAL, false);
    for (i = 0; i < 3; i++) {
        event_data[0].eventTimeStamps[i].tag = TIME_STAMP_SEQUENCE;
        event_data[0].eventTimeStamps[i].value.sequenceNum = 0;
    }
    event_data[0].notifyType = NOTIFY_ALARM;
    bitstring_init(&event_data[0].eventEnable);
    bitstring_set_bit(
        &event_data[0].eventEnable, TRANSITION_TO_OFFNORMAL, true);
    bitstring_set_bit(&event_data[0].eventEnable, TRANSITION_TO_FAULT, true);
    bitstring_set_bit(&event_data[0].eventEnable, TRANSITION_TO_NORMAL, true);
    for (i = 0; i < 3; i++) {
        event_data[0].eventPriorities[i] = 1;
    }
    len = getevent_ack_encode_apdu_init(&apdu[0], sizeof(apdu), invoke_id);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len = len;
    len = getevent_ack_encode_apdu_data(
        &apdu[apdu_len], sizeof(apdu) - apdu_len, &event_data[0]);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len += len;
    len = getevent_ack_encode_apdu_end(
        &apdu[apdu_len], sizeof(apdu) - apdu_len, moreEvents);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len += len;
    len = getevent_ack_decode_apdu(
        &apdu[0], apdu_len, /* total length of the apdu */
        &test_invoke_id, &test_event_data[0], &test_moreEvents);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);

    zassert_equal(
        event_data[0].objectIdentifier.type,
        test_event_data[0].objectIdentifier.type, NULL);
    zassert_equal(
        event_data[0].objectIdentifier.instance,
        test_event_data[0].objectIdentifier.instance, NULL);

    zassert_equal(
        event_data[0].eventState, test_event_data[0].eventState, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(getevent_tests, testGetEventInformation)
#else
static void testGetEventInformation(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int apdu_len, test_len, null_len;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    BACNET_OBJECT_ID lastReceivedObjectIdentifier;
    BACNET_OBJECT_ID test_lastReceivedObjectIdentifier;

    lastReceivedObjectIdentifier.type = OBJECT_BINARY_INPUT;
    lastReceivedObjectIdentifier.instance = 12345;
    null_len =
        getevent_encode_apdu(NULL, invoke_id, &lastReceivedObjectIdentifier);
    apdu_len = getevent_encode_apdu(
        &apdu[0], invoke_id, &lastReceivedObjectIdentifier);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_not_equal(apdu_len, 0, NULL);

    test_len = getevent_decode_apdu(
        &apdu[0], apdu_len, &test_invoke_id,
        &test_lastReceivedObjectIdentifier);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);
    zassert_equal(
        test_lastReceivedObjectIdentifier.type,
        lastReceivedObjectIdentifier.type, NULL);
    zassert_equal(
        test_lastReceivedObjectIdentifier.instance,
        lastReceivedObjectIdentifier.instance, NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(getevent_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        getevent_tests, ztest_unit_test(testGetEventInformation),
        ztest_unit_test(testGetEventInformationAck));

    ztest_run_test_suite(getevent_tests);
}
#endif
