/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/indtext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static INDTEXT_DATA data_list[] = { { 1, "Joshua" }, { 2, "Mary" },
    { 3, "Anna" }, { 4, "Christopher" }, { 5, "Patricia" }, { 0, NULL } };

static void testIndexText(void)
{
    unsigned i; /*counter */
    const char *pString;
    unsigned index;
    bool valid;
    unsigned count = 0;

    for (i = 0; i < 10; i++) {
        pString = indtext_by_index(data_list, i);
        if (pString) {
            count++;
            valid = indtext_by_string(data_list, pString, &index);
            zassert_true(valid, NULL);
            zassert_equal(index, i, NULL);
            zassert_equal(
                index, indtext_by_string_default(data_list, pString, index), NULL);
        }
    }
    zassert_equal(indtext_count(data_list), count, NULL);
    zassert_false(indtext_by_string(data_list, "Harry", NULL), NULL);
    zassert_false(indtext_by_string(data_list, NULL, NULL), NULL);
    zassert_false(indtext_by_string(NULL, NULL, NULL), NULL);
    zassert_is_null(indtext_by_index(data_list, 0), NULL);
    zassert_is_null(indtext_by_index(data_list, 10), NULL);
    zassert_is_null(indtext_by_index(NULL, 10), NULL);
    /* case insensitive versions */
    zassert_true(indtext_by_istring(data_list, "JOSHUA", NULL), NULL);
    zassert_true(indtext_by_istring(data_list, "joshua", NULL), NULL);
    valid = indtext_by_istring(data_list, "ANNA", &index);
    zassert_equal(
        index, indtext_by_istring_default(data_list, "ANNA", index), NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(indtext_tests,
     ztest_unit_test(testIndexText)
     );

    ztest_run_test_suite(indtext_tests);
}
