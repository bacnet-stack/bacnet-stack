/**
 * @file
 * @brief Tests for separating BACnet/IP bind and broadcast destination ports
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/datalink/bip.h>

static uint16_t bip_address_port(const BACNET_ADDRESS *address)
{
    return ((uint16_t)address->mac[4] << 8) | address->mac[5];
}

static void test_broadcast_port_defaults_to_bind_port(void)
{
    BACNET_ADDRESS broadcast = { 0 };
    BACNET_IP_ADDRESS broadcast_addr = { 0 };

    bip_cleanup();
    bip_set_port(0xBAC1U);

    zassert_equal(bip_get_port(), 0xBAC1U, NULL);
    zassert_equal(bip_get_broadcast_port(), 0xBAC1U, NULL);

    bip_get_broadcast_address(&broadcast);
    zassert_equal(bip_address_port(&broadcast), 0xBAC1U, NULL);

    zassert_true(bip_get_broadcast_addr(&broadcast_addr), NULL);
    zassert_equal(broadcast_addr.port, 0xBAC1U, NULL);
}

static void test_broadcast_port_can_differ_from_bind_port(void)
{
    BACNET_ADDRESS local = { 0 };
    BACNET_ADDRESS broadcast = { 0 };
    BACNET_IP_ADDRESS broadcast_addr = { 0 };

    bip_cleanup();
    bip_set_port(0xBAC1U);
    bip_set_broadcast_port(0xBAC2U);

    zassert_equal(bip_get_port(), 0xBAC1U, NULL);
    zassert_equal(bip_get_broadcast_port(), 0xBAC2U, NULL);

    bip_get_my_address(&local);
    zassert_equal(bip_address_port(&local), 0xBAC1U, NULL);

    bip_get_broadcast_address(&broadcast);
    zassert_equal(bip_address_port(&broadcast), 0xBAC2U, NULL);

    zassert_true(bip_get_broadcast_addr(&broadcast_addr), NULL);
    zassert_equal(broadcast_addr.port, 0xBAC2U, NULL);
}

void test_main(void)
{
    ztest_test_suite(
        bip_broadcast_port_test,
        ztest_unit_test(test_broadcast_port_defaults_to_bind_port),
        ztest_unit_test(test_broadcast_port_can_differ_from_bind_port));
    ztest_run_test_suite(bip_broadcast_port_test);
}
