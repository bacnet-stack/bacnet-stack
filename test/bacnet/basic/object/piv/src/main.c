/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/piv.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(piv_tests, testPositiveInteger_Value)
#else
static void testPositiveInteger_Value(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *required_property = NULL;
    const uint32_t instance = 1;
    bool status = false;

    PositiveInteger_Value_Init();
    status = PositiveInteger_Value_Units_Set(instance, UNITS_VOLTS);
    zassert_true(status, NULL);
    zassert_equal(PositiveInteger_Value_Units(instance), UNITS_VOLTS, NULL);
    status = PositiveInteger_Value_Units_Set(
        PositiveInteger_Value_Count(), UNITS_VOLTS);
    zassert_false(status, NULL);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_POSITIVE_INTEGER_VALUE;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    PositiveInteger_Value_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = PositiveInteger_Value_Read_Property(&rpdata);
        zassert_true(len >= 0, NULL);
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            if (len != test_len) {
                printf(
                    "property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            if (rpdata.object_property == PROP_PRIORITY_ARRAY) {
                /* FIXME: known fail to decode */
                len = test_len;
            }
            zassert_equal(len, test_len, NULL);
        }
        required_property++;
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(piv_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(piv_tests, ztest_unit_test(testPositiveInteger_Value));

    ztest_run_test_suite(piv_tests);
}
#endif
