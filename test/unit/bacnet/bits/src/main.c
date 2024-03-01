/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/ztest.h>

#undef BIT
#undef _BV
#undef BIT_SET
#undef BIT_CLEAR
#undef BIT_FLIP
#undef BIT_CHECK
#undef BITMASK_SET
#undef BITMASK_CLEAR
#undef BITMASK_FLIP
#undef BITMASK_CHECK

#include "bacnet/bits.h"


/* NOTE: These tests must be compatible with C90, so they do not
 * verify support for ULL.
 */

static void test_BIT(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
	bitpos -= 1;
        zassert_true(BIT(bitpos) == (1U << bitpos), NULL);
    } while (bitpos > 0);
}

static void test__BV(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
	bitpos -= 1;
        zassert_true(BIT(bitpos) == (1U << bitpos), NULL);
    } while (bitpos > 0);
}

static void test_BIT_SET(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
        unsigned int a = 0U;
	bitpos -= 1;
	BIT_SET(a, bitpos);
        zassert_true(a == (1U << bitpos), NULL);
    } while (bitpos > 0);
}

static void test_BIT_CLEAR(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
        unsigned int a = ~0U;
	bitpos -= 1;
	BIT_CLEAR(a, bitpos);
        zassert_true(~(a) == (1U << bitpos), NULL);
    } while (bitpos > 0);
}

static void test_BIT_FLIP(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
        unsigned int a = ~0U;
	bitpos -= 1;
	BIT_FLIP(a, bitpos);
        zassert_true(a == ~(1U << bitpos), NULL);
	BIT_FLIP(a, bitpos);
        zassert_true(a == ~0U, NULL);

	a = 0U;
	BIT_FLIP(a, bitpos);
        zassert_true(a == (1U << bitpos), NULL);
	BIT_FLIP(a, bitpos);
        zassert_true(a == 0U, NULL);
	
    } while (bitpos > 0);
}

static void test_BIT_CHECK(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
        unsigned int a = ~0U;
	bitpos -= 1;
	zassert_true(BIT_CHECK(a, bitpos), NULL);

	a = 0U;
	zassert_false(BIT_CHECK(a, bitpos), NULL);
	
    } while (bitpos > 0);
}

static void test_BITMASK_SET(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
        unsigned int a = 0U;
	bitpos -= 1;
	BITMASK_SET(a, (1U << bitpos));
        zassert_true(a == (1U << bitpos), NULL);
    } while (bitpos > 0);
}

static void test_BITMASK_CLEAR(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
        unsigned int a = ~0U;
	bitpos -= 1;
	BITMASK_CLEAR(a, (1U << bitpos));
        zassert_true(~(a) == (1U << bitpos), NULL);
    } while (bitpos > 0);
}

static void test_BITMASK_FLIP(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
        unsigned int a = ~0U;
	bitpos -= 1;
	BITMASK_FLIP(a, (1U << bitpos));
        zassert_true(a == ~(1U << bitpos), NULL);
	BITMASK_FLIP(a, (1U << bitpos));
        zassert_true(a == ~0U, NULL);

	a = 0U;
	BITMASK_FLIP(a, (1U << bitpos));
        zassert_true(a == (1U << bitpos), NULL);
	BITMASK_FLIP(a, (1U << bitpos));
        zassert_true(a == 0U, NULL);
	
    } while (bitpos > 0);
}

static void test_BITMASK_CHECK(void)
{
    unsigned int bitpos = sizeof(unsigned int) * 8;

    do {
        unsigned int a = ~0U;
	bitpos -= 1;
	zassert_true(BITMASK_CHECK(a, (1U << bitpos)), NULL);

	a = 0U;
	zassert_false(BITMASK_CHECK(a, (1U << bitpos)), NULL);
	
    } while (bitpos > 0);
}

void test_main(void)
{
    ztest_test_suite(bacnet_bits,
     ztest_unit_test(test_BIT),
     ztest_unit_test(test__BV),
     ztest_unit_test(test_BIT_SET),
     ztest_unit_test(test_BIT_CLEAR),
     ztest_unit_test(test_BIT_FLIP),
     ztest_unit_test(test_BIT_CHECK),
     ztest_unit_test(test_BITMASK_SET),
     ztest_unit_test(test_BITMASK_CLEAR),
     ztest_unit_test(test_BITMASK_FLIP),
     ztest_unit_test(test_BITMASK_CHECK)
     );
    ztest_run_test_suite(bacnet_bits);
}
