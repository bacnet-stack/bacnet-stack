/* @file
 * @brief test BACnet AtomicReadFile service encode/decode APIs
 * @date 2007
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/arf.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testAtomicReadFileAckAccess(BACNET_ATOMIC_READ_FILE_DATA *data)
{
    BACNET_ATOMIC_READ_FILE_DATA test_data = { 0 };
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    int null_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    unsigned int i = 0;

    null_len = arf_ack_encode_apdu(NULL, invoke_id, data);
    len = arf_ack_encode_apdu(&apdu[0], invoke_id, data);
    zassert_not_equal(len, 0, NULL);
    zassert_equal(null_len, len, NULL);
    apdu_len = len;

    null_len = arf_ack_decode_apdu(&apdu[0], apdu_len, NULL, NULL);
    len = arf_ack_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_true(len > 0, NULL);
    zassert_equal(null_len, len, NULL);
    zassert_equal(test_data.endOfFile, data->endOfFile, NULL);
    zassert_equal(test_data.access, data->access, NULL);
    if (test_data.access == FILE_STREAM_ACCESS) {
        zassert_equal(
            test_data.type.stream.fileStartPosition,
            data->type.stream.fileStartPosition, NULL);
        zassert_equal(
            octetstring_length(&test_data.fileData[0]),
            octetstring_length(&data->fileData[0]), NULL);
        zassert_equal(
            memcmp(
                octetstring_value(&test_data.fileData[0]),
                octetstring_value(&data->fileData[0]),
                octetstring_length(&test_data.fileData[0])),
            0, NULL);
    } else if (test_data.access == FILE_RECORD_ACCESS) {
        zassert_equal(
            test_data.type.record.fileStartRecord,
            data->type.record.fileStartRecord, NULL);
        zassert_equal(
            test_data.type.record.RecordCount, data->type.record.RecordCount,
            NULL);
        for (i = 0; i < data->type.record.RecordCount; i++) {
            zassert_equal(
                octetstring_length(&test_data.fileData[i]),
                octetstring_length(&data->fileData[i]), NULL);
            zassert_equal(
                memcmp(
                    octetstring_value(&test_data.fileData[i]),
                    octetstring_value(&data->fileData[i]),
                    octetstring_length(&test_data.fileData[i])),
                0, NULL);
        }
    }
    /* test APDU too short */
    while (apdu_len) {
        apdu_len--;
        len = arf_ack_decode_apdu(apdu, apdu_len, NULL, NULL);
        zassert_true(len < 0, "len=%d apdu_len=%d", len, apdu_len);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(arf_tests, testAtomicReadFileAck)
#else
static void testAtomicReadFileAck(void)
#endif
{
    BACNET_ATOMIC_READ_FILE_DATA data = { 0 };
    uint8_t test_octet_string[32] = "Joshua-Mary-Anna-Christopher";
    unsigned int i = 0;

    data.endOfFile = true;
    data.access = FILE_STREAM_ACCESS;
    data.type.stream.fileStartPosition = 0;
    octetstring_init(
        &data.fileData[0], test_octet_string, sizeof(test_octet_string));
    testAtomicReadFileAckAccess(&data);

    data.endOfFile = false;
    data.access = FILE_RECORD_ACCESS;
    data.type.record.fileStartRecord = 1;
    data.type.record.RecordCount = BACNET_READ_FILE_RECORD_COUNT;
    for (i = 0; i < data.type.record.RecordCount; i++) {
        octetstring_init(
            &data.fileData[i], test_octet_string, sizeof(test_octet_string));
    }
    testAtomicReadFileAckAccess(&data);

    return;
}

static void testAtomicReadFileAccess(const BACNET_ATOMIC_READ_FILE_DATA *data)
{
    BACNET_ATOMIC_READ_FILE_DATA test_data = { 0 };
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int null_len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;

    null_len = arf_encode_apdu(NULL, invoke_id, data);
    len = arf_encode_apdu(&apdu[0], invoke_id, data);
    zassert_not_equal(len, 0, NULL);
    zassert_equal(len, null_len, NULL);
    apdu_len = len;

    null_len = arf_decode_apdu(&apdu[0], apdu_len, NULL, NULL);
    len = arf_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_equal(len, null_len, NULL);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_data.object_type, data->object_type, NULL);
    zassert_equal(test_data.object_instance, data->object_instance, NULL);
    zassert_equal(test_data.access, data->access, NULL);
    if (test_data.access == FILE_STREAM_ACCESS) {
        zassert_equal(
            test_data.type.stream.fileStartPosition,
            data->type.stream.fileStartPosition, NULL);
        zassert_equal(
            test_data.type.stream.requestedOctetCount,
            data->type.stream.requestedOctetCount, NULL);
    } else if (test_data.access == FILE_RECORD_ACCESS) {
        zassert_equal(
            test_data.type.record.fileStartRecord,
            data->type.record.fileStartRecord, NULL);
        zassert_equal(
            test_data.type.record.RecordCount, data->type.record.RecordCount,
            NULL);
    }
    /* test APDU too short */
    while (apdu_len) {
        apdu_len--;
        len = arf_decode_apdu(apdu, apdu_len, NULL, NULL);
        zassert_true(len < 0, "len=%d apdu_len=%d", len, apdu_len);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(arf_tests, testAtomicReadFile)
#else
static void testAtomicReadFile(void)
#endif
{
    BACNET_ATOMIC_READ_FILE_DATA data = { 0 };

    data.object_type = OBJECT_FILE;
    data.object_instance = 1;
    data.access = FILE_STREAM_ACCESS;
    data.type.stream.fileStartPosition = 0;
    data.type.stream.requestedOctetCount = 128;
    testAtomicReadFileAccess(&data);

    data.object_type = OBJECT_FILE;
    data.object_instance = 2;
    data.access = FILE_RECORD_ACCESS;
    data.type.record.fileStartRecord = 1;
    data.type.record.RecordCount = 2;
    testAtomicReadFileAccess(&data);

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(arf_tests, testAtomicReadFileMalformed)
#else
static void testAtomicReadFileMalformed(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    /* payloads with malformation */
    uint8_t payload_1[] = { 0xc4, 0x02, 0x80, 0x00, 0x00, 0x0e,
                            0x35, 0xff, 0xdf, 0x62, 0xee, 0x00,
                            0x00, 0x22, 0x05, 0x84, 0x0f };
    uint8_t payload_2[] = { 0xc4, 0x02, 0x80, 0x00, 0x00, 0x0e, 0x31, 0x00,
                            0x25, 0xff, 0xd4, 0x9e, 0xbf, 0x79, 0x05, 0x84 };
    BACNET_ATOMIC_READ_FILE_DATA data = { 0 };
    int len = 0;
    uint8_t test_invoke_id = 0;

    len = arf_decode_apdu(NULL, sizeof(apdu), &test_invoke_id, &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    len = arf_decode_apdu(apdu, sizeof(apdu), &test_invoke_id, &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
    apdu[1] = 0;
    apdu[2] = 1;
    len = arf_decode_apdu(apdu, sizeof(apdu), &test_invoke_id, &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    apdu[2] = SERVICE_CONFIRMED_ATOMIC_READ_FILE;
    len = arf_decode_apdu(apdu, sizeof(apdu), &test_invoke_id, &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    /* malformed payload testing */
    len = arf_decode_service_request(payload_1, sizeof(payload_1), &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    len = arf_decode_service_request(payload_2, sizeof(payload_2), &data);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(arf_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        arf_tests, ztest_unit_test(testAtomicReadFile),
        ztest_unit_test(testAtomicReadFileAck),
        ztest_unit_test(testAtomicReadFileMalformed));

    ztest_run_test_suite(arf_tests);
}
#endif
