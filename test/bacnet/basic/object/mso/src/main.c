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
#include <bacnet/basic/object/mso.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mso_tests, testMultistateOutput)
#else
static void testMultistateOutput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { PROP_PRIORITY_ARRAY, -1 };

    Multistate_Output_Init();
    object_instance = Multistate_Output_Create(object_instance);
    count = Multistate_Output_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Multistate_Output_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_MULTI_STATE_OUTPUT, object_instance,
        Multistate_Output_Property_Lists, Multistate_Output_Read_Property,
        Multistate_Output_Write_Property, skip_fail_property_list);
    status = Multistate_Output_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(mso_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(mso_tests, ztest_unit_test(testMultistateOutput));

    ztest_run_test_suite(mso_tests);
}
#endif
