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
#include <bacnet/proplist.h>
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
    const int32_t skip_fail_property_list[] = { -1 };

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
 * @brief Test Binary Value Writable_Property_List and Write_Enabled APIs
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bv_tests, testBinary_Value_Writable_Properties)
#else
static void testBinary_Value_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Binary_Value_Init();
    zassert_not_equal(Binary_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* write-enabled (default): list starts with PROP_PRESENT_VALUE */
    zassert_true(Binary_Value_Write_Enabled(instance), NULL);
    Binary_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write-disabled: list skips PROP_PRESENT_VALUE */
    Binary_Value_Write_Disable(instance);
    zassert_false(Binary_Value_Write_Enabled(instance), NULL);
    Binary_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_not_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    Binary_Value_Write_Enable(instance);
    zassert_true(Binary_Value_Write_Enabled(instance), NULL);
    Binary_Value_Writable_Property_List(instance, &properties);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* unknown instance: must return a valid list, not NULL/garbage */
    properties = NULL;
    Binary_Value_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* NULL properties pointer: must not crash */
    Binary_Value_Writable_Property_List(instance, NULL);

    Binary_Value_Delete(instance);
    Binary_Value_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bv_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bv_tests, ztest_unit_test(testBinary_Value),
        ztest_unit_test(testBinary_Value_Writable_Properties));

    ztest_run_test_suite(bv_tests);
}
#endif
