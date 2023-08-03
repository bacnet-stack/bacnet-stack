/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/alarm_ack.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(alarm_ack_tests, testAlarmAck)
#else
static void testAlarmAck(void)
#endif
{
    BACNET_ALARM_ACK_DATA testAlarmAckIn;
    BACNET_ALARM_ACK_DATA testAlarmAckOut;

    uint8_t buffer[MAX_APDU];
    int inLen;
    int outLen;
    bool status;

    testAlarmAckIn.ackProcessIdentifier = 0x1234;
    characterstring_init_ansi(&testAlarmAckIn.ackSource, "This is a test");
    status = bacapp_timestamp_init_ascii(&testAlarmAckIn.ackTimeStamp, "1234");
    zassert_true(status, NULL);
    zassert_equal(testAlarmAckIn.ackTimeStamp.tag, TIME_STAMP_SEQUENCE, NULL);
    zassert_equal(testAlarmAckIn.ackTimeStamp.value.sequenceNum, 1234, NULL);

    testAlarmAckIn.eventObjectIdentifier.instance = 567;
    testAlarmAckIn.eventObjectIdentifier.type = OBJECT_DEVICE;
    status = bacapp_timestamp_init_ascii(
        &testAlarmAckIn.eventTimeStamp, "10:11:12.14");
    zassert_true(status, NULL);
    zassert_equal(testAlarmAckIn.eventTimeStamp.tag, TIME_STAMP_TIME, NULL);
    testAlarmAckIn.eventStateAcked = EVENT_STATE_OFFNORMAL;
    memset(&testAlarmAckOut, 0, sizeof(testAlarmAckOut));

    inLen = alarm_ack_encode_service_request(buffer, &testAlarmAckIn);
    outLen = alarm_ack_decode_service_request(buffer, inLen, &testAlarmAckOut);

    zassert_equal(inLen, outLen, "inlen=%d outlen=%d", inLen, outLen);

    zassert_equal(testAlarmAckIn.ackProcessIdentifier,
        testAlarmAckOut.ackProcessIdentifier, NULL);

    zassert_equal(testAlarmAckIn.ackTimeStamp.tag,
        testAlarmAckOut.ackTimeStamp.tag, "in-tag=%d out-tag=%d",
        testAlarmAckIn.ackTimeStamp.tag, testAlarmAckOut.ackTimeStamp.tag);
    zassert_equal(testAlarmAckIn.ackTimeStamp.value.sequenceNum,
        testAlarmAckOut.ackTimeStamp.value.sequenceNum, NULL);

    zassert_equal(testAlarmAckIn.ackProcessIdentifier,
        testAlarmAckOut.ackProcessIdentifier, NULL);

    zassert_equal(testAlarmAckIn.eventObjectIdentifier.instance,
        testAlarmAckOut.eventObjectIdentifier.instance, NULL);
    zassert_equal(testAlarmAckIn.eventObjectIdentifier.type,
        testAlarmAckOut.eventObjectIdentifier.type, NULL);

    zassert_equal(testAlarmAckIn.eventTimeStamp.tag,
        testAlarmAckOut.eventTimeStamp.tag, NULL);
    zassert_equal(testAlarmAckIn.eventTimeStamp.value.time.hour,
        testAlarmAckOut.eventTimeStamp.value.time.hour, NULL);
    zassert_equal(testAlarmAckIn.eventTimeStamp.value.time.min,
        testAlarmAckOut.eventTimeStamp.value.time.min, NULL);
    zassert_equal(testAlarmAckIn.eventTimeStamp.value.time.sec,
        testAlarmAckOut.eventTimeStamp.value.time.sec, NULL);
    zassert_equal(testAlarmAckIn.eventTimeStamp.value.time.hundredths,
        testAlarmAckOut.eventTimeStamp.value.time.hundredths, NULL);

    zassert_equal(
        testAlarmAckIn.eventStateAcked, testAlarmAckOut.eventStateAcked, NULL);

    status = bacapp_timestamp_init_ascii(
        &testAlarmAckIn.eventTimeStamp, "2021/12/31");
    zassert_true(status, NULL);
    zassert_equal(testAlarmAckIn.eventTimeStamp.tag, TIME_STAMP_DATETIME, NULL);
    inLen = alarm_ack_encode_service_request(buffer, &testAlarmAckIn);
    outLen = alarm_ack_decode_service_request(buffer, inLen, &testAlarmAckOut);
    zassert_equal(inLen, outLen, "inlen=%d outlen=%d", inLen, outLen);

    status =
        bacapp_timestamp_init_ascii(&testAlarmAckIn.eventTimeStamp, "1234");
    zassert_true(status, NULL);
    zassert_equal(testAlarmAckIn.eventTimeStamp.tag, TIME_STAMP_SEQUENCE, NULL);
    inLen = alarm_ack_encode_service_request(buffer, &testAlarmAckIn);
    outLen = alarm_ack_decode_service_request(buffer, inLen, &testAlarmAckOut);
    zassert_equal(inLen, outLen, "inlen=%d outlen=%d", inLen, outLen);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(alarm_ack_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(alarm_ack_tests, ztest_unit_test(testAlarmAck));

    ztest_run_test_suite(alarm_ack_tests);
}
#endif
