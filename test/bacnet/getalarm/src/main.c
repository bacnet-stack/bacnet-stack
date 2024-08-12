/*
 * Copyright (c) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet service GetAlarmSummary encode and decode
 */

#include <zephyr/ztest.h>
#include <bacnet/get_alarm_sum.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Stub function for encoding
 * @param apdu - buffer to extract the decoding
 * @param apdu_size - size of the buffer
 * @param invoke_id - unique sequence number sent with the message
 * @return number of bytes decoded
 */
static int get_alarm_summary_decode_apdu(
    uint8_t *apdu, unsigned apdu_size, uint8_t *invoke_id)
{
    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size < 4) {
        return BACNET_STATUS_ERROR;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST) {
        return BACNET_STATUS_ERROR;
    }
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_GET_ALARM_SUMMARY) {
        return BACNET_STATUS_ERROR;
    }

    return 4;
}

/**
 * @brief Stub function for decoding
 * @param apdu - buffer containing encoded data
 * @param apdu_len - total length of the apdu
 * @param invoke_id - unique sequence number sent with the message
 * @return number of bytes decoded
 */
static int get_alarm_summary_ack_decode_apdu(
    uint8_t *apdu,
    int apdu_len,
    uint8_t *invoke_id,
    BACNET_GET_ALARM_SUMMARY_DATA *get_alarm_data)
{
    int len = 0;
    int offset = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK) {
        return BACNET_STATUS_ERROR;
    }
    *invoke_id = apdu[1];
    if (apdu[2] != SERVICE_CONFIRMED_GET_ALARM_SUMMARY) {
        return BACNET_STATUS_ERROR;
    }
    offset = 3;
    if (apdu_len > offset) {
        len = get_alarm_summary_ack_decode_apdu_data(
            &apdu[offset], apdu_len - offset, get_alarm_data);
        if (get_alarm_data) {
            get_alarm_data->next = NULL;
        }
    }

    return len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(getalarm_tests, testGetAlarmSummaryAck)
#else
static void testGetAlarmSummaryAck(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 1;
    uint8_t test_invoke_id = 0;
    BACNET_GET_ALARM_SUMMARY_DATA alarm_data;
    BACNET_GET_ALARM_SUMMARY_DATA test_alarm_data;

    alarm_data.objectIdentifier.type = OBJECT_BINARY_INPUT;
    alarm_data.objectIdentifier.instance = 1;
    alarm_data.alarmState = EVENT_STATE_NORMAL;
    bitstring_init(&alarm_data.acknowledgedTransitions);
    bitstring_set_bit(
        &alarm_data.acknowledgedTransitions, TRANSITION_TO_OFFNORMAL, false);
    bitstring_set_bit(
        &alarm_data.acknowledgedTransitions, TRANSITION_TO_FAULT, false);
    bitstring_set_bit(
        &alarm_data.acknowledgedTransitions, TRANSITION_TO_NORMAL, false);
    /* encode the initial service */
    len = get_alarm_summary_ack_encode_apdu_init(&apdu[0], invoke_id);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len = len;
    /* append the data portion of the service */
    len = get_alarm_summary_ack_encode_apdu_data(
        &apdu[apdu_len], sizeof(apdu) - apdu_len, &alarm_data);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, -1, NULL);
    apdu_len += len;
    len = get_alarm_summary_ack_decode_apdu(
        &apdu[0], apdu_len, /* total length of the apdu */
        &test_invoke_id, &test_alarm_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);

    zassert_equal(
        alarm_data.objectIdentifier.type, test_alarm_data.objectIdentifier.type,
        NULL);
    zassert_equal(
        alarm_data.objectIdentifier.instance,
        test_alarm_data.objectIdentifier.instance, NULL);

    zassert_equal(alarm_data.alarmState, test_alarm_data.alarmState, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(getalarm_tests, testGetAlarmSummary)
#else
static void testGetAlarmSummary(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;

    len = get_alarm_summary_encode_apdu(&apdu[0], invoke_id);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = get_alarm_summary_decode_apdu(&apdu[0], apdu_len, &test_invoke_id);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_invoke_id, invoke_id, NULL);

    return;
}

/**
 * Main entry point for testing
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(getalarm_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        getalarm_tests, ztest_unit_test(testGetAlarmSummary),
        ztest_unit_test(testGetAlarmSummaryAck));

    ztest_run_test_suite(getalarm_tests);
}
#endif
