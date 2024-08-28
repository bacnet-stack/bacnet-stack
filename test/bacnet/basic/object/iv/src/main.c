/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/iv.h>
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testInteger_Value(void)
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };
    const char *test_name = NULL;
    char *sample_name = "sample";

    Integer_Value_Init();
    object_instance = Integer_Value_Create(object_instance);
    count = Integer_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Integer_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_INTEGER_VALUE, object_instance, Integer_Value_Property_Lists,
        Integer_Value_Read_Property, Integer_Value_Write_Property,
        skip_fail_property_list);
    /* test the ASCII name get/set */
    status = Integer_Value_Name_Set(object_instance, sample_name);
    zassert_true(status, NULL);
    test_name = Integer_Value_Name_ASCII(object_instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Integer_Value_Name_Set(object_instance, NULL);
    zassert_true(status, NULL);
    test_name = Integer_Value_Name_ASCII(object_instance);
    zassert_equal(test_name, NULL, NULL);
    /* cleanup */
    status = Integer_Value_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(piv_tests, ztest_unit_test(testInteger_Value));

    ztest_run_test_suite(piv_tests);
}
