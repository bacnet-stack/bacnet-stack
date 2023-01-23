/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/whohas.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
int whohas_decode_apdu(
    uint8_t *apdu, unsigned apdu_len, BACNET_WHO_HAS_DATA *data)
{
    int len = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST)
        return -1;
    if (apdu[1] != SERVICE_UNCONFIRMED_WHO_HAS)
        return -1;
    /* optional limits - must be used as a pair */
    if (apdu_len > 2) {
        len = whohas_decode_service_request(&apdu[2], apdu_len - 2, data);
    }

    return len;
}

static void testWhoHasData(BACNET_WHO_HAS_DATA *data)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_WHO_HAS_DATA test_data;

    len = whohas_encode_apdu(&apdu[0], data);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;

    len = whohas_decode_apdu(&apdu[0], apdu_len, &test_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_data.low_limit, data->low_limit, NULL);
    zassert_equal(test_data.high_limit, data->high_limit, NULL);
    zassert_equal(test_data.is_object_name, data->is_object_name, NULL);
    /* Object ID */
    if (data->is_object_name == false) {
        zassert_equal(
            test_data.object.identifier.type, data->object.identifier.type, NULL);
        zassert_equal(
            test_data.object.identifier.instance,
                data->object.identifier.instance, NULL);
    }
    /* Object Name */
    else {
        zassert_true(
            characterstring_same(&test_data.object.name, &data->object.name), NULL);
    }
}

static void testWhoHas(void)
{
    BACNET_WHO_HAS_DATA data;

    data.low_limit = -1;
    data.high_limit = -1;
    data.is_object_name = false;
    data.object.identifier.type = OBJECT_ANALOG_INPUT;
    data.object.identifier.instance = 1;
    testWhoHasData(&data);

    for (data.low_limit = 0; data.low_limit <= BACNET_MAX_INSTANCE;
         data.low_limit += (BACNET_MAX_INSTANCE / 4)) {
        for (data.high_limit = 0; data.high_limit <= BACNET_MAX_INSTANCE;
             data.high_limit += (BACNET_MAX_INSTANCE / 4)) {
            data.is_object_name = false;
            for (data.object.identifier.type = OBJECT_ANALOG_INPUT;
                 data.object.identifier.type < MAX_BACNET_OBJECT_TYPE;
                 data.object.identifier.type++) {
                for (data.object.identifier.instance = 1;
                     data.object.identifier.instance <= BACNET_MAX_INSTANCE;
                     data.object.identifier.instance <<= 1) {
                    testWhoHasData(&data);
                }
            }
            data.is_object_name = true;
            characterstring_init_ansi(&data.object.name, "patricia");
            testWhoHasData(&data);
        }
    }
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(whohas_tests,
     ztest_unit_test(testWhoHas)
     );

    ztest_run_test_suite(whohas_tests);
}
