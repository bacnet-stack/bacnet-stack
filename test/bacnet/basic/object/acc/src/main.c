/**
 * @file
 * @brief test BACnet Accumulator object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2017
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/acc.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(acc_tests, test_Accumulator)
#else
static void test_Accumulator(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *required_property = NULL;
    BACNET_UNSIGNED_INTEGER unsigned_value = 1;

    Accumulator_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCUMULATOR;
    rpdata.object_instance = Accumulator_Index_To_Instance(0);

    Accumulator_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Accumulator_Read_Property(&rpdata);
        zassert_true(len >= 0, NULL);
        if (len >= 0) {
            if (IS_CONTEXT_SPECIFIC(rpdata.application_data[0])) {
                test_len = bacapp_decode_context_data(
                    rpdata.application_data, len, &value,
                    rpdata.object_property);
            } else {
                test_len = bacapp_decode_application_data(
                    rpdata.application_data, len, &value);
            }
            if (len != test_len) {
                printf(
                    "property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            zassert_equal(len, test_len, NULL);
        }
        required_property++;
    }
    /* test 1-bit to 64-bit encode/decode of present-value */
    rpdata.object_property = PROP_PRESENT_VALUE;
    while (unsigned_value != BACNET_UNSIGNED_INTEGER_MAX) {
        Accumulator_Present_Value_Set(0, unsigned_value);
        len = Accumulator_Read_Property(&rpdata);
        zassert_not_equal(len, 0, NULL);
        test_len = bacapp_decode_application_data(
            rpdata.application_data, len, &value);
        zassert_equal(len, test_len, NULL);
        unsigned_value |= (unsigned_value << 1);
    }

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(acc_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(acc_tests, ztest_unit_test(test_Accumulator));

    ztest_run_test_suite(acc_tests);
}
#endif
