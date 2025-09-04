/**
 * @file
 * @brief test BACnet Binary Value object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/bv.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bv_tests, testBinary_Value)
#else
static void testBinary_Value(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Binary_Value_Init();
    object_instance = Binary_Value_Create(object_instance);
    count = Binary_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Binary_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_BINARY_VALUE, object_instance, Binary_Value_Property_Lists,
        Binary_Value_Read_Property, Binary_Value_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Binary_Value_Name_Set, Binary_Value_Name_ASCII);
    status = Binary_Value_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bv_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bv_tests, ztest_unit_test(testBinary_Value));

    ztest_run_test_suite(bv_tests);
}
#endif
