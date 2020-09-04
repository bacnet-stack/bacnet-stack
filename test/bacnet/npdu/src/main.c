/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/npdu.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testNPDU2(void)
{
    uint8_t pdu[480] = { 0 };
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_ADDRESS npdu_dest = { 0 };
    BACNET_ADDRESS npdu_src = { 0 };
    int len = 0;
    bool data_expecting_reply = true;
    BACNET_MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL;
    BACNET_NPDU_DATA npdu_data = { 0 };
    int i = 0; /* counter */
    int npdu_len = 0;
    bool network_layer_message = false; /* false if APDU */
    BACNET_NETWORK_MESSAGE_TYPE network_message_type = 0; /* optional */
    uint16_t vendor_id = 0; /* optional, if net message type is > 0x80 */

    dest.mac_len = 6;
    for (i = 0; i < dest.mac_len; i++) {
        dest.mac[i] = i;
    }
    /* DNET,DLEN,DADR */
    dest.net = 1;
    dest.len = 6;
    for (i = 0; i < dest.len; i++) {
        dest.adr[i] = i * 10;
    }
    src.mac_len = 1;
    for (i = 0; i < src.mac_len; i++) {
        src.mac[i] = 0x80;
    }
    /* SNET,SLEN,SADR */
    src.net = 2;
    src.len = 1;
    for (i = 0; i < src.len; i++) {
        src.adr[i] = 0x40;
    }
    npdu_encode_npdu_data(&npdu_data, true, priority);
    len = npdu_encode_pdu(&pdu[0], &dest, &src, &npdu_data);
    zassert_not_equal(len, 0, NULL);
    /* can we get the info back? */
    npdu_len = npdu_decode(&pdu[0], &npdu_dest, &npdu_src, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    zassert_equal(npdu_data.data_expecting_reply, data_expecting_reply, NULL);
    zassert_equal(npdu_data.network_layer_message, network_layer_message, NULL);
    if (npdu_data.network_layer_message) {
        zassert_equal(npdu_data.network_message_type, network_message_type, NULL);
    }
    zassert_equal(npdu_data.vendor_id, vendor_id, NULL);
    zassert_equal(npdu_data.priority, priority, NULL);
    /* DNET,DLEN,DADR */
    zassert_equal(npdu_dest.net, dest.net, NULL);
    zassert_equal(npdu_dest.len, dest.len, NULL);
    for (i = 0; i < dest.len; i++) {
        zassert_equal(npdu_dest.adr[i], dest.adr[i], NULL);
    }
    /* SNET,SLEN,SADR */
    zassert_equal(npdu_src.net, src.net, NULL);
    zassert_equal(npdu_src.len, src.len, NULL);
    for (i = 0; i < src.len; i++) {
        zassert_equal(npdu_src.adr[i], src.adr[i], NULL);
    }
}

static void testNPDU1(void)
{
    uint8_t pdu[480] = { 0 };
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_ADDRESS npdu_dest = { 0 };
    BACNET_ADDRESS npdu_src = { 0 };
    int len = 0;
    bool data_expecting_reply = false;
    BACNET_MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL;
    BACNET_NPDU_DATA npdu_data = { 0 };
    int i = 0; /* counter */
    int npdu_len = 0;
    bool network_layer_message = false; /* false if APDU */
    BACNET_NETWORK_MESSAGE_TYPE network_message_type = 0; /* optional */
    uint16_t vendor_id = 0; /* optional, if net message type is > 0x80 */

    /* mac_len = 0 if global address */
    dest.mac_len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        dest.mac[i] = 0;
    }
    /* DNET,DLEN,DADR */
    dest.net = 0;
    dest.len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        dest.adr[i] = 0;
    }
    src.mac_len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        src.mac[i] = 0;
    }
    /* SNET,SLEN,SADR */
    src.net = 0;
    src.len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        src.adr[i] = 0;
    }
    npdu_encode_npdu_data(&npdu_data, false, priority);
    len = npdu_encode_pdu(&pdu[0], &dest, &src, &npdu_data);
    zassert_not_equal(len, 0, NULL);
    /* can we get the info back? */
    npdu_len = npdu_decode(&pdu[0], &npdu_dest, &npdu_src, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    zassert_equal(npdu_data.data_expecting_reply, data_expecting_reply, NULL);
    zassert_equal(npdu_data.network_layer_message, network_layer_message, NULL);
    if (npdu_data.network_layer_message) {
        zassert_equal(npdu_data.network_message_type, network_message_type, NULL);
    }
    zassert_equal(npdu_data.vendor_id, vendor_id, NULL);
    zassert_equal(npdu_data.priority, priority, NULL);
    zassert_equal(npdu_dest.mac_len, src.mac_len, NULL);
    zassert_equal(npdu_src.mac_len, dest.mac_len, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(npdu_tests,
     ztest_unit_test(testNPDU1),
     ztest_unit_test(testNPDU2)
     );

    ztest_run_test_suite(npdu_tests);
}
