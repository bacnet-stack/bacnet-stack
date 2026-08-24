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

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bo_tests, testBinaryOutput_Writable_Properties)
#else
static void testBinaryOutput_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const int32_t *properties = NULL;
    uint32_t count = 0;
    uint32_t i = 0;
    bool has_object_name = false;
    bool has_description = false;

    Binary_Output_Init();
    zassert_not_equal(
        Binary_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    Binary_Output_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

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

    Binary_Output_Delete(instance);
    Binary_Output_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bo_tests, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bo_tests, ztest_unit_test(testBinaryOutput),
        ztest_unit_test(testBinaryOutput_Writable_Properties));

    ztest_run_test_suite(bo_tests);
}
#endif
