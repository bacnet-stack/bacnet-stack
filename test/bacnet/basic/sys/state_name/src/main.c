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

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    count = state_name_list_count(list);
    zassert_equal(count, 0, NULL);

    state_name_list_init(list, State_Names);

    count = state_name_list_count(list);
    zassert_equal(count, 3, NULL);

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

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    list = state_name_list_init(list, State_Names);
    zassert_not_null(list, NULL);
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
    list = state_name_list_init(list, State_Names);
    zassert_not_null(list, NULL);
    zassert_equal(state_name_list_count(list), 3, NULL);

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
    const char *new_text = "Active";

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    state_name_list_init(list, State_Names);

    /* update an existing entry */
    status = state_name_list_set(list, new_text, strlen(new_text), 1);
    zassert_true(status, NULL);

    name = Keylist_Data(list, 1);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, new_text), 0, NULL);

    /* set the same value again - should succeed */
    status = state_name_list_set(list, new_text, strlen(new_text), 1);
    zassert_true(status, NULL);

    /* add a new entry beyond current size */
    status = state_name_list_set(list, "New State", strlen("New State"), 10);
    zassert_true(status, NULL);

    name = Keylist_Data(list, 10);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, "New State"), 0, NULL);

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
    const char *name;

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    state_name_list_init(list, State_Names);

    /* write array size = 0 to resize to 2 entries */
    error_code = state_name_list_write(list, 0, 2, NULL, 0);
    zassert_equal(error_code, ERROR_CODE_SUCCESS, NULL);
    zassert_equal(state_name_list_count(list), 2, NULL);

    /* write array size = 0 to expand to 5 entries */
    error_code = state_name_list_write(list, 0, 5, NULL, 0);
    zassert_equal(error_code, ERROR_CODE_SUCCESS, NULL);
    zassert_equal(state_name_list_count(list), 5, NULL);

    /* write a character string to index 1 */
    characterstring_init_ansi(&cstr, "Updated State");
    apdu_len = encode_application_character_string(apdu, &cstr);
    zassert_true(apdu_len > 0, NULL);

    error_code = state_name_list_write(list, 1, 5, apdu, apdu_len);
    zassert_equal(error_code, ERROR_CODE_SUCCESS, NULL);

    name = Keylist_Data(list, 1);
    zassert_not_null(name, NULL);
    zassert_equal(strcmp(name, "Updated State"), 0, NULL);

    /* write to an index beyond array size - auto-expands only */
    error_code = state_name_list_write(list, 8, 5, apdu, apdu_len);
    zassert_equal(error_code, ERROR_CODE_SUCCESS, NULL);
    zassert_equal(state_name_list_count(list), 8, NULL);

    /* value is not written on auto-expand; key 8 is a placeholder NULL */
    name = Keylist_Data(list, 8);
    zassert_is_null(name, NULL);
    /* invalid data type (empty application data) */
    error_code = state_name_list_write(list, 1, 8, apdu, 0);
    zassert_equal(error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);

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
