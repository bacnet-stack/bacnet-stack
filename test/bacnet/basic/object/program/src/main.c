/**
 * @file
 * @brief Unit test for the Program object type
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2025
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/program.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testProgramObject)
#else
static void testProgramObject(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Program_Init();
    object_instance = Program_Create(object_instance);
    count = Program_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Program_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_PROGRAM, object_instance, Program_Property_Lists,
        Program_Read_Property, Program_Write_Property, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Program_Name_Set, Program_Name_ASCII);
    status = Program_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(program_object_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(program_object_tests, ztest_unit_test(testProgramObject));

    ztest_run_test_suite(program_object_tests);
}
#endif
