/**
 * @file
 * @brief Tests for BACnet/IP subnet prefix caching on Linux
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/datalink/bip.h>

static void test_prefix_defaults_to_zero(void)
{
    bip_cleanup();
    zassert_equal(
        bip_get_subnet_prefix(), 0, "Prefix should be zero by default");
}

static void test_prefix_roundtrip(void)
{
    bip_cleanup();
    zassert_true(bip_set_subnet_prefix(24), NULL);
    zassert_equal(bip_get_subnet_prefix(), 24, NULL);

    zassert_true(bip_set_subnet_prefix(16), NULL);
    zassert_equal(bip_get_subnet_prefix(), 16, NULL);
}

static void test_prefix_invalid_values(void)
{
    bip_cleanup();
    zassert_false(bip_set_subnet_prefix(0), NULL);
    zassert_false(bip_set_subnet_prefix(33), NULL);
    zassert_equal(bip_get_subnet_prefix(), 0, NULL);
}

static void test_prefix_all_bits_set(void)
{
    bip_cleanup();
    zassert_true(bip_set_subnet_prefix(32), NULL);
    zassert_equal(bip_get_subnet_prefix(), 32, NULL);
}

void test_main(void)
{
    ztest_test_suite(
        bip_subnet_test, ztest_unit_test(test_prefix_defaults_to_zero),
        ztest_unit_test(test_prefix_roundtrip),
        ztest_unit_test(test_prefix_invalid_values),
        ztest_unit_test(test_prefix_all_bits_set));
    ztest_run_test_suite(bip_subnet_test);
}
