/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/command.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command)
#else
static void test_object_command(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = 0;
    const int skip_fail_property_list[] = { PROP_ACTION, -1 };

    Command_Init();
    count = Command_Count();
    zassert_true(count > 0, NULL);
    object_instance = Command_Index_To_Instance(0);
    bacnet_object_properties_read_write_test(
        OBJECT_COMMAND,
        object_instance,
        Command_Property_Lists,
        Command_Read_Property,
        Command_Write_Property,
        skip_fail_property_list);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(tests_object_command, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        tests_object_command, ztest_unit_test(test_object_command));

    ztest_run_test_suite(tests_object_command);
}
#endif
