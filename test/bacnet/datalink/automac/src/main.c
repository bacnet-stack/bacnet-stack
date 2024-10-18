/**
 * @file
 * @brief test BACnet MSTP zero-config auto MAC address
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2023-June
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/datalink/automac.h>
#include <bacnet/basic/sys/bytes.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test Auto-MAC initialization
 */
static void test_automac_init(void)
{
    uint8_t mac = 255;
    bool options;

    automac_init();
    zassert_equal(automac_free_address_count(), 0, NULL);
    mac = automac_free_address_mac(0);
    zassert_equal(mac, 255, NULL);
    automac_pfm_set(MSTP_MAC_SLOTS_OFFSET);
    mac = automac_free_address_mac(0);
    zassert_equal(mac, MSTP_MAC_SLOTS_OFFSET, NULL);
    zassert_equal(automac_free_address_count(), 1, NULL);
    automac_token_set(MSTP_MAC_SLOTS_OFFSET);
    mac = automac_free_address_mac(0);
    zassert_equal(mac, 255, NULL);
    zassert_equal(automac_free_address_count(), 0, NULL);
    automac_pfm_set(127);
    mac = automac_free_address_mac(0);
    zassert_equal(mac, 127, NULL);
    zassert_equal(automac_free_address_count(), 1, NULL);
    automac_token_set(127);
    mac = automac_free_address_mac(0);
    zassert_equal(mac, 255, NULL);
    zassert_equal(automac_free_address_count(), 0, NULL);
    /* the ANSI rand() uses consistent sequence based on seed */
    srand(42);
    mac = automac_free_address_random();
    zassert_equal(mac, 255, NULL);
    automac_pfm_set(MSTP_MAC_SLOTS_OFFSET + 1);
    mac = automac_free_address_mac(0);
    zassert_equal(mac, (MSTP_MAC_SLOTS_OFFSET + 1), NULL);
    mac = automac_free_address_random();
    zassert_equal(mac, (MSTP_MAC_SLOTS_OFFSET + 1), NULL);
    /* test 2 free addresses */
    automac_pfm_set(MSTP_MAC_SLOTS_OFFSET + 2);
    mac = automac_free_address_mac(0);
    zassert_equal(mac, (MSTP_MAC_SLOTS_OFFSET + 1), NULL);
    mac = automac_free_address_mac(1);
    zassert_equal(mac, (MSTP_MAC_SLOTS_OFFSET + 2), NULL);
    mac = automac_free_address_random();
    options = (mac == (MSTP_MAC_SLOTS_OFFSET + 1)) ||
        (mac == (MSTP_MAC_SLOTS_OFFSET + 2));
    zassert_true(options, NULL);
    /* test 3 free addresses */
    automac_pfm_set(126);
    mac = automac_free_address_mac(0);
    zassert_equal(mac, (MSTP_MAC_SLOTS_OFFSET + 1), NULL);
    mac = automac_free_address_mac(1);
    zassert_equal(mac, (MSTP_MAC_SLOTS_OFFSET + 2), NULL);
    mac = automac_free_address_mac(2);
    zassert_equal(mac, 126, NULL);
    mac = automac_free_address_random();
    options = (mac == (MSTP_MAC_SLOTS_OFFSET + 1)) ||
        (mac == (MSTP_MAC_SLOTS_OFFSET + 2)) || (mac == 126);
    zassert_true(options, NULL);
    /* test the stored address */
    mac = automac_address();
    zassert_true(mac < MSTP_MAC_SLOTS_MAX, NULL);
    automac_address_set(MSTP_MAC_SLOTS_OFFSET);
    mac = automac_address();
    zassert_equal(mac, MSTP_MAC_SLOTS_OFFSET, NULL);

    automac_init();
    automac_token_set(0x6B);
    mac = automac_next_station(0x25);
    zassert_equal(mac, 0x6B, NULL);
}

/**
 * @}
 */
void test_main(void)
{
    ztest_test_suite(automac_tests, ztest_unit_test(test_automac_init));

    ztest_run_test_suite(automac_tests);
}
