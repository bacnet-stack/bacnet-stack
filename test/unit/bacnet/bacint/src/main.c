/**
 * @file
 * @brief unit test for BACnet Integer encoding and decoding
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Greg Shue <greg.shue@outlook.com>
 * @date 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#include "bacnet/bacint.c"

#include <zephyr/ztest.h>
#include <string.h> // for memset(), memcpy
#include <limits.h>

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_unsigned16)
#else
static void test_unsigned16(void)
#endif
{
    uint8_t apdu[8] = { 0 };
    uint8_t test_apdu[8] = { 0 };
    uint16_t test_value = 0x1234U;
    uint16_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(2, encode_unsigned16(NULL, 0U), NULL);
    zassert_equal(2, decode_unsigned16(NULL, NULL), NULL);

    value = 42;
    zassert_equal(2, decode_unsigned16(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(2, decode_unsigned16(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(2, decode_unsigned16(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 8);
    test_apdu[3] = (uint8_t)(test_value >> 0);
    zassert_equal(2, encode_unsigned16(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(2, decode_unsigned16(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 8);
    test_apdu[3] = (uint8_t)(test_value >> 0);
    zassert_equal(2, encode_unsigned16(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(2, decode_unsigned16(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 8);
    test_apdu[4] = (uint8_t)(test_value >> 0);
    zassert_equal(2, encode_unsigned16(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(2, decode_unsigned16(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 8);
    test_apdu[4] = (uint8_t)(test_value >> 0);
    zassert_equal(2, encode_unsigned16(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(2, decode_unsigned16(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_unsigned24)
#else
static void test_unsigned24(void)
#endif
{
    uint8_t apdu[8] = { 0 };
    uint8_t test_apdu[8] = { 0 };
    uint32_t test_value = 0x123456UL;
    uint32_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(3, encode_unsigned24(NULL, 0U), NULL);
    zassert_equal(3, decode_unsigned24(NULL, NULL), NULL);

    value = 42;
    zassert_equal(3, decode_unsigned24(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(3, decode_unsigned24(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(3, decode_unsigned24(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 16);
    test_apdu[3] = (uint8_t)(test_value >> 8);
    test_apdu[4] = (uint8_t)(test_value >> 0);
    zassert_equal(3, encode_unsigned24(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(3, decode_unsigned24(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 16);
    test_apdu[3] = (uint8_t)(test_value >> 8);
    test_apdu[4] = (uint8_t)(test_value >> 0);
    zassert_equal(3, encode_unsigned24(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(3, decode_unsigned24(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 16);
    test_apdu[4] = (uint8_t)(test_value >> 8);
    test_apdu[5] = (uint8_t)(test_value >> 0);
    zassert_equal(3, encode_unsigned24(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(3, decode_unsigned24(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 16);
    test_apdu[4] = (uint8_t)(test_value >> 8);
    test_apdu[5] = (uint8_t)(test_value >> 0);
    zassert_equal(3, encode_unsigned24(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(3, decode_unsigned24(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_unsigned32)
#else
static void test_unsigned32(void)
#endif
{
    uint8_t apdu[8] = { 0 };
    uint8_t test_apdu[8] = { 0 };
    uint32_t test_value = 0x12345678UL;
    uint32_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(4, encode_unsigned32(NULL, 0U), NULL);
    zassert_equal(4, decode_unsigned32(NULL, NULL), NULL);

    value = 42;
    zassert_equal(4, decode_unsigned32(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(4, decode_unsigned32(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(4, decode_unsigned32(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 24);
    test_apdu[3] = (uint8_t)(test_value >> 16);
    test_apdu[4] = (uint8_t)(test_value >> 8);
    test_apdu[5] = (uint8_t)(test_value >> 0);
    zassert_equal(4, encode_unsigned32(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(4, decode_unsigned32(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 24);
    test_apdu[3] = (uint8_t)(test_value >> 16);
    test_apdu[4] = (uint8_t)(test_value >> 8);
    test_apdu[5] = (uint8_t)(test_value >> 0);
    zassert_equal(4, encode_unsigned32(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(4, decode_unsigned32(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 24);
    test_apdu[4] = (uint8_t)(test_value >> 16);
    test_apdu[5] = (uint8_t)(test_value >> 8);
    test_apdu[6] = (uint8_t)(test_value >> 0);
    zassert_equal(4, encode_unsigned32(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(4, decode_unsigned32(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 24);
    test_apdu[4] = (uint8_t)(test_value >> 16);
    test_apdu[5] = (uint8_t)(test_value >> 8);
    test_apdu[6] = (uint8_t)(test_value >> 0);
    zassert_equal(4, encode_unsigned32(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(4, decode_unsigned32(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_unsigned40)
#else
static void test_unsigned40(void)
#endif
{
#ifdef UINT64_MAX
    uint8_t apdu[12] = { 0 };
    uint8_t test_apdu[12] = { 0 };
    uint64_t test_value = 0x123456789AULL;
    uint64_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(5, encode_unsigned40(NULL, 0U), NULL);
    zassert_equal(5, decode_unsigned40(NULL, NULL), NULL);

    value = 42;
    zassert_equal(5, decode_unsigned40(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(5, decode_unsigned40(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(5, decode_unsigned40(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 32);
    test_apdu[3] = (uint8_t)(test_value >> 24);
    test_apdu[4] = (uint8_t)(test_value >> 16);
    test_apdu[5] = (uint8_t)(test_value >> 8);
    test_apdu[6] = (uint8_t)(test_value >> 0);
    zassert_equal(5, encode_unsigned40(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(5, decode_unsigned40(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 32);
    test_apdu[3] = (uint8_t)(test_value >> 24);
    test_apdu[4] = (uint8_t)(test_value >> 16);
    test_apdu[5] = (uint8_t)(test_value >> 8);
    test_apdu[6] = (uint8_t)(test_value >> 0);
    zassert_equal(5, encode_unsigned40(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(5, decode_unsigned40(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 32);
    test_apdu[4] = (uint8_t)(test_value >> 24);
    test_apdu[5] = (uint8_t)(test_value >> 16);
    test_apdu[6] = (uint8_t)(test_value >> 8);
    test_apdu[7] = (uint8_t)(test_value >> 0);
    zassert_equal(5, encode_unsigned40(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(5, decode_unsigned40(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 32);
    test_apdu[4] = (uint8_t)(test_value >> 24);
    test_apdu[5] = (uint8_t)(test_value >> 16);
    test_apdu[6] = (uint8_t)(test_value >> 8);
    test_apdu[7] = (uint8_t)(test_value >> 0);
    zassert_equal(5, encode_unsigned40(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(5, decode_unsigned40(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
#else
#if (defined(__ZEPHYR__) && defined(KERNEL))
    ztest_test_skip();
#else
    ztest_test_pass();
#endif
#endif /* defined(UINT64_MAX) */
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_unsigned48)
#else
static void test_unsigned48(void)
#endif
{
#ifdef UINT64_MAX
    uint8_t apdu[12] = { 0 };
    uint8_t test_apdu[12] = { 0 };
    uint64_t test_value = 0x123456789ABCULL;
    uint64_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(6, encode_unsigned48(NULL, 0U), NULL);
    zassert_equal(6, decode_unsigned48(NULL, NULL), NULL);

    value = 42;
    zassert_equal(6, decode_unsigned48(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(6, decode_unsigned48(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(6, decode_unsigned48(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 40);
    test_apdu[3] = (uint8_t)(test_value >> 32);
    test_apdu[4] = (uint8_t)(test_value >> 24);
    test_apdu[5] = (uint8_t)(test_value >> 16);
    test_apdu[6] = (uint8_t)(test_value >> 8);
    test_apdu[7] = (uint8_t)(test_value >> 0);
    zassert_equal(6, encode_unsigned48(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(6, decode_unsigned48(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 40);
    test_apdu[3] = (uint8_t)(test_value >> 32);
    test_apdu[4] = (uint8_t)(test_value >> 24);
    test_apdu[5] = (uint8_t)(test_value >> 16);
    test_apdu[6] = (uint8_t)(test_value >> 8);
    test_apdu[7] = (uint8_t)(test_value >> 0);
    zassert_equal(6, encode_unsigned48(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(6, decode_unsigned48(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 40);
    test_apdu[4] = (uint8_t)(test_value >> 32);
    test_apdu[5] = (uint8_t)(test_value >> 24);
    test_apdu[6] = (uint8_t)(test_value >> 16);
    test_apdu[7] = (uint8_t)(test_value >> 8);
    test_apdu[8] = (uint8_t)(test_value >> 0);
    zassert_equal(6, encode_unsigned48(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(6, decode_unsigned48(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 40);
    test_apdu[4] = (uint8_t)(test_value >> 32);
    test_apdu[5] = (uint8_t)(test_value >> 24);
    test_apdu[6] = (uint8_t)(test_value >> 16);
    test_apdu[7] = (uint8_t)(test_value >> 8);
    test_apdu[8] = (uint8_t)(test_value >> 0);
    zassert_equal(6, encode_unsigned48(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(6, decode_unsigned48(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
#else
#if (defined(__ZEPHYR__) && defined(KERNEL))
    ztest_test_skip();
#else
    ztest_test_pass();
#endif
#endif /* defined(UINT64_MAX) */
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_unsigned56)
#else
static void test_unsigned56(void)
#endif
{
#ifdef UINT64_MAX
    uint8_t apdu[12] = { 0 };
    uint8_t test_apdu[12] = { 0 };
    uint64_t test_value = 0x123456789ABCDEULL;
    uint64_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(7, encode_unsigned56(NULL, 0U), NULL);
    zassert_equal(7, decode_unsigned56(NULL, NULL), NULL);

    value = 42;
    zassert_equal(7, decode_unsigned56(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(7, decode_unsigned56(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(7, decode_unsigned56(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 48);
    test_apdu[3] = (uint8_t)(test_value >> 40);
    test_apdu[4] = (uint8_t)(test_value >> 32);
    test_apdu[5] = (uint8_t)(test_value >> 24);
    test_apdu[6] = (uint8_t)(test_value >> 16);
    test_apdu[7] = (uint8_t)(test_value >> 8);
    test_apdu[8] = (uint8_t)(test_value >> 0);
    zassert_equal(7, encode_unsigned56(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(7, decode_unsigned56(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 48);
    test_apdu[3] = (uint8_t)(test_value >> 40);
    test_apdu[4] = (uint8_t)(test_value >> 32);
    test_apdu[5] = (uint8_t)(test_value >> 24);
    test_apdu[6] = (uint8_t)(test_value >> 16);
    test_apdu[7] = (uint8_t)(test_value >> 8);
    test_apdu[8] = (uint8_t)(test_value >> 0);
    zassert_equal(7, encode_unsigned56(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(7, decode_unsigned56(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 48);
    test_apdu[4] = (uint8_t)(test_value >> 40);
    test_apdu[5] = (uint8_t)(test_value >> 32);
    test_apdu[6] = (uint8_t)(test_value >> 24);
    test_apdu[7] = (uint8_t)(test_value >> 16);
    test_apdu[8] = (uint8_t)(test_value >> 8);
    test_apdu[9] = (uint8_t)(test_value >> 0);
    zassert_equal(7, encode_unsigned56(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(7, decode_unsigned56(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 48);
    test_apdu[4] = (uint8_t)(test_value >> 40);
    test_apdu[5] = (uint8_t)(test_value >> 32);
    test_apdu[6] = (uint8_t)(test_value >> 24);
    test_apdu[7] = (uint8_t)(test_value >> 16);
    test_apdu[8] = (uint8_t)(test_value >> 8);
    test_apdu[9] = (uint8_t)(test_value >> 0);
    zassert_equal(7, encode_unsigned56(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(7, decode_unsigned56(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
#else
#if (defined(__ZEPHYR__) && defined(KERNEL))
    ztest_test_skip();
#else
    ztest_test_pass();
#endif
#endif /* defined(UINT64_MAX) */
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_unsigned64)
#else
static void test_unsigned64(void)
#endif
{
#ifdef UINT64_MAX
    uint8_t apdu[12] = { 0 };
    uint8_t test_apdu[12] = { 0 };
    uint64_t test_value = 0x123456789ABCDEF0ULL;
    uint64_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(8, encode_unsigned64(NULL, 0U), NULL);
    zassert_equal(8, decode_unsigned64(NULL, NULL), NULL);

    value = 42;
    zassert_equal(8, decode_unsigned64(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(8, decode_unsigned64(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(8, decode_unsigned64(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 56);
    test_apdu[3] = (uint8_t)(test_value >> 48);
    test_apdu[4] = (uint8_t)(test_value >> 40);
    test_apdu[5] = (uint8_t)(test_value >> 32);
    test_apdu[6] = (uint8_t)(test_value >> 24);
    test_apdu[7] = (uint8_t)(test_value >> 16);
    test_apdu[8] = (uint8_t)(test_value >> 8);
    test_apdu[9] = (uint8_t)(test_value >> 0);
    zassert_equal(8, encode_unsigned64(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(8, decode_unsigned64(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 56);
    test_apdu[3] = (uint8_t)(test_value >> 48);
    test_apdu[4] = (uint8_t)(test_value >> 40);
    test_apdu[5] = (uint8_t)(test_value >> 32);
    test_apdu[6] = (uint8_t)(test_value >> 24);
    test_apdu[7] = (uint8_t)(test_value >> 16);
    test_apdu[8] = (uint8_t)(test_value >> 8);
    test_apdu[9] = (uint8_t)(test_value >> 0);
    zassert_equal(8, encode_unsigned64(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(8, decode_unsigned64(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 56);
    test_apdu[4] = (uint8_t)(test_value >> 48);
    test_apdu[5] = (uint8_t)(test_value >> 40);
    test_apdu[6] = (uint8_t)(test_value >> 32);
    test_apdu[7] = (uint8_t)(test_value >> 24);
    test_apdu[8] = (uint8_t)(test_value >> 16);
    test_apdu[9] = (uint8_t)(test_value >> 8);
    test_apdu[10] = (uint8_t)(test_value >> 0);
    zassert_equal(8, encode_unsigned64(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(8, decode_unsigned64(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 56);
    test_apdu[4] = (uint8_t)(test_value >> 48);
    test_apdu[5] = (uint8_t)(test_value >> 40);
    test_apdu[6] = (uint8_t)(test_value >> 32);
    test_apdu[7] = (uint8_t)(test_value >> 24);
    test_apdu[8] = (uint8_t)(test_value >> 16);
    test_apdu[9] = (uint8_t)(test_value >> 8);
    test_apdu[10] = (uint8_t)(test_value >> 0);
    zassert_equal(8, encode_unsigned64(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(8, decode_unsigned64(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
#else
#if (defined(__ZEPHYR__) && defined(KERNEL))
    ztest_test_skip();
#else
    ztest_test_pass();
#endif
#endif /* defined(UINT64_MAX) */
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_unsigned_length)
#else
static void test_unsigned_length(void)
#endif
{
    zassert_equal(1, bacnet_unsigned_length(0), NULL);
    zassert_equal(1, bacnet_unsigned_length(0x7EUL), NULL);
    zassert_equal(1, bacnet_unsigned_length(0xFFUL), NULL);

    zassert_equal(2, bacnet_unsigned_length(1UL << 8), NULL);
    zassert_equal(2, bacnet_unsigned_length(0x7DUL << 8), NULL);
    zassert_equal(2, bacnet_unsigned_length(0xFFUL << 8), NULL);
    zassert_equal(2, bacnet_unsigned_length(0xFFFFUL), NULL);

    zassert_equal(3, bacnet_unsigned_length(1UL << 16), NULL);
    zassert_equal(3, bacnet_unsigned_length(0x7DUL << 16), NULL);
    zassert_equal(3, bacnet_unsigned_length(0xFFUL << 16), NULL);
    zassert_equal(3, bacnet_unsigned_length(0xFFFFFFUL), NULL);

    zassert_equal(4, bacnet_unsigned_length(1UL << 24), NULL);
    zassert_equal(4, bacnet_unsigned_length(0x7DUL << 24), NULL);
    zassert_equal(4, bacnet_unsigned_length(0xFFUL << 24), NULL);
    zassert_equal(4, bacnet_unsigned_length(0xFFFFFFFFUL), NULL);

#ifdef UINT64_MAX
    zassert_equal(5, bacnet_unsigned_length(1ULL << 32), NULL);
    zassert_equal(5, bacnet_unsigned_length(0x7DULL << 32), NULL);
    zassert_equal(5, bacnet_unsigned_length(0xFFULL << 32), NULL);
    zassert_equal(5, bacnet_unsigned_length(0xFFFFFFFFFFULL), NULL);

    zassert_equal(6, bacnet_unsigned_length(1ULL << 40), NULL);
    zassert_equal(6, bacnet_unsigned_length(0x7DULL << 40), NULL);
    zassert_equal(6, bacnet_unsigned_length(0xFFULL << 40), NULL);
    zassert_equal(6, bacnet_unsigned_length(0xFFFFFFFFFFFFULL), NULL);

    zassert_equal(7, bacnet_unsigned_length(1ULL << 48), NULL);
    zassert_equal(7, bacnet_unsigned_length(0x7DULL << 48), NULL);
    zassert_equal(7, bacnet_unsigned_length(0xFFULL << 48), NULL);
    zassert_equal(7, bacnet_unsigned_length(0xFFFFFFFFFFFFFFULL), NULL);

    zassert_equal(8, bacnet_unsigned_length(1ULL << 56), NULL);
    zassert_equal(8, bacnet_unsigned_length(0x7DULL << 56), NULL);
    zassert_equal(8, bacnet_unsigned_length(0xFFULL << 56), NULL);
    zassert_equal(8, bacnet_unsigned_length(0xFFFFFFFFFFFFFFFFULL), NULL);
#endif
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_signed8)
#else
static void test_signed8(void)
#endif
{
#if BACNET_USE_SIGNED
    uint8_t apdu[8] = { 0 };
    uint8_t test_apdu[8] = { 0 };
    int8_t test_value = 0x21;
    int32_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(1, encode_signed8(NULL, 0), NULL);
    zassert_equal(1, decode_signed8(NULL, NULL), NULL);

    value = 42;
    zassert_equal(1, decode_signed8(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(1, decode_signed8(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(1, decode_signed8(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 0);
    zassert_equal(1, encode_signed8(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(1, decode_signed8(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 0);
    zassert_equal(1, encode_signed8(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(1, decode_signed8(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 0);
    zassert_equal(1, encode_signed8(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(1, decode_signed8(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 0);
    zassert_equal(1, encode_signed8(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(1, decode_signed8(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
#else
#if (defined(__ZEPHYR__) && defined(KERNEL))
    ztest_test_skip();
#else
    ztest_test_pass();
#endif
#endif /* BACNET_USE_SIGNED */
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_signed16)
#else
static void test_signed16(void)
#endif
{
#if BACNET_USE_SIGNED
    uint8_t apdu[8] = { 0 };
    uint8_t test_apdu[8] = { 0 };
    int16_t test_value = 0x4321;
    int32_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(2, encode_signed16(NULL, 0U), NULL);
    zassert_equal(2, decode_signed16(NULL, NULL), NULL);

    value = 42;
    zassert_equal(2, decode_signed16(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(2, decode_signed16(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(2, decode_signed16(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 8);
    test_apdu[3] = (uint8_t)(test_value >> 0);
    zassert_equal(2, encode_signed16(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(2, decode_signed16(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 8);
    test_apdu[3] = (uint8_t)(test_value >> 0);
    zassert_equal(2, encode_signed16(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(2, decode_signed16(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 8);
    test_apdu[4] = (uint8_t)(test_value >> 0);
    zassert_equal(2, encode_signed16(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(2, decode_signed16(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 8);
    test_apdu[4] = (uint8_t)(test_value >> 0);
    zassert_equal(2, encode_signed16(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(2, decode_signed16(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
#else
    ztest_test_skip();
#endif /* BACNET_USE_SIGNED */
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_signed24)
#else
static void test_signed24(void)
#endif
{
#if BACNET_USE_SIGNED
    uint8_t apdu[8] = { 0 };
    uint8_t test_apdu[8] = { 0 };
    int32_t test_value = 0x123456L;
    int32_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(3, encode_signed24(NULL, 0U), NULL);
    zassert_equal(3, decode_signed24(NULL, NULL), NULL);

    value = 42;
    zassert_equal(3, decode_signed24(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(3, decode_signed24(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(3, decode_signed24(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 16);
    test_apdu[3] = (uint8_t)(test_value >> 8);
    test_apdu[4] = (uint8_t)(test_value >> 0);
    zassert_equal(3, encode_signed24(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(3, decode_signed24(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 16);
    test_apdu[3] = (uint8_t)(test_value >> 8);
    test_apdu[4] = (uint8_t)(test_value >> 0);
    zassert_equal(3, encode_signed24(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(3, decode_signed24(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 16);
    test_apdu[4] = (uint8_t)(test_value >> 8);
    test_apdu[5] = (uint8_t)(test_value >> 0);
    zassert_equal(3, encode_signed24(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(3, decode_signed24(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 16);
    test_apdu[4] = (uint8_t)(test_value >> 8);
    test_apdu[5] = (uint8_t)(test_value >> 0);
    zassert_equal(3, encode_signed24(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(3, decode_signed24(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
#else
    ztest_test_skip();
#endif /* BACNET_USE_SIGNED */
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_signed32)
#else
static void test_signed32(void)
#endif
{
#if BACNET_USE_SIGNED
    uint8_t apdu[8] = { 0 };
    uint8_t test_apdu[8] = { 0 };
    int32_t test_value = 0x12345678L;
    int32_t value = 0;

    /*
     * Verify invalid parameter detection and handling
     */
    zassert_equal(4, encode_signed32(NULL, 0U), NULL);
    zassert_equal(4, decode_signed32(NULL, NULL), NULL);

    value = 42;
    zassert_equal(4, decode_signed32(NULL, &value), NULL);
    zassert_equal(42, value, NULL);

    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(4, decode_signed32(&apdu[2], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    memset(test_apdu, 0xA5U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    zassert_equal(4, decode_signed32(&apdu[3], NULL), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);

    /*
     * Verify multi-byte write in correct order
     */

    /* Verify aligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 24);
    test_apdu[3] = (uint8_t)(test_value >> 16);
    test_apdu[4] = (uint8_t)(test_value >> 8);
    test_apdu[5] = (uint8_t)(test_value >> 0);
    zassert_equal(4, encode_signed32(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(4, decode_signed32(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[2] = (uint8_t)(test_value >> 24);
    test_apdu[3] = (uint8_t)(test_value >> 16);
    test_apdu[4] = (uint8_t)(test_value >> 8);
    test_apdu[5] = (uint8_t)(test_value >> 0);
    zassert_equal(4, encode_signed32(&apdu[2], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(4, decode_signed32(&apdu[2], &value), NULL);
    zassert_equal(test_value, value, NULL);

    /* Verify unaligned access with no extra bytes written */
    memset(test_apdu, ~0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 24);
    test_apdu[4] = (uint8_t)(test_value >> 16);
    test_apdu[5] = (uint8_t)(test_value >> 8);
    test_apdu[6] = (uint8_t)(test_value >> 0);
    zassert_equal(4, encode_signed32(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(4, decode_signed32(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);

    memset(test_apdu, 0U, sizeof(test_apdu));
    memcpy(apdu, test_apdu, sizeof(test_apdu));
    test_apdu[3] = (uint8_t)(test_value >> 24);
    test_apdu[4] = (uint8_t)(test_value >> 16);
    test_apdu[5] = (uint8_t)(test_value >> 8);
    test_apdu[6] = (uint8_t)(test_value >> 0);
    zassert_equal(4, encode_signed32(&apdu[3], test_value), NULL);
    zassert_mem_equal(test_apdu, apdu, sizeof(test_apdu), NULL);
    zassert_equal(4, decode_signed32(&apdu[3], &value), NULL);
    zassert_equal(test_value, value, NULL);
#else
    ztest_test_skip();
#endif /* BACNET_USE_SIGNED */
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_bacint, test_signed_length)
#else
static void test_signed_length(void)
#endif
{
#if BACNET_USE_SIGNED
    const int32_t sint24_max = 0x7FFFFFL;
    const int32_t sint24_min = -sint24_max - 1L;

    zassert_equal(1, bacnet_signed_length(CHAR_MIN), NULL);
    zassert_equal(1, bacnet_signed_length(-1), NULL);
    zassert_equal(1, bacnet_signed_length(0), NULL);
    zassert_equal(1, bacnet_signed_length(1), NULL);
    zassert_equal(1, bacnet_signed_length(CHAR_MAX), NULL);

    zassert_equal(2, bacnet_signed_length(SHRT_MIN), NULL);
    zassert_equal(2, bacnet_signed_length(CHAR_MIN - 1L), NULL);
    zassert_equal(2, bacnet_signed_length(CHAR_MAX + 1L), NULL);
    zassert_equal(2, bacnet_signed_length(SHRT_MAX), NULL);

    zassert_equal(3, bacnet_signed_length(sint24_min), NULL);
    zassert_equal(3, bacnet_signed_length(SHRT_MIN - 1L), NULL);
    zassert_equal(3, bacnet_signed_length(SHRT_MAX + 1L), NULL);
    zassert_equal(3, bacnet_signed_length(sint24_max), NULL);

    zassert_equal(4, bacnet_signed_length(INT_MIN), NULL);
    zassert_equal(4, bacnet_signed_length(sint24_min - 1L), NULL);
    zassert_equal(4, bacnet_signed_length(sint24_max + 1L), NULL);
    zassert_equal(4, bacnet_signed_length(INT_MAX), NULL);
#else
    ztest_test_skip();
#endif /* BACNET_USE_SIGNED */
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST_SUITE(bacnet_bacint, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacnet_bacint, ztest_unit_test(test_unsigned16),
        ztest_unit_test(test_unsigned24), ztest_unit_test(test_unsigned32),
        ztest_unit_test(test_unsigned40), ztest_unit_test(test_unsigned48),
        ztest_unit_test(test_unsigned56), ztest_unit_test(test_unsigned64),
        ztest_unit_test(test_unsigned_length), ztest_unit_test(test_signed8),
        ztest_unit_test(test_signed16), ztest_unit_test(test_signed24),
        ztest_unit_test(test_signed32), ztest_unit_test(test_signed_length));
    ztest_run_test_suite(bacnet_bacint);
}
#endif
