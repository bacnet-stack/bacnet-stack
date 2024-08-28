/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/structured_view.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_object_structured_view)
#else
static void test_object_structured_view(void)
#endif
{
    bool status = false;
    unsigned count = 0, index = 0;
    const uint32_t instance = 123;
    const int skip_fail_property_list[] = { -1 };

    Structured_View_Init();
    Structured_View_Create(instance);
    status = Structured_View_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Structured_View_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    count = Structured_View_Count();
    zassert_true(count > 0, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_STRUCTURED_VIEW, instance, Structured_View_Property_Lists,
        Structured_View_Read_Property, NULL, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        instance,
        Structured_View_Name_Set,
        Structured_View_Name_ASCII);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(tests_object_structured_view, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        tests_object_structured_view,
        ztest_unit_test(test_object_structured_view));

    ztest_run_test_suite(tests_object_structured_view);
}
#endif
