/**
 * @file
 * @brief Unit test for BACnet Time Value object encode/decode APIs
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/time_value.h>
#include <bacnet/bactext.h>
#include <bacnet/proplist.h>
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
    const int32_t skip_fail_property_list[] = { -1 };

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

/**
 * @brief Test Time Value Writable_Property_List and Write_Enabled APIs
 */
#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_tv, testTimeValue_Writable_Properties)
#else
static void testTimeValue_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Time_Value_Init();
    zassert_not_equal(Time_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* write-enabled (default): list starts with PROP_PRESENT_VALUE */
    zassert_true(Time_Value_Write_Enabled(instance), NULL);
    Time_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write-disabled: list skips PROP_PRESENT_VALUE */
    Time_Value_Write_Disable(instance);
    zassert_false(Time_Value_Write_Enabled(instance), NULL);
    Time_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_not_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    Time_Value_Write_Enable(instance);
    zassert_true(Time_Value_Write_Enabled(instance), NULL);
    Time_Value_Writable_Property_List(instance, &properties);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* unknown instance: must return a valid list, not NULL/garbage */
    properties = NULL;
    Time_Value_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* NULL properties pointer: must not crash */
    Time_Value_Writable_Property_List(instance, NULL);

    Time_Value_Delete(instance);
    Time_Value_Cleanup();
}
/**
 * @}
 */

#ifdef CONFIG_ZTEST_NEW_API
ZTEST_SUITE(bacnet_tv, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        tv_tests, ztest_unit_test(testTimeValue),
        ztest_unit_test(testTimeValue_Writable_Properties));

    ztest_run_test_suite(tv_tests);
}
#endif
