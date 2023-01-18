/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/timestamp.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testTimestampSequence(void)
{
    BACNET_TIMESTAMP testTimestampIn;
    BACNET_TIMESTAMP testTimestampOut;
    uint8_t buffer[MAX_APDU];
    int inLen;
    int outLen;

    testTimestampIn.tag = TIME_STAMP_SEQUENCE;
    testTimestampIn.value.sequenceNum = 0x1234;

    memset(&testTimestampOut, 0, sizeof(testTimestampOut));

    inLen = bacapp_encode_context_timestamp(buffer, 2, &testTimestampIn);
    outLen = bacapp_decode_context_timestamp(buffer, 2, &testTimestampOut);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(testTimestampIn.tag, testTimestampOut.tag, NULL);
    zassert_equal(
        testTimestampIn.value.sequenceNum,
            testTimestampOut.value.sequenceNum, NULL);
}

static void testTimestampTime(void)
{
    BACNET_TIMESTAMP testTimestampIn;
    BACNET_TIMESTAMP testTimestampOut;
    uint8_t buffer[MAX_APDU];
    int inLen;
    int outLen;

    testTimestampIn.tag = TIME_STAMP_TIME;
    testTimestampIn.value.time.hour = 1;
    testTimestampIn.value.time.min = 2;
    testTimestampIn.value.time.sec = 3;
    testTimestampIn.value.time.hundredths = 4;

    memset(&testTimestampOut, 0, sizeof(testTimestampOut));

    inLen = bacapp_encode_context_timestamp(buffer, 2, &testTimestampIn);
    outLen = bacapp_decode_context_timestamp(buffer, 2, &testTimestampOut);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(testTimestampIn.tag, testTimestampOut.tag, NULL);
    zassert_equal(
        testTimestampIn.value.time.hour, testTimestampOut.value.time.hour, NULL);
    zassert_equal(
        testTimestampIn.value.time.min, testTimestampOut.value.time.min, NULL);
    zassert_equal(
        testTimestampIn.value.time.sec, testTimestampOut.value.time.sec, NULL);
    zassert_equal(
        testTimestampIn.value.time.hundredths,
            testTimestampOut.value.time.hundredths, NULL);
}

static void testTimestampTimeDate(void)
{
    BACNET_TIMESTAMP testTimestampIn;
    BACNET_TIMESTAMP testTimestampOut;
    uint8_t buffer[MAX_APDU];
    int inLen;
    int outLen;

    testTimestampIn.tag = TIME_STAMP_DATETIME;
    testTimestampIn.value.dateTime.time.hour = 1;
    testTimestampIn.value.dateTime.time.min = 2;
    testTimestampIn.value.dateTime.time.sec = 3;
    testTimestampIn.value.dateTime.time.hundredths = 4;

    testTimestampIn.value.dateTime.date.year = 1901;
    testTimestampIn.value.dateTime.date.month = 1;
    testTimestampIn.value.dateTime.date.wday = 2;
    testTimestampIn.value.dateTime.date.day = 3;

    memset(&testTimestampOut, 0, sizeof(testTimestampOut));

    inLen = bacapp_encode_context_timestamp(buffer, 2, &testTimestampIn);
    outLen = bacapp_decode_context_timestamp(buffer, 2, &testTimestampOut);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(testTimestampIn.tag, testTimestampOut.tag, NULL);
    zassert_equal(
        testTimestampIn.value.dateTime.time.hour,
            testTimestampOut.value.dateTime.time.hour, NULL);
    zassert_equal(
        testTimestampIn.value.dateTime.time.min,
            testTimestampOut.value.dateTime.time.min, NULL);
    zassert_equal(
        testTimestampIn.value.dateTime.time.sec,
            testTimestampOut.value.dateTime.time.sec, NULL);
    zassert_equal(
        testTimestampIn.value.dateTime.time.hundredths,
            testTimestampOut.value.dateTime.time.hundredths, NULL);

    zassert_equal(
        testTimestampIn.value.dateTime.date.year,
            testTimestampOut.value.dateTime.date.year, NULL);
    zassert_equal(
        testTimestampIn.value.dateTime.date.month,
            testTimestampOut.value.dateTime.date.month, NULL);
    zassert_equal(
        testTimestampIn.value.dateTime.date.wday,
            testTimestampOut.value.dateTime.date.wday, NULL);
    zassert_equal(
        testTimestampIn.value.dateTime.date.day,
            testTimestampOut.value.dateTime.date.day, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(timestamp_tests,
     ztest_unit_test(testTimestampSequence),
     ztest_unit_test(testTimestampTime),
     ztest_unit_test(testTimestampTimeDate)
     );

    ztest_run_test_suite(timestamp_tests);
}
