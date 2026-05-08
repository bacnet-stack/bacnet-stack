/**
 * @file
 * @brief Functional tests for handler_atomic_read_file bounds checks
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 *
 * Tests exercise the two out-of-bounds guards added to
 * handler_atomic_read_file:
 *   1. RecordCount > ARRAY_SIZE(data.fileData)  -> PDU_TYPE_REJECT
 *   2. fileStartRecord >= ARRAY_SIZE(data.fileData) -> PDU_TYPE_ERROR
 */
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/arf.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

/* Stub control variables defined in bacfile_stub.c */
extern bool Bacfile_Valid_Instance_Result;
extern bool Bacfile_Read_Record_Data_Result;

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

/* -------------------------------------------------------------------------
 * Test 1: service_len == 0  -> REJECT (missing required parameter)
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_arf_tests, testHandlerARF_EmptyRequest)
#else
static void testHandlerARF_EmptyRequest(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t test_pdu_type, pdu_type;
    uint8_t transmit_buffer[480] = { 0 };
    int pdu_len, apdu_offset;

    make_service_data(&service_data, 1);
    Bacfile_Valid_Instance_Result = false;
    Bacfile_Read_Record_Data_Result = false;
    pdu_len = handler_atomic_read_file_encode(
        transmit_buffer, NULL, 0, &src, &npdu_data, &service_data);
    zassert_true(pdu_len > 0, "encoding failed: len=%d", pdu_len);
    /* ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER */
    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    pdu_type = PDU_TYPE_REJECT;
    test_pdu_type = transmit_buffer[apdu_offset];
    zassert_equal(
        pdu_type, test_pdu_type,
        "Expected PDU_TYPE_REJECT (0x%02x), got 0x%02x", (unsigned)pdu_type,
        (unsigned)test_pdu_type);
}

/* -------------------------------------------------------------------------
 * Test 2: RecordCount > ARRAY_SIZE(data.fileData)  -> REJECT
 *
 * BACNET_READ_FILE_RECORD_COUNT defaults to 1, so ARRAY_SIZE(data.fileData)
 * is 1.  Requesting RecordCount=2 must trigger the new bounds guard and
 * return a Reject PDU.
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_arf_tests, testHandlerARF_RecordCountOutOfBounds)
#else
static void testHandlerARF_RecordCountOutOfBounds(void)
#endif
{
    BACNET_ATOMIC_READ_FILE_DATA req = { 0 };
    uint8_t service_request[480] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset, len, service_len;

    /* Build a FILE_RECORD_ACCESS request with RecordCount=2 (out of bounds) */
    req.object_type = OBJECT_FILE;
    req.object_instance = 0;
    req.access = FILE_RECORD_ACCESS;
    req.type.record.fileStartRecord = 0;
    req.type.record.RecordCount = 2; /* > ARRAY_SIZE(data.fileData) == 1 */

    service_len = arf_service_encode_apdu(service_request, &req);
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 2);
    Bacfile_Valid_Instance_Result = true;
    Bacfile_Read_Record_Data_Result = false;

    len = handler_atomic_read_file_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);

    /* RecordCount exceeds the buffer size: handler must send a Reject */
    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_REJECT,
        "Expected PDU_TYPE_REJECT (0x%02x) for RecordCount=2, got 0x%02x",
        (unsigned)PDU_TYPE_REJECT, (unsigned)transmit_buffer[apdu_offset]);
}

/* -------------------------------------------------------------------------
 * Test 3: fileStartRecord >= ARRAY_SIZE(data.fileData)  -> ERROR
 *
 * RecordCount=1 is within bounds, but fileStartRecord=1 equals
 * ARRAY_SIZE(data.fileData) and must trigger the second new guard.
 * The error code is ERROR_CODE_INVALID_FILE_START_POSITION which maps to
 * ERROR_CLASS_SERVICES and is encoded as a regular Error PDU.
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_arf_tests, testHandlerARF_FileStartRecordOutOfBounds)
#else
static void testHandlerARF_FileStartRecordOutOfBounds(void)
#endif
{
    BACNET_ATOMIC_READ_FILE_DATA req = { 0 };
    uint8_t service_request[480] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset, len, service_len;

    /* Build a FILE_RECORD_ACCESS request with fileStartRecord=1 (out of
     * bounds) */
    req.object_type = OBJECT_FILE;
    req.object_instance = 0;
    req.access = FILE_RECORD_ACCESS;
    req.type.record.fileStartRecord = 1; /* >= ARRAY_SIZE(data.fileData) == 1 */
    req.type.record.RecordCount = 1; /* within bounds */

    service_len = arf_service_encode_apdu(service_request, &req);
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 3);
    Bacfile_Valid_Instance_Result = true;
    Bacfile_Read_Record_Data_Result = false;

    len = handler_atomic_read_file_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);
    /* fileStartRecord out of bounds: handler must send a regular Error PDU */
    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_ERROR,
        "Expected PDU_TYPE_ERROR (0x%02x) for fileStartRecord=1, got 0x%02x",
        (unsigned)PDU_TYPE_ERROR, (unsigned)transmit_buffer[apdu_offset]);
}

/* -------------------------------------------------------------------------
 * Test 4: Valid record request with RecordCount=1, fileStartRecord=0  -> ACK
 *
 * Verify the normal success path still works after the bounds checks.
 * ------------------------------------------------------------------------- */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_arf_tests, testHandlerARF_ValidRecordRequest)
#else
static void testHandlerARF_ValidRecordRequest(void)
#endif
{
    BACNET_ATOMIC_READ_FILE_DATA req = { 0 };
    uint8_t service_request[480] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset, len, service_len;

    /* Build a FILE_RECORD_ACCESS request within bounds */
    req.object_type = OBJECT_FILE;
    req.object_instance = 0;
    req.access = FILE_RECORD_ACCESS;
    req.type.record.fileStartRecord = 0;
    req.type.record.RecordCount = 1;

    service_len = arf_service_encode_apdu(service_request, &req);
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 4);
    /* Both the instance check and the read succeed */
    Bacfile_Valid_Instance_Result = true;
    Bacfile_Read_Record_Data_Result = true;

    len = handler_atomic_read_file_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);
    /* Successful read: handler must send a ComplexACK */
    apdu_offset = npdu_decode(transmit_buffer, NULL, NULL, &npdu_data);
    zassert_equal(
        transmit_buffer[apdu_offset], (uint8_t)PDU_TYPE_COMPLEX_ACK,
        "Expected PDU_TYPE_COMPLEX_ACK (0x%02x), got 0x%02x",
        (unsigned)PDU_TYPE_COMPLEX_ACK, (unsigned)transmit_buffer[apdu_offset]);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_arf_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_arf_tests, ztest_unit_test(testHandlerARF_EmptyRequest),
        ztest_unit_test(testHandlerARF_RecordCountOutOfBounds),
        ztest_unit_test(testHandlerARF_FileStartRecordOutOfBounds),
        ztest_unit_test(testHandlerARF_ValidRecordRequest));

    ztest_run_test_suite(h_arf_tests);
}
#endif
