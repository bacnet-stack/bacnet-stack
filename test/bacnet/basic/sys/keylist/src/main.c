/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/basic/sys/keylist.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test the FIFO
 */
static void testKeyListFIFO(void)
{
    OS_Keylist list;
    KEY key;
    int index;
    char *data1 = "Joshua";
    char *data2 = "Anna";
    char *data3 = "Mary";
    char *data;

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    key = 0;
    index = Keylist_Data_Add(list, key, data1);
    zassert_equal(index, 0, NULL);
    index = Keylist_Data_Add(list, key, data2);
    zassert_equal(index, 0, NULL);
    index = Keylist_Data_Add(list, key, data3);
    zassert_equal(index, 0, NULL);

    zassert_equal(Keylist_Count(list), 3, NULL);

    data = Keylist_Data_Pop(list);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data1), 0, NULL);
    data = Keylist_Data_Pop(list);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data2), 0, NULL);
    data = Keylist_Data_Pop(list);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data3), 0, NULL);
    data = Keylist_Data_Pop(list);
    zassert_equal(data, NULL, NULL);
    data = Keylist_Data_Pop(list);
    zassert_equal(data, NULL, NULL);

    Keylist_Delete(list);

    return;
}

/* test the FILO */
static void testKeyListFILO(void)
{
    OS_Keylist list;
    KEY key;
    int index;
    char *data1 = "Joshua";
    char *data2 = "Anna";
    char *data3 = "Mary";
    char *data;

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    key = 0;
    index = Keylist_Data_Add(list, key, data1);
    zassert_equal(index, 0, NULL);
    index = Keylist_Data_Add(list, key, data2);
    zassert_equal(index, 0, NULL);
    index = Keylist_Data_Add(list, key, data3);
    zassert_equal(index, 0, NULL);

    zassert_equal(Keylist_Count(list), 3, NULL);

    data = Keylist_Data_Delete_By_Index(list, 0);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data3), 0, NULL);

    data = Keylist_Data_Delete_By_Index(list, 0);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data2), 0, NULL);

    data = Keylist_Data_Delete_By_Index(list, 0);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data1), 0, NULL);

    data = Keylist_Data_Delete_By_Index(list, 0);
    zassert_equal(data, NULL, NULL);

    data = Keylist_Data_Delete_By_Index(list, 0);
    zassert_equal(data, NULL, NULL);

    Keylist_Delete(list);

    return;
}

static void testKeyListDataKey(void)
{
    OS_Keylist list;
    KEY key;
    KEY test_key;
    int index;
    char *data1 = "Joshua";
    char *data2 = "Anna";
    char *data3 = "Mary";
    char *data;

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    key = 1;
    index = Keylist_Data_Add(list, key, data1);
    zassert_equal(index, 0, NULL);
    test_key = Keylist_Key(list, index);
    zassert_equal(test_key, key, NULL);

    key = 2;
    index = Keylist_Data_Add(list, key, data2);
    zassert_equal(index, 1, NULL);
    test_key = Keylist_Key(list, index);
    zassert_equal(test_key, key, NULL);

    key = 3;
    index = Keylist_Data_Add(list, key, data3);
    zassert_equal(index, 2, NULL);
    test_key = Keylist_Key(list, index);
    zassert_equal(test_key, key, NULL);

    zassert_equal(Keylist_Count(list), 3, NULL);

    /* look at the data */
    key = 2;
    data = Keylist_Data(list, key);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data2), 0, NULL);

    key = 1;
    data = Keylist_Data(list, key);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data1), 0, NULL);

    key = 3;
    data = Keylist_Data(list, key);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data3), 0, NULL);

    /* work the data */
    key = 2;
    data = Keylist_Data_Delete(list, key);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data2), 0, NULL);
    data = Keylist_Data_Delete(list, key);
    zassert_equal(data, NULL, NULL);
    zassert_equal(Keylist_Count(list), 2, NULL);

    key = 1;
    data = Keylist_Data(list, key);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data1), 0, NULL);

    key = 3;
    data = Keylist_Data(list, key);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data3), 0, NULL);

    /* cleanup */
    do {
        data = Keylist_Data_Pop(list);
    } while (data);

    Keylist_Delete(list);

    return;
}

static void testKeyListDataIndex(void)
{
    OS_Keylist list;
    KEY key;
    int index;
    char *data1 = "Joshua";
    char *data2 = "Anna";
    char *data3 = "Mary";
    char *data;

    list = Keylist_Create();
    zassert_not_null(list, NULL);

    key = 0;
    index = Keylist_Data_Add(list, key, data1);
    zassert_equal(index, 0, NULL);
    index = Keylist_Data_Add(list, key, data2);
    zassert_equal(index, 0, NULL);
    index = Keylist_Data_Add(list, key, data3);
    zassert_equal(index, 0, NULL);

    zassert_equal(Keylist_Count(list), 3, NULL);

    /* look at the data */
    data = Keylist_Data_Index(list, 0);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data3), 0, NULL);

    data = Keylist_Data_Index(list, 1);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data2), 0, NULL);

    data = Keylist_Data_Index(list, 2);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data1), 0, NULL);

    /* work the data */
    data = Keylist_Data_Delete_By_Index(list, 1);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data2), 0, NULL);

    zassert_equal(Keylist_Count(list), 2, NULL);

    data = Keylist_Data_Index(list, 0);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data3), 0, NULL);

    data = Keylist_Data_Index(list, 1);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data1), 0, NULL);

    data = Keylist_Data_Delete_By_Index(list, 1);
    zassert_not_null(data, NULL);
    zassert_equal(strcmp(data, data1), 0, NULL);

    data = Keylist_Data_Delete_By_Index(list, 1);
    zassert_equal(data, NULL, NULL);

    /* cleanup */
    do {
        data = Keylist_Data_Pop(list);
    } while (data);

    Keylist_Delete(list);

    return;
}

/* test access of a lot of entries */
static void testKeyListLarge(void)
{
    int data1 = 42;
    int *data;
    OS_Keylist list;
    KEY key;
    int index;
    const unsigned num_keys = 1024 * 16;

    list = Keylist_Create();
    if (!list)
        return;

    for (key = 0; key < num_keys; key++) {
        index = Keylist_Data_Add(list, key, &data1);
    }
    for (key = 0; key < num_keys; key++) {
        data = Keylist_Data(list, key);
        zassert_equal(*data, data1, NULL);
    }
    for (index = 0; index < num_keys; index++) {
        data = Keylist_Data_Index(list, index);
        zassert_equal(*data, data1, NULL);
    }
    Keylist_Delete(list);

    return;
}

/* test the encode and decode macros */
static void testKeySample(void)
{
    int type, id;
    int type_list[] = { 0, 1, KEY_TYPE_MAX / 2, KEY_TYPE_MAX - 1, -1 };
    int id_list[] = { 0, 1, KEY_ID_MAX / 2, KEY_ID_MAX - 1, -1 };
    int type_index = 0;
    int id_index = 0;
    int decoded_type, decoded_id;
    KEY key;

    while (type_list[type_index] != -1) {
        while (id_list[id_index] != -1) {
            type = type_list[type_index];
            id = id_list[id_index];
            key = KEY_ENCODE(type, id);
            decoded_type = KEY_DECODE_TYPE(key);
            decoded_id = KEY_DECODE_ID(key);
            zassert_equal(decoded_type, type, NULL);
            zassert_equal(decoded_id, id, NULL);
            id_index++;
        }
        id_index = 0;
        type_index++;
    }

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(keylist_tests,
     ztest_unit_test(testKeyListFIFO),
     ztest_unit_test(testKeyListFILO),
     ztest_unit_test(testKeyListDataKey),
     ztest_unit_test(testKeyListDataIndex),
     ztest_unit_test(testKeyListLarge),
     ztest_unit_test(testKeySample)
     );

    ztest_run_test_suite(keylist_tests);
}
