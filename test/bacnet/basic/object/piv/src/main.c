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
#include <property_test.h>

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
    bool status = false;
    unsigned count = 0, index = 0;
    uint32_t object_instance = 0, test_object_instance = 0;
    const int32_t *writable_properties = NULL;
    const int32_t skip_fail_property_list[] = { -1 };

    PositiveInteger_Value_Init();
    test_object_instance =
        PositiveInteger_Value_Create(BACNET_MAX_INSTANCE + 1);
    zassert_equal(test_object_instance, BACNET_MAX_INSTANCE, NULL);
    test_object_instance = PositiveInteger_Value_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(test_object_instance, BACNET_MAX_INSTANCE, NULL);
    status = PositiveInteger_Value_Delete(test_object_instance);
    zassert_true(status, NULL);
    count = PositiveInteger_Value_Count();
    zassert_true(count == 0, NULL);
    test_object_instance = PositiveInteger_Value_Create(object_instance);
    zassert_equal(test_object_instance, object_instance, NULL);
    status = PositiveInteger_Value_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    status = PositiveInteger_Value_Valid_Instance(object_instance - 1);
    zassert_false(status, NULL);
    index = PositiveInteger_Value_Instance_To_Index(object_instance);
    zassert_equal(index, 0, NULL);
    test_object_instance = PositiveInteger_Value_Index_To_Instance(index);
    zassert_equal(object_instance, test_object_instance, NULL);
    count = PositiveInteger_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = PositiveInteger_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_POSITIVE_INTEGER_VALUE, object_instance,
        PositiveInteger_Value_Property_Lists,
        PositiveInteger_Value_Read_Property,
        PositiveInteger_Value_Write_Property, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, PositiveInteger_Value_Name_Set,
        PositiveInteger_Value_Name_ASCII);
    PositiveInteger_Value_Writable_Property_List(
        object_instance, &writable_properties);
    zassert_not_null(writable_properties, NULL);
    status = PositiveInteger_Value_Delete(object_instance);
    zassert_true(status, NULL);
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
