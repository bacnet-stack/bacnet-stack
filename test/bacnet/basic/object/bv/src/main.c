/**
 * @file
 * @brief test BACnet Binary Value object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/bv.h>
#include <bacnet/proplist.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Helper: write PROP_PRESENT_VALUE at the given priority through
 *  Binary_Value_Write_Property(), as a WriteProperty request would.
 */
static bool bv_present_value_write(
    uint32_t object_instance, BACNET_BINARY_PV value, uint8_t priority)
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    wp_data.object_type = OBJECT_BINARY_VALUE;
    wp_data.object_instance = object_instance;
    wp_data.object_property = PROP_PRESENT_VALUE;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = priority;
    wp_data.application_data_len =
        encode_application_enumerated(wp_data.application_data, value);

    return Binary_Value_Write_Property(&wp_data);
}

/**
 * @brief Helper: relinquish PROP_PRESENT_VALUE at the given priority by
 *  writing NULL, as a client releasing its command would.
 */
static bool
bv_present_value_relinquish(uint32_t object_instance, uint8_t priority)
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    wp_data.object_type = OBJECT_BINARY_VALUE;
    wp_data.object_instance = object_instance;
    wp_data.object_property = PROP_PRESENT_VALUE;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = priority;
    wp_data.application_data_len =
        encode_application_null(wp_data.application_data);

    return Binary_Value_Write_Property(&wp_data);
}

/**
 * @brief Helper: read a property through Binary_Value_Read_Property() and
 *  decode the single application-tagged value it encoded.
 */
static void bv_property_read_decode(
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    int len = 0;
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };

    rp_data.object_type = OBJECT_BINARY_VALUE;
    rp_data.object_instance = object_instance;
    rp_data.object_property = object_property;
    rp_data.array_index = array_index;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = Binary_Value_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, value);
    zassert_true(len > 0, NULL);
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bv_tests, testBinary_Value)
#else
static void testBinary_Value(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };

    Binary_Value_Init();
    object_instance = Binary_Value_Create(object_instance);
    count = Binary_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Binary_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_BINARY_VALUE, object_instance, Binary_Value_Property_Lists,
        Binary_Value_Read_Property, Binary_Value_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Binary_Value_Name_Set, Binary_Value_Name_ASCII);
    status = Binary_Value_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test Binary Value Writable_Property_List and Write_Enabled APIs
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bv_tests, testBinary_Value_Writable_Properties)
#else
static void testBinary_Value_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;
    uint32_t i = 0;
    bool has_object_name = false;
    bool has_description = false;

    Binary_Value_Init();
    zassert_not_equal(Binary_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* write-enabled (default): list starts with PROP_PRESENT_VALUE */
    zassert_true(Binary_Value_Write_Enabled(instance), NULL);
    Binary_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);
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
    /* write-disabled: list skips PROP_PRESENT_VALUE */
    Binary_Value_Write_Disable(instance);
    zassert_false(Binary_Value_Write_Enabled(instance), NULL);
    Binary_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_false(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    Binary_Value_Write_Enable(instance);
    zassert_true(Binary_Value_Write_Enabled(instance), NULL);
    Binary_Value_Writable_Property_List(instance, &properties);
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* unknown instance: must return a valid list, not NULL/garbage */
    properties = NULL;
    Binary_Value_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* NULL properties pointer: must not crash */
    Binary_Value_Writable_Property_List(instance, NULL);

    Binary_Value_Delete(instance);
    Binary_Value_Cleanup();
}
/**
 * @brief Test that changes to Present_Value (priority-array),
 *  Relinquish_Default, and Status_Flags (Out_Of_Service, Reliability/Fault)
 *  set the Change_Of_Value flag when the observable value actually changes,
 *  and that redundant writes do not
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bv_tests, testBinary_Value_COV)
#else
static void testBinary_Value_COV(void)
#endif
{
    const uint32_t instance = 654;
    bool status = false;

    Binary_Value_Init();
    zassert_not_equal(Binary_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* all priorities relinquished: Present_Value tracks Relinquish_Default,
       which defaults to BINARY_INACTIVE */
    Binary_Value_Change_Of_Value_Clear(instance);
    zassert_false(Binary_Value_Change_Of_Value(instance), NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_INACTIVE, NULL);

    /* Present_Value via the priority array must set Changed */
    Binary_Value_Change_Of_Value_Clear(instance);
    status =
        Binary_Value_Present_Value_Priority_Set(instance, BINARY_ACTIVE, 8);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_ACTIVE, NULL);
    zassert_true(Binary_Value_Change_Of_Value(instance), NULL);

    /* relinquishing that priority reverts to Relinquish_Default and must
       set Changed again */
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Present_Value_Relinquish(instance, 8);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_INACTIVE, NULL);
    zassert_true(Binary_Value_Change_Of_Value(instance), NULL);

    /* a Relinquish_Default change must set Changed */
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Relinquish_Default_Set(instance, BINARY_ACTIVE);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_ACTIVE, NULL);
    zassert_true(Binary_Value_Change_Of_Value(instance), NULL);

    /* setting the same effective value again must not set Changed */
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Relinquish_Default_Set(instance, BINARY_ACTIVE);
    zassert_true(status, NULL);
    zassert_false(Binary_Value_Change_Of_Value(instance), NULL);

    /* Status_Flags: an Out_Of_Service change must set Changed */
    Binary_Value_Change_Of_Value_Clear(instance);
    Binary_Value_Out_Of_Service_Set(instance, true);
    zassert_true(Binary_Value_Change_Of_Value(instance), NULL);

    /* setting the same Out_Of_Service value again must not set Changed */
    Binary_Value_Change_Of_Value_Clear(instance);
    Binary_Value_Out_Of_Service_Set(instance, true);
    zassert_false(Binary_Value_Change_Of_Value(instance), NULL);
    Binary_Value_Out_Of_Service_Set(instance, false);

    /* Status_Flags: a Fault change via Reliability must set Changed */
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Reliability_Set(instance, RELIABILITY_NO_SENSOR);
    zassert_true(status, NULL);
    zassert_true(Binary_Value_Change_Of_Value(instance), NULL);

    /* setting the same reliability again must not set Changed */
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Reliability_Set(instance, RELIABILITY_NO_SENSOR);
    zassert_true(status, NULL);
    zassert_false(Binary_Value_Change_Of_Value(instance), NULL);

    Binary_Value_Delete(instance);
    Binary_Value_Cleanup();
}

/**
 * @brief Test that the commandable Present_Value is resolved from the
 *  highest active priority slot, that relinquishing it exposes the
 *  next-highest active slot, and that Priority_Array and
 *  Current_Command_Priority report the same state to a client
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bv_tests, testBinary_Value_Priority_Array)
#else
static void testBinary_Value_Priority_Array(void)
#endif
{
    bool status = false;
    const uint32_t instance = 321;
    const uint8_t low_priority = 10;
    const uint8_t high_priority = 5;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    Binary_Value_Init();
    zassert_not_equal(Binary_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* a new object commands nothing: every slot is relinquished and
       Current_Command_Priority is NULL */
    zassert_equal(Binary_Value_Present_Value_Priority(instance), 0, NULL);
    bv_property_read_decode(
        instance, PROP_CURRENT_COMMAND_PRIORITY, BACNET_ARRAY_ALL, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_NULL, NULL);

    /* a lower (higher-numbered) priority command drives Present_Value */
    status = bv_present_value_write(instance, BINARY_INACTIVE, low_priority);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_INACTIVE, NULL);
    zassert_equal(
        Binary_Value_Present_Value_Priority(instance), low_priority, NULL);
    zassert_false(
        Binary_Value_Priority_Array_Relinquished(instance, low_priority), NULL);
    zassert_true(
        Binary_Value_Priority_Array_Relinquished(instance, high_priority),
        NULL);

    /* a higher (lower-numbered) priority command takes over */
    status = bv_present_value_write(instance, BINARY_ACTIVE, high_priority);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_ACTIVE, NULL);
    zassert_equal(
        Binary_Value_Present_Value_Priority(instance), high_priority, NULL);
    zassert_equal(
        Binary_Value_Priority_Array_Value(instance, high_priority),
        BINARY_ACTIVE, NULL);
    bv_property_read_decode(
        instance, PROP_CURRENT_COMMAND_PRIORITY, BACNET_ARRAY_ALL, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_UNSIGNED_INT, NULL);
    zassert_equal(value.type.Unsigned_Int, high_priority, NULL);

    /* Priority_Array encodes a commanded slot as its value, and an
       uncommanded slot as NULL */
    bv_property_read_decode(
        instance, PROP_PRIORITY_ARRAY, high_priority, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_ENUMERATED, NULL);
    zassert_equal(value.type.Enumerated, BINARY_ACTIVE, NULL);
    bv_property_read_decode(
        instance, PROP_PRIORITY_ARRAY, BACNET_MAX_PRIORITY, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_NULL, NULL);

    /* relinquishing the higher priority exposes the lower one again */
    status = bv_present_value_relinquish(instance, high_priority);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_INACTIVE, NULL);
    zassert_equal(
        Binary_Value_Present_Value_Priority(instance), low_priority, NULL);

    /* relinquishing the last active slot leaves nothing commanded */
    status = bv_present_value_relinquish(instance, low_priority);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value_Priority(instance), 0, NULL);
    zassert_true(
        Binary_Value_Priority_Array_Relinquished(instance, low_priority), NULL);
    bv_property_read_decode(
        instance, PROP_CURRENT_COMMAND_PRIORITY, BACNET_ARRAY_ALL, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_NULL, NULL);

    Binary_Value_Delete(instance);
    Binary_Value_Cleanup();
}

/**
 * @brief Test that a WriteProperty of Relinquish_Default reports an
 *  out-of-range enumerated value as an error instead of silently
 *  acknowledging it, and that an accepted value becomes the fallback
 *  Present_Value once every priority slot is relinquished
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bv_tests, testBinary_Value_Relinquish_Default_Write)
#else
static void testBinary_Value_Relinquish_Default_Write(void)
#endif
{
    bool status = false;
    const uint32_t instance = 987;
    const uint8_t priority = 8;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    Binary_Value_Init();
    zassert_not_equal(Binary_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    wp_data.object_type = OBJECT_BINARY_VALUE;
    wp_data.object_instance = instance;
    wp_data.object_property = PROP_RELINQUISH_DEFAULT;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    /* an out-of-range enumerated value is rejected, and leaves
       Relinquish_Default at its default of BINARY_INACTIVE */
    wp_data.application_data_len =
        encode_application_enumerated(wp_data.application_data, BINARY_PV_MAX);
    status = Binary_Value_Write_Property(&wp_data);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
    zassert_equal(
        Binary_Value_Relinquish_Default(instance), BINARY_INACTIVE, NULL);

    /* an in-range value is accepted and, with no slot commanded, becomes
       the effective Present_Value */
    wp_data.application_data_len =
        encode_application_enumerated(wp_data.application_data, BINARY_ACTIVE);
    status = Binary_Value_Write_Property(&wp_data);
    zassert_true(
        status, "error code=%s", bactext_error_code_name(wp_data.error_code));
    zassert_equal(
        Binary_Value_Relinquish_Default(instance), BINARY_ACTIVE, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_ACTIVE, NULL);

    /* a commanded slot overrides Relinquish_Default until relinquished */
    status = bv_present_value_write(instance, BINARY_INACTIVE, priority);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_INACTIVE, NULL);
    status = bv_present_value_relinquish(instance, priority);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_ACTIVE, NULL);

    Binary_Value_Delete(instance);
    Binary_Value_Cleanup();
}

/**
 * @brief Test that the priority-less Binary_Value_Present_Value_Set()
 *  writes wherever the commandable Present_Value is resolved from: the
 *  slot of the active command, or Relinquish_Default when every slot is
 *  relinquished. Either way the Present_Value must become the value given
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bv_tests, testBinary_Value_Present_Value_Set)
#else
static void testBinary_Value_Present_Value_Set(void)
#endif
{
    bool status = false;
    const uint32_t instance = 135;
    const uint32_t invalid_instance = instance + 1;
    const uint8_t priority = 5;

    Binary_Value_Init();
    zassert_not_equal(Binary_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* nothing commanded: the value lands on Relinquish_Default, without
       claiming a priority slot, and the Present_Value follows */
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Present_Value_Set(instance, BINARY_ACTIVE);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_ACTIVE, NULL);
    zassert_equal(
        Binary_Value_Relinquish_Default(instance), BINARY_ACTIVE, NULL);
    zassert_equal(Binary_Value_Present_Value_Priority(instance), 0, NULL);
    zassert_true(Binary_Value_Change_Of_Value(instance), NULL);

    /* setting the same value again must not raise a spurious
       Change_Of_Value */
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Present_Value_Set(instance, BINARY_ACTIVE);
    zassert_true(status, NULL);
    zassert_false(Binary_Value_Change_Of_Value(instance), NULL);

    /* with a command active, the value lands in the commanded slot */
    status = Binary_Value_Relinquish_Default_Set(instance, BINARY_INACTIVE);
    zassert_true(status, NULL);
    status = bv_present_value_write(instance, BINARY_INACTIVE, priority);
    zassert_true(status, NULL);
    zassert_equal(
        Binary_Value_Present_Value_Priority(instance), priority, NULL);

    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Present_Value_Set(instance, BINARY_ACTIVE);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_ACTIVE, NULL);
    zassert_equal(
        Binary_Value_Present_Value_Priority(instance), priority, NULL);
    zassert_equal(
        Binary_Value_Priority_Array_Value(instance, priority), BINARY_ACTIVE,
        NULL);
    zassert_true(Binary_Value_Change_Of_Value(instance), NULL);
    /* no further slot is claimed, and Relinquish_Default stays untouched
       for the client that owns it */
    zassert_true(
        Binary_Value_Priority_Array_Relinquished(instance, BACNET_MAX_PRIORITY),
        NULL);
    zassert_equal(
        Binary_Value_Relinquish_Default(instance), BINARY_INACTIVE, NULL);

    /* relinquishing the commanded slot falls back to Relinquish_Default */
    status = bv_present_value_relinquish(instance, priority);
    zassert_true(status, NULL);
    zassert_equal(Binary_Value_Present_Value_Priority(instance), 0, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_INACTIVE, NULL);

    /* an out-of-range value is rejected with no side effect, whether the
       write would have gone to Relinquish_Default ... */
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Present_Value_Set(instance, BINARY_PV_MAX);
    zassert_false(status, NULL);
    zassert_equal(
        Binary_Value_Relinquish_Default(instance), BINARY_INACTIVE, NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_INACTIVE, NULL);
    zassert_false(Binary_Value_Change_Of_Value(instance), NULL);

    /* ... or to the commanded slot */
    status = bv_present_value_write(instance, BINARY_ACTIVE, priority);
    zassert_true(status, NULL);
    Binary_Value_Change_Of_Value_Clear(instance);
    status = Binary_Value_Present_Value_Set(instance, BINARY_PV_MAX);
    zassert_false(status, NULL);
    zassert_equal(
        Binary_Value_Priority_Array_Value(instance, priority), BINARY_ACTIVE,
        NULL);
    zassert_equal(Binary_Value_Present_Value(instance), BINARY_ACTIVE, NULL);
    zassert_false(Binary_Value_Change_Of_Value(instance), NULL);

    /* an unknown instance is rejected */
    status = Binary_Value_Present_Value_Set(invalid_instance, BINARY_ACTIVE);
    zassert_false(status, NULL);

    Binary_Value_Delete(instance);
    Binary_Value_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bv_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bv_tests, ztest_unit_test(testBinary_Value),
        ztest_unit_test(testBinary_Value_Writable_Properties),
        ztest_unit_test(testBinary_Value_COV),
        ztest_unit_test(testBinary_Value_Priority_Array),
        ztest_unit_test(testBinary_Value_Relinquish_Default_Write),
        ztest_unit_test(testBinary_Value_Present_Value_Set));

    ztest_run_test_suite(bv_tests);
}
#endif
