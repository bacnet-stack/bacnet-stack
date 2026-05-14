/**
 * @file
 * @brief test BACnet state name utility APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/state_name.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static const char State_Names[] = "State 1\0"
                                  "State 2\0"
                                  "State 3\0";

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(state_name_tests, testStateNameCount)
#else
static void testStateNameCount(void)
#endif
{
    unsigned count = 0;

    count = state_name_count(NULL);
    zassert_equal(count, 0, NULL);

    count = state_name_count("");
    zassert_equal(count, 0, NULL);

    count = state_name_count(State_Names);
    zassert_equal(count, 3, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(state_name_tests, testStateNameByIndex)
#else
static void testStateNameByIndex(void)
#endif
{
    const char *name = NULL;

    name = state_name_by_index(NULL, 1);
    zassert_is_null(name, NULL);

    name = state_name_by_index(State_Names, 0);
    zassert_is_null(name, NULL);

    name = state_name_by_index(State_Names, 1);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, "State 1"), 0, NULL);

    name = state_name_by_index(State_Names, 2);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, "State 2"), 0, NULL);

    name = state_name_by_index(State_Names, 3);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, "State 3"), 0, NULL);

    name = state_name_by_index(State_Names, 4);
    zassert_is_null(name, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(state_name_tests, testStateNameFromIndex)
#else
static void testStateNameFromIndex(void)
#endif
{
    unsigned index = 0;

    index = state_name_to_index(NULL, "State 1");
    zassert_equal(index, 0, NULL);

    index = state_name_to_index(State_Names, NULL);
    zassert_equal(index, 0, NULL);

    index = state_name_to_index(State_Names, "State 2");
    zassert_equal(index, 2, NULL);

    index = state_name_to_index(State_Names, "State 3");
    zassert_equal(index, 3, NULL);

    index = state_name_to_index(State_Names, "Missing State");
    zassert_equal(index, 0, NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(state_name_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        state_name_tests, ztest_unit_test(testStateNameCount),
        ztest_unit_test(testStateNameByIndex),
        ztest_unit_test(testStateNameFromIndex));

    ztest_run_test_suite(state_name_tests);
}
#endif
