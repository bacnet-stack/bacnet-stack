/**
 * @file
 * @brief unit test for BACnetAcknowledgeAlarmInfo encoding and decoding
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Greg Shue <greg.shue@outlook.com>
 * @date 2009
 * @copyright SPDX-License-Identifier: MIT
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
    BACNET_ALARM_ACK_DATA data;
    BACNET_ALARM_ACK_DATA test_data;

    uint8_t apdu[MAX_APDU];
    int null_len, test_len, apdu_len;
    bool status;

    data.ackProcessIdentifier = 0x1234;
    characterstring_init_ansi(&data.ackSource, "This is a test");
    status = bacapp_timestamp_init_ascii(&data.ackTimeStamp, "1234");
    zassert_true(status, NULL);
    zassert_equal(data.ackTimeStamp.tag, TIME_STAMP_SEQUENCE, NULL);
    zassert_equal(data.ackTimeStamp.value.sequenceNum, 1234, NULL);

    data.eventObjectIdentifier.instance = 567;
    data.eventObjectIdentifier.type = OBJECT_DEVICE;
    status = bacapp_timestamp_init_ascii(&data.eventTimeStamp, "10:11:12.14");
    zassert_true(status, NULL);
    zassert_equal(data.eventTimeStamp.tag, TIME_STAMP_TIME, NULL);
    data.eventStateAcked = EVENT_STATE_OFFNORMAL;
    memset(&test_data, 0, sizeof(test_data));

    apdu_len =
        bacnet_acknowledge_alarm_info_request_encode(apdu, sizeof(apdu), &data);
    null_len =
        bacnet_acknowledge_alarm_info_request_encode(NULL, apdu_len, &data);
    zassert_equal(null_len, apdu_len, NULL);
    test_len = alarm_ack_decode_service_request(apdu, apdu_len, &test_data);

    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);

    zassert_equal(
        data.ackProcessIdentifier, test_data.ackProcessIdentifier, NULL);

    zassert_equal(
        data.ackTimeStamp.tag, test_data.ackTimeStamp.tag,
        "in-tag=%d out-tag=%d", data.ackTimeStamp.tag,
        test_data.ackTimeStamp.tag);
    zassert_equal(
        data.ackTimeStamp.value.sequenceNum,
        test_data.ackTimeStamp.value.sequenceNum, NULL);

    zassert_equal(
        data.ackProcessIdentifier, test_data.ackProcessIdentifier, NULL);

    zassert_equal(
        data.eventObjectIdentifier.instance,
        test_data.eventObjectIdentifier.instance, NULL);
    zassert_equal(
        data.eventObjectIdentifier.type, test_data.eventObjectIdentifier.type,
        NULL);

    zassert_equal(data.eventTimeStamp.tag, test_data.eventTimeStamp.tag, NULL);
    zassert_equal(
        data.eventTimeStamp.value.time.hour,
        test_data.eventTimeStamp.value.time.hour, NULL);
    zassert_equal(
        data.eventTimeStamp.value.time.min,
        test_data.eventTimeStamp.value.time.min, NULL);
    zassert_equal(
        data.eventTimeStamp.value.time.sec,
        test_data.eventTimeStamp.value.time.sec, NULL);
    zassert_equal(
        data.eventTimeStamp.value.time.hundredths,
        test_data.eventTimeStamp.value.time.hundredths, NULL);

    zassert_equal(data.eventStateAcked, test_data.eventStateAcked, NULL);

    status = bacapp_timestamp_init_ascii(&data.eventTimeStamp, "2021/12/31");
    zassert_true(status, NULL);
    zassert_equal(data.eventTimeStamp.tag, TIME_STAMP_DATETIME, NULL);
    apdu_len =
        bacnet_acknowledge_alarm_info_request_encode(apdu, sizeof(apdu), &data);
    null_len =
        bacnet_acknowledge_alarm_info_request_encode(NULL, apdu_len, &data);
    zassert_equal(null_len, apdu_len, NULL);
    test_len = alarm_ack_decode_service_request(apdu, apdu_len, &test_data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);

    status = bacapp_timestamp_init_ascii(&data.eventTimeStamp, "1234");
    zassert_true(status, NULL);
    zassert_equal(data.eventTimeStamp.tag, TIME_STAMP_SEQUENCE, NULL);
    apdu_len =
        bacnet_acknowledge_alarm_info_request_encode(apdu, sizeof(apdu), &data);
    null_len =
        bacnet_acknowledge_alarm_info_request_encode(NULL, apdu_len, &data);
    zassert_equal(null_len, apdu_len, NULL);
    test_len = alarm_ack_decode_service_request(apdu, apdu_len, &test_data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    while (--apdu_len) {
        test_len =
            bacnet_acknowledge_alarm_info_request_encode(apdu, apdu_len, &data);
        zassert_equal(
            test_len, 0, "apdu_len=%d test_len=%d", apdu_len, test_len);
    }
    apdu_len = null_len;
    while (--apdu_len) {
        test_len = alarm_ack_decode_service_request(apdu, apdu_len, &data);
        zassert_true(
            test_len < 0, "apdu_len=%d test_len=%d", apdu_len, test_len);
    }
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
