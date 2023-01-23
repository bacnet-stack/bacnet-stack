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
static int getevent_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_OBJECT_ID *lastReceivedObjectIdentifier)
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
    if (apdu[3] != SERVICE_CONFIRMED_GET_EVENT_INFORMATION)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len = getevent_decode_service_request(
            &apdu[offset], apdu_len - offset, lastReceivedObjectIdentifier);
    }

    return len;
}

static int getevent_ack_decode_apdu(uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    uint8_t *invoke_id,
    BACNET_GET_EVENT_INFORMATION_DATA *get_event_data,
    bool *moreEvents)
{
    int len = 0;
    int offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK)
        return -1;
    *invoke_id = apdu[1];
    if (apdu[2] != SERVICE_CONFIRMED_GET_EVENT_INFORMATION)
        return -1;
    offset = 3;
    if (apdu_len > offset) {
        len = getevent_ack_decode_service_request(
            &apdu[offset], apdu_len - offset, get_event_data, moreEvents);
    }

    return len;
}

static void testGetEventInformationAck(void)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 1;
    uint8_t test_invoke_id = 0;
    BACNET_GET_EVENT_INFORMATION_DATA event_data;
    BACNET_GET_EVENT_INFORMATION_DATA test_event_data;
    bool moreEvents = false;
    bool test_moreEvents = false;
    unsigned i = 0;

    event_data.objectIdentifier.type = OBJECT_BINARY_INPUT;
    event_data.objectIdentifier.instance = 1;
    event_data.eventState = EVENT_STATE_NORMAL;
    bitstring_init(&event_data.acknowledgedTransitions);
    bitstring_set_bit(
        &event_data.acknowledgedTransitions, TRANSITION_TO_OFFNORMAL, false);
    bitstring_set_bit(
        &event_data.acknowledgedTransitions, TRANSITION_TO_FAULT, false);
    bitstring_set_bit(
        &event_data.acknowledgedTransitions, TRANSITION_TO_NORMAL, false);
    for (i = 0; i < 3; i++) {
        event_data.eventTimeStamps[i].tag = TIME_STAMP_SEQUENCE;
        event_data.eventTimeStamps[i].value.sequenceNum = 0;
    }
    event_data.notifyType = NOTIFY_ALARM;
    bitstring_init(&event_data.eventEnable);
    bitstring_set_bit(&event_data.eventEnable, TRANSITION_TO_OFFNORMAL, true);
    bitstring_set_bit(&event_data.eventEnable, TRANSITION_TO_FAULT, true);
    bitstring_set_bit(&event_data.eventEnable, TRANSITION_TO_NORMAL, true);
    for (i = 0; i < 3; i++) {
        event_data.eventPriorities[i] = 1;
    }
    event_data.next = NULL;

    len = getevent_ack_encode_apdu_init(&apdu[0], sizeof(apdu), invoke_id);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len = len;
    len = getevent_ack_encode_apdu_data(
        &apdu[apdu_len], sizeof(apdu) - apdu_len, &event_data);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len += len;
    len = getevent_ack_encode_apdu_end(
        &apdu[apdu_len], sizeof(apdu) - apdu_len, moreEvents);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len += len;
    len = getevent_ack_decode_apdu(&apdu[0],
        apdu_len, /* total length of the apdu */
        &test_invoke_id, &test_event_data, &test_moreEvents);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);

    zassert_equal(
        event_data.objectIdentifier.type,
            test_event_data.objectIdentifier.type, NULL);
    zassert_equal(
        event_data.objectIdentifier.instance,
            test_event_data.objectIdentifier.instance, NULL);

    zassert_equal(event_data.eventState, test_event_data.eventState, NULL);
}

static void testGetEventInformation(void)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    BACNET_OBJECT_ID lastReceivedObjectIdentifier;
    BACNET_OBJECT_ID test_lastReceivedObjectIdentifier;

    lastReceivedObjectIdentifier.type = OBJECT_BINARY_INPUT;
    lastReceivedObjectIdentifier.instance = 12345;
    len = getevent_encode_apdu(
        &apdu[0], invoke_id, &lastReceivedObjectIdentifier);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = getevent_decode_apdu(&apdu[0], apdu_len, &test_invoke_id,
        &test_lastReceivedObjectIdentifier);
    zassert_not_equal(len, -1, NULL);
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


void test_main(void)
{
    ztest_test_suite(getevent_tests,
     ztest_unit_test(testGetEventInformation),
     ztest_unit_test(testGetEventInformationAck)
     );

    ztest_run_test_suite(getevent_tests);
}
