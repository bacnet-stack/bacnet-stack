/**
 * @file
 * @brief Unit test for BACnet WhoHas-Request encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Greg Shue <greg.shue@outlook.com>
 * @date Aug 2022
 * @copyright SPDX-License-Identifier: MIT
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
    uint8_t *apdu, unsigned apdu_size, BACNET_WHO_HAS_DATA *data)
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
    if (apdu_size > 2) {
        len = whohas_decode_service_request(&apdu[2], apdu_size - 2, data);
    }

    return len;
}

static void testWhoHasData(BACNET_WHO_HAS_DATA *data)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    int null_len = 0;
    int test_len = 0;
    BACNET_WHO_HAS_DATA test_data = { 0 };

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
            test_data.object.identifier.type, data->object.identifier.type,
            NULL);
        zassert_equal(
            test_data.object.identifier.instance,
            data->object.identifier.instance, NULL);
    }
    /* Object Name */
    else {
        zassert_true(
            characterstring_same(&test_data.object.name, &data->object.name),
            NULL);
    }
    /* encoder bounds checking */
    null_len = bacnet_who_has_request_encode(NULL, data);
    apdu_len = bacnet_who_has_request_encode(apdu, data);
    zassert_true(apdu_len > 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    null_len = bacnet_who_has_service_request_encode(NULL, sizeof(apdu), data);
    apdu_len = bacnet_who_has_service_request_encode(apdu, sizeof(apdu), data);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len > 0, NULL);
    /* test short APDU buffer */
    while (--apdu_len) {
        test_len = bacnet_who_has_service_request_encode(apdu, apdu_len, data);
        zassert_equal(test_len, 0 , NULL);
    }
    /* decoder bounds checking */
    apdu_len = bacnet_who_has_request_encode(apdu, data);
    zassert_true(apdu_len > 0, NULL);
    test_len = whohas_decode_service_request(apdu, apdu_len, data);
    null_len = whohas_decode_service_request(apdu, apdu_len, NULL);
    zassert_equal(test_len, null_len, NULL);
    /* test short APDU buffer */
    while (--apdu_len) {
        test_len = whohas_decode_service_request(apdu, apdu_len, data);
        zassert_equal(test_len, BACNET_STATUS_ERROR , NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(whohas_tests, testWhoHas)
#else
static void testWhoHas(void)
#endif
{
    BACNET_WHO_HAS_DATA data;

    data.low_limit = -1;
    data.high_limit = -1;
    data.is_object_name = false;
    data.object.identifier.type = OBJECT_ANALOG_INPUT;
    data.object.identifier.instance = 0;
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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(whohas_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(whohas_tests, ztest_unit_test(testWhoHas));

    ztest_run_test_suite(whohas_tests);
}
#endif
