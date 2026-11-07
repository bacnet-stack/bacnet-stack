/**
 * @file
 * @brief test BACnet access_door object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/access_door.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_access_door, test_object_access_door)
#else
static void test_object_access_door(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    /* for decode value data */
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int32_t *pRequired = NULL;
    const int32_t *pOptional = NULL;
    const int32_t *pProprietary = NULL;
    unsigned count = 0;
    uint32_t object_instance = 0;

    Access_Door_Init();
    count = Access_Door_Count();
    zassert_true(count > 0, NULL);
    object_instance = Access_Door_Index_To_Instance(0);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_DOOR;
    rpdata.object_instance = object_instance;
    Access_Door_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Access_Door_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            zassert_true(test_len >= 0, NULL);
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Access_Door_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            zassert_true(test_len >= 0, NULL);
        }
        pOptional++;
    }

    return;
}

/**
 * @brief Test Present_Value priority handling and Current_Command_Priority
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_access_door, test_object_access_door_priority)
#else
static void test_object_access_door_priority(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    uint32_t object_instance = 1;
    unsigned count = 0;

    Access_Door_Init();
    count = Access_Door_Count();
    zassert_true(count > 1, NULL);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_DOOR;
    rpdata.object_instance = object_instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    /* nothing commanded: Current_Command_Priority is NULL */
    rpdata.object_property = PROP_CURRENT_COMMAND_PRIORITY;
    len = Access_Door_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, (uint8_t)len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_NULL, NULL);

    /* command a value: Current_Command_Priority is the priority */
    zassert_true(
        Access_Door_Present_Value_Set(object_instance, DOOR_VALUE_UNLOCK, 8),
        NULL);
    zassert_equal(
        Access_Door_Present_Value(object_instance), DOOR_VALUE_UNLOCK, NULL);
    len = Access_Door_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, (uint8_t)len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_UNSIGNED_INT, NULL);
    zassert_equal(value.type.Unsigned_Int, 8, NULL);

    /* Priority_Array element is NULL only when relinquished */
    rpdata.object_property = PROP_PRIORITY_ARRAY;
    rpdata.array_index = 8;
    len = Access_Door_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, (uint8_t)len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_ENUMERATED, NULL);
    rpdata.array_index = 9;
    len = Access_Door_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, (uint8_t)len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_NULL, NULL);

    /* other doors are not affected */
    zassert_equal(Access_Door_Present_Value_Priority(0), 0, NULL);

    /* invalid instance is rejected */
    rpdata.object_instance = count;
    rpdata.object_property = PROP_PRESENT_VALUE;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Access_Door_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(tests_object_access_door, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        tests_object_access_door, ztest_unit_test(test_object_access_door),
        ztest_unit_test(test_object_access_door_priority));

    ztest_run_test_suite(tests_object_access_door);
}
#endif
