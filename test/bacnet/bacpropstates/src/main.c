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
void testPropStates(void)
{
    BACNET_PROPERTY_STATE inData;
    BACNET_PROPERTY_STATE outData;
    uint8_t appMsg[MAX_APDU];
    int inLen;
    int outLen;

    inData.tag = BOOLEAN_VALUE;
    inData.state.booleanValue = true;

    inLen = bacapp_encode_property_state(appMsg, &inData);

    memset(&outData, 0, sizeof(outData));

    outLen = bacapp_decode_property_state(appMsg, &outData);

    zassert_equal(outLen, inLen, NULL);
    zassert_equal(inData.tag, outData.tag, NULL);
    zassert_equal(inData.state.booleanValue, outData.state.booleanValue, NULL);

    /****************
    *****************
    ****************/
    inData.tag = BINARY_VALUE;
    inData.state.binaryValue = BINARY_ACTIVE;

    inLen = bacapp_encode_property_state(appMsg, &inData);

    memset(&outData, 0, sizeof(outData));

    outLen = bacapp_decode_property_state(appMsg, &outData);

    zassert_equal(outLen, inLen, NULL);
    zassert_equal(inData.tag, outData.tag, NULL);
    zassert_equal(inData.state.binaryValue, outData.state.binaryValue, NULL);

    /****************
    *****************
    ****************/
    inData.tag = EVENT_TYPE;
    inData.state.eventType = EVENT_BUFFER_READY;

    inLen = bacapp_encode_property_state(appMsg, &inData);

    memset(&outData, 0, sizeof(outData));

    outLen = bacapp_decode_property_state(appMsg, &outData);

    zassert_equal(outLen, inLen, NULL);
    zassert_equal(inData.tag, outData.tag, NULL);
    zassert_equal(inData.state.eventType, outData.state.eventType, NULL);

    /****************
    *****************
    ****************/
    inData.tag = POLARITY;
    inData.state.polarity = POLARITY_REVERSE;

    inLen = bacapp_encode_property_state(appMsg, &inData);

    memset(&outData, 0, sizeof(outData));

    outLen = bacapp_decode_property_state(appMsg, &outData);

    zassert_equal(outLen, inLen, NULL);
    zassert_equal(inData.tag, outData.tag, NULL);
    zassert_equal(inData.state.polarity, outData.state.polarity, NULL);

    /****************
    *****************
    ****************/
    inData.tag = PROGRAM_CHANGE;
    inData.state.programChange = PROGRAM_REQUEST_RESTART;

    inLen = bacapp_encode_property_state(appMsg, &inData);

    memset(&outData, 0, sizeof(outData));

    outLen = bacapp_decode_property_state(appMsg, &outData);

    zassert_equal(outLen, inLen, NULL);
    zassert_equal(inData.tag, outData.tag, NULL);
    zassert_equal(inData.state.programChange, outData.state.programChange, NULL);

    /****************
    *****************
    ****************/
    inData.tag = UNSIGNED_VALUE;
    inData.state.unsignedValue = 0xdeadbeef;

    inLen = bacapp_encode_property_state(appMsg, &inData);

    memset(&outData, 0, sizeof(outData));

    outLen = bacapp_decode_property_state(appMsg, &outData);

    zassert_equal(outLen, inLen, NULL);
    zassert_equal(inData.tag, outData.tag, NULL);
    zassert_equal(inData.state.unsignedValue, outData.state.unsignedValue, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(bacpropstates_tests,
     ztest_unit_test(testPropStates)
     );

    ztest_run_test_suite(bacpropstates_tests);
}
