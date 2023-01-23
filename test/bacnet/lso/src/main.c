/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/lso.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testLSO(void)
{
    uint8_t apdu[1000];
    int len;

    BACNET_LSO_DATA data;
    BACNET_LSO_DATA rxdata;

    memset(&rxdata, 0, sizeof(rxdata));

    characterstring_init_ansi(&data.requestingSrc, "foobar");
    data.operation = LIFE_SAFETY_OP_RESET;
    data.processId = 0x1234;
    data.use_target = true;
    data.targetObject.instance = 0x1000;
    data.targetObject.type = OBJECT_BINARY_INPUT;

    len = lso_encode_apdu(apdu, 100, &data);

    lso_decode_service_request(&apdu[4], len, &rxdata);

    zassert_equal(data.operation, rxdata.operation, NULL);
    zassert_equal(data.processId, rxdata.processId, NULL);
    zassert_equal(data.use_target, rxdata.use_target, NULL);
    zassert_equal(data.targetObject.instance, rxdata.targetObject.instance, NULL);
    zassert_equal(data.targetObject.type, rxdata.targetObject.type, NULL);
    zassert_equal(
        memcmp(data.requestingSrc.value, rxdata.requestingSrc.value,
            rxdata.requestingSrc.length), 0, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(lso_tests,
     ztest_unit_test(testLSO)
     );

    ztest_run_test_suite(lso_tests);
}
