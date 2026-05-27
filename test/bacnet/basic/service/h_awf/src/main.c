/**
 * @file
 * @brief Functional tests for handler_atomic_write_file bounds checks
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 *
 * Tests exercise the two guards added to handler_atomic_write_file:
 *   1. fileStartPosition < -1  -> PDU_TYPE_ERROR (INVALID_FILE_START_POSITION)
 *   2. fileStartRecord < -1    -> PDU_TYPE_ERROR (INVALID_FILE_START_POSITION)
 * and verify the success paths still produce PDU_TYPE_COMPLEX_ACK.
 */
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacenum.h"
#include "bacnet/bacstr.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/awf.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

/* Stub control variables defined in bacfile_stub.c */
extern bool Bacfile_Valid_Instance_Result;
extern bool Bacfile_Write_Stream_Data_Result;
extern bool Bacfile_Write_Record_Data_Result;

/**
 * @brief Build a minimal BACNET_CONFIRMED_SERVICE_DATA for use as the
 *        service_data argument.
 */
static void
make_service_data(BACNET_CONFIRMED_SERVICE_DATA *sd, uint8_t invoke_id)
{
    memset(sd, 0, sizeof(BACNET_CONFIRMED_SERVICE_DATA));
    sd->invoke_id = invoke_id;
    sd->priority = MESSAGE_PRIORITY_NORMAL;
    sd->segmented_message = false;
}

/**
 * @brief Build a minimal AWF stream request into service_request[].
 * @return number of bytes encoded, or negative on failure
 */
static int make_stream_request(
    uint8_t *service_request,
    int32_t fileStartPosition,
    const uint8_t *payload,
    size_t payload_len)
{
    BACNET_ATOMIC_WRITE_FILE_DATA req = { 0 };

    req.object_type = OBJECT_FILE;
    req.object_instance = 0;
    req.access = FILE_STREAM_ACCESS;
    req.type.stream.fileStartPosition = fileStartPosition;
    octetstring_init(&req.fileData[0], payload, payload_len);
    return awf_service_encode_apdu(service_request, &req);
}

/**
 * @brief Build a minimal AWF record request into service_request[].
 * @return number of bytes encoded, or negative on failure
 */
static int make_record_request(
    uint8_t *service_request,
    int32_t fileStartRecord,
    const uint8_t *payload,
    size_t payload_len)
{
    BACNET_ATOMIC_WRITE_FILE_DATA req = { 0 };

    req.object_type = OBJECT_FILE;
    req.object_instance = 0;
    req.access = FILE_RECORD_ACCESS;
    req.type.record.fileStartRecord = fileStartRecord;
    req.type.record.returnedRecordCount = 1;
    octetstring_init(&req.fileData[0], payload, payload_len);
    return awf_service_encode_apdu(service_request, &req);
}

/* -------------------------------------------------------------------------
 * Test 1: service_len == 0  -> REJECT (missing required parameter)
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_awf_tests, testHandlerAWF_EmptyRequest)
#else
static void testHandlerAWF_EmptyRequest(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    int pdu_len, apdu_offset;

    make_service_data(&service_data, 1);
    Bacfile_Valid_Instance_Result = false;
    Bacfile_Write_Stream_Data_Result = false;
    pdu_len = handler_atomic_write_file_encode(
        transmit_buffer, NULL, 0, &src, &npdu_data, &service_data);
    zassert_true(pdu_len > 0, "encoding failed: len=%d", pdu_len);
    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_REJECT,
        "Expected PDU_TYPE_REJECT (0x%02x), got 0x%02x",
        (unsigned)PDU_TYPE_REJECT, (unsigned)transmit_buffer[apdu_offset]);
}

/* -------------------------------------------------------------------------
 * Test 2: stream access with fileStartPosition < -1  -> ERROR
 *
 * The staged change added a guard that returns
 * ERROR_CODE_INVALID_FILE_START_POSITION when fileStartPosition < -1.
 * The special value -1 means "append"; anything below -1 is invalid.
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_awf_tests, testHandlerAWF_InvalidStreamStartPosition)
#else
static void testHandlerAWF_InvalidStreamStartPosition(void)
#endif
{
    uint8_t payload[] = { "test data" };
    uint8_t service_request[480] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset, len, service_len;

    service_len =
        make_stream_request(service_request, -2, payload, sizeof(payload));
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 2);
    Bacfile_Valid_Instance_Result = true;
    Bacfile_Write_Stream_Data_Result = false;

    len = handler_atomic_write_file_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);

    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_ERROR,
        "Expected PDU_TYPE_ERROR (0x%02x) for fileStartPosition=-2, got 0x%02x",
        (unsigned)PDU_TYPE_ERROR, (unsigned)transmit_buffer[apdu_offset]);
}

/* -------------------------------------------------------------------------
 * Test 3: valid stream write (fileStartPosition=0)  -> ComplexACK
 *
 * Verify the stream success path still works after the new guard.
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_awf_tests, testHandlerAWF_ValidStreamWrite)
#else
static void testHandlerAWF_ValidStreamWrite(void)
#endif
{
    uint8_t payload[] = { "test data" };
    uint8_t service_request[480] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset, len, service_len;

    service_len =
        make_stream_request(service_request, 0, payload, sizeof(payload));
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 3);
    Bacfile_Valid_Instance_Result = true;
    Bacfile_Write_Stream_Data_Result = true;

    len = handler_atomic_write_file_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);

    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_COMPLEX_ACK,
        "Expected PDU_TYPE_COMPLEX_ACK (0x%02x) for valid stream write, "
        "got 0x%02x",
        (unsigned)PDU_TYPE_COMPLEX_ACK, (unsigned)transmit_buffer[apdu_offset]);
}

/* -------------------------------------------------------------------------
 * Test 4: valid stream append (fileStartPosition=-1)  -> ComplexACK
 *
 * -1 is the special "append to end of file" value and must be accepted.
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_awf_tests, testHandlerAWF_ValidStreamAppend)
#else
static void testHandlerAWF_ValidStreamAppend(void)
#endif
{
    uint8_t payload[] = { "append data" };
    uint8_t service_request[480] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset, len, service_len;

    service_len =
        make_stream_request(service_request, -1, payload, sizeof(payload));
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 4);
    Bacfile_Valid_Instance_Result = true;
    Bacfile_Write_Stream_Data_Result = true;

    len = handler_atomic_write_file_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);

    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_COMPLEX_ACK,
        "Expected PDU_TYPE_COMPLEX_ACK (0x%02x) for stream append "
        "(fileStartPosition=-1), got 0x%02x",
        (unsigned)PDU_TYPE_COMPLEX_ACK, (unsigned)transmit_buffer[apdu_offset]);
}

/* -------------------------------------------------------------------------
 * Test 5: record access with fileStartRecord < -1  -> ERROR
 *
 * The staged change added a guard that returns
 * ERROR_CODE_INVALID_FILE_START_POSITION when fileStartRecord < -1.
 * The special value -1 means "append"; anything below -1 is invalid.
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_awf_tests, testHandlerAWF_InvalidRecordStartPosition)
#else
static void testHandlerAWF_InvalidRecordStartPosition(void)
#endif
{
    uint8_t payload[] = { "record data" };
    uint8_t service_request[480] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset, len, service_len;

    service_len =
        make_record_request(service_request, -2, payload, sizeof(payload));
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 5);
    Bacfile_Valid_Instance_Result = true;
    Bacfile_Write_Record_Data_Result = false;

    len = handler_atomic_write_file_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);

    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_ERROR,
        "Expected PDU_TYPE_ERROR (0x%02x) for fileStartRecord=-2, got 0x%02x",
        (unsigned)PDU_TYPE_ERROR, (unsigned)transmit_buffer[apdu_offset]);
}

/* -------------------------------------------------------------------------
 * Test 6: valid record write (fileStartRecord=0)  -> ComplexACK
 *
 * Verify the record success path still works after the new guard.
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_awf_tests, testHandlerAWF_ValidRecordWrite)
#else
static void testHandlerAWF_ValidRecordWrite(void)
#endif
{
    uint8_t payload[] = { "record data" };
    uint8_t service_request[480] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset, len, service_len;

    service_len =
        make_record_request(service_request, 0, payload, sizeof(payload));
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 6);
    Bacfile_Valid_Instance_Result = true;
    Bacfile_Write_Record_Data_Result = true;

    len = handler_atomic_write_file_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);

    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_COMPLEX_ACK,
        "Expected PDU_TYPE_COMPLEX_ACK (0x%02x) for valid record write, "
        "got 0x%02x",
        (unsigned)PDU_TYPE_COMPLEX_ACK, (unsigned)transmit_buffer[apdu_offset]);
}

/* -------------------------------------------------------------------------
 * Test 7: valid record append (fileStartRecord=-1)  -> ComplexACK
 *
 * -1 is the special "append to end of file" value and must be accepted.
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_awf_tests, testHandlerAWF_ValidRecordAppend)
#else
static void testHandlerAWF_ValidRecordAppend(void)
#endif
{
    uint8_t payload[] = { "append record" };
    uint8_t service_request[480] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset, len, service_len;

    service_len =
        make_record_request(service_request, -1, payload, sizeof(payload));
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 7);
    Bacfile_Valid_Instance_Result = true;
    Bacfile_Write_Record_Data_Result = true;

    len = handler_atomic_write_file_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);

    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_COMPLEX_ACK,
        "Expected PDU_TYPE_COMPLEX_ACK (0x%02x) for record append "
        "(fileStartRecord=-1), got 0x%02x",
        (unsigned)PDU_TYPE_COMPLEX_ACK, (unsigned)transmit_buffer[apdu_offset]);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_awf_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_awf_tests, ztest_unit_test(testHandlerAWF_EmptyRequest),
        ztest_unit_test(testHandlerAWF_InvalidStreamStartPosition),
        ztest_unit_test(testHandlerAWF_ValidStreamWrite),
        ztest_unit_test(testHandlerAWF_ValidStreamAppend),
        ztest_unit_test(testHandlerAWF_InvalidRecordStartPosition),
        ztest_unit_test(testHandlerAWF_ValidRecordWrite),
        ztest_unit_test(testHandlerAWF_ValidRecordAppend));

    ztest_run_test_suite(h_awf_tests);
}
#endif
