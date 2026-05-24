/**
 * @file
 * @brief test BACnet state name utility APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <stdlib.h>
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/basic/sys/keylist.h>
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
ZTEST(state_name_tests, testStateNameToIndex)
#else
static void testStateNameToIndex(void)
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(state_name_tests, testStateNameListCount)
#else
static void testStateNameListCount(void)
#endif
{
    OS_Keylist list;
    unsigned count;
    bool status;

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    count = state_name_list_count(list);
    zassert_equal(count, 0, NULL);

    state_name_list_init(list, State_Names);

    count = state_name_list_count(list);
    zassert_equal(count, 3, NULL);

    /* cleanup */
    status = state_name_list_init(list, NULL);
    zassert_true(status, NULL);
    Keylist_Delete(list);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(state_name_tests, testStateNameListIndex)
#else
static void testStateNameListIndex(void)
#endif
{
    OS_Keylist list;
    unsigned index;
    bool status;

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    state_name_list_init(list, State_Names);

    index = state_name_list_index(list, "State 1");
    zassert_equal(index, 1, NULL);

    index = state_name_list_index(list, "State 2");
    zassert_equal(index, 2, NULL);

    index = state_name_list_index(list, "State 3");
    zassert_equal(index, 3, NULL);

    index = state_name_list_index(list, "Missing State");
    zassert_equal(index, 0, NULL);

    /* cleanup */
    status = state_name_list_init(list, NULL);
    zassert_true(status, NULL);
    Keylist_Delete(list);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(state_name_tests, testStateNameListInit)
#else
static void testStateNameListInit(void)
#endif
{
    OS_Keylist list;
    const char *name;
    bool status;

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    status = state_name_list_init(list, State_Names);
    zassert_true(status, NULL);
    zassert_equal(state_name_list_count(list), 3, NULL);

    name = Keylist_Data(list, 1);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, "State 1"), 0, NULL);

    name = Keylist_Data(list, 2);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, "State 2"), 0, NULL);

    name = Keylist_Data(list, 3);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, "State 3"), 0, NULL);

    /* re-initialize with same data replaces existing entries */
    status = state_name_list_init(list, State_Names);
    zassert_true(status, NULL);
    zassert_equal(state_name_list_count(list), 3, NULL);

    /* cleanup */
    status = state_name_list_init(list, NULL);
    zassert_true(status, NULL);
    Keylist_Delete(list);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(state_name_tests, testStateNameListSet)
#else
static void testStateNameListSet(void)
#endif
{
    OS_Keylist list;
    bool status;
    const char *name;
    const char *new_text = "Active", *test_name = "State 1";
    unsigned count;

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    state_name_list_init(list, State_Names);
    count = state_name_list_count(list);
    zassert_equal(count, 3, NULL);

    /* update an existing entry */
    status = state_name_list_set(list, new_text, strlen(new_text), count - 1);
    zassert_true(status, NULL);

    name = Keylist_Data(list, count - 1);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, new_text), 0, NULL);

    /* set the same value again - should succeed */
    status = state_name_list_set(list, new_text, strlen(new_text), count - 1);
    zassert_true(status, NULL);

    /* set an entry beyond current size - should fail */
    status = state_name_list_set(list, test_name, strlen(test_name), count + 1);
    zassert_false(status, NULL);

    name = Keylist_Data(list, count + 1);
    zassert_is_null(name, NULL);

    /* cleanup */
    status = state_name_list_init(list, NULL);
    zassert_true(status, NULL);
    Keylist_Delete(list);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(state_name_tests, testStateNameListWrite)
#else
static void testStateNameListWrite(void)
#endif
{
    OS_Keylist list;
    BACNET_ERROR_CODE error_code;
    uint8_t apdu[64];
    int apdu_len;
    BACNET_CHARACTER_STRING cstr;
    unsigned count;
    bool status;
    const char *name, *test_name = "Test State";

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    state_name_list_init(list, State_Names);

    /* write array size = 0 to resize to 2 entries */
    error_code = state_name_list_write_resizable(list, 0, 2, NULL, 0);
    zassert_equal(error_code, ERROR_CODE_SUCCESS, NULL);
    zassert_equal(state_name_list_count(list), 2, NULL);

    /* write array size = 0 to expand to 5 entries */
    error_code = state_name_list_write_resizable(list, 0, 5, NULL, 0);
    zassert_equal(error_code, ERROR_CODE_SUCCESS, NULL);
    zassert_equal(state_name_list_count(list), 5, NULL);

    /* write a character string to index 1 */
    characterstring_init_ansi(&cstr, test_name);
    apdu_len = encode_application_character_string(apdu, &cstr);
    zassert_true(apdu_len > 0, NULL);

    count = state_name_list_count(list);
    error_code =
        state_name_list_write_resizable(list, 1, count, apdu, apdu_len);
    zassert_equal(error_code, ERROR_CODE_SUCCESS, NULL);

    name = Keylist_Data(list, 1);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, test_name), 0, NULL);

    /* write to an index beyond array size - auto-expands only */
    count = state_name_list_count(list);
    error_code =
        state_name_list_write_resizable(list, count + 3, count, apdu, apdu_len);
    zassert_equal(error_code, ERROR_CODE_SUCCESS, NULL);
    zassert_equal(state_name_list_count(list), count + 3, NULL);

    /* value is written on auto-expand, intermediate elements are NULL */
    name = Keylist_Data(list, count + 1);
    zassert_is_null(name, NULL);
    name = Keylist_Data(list, count + 2);
    zassert_is_null(name, NULL);
    name = Keylist_Data(list, count + 3);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, test_name), 0, NULL);

    /* invalid data type (empty application data) */
    error_code = state_name_list_write_resizable(list, 1, count + 3, apdu, 0);
    zassert_equal(error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);

    /* cleanup */
    status = state_name_list_init(list, NULL);
    zassert_true(status, NULL);
    Keylist_Delete(list);
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
        ztest_unit_test(testStateNameToIndex),
        ztest_unit_test(testStateNameListCount),
        ztest_unit_test(testStateNameListIndex),
        ztest_unit_test(testStateNameListInit),
        ztest_unit_test(testStateNameListSet),
        ztest_unit_test(testStateNameListWrite));

    ztest_run_test_suite(state_name_tests);
}
#endif
