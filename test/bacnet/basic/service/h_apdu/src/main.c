/**
 * @file
 * @brief Unit tests for client response source matching
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
#include "bacnet/bacerror.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/service/h_apdu.h"
#include "bacnet/basic/tsm/tsm.h"

static unsigned Handler_Calls;

static void address_set(BACNET_ADDRESS *addr, uint8_t host)
{
    memset(addr, 0, sizeof(*addr));
    addr->mac_len = 6;
    addr->mac[0] = 192;
    addr->mac[1] = 168;
    addr->mac[2] = 1;
    addr->mac[3] = host;
    addr->mac[4] = 0xBA;
    addr->mac[5] = 0xC0;
}

static uint8_t pending_invoke_id(const BACNET_ADDRESS *dest)
{
    BACNET_NPDU_DATA npdu_data;
    uint8_t apdu[2];
    uint8_t invoke_id;

    invoke_id = tsm_next_free_invokeID();
    zassert_true(invoke_id != 0, NULL);
    memset(&npdu_data, 0, sizeof(npdu_data));
    npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
    apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
    apdu[1] = invoke_id;
    tsm_set_confirmed_unsegmented_transaction(
        invoke_id, dest, &npdu_data, apdu, (uint16_t)sizeof(apdu));
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);

    return invoke_id;
}

static void deliver_foreign_then_dest(
    BACNET_ADDRESS *dest,
    BACNET_ADDRESS *foreign,
    uint8_t *apdu,
    uint16_t apdu_len,
    uint8_t invoke_id)
{
    Handler_Calls = 0;
    apdu[1] = invoke_id;
    apdu_handler(foreign, apdu, apdu_len);
    zassert_equal(Handler_Calls, 0, NULL);
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);

    apdu_handler(dest, apdu, apdu_len);
    zassert_equal(Handler_Calls, 1, NULL);
    zassert_true(tsm_invoke_id_free(invoke_id), NULL);
}

static void simple_ack_handler(BACNET_ADDRESS *src, uint8_t invoke_id)
{
    (void)src;
    (void)invoke_id;
    Handler_Calls++;
}

static void complex_ack_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    (void)service_request;
    (void)service_len;
    (void)src;
    (void)service_data;
    Handler_Calls++;
}

static void error_handler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    (void)src;
    (void)invoke_id;
    (void)error_class;
    (void)error_code;
    Handler_Calls++;
}

static void
reject_handler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    (void)src;
    (void)invoke_id;
    (void)reject_reason;
    Handler_Calls++;
}

static void abort_handler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)src;
    (void)invoke_id;
    (void)abort_reason;
    (void)server;
    Handler_Calls++;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_apdu_tests, testInvokeIdSourceMatch)
#else
static void testInvokeIdSourceMatch(void)
#endif
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS other_mac;
    BACNET_ADDRESS other_net;
    uint8_t invoke_id;
    uint8_t unknown_id;

    address_set(&dest, 10);
    address_set(&other_mac, 11);
    address_set(&other_net, 10);
    other_net.net = 1;
    other_net.len = 1;
    other_net.adr[0] = 10;

    zassert_false(tsm_invoke_id_source_match(0, &dest), NULL);
    zassert_false(tsm_invoke_id_source_match(1, NULL), NULL);

    invoke_id = pending_invoke_id(&dest);
    unknown_id = (uint8_t)(invoke_id + 1U);
    if (unknown_id == 0) {
        unknown_id = 1;
    }
    zassert_false(tsm_invoke_id_source_match(invoke_id, NULL), NULL);
    zassert_false(tsm_invoke_id_source_match(invoke_id, &other_mac), NULL);
    zassert_false(tsm_invoke_id_source_match(invoke_id, &other_net), NULL);
    zassert_false(tsm_invoke_id_source_match(unknown_id, &dest), NULL);
    zassert_true(tsm_invoke_id_source_match(invoke_id, &dest), NULL);

    tsm_free_invoke_id(invoke_id);
    zassert_false(tsm_invoke_id_source_match(invoke_id, &dest), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_apdu_tests, testInvokeIdSourceMatchRouted)
#else
static void testInvokeIdSourceMatchRouted(void)
#endif
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS other_router;
    BACNET_ADDRESS other_remote;
    BACNET_ADDRESS other_net;
    uint8_t invoke_id;

    /* remote device 5 on network 2000, reached via router at host 10 */
    address_set(&dest, 10);
    dest.net = 2000;
    dest.len = 1;
    dest.adr[0] = 5;
    /* same remote device, response came back through another router */
    other_router = dest;
    other_router.mac[3] = 11;
    /* different remote device behind the same router */
    other_remote = dest;
    other_remote.adr[0] = 6;
    /* same remote address on a different network */
    other_net = dest;
    other_net.net = 2001;

    invoke_id = pending_invoke_id(&dest);
    zassert_true(tsm_invoke_id_source_match(invoke_id, &dest), NULL);
    zassert_true(tsm_invoke_id_source_match(invoke_id, &other_router), NULL);
    zassert_false(tsm_invoke_id_source_match(invoke_id, &other_remote), NULL);
    zassert_false(tsm_invoke_id_source_match(invoke_id, &other_net), NULL);

    tsm_free_invoke_id(invoke_id);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_apdu_tests, testSimpleAckSourceMatch)
#else
static void testSimpleAckSourceMatch(void)
#endif
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS foreign;
    uint8_t invoke_id;
    uint8_t apdu[3];

    address_set(&dest, 10);
    address_set(&foreign, 99);
    invoke_id = pending_invoke_id(&dest);
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, simple_ack_handler);
    apdu[0] = PDU_TYPE_SIMPLE_ACK;
    apdu[2] = SERVICE_CONFIRMED_WRITE_PROPERTY;
    deliver_foreign_then_dest(
        &dest, &foreign, apdu, (uint16_t)sizeof(apdu), invoke_id);
    apdu_handler(&dest, apdu, (uint16_t)sizeof(apdu));
    zassert_equal(Handler_Calls, 1, NULL);
    zassert_true(tsm_invoke_id_free(invoke_id), NULL);
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_apdu_tests, testComplexAckSourceMatch)
#else
static void testComplexAckSourceMatch(void)
#endif
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS foreign;
    uint8_t invoke_id;
    uint8_t apdu[3];

    address_set(&dest, 10);
    address_set(&foreign, 99);
    invoke_id = pending_invoke_id(&dest);
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, complex_ack_handler);
    apdu[0] = PDU_TYPE_COMPLEX_ACK;
    apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
    deliver_foreign_then_dest(
        &dest, &foreign, apdu, (uint16_t)sizeof(apdu), invoke_id);
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_apdu_tests, testErrorSourceMatch)
#else
static void testErrorSourceMatch(void)
#endif
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS foreign;
    uint8_t invoke_id;
    uint8_t apdu[16];
    int len;

    address_set(&dest, 10);
    address_set(&foreign, 99);
    invoke_id = pending_invoke_id(&dest);
    apdu_set_error_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, error_handler);
    len = bacerror_encode_apdu(
        apdu, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY, ERROR_CLASS_OBJECT,
        ERROR_CODE_UNKNOWN_OBJECT);
    zassert_true(len > 3, NULL);
    deliver_foreign_then_dest(&dest, &foreign, apdu, (uint16_t)len, invoke_id);
    apdu_set_error_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_apdu_tests, testRejectSourceMatch)
#else
static void testRejectSourceMatch(void)
#endif
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS foreign;
    uint8_t invoke_id;
    uint8_t apdu[3];

    address_set(&dest, 10);
    address_set(&foreign, 99);
    invoke_id = pending_invoke_id(&dest);
    apdu_set_reject_handler(reject_handler);
    apdu[0] = PDU_TYPE_REJECT;
    apdu[2] = REJECT_REASON_OTHER;
    deliver_foreign_then_dest(
        &dest, &foreign, apdu, (uint16_t)sizeof(apdu), invoke_id);
    apdu_set_reject_handler(NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_apdu_tests, testAbortSourceMatch)
#else
static void testAbortSourceMatch(void)
#endif
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS foreign;
    uint8_t invoke_id;
    uint8_t apdu[3];

    address_set(&dest, 10);
    address_set(&foreign, 99);
    invoke_id = pending_invoke_id(&dest);
    apdu_set_abort_handler(abort_handler);
    apdu[0] = PDU_TYPE_ABORT;
    apdu[2] = ABORT_REASON_OTHER;
    deliver_foreign_then_dest(
        &dest, &foreign, apdu, (uint16_t)sizeof(apdu), invoke_id);
    apdu_set_abort_handler(NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_apdu_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_apdu_tests, ztest_unit_test(testInvokeIdSourceMatch),
        ztest_unit_test(testInvokeIdSourceMatchRouted),
        ztest_unit_test(testSimpleAckSourceMatch),
        ztest_unit_test(testComplexAckSourceMatch),
        ztest_unit_test(testErrorSourceMatch),
        ztest_unit_test(testRejectSourceMatch),
        ztest_unit_test(testAbortSourceMatch));

    ztest_run_test_suite(h_apdu_tests);
}
#endif
