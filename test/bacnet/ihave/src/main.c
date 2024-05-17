/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/ihave.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testIHaveData(BACNET_I_HAVE_DATA *data)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_I_HAVE_DATA test_data;

    len = ihave_encode_apdu(&apdu[0], data);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = ihave_decode_apdu(&apdu[0], apdu_len, &test_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_data.device_id.type, data->device_id.type, NULL);
    zassert_equal(test_data.device_id.instance, data->device_id.instance, NULL);
    zassert_equal(test_data.object_id.type, data->object_id.type, NULL);
    zassert_equal(test_data.object_id.instance, data->object_id.instance, NULL);
    zassert_true(
        characterstring_same(&test_data.object_name, &data->object_name), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ihave_tests, testIHave)
#else
static void testIHave(void)
#endif
{
    BACNET_I_HAVE_DATA data;

    characterstring_init_ansi(&data.object_name, "Patricia - my love!");
    data.device_id.type = OBJECT_DEVICE;
    for (data.device_id.instance = 1;
         data.device_id.instance <= BACNET_MAX_INSTANCE;
         data.device_id.instance <<= 1) {
        for (data.object_id.type = OBJECT_ANALOG_INPUT;
             data.object_id.type < MAX_BACNET_OBJECT_TYPE;
             data.object_id.type++) {
            for (data.object_id.instance = 1;
                 data.object_id.instance <= BACNET_MAX_INSTANCE;
                 data.object_id.instance <<= 1) {
                testIHaveData(&data);
            }
        }
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(ihave_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(ihave_tests, ztest_unit_test(testIHave));

    ztest_run_test_suite(ihave_tests);
}
#endif
