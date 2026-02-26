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
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(osv_tests, testOctetString_Value_Value)
#else
static void testOctetString_Value_Value(void)
#endif
{
    bool status = false;
    unsigned count = 0, index = 0;
    uint32_t object_instance = 0, test_object_instance = 0;
    const int32_t *writable_properties = NULL;
    const int32_t skip_fail_property_list[] = { -1 };

    OctetString_Value_Init();
    test_object_instance = OctetString_Value_Create(BACNET_MAX_INSTANCE + 1);
    zassert_equal(test_object_instance, BACNET_MAX_INSTANCE, NULL);
    test_object_instance = OctetString_Value_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(test_object_instance, BACNET_MAX_INSTANCE, NULL);
    status = OctetString_Value_Delete(test_object_instance);
    zassert_true(status, NULL);
    count = OctetString_Value_Count();
    zassert_true(count == 0, NULL);
    test_object_instance = OctetString_Value_Create(object_instance);
    zassert_equal(test_object_instance, object_instance, NULL);
    status = OctetString_Value_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    status = OctetString_Value_Valid_Instance(object_instance - 1);
    zassert_false(status, NULL);
    index = OctetString_Value_Instance_To_Index(object_instance);
    zassert_equal(index, 0, NULL);
    test_object_instance = OctetString_Value_Index_To_Instance(index);
    zassert_equal(object_instance, test_object_instance, NULL);
    count = OctetString_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = OctetString_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_OCTETSTRING_VALUE, object_instance,
        OctetString_Value_Property_Lists, OctetString_Value_Read_Property,
        OctetString_Value_Write_Property, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, OctetString_Value_Name_Set,
        OctetString_Value_Name_ASCII);
    OctetString_Value_Writable_Property_List(
        object_instance, &writable_properties);
    zassert_not_null(writable_properties, NULL);
    status = OctetString_Value_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(osv_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(osv_tests, ztest_unit_test(testOctetString_Value_Value));

    ztest_run_test_suite(osv_tests);
}
#endif
