/**
 * @file
 * @brief Unit tests for handler_confirmed_private_transfer_encode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
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
#include "bacnet/ptransfer.h"
#include "bacnet/basic/service/h_pt.h"
#include "bacnet/basic/services.h"

static BACNET_ERROR_CODE Stub_Private_Transfer_Result = ERROR_CODE_SUCCESS;

static BACNET_ERROR_CODE
stub_private_transfer_handler(BACNET_PRIVATE_TRANSFER_DATA *data, void *context)
{
    (void)context;
    if (data) {
        return Stub_Private_Transfer_Result;
    }
    return ERROR_CODE_OTHER;
}

static void make_service_data(
    BACNET_CONFIRMED_SERVICE_DATA *sd, uint8_t invoke_id, bool segmented)
{
    memset(sd, 0, sizeof(BACNET_CONFIRMED_SERVICE_DATA));
    sd->invoke_id = invoke_id;
    sd->priority = MESSAGE_PRIORITY_NORMAL;
    sd->segmented_message = segmented;
}

static uint8_t extract_pdu_type(const uint8_t *buffer)
{
    BACNET_NPDU_DATA npdu_data;
    int apdu_offset;

    apdu_offset = npdu_decode(buffer, NULL, NULL, &npdu_data);
    if (apdu_offset > 0) {
        return buffer[apdu_offset] & 0xF0;
    }
    return 0;
}

static int build_private_transfer_request(uint8_t *service_request)
{
    BACNET_PRIVATE_TRANSFER_DATA req = { 0 };
    uint8_t service_parameters[16] = { 0 };
    int service_parameters_len = 0;
    int service_len = 0;

    req.vendorID = 999;
    req.serviceNumber = 42;
    service_parameters_len =
        encode_application_real(service_parameters, 3.14159f);
    req.serviceParameters = service_parameters;
    req.serviceParametersLen = service_parameters_len;
    service_len =
        private_transfer_request_service_encode(service_request, 128, &req);
    return service_len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_pt_tests, testHandlerPT_EmptyRequest)
#else
static void testHandlerPT_EmptyRequest(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    int len;

    make_service_data(&service_data, 1, false);
    handler_confirmed_private_transfer_callback_set(NULL);

    len = handler_confirmed_private_transfer_encode(
        transmit_buffer, NULL, 0, &src, &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);
    zassert_equal(
        extract_pdu_type(transmit_buffer), (uint8_t)PDU_TYPE_REJECT,
        "Expected PDU_TYPE_REJECT (0x%02x), got 0x%02x",
        (unsigned)PDU_TYPE_REJECT, (unsigned)extract_pdu_type(transmit_buffer));
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_pt_tests, testHandlerPT_SegmentedMessage)
#else
static void testHandlerPT_SegmentedMessage(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t service_request[128] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    int service_len;
    int len;

    service_len = build_private_transfer_request(service_request);
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 2, true);
    handler_confirmed_private_transfer_callback_set(NULL);

    len = handler_confirmed_private_transfer_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);
    zassert_equal(
        extract_pdu_type(transmit_buffer), (uint8_t)PDU_TYPE_ABORT,
        "Expected PDU_TYPE_ABORT (0x%02x), got 0x%02x",
        (unsigned)PDU_TYPE_ABORT, (unsigned)extract_pdu_type(transmit_buffer));
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_pt_tests, testHandlerPT_ValidRequestSucceeds)
#else
static void testHandlerPT_ValidRequestSucceeds(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t service_request[128] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    int service_len;
    int len;

    service_len = build_private_transfer_request(service_request);
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 3, false);
    Stub_Private_Transfer_Result = ERROR_CODE_SUCCESS;
    handler_confirmed_private_transfer_callback_set(
        stub_private_transfer_handler);

    len = handler_confirmed_private_transfer_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);
    zassert_equal(
        extract_pdu_type(transmit_buffer), (uint8_t)PDU_TYPE_COMPLEX_ACK,
        "Expected PDU_TYPE_COMPLEX_ACK (0x%02x), got 0x%02x",
        (unsigned)PDU_TYPE_COMPLEX_ACK,
        (unsigned)extract_pdu_type(transmit_buffer));

    handler_confirmed_private_transfer_callback_set(NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_pt_tests, testHandlerPT_CallbackError)
#else
static void testHandlerPT_CallbackError(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data;
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t service_request[128] = { 0 };
    uint8_t transmit_buffer[480] = { 0 };
    int service_len;
    int len;

    service_len = build_private_transfer_request(service_request);
    zassert_true(service_len > 0, "encoding failed: len=%d", service_len);

    make_service_data(&service_data, 4, false);
    Stub_Private_Transfer_Result = ERROR_CODE_OTHER;
    handler_confirmed_private_transfer_callback_set(
        stub_private_transfer_handler);

    len = handler_confirmed_private_transfer_encode(
        transmit_buffer, service_request, (uint16_t)service_len, &src,
        &npdu_data, &service_data);
    zassert_true(len > 0, "encoding failed: len=%d", len);
    zassert_equal(
        extract_pdu_type(transmit_buffer), (uint8_t)PDU_TYPE_ERROR,
        "Expected PDU_TYPE_ERROR (0x%02x), got 0x%02x",
        (unsigned)PDU_TYPE_ERROR, (unsigned)extract_pdu_type(transmit_buffer));

    handler_confirmed_private_transfer_callback_set(NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_pt_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_pt_tests, ztest_unit_test(testHandlerPT_EmptyRequest),
        ztest_unit_test(testHandlerPT_SegmentedMessage),
        ztest_unit_test(testHandlerPT_ValidRequestSucceeds),
        ztest_unit_test(testHandlerPT_CallbackError));

    ztest_run_test_suite(h_pt_tests);
}
#endif
