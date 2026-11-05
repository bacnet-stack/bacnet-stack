/*
 * Copyright (c) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * @copyright SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet service GetAlarmSummary encode and decode
 */

#include <string.h>
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
    const uint8_t *apdu, unsigned apdu_size, uint8_t *invoke_id)
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
    const uint8_t *apdu,
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
    BACNET_GET_ALARM_SUMMARY_DATA alarm_data = { 0 };
    BACNET_GET_ALARM_SUMMARY_DATA test_alarm_data = { 0 };

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
    zassert_equal(
        len, get_alarm_summary_ack_encode_apdu_init(NULL, invoke_id), NULL);
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
    zassert_equal(len, apdu_len - 3, NULL);
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
ZTEST(getalarm_tests, testGetAlarmSummaryAckData)
#else
static void testGetAlarmSummaryAckData(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, apdu_len = 0, null_len = 0;
    unsigned i = 0;
    BACNET_GET_ALARM_SUMMARY_DATA alarm_data = { 0 };
    BACNET_GET_ALARM_SUMMARY_DATA test_alarm_data = { 0 };

    alarm_data.objectIdentifier.type = OBJECT_ANALOG_INPUT;
    alarm_data.objectIdentifier.instance = BACNET_MAX_INSTANCE;
    /* large enumeration and bitstring to exceed the 10-byte minimum */
    alarm_data.alarmState = (BACNET_EVENT_STATE)300;
    bitstring_init(&alarm_data.acknowledgedTransitions);
    for (i = 0; i < 16; i++) {
        bitstring_set_bit(
            &alarm_data.acknowledgedTransitions, (uint8_t)i, (i % 3) == 0);
    }
    /* encode */
    apdu_len =
        get_alarm_summary_ack_encode_apdu_data(apdu, sizeof(apdu), &alarm_data);
    zassert_true(apdu_len > 10, NULL);
    len =
        get_alarm_summary_ack_encode_apdu_data(NULL, sizeof(apdu), &alarm_data);
    zassert_equal(len, apdu_len, NULL);
    len = get_alarm_summary_ack_encode_apdu_data(NULL, 0, &alarm_data);
    zassert_equal(len, apdu_len, NULL);
    len = get_alarm_summary_ack_data_encode(NULL, &alarm_data);
    zassert_equal(len, apdu_len, NULL);
    len = get_alarm_summary_ack_data_encode(apdu, NULL);
    zassert_equal(len, 0, NULL);
    len = get_alarm_summary_ack_encode_apdu_data(apdu, 9, &alarm_data);
    zassert_equal(len, BACNET_STATUS_ABORT, NULL);
    len =
        get_alarm_summary_ack_encode_apdu_data(apdu, apdu_len - 1, &alarm_data);
    zassert_equal(len, BACNET_STATUS_ABORT, NULL);
    len = get_alarm_summary_ack_encode_apdu_data(apdu, sizeof(apdu), NULL);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    len = get_alarm_summary_ack_encode_apdu_data(apdu, apdu_len, &alarm_data);
    zassert_equal(len, apdu_len, NULL);
    /* decode */
    len = get_alarm_summary_ack_decode_apdu_data(
        apdu, apdu_len, &test_alarm_data);
    zassert_equal(len, apdu_len, NULL);
    zassert_equal(
        test_alarm_data.objectIdentifier.type, alarm_data.objectIdentifier.type,
        NULL);
    zassert_equal(
        test_alarm_data.objectIdentifier.instance,
        alarm_data.objectIdentifier.instance, NULL);
    zassert_equal(test_alarm_data.alarmState, alarm_data.alarmState, NULL);
    zassert_true(
        bitstring_same(
            &test_alarm_data.acknowledgedTransitions,
            &alarm_data.acknowledgedTransitions),
        NULL);
    /* decode with NULL data returns the same length */
    null_len = get_alarm_summary_ack_decode_apdu_data(apdu, apdu_len, NULL);
    zassert_equal(null_len, apdu_len, NULL);
    /* NULL apdu */
    len = get_alarm_summary_ack_decode_apdu_data(
        NULL, apdu_len, &test_alarm_data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    /* truncated data */
    for (i = 0; i < (unsigned)apdu_len; i++) {
        len = get_alarm_summary_ack_decode_apdu_data(apdu, i, NULL);
        zassert_true(len <= 0, "len=%d at size=%u", len, i);
    }
    /* wrong tag for each field */
    apdu[0] = 0x21; /* unsigned, len 1 */
    len = get_alarm_summary_ack_decode_apdu_data(apdu, apdu_len, NULL);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    get_alarm_summary_ack_encode_apdu_data(apdu, sizeof(apdu), &alarm_data);
    apdu[5] = 0x10; /* boolean */
    len = get_alarm_summary_ack_decode_apdu_data(apdu, apdu_len, NULL);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    get_alarm_summary_ack_encode_apdu_data(apdu, sizeof(apdu), &alarm_data);
    apdu[8] = 0x10; /* boolean */
    len = get_alarm_summary_ack_decode_apdu_data(apdu, apdu_len, NULL);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    /* malformed tag that is not complete */
    memset(apdu, 0xFF, sizeof(apdu));
    len = get_alarm_summary_ack_decode_apdu_data(apdu, 10, NULL);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
}

static unsigned Iterate_Count;
static BACNET_GET_ALARM_SUMMARY_DATA Iterate_Data[3];

static void
test_iterate_callback(void *context, const BACNET_GET_ALARM_SUMMARY_DATA *data)
{
    zassert_equal(context, &Iterate_Count, NULL);
    if (Iterate_Count < 3) {
        Iterate_Data[Iterate_Count] = *data;
    }
    Iterate_Count++;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(getalarm_tests, testGetAlarmSummaryAckIterate)
#else
static void testGetAlarmSummaryAckIterate(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, apdu_len = 0, entry_len = 0;
    unsigned i = 0;
    BACNET_GET_ALARM_SUMMARY_DATA alarm_data = { 0 };

    alarm_data.objectIdentifier.type = OBJECT_ANALOG_INPUT;
    alarm_data.alarmState = EVENT_STATE_OFFNORMAL;
    bitstring_init(&alarm_data.acknowledgedTransitions);
    for (i = 0; i < MAX_BACNET_EVENT_TRANSITION; i++) {
        bitstring_set_bit(&alarm_data.acknowledgedTransitions, i, true);
    }
    for (i = 0; i < 3; i++) {
        alarm_data.objectIdentifier.instance = i + 1;
        entry_len = get_alarm_summary_ack_encode_apdu_data(
            &apdu[apdu_len], sizeof(apdu) - apdu_len, &alarm_data);
        zassert_true(entry_len > 0, NULL);
        apdu_len += entry_len;
    }
    /* all entries are reported in order */
    Iterate_Count = 0;
    len = get_alarm_summary_ack_decode_apdu_iterate(
        apdu, apdu_len, test_iterate_callback, &Iterate_Count);
    zassert_equal(len, apdu_len, NULL);
    zassert_equal(Iterate_Count, 3, NULL);
    for (i = 0; i < 3; i++) {
        zassert_equal(Iterate_Data[i].objectIdentifier.instance, i + 1, NULL);
    }
    /* validate only */
    len = get_alarm_summary_ack_decode_apdu_iterate(apdu, apdu_len, NULL, NULL);
    zassert_equal(len, apdu_len, NULL);
    /* empty ACK is valid and reports nothing */
    Iterate_Count = 0;
    len = get_alarm_summary_ack_decode_apdu_iterate(
        apdu, 0, test_iterate_callback, &Iterate_Count);
    zassert_equal(len, 0, NULL);
    len = get_alarm_summary_ack_decode_apdu_iterate(
        NULL, 0, test_iterate_callback, &Iterate_Count);
    zassert_equal(len, 0, NULL);
    zassert_equal(Iterate_Count, 0, NULL);
    /* NULL apdu with data */
    len = get_alarm_summary_ack_decode_apdu_iterate(
        NULL, apdu_len, test_iterate_callback, &Iterate_Count);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    /* truncation anywhere but an entry boundary is an error with no callback */
    for (i = 1; i < (unsigned)apdu_len; i++) {
        Iterate_Count = 0;
        len = get_alarm_summary_ack_decode_apdu_iterate(
            apdu, i, test_iterate_callback, &Iterate_Count);
        if ((i % entry_len) == 0) {
            zassert_equal(len, (int)i, "size=%u", i);
            zassert_equal(Iterate_Count, i / entry_len, "size=%u", i);
        } else {
            zassert_equal(len, BACNET_STATUS_ERROR, "size=%u", i);
            zassert_equal(Iterate_Count, 0, "size=%u", i);
        }
    }
    /* malformed second entry: no entries are reported */
    apdu[entry_len] = 0xFF;
    Iterate_Count = 0;
    len = get_alarm_summary_ack_decode_apdu_iterate(
        apdu, apdu_len, test_iterate_callback, &Iterate_Count);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(Iterate_Count, 0, NULL);
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
    zassert_equal(len, get_alarm_summary_encode_apdu(NULL, invoke_id), NULL);
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
        ztest_unit_test(testGetAlarmSummaryAck),
        ztest_unit_test(testGetAlarmSummaryAckData),
        ztest_unit_test(testGetAlarmSummaryAckIterate));

    ztest_run_test_suite(getalarm_tests);
}
#endif
