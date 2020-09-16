/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/basic/sys/key.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test the encode and decode macros
 */
static void testKeySample(void)
{
    int type, id;
    int type_list[] = { 0, 1, KEY_TYPE_MAX / 2, KEY_TYPE_MAX - 1, -1 };
    int id_list[] = { 0, 1, KEY_ID_MAX / 2, KEY_ID_MAX - 1, -1 };
    int type_index = 0;
    int id_index = 0;
    int decoded_type;
    int decoded_id;
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
    ztest_test_suite(key_tests,
     ztest_unit_test(testKeySample)
     );

    ztest_run_test_suite(key_tests);
}
