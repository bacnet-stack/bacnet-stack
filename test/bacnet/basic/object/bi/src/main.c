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
#include <bacnet/bactext.h>
#include <bacnet/basic/object/bi.h>
#include <bacnet/proplist.h>
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
    const int32_t skip_fail_property_list[] = { -1 };

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
        object_instance, Binary_Input_Name_Set, Binary_Input_Name_ASCII);
    status = Binary_Input_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test Binary Input Writable_Property_List and Write_Enabled APIs
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bi_tests, testBinaryInput_Writable_Properties)
#else
static void testBinaryInput_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Binary_Input_Init();
    zassert_not_equal(Binary_Input_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* write-disabled (default for bi): list skips PROP_PRESENT_VALUE */
    zassert_false(Binary_Input_Write_Enabled(instance), NULL);
    Binary_Input_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);
    zassert_not_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write-enabled: list starts with PROP_PRESENT_VALUE */
    Binary_Input_Write_Enable(instance);
    zassert_true(Binary_Input_Write_Enabled(instance), NULL);
    Binary_Input_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write-disabled again: list skips PROP_PRESENT_VALUE */
    Binary_Input_Write_Disable(instance);
    zassert_false(Binary_Input_Write_Enabled(instance), NULL);
    Binary_Input_Writable_Property_List(instance, &properties);
    zassert_not_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* unknown instance: must return a valid list, not NULL/garbage */
    properties = NULL;
    Binary_Input_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* NULL properties pointer: must not crash */
    Binary_Input_Writable_Property_List(instance, NULL);

    Binary_Input_Delete(instance);
    Binary_Input_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bi_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bi_tests, ztest_unit_test(testBinaryInput),
        ztest_unit_test(testBinaryInput_Writable_Properties));

    ztest_run_test_suite(bi_tests);
}
#endif
