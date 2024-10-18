/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacint.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API for unsigned 16b integers
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetUnsigned16)
#else
static void testBACnetUnsigned16(void)
#endif
{
    uint8_t apdu[32] = { 0 };
    uint16_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_unsigned16(&apdu[0], NULL);
    zassert_equal(len, 2, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_unsigned16(&apdu[0], value);
    zassert_equal(len, 2, NULL);
    test_value = (uint16_t)~0U;
    len = decode_unsigned16(&apdu[0], &test_value);
    zassert_equal(len, 2, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = 1; value != 0; value = (value << 1) & ((1UL << 16) - 1)) {
        len = encode_unsigned16(&apdu[0], value);
        zassert_equal(len, 2, NULL);
        test_value = (uint16_t)~0U;
        len = decode_unsigned16(&apdu[0], &test_value);
        zassert_equal(value, test_value, NULL);
    }
}

/**
 * @brief Test encode/decode API for unsigned 24b integers
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetUnsigned24)
#else
static void testBACnetUnsigned24(void)
#endif
{
    uint8_t apdu[32] = { 0 };
    uint32_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_unsigned24(&apdu[0], NULL);
    zassert_equal(len, 3, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_unsigned24(&apdu[0], value);
    zassert_equal(len, 3, NULL);
    test_value = ~0U;
    len = decode_unsigned24(&apdu[0], &test_value);
    zassert_equal(len, 3, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = 1; value != 0; value = (value << 1) & ((1UL << 24) - 1)) {
        len = encode_unsigned24(&apdu[0], value);
        zassert_equal(len, 3, NULL);
        test_value = ~0U;
        len = decode_unsigned24(&apdu[0], &test_value);
        zassert_equal(value, test_value, NULL);
    }
}

/**
 * @brief Test encode/decode API for unsigned 32b integers
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetUnsigned32)
#else
static void testBACnetUnsigned32(void)
#endif
{
    uint8_t apdu[32] = { 0 };
    uint32_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_unsigned32(&apdu[0], NULL);
    zassert_equal(len, 4, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_unsigned32(&apdu[0], value);
    zassert_equal(len, 4, NULL);
    test_value = ~0U;
    len = decode_unsigned32(&apdu[0], &test_value);
    zassert_equal(len, 4, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = 0x01; value != 0; value <<= 1) {
        len = encode_unsigned32(&apdu[0], value);
        zassert_equal(len, 4, NULL);
        test_value = ~0U;
        len = decode_unsigned32(&apdu[0], &test_value);
        zassert_equal(value, test_value, NULL);
    }
}

/**
 * @brief Test encode/decode API for unsigned 40b integers (if 64b supported)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetUnsigned40)
#else
static void testBACnetUnsigned40(void)
#endif
{
#ifdef UINT64_MAX
    uint8_t apdu[64] = { 0 };
    uint64_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_unsigned40(&apdu[0], NULL);
    zassert_equal(len, 5, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_unsigned40(&apdu[0], value);
    zassert_equal(len, 5, NULL);
    test_value = ~0ULL;
    len = decode_unsigned40(&apdu[0], &test_value);
    zassert_equal(len, 5, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = 1; value != 0; value = (value << 1) & ((1ULL << 40) - 1)) {
        len = encode_unsigned40(&apdu[0], value);
        zassert_equal(len, 5, NULL);
        test_value = ~0ULL;
        len = decode_unsigned40(&apdu[0], &test_value);
        zassert_equal(len, 5, NULL);
        zassert_equal(value, test_value, NULL);
    }
#else
#warning "UINT64_MAX not supported!"
    ztest_test_skip();
#endif
}

/**
 * @brief Test encode/decode API for unsigned 48b integers (if 64b supported)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetUnsigned48)
#else
static void testBACnetUnsigned48(void)
#endif
{
#ifdef UINT64_MAX
    uint8_t apdu[64] = { 0 };
    uint64_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_unsigned48(&apdu[0], NULL);
    zassert_equal(len, 6, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_unsigned48(&apdu[0], value);
    zassert_equal(len, 6, NULL);
    test_value = ~0ULL;
    len = decode_unsigned48(&apdu[0], &test_value);
    zassert_equal(len, 6, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = 1; value != 0; value = (value << 1) & ((1ULL << 48) - 1)) {
        len = encode_unsigned48(&apdu[0], value);
        zassert_equal(len, 6, NULL);
        test_value = ~0ULL;
        len = decode_unsigned48(&apdu[0], &test_value);
        zassert_equal(len, 6, NULL);
        zassert_equal(value, test_value, NULL);
    }
#else
#warning "UINT64_MAX not supported!"
    ztest_test_skip();
#endif
}

/**
 * @brief Test encode/decode API for unsigned 56b integers (if 64b supported)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetUnsigned56)
#else
static void testBACnetUnsigned56(void)
#endif
{
#ifdef UINT64_MAX
    uint8_t apdu[64] = { 0 };
    uint64_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_unsigned56(&apdu[0], NULL);
    zassert_equal(len, 7, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_unsigned56(&apdu[0], value);
    zassert_equal(len, 7, NULL);
    test_value = ~0ULL;
    len = decode_unsigned56(&apdu[0], &test_value);
    zassert_equal(len, 7, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = 1; value != 0; value = (value << 1) & ((1ULL << 56) - 1)) {
        len = encode_unsigned56(&apdu[0], value);
        zassert_equal(len, 7, NULL);
        test_value = ~0ULL;
        len = decode_unsigned56(&apdu[0], &test_value);
        zassert_equal(len, 7, NULL);
        zassert_equal(value, test_value, NULL);
    }
#else
#warning "UINT64_MAX not supported!"
    ztest_test_skip();
#endif
}

/**
 * @brief Test encode/decode API for unsigned 64b integers (if 64b supported)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetUnsigned64)
#else
static void testBACnetUnsigned64(void)
#endif
{
#ifdef UINT64_MAX
    uint8_t apdu[64] = { 0 };
    uint64_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_unsigned64(&apdu[0], NULL);
    zassert_equal(len, 8, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_unsigned64(&apdu[0], value);
    zassert_equal(len, 8, NULL);
    test_value = ~0ULL;
    len = decode_unsigned64(&apdu[0], &test_value);
    zassert_equal(len, 8, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = 0x01; value != 0; value <<= 1) {
        len = encode_unsigned64(&apdu[0], value);
        zassert_equal(len, 8, NULL);
        test_value = ~0ULL;
        len = decode_unsigned64(&apdu[0], &test_value);
        zassert_equal(len, 8, NULL);
        zassert_equal(value, test_value, NULL);
    }
#else
#warning "UINT64_MAX not supported!"
    ztest_test_skip();
#endif
}

/**
 * @brief Test calculation of unsigned length
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetUnsignedLength)
#else
static void testBACnetUnsignedLength(void)
#endif
{
    BACNET_UNSIGNED_INTEGER value = 0;
    int len = 0;
    int bits = (8 * sizeof(BACNET_UNSIGNED_INTEGER));

    for (value = ~value; value > 0; value >>= 1) {
        len = bacnet_unsigned_length(value);
        zassert_equal(len, (bits + 7) / 8, NULL);
        bits -= 1;
    }
    len = bacnet_unsigned_length(0);
    zassert_equal(len, 1, NULL);
}

/**
 * @brief Test encode/decode API for signed 8b integers
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetSigned8)
#else
static void testBACnetSigned8(void)
#endif
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_signed8(&apdu[0], NULL);
    zassert_equal(len, 1, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_signed8(&apdu[0], value);
    zassert_equal(len, 1, NULL);
    test_value = ~0U;
    len = decode_signed8(&apdu[0], &test_value);
    zassert_equal(len, 1, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = -127;; value++) {
        len = encode_signed8(&apdu[0], value);
        zassert_equal(len, 1, NULL);
        test_value = ~0U;
        len = decode_signed8(&apdu[0], &test_value);
        zassert_equal(value, test_value, NULL);
        if (value == 127) {
            break;
        }
    }
}

/**
 * @brief Test encode/decode API for signed 16b integers
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetSigned16)
#else
static void testBACnetSigned16(void)
#endif
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_signed16(&apdu[0], NULL);
    zassert_equal(len, 2, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_signed16(&apdu[0], value);
    zassert_equal(len, 2, NULL);
    test_value = ~0U;
    len = decode_signed16(&apdu[0], &test_value);
    zassert_equal(len, 2, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = -32767;; value++) {
        len = encode_signed16(&apdu[0], value);
        zassert_equal(len, 2, NULL);
        test_value = ~0U;
        len = decode_signed16(&apdu[0], &test_value);
        zassert_equal(value, test_value, NULL);
        if (value == 32767) {
            break;
        }
    }
}

/**
 * @brief Test encode/decode API for signed 24b integers
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetSigned24)
#else
static void testBACnetSigned24(void)
#endif
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_signed24(&apdu[0], NULL);
    zassert_equal(len, 3, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_signed24(&apdu[0], value);
    zassert_equal(len, 3, NULL);
    test_value = ~0U;
    len = decode_signed24(&apdu[0], &test_value);
    zassert_equal(len, 3, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = -8388607; value <= 8388607; value += 15) {
        len = encode_signed24(&apdu[0], value);
        zassert_equal(len, 3, NULL);
        test_value = ~0U;
        len = decode_signed24(&apdu[0], &test_value);
        zassert_equal(value, test_value, NULL);
    }
}

/**
 * @brief Test encode/decode API for signed 32b integers
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacint_tests, testBACnetSigned32)
#else
static void testBACnetSigned32(void)
#endif
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0;

    /* Test invalid parameter values */
    len = decode_signed32(&apdu[0], NULL);
    zassert_equal(len, 4, NULL); /* TODO: How do we indicate an error? */

    /* Test value zero (0) */
    memset(apdu, -1, sizeof(apdu));
    len = encode_signed32(&apdu[0], value);
    zassert_equal(len, 4, NULL);
    test_value = ~0U;
    len = decode_signed32(&apdu[0], &test_value);
    zassert_equal(len, 4, NULL);
    zassert_equal(value, test_value, NULL);

    for (value = -2147483647; value < 0; value += 127) {
        len = encode_signed32(&apdu[0], value);
        zassert_equal(len, 4, NULL);
        test_value = ~0U;
        len = decode_signed32(&apdu[0], &test_value);
        zassert_equal(value, test_value, NULL);
    }
    for (value = 2147483647; value > 0; value -= 127) {
        len = encode_signed32(&apdu[0], value);
        zassert_equal(len, 4, NULL);
        test_value = ~0U;
        len = decode_signed32(&apdu[0], &test_value);
        zassert_equal(value, test_value, NULL);
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacint_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacint_tests, ztest_unit_test(testBACnetUnsigned16),
        ztest_unit_test(testBACnetUnsigned24),
        ztest_unit_test(testBACnetUnsigned32),
        ztest_unit_test(testBACnetUnsigned40),
        ztest_unit_test(testBACnetUnsigned48),
        ztest_unit_test(testBACnetUnsigned56),
        ztest_unit_test(testBACnetUnsigned64),
        ztest_unit_test(testBACnetUnsignedLength),
        ztest_unit_test(testBACnetSigned8), ztest_unit_test(testBACnetSigned16),
        ztest_unit_test(testBACnetSigned24),
        ztest_unit_test(testBACnetSigned32));

    ztest_run_test_suite(bacint_tests);
}
#endif
