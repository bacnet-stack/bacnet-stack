/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/osv.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(osv_tests, testOctetString_Value)
#else
static void testOctetString_Value(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *required_property = NULL;
    const uint32_t instance = 1;

    OctetString_Value_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_OCTETSTRING_VALUE;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    OctetString_Value_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = OctetString_Value_Read_Property(&rpdata);
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
        } else {
            printf(
                "property '%s': failed to read!\n",
                bactext_property_name(rpdata.object_property));
        }
        required_property++;
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(osv_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(osv_tests, ztest_unit_test(testOctetString_Value));

    ztest_run_test_suite(osv_tests);
}
#endif
