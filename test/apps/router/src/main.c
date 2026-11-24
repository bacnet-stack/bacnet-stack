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
#include "bacnet/bacint.h"
#include "bacnet/basic/sys/mstimer.h"
/* the router PRINT macro conflicts with the one from ztest */
#undef PRINT
#include <zephyr/ztest.h>

/* router globals normally provided by the router main.c */
ROUTER_PORT *head;
int port_count;

static ROUTER_PORT Test_Port;
static ROUTER_PORT Test_Port_B;

static void reset_routes(void)
{
    cleanup_dnets(Test_Port.route_info.dnets);
    cleanup_dnets(Test_Port_B.route_info.dnets);
    memset(&Test_Port, 0, sizeof(Test_Port));
    memset(&Test_Port_B, 0, sizeof(Test_Port_B));
    Test_Port.route_info.net = 1;
    head = &Test_Port;
}

static bool port_has_net(const RT_ENTRY *route, uint16_t net)
{
    const DNET *dnet;

    if (!route) {
        return false;
    }
    dnet = route->dnets;
    while (dnet != NULL) {
        if (dnet->net == net) {
            return true;
        }
        dnet = dnet->next;
    }

    return false;
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
static int
run_process_msg(const uint8_t *pdu, size_t len, uint8_t **buff, int *reply_len);

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
    uint16_t victim;
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
    zassert_equal(Test_Port.route_info.dnet_count, ROUTER_DNET_MAX_COUNT, NULL);

    /* One millisecond older than the rest, and inside the test age. */
    dnet = Test_Port.route_info.dnets;
    zassert_not_null(dnet, NULL);
    victim = dnet->net;
    {
        unsigned long seen_now;
        DNET *newer;

        seen_now = mstimer_now();
        if (seen_now > 0UL) {
            dnet->last_seen_ms = seen_now - 1UL;
        } else {
            dnet->last_seen_ms = 0UL;
        }
        newer = dnet->next;
        while (newer != NULL) {
            newer->last_seen_ms = seen_now;
            newer = newer->next;
        }
    }
    zassert_true(
        add_dnet(
            &Test_Port.route_info, (uint16_t)(1000 + ROUTER_DNET_MAX_COUNT),
            addr),
        NULL);
    zassert_equal(Test_Port.route_info.dnet_count, ROUTER_DNET_MAX_COUNT, NULL);
    zassert_false(port_has_net(&Test_Port.route_info, victim), NULL);
    zassert_true(
        port_has_net(
            &Test_Port.route_info, (uint16_t)(1000 + ROUTER_DNET_MAX_COUNT)),
        NULL);

    dnet = Test_Port.route_info.dnets;
    zassert_not_null(dnet, NULL);
    victim = dnet->net;
    dnet->last_seen_ms = mstimer_now() - ROUTER_DNET_MAX_AGE_MS - 1;
    zassert_true(add_dnet(&Test_Port.route_info, 42, addr), NULL);
    zassert_equal(Test_Port.route_info.dnet_count, ROUTER_DNET_MAX_COUNT, NULL);
    zassert_false(port_has_net(&Test_Port.route_info, victim), NULL);
    zassert_true(port_has_net(&Test_Port.route_info, 42), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_DNET_Poison_Guards)
#else
static void test_DNET_Poison_Guards(void)
#endif
{
    const uint8_t iam_bad[] = {
        1, 0x80, 1, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x2A
    };
    const uint8_t iam_local[] = { 1, 0x80, 1, 0x00, 0x01 };
    const uint8_t irt_bad[] = { 1,    0x80, 6, 3, 0x00, 0x00, 1, 0,
                                0xFF, 0xFF, 2, 0, 0x00, 0x07, 3, 0 };
    BACNET_ADDRESS addr = { 0 };
    DNET *poison;

    reset_routes();
    addr.len = 1;
    addr.adr[0] = 4;
    zassert_false(add_dnet(&Test_Port.route_info, 0, addr), NULL);
    zassert_false(
        add_dnet(&Test_Port.route_info, BACNET_BROADCAST_NETWORK, addr), NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 0, NULL);

    zassert_equal(run(iam_bad, sizeof(iam_bad), NULL), 0, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 1, NULL);
    zassert_equal(added_net(0), 0x002A, NULL);
    zassert_false(port_has_net(&Test_Port.route_info, 0), NULL);
    zassert_false(
        port_has_net(&Test_Port.route_info, BACNET_BROADCAST_NETWORK), NULL);

    reset_routes();
    zassert_true(run(irt_bad, sizeof(irt_bad), NULL) > 0, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 1, NULL);
    zassert_equal(added_net(0), 7, NULL);

    reset_routes();
    zassert_equal(run(iam_local, sizeof(iam_local), NULL), 0, NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 0, NULL);
    zassert_false(add_dnet(&Test_Port.route_info, 1, addr), NULL);

    Test_Port_B.route_info.net = 2;
    Test_Port.next = &Test_Port_B;
    zassert_false(add_dnet(&Test_Port.route_info, 2, addr), NULL);
    zassert_false(add_dnet(&Test_Port_B.route_info, 1, addr), NULL);
    zassert_equal(Test_Port.route_info.dnet_count, 0, NULL);
    zassert_equal(Test_Port_B.route_info.dnet_count, 0, NULL);

    poison = (DNET *)calloc(1, sizeof(DNET));
    zassert_not_null(poison, NULL);
    poison->net = 2;
    Test_Port.route_info.dnets = poison;
    Test_Port.route_info.dnet_count = 1;
    zassert_equal(find_dnet(2, NULL), &Test_Port_B, NULL);

    zassert_true(add_dnet(&Test_Port.route_info, 100, addr), NULL);
    addr.adr[0] = 8;
    zassert_true(add_dnet(&Test_Port_B.route_info, 100, addr), NULL);
    zassert_false(port_has_net(&Test_Port.route_info, 100), NULL);
    zassert_true(port_has_net(&Test_Port_B.route_info, 100), NULL);
    zassert_equal(Test_Port_B.route_info.dnets->mac[0], 8, NULL);
    zassert_equal(Test_Port_B.route_info.dnet_count, 1, NULL);
    zassert_equal(find_dnet(100, NULL), &Test_Port_B, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Process_Msg_Unknown_Source)
#else
static void test_Process_Msg_Unknown_Source(void)
#endif
{
    const uint8_t local_dnet[] = { 0x01, 0x20, 0x00, 0x01, 0x00, 0xFF, 0x10 };
    uint8_t *buff = NULL;
    int buff_len;

    reset_routes();
    head = NULL;
    buff_len = run_process_msg(local_dnet, sizeof(local_dnet), &buff, NULL);
    zassert_equal(buff_len, -2, NULL);
    zassert_false(buff_len > 0, "an unknown source must not be forwarded");
    zassert_true(buff_len != -1, "an unknown source must not start a search");
    zassert_is_null(buff, NULL);
    reset_routes();
}

/**
 * @brief Local NPDU for network 1 plus an APDU of the given length.
 *  Layout: version, control (destination present), DNET 0x0001,
 *  DLEN 0, hop count 0xFF, then the APDU.
 *  process_msg() re-encodes this without the destination specifier.
 */
static uint8_t *local_apdu_message(size_t apdu_len, size_t *pdu_len)
{
    uint8_t *pdu;
    size_t len;

    len = 6 + apdu_len;
    pdu = (uint8_t *)calloc(1, len);
    zassert_not_null(pdu, NULL);
    pdu[0] = 0x01;
    pdu[1] = 0x20;
    pdu[2] = 0x00;
    pdu[3] = 0x01;
    pdu[4] = 0x00;
    pdu[5] = 0xFF;
    if (apdu_len > 0) {
        pdu[6] = 0x10;
    }
    *pdu_len = len;

    return pdu;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Process_Msg_Too_Long)
#else
static void test_Process_Msg_Too_Long(void)
#endif
{
    uint8_t *pdu;
    uint8_t *buff = NULL;
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS src = { 0 };
    size_t len;
    size_t apdu_len;
    int buff_len;
    int reply_len;
    int overhead;
    int offset;
    uint16_t rejected_net;

    reset_routes();
    pdu = local_apdu_message(1, &len);
    buff_len = run_process_msg(pdu, len, &buff, NULL);
    free(pdu);
    zassert_true(buff_len > 1, NULL);
    overhead = buff_len - 1;
    free(buff);
    buff = NULL;

    apdu_len = (size_t)((int)MAX_PDU - overhead);
    pdu = local_apdu_message(apdu_len, &len);
    buff_len = run_process_msg(pdu, len, &buff, NULL);
    free(pdu);
    zassert_equal(buff_len, (int)MAX_PDU, "MAX_PDU is forwarded");
    zassert_not_null(buff, NULL);
    zassert_equal(buff[1] & 0x80, 0, "a forwarded APDU is not a reject");
    free(buff);
    buff = NULL;

    apdu_len = (size_t)((int)MAX_PDU - overhead + 1);
    pdu = local_apdu_message(apdu_len, &len);
    reply_len = 0;
    buff_len = run_process_msg(pdu, len, &buff, &reply_len);
    free(pdu);
    zassert_equal(buff_len, ROUTER_MSG_REJECT, NULL);
    zassert_false(buff_len > 0, "MAX_PDU + 1 must not be forwarded");
    zassert_not_null(buff, NULL);
    zassert_true(reply_len > 0, NULL);
    offset =
        bacnet_npdu_decode(buff, (uint16_t)reply_len, &dest, &src, &npdu_data);
    zassert_true(offset > 0, NULL);
    zassert_equal(dest.net, 0, "a local origin has no DNET");
    zassert_equal(dest.len, 0, NULL);
    zassert_true(npdu_data.network_layer_message, NULL);
    zassert_equal(
        npdu_data.network_message_type,
        NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK, NULL);
    zassert_equal(buff[offset], NETWORK_REJECT_MESSAGE_TOO_LONG, NULL);
    rejected_net = 0;
    decode_unsigned16(&buff[offset + 1], &rejected_net);
    zassert_equal(rejected_net, 1, NULL);
    free(buff);
    buff = NULL;

    /* SNET 50, SADR 0x0A, destined for the directly connected network.
     * version, control (DNET and SNET), DNET 0x0001, DLEN 0,
     * SNET 0x0032, SLEN 1, SADR 0x0A, hop count, then the APDU */
    len = 10 + (size_t)MAX_PDU;
    pdu = (uint8_t *)calloc(1, len);
    zassert_not_null(pdu, NULL);
    pdu[0] = 0x01;
    pdu[1] = 0x28;
    pdu[2] = 0x00;
    pdu[3] = 0x01;
    pdu[4] = 0x00;
    pdu[5] = 0x00;
    pdu[6] = 0x32;
    pdu[7] = 0x01;
    pdu[8] = 0x0A;
    pdu[9] = 0xFF;
    reply_len = 0;
    memset(&dest, 0, sizeof(dest));
    memset(&npdu_data, 0, sizeof(npdu_data));
    buff_len = run_process_msg(pdu, len, &buff, &reply_len);
    free(pdu);
    zassert_equal(buff_len, ROUTER_MSG_REJECT, NULL);
    zassert_not_null(buff, NULL);
    zassert_true(reply_len > 0, NULL);
    offset =
        bacnet_npdu_decode(buff, (uint16_t)reply_len, &dest, &src, &npdu_data);
    zassert_true(offset > 0, NULL);
    zassert_equal(dest.net, 50, "reject DNET is the original SNET");
    zassert_equal(dest.len, 1, "reject DADR is present");
    zassert_equal(dest.adr[0], 0x0A, "reject DADR is the original SADR");
    zassert_equal(
        npdu_data.network_message_type,
        NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK, NULL);
    zassert_equal(buff[offset], NETWORK_REJECT_MESSAGE_TOO_LONG, NULL);
    rejected_net = 0;
    decode_unsigned16(&buff[offset + 1], &rejected_net);
    zassert_equal(rejected_net, 1, "payload DNET is the rejected network");
    free(buff);
    buff = NULL;

    /* SNET equals the source port's own network. That specifier is ignored,
     * so the reject stays local and does not name network 1 as DNET. */
    len = 10 + (size_t)MAX_PDU;
    pdu = (uint8_t *)calloc(1, len);
    zassert_not_null(pdu, NULL);
    /* version, control (DNET and SNET), DNET 0x0001, DLEN 0,
     * SNET 0x0001, SLEN 1, SADR 0x0A, hop count, then the APDU */
    pdu[0] = 0x01;
    pdu[1] = 0x28;
    pdu[2] = 0x00;
    pdu[3] = 0x01;
    pdu[4] = 0x00;
    pdu[5] = 0x00;
    pdu[6] = 0x01;
    pdu[7] = 0x01;
    pdu[8] = 0x0A;
    pdu[9] = 0xFF;
    reply_len = 0;
    memset(&dest, 0, sizeof(dest));
    memset(&npdu_data, 0, sizeof(npdu_data));
    buff_len = run_process_msg(pdu, len, &buff, &reply_len);
    free(pdu);
    zassert_equal(buff_len, ROUTER_MSG_REJECT, NULL);
    zassert_not_null(buff, NULL);
    zassert_true(reply_len > 0, NULL);
    offset =
        bacnet_npdu_decode(buff, (uint16_t)reply_len, &dest, &src, &npdu_data);
    zassert_true(offset > 0, NULL);
    zassert_equal(dest.net, 0, "spoofed local SNET is not a reject DNET");
    zassert_equal(dest.len, 0, NULL);
    zassert_equal(buff[offset], NETWORK_REJECT_MESSAGE_TOO_LONG, NULL);
    rejected_net = 0;
    decode_unsigned16(&buff[offset + 1], &rejected_net);
    zassert_equal(rejected_net, 1, NULL);
    free(buff);
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

/**
 * @brief Invoke process_msg() the way main() does and return its status.
 *  The received PDU is freed here; a forwarded PDU is left in @p buff.
 */
static int
run_process_msg(const uint8_t *pdu, size_t len, uint8_t **buff, int *reply_len)
{
    MSG_DATA rx = { 0 };
    MSG_DATA work = { 0 };
    BACMSG msg = { 0 };
    uint8_t *storage;
    int rc;

    storage = (uint8_t *)malloc(len);
    zassert_not_null(storage, NULL);
    memcpy(storage, pdu, len);
    rx.pdu = storage;
    rx.pdu_len = (uint16_t)len;
    msg.type = DATA;
    msg.origin = Test_Port.port_id;
    msg.data = &rx;

    rc = process_msg(&msg, &work, buff);
    if (reply_len) {
        *reply_len = work.pdu_len;
    }
    free(storage);

    return rc;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Process_Msg_Return_Code)
#else
static void test_Process_Msg_Return_Code(void)
#endif
{
    /* application NPDU for a network this router has not learned */
    const uint8_t unknown_dnet[] = { 0x01, 0x20, 0x00, 0x63, 0x00, 0xFF, 0x10 };
    /* destination length is larger than the NPDU */
    const uint8_t malformed[] = { 0x01, 0x20, 0x00, 0x05, 0xFF };
    /* application NPDU for the directly connected network */
    const uint8_t local_dnet[] = { 0x01, 0x20, 0x00, 0x01, 0x00, 0xFF, 0x10 };
    uint8_t stale = 0xA5;
    uint8_t *buff;
    int buff_len;

    reset_routes();

    /* main() sends only when the result is a positive length. */
    buff = &stale;
    buff_len = run_process_msg(unknown_dnet, sizeof(unknown_dnet), &buff, NULL);
    zassert_equal(buff_len, -1, NULL);
    zassert_false(buff_len > 0, "a net search must not be sent as a PDU");
    zassert_is_null(buff, NULL);

    buff = &stale;
    buff_len = run_process_msg(malformed, sizeof(malformed), &buff, NULL);
    zassert_equal(buff_len, -2, NULL);
    zassert_false(buff_len > 0, "a discarded NPDU must not be sent as a PDU");
    zassert_true(buff_len != -1, "a malformed NPDU must not start a search");
    zassert_is_null(buff, NULL);

    buff = NULL;
    buff_len = run_process_msg(local_dnet, sizeof(local_dnet), &buff, NULL);
    zassert_true(buff_len > 0, "a known destination is a PDU length");
    zassert_not_null(buff, NULL);
    free(buff);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_DNET_Refresh_On_Traffic)
#else
static void test_DNET_Refresh_On_Traffic(void)
#endif
{
    /* forwarded to learned network 99 */
    const uint8_t to_remote[] = { 0x01, 0x20, 0x00, 0x63, 0x00, 0xFF, 0x10 };
    /* received from learned network 50 and delivered locally */
    const uint8_t from_remote[] = { 0x01, 0x28, 0x00, 0x01, 0x00, 0x00,
                                    0x32, 0x01, 0x0A, 0xFF, 0x10 };
    BACNET_ADDRESS addr = { 0 };
    DNET *forwarded;
    DNET *received;
    uint8_t *buff = NULL;
    unsigned long now;
    unsigned long forwarded_seen;
    unsigned long received_seen;
    int buff_len;

    reset_routes();
    addr.len = 1;
    addr.adr[0] = 9;
    zassert_true(add_dnet(&Test_Port.route_info, 99, addr), NULL);
    addr.adr[0] = 5;
    zassert_true(add_dnet(&Test_Port.route_info, 50, addr), NULL);
    forwarded = Test_Port.route_info.dnets;
    received = forwarded->next;
    zassert_not_null(received, NULL);
    zassert_equal(forwarded->net, 99, NULL);
    zassert_equal(received->net, 50, NULL);

    now = mstimer_now();
    if (now > 1000UL) {
        forwarded_seen = now - 1000UL;
    } else {
        forwarded_seen = 0UL;
    }
    received_seen = forwarded_seen;
    forwarded->last_seen_ms = forwarded_seen;
    received->last_seen_ms = received_seen;

    buff_len = run_process_msg(to_remote, sizeof(to_remote), &buff, NULL);
    zassert_true(buff_len > 0, NULL);
    zassert_true(
        forwarded->last_seen_ms > forwarded_seen,
        "forwarding via a DNET must refresh it");
    free(buff);
    buff = NULL;

    /* find_dnet() of the local network does not walk learned entries */
    buff_len = run_process_msg(from_remote, sizeof(from_remote), &buff, NULL);
    zassert_true(buff_len > 0, NULL);
    zassert_true(
        received->last_seen_ms > received_seen,
        "traffic received from a DNET must refresh it");
    free(buff);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Send_Failure_Keeps_Caller_Data)
#else
static void test_Send_Failure_Keeps_Caller_Data(void)
#endif
{
    MSG_DATA *data;
    uint8_t *buff = NULL;
    int rc;

    reset_routes();
    data = (MSG_DATA *)calloc(1, sizeof(*data));
    zassert_not_null(data, NULL);
    data->dest.net = BACNET_BROADCAST_NETWORK;
    rc = send_network_message(
        NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK, data, &buff, NULL);
    zassert_true(rc < 0, NULL);
    zassert_is_null(buff, NULL);
    /* caller still owns the message and must free it, as main() does */
    free(data);
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
        ztest_unit_test(test_DNET_Poison_Guards),
        ztest_unit_test(test_Process_Msg_Unknown_Source),
        ztest_unit_test(test_Process_Msg_Too_Long),
        ztest_unit_test(test_Malformed_Npdu),
        ztest_unit_test(test_Process_Msg_Return_Code),
        ztest_unit_test(test_DNET_Refresh_On_Traffic),
        ztest_unit_test(test_Send_Failure_Keeps_Caller_Data),
        ztest_unit_test(test_Is_Network_Msg));

    ztest_run_test_suite(router_tests);
}
#endif
