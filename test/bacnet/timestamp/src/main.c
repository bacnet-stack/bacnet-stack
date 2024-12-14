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
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(timestamp_tests, testTimestampSequence)
#else
static void testTimestampSequence(void)
#endif
{
    BACNET_TIMESTAMP testTimestampIn;
    BACNET_TIMESTAMP testTimestampOut;
    uint8_t buffer[MAX_APDU];
    int len;
    int test_len;

    testTimestampIn.tag = TIME_STAMP_SEQUENCE;
    testTimestampIn.value.sequenceNum = 0x1234;

    memset(&testTimestampOut, 0, sizeof(testTimestampOut));

    len = bacapp_encode_context_timestamp(buffer, 2, &testTimestampIn);
    test_len = bacapp_decode_context_timestamp(buffer, 2, &testTimestampOut);

    zassert_equal(len, test_len, NULL);
    zassert_equal(testTimestampIn.tag, testTimestampOut.tag, NULL);
    zassert_equal(
        testTimestampIn.value.sequenceNum, testTimestampOut.value.sequenceNum,
        NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(timestamp_tests, testTimestampTime)
#else
static void testTimestampTime(void)
#endif
{
    BACNET_TIMESTAMP testTimestampIn;
    BACNET_TIMESTAMP testTimestampOut;
    uint8_t buffer[MAX_APDU];
    bool status = false;
    char str[64] = "";
    int len;
    int test_len;

    testTimestampIn.tag = TIME_STAMP_TIME;
    testTimestampIn.value.time.hour = 1;
    testTimestampIn.value.time.min = 2;
    testTimestampIn.value.time.sec = 3;
    testTimestampIn.value.time.hundredths = 4;

    memset(&testTimestampOut, 0, sizeof(testTimestampOut));

    len = bacapp_encode_context_timestamp(buffer, 2, &testTimestampIn);
    test_len = bacapp_decode_context_timestamp(buffer, 2, &testTimestampOut);

    zassert_equal(len, test_len, NULL);
    zassert_equal(testTimestampIn.tag, testTimestampOut.tag, NULL);
    zassert_equal(
        testTimestampIn.value.time.hour, testTimestampOut.value.time.hour,
        NULL);
    zassert_equal(
        testTimestampIn.value.time.min, testTimestampOut.value.time.min, NULL);
    zassert_equal(
        testTimestampIn.value.time.sec, testTimestampOut.value.time.sec, NULL);
    zassert_equal(
        testTimestampIn.value.time.hundredths,
        testTimestampOut.value.time.hundredths, NULL);

    bacapp_timestamp_to_ascii(str, sizeof(str), &testTimestampIn);
    status = bacapp_timestamp_init_ascii(&testTimestampOut, str);
    zassert_true(status, NULL);
    status = bacapp_timestamp_same(&testTimestampIn, &testTimestampOut);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(timestamp_tests, testTimestampTimeDate)
#else
static void testTimestampTimeDate(void)
#endif
{
    BACNET_TIMESTAMP testTimestampIn;
    BACNET_TIMESTAMP testTimestampOut = { 0 };
    uint8_t tag_number = 2;
    uint8_t buffer[MAX_APDU];
    int len;
    int test_len;
    int null_len;
    bool status;

    status =
        bacapp_timestamp_init_ascii(&testTimestampIn, "1901/01/03-1:02:03");
    zassert_true(status, NULL);
    null_len = bacapp_encode_timestamp(NULL, &testTimestampIn);
    len = bacapp_encode_timestamp(buffer, &testTimestampIn);
    zassert_equal(null_len, len, NULL);
    null_len = bacnet_timestamp_decode(buffer, len, NULL);
    test_len = bacnet_timestamp_decode(buffer, len, &testTimestampOut);
    zassert_equal(null_len, test_len, NULL);
    zassert_equal(len, test_len, "len=%d test_len=%d", len, test_len);
    /* test ERROR when APDU is too short*/
    while (len) {
        len--;
        test_len = bacnet_timestamp_decode(buffer, len, &testTimestampOut);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    }
    null_len =
        bacapp_encode_context_timestamp(NULL, tag_number, &testTimestampIn);
    len = bacapp_encode_context_timestamp(buffer, tag_number, &testTimestampIn);
    zassert_equal(null_len, len, NULL);
    zassert_true(len > 0, NULL);
    null_len = bacnet_timestamp_context_decode(buffer, len, tag_number, NULL);
    test_len = bacnet_timestamp_context_decode(
        buffer, len, tag_number, &testTimestampOut);
    zassert_equal(null_len, test_len, NULL);
    zassert_equal(len, test_len, NULL);
    /* test ERROR when APDU is too short*/
    while (len) {
        len--;
        test_len = bacnet_timestamp_context_decode(
            buffer, len, tag_number, &testTimestampOut);
        zassert_true(test_len <= 0, "len=%d test_len=%d", len, test_len);
    }
    /* test for valid values */
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(timestamp_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        timestamp_tests, ztest_unit_test(testTimestampSequence),
        ztest_unit_test(testTimestampTime),
        ztest_unit_test(testTimestampTimeDate));

    ztest_run_test_suite(timestamp_tests);
}
#endif
