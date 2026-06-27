/**
 * @file
 * @brief test BACnet NPDU encoding and decoding API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/npdu.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_Network)
#else
static void test_NPDU_Network(void)
#endif
{
    uint8_t pdu[MAX_NPDU] = { 0 };
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_ADDRESS npdu_dest = { 0 };
    BACNET_ADDRESS npdu_src = { 0 };
    int len = 0, null_len = 0, test_len = 0;
    bool data_expecting_reply = true;
    BACNET_NETWORK_MESSAGE_TYPE network_message_type =
        NETWORK_MESSAGE_NETWORK_NUMBER_IS;
    BACNET_MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL;
    BACNET_NPDU_DATA npdu_data = { 0 };
    bool network_layer_message = true;
    uint16_t vendor_id = 0; /* optional, if net message type is > 0x80 */

    npdu_encode_npdu_network(
        &npdu_data, network_message_type, data_expecting_reply, priority);
    null_len = bacnet_npdu_encode_pdu(NULL, 0, &dest, &src, &npdu_data);
    len = bacnet_npdu_encode_pdu(&pdu[0], sizeof(pdu), &dest, &src, &npdu_data);
    zassert_equal(len, null_len, NULL);
    zassert_not_equal(len, 0, NULL);
    /* can we get the info back? */
    test_len =
        bacnet_npdu_decode(pdu, sizeof(pdu), &npdu_dest, &npdu_src, &npdu_data);
    zassert_not_equal(test_len, 0, NULL);
    zassert_equal(len, test_len, NULL);
    zassert_equal(npdu_data.data_expecting_reply, data_expecting_reply, NULL);
    zassert_equal(npdu_data.network_layer_message, network_layer_message, NULL);
    zassert_equal(npdu_data.network_message_type, network_message_type, NULL);
    zassert_equal(npdu_data.vendor_id, vendor_id, NULL);
    zassert_equal(npdu_data.priority, priority, NULL);
    /* test for invalid short PDU */
    while (len) {
        len--;
        test_len =
            bacnet_npdu_decode(pdu, len, &npdu_dest, &npdu_src, &npdu_data);
        if (len == 2) {
            /* special case with no NPDU options */
            zassert_equal(len, test_len, NULL);
        } else {
            zassert_true(test_len <= 0, "len=%d test_len=%d\n", len, test_len);
        }
    }
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, testNPDU2)
#else
static void testNPDU2(void)
#endif
{
    uint8_t pdu[MAX_NPDU] = { 0 };
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
    len = bacnet_npdu_encode_pdu(&pdu[0], sizeof(pdu), &dest, &src, &npdu_data);
    zassert_not_equal(len, 0, NULL);
    /* can we get the info back? */
    npdu_len =
        bacnet_npdu_decode(pdu, sizeof(pdu), &npdu_dest, &npdu_src, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    zassert_equal(npdu_data.data_expecting_reply, data_expecting_reply, NULL);
    zassert_equal(npdu_data.network_layer_message, network_layer_message, NULL);
    if (npdu_data.network_layer_message) {
        zassert_equal(
            npdu_data.network_message_type, network_message_type, NULL);
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, testNPDU1)
#else
static void testNPDU1(void)
#endif
{
    uint8_t pdu[MAX_NPDU] = { 0 };
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
    len = bacnet_npdu_encode_pdu(&pdu[0], sizeof(pdu), &dest, &src, &npdu_data);
    zassert_not_equal(len, 0, NULL);
    /* can we get the info back? */
    npdu_len =
        bacnet_npdu_decode(pdu, sizeof(pdu), &npdu_dest, &npdu_src, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    zassert_equal(npdu_data.data_expecting_reply, data_expecting_reply, NULL);
    zassert_equal(npdu_data.network_layer_message, network_layer_message, NULL);
    if (npdu_data.network_layer_message) {
        zassert_equal(
            npdu_data.network_message_type, network_message_type, NULL);
    }
    zassert_equal(npdu_data.vendor_id, vendor_id, NULL);
    zassert_equal(npdu_data.priority, priority, NULL);
    zassert_equal(npdu_dest.mac_len, src.mac_len, NULL);
    zassert_equal(npdu_src.mac_len, dest.mac_len, NULL);
}
/**
 * @}
 */

/**
 * @brief Test npdu_i_am_router_to_network_process
 */
struct test_dnet_add_data {
    uint16_t snet;
    uint16_t net[16];
    unsigned count;
};

static void
test_dnet_add_callback(uint16_t snet, uint16_t net, const BACNET_ADDRESS *addr)
{
    struct test_dnet_add_data *data =
        (struct test_dnet_add_data *)(uintptr_t)(const void *)addr;
    data->snet = snet;
    if (data->count < 16) {
        data->net[data->count++] = net;
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_I_Am_Router_To_Network_Process)
#else
static void test_NPDU_I_Am_Router_To_Network_Process(void)
#endif
{
    struct test_dnet_add_data cb_data = { 0 };
    uint8_t npdu[16] = { 0 };
    uint16_t npdu_size = 0;
    uint16_t snet = 42;

    /* encode two network numbers: 100 and 200 */
    npdu[0] = 0x00;
    npdu[1] = 100;
    npdu[2] = 0x00;
    npdu[3] = 200;
    npdu_size = 4;

    npdu_i_am_router_to_network_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size,
        test_dnet_add_callback);

    zassert_equal(cb_data.count, 2, NULL);
    zassert_equal(cb_data.snet, snet, NULL);
    zassert_equal(cb_data.net[0], 100, NULL);
    zassert_equal(cb_data.net[1], 200, NULL);

    /* empty buffer - nothing should be added */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu_i_am_router_to_network_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, 1, test_dnet_add_callback);
    zassert_equal(cb_data.count, 0, NULL);

    /* NULL callback - no crash */
    npdu_i_am_router_to_network_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size, NULL);

    /* single network number */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu[0] = 0x00;
    npdu[1] = 0xFF;
    npdu_size = 2;
    npdu_i_am_router_to_network_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size,
        test_dnet_add_callback);
    zassert_equal(cb_data.count, 1, NULL);
    zassert_equal(cb_data.net[0], 255, NULL);
}

/**
 * @brief Test npdu_init_routing_table_process
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_Init_Routing_Table_Process)
#else
static void test_NPDU_Init_Routing_Table_Process(void)
#endif
{
    struct test_dnet_add_data cb_data = { 0 };
    uint8_t npdu[64] = { 0 };
    uint16_t npdu_size = 0;
    uint16_t snet = 10;
    uint8_t *p = npdu;

    /* encode 2 entries:
     * net_count = 2
     * Entry 1: DNET=300, PortID=1, PortInfoLen=0
     * Entry 2: DNET=400, PortID=2, PortInfoLen=3, PortInfo={0xAA,0xBB,0xCC} */
    *p++ = 2; /* net_count */
    /* entry 1 */
    *p++ = 0x01; /* DNET high */
    *p++ = 0x2C; /* DNET low: 300 */
    *p++ = 1; /* PortID */
    *p++ = 0; /* PortInfoLen */
    /* entry 2 */
    *p++ = 0x01; /* DNET high */
    *p++ = 0x90; /* DNET low: 400 */
    *p++ = 2; /* PortID */
    *p++ = 3; /* PortInfoLen */
    *p++ = 0xAA;
    *p++ = 0xBB;
    *p++ = 0xCC;
    npdu_size = (uint16_t)(p - npdu);

    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size,
        test_dnet_add_callback);

    zassert_equal(cb_data.count, 2, NULL);
    zassert_equal(cb_data.snet, snet, NULL);
    zassert_equal(cb_data.net[0], 300, NULL);
    zassert_equal(cb_data.net[1], 400, NULL);

    /* malformed: too short (<=1 byte) */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, 1, test_dnet_add_callback);
    zassert_equal(cb_data.count, 0, NULL);

    /* net_count == 0 */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu[0] = 0;
    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size,
        test_dnet_add_callback);
    zassert_equal(cb_data.count, 0, NULL);

    /* NULL callback - no crash */
    npdu[0] = 2;
    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size, NULL);

    /* truncated buffer - only enough bytes for first entry's
     * DNET+PortID+PortInfoLen but not enough for second entry's 4-byte minimum;
     * first entry should be added */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu[0] = 2;
    /* npdu_size=7: net_count(1)+DNET(2)+PortID(1)+PortInfoLen(1)+2bytes of
     * entry2 After processing entry 1: npdu_len=3, which is < 4 so loop exits
     */
    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, 7, test_dnet_add_callback);
    zassert_equal(cb_data.count, 1, NULL);
    zassert_equal(cb_data.net[0], 300, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(npdu_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        npdu_tests, ztest_unit_test(testNPDU1), ztest_unit_test(testNPDU2),
        ztest_unit_test(test_NPDU_Network),
        ztest_unit_test(test_NPDU_I_Am_Router_To_Network_Process),
        ztest_unit_test(test_NPDU_Init_Routing_Table_Process));

    ztest_run_test_suite(npdu_tests);
}
#endif
