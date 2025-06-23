/**
 * @file
 * @brief Unit test for ReadRange services encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/readrange.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static const char *read_range_request_type(int type)
{
    switch (type) {
        case RR_BY_POSITION:
            return "RR_BY_POSITION";
        case RR_BY_SEQUENCE:
            return "RR_BY_SEQUENCE";
        case RR_BY_TIME:
            return "RR_BY_TIME";
        case RR_READ_ALL:
            return "RR_READ_ALL";
        default:
            return "UNKNOWN";
    }
}

static int
testlist_item_encode(uint32_t object_instance, uint32_t item, uint8_t *apdu)
{
    int apdu_len = 0;

    apdu_len += encode_application_unsigned(apdu, object_instance);
    apdu_len += encode_application_unsigned(apdu, item);

    return apdu_len;
}

/**
 * @brief Test
 */
static void testReadRangeAckUnit(BACNET_READ_RANGE_DATA *data)
{
    uint8_t apdu[480] = { 0 };
    uint8_t apdu2[480] = { 0 };
    int apdu_len = 0, test_len = 0, null_len = 0;
    BACNET_READ_RANGE_DATA test_data = { 0 };
    BACNET_OBJECT_TYPE object_type = OBJECT_DEVICE;
    uint32_t object_instance = 0;
    BACNET_OBJECT_TYPE object = 0;

    data->application_data_len = encode_bacnet_object_id(
        &apdu2[0], data->object_type, data->object_instance);
    data->application_data = &apdu2[0];

    null_len = readrange_ack_service_encode(&apdu[0], sizeof(apdu), NULL);
    zassert_equal(null_len, 0, NULL);
    null_len = readrange_ack_service_encode(&apdu[0], 0, NULL);
    zassert_equal(null_len, 0, NULL);
    null_len = readrange_ack_service_encode(NULL, sizeof(apdu), data);
    zassert_not_equal(null_len, 0, NULL);
    apdu_len = readrange_ack_service_encode(&apdu[0], sizeof(apdu), data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    zassert_not_equal(apdu_len, 0, NULL);
    zassert_not_equal(apdu_len, BACNET_STATUS_ERROR, NULL);
    null_len = rr_ack_decode_service_request(NULL, apdu_len, &test_data);
    zassert_true(null_len < 0, NULL);
    test_len = rr_ack_decode_service_request(&apdu[0], apdu_len, &test_data);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_not_equal(test_len, -1, NULL);

    zassert_equal(test_data.object_type, data->object_type, NULL);
    zassert_equal(test_data.object_instance, data->object_instance, NULL);
    zassert_equal(test_data.object_property, data->object_property, NULL);
    zassert_equal(test_data.array_index, data->array_index, NULL);
    zassert_equal(
        test_data.application_data_len, data->application_data_len,
        "test app len=%d app len=%d", test_data.application_data_len,
        data->application_data_len);

    /* since object property == object_id, decode the application data using
       the appropriate decode function */
    test_len =
        decode_object_id(test_data.application_data, &object, &object_instance);
    object_type = object;
    zassert_equal(object_type, data->object_type, NULL);
    zassert_equal(object_instance, data->object_instance, NULL);
    while (apdu_len) {
        apdu_len--;
        if (apdu_len == 17) {
            /* boundary of optional parameters, so becomes valid */
            continue;
        }
        test_len =
            rr_ack_decode_service_request(&apdu[0], apdu_len, &test_data);
        zassert_true(
            test_len < 0, "test_len=%d apdu_len=%d", test_len, apdu_len);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(rp_tests, testReadRangeAck)
#else
static void testReadRangeAck(void)
#endif
{
    BACNET_READ_RANGE_DATA data = { 0 };

    data.object_type = OBJECT_DEVICE;
    data.object_instance = 1;
    data.object_property = PROP_OBJECT_IDENTIFIER;
    data.array_index = 0;
    data.RequestType = RR_READ_ALL;
    testReadRangeAckUnit(&data);
    data.array_index = BACNET_ARRAY_ALL;
    for (int i = 0; i < 3; i++) {
        data.ItemCount = i;
        data.RequestType = RR_READ_ALL;
        testReadRangeAckUnit(&data);
        /*  firstSequenceNumber - used only if 'Item Count' > 0 and
            the request was either of type 'By Sequence Number' or 'By Time' */
        for (int j = 0; j < 3; j++) {
            data.FirstSequence = j;
            data.RequestType = RR_BY_TIME;
            testReadRangeAckUnit(&data);
            data.RequestType = RR_BY_SEQUENCE;
            testReadRangeAckUnit(&data);
        }
        data.FirstSequence = 0;
        data.RequestType = RR_BY_POSITION;
        testReadRangeAckUnit(&data);
    }
}

static void testReadRangeUnit(BACNET_READ_RANGE_DATA *data)
{
    uint8_t apdu[480] = { 0 };
    int apdu_len = 0, test_len = 0, null_len = 0;
    uint32_t item_count = 5, item_count_total = 1200;
    BACNET_READ_RANGE_DATA test_data;

    null_len = read_range_request_encode(&apdu[0], 0, data);
    zassert_equal(null_len, 0, NULL);
    null_len = read_range_request_encode(&apdu[0], sizeof(apdu), NULL);
    zassert_equal(null_len, 0, NULL);
    null_len = read_range_request_encode(NULL, sizeof(apdu), data);
    zassert_not_equal(null_len, 0, NULL);
    apdu_len = read_range_request_encode(&apdu[0], sizeof(apdu), data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    zassert_not_equal(apdu_len, 0, NULL);
    null_len = rr_decode_service_request(NULL, apdu_len, &test_data);
    zassert_true(null_len < 0, NULL);
    test_len = rr_decode_service_request(&apdu[0], apdu_len, &test_data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    zassert_not_equal(test_len, -1, NULL);
    zassert_equal(test_data.object_type, data->object_type, NULL);
    zassert_equal(test_data.object_instance, data->object_instance, NULL);
    zassert_equal(test_data.object_property, data->object_property, NULL);
    zassert_equal(test_data.array_index, data->array_index, NULL);
    if (data->RequestType == RR_BY_POSITION) {
        test_len = readrange_ack_by_position_encode(
            data, testlist_item_encode, item_count, apdu, sizeof(apdu));
        if (data->Range.RefIndex >= item_count) {
            /* if the reference index must be less than the item count,
               or it should encode nothing */
            zassert_equal(test_len, 0, NULL);
        } else {
            zassert_not_equal(test_len, 0, NULL);
        }
    } else if (data->RequestType == RR_BY_SEQUENCE) {
        test_len = readrange_ack_by_sequence_encode(
            data, testlist_item_encode, item_count, item_count_total, apdu,
            sizeof(apdu));
        zassert_not_equal(test_len, 0, NULL);
        item_count_total = item_count;
        test_len = readrange_ack_by_sequence_encode(
            data, testlist_item_encode, item_count, item_count_total, apdu,
            sizeof(apdu));
        zassert_not_equal(test_len, 0, NULL);
    }
    while (apdu_len) {
        apdu_len--;
        test_len = rr_decode_service_request(&apdu[0], apdu_len, &test_data);
        if (apdu_len == 7) {
            /* boundary of optional parameters, so becomes valid */
            continue;
        }
        zassert_true(
            test_len < 0, "test_len=%d apdu_len=%d request=%s array=%u",
            test_len, apdu_len, read_range_request_type(data->RequestType),
            data->array_index);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(rp_tests, testReadRange)
#else
static void testReadRange(void)
#endif
{
    BACNET_READ_RANGE_DATA data = { 0 };

    data.object_type = OBJECT_DEVICE;
    data.object_instance = 1;
    data.object_property = PROP_OBJECT_IDENTIFIER;
    data.array_index = 0;
    data.RequestType = RR_READ_ALL;
    testReadRangeUnit(&data);
    data.array_index = BACNET_ARRAY_ALL;
    data.RequestType = RR_READ_ALL;
    testReadRangeUnit(&data);

    data.RequestType = RR_BY_POSITION;
    testReadRangeUnit(&data);
    data.Range.RefIndex = 5;
    data.RequestType = RR_BY_POSITION;
    testReadRangeUnit(&data);
    data.Range.RefIndex = 6;
    data.RequestType = RR_BY_POSITION;
    testReadRangeUnit(&data);

    data.Count = 0;
    data.RequestType = RR_BY_SEQUENCE;
    testReadRangeUnit(&data);
    data.Range.RefSeqNum = 5;
    data.RequestType = RR_BY_SEQUENCE;
    testReadRangeUnit(&data);
    data.Range.RefSeqNum = 6;
    data.RequestType = RR_BY_SEQUENCE;
    testReadRangeUnit(&data);
    data.Range.RefSeqNum = 1200;
    data.RequestType = RR_BY_SEQUENCE;
    testReadRangeUnit(&data);

    data.RequestType = RR_BY_TIME;
    testReadRangeUnit(&data);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(rp_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        readrange_tests, ztest_unit_test(testReadRange),
        ztest_unit_test(testReadRangeAck));

    ztest_run_test_suite(readrange_tests);
}
#endif
