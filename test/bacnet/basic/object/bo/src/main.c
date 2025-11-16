/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @section LICENSE
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/bo.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bo_tests, testBinaryOutput)
#else
static void testBinaryOutput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { PROP_PRIORITY_ARRAY, -1 };

    Binary_Output_Init();
    object_instance = Binary_Output_Create(object_instance);
    count = Binary_Output_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Binary_Output_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_BINARY_OUTPUT, object_instance, Binary_Output_Property_Lists,
        Binary_Output_Read_Property, Binary_Output_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Binary_Output_Name_Set, Binary_Output_Name_ASCII);
    status = Binary_Output_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bo_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bo_tests, ztest_unit_test(testBinaryOutput));

    ztest_run_test_suite(bo_tests);
}
#endif
