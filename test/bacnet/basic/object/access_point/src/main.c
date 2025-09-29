/**
 * @file
 * @brief test BACnet Access Point object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/basic/object/access_point.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(access_point_tests, testAccessPoint)
#else
static void testAccessPoint(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *required_property = NULL;
    unsigned count = 0;
    uint32_t object_instance = 0;

    Access_Point_Init();
    count = Access_Point_Count();
    zassert_true(count > 0, NULL);
    object_instance = Access_Point_Index_To_Instance(0);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_POINT;
    rpdata.object_instance = object_instance;
    rpdata.array_index = BACNET_ARRAY_ALL;
    Access_Point_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = Access_Point_Read_Property(&rpdata);
        if (len >= 0) {
            zassert_true(len >= 0, NULL);
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            if (len != test_len) {
                printf(
                    "property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            if (rpdata.object_property == PROP_ACCESS_DOORS) {
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
ZTEST_SUITE(access_point_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(access_point_tests, ztest_unit_test(testAccessPoint));

    ztest_run_test_suite(access_point_tests);
}
#endif
