/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/schedule.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(schedule_tests, testSchedule)
#else
static void testSchedule(void)
#endif
{
    unsigned count = 0;
    uint32_t object_instance = 0;
    const int skip_fail_property_list[] = { 
        PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES, -1 };

    Schedule_Init();
    count = Schedule_Count();
    zassert_true(count > 0, NULL);
    object_instance = Schedule_Index_To_Instance(0);
    bacnet_object_properties_read_write_test(
        OBJECT_SCHEDULE,
        object_instance,
        Schedule_Property_Lists,
        Schedule_Read_Property,
        Schedule_Write_Property,
        skip_fail_property_list);
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(schedule_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(schedule_tests,
     ztest_unit_test(testSchedule)
     );

    ztest_run_test_suite(schedule_tests);
}
#endif
