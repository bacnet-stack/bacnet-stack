/**
 * @file
 * @brief test the network layer message handling of the BACnet/IP to MS/TP
 *  reference router application
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "network_layer.h"
#include "bacnet/basic/sys/mstimer.h"
/* the router PRINT macro conflicts with the one from ztest */
#undef PRINT
#include <zephyr/ztest.h>

/* router globals normally provided by the router main.c */
ROUTER_PORT *head;
int port_count;

static ROUTER_PORT Test_Port;

static void reset_routes(void)
{
    cleanup_dnets(Test_Port.route_info.dnets);
    memset(&Test_Port, 0, sizeof(Test_Port));
    Test_Port.route_info.net = 1;
    head = &Test_Port;
}

static uint16_t added_net(size_t index)
{
    DNET *dnet = Test_Port.route_info.dnets;

    while ((index > 0) && (dnet != NULL)) {
        dnet = dnet->next;
        index--;
    }

    return dnet ? dnet->net : 0;
}

static void add_test_dnets(size_t count)
{
    BACNET_ADDRESS addr = { 0 };
    size_t i;

    for (i = 0; i < count; i++) {
        zassert_true(
            add_dnet(&Test_Port.route_info, (uint16_t)(i + 2), addr),
            "failed to add test DNET %u", (unsigned)i);
    }
}

static void add_unbounded_test_dnets(size_t count)
{
    DNET **link = &Test_Port.route_info.dnets;
    DNET *dnet;
    size_t i;
    unsigned long now = mstimer_now();

    for (i = 0; i < count; i++) {
        dnet = (DNET *)calloc(1, sizeof(DNET));
        zassert_not_null(dnet, NULL);
        dnet->net = (uint16_t)(i + 2);
        dnet->last_seen_ms = now;
        *link = dnet;
        link = &dnet->next;
    }
    Test_Port.route_info.dnet_count = count;
}

/**
 * @brief Run process_network_message() on a PDU that ends exactly at an
 *  inaccessible page, so any read past the end of the PDU faults.
 * @param pdu - NPDU bytes of the received message
 * @param len - number of NPDU bytes
 * @param dest_net - returns the dest net the router set for a net search
 * @return value returned by process_network_message()
 */
static int run(const uint8_t *pdu, size_t len, uint16_t *dest_net)
{
    long page_size = sysconf(_SC_PAGESIZE);
    size_t page;
    uint8_t *mem;
    MSG_DATA rx = { 0 };
    MSG_DATA work = { 0 };
    BACMSG msg = { 0 };
    uint8_t *buff = NULL;
    int rc;

    zassert_true(page_size > 0, NULL);
    page = (size_t)page_size;
    mem = mmap(
        NULL, 2 * page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
        0);
    zassert_true(mem != MAP_FAILED, NULL);
    zassert_equal(mprotect(mem + page, page, PROT_NONE), 0, NULL);
    rx.pdu = mem + page - len;
    memcpy(rx.pdu, pdu, len);
    rx.pdu_len = (uint16_t)len;
    msg.type = DATA;
    msg.data = &rx;

    rc = process_network_message(&msg, &work, &buff);
    if (dest_net) {
        *dest_net = work.dest.net;
    }
    if (rc > 0) {
        free(buff);
    }
    munmap(mem, 2 * page);

    return rc;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Who_Is_Router)
#else
static void test_Who_Is_Router(void)
#endif
{
    const uint8_t no_net[] = { 1, 0x80, 0 };
    const uint8_t short_net[] = { 1, 0x80, 0, 0x01 };
    const uint8_t net[] = { 1, 0x80, 0, 0x01, 0x02 };
    uint16_t dest_net = 0;

    reset_routes();
    zassert_true(run(no_net, sizeof(no_net), NULL) > 0, NULL);
    zassert_equal(run(short_net, sizeof(short_net), NULL), -2, NULL);
    /* unknown network: router starts a search for it */
    zassert_equal(run(net, sizeof(net), &dest_net), -1, NULL);
    zassert_equal(dest_net, 0x0102, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_I_Am_Router)
#else
static void test_I_Am_Router(void)
#endif
{
    const uint8_t odd[] = { 1, 0x80, 1, 0x00, 0x05, 0x00 };

    reset_routes();
    zassert_equal(run(odd, sizeof(odd), NULL), 0, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 1, NULL);
    zassert_equal(added_net(0), 5, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Reject_Message)
#else
static void test_Reject_Message(void)
#endif
{
    const uint8_t empty[] = { 1, 0x80, 3 };
    const uint8_t reason[] = { 1, 0x80, 3, 1 };

    reset_routes();
    zassert_equal(run(empty, sizeof(empty), NULL), -2, NULL);
    zassert_equal(run(reason, sizeof(reason), NULL), 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Init_Routing_Table)
#else
static void test_Init_Routing_Table(void)
#endif
{
    const uint8_t query[] = { 1, 0x80, 6, 0 };
    const uint8_t no_count[] = { 1, 0x80, 6 };
    const uint8_t too_many[] = { 1, 0x80, 6, 200 };
    /* entry 1 has 2 port info bytes, so entry 2 starts 6 octets later */
    const uint8_t two[] = { 1,    0x80, 6,    2,    0x00, 0x05, 1, 2,
                            0xAA, 0xBB, 0x01, 0x00, 7,    0,    0 };
    const uint8_t cut_entry[] = {
        1, 0x80, 6, 2, 0x00, 0x05, 1, 0, 0x00, 0x06, 2
    };
    const uint8_t cut_info[] = { 1, 0x80, 6, 1, 0x00, 0x05, 1, 3, 0xAA };

    reset_routes();
    zassert_true(run(query, sizeof(query), NULL) > 0, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 0, NULL);
    zassert_equal(run(no_count, sizeof(no_count), NULL), -2, NULL);
    zassert_equal(run(too_many, sizeof(too_many), NULL), -2, NULL);
    zassert_true(run(two, sizeof(two), NULL) > 0, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 2, NULL);
    zassert_equal(added_net(0), 5, NULL);
    zassert_equal(added_net(1), 0x0100, NULL);
    /* a truncated table is rejected without adding any entry */
    zassert_equal(run(cut_entry, sizeof(cut_entry), NULL), -2, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 2, NULL);
    zassert_equal(run(cut_info, sizeof(cut_info), NULL), -2, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 2, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Init_Routing_Table_Ack)
#else
static void test_Init_Routing_Table_Ack(void)
#endif
{
    const uint8_t no_count[] = { 1, 0x80, 7 };
    const uint8_t too_many[] = { 1, 0x80, 7, 200 };
    const uint8_t one[] = { 1, 0x80, 7, 1, 0x00, 0x09, 1, 0 };
    const uint8_t cut_info[] = { 1, 0x80, 7, 1, 0x00, 0x09, 1, 3, 0xAA };

    reset_routes();
    zassert_equal(run(no_count, sizeof(no_count), NULL), -2, NULL);
    zassert_equal(run(too_many, sizeof(too_many), NULL), -2, NULL);
    zassert_equal(run(one, sizeof(one), NULL), 0, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 1, NULL);
    zassert_equal(added_net(0), 9, NULL);
    zassert_equal(run(cut_info, sizeof(cut_info), NULL), -2, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 1, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_I_Am_Router_Response_Bounds)
#else
static void test_I_Am_Router_Response_Bounds(void)
#endif
{
    const uint8_t request[] = { 1, 0xA0, 0, 100, 7, 1, 2, 3, 4, 5, 6, 7, 1, 0 };
    MSG_DATA data = { 0 };
    uint8_t *buff = NULL;
    int len;
    size_t max_routes = (MAX_PDU - 14) / 2;

    data.src.net = 1;
    data.dest.net = BACNET_BROADCAST_NETWORK;
    data.dest.len = 7;

    reset_routes();
    len = create_network_message(
        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, &data, &buff, NULL);
    zassert_equal(len, 14, NULL);
    free(buff);

    add_test_dnets(57);
    len = create_network_message(
        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, &data, &buff, NULL);
    zassert_equal(len, 128, NULL);
    free(buff);

    reset_routes();
    add_test_dnets(58);
    len = run(request, sizeof(request), NULL);
    zassert_equal(len, 130, NULL);

    reset_routes();
    add_unbounded_test_dnets(max_routes);
    len = create_network_message(
        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, &data, &buff, NULL);
    zassert_equal(len, (int)MAX_PDU, NULL);
    free(buff);

    reset_routes();
    add_unbounded_test_dnets(max_routes + 1);
    len = create_network_message(
        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, &data, &buff, NULL);
    zassert_equal(len, (int)MAX_PDU, NULL);
    free(buff);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_DNET_Limit_Update_And_Expiry)
#else
static void test_DNET_Limit_Update_And_Expiry(void)
#endif
{
    BACNET_ADDRESS addr = { 0 };
    DNET *dnet;
    size_t i;

    reset_routes();
    addr.len = 1;
    addr.adr[0] = 10;
    zassert_true(add_dnet(&Test_Port.route_info, 100, addr), NULL);
    addr.adr[0] = 20;
    zassert_true(add_dnet(&Test_Port.route_info, 100, addr), NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 1, NULL);
    zassert_equal(Test_Port.route_info.dnets->mac[0], 20, NULL);

    Test_Port.route_info.dnets->last_seen_ms =
        mstimer_now() - ROUTER_DNET_MAX_AGE_MS - 1;
    expire_dnets(&Test_Port.route_info);
    zassert_is_null(Test_Port.route_info.dnets, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 0, NULL);

    for (i = 0; i < ROUTER_DNET_MAX_COUNT; i++) {
        zassert_true(
            add_dnet(&Test_Port.route_info, (uint16_t)(1000 + i), addr), NULL);
    }
    zassert_false(
        add_dnet(
            &Test_Port.route_info, (uint16_t)(1000 + ROUTER_DNET_MAX_COUNT),
            addr),
        NULL);
    zassert_equal(Test_Port.route_info.dnet_count, ROUTER_DNET_MAX_COUNT, NULL);

    dnet = Test_Port.route_info.dnets;
    zassert_not_null(dnet, NULL);
    dnet->last_seen_ms = mstimer_now() - ROUTER_DNET_MAX_AGE_MS - 1;
    zassert_true(
        add_dnet(
            &Test_Port.route_info, (uint16_t)(1000 + ROUTER_DNET_MAX_COUNT),
            addr),
        NULL);
    zassert_equal(Test_Port.route_info.dnet_count, ROUTER_DNET_MAX_COUNT, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Malformed_Npdu)
#else
static void test_Malformed_Npdu(void)
#endif
{
    /* destination address longer than the largest MAC address */
    const uint8_t dlen[] = { 1, 0xA0, 0x00, 0x05, 0xFF, 0x00 };
    /* too short to hold the control octet */
    const uint8_t one[] = { 1 };

    reset_routes();
    zassert_equal(run(dlen, sizeof(dlen), NULL), -2, NULL);
    zassert_equal(run(one, sizeof(one), NULL), -2, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Is_Network_Msg)
#else
static void test_Is_Network_Msg(void)
#endif
{
    uint8_t pdu[2] = { 1, 0x80 };
    MSG_DATA data = { 0 };
    BACMSG msg = { 0 };

    msg.data = &data;
    data.pdu = pdu;
    data.pdu_len = 2;
    zassert_true(is_network_msg(&msg), NULL);
    pdu[1] = 0x00;
    zassert_false(is_network_msg(&msg), NULL);
    /* too short to hold the control octet */
    data.pdu_len = 1;
    pdu[1] = 0x80;
    zassert_false(is_network_msg(&msg), NULL);
    data.pdu_len = 0;
    zassert_false(is_network_msg(&msg), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(router_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        router_tests, ztest_unit_test(test_Who_Is_Router),
        ztest_unit_test(test_I_Am_Router), ztest_unit_test(test_Reject_Message),
        ztest_unit_test(test_Init_Routing_Table),
        ztest_unit_test(test_Init_Routing_Table_Ack),
        ztest_unit_test(test_I_Am_Router_Response_Bounds),
        ztest_unit_test(test_DNET_Limit_Update_And_Expiry),
        ztest_unit_test(test_Malformed_Npdu),
        ztest_unit_test(test_Is_Network_Msg));

    ztest_run_test_suite(router_tests);
}
#endif
