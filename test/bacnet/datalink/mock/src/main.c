/**
 * @file
 * @brief Mock for all the BACnet datalinks
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Greg Shue <greg.shue@outlook.com>
 * @date 2020
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdlib.h> /* For calloc() */
#include <zephyr/ztest.h>
#include <bacnet/datalink/datalink.h>
#include "bacnet/apdu.h"

void bvlc_maintenance_timer(uint16_t seconds)
{
    ztest_check_expected_value(seconds);
}

void bvlc6_maintenance_timer(uint16_t seconds)
{
    ztest_check_expected_value(seconds);
}

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test datalink
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datalink_tests, test_datalink_arcnet)
#else
static void test_datalink_arcnet(void)
#endif
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = { .mac_len = 6,
                            .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
                            .net = 54,
                            .len = 7,
                            .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54,
                                     0x32 } };
    BACNET_ADDRESS addr2 = { 0 };
    BACNET_NPDU_DATA npdu = { 0 };

    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("arcnet");

    // init
    ztest_expect_value(arcnet_init, interface_name, iface);
    ztest_returns_value(arcnet_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(arcnet_init, interface_name, iface2);
    ztest_returns_value(arcnet_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(arcnet_send_pdu, dest, &addr);
    ztest_expect_value(arcnet_send_pdu, npdu_data, &npdu);
    ztest_expect_data(arcnet_send_pdu, pdu, expected_data);
    ztest_returns_value(arcnet_send_pdu, 4);
    zassert_equal(
        datalink_send_pdu(&addr, &npdu, expected_data, sizeof(expected_data)),
        4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(arcnet_receive, src, &addr);
    ztest_expect_value(arcnet_receive, timeout, 10);
    ztest_expect_data(arcnet_receive, pdu, expected_data);
    ztest_returns_value(arcnet_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(arcnet_receive, src, &addr);
    ztest_expect_value(arcnet_receive, timeout, 15);
    ztest_expect_data(arcnet_receive, pdu, expected_data);
    ztest_returns_value(arcnet_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(arcnet_get_broadcast_address, dest, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(arcnet_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);

    // maintenance_timer - do nothing for arcnet
    datalink_maintenance_timer(42);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datalink_tests, test_datalink_bip)
#else
static void test_datalink_bip(void)
#endif
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = { .mac_len = 6,
                            .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
                            .net = 54,
                            .len = 7,
                            .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54,
                                     0x32 } };
    BACNET_ADDRESS addr2 = { 0 };
    BACNET_NPDU_DATA npdu = { 0 };

    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("bip");

    // init
    ztest_expect_value(bip_init, ifname, iface);
    ztest_returns_value(bip_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(bip_init, ifname, iface2);
    ztest_returns_value(bip_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(bip_send_pdu, dest, &addr);
    ztest_expect_value(bip_send_pdu, npdu_data, &npdu);
    ztest_expect_data(bip_send_pdu, pdu, expected_data);
    ztest_returns_value(bip_send_pdu, 4);
    zassert_equal(
        datalink_send_pdu(&addr, &npdu, expected_data, sizeof(expected_data)),
        4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(bip_receive, src, &addr);
    ztest_expect_value(bip_receive, timeout, 10);
    ztest_expect_data(bip_receive, pdu, expected_data);
    ztest_returns_value(bip_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(bip_receive, src, &addr);
    ztest_expect_value(bip_receive, timeout, 15);
    ztest_expect_data(bip_receive, pdu, expected_data);
    ztest_returns_value(bip_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(bip_get_broadcast_address, dest, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(bip_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // maintenance_timer
    ztest_expect_value(bvlc_maintenance_timer, seconds, 42);
    datalink_maintenance_timer(42);
    zassert_equal(z_cleanup_mock(), 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datalink_tests, test_datalink_bip6)
#else
static void test_datalink_bip6(void)
#endif
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = { .mac_len = 6,
                            .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
                            .net = 54,
                            .len = 7,
                            .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54,
                                     0x32 } };
    BACNET_ADDRESS addr2 = { 0 };
    BACNET_NPDU_DATA npdu = { 0 };

    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("bip6");

    // init
    ztest_expect_value(bip6_init, ifname, iface);
    ztest_returns_value(bip6_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(bip6_init, ifname, iface2);
    ztest_returns_value(bip6_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(bip6_send_pdu, dest, &addr);
    ztest_expect_value(bip6_send_pdu, npdu_data, &npdu);
    ztest_expect_data(bip6_send_pdu, pdu, expected_data);
    ztest_returns_value(bip6_send_pdu, 4);
    zassert_equal(
        datalink_send_pdu(&addr, &npdu, expected_data, sizeof(expected_data)),
        4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(bip6_receive, src, &addr);
    ztest_expect_value(bip6_receive, timeout, 10);
    ztest_expect_data(bip6_receive, pdu, expected_data);
    ztest_returns_value(bip6_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(bip6_receive, src, &addr);
    ztest_expect_value(bip6_receive, timeout, 15);
    ztest_expect_data(bip6_receive, pdu, expected_data);
    ztest_returns_value(bip6_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(bip6_get_broadcast_address, my_address, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(bip6_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);

    // maintenance_timer
    ztest_expect_value(bvlc6_maintenance_timer, seconds, 42);
    datalink_maintenance_timer(42);
    zassert_equal(z_cleanup_mock(), 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datalink_tests, test_datalink_dlmstp)
#else
static void test_datalink_dlmstp(void)
#endif
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = { .mac_len = 6,
                            .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
                            .net = 54,
                            .len = 7,
                            .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54,
                                     0x32 } };
    BACNET_ADDRESS addr2 = { 0 };
    BACNET_NPDU_DATA npdu = { 0 };

    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("mstp");

    // init
    ztest_expect_value(dlmstp_init, ifname, iface);
    ztest_returns_value(dlmstp_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(dlmstp_init, ifname, iface2);
    ztest_returns_value(dlmstp_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(dlmstp_send_pdu, dest, &addr);
    ztest_expect_value(dlmstp_send_pdu, npdu_data, &npdu);
    ztest_expect_data(dlmstp_send_pdu, pdu, expected_data);
    ztest_returns_value(dlmstp_send_pdu, 4);
    zassert_equal(
        datalink_send_pdu(&addr, &npdu, expected_data, sizeof(expected_data)),
        4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(dlmstp_receive, src, &addr);
    ztest_expect_value(dlmstp_receive, timeout, 10);
    ztest_expect_data(dlmstp_receive, pdu, expected_data);
    ztest_returns_value(dlmstp_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(dlmstp_receive, src, &addr);
    ztest_expect_value(dlmstp_receive, timeout, 15);
    ztest_expect_data(dlmstp_receive, pdu, expected_data);
    ztest_returns_value(dlmstp_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(dlmstp_get_broadcast_address, dest, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(dlmstp_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);

    // maintenance_timer - do nothing for dlmstp
    datalink_maintenance_timer(42);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(datalink_tests, )
#else
static void test_datalink_ethernet(void)
#endif
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = { .mac_len = 6,
                            .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
                            .net = 54,
                            .len = 7,
                            .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54,
                                     0x32 } };
    BACNET_ADDRESS addr2 = { 0 };
    BACNET_NPDU_DATA npdu = { 0 };

    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("ethernet");

    // init
    ztest_expect_value(ethernet_init, interface_name, iface);
    ztest_returns_value(ethernet_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(ethernet_init, interface_name, iface2);
    ztest_returns_value(ethernet_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(ethernet_send_pdu, dest, &addr);
    ztest_expect_value(ethernet_send_pdu, npdu_data, &npdu);
    ztest_expect_data(ethernet_send_pdu, pdu, expected_data);
    ztest_returns_value(ethernet_send_pdu, 4);
    zassert_equal(
        datalink_send_pdu(&addr, &npdu, expected_data, sizeof(expected_data)),
        4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(ethernet_receive, src, &addr);
    ztest_expect_value(ethernet_receive, timeout, 10);
    ztest_expect_data(ethernet_receive, pdu, expected_data);
    ztest_returns_value(ethernet_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(ethernet_receive, src, &addr);
    ztest_expect_value(ethernet_receive, timeout, 15);
    ztest_expect_data(ethernet_receive, pdu, expected_data);
    ztest_returns_value(ethernet_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(ethernet_get_broadcast_address, dest, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(ethernet_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);

    // maintenance_timer - do nothing for ethernet
    datalink_maintenance_timer(42);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(datalink_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        datalink_tests, ztest_unit_test(test_datalink_arcnet),
        ztest_unit_test(test_datalink_bip), ztest_unit_test(test_datalink_bip6),
        ztest_unit_test(test_datalink_dlmstp),
        ztest_unit_test(test_datalink_ethernet));

    ztest_run_test_suite(datalink_tests);
}
#endif
