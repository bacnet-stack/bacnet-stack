/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/awf.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testAtomicWriteFileAccess(BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    BACNET_ATOMIC_WRITE_FILE_DATA test_data = { 0 };
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    int null_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;

    null_len = awf_encode_apdu(NULL, invoke_id, data);
    len = awf_encode_apdu(&apdu[0], invoke_id, data);
    zassert_not_equal(len, 0, NULL);
    zassert_equal(len, null_len, NULL);
    apdu_len = len;

    null_len = awf_decode_apdu(&apdu[0], apdu_len, NULL, NULL);
    len = awf_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(len, null_len, NULL);
    zassert_equal(test_data.object_type, data->object_type, NULL);
    zassert_equal(test_data.object_instance, data->object_instance, NULL);
    zassert_equal(test_data.access, data->access, NULL);
    if (test_data.access == FILE_STREAM_ACCESS) {
        zassert_equal(
            test_data.type.stream.fileStartPosition,
            data->type.stream.fileStartPosition, NULL);
    } else if (test_data.access == FILE_RECORD_ACCESS) {
        zassert_equal(
            test_data.type.record.fileStartRecord,
            data->type.record.fileStartRecord, NULL);
        zassert_equal(
            test_data.type.record.returnedRecordCount,
            data->type.record.returnedRecordCount, NULL);
    }
    zassert_equal(
        octetstring_length(&test_data.fileData[0]),
        octetstring_length(&data->fileData[0]), NULL);
    zassert_equal(
        memcmp(
            octetstring_value(&test_data.fileData[0]),
            octetstring_value(&data->fileData[0]),
            octetstring_length(&test_data.fileData[0])),
        0, NULL);
    /* test APDU too short */
    while (apdu_len) {
        apdu_len--;
        len = awf_decode_apdu(apdu, apdu_len, NULL, NULL);
        zassert_true(len < 0, "len=%d apdu_len=%d", len, apdu_len);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(awf_tests, testAtomicWriteFile)
#else
static void testAtomicWriteFile(void)
#endif
{
    BACNET_ATOMIC_WRITE_FILE_DATA data = { 0 };
    uint8_t test_octet_string[32] = "Joshua-Mary-Anna-Christopher";

    data.object_type = OBJECT_FILE;
    data.object_instance = 1;
    data.access = FILE_STREAM_ACCESS;
    data.type.stream.fileStartPosition = 0;
    octetstring_init(
        &data.fileData[0], test_octet_string, sizeof(test_octet_string));
    testAtomicWriteFileAccess(&data);

    data.object_type = OBJECT_FILE;
    data.object_instance = 1;
    data.access = FILE_RECORD_ACCESS;
    data.type.record.fileStartRecord = 1;
    data.type.record.returnedRecordCount = 1;
    octetstring_init(
        &data.fileData[0], test_octet_string, sizeof(test_octet_string));
    testAtomicWriteFileAccess(&data);

    return;
}

static void
testAtomicWriteFileAckAccess(const BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    BACNET_ATOMIC_WRITE_FILE_DATA test_data = { 0 };
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    int null_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;

    null_len = awf_ack_encode_apdu(NULL, invoke_id, data);
    len = awf_ack_encode_apdu(&apdu[0], invoke_id, data);
    zassert_not_equal(len, 0, NULL);
    zassert_equal(len, null_len, NULL);
    apdu_len = len;

    null_len = awf_ack_decode_apdu(&apdu[0], apdu_len, NULL, NULL);
    len = awf_ack_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_equal(len, null_len, NULL);
    if (len == BACNET_STATUS_ERROR) {
        if (data->access == FILE_STREAM_ACCESS) {
            printf("testing FILE_STREAM_ACCESS failed decode\n");
        } else if (data->access == FILE_RECORD_ACCESS) {
            printf("testing FILE_RECORD_ACCESS failed decode\n");
        }
    }
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(test_data.access, data->access, NULL);
    if (test_data.access == FILE_STREAM_ACCESS) {
        zassert_equal(
            test_data.type.stream.fileStartPosition,
            data->type.stream.fileStartPosition, NULL);
    } else if (test_data.access == FILE_RECORD_ACCESS) {
        zassert_equal(
            test_data.type.record.fileStartRecord,
            data->type.record.fileStartRecord, NULL);
    }
    /* test APDU too short */
    while (apdu_len) {
        apdu_len--;
        len = awf_ack_decode_apdu(apdu, apdu_len, NULL, NULL);
        zassert_true(len < 0, "len=%d apdu_len=%d", len, apdu_len);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(awf_tests, testAtomicWriteFileAck)
#else
static void testAtomicWriteFileAck(void)
#endif
{
    BACNET_ATOMIC_WRITE_FILE_DATA data = { 0 };

    data.access = FILE_STREAM_ACCESS;
    data.type.stream.fileStartPosition = 42;
    testAtomicWriteFileAckAccess(&data);

    data.access = FILE_RECORD_ACCESS;
    data.type.record.fileStartRecord = 54;
    testAtomicWriteFileAckAccess(&data);

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(awf_tests, testAtomicWriteFileMalformed)
#else
static void testAtomicWriteFileMalformed(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    /* payloads with malformation */
    uint8_t payload_1[] = { 0xc4, 0x02, 0x80, 0x00, 0x00, 0x0e,
                            0x35, 0xff, 0x5e, 0xd5, 0xc0, 0x85,
                            0x0a, 0x62, 0x64, 0x0a, 0x0f };
    uint8_t payload_2[] = { 0xc4, 0x02, 0x80, 0x00, 0x00, 0x0e, 0x35,
                            0xff, 0xc4, 0x4d, 0x92, 0xd9, 0x0a, 0x62,
                            0x64, 0x0a, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
                            0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
                            0x0f, 0x0f, 0x0f, 0x0f, 0x0f };
    BACNET_ATOMIC_WRITE_FILE_DATA data = { 0 };
    int len = 0;
    uint8_t test_invoke_id = 0;

    len = awf_decode_apdu(NULL, sizeof(apdu), &test_invoke_id, &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    len = awf_decode_apdu(apdu, sizeof(apdu), &test_invoke_id, &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
    apdu[1] = 0;
    apdu[2] = 1;
    len = awf_decode_apdu(apdu, sizeof(apdu), &test_invoke_id, &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    apdu[2] = SERVICE_CONFIRMED_ATOMIC_WRITE_FILE;
    len = awf_decode_apdu(apdu, sizeof(apdu), &test_invoke_id, &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    /* malformed payload testing */
    len = awf_decode_service_request(payload_1, sizeof(payload_1), &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    len = awf_decode_service_request(payload_2, sizeof(payload_2), &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(awf_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        awf_tests, ztest_unit_test(testAtomicWriteFile),
        ztest_unit_test(testAtomicWriteFileAck),
        ztest_unit_test(testAtomicWriteFileMalformed));

    ztest_run_test_suite(awf_tests);
}
#endif
