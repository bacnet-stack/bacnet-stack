/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/msv.h>
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(msv_tests, testMultistateValue)
#else
static void testMultistateValue(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Multistate_Value_Init();
    object_instance = Multistate_Value_Create(object_instance);
    count = Multistate_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Multistate_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_MULTI_STATE_VALUE, object_instance,
        Multistate_Value_Property_Lists, Multistate_Value_Read_Property,
        Multistate_Value_Write_Property, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Multistate_Value_Name_Set,
        Multistate_Value_Name_ASCII);
    status = Multistate_Value_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(msv_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(msv_tests, ztest_unit_test(testMultistateValue));

    ztest_run_test_suite(msv_tests);
}
#endif
