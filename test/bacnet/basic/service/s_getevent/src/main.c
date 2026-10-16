/**
 * @file
 * @brief Unit tests for Send_GetEvent and the reply path
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
#include "bacnet/getevent.h"
#include "bacnet/basic/service/h_apdu.h"
#include "bacnet/basic/service/s_getevent.h"
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

static void getevent_ack_handler(
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

/**
 * @brief Send a GetEventInformation request and verify that the reply
 *  from the destination is delivered and releases the Invoke ID, while a
 *  reply from another address is discarded.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(s_getevent_tests, test_Send_GetEvent_Reply)
#else
static void test_Send_GetEvent_Reply(void)
#endif
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS foreign;
    BACNET_ADDRESS stored_dest;
    BACNET_NPDU_DATA npdu_data;
    uint8_t apdu[MAX_APDU];
    uint8_t stored_apdu[MAX_APDU];
    uint16_t stored_len = 0;
    uint8_t invoke_id;
    int len;

    address_set(&dest, 10);
    address_set(&foreign, 99);
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_GET_EVENT_INFORMATION, getevent_ack_handler);

    invoke_id = Send_GetEvent(&dest, NULL);
    zassert_true(invoke_id != 0, NULL);
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    zassert_true(tsm_invoke_id_source_match(invoke_id, &dest), NULL);
    zassert_false(tsm_invoke_id_source_match(invoke_id, &foreign), NULL);
    zassert_true(
        tsm_get_transaction_pdu(
            invoke_id, &stored_dest, &npdu_data, stored_apdu, &stored_len),
        NULL);
    zassert_true(npdu_data.data_expecting_reply, NULL);
    zassert_true(stored_len > 0, NULL);

    len = getevent_ack_encode_apdu_init(apdu, sizeof(apdu), invoke_id);
    zassert_true(len > 0, NULL);
    len += getevent_ack_encode_apdu_end(&apdu[len], sizeof(apdu) - len, false);

    /* a reply from another address is discarded */
    Handler_Calls = 0;
    apdu_handler(&foreign, apdu, (uint16_t)len);
    zassert_equal(Handler_Calls, 0, NULL);
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);

    /* the reply from the destination is handled and frees the Invoke ID */
    apdu_handler(&dest, apdu, (uint16_t)len);
    zassert_equal(Handler_Calls, 1, NULL);
    zassert_true(tsm_invoke_id_free(invoke_id), NULL);

    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_GET_EVENT_INFORMATION, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(s_getevent_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        s_getevent_tests, ztest_unit_test(test_Send_GetEvent_Reply));

    ztest_run_test_suite(s_getevent_tests);
}
#endif
