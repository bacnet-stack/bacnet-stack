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
#include <bacnet/bactext.h>
#include <bacnet/basic/object/bi.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test Binary Input handling
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bi_tests, testBinaryInput)
#else
static void testBinaryInput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Binary_Input_Init();
    object_instance = Binary_Input_Create(object_instance);
    count = Binary_Input_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Binary_Input_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_BINARY_INPUT, object_instance, Binary_Input_Property_Lists,
        Binary_Input_Read_Property, Binary_Input_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance,
        Binary_Input_Name_Set,
        Binary_Input_Name_ASCII);
    status = Binary_Input_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bi_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bi_tests, ztest_unit_test(testBinaryInput));

    ztest_run_test_suite(bi_tests);
}
#endif
