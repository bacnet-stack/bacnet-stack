/**
 * @file
 * @brief test BACnet Life Safety Operation service API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: MIT
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
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lso_tests, testLSO)
#else
static void testLSO(void)
#endif
{
    uint8_t apdu[1000] = { 0 };
    uint8_t invoke_id = 100;
    int apdu_len = 0, null_len = 0, test_len = 0;

    BACNET_LSO_DATA data = { 0 };
    BACNET_LSO_DATA test_data = { 0 };

    characterstring_init_ansi(&data.requestingSrc, "foobar");
    data.operation = LIFE_SAFETY_OP_RESET;
    data.processId = 0x1234;
    data.use_target = true;
    data.targetObject.instance = 0x1000;
    data.targetObject.type = OBJECT_BINARY_INPUT;
    /* encode/decode */
    null_len = lso_encode_apdu(NULL, invoke_id, &data);
    apdu_len = lso_encode_apdu(apdu, invoke_id, &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    test_len = lso_decode_service_request(&apdu[4], apdu_len, &test_data);
    zassert_true(test_len > 0, "test_len=%d", test_len);
    /* check the values decoded */
    zassert_equal(data.operation, test_data.operation, NULL);
    zassert_equal(data.processId, test_data.processId, NULL);
    zassert_equal(data.use_target, test_data.use_target, NULL);
    zassert_equal(
        data.targetObject.instance, test_data.targetObject.instance, NULL);
    zassert_equal(data.targetObject.type, test_data.targetObject.type, NULL);
    zassert_equal(
        memcmp(
            data.requestingSrc.value, test_data.requestingSrc.value,
            test_data.requestingSrc.length),
        0, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(lso_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(lso_tests, ztest_unit_test(testLSO));

    ztest_run_test_suite(lso_tests);
}
#endif
