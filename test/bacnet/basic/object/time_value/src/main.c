/**
 * @file
 * @brief Unit test for BACnet Time Value object encode/decode APIs
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2023
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/time_value.h>
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test Time Value handling
 */
#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_tv, testTimeValue)
#else
static void testTimeValue(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Time_Value_Init();
    object_instance = Time_Value_Create(BACNET_MAX_INSTANCE);
    count = Time_Value_Count();
    zassert_true(count > 0, NULL);
    object_instance = Time_Value_Index_To_Instance(0);
    bacnet_object_properties_read_write_test(
        OBJECT_TIME_VALUE, object_instance, Time_Value_Property_Lists,
        Time_Value_Read_Property, Time_Value_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Time_Value_Name_Set, Time_Value_Name_ASCII);
    /* check the delete function */
    status = Time_Value_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#ifdef CONFIG_ZTEST_NEW_API
ZTEST_SUITE(bacnet_tv, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(tv_tests, ztest_unit_test(testTimeValue));

    ztest_run_test_suite(tv_tests);
}
#endif
