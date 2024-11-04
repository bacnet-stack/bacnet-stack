/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacpropstates.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacpropstates_tests, testPropStates)
#else
static void testPropStates(void)
#endif
{
    BACNET_PROPERTY_STATE data = { 0 };
    BACNET_PROPERTY_STATE test_data = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int apdu_len = 0;
    int test_len = 0;

    /*************************************************/
    data.tag = PROP_STATE_BOOLEAN_VALUE;
    data.state.booleanValue = true;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(
        test_len, apdu_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.booleanValue, test_data.state.booleanValue, NULL);

    /*************************************************/
    data.tag = PROP_STATE_BINARY_VALUE;
    data.state.binaryValue = BINARY_ACTIVE;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.binaryValue, test_data.state.binaryValue, NULL);

    /*************************************************/
    data.tag = PROP_STATE_EVENT_TYPE;
    data.state.eventType = EVENT_BUFFER_READY;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.eventType, test_data.state.eventType, NULL);

    /*************************************************/
    data.tag = PROP_STATE_POLARITY;
    data.state.polarity = POLARITY_REVERSE;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.polarity, test_data.state.polarity, NULL);

    /*************************************************/
    data.tag = PROP_STATE_PROGRAM_CHANGE;
    data.state.programChange = PROGRAM_REQUEST_RESTART;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(
        data.state.programChange, test_data.state.programChange, NULL);

    /*************************************************/
    data.tag = PROP_STATE_PROGRAM_STATE;
    data.state.programState = PROGRAM_STATE_HALTED;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.programState, test_data.state.programState, NULL);

    /*************************************************/
    data.tag = PROP_STATE_REASON_FOR_HALT;
    data.state.programError = PROGRAM_ERROR_LOAD_FAILED;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.programError, test_data.state.programError, NULL);

    /*************************************************/
    data.tag = PROP_STATE_RELIABILITY;
    data.state.reliability = RELIABILITY_COMMUNICATION_FAILURE;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.reliability, test_data.state.reliability, NULL);

    /*************************************************/
    data.tag = PROP_STATE_EVENT_STATE;
    data.state.state = EVENT_STATE_FAULT;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.state, test_data.state.state, NULL);

    /*************************************************/
    data.tag = PROP_STATE_SYSTEM_STATUS;
    data.state.systemStatus = STATUS_OPERATIONAL_READ_ONLY;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.systemStatus, test_data.state.systemStatus, NULL);

    /*************************************************/
    data.tag = PROP_STATE_UNITS;
    data.state.units = UNITS_BARS;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(data.state.units, test_data.state.units, NULL);

    /*************************************************/
    data.tag = PROP_STATE_UNSIGNED_VALUE;
    data.state.unsignedValue = 0xdeadbeef;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(
        data.state.unsignedValue, test_data.state.unsignedValue, NULL);

    /*************************************************/
    data.tag = PROP_STATE_LIFE_SAFETY_MODE;
    data.state.lifeSafetyMode = LIFE_SAFETY_MODE_ON;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(
        data.state.lifeSafetyMode, test_data.state.lifeSafetyMode, NULL);

    /*************************************************/
    data.tag = PROP_STATE_LIFE_SAFETY_STATE;
    data.state.lifeSafetyState = LIFE_SAFETY_STATE_ABNORMAL;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(
        data.state.lifeSafetyState, test_data.state.lifeSafetyState, NULL);

    /*************************************************/
    data.tag = PROP_STATE_RESTART_REASON;
    data.state.restartReason = RESTART_REASON_COLDSTART;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(
        data.state.restartReason, test_data.state.restartReason, NULL);

    /*************************************************/
    data.tag = PROP_STATE_DOOR_ALARM_STATE;
    data.state.doorAlarmState = DOOR_ALARM_STATE_ALARM;
    apdu_len = bacapp_encode_property_state(apdu, &data);
    memset(&test_data, 0, sizeof(test_data));
    test_len = bacapp_decode_property_state(apdu, &test_data);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(data.tag, test_data.tag, NULL);
    zassert_equal(
        data.state.doorAlarmState, test_data.state.doorAlarmState, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacpropstates_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bacpropstates_tests, ztest_unit_test(testPropStates));

    ztest_run_test_suite(bacpropstates_tests);
}
#endif
