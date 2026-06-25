/**
 * @file
 * @brief Unit tests for handler_read_range service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 *
 * Tests cover the main code paths in handler_read_range:
 * 1. Empty request (service_len == 0) -> PDU_TYPE_REJECT
 * 2. Segmented message -> PDU_TYPE_ABORT
 * 3. Bad encoding (rr_decode_service_request fails) -> PDU_TYPE_ABORT
 * 4. Valid request with valid handler -> PDU_TYPE_COMPLEX_ACK
 * 5. Valid request without handler -> PDU_TYPE_ERROR
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
#include "bacnet/readrange.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

/* Stub control variables defined in device_stub.c */
extern bool Device_RR_Info_Should_Return_Handler;
extern int Stub_RR_Handler(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);
extern bool
Stub_RR_Info_Function(BACNET_READ_RANGE_DATA *pRequest, RR_PROP_INFO *pInfo);

/**
 * @brief Build a minimal BACNET_CONFIRMED_SERVICE_DATA for use as the
 *        service_data argument.
 */
static void make_service_data(
    BACNET_CONFIRMED_SERVICE_DATA *sd, uint8_t invoke_id, bool segmented)
{
    memset(sd, 0, sizeof(BACNET_CONFIRMED_SERVICE_DATA));
    sd->invoke_id = invoke_id;
    sd->priority = MESSAGE_PRIORITY_NORMAL;
    sd->segmented_message = segmented;
}

/**
 * Helper to extract the PDU type from the transmit buffer
 */
static uint8_t extract_pdu_type(const uint8_t *buffer)
{
    BACNET_NPDU_DATA npdu_data;
    int apdu_offset;

    apdu_offset = npdu_decode(buffer, NULL, NULL, &npdu_data);
    if (apdu_offset > 0) {
        /* APDU control byte: high nibble is PDU type, low nibble has flags */
        return buffer[apdu_offset] & 0xF0;
    }
    return 0;
}

/**
 * Helper to build a valid ReadRange service request
 */
static int build_rr_request(
    uint8_t *service_request,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property_id)
{
    BACNET_READ_RANGE_DATA rr_data;
    int service_len;

    memset(&rr_data, 0, sizeof(rr_data));
    rr_data.object_type = object_type;
    rr_data.object_instance = object_instance;
    rr_data.object_property = property_id;
    rr_data.array_index = BACNET_ARRAY_ALL;
    rr_data.RequestType = RR_READ_ALL;

    service_len = read_range_encode(service_request, &rr_data);
    return service_len;
}

/**
 * Helper to build a by-position request with a missing referenceIndex.
 *
 * Starts with a valid encoded request, then removes the first application
 * element inside the byPosition [3] sequence (referenceIndex). The next field
 * (count) remains, so unsigned decode sees a tag mismatch (len == 0).
 */
static int build_rr_request_missing_refindex(uint8_t *service_request)
{
    BACNET_READ_RANGE_DATA rr_data;
    BACNET_OBJECT_TYPE object_type = OBJECT_ANALOG_INPUT;
    uint32_t object_instance = 0;
    BACNET_PROPERTY_ID property_id = PROP_ACKED_TRANSITIONS;
    BACNET_UNSIGNED_INTEGER unsigned_context_value = BACNET_ARRAY_ALL;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    uint32_t enum_value = 0;
    int service_len;
    int apdu_len = 0;
    int len;

    memset(&rr_data, 0, sizeof(rr_data));
    rr_data.object_type = object_type;
    rr_data.object_instance = object_instance;
    rr_data.object_property = property_id;
    rr_data.array_index = BACNET_ARRAY_ALL;
    rr_data.RequestType = RR_BY_POSITION;
    rr_data.Range.RefIndex = 1;
    rr_data.Count = 10;

    service_len = read_range_encode(service_request, &rr_data);
    if (service_len <= 0) {
        return service_len;
    }

    len = bacnet_object_id_context_decode(
        &service_request[apdu_len], service_len - apdu_len, 0, &object_type,
        &object_instance);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    len = bacnet_enumerated_context_decode(
        &service_request[apdu_len], service_len - apdu_len, 1, &enum_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    len = bacnet_unsigned_context_decode(
        &service_request[apdu_len], service_len - apdu_len, 2,
        &unsigned_context_value);
    if (len > 0) {
        apdu_len += len;
    }

    if (!bacnet_is_opening_tag_number(
            &service_request[apdu_len], service_len - apdu_len, 3, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    len = bacnet_unsigned_application_decode(
        &service_request[apdu_len], service_len - apdu_len, &unsigned_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }

    memmove(
        &service_request[apdu_len], &service_request[apdu_len + len],
        (size_t)(service_len - (apdu_len + len)));
    service_len -= len;

    return service_len;
}

/* -------------------------------------------------------------------------
 * Test 1: Empty request (service_len == 0) -> REJECT
 * (Missing Required Parameter)
 * --------- -------------------------------------------------------*/
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rr_tests, testHandlerRR_EmptyRequest)
#else
static void testHandlerRR_EmptyRequest(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    uint8_t pdu_type;

    make_service_data(&service_data, 1, false);
    Device_RR_Info_Should_Return_Handler = false;

    /* Call handler with empty service request */
    handler_read_range(NULL, 0, &src, &service_data);

    /* Verify REJECT response */
    pdu_type = extract_pdu_type(Handler_Transmit_Buffer);
    zassert_equal(
        pdu_type, (uint8_t)PDU_TYPE_REJECT,
        "Expected PDU_TYPE_REJECT (0x%02x) for empty request, got 0x%02x",
        (unsigned)PDU_TYPE_REJECT, (unsigned)pdu_type);
}

/* -------------------------------------------------------------------------
 * Test 2: Segmented message -> ABORT
 * (Segmentation Not Supported)
 * --------- -------------------------------------------------------*/
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rr_tests, testHandlerRR_SegmentedMessage)
#else
static void testHandlerRR_SegmentedMessage(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    uint8_t service_request[32] = { 0 };
    uint8_t pdu_type;

    make_service_data(&service_data, 2, true); /* segmented_message = true */
    Device_RR_Info_Should_Return_Handler = false;

    /* Call handler with segmented message flag */
    handler_read_range(
        service_request, sizeof(service_request), &src, &service_data);

    /* Verify ABORT response */
    pdu_type = extract_pdu_type(Handler_Transmit_Buffer);
    zassert_equal(
        pdu_type, (uint8_t)PDU_TYPE_ABORT,
        "Expected PDU_TYPE_ABORT (0x%02x) for segmented message, got 0x%02x",
        (unsigned)PDU_TYPE_ABORT, (unsigned)pdu_type);
}

/* -------------------------------------------------------------------------
 * Test 3: Bad encoding (rr_decode_service_request fails) -> ABORT
 *
 * When the APDU decoding fails, we should get an ABORT response
 * --------- -------------------------------------------------------*/
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rr_tests, testHandlerRR_BadEncoding)
#else
static void testHandlerRR_BadEncoding(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    uint8_t service_request[32] = { 0xFF };
    uint8_t pdu_type;

    make_service_data(&service_data, 3, false);
    Device_RR_Info_Should_Return_Handler = false;
    /* Create a malformed request with invalid APDU data */
    /* This will fail to decode in rr_decode_service_request */

    /* Call handler with invalid request */
    handler_read_range(service_request, 1, &src, &service_data);

    /* Verify ABORT response for bad encoding */
    pdu_type = extract_pdu_type(Handler_Transmit_Buffer);
    zassert_equal(
        pdu_type, (uint8_t)PDU_TYPE_ABORT,
        "Expected PDU_TYPE_ABORT (0x%02x) for bad encoding, got 0x%02x",
        (unsigned)PDU_TYPE_ABORT, (unsigned)pdu_type);
}

/* -------------------------------------------------------------------------
 * Test 4: Valid request, no RR handler for object type -> ERROR
 *
 * When Device_Objects_RR_Info returns NULL, we should get an ERROR response
 * with error_code = ERROR_CODE_PROPERTY_IS_NOT_A_LIST
 * --------- -------------------------------------------------------*/
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rr_tests, testHandlerRR_NoHandlerForObjectType)
#else
static void testHandlerRR_NoHandlerForObjectType(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    uint8_t service_request[64] = { 0 };
    int service_len;
    uint8_t pdu_type;

    make_service_data(&service_data, 4, false);

    /* Create a valid ReadRange request */
    service_len = build_rr_request(
        service_request, OBJECT_ANALOG_INPUT, 0, PROP_ACKED_TRANSITIONS);
    zassert_true(service_len > 0, "Failed to build valid RR request");

    /* Set handler to NULL to simulate unsupported object type */
    Device_RR_Info_Should_Return_Handler = false;

    /* Call handler with valid request but no handler */
    handler_read_range(
        service_request, (uint16_t)service_len, &src, &service_data);

    /* Verify ERROR response */
    pdu_type = extract_pdu_type(Handler_Transmit_Buffer);
    zassert_equal(
        pdu_type, (uint8_t)PDU_TYPE_ERROR,
        "Expected PDU_TYPE_ERROR (0x%02x) for unsupported object type, got "
        "0x%02x",
        (unsigned)PDU_TYPE_ERROR, (unsigned)pdu_type);
}

/* -------------------------------------------------------------------------
 * Test 5: Valid request with valid handler -> COMPLEX_ACK
 *
 * When a valid handler is available and returns valid payload,
 * we should get a COMPLEX_ACK response
 * --------- -------------------------------------------------------*/
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rr_tests, testHandlerRR_ValidRequestWithHandler)
#else
static void testHandlerRR_ValidRequestWithHandler(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    uint8_t service_request[64] = { 0 };
    int service_len;
    uint8_t pdu_type;

    make_service_data(&service_data, 5, false);

    /* Create a valid ReadRange request */
    service_len = build_rr_request(
        service_request, OBJECT_ANALOG_INPUT, 0, PROP_ACKED_TRANSITIONS);
    zassert_true(service_len > 0, "Failed to build valid RR request");

    /* Set handler to valid stub handler */
    Device_RR_Info_Should_Return_Handler = true;

    /* Call handler with valid request and handler */
    handler_read_range(
        service_request, (uint16_t)service_len, &src, &service_data);

    /* Verify COMPLEX_ACK response */
    pdu_type = extract_pdu_type(Handler_Transmit_Buffer);
    zassert_equal(
        pdu_type, (uint8_t)PDU_TYPE_COMPLEX_ACK,
        "Expected PDU_TYPE_COMPLEX_ACK (0x%02x) for valid request, got 0x%02x",
        (unsigned)PDU_TYPE_COMPLEX_ACK, (unsigned)pdu_type);
}

/* -------------------------------------------------------------------------
 * Test 6: Valid request with RR_BY_POSITION range type -> COMPLEX_ACK
 *
 * Test a specific range request (by position) to ensure the handler
 * properly processes different request types
 * --------- -------------------------------------------------------*/
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rr_tests, testHandlerRR_RangeByPosition)
#else
static void testHandlerRR_RangeByPosition(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    uint8_t service_request[64] = { 0 };
    int service_len;
    uint8_t pdu_type;
    BACNET_READ_RANGE_DATA rr_data;

    make_service_data(&service_data, 6, false);

    /* Create a ReadRange request with RR_BY_POSITION */
    memset(&rr_data, 0, sizeof(rr_data));
    rr_data.object_type = OBJECT_ANALOG_INPUT;
    rr_data.object_instance = 1;
    rr_data.object_property = PROP_ACKED_TRANSITIONS;
    rr_data.array_index = BACNET_ARRAY_ALL;
    rr_data.RequestType = RR_BY_POSITION;
    rr_data.Range.RefIndex = 0;
    rr_data.Count = 10;

    service_len = read_range_encode(service_request, &rr_data);
    zassert_true(service_len > 0, "Failed to build RR by position request");

    /* Set handler to valid stub handler */
    Device_RR_Info_Should_Return_Handler = true;

    /* Call handler */
    handler_read_range(
        service_request, (uint16_t)service_len, &src, &service_data);

    /* Verify COMPLEX_ACK response */
    pdu_type = extract_pdu_type(Handler_Transmit_Buffer);
    zassert_equal(
        pdu_type, (uint8_t)PDU_TYPE_COMPLEX_ACK,
        "Expected PDU_TYPE_COMPLEX_ACK (0x%02x) for range by position, got "
        "0x%02x",
        (unsigned)PDU_TYPE_COMPLEX_ACK, (unsigned)pdu_type);
}

/* -------------------------------------------------------------------------
 * Test 7: Missing required range parameter -> REJECT
 *
 * Request decodes with byPosition [3] opening tag but missing referenceIndex.
 * When decoder tries to read the first required element (RefIndex), it gets
 * len=0 (tag mismatch) and returns BACNET_STATUS_REJECT.
 * Handler responds with REJECT for missing required parameter.
 * --------- -------------------------------------------------------*/
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rr_tests, testHandlerRR_MissingRangeParameter)
#else
static void testHandlerRR_MissingRangeParameter(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    uint8_t service_request[64] = { 0 };
    int service_len;
    uint8_t pdu_type;

    make_service_data(&service_data, 7, false);
    service_len = build_rr_request_missing_refindex(service_request);
    zassert_true(
        service_len > 0,
        "Failed to build RR by-position request with missing referenceIndex");

    Device_RR_Info_Should_Return_Handler = true;
    handler_read_range(
        service_request, (uint16_t)service_len, &src, &service_data);

    /* Verify REJECT response for missing range parameter */
    pdu_type = extract_pdu_type(Handler_Transmit_Buffer);
    zassert_equal(
        pdu_type, (uint8_t)PDU_TYPE_REJECT,
        "Expected PDU_TYPE_REJECT (0x%02x) for missing range parameter, got "
        "0x%02x",
        (unsigned)PDU_TYPE_REJECT, (unsigned)pdu_type);
}

/* --------- -------------------------------------------------------
 * Test Suite Definition
 * --------- -------------------------------------------------------*/
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_rr_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_rr_tests, ztest_unit_test(testHandlerRR_EmptyRequest),
        ztest_unit_test(testHandlerRR_SegmentedMessage),
        ztest_unit_test(testHandlerRR_BadEncoding),
        ztest_unit_test(testHandlerRR_NoHandlerForObjectType),
        ztest_unit_test(testHandlerRR_ValidRequestWithHandler),
        ztest_unit_test(testHandlerRR_RangeByPosition),
        ztest_unit_test(testHandlerRR_MissingRangeParameter));

    ztest_run_test_suite(h_rr_tests);
}
#endif
