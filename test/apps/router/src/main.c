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
#include "network_layer.h"
/* the router PRINT macro conflicts with the one from ztest */
#undef PRINT
#include <zephyr/ztest.h>

/* router globals normally provided by the router main.c */
ROUTER_PORT *head;
int port_count;

/* routing table calls captured by the stubs below */
#define ADDED_MAX 8
static uint16_t Added_Net[ADDED_MAX];
static int Added_Count;
static ROUTER_PORT Test_Port;

ROUTER_PORT *find_snet(MSGBOX_ID id)
{
    (void)id;
    return &Test_Port;
}

ROUTER_PORT *find_dnet(uint16_t net, BACNET_ADDRESS *addr)
{
    (void)net;
    (void)addr;
    return NULL;
}

void add_dnet(RT_ENTRY *route_info, uint16_t net, BACNET_ADDRESS addr)
{
    (void)route_info;
    (void)addr;
    if (Added_Count < ADDED_MAX) {
        Added_Net[Added_Count] = net;
    }
    Added_Count++;
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
    const size_t page = 4096;
    uint8_t *mem;
    MSG_DATA rx = { 0 };
    MSG_DATA work = { 0 };
    BACMSG msg = { 0 };
    uint8_t *buff = NULL;
    int rc;

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

    Added_Count = 0;
    rc = (int16_t)process_network_message(&msg, &work, &buff);
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

    zassert_equal(run(odd, sizeof(odd), NULL), 0, NULL);
    zassert_equal(Added_Count, 1, NULL);
    zassert_equal(Added_Net[0], 5, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(router_tests, test_Reject_Message)
#else
static void test_Reject_Message(void)
#endif
{
    const uint8_t empty[] = { 1, 0x80, 3 };
    const uint8_t reason[] = { 1, 0x80, 3, 1 };

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

    zassert_true(run(query, sizeof(query), NULL) > 0, NULL);
    zassert_equal(Added_Count, 0, NULL);
    zassert_equal(run(no_count, sizeof(no_count), NULL), -2, NULL);
    zassert_equal(run(too_many, sizeof(too_many), NULL), -2, NULL);
    zassert_true(run(two, sizeof(two), NULL) > 0, NULL);
    zassert_equal(Added_Count, 2, NULL);
    zassert_equal(Added_Net[0], 5, NULL);
    zassert_equal(Added_Net[1], 0x0100, NULL);
    /* a truncated table is rejected without adding any entry */
    zassert_equal(run(cut_entry, sizeof(cut_entry), NULL), -2, NULL);
    zassert_equal(Added_Count, 0, NULL);
    zassert_equal(run(cut_info, sizeof(cut_info), NULL), -2, NULL);
    zassert_equal(Added_Count, 0, NULL);
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

    zassert_equal(run(no_count, sizeof(no_count), NULL), -2, NULL);
    zassert_equal(run(too_many, sizeof(too_many), NULL), -2, NULL);
    zassert_equal(run(one, sizeof(one), NULL), 0, NULL);
    zassert_equal(Added_Count, 1, NULL);
    zassert_equal(Added_Net[0], 9, NULL);
    zassert_equal(run(cut_info, sizeof(cut_info), NULL), -2, NULL);
    zassert_equal(Added_Count, 0, NULL);
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
        ztest_unit_test(test_Malformed_Npdu),
        ztest_unit_test(test_Is_Network_Msg));

    ztest_run_test_suite(router_tests);
}
#endif
