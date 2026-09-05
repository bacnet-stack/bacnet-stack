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
    uint32_t i = 0;
    bool has_object_name = false;
    bool has_description = false;

    Binary_Value_Init();
    zassert_not_equal(Binary_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* write-enabled (default): list starts with PROP_PRESENT_VALUE */
    zassert_true(Binary_Value_Write_Enabled(instance), NULL);
    Binary_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);
    has_object_name = false;
    has_description = false;
    for (i = 0; properties[i] != -1; ++i) {
        if (properties[i] == PROP_OBJECT_NAME) {
            has_object_name = true;
        }
        if (properties[i] == PROP_DESCRIPTION) {
            has_description = true;
        }
    }
    zassert_true(has_object_name, NULL);
    zassert_true(has_description, NULL);
    /* write-disabled: list skips PROP_PRESENT_VALUE */
    Binary_Value_Write_Disable(instance);
    zassert_false(Binary_Value_Write_Enabled(instance), NULL);
    Binary_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_false(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    Binary_Value_Write_Enable(instance);
    zassert_true(Binary_Value_Write_Enabled(instance), NULL);
    Binary_Value_Writable_Property_List(instance, &properties);
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

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
 * @brief Test that changing Relinquish_Default alters the effective
 *  Present_Value sets the Change_Of_Value flag, and that setting the
 *  same effective value again does not
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bv_tests, testBinary_Value_Relinquish_Default_COV)
#else
static void testBinary_Value_Relinquish_Default_COV(void)
#endif
{
    const uint32_t instance = 654;
    bool status = false;

    Binary_Value_Init();
    zassert_not_equal(Binary_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* all priorities relinquished: Present_Value tracks Relinquish_Default,
       which defaults to BINARY_INACTIVE */
    Binary_Value_Change_Of_Value_Clear(instance);
    zassert_false(Binary_Value_Change_Of_Value(instance), NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_INACTIVE, NULL);

    /* a value change must set Changed */
    status = Binary_Value_Relinquish_Default_Set(instance, BINARY_ACTIVE);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_ACTIVE, NULL);
    zassert_true(Binary_Value_Change_Of_Value(instance), NULL);

    /* setting the same effective value again must not set Changed */
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Relinquish_Default_Set(instance, BINARY_ACTIVE);
    zassert_true(status, NULL);
    zassert_false(Binary_Value_Change_Of_Value(instance), NULL);

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
        ztest_unit_test(testBinary_Value_Writable_Properties),
        ztest_unit_test(testBinary_Value_Relinquish_Default_COV));

    ztest_run_test_suite(bv_tests);
}
#endif
