/**
 * @file
 * @brief Unit test for BACnetSpecialEvent. This test also indirectly tests
 *  BACnetCalendarEntry
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @date Aug 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/ztest.h>
#include "bacnet/bacdcode.h"
#include "bacnet/hostnport.h"
#include "bacnet/secure_connect.h"
#include "bacnet/datetime.h"

/**
 * @addtogroup bacnet_tests
 * @{
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnet_Secure_Connect_Tests, test_BACnet_Secure_Connect)
#else
static void test_BACnet_Secure_Connect(void)
#endif
{
    int apdu_len = 0, test_len = 0, null_len = 0, diff = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_SC_HUB_CONNECTION_STATUS data = { 0 }, test_data;

    data.State = BACNET_SC_CONNECTION_STATE_CONNECTED;
    datetime_init_ascii(&data.Connect_Timestamp, "2023/08/01-12:00:00");
    datetime_init_ascii(&data.Disconnect_Timestamp, "2023/08/02-12:00:00");
    data.Error = ERROR_CODE_DEFAULT;
    data.Error_Details[0] = 0;

    null_len = bacapp_encode_SCHubConnection(NULL, &data);
    apdu_len = bacapp_encode_SCHubConnection(apdu, &data);
    zassert_true(null_len == apdu_len, NULL);

    test_len = bacapp_decode_SCHubConnection(apdu, apdu_len, &test_data);
    zassert_true(test_len == apdu_len, NULL);
    zassert_true(test_data.State == data.State, NULL);
    diff =
        datetime_compare(&test_data.Connect_Timestamp, &data.Connect_Timestamp);
    zassert_equal(diff, 0, NULL);
    diff = datetime_compare(
        &test_data.Disconnect_Timestamp, &data.Disconnect_Timestamp);
    zassert_equal(diff, 0, NULL);
    zassert_true(test_data.Error == data.Error, NULL);
    diff = strcmp(test_data.Error_Details, data.Error_Details);
    zassert_equal(diff, 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnet_Secure_Connect_Tests, test_BACnet_Secure_Connect_Truncated_Decode)
#else
static void test_BACnet_Secure_Connect_Truncated_Decode(void)
#endif
{
    int apdu_len = 0;
    int test_len = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_SC_FAILED_CONNECTION_REQUEST failed_req = { 0 };
    BACNET_SC_FAILED_CONNECTION_REQUEST failed_req_out = { 0 };
    BACNET_SC_DIRECT_CONNECTION_STATUS direct_conn = { 0 };
    BACNET_SC_DIRECT_CONNECTION_STATUS direct_conn_out = { 0 };
    BACNET_DATE_TIME datetime = { 0 };
    BACNET_CHARACTER_STRING str = { 0 };
    BACNET_HOST_N_PORT hp = { 0 };
    uint32_t ui32 = 0;
    int peer_start = 0;
    int peer_len = 0;
    int trunc_len = 0;

    datetime_init_ascii(&failed_req.Timestamp, "2023/08/01-12:00:00");
    failed_req.Peer_Address.type = BACNET_HOST_N_PORT_IP;
    memcpy(failed_req.Peer_Address.host, "10.1.2.3", 9);
    failed_req.Peer_Address.port = 0xBAC0;
    memcpy(
        failed_req.Peer_VMAC,
        (uint8_t[]) { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 },
        sizeof(failed_req.Peer_VMAC));
    memcpy(
        failed_req.Peer_UUID.uuid.uuid128,
        (uint8_t[]) { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                      0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF },
        sizeof(failed_req.Peer_UUID.uuid.uuid128));
    failed_req.Error = ERROR_CODE_ABORT_BUFFER_OVERFLOW;
    memcpy(failed_req.Error_Details, "overflow", 9);

    apdu_len = bacapp_encode_SCFailedConnectionRequest(apdu, &failed_req);
    zassert_true(apdu_len > 0, NULL);
    test_len = bacapp_decode_SCFailedConnectionRequest(
        apdu, apdu_len, &failed_req_out);
    zassert_equal(test_len, apdu_len, NULL);
    peer_start =
        bacnet_datetime_context_decode(apdu, (size_t)apdu_len, 0, &datetime);
    zassert_true(peer_start > 0, NULL);
    peer_len = host_n_port_context_decode(
        &apdu[peer_start], (size_t)(apdu_len - peer_start), 1, NULL, &hp);
    zassert_true(peer_len > 0, NULL);
    for (trunc_len = peer_start + 1; trunc_len < (peer_start + peer_len);
         trunc_len++) {
        test_len = bacapp_decode_SCFailedConnectionRequest(
            apdu, (size_t)trunc_len, &failed_req_out);
        zassert_true(
            test_len < 0, "SCFailedConnectionRequest peer trunc len=%d",
            trunc_len);
    }

    datetime_init_ascii(&direct_conn.Connect_Timestamp, "2023/08/01-12:00:00");
    datetime_init_ascii(
        &direct_conn.Disconnect_Timestamp, "2023/08/02-12:00:00");
    memcpy(direct_conn.URI, "wss://bacnet.example/ws", 24);
    direct_conn.State = BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT;
    direct_conn.Peer_Address.type = BACNET_HOST_N_PORT_IP;
    memcpy(direct_conn.Peer_Address.host, "192.168.1.10", 13);
    direct_conn.Peer_Address.port = 0xBAC0;
    memcpy(
        direct_conn.Peer_VMAC,
        (uint8_t[]) { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 },
        sizeof(direct_conn.Peer_VMAC));
    memcpy(
        direct_conn.Peer_UUID.uuid.uuid128,
        (uint8_t[]) { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77,
                      0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 },
        sizeof(direct_conn.Peer_UUID.uuid.uuid128));
    direct_conn.Error = ERROR_CODE_ABORT_BUFFER_OVERFLOW;
    memcpy(direct_conn.Error_Details, "connect-failed", 15);

    apdu_len = bacapp_encode_SCDirectConnection(apdu, &direct_conn);
    zassert_true(apdu_len > 0, NULL);
    test_len =
        bacapp_decode_SCDirectConnection(apdu, apdu_len, &direct_conn_out);
    zassert_equal(test_len, apdu_len, NULL);
    peer_start =
        bacnet_character_string_context_decode(apdu, (size_t)apdu_len, 0, &str);
    zassert_true(peer_start > 0, NULL);
    test_len = bacnet_enumerated_context_decode(
        &apdu[peer_start], (size_t)(apdu_len - peer_start), 1, &ui32);
    zassert_true(test_len > 0, NULL);
    peer_start += test_len;
    test_len = bacnet_datetime_context_decode(
        &apdu[peer_start], (size_t)(apdu_len - peer_start), 2, &datetime);
    zassert_true(test_len > 0, NULL);
    peer_start += test_len;
    test_len = bacnet_datetime_context_decode(
        &apdu[peer_start], (size_t)(apdu_len - peer_start), 3, &datetime);
    zassert_true(test_len > 0, NULL);
    peer_start += test_len;
    peer_len = host_n_port_context_decode(
        &apdu[peer_start], (size_t)(apdu_len - peer_start), 4, NULL, &hp);
    zassert_true(peer_len > 0, NULL);
    for (trunc_len = peer_start + 1; trunc_len < (peer_start + peer_len);
         trunc_len++) {
        test_len = bacapp_decode_SCDirectConnection(
            apdu, (size_t)trunc_len, &direct_conn_out);
        zassert_true(
            test_len < 0, "SCDirectConnection peer trunc len=%d", trunc_len);
    }
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnet_Secure_Connect_Tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        BACnet_Secure_Connect_Tests,
        ztest_unit_test(test_BACnet_Secure_Connect),
        ztest_unit_test(test_BACnet_Secure_Connect_Truncated_Decode));

    ztest_run_test_suite(BACnet_Secure_Connect_Tests);
}
#endif
