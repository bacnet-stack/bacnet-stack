/**
 * @file
 * @brief Unit test for BACNET_ADDRESS copy, init, compare
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2023
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacaddr.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_address_tests, test_BACNET_ADDRESS)
#else
static void test_BACNET_ADDRESS(void)
#endif
{
    BACNET_ADDRESS src = { 0 }, dest = { 0 };
    BACNET_ADDRESS test_src = { 0 }, test_dest = { 0 };
    BACNET_MAC_ADDRESS mac = { 0 }, adr = { 0 };
    uint16_t dnet = 0;
    bool status = false;

    /* invalid parameters */
    status = bacnet_address_same(NULL, &dest);
    zassert_false(status, NULL);
    status = bacnet_address_same(&dest, NULL);
    zassert_false(status, NULL);
    status = bacnet_address_same(NULL, NULL);
    zassert_false(status, NULL);

    /* happy day cases */
    status = bacnet_address_same(&dest, &dest);
    zassert_true(status, NULL);
    bacnet_address_copy(&dest, &src);
    status = bacnet_address_same(&dest, &src);
    zassert_true(status, NULL);
    status = bacnet_address_init(&dest, &mac, dnet, &adr);
    zassert_true(status, NULL);
    status = bacnet_address_init(&src, &mac, dnet, &adr);
    zassert_true(status, NULL);
    status = bacnet_address_same(&dest, &src);
    zassert_true(status, NULL);
    /* remote dnet is non-zero */
    dnet = 1;
    status = bacnet_address_init(&dest, &mac, dnet, &adr);
    zassert_true(status, NULL);
    status = bacnet_address_init(&src, &mac, dnet, &adr);
    zassert_true(status, NULL);
    status = bacnet_address_same(&dest, &src);
    zassert_true(status, NULL);
    /* different src->len */
    bacnet_address_copy(&dest, &src);
    src.len = 4;
    status = bacnet_address_same(&dest, &src);
    zassert_false(status, NULL);
    /* large src->len */
    src.len = MAX_MAC_LEN;
    status = bacnet_address_same(&dest, &src);
    zassert_false(status, NULL);
    bacnet_address_copy(&dest, &src);
    /* different src->dnet */
    dnet = 12;
    status = bacnet_address_init(&src, &mac, dnet, &adr);
    zassert_true(status, NULL);
    status = bacnet_address_same(&dest, &src);
    zassert_false(status, NULL);
    /* init checking */
    status = bacnet_address_init(NULL, &mac, dnet, &adr);
    zassert_false(status, NULL);
    mac.len = MAX_MAC_LEN;
    status = bacnet_address_init(&dest, &mac, dnet, &adr);
    zassert_true(status, NULL);
    adr.len = MAX_MAC_LEN;
    status = bacnet_address_init(&dest, &mac, dnet, &adr);
    zassert_true(status, NULL);
    /* remote dnet adr is different */
    dnet = 1;
    status = bacnet_address_init(&dest, &mac, dnet, &adr);
    zassert_true(status, NULL);
    status = bacnet_address_init(&src, &mac, dnet, &adr);
    src.adr[MAX_MAC_LEN - 1] = 1;
    status = bacnet_address_same(&dest, &src);
    zassert_false(status, NULL);
    /* mac_len is different */
    dnet = 0;
    status = bacnet_address_init(&dest, &mac, dnet, &adr);
    zassert_true(status, NULL);
    status = bacnet_address_init(&src, &mac, dnet, &adr);
    src.mac_len = MAX_MAC_LEN;
    dest.mac_len = MAX_MAC_LEN;
    status = bacnet_address_same(&dest, &src);
    zassert_true(status, NULL);
    dest.mac_len = 1;
    status = bacnet_address_same(&dest, &src);
    zassert_false(status, NULL);
    /* only setting a DNET address */
    dnet = 1234;
    status = bacnet_address_init(&dest, NULL, dnet, NULL);
    zassert_true(status, NULL);
    status = bacnet_address_init(&src, NULL, dnet, NULL);
    zassert_true(status, NULL);
    status = bacnet_address_same(&dest, &src);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_address_tests, test_BACNET_MAC_ADDRESS)
#else
static void test_BACNET_MAC_ADDRESS(void)
#endif
{
    BACNET_MAC_ADDRESS dest = { 0 }, src = { 0 };
    uint8_t adr[MAX_MAC_LEN] = { 0 };
    uint8_t len = MAX_MAC_LEN;
    bool status = false;
    const char *bip_ascii = "255.255.255.255:47808";
    const char *bip_ascii_no_port = "255.255.255.255";
    const char *ethernet_ascii = "f0:f1:f2:f3:f4:f5";
    const char *mstp_ascii = "7F";
    const char *vmac_ascii = "12:34:56";

    bacnet_address_mac_init(&src, adr, len);
    bacnet_address_mac_init(&dest, adr, len);
    status = bacnet_address_mac_same(&dest, &src);
    zassert_true(status, NULL);

    status = bacnet_address_mac_same(NULL, &src);
    zassert_false(status, NULL);
    status = bacnet_address_mac_same(&dest, NULL);
    zassert_false(status, NULL);

    bacnet_address_mac_init(&src, NULL, 0);
    bacnet_address_mac_init(&dest, NULL, 0);
    status = bacnet_address_mac_same(&dest, &src);
    zassert_true(status, NULL);

    bacnet_address_mac_init(&src, adr, 1);
    bacnet_address_mac_init(&dest, adr, 2);
    status = bacnet_address_mac_same(&dest, &src);
    zassert_false(status, NULL);

    status = bacnet_address_mac_from_ascii(&dest, bip_ascii);
    zassert_true(status, NULL);
    zassert_equal(dest.len, 6, NULL);
    zassert_equal(dest.adr[0], 255, NULL);
    zassert_equal(dest.adr[1], 255, NULL);
    zassert_equal(dest.adr[2], 255, NULL);
    zassert_equal(dest.adr[3], 255, NULL);
    status = bacnet_address_mac_from_ascii(&dest, bip_ascii_no_port);
    zassert_true(status, NULL);
    status = bacnet_address_mac_from_ascii(&dest, ethernet_ascii);
    zassert_equal(dest.len, 6, NULL);
    zassert_equal(dest.adr[0], 0xf0, NULL);
    zassert_equal(dest.adr[1], 0xf1, NULL);
    zassert_equal(dest.adr[2], 0xf2, NULL);
    zassert_equal(dest.adr[3], 0xf3, NULL);
    zassert_equal(dest.adr[4], 0xf4, NULL);
    zassert_equal(dest.adr[5], 0xf5, NULL);
    zassert_true(status, NULL);
    status = bacnet_address_mac_from_ascii(&dest, mstp_ascii);
    zassert_true(status, NULL);
    zassert_equal(dest.len, 1, NULL);
    zassert_equal(dest.adr[0], 0x7f, NULL);
    status = bacnet_address_mac_from_ascii(&dest, vmac_ascii);
    zassert_true(status, NULL);
    zassert_equal(dest.len, 3, NULL);
    zassert_equal(dest.adr[0], 0x12, NULL);
    zassert_equal(dest.adr[1], 0x34, NULL);
    zassert_equal(dest.adr[2], 0x56, NULL);
    /* different MAC of same len */
    status = bacnet_address_mac_from_ascii(&src, vmac_ascii);
    zassert_true(status, NULL);
    dest.adr[1] = 0x11;
    dest.adr[2] = 0x22;
    dest.adr[3] = 0x33;
    status = bacnet_address_mac_same(&dest, &src);
    zassert_false(status, NULL);
    /* NULL parameters */
    status = bacnet_address_mac_from_ascii(NULL, vmac_ascii);
    zassert_false(status, NULL);
    status = bacnet_address_mac_from_ascii(&dest, NULL);
    zassert_false(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_address_tests, test_BACnetAddress_Codec)
#else
static void test_BACnetAddress_Codec(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    BACNET_ADDRESS value = { 0 }, test_value = { 0 };
    uint8_t tag_number, wrong_tag_number;
    int len, test_len;

    value.mac[0] = 1;
    value.mac[1] = 2;
    value.mac[2] = 3;
    value.mac[3] = 4;
    value.mac[4] = 0xba;
    value.mac[5] = 0xc0;
    value.mac_len = 6;
    value.net = 0;
    len = encode_bacnet_address(NULL, &value);
    test_len = encode_bacnet_address(apdu, &value);
    zassert_true(len > 0, NULL);
    zassert_true(test_len > 0, NULL);
    zassert_equal(len, test_len, "len=%d test_len=%d", len, test_len);
    test_len = bacnet_address_decode(apdu, sizeof(apdu), &test_value);
    zassert_equal(len, test_len, NULL);
    zassert_equal(value.net, test_value.net, NULL);
    zassert_equal(value.mac_len, test_value.mac_len, NULL);
    zassert_equal(value.mac_len, 6, NULL);
    test_len = bacnet_address_decode(apdu, sizeof(apdu), NULL);
    zassert_equal(len, test_len, NULL);
    tag_number = 1;
    len = encode_context_bacnet_address(NULL, tag_number, &value);
    test_len = encode_context_bacnet_address(apdu, tag_number, &value);
    zassert_true(len > 0, NULL);
    zassert_true(test_len > 0, NULL);
    zassert_equal(len, test_len, NULL);
    test_len = bacnet_address_context_decode(
        apdu, sizeof(apdu), tag_number, &test_value);
    zassert_equal(len, test_len, NULL);
    zassert_equal(value.net, test_value.net, NULL);
    zassert_equal(value.mac_len, test_value.mac_len, NULL);
    zassert_equal(value.mac_len, 6, NULL);
    test_len = bacnet_address_context_decode(
        apdu, sizeof(apdu), tag_number, &test_value);
    /* negative tests - NULL value */
    test_len =
        bacnet_address_context_decode(apdu, sizeof(apdu), tag_number, NULL);
    zassert_equal(len, test_len, NULL);
    /* negative tests - wrong tag number */
    wrong_tag_number = 4;
    test_len = bacnet_address_context_decode(
        apdu, sizeof(apdu), wrong_tag_number, &test_value);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    /* negative tests - apdu too short */
    while (len) {
        len--;
        test_len = bacnet_address_context_decode(apdu, len, tag_number, NULL);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    }
}

/**
 * @}
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacnet_address_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacnet_address_tests, ztest_unit_test(test_BACNET_ADDRESS),
        ztest_unit_test(test_BACNET_MAC_ADDRESS),
        ztest_unit_test(test_BACnetAddress_Codec));

    ztest_run_test_suite(bacnet_address_tests);
}
#endif
