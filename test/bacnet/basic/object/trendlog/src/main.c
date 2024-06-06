/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/trendlog.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_Trend_Log_ReadProperty(void)
{
    unsigned count = 0;
    uint32_t object_instance = 0;
    bool status = false;
    const int known_fail_property_list[] = { -1 };

    Trend_Log_Init();
    count = Trend_Log_Count();
    zassert_true(count > 0, NULL);
    object_instance = Trend_Log_Index_To_Instance(0);
    status = Trend_Log_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_TRENDLOG, object_instance, Trend_Log_Property_Lists,
        Trend_Log_Read_Property, Trend_Log_Write_Property,
        known_fail_property_list);
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(
        trendlog_tests, ztest_unit_test(test_Trend_Log_ReadProperty));

    ztest_run_test_suite(trendlog_tests);
}
