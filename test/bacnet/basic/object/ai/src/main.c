/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @section LICENSE
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <float.h>
#include <math.h>
#include <zephyr/ztest.h>
#include <bacnet/bacapp.h>
#include <bacnet/bacstr.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/ai.h>
#include <bacnet/proplist.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ai_tests, testAnalogInput)
#else
static void testAnalogInput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };

    Analog_Input_Init();
    object_instance = Analog_Input_Create(object_instance);
    count = Analog_Input_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Analog_Input_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_ANALOG_INPUT, object_instance, Analog_Input_Property_Lists,
        Analog_Input_Read_Property, Analog_Input_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Analog_Input_Name_Set, Analog_Input_Name_ASCII);
    status = Analog_Input_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test Analog Input Writable_Property_List API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ai_tests, testAnalogInput_Writable_Properties)
#else
static void testAnalogInput_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Analog_Input_Init();
    zassert_not_equal(Analog_Input_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* valid instance: list is non-NULL and -1-terminated */
    Analog_Input_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* unknown instance: must still return a valid list, not NULL/garbage */
    properties = NULL;
    Analog_Input_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);

    /* NULL properties pointer: must not crash */
    Analog_Input_Writable_Property_List(instance, NULL);

    Analog_Input_Delete(instance);
    Analog_Input_Cleanup();
}

#if defined(INTRINSIC_REPORTING)
/**
 * @brief ReadProperty a property of an Analog Input object and decode it
 * @param object_instance - object-instance number of the object
 * @param object_property - property to be read
 * @param value - decoded property value
 */
static void Analog_Input_Property_Decode(
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    int len = 0, test_len = 0;

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ANALOG_INPUT;
    rpdata.object_instance = object_instance;
    rpdata.object_property = object_property;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Analog_Input_Read_Property(&rpdata);
    zassert_true(
        len > 0, "property '%s': failed to ReadProperty!\n",
        bactext_property_name(object_property));
    test_len = bacapp_decode_known_property(
        rpdata.application_data, len, value, rpdata.object_type,
        object_property);
    zassert_equal(
        len, test_len, "property '%s': failed to decode!\n",
        bactext_property_name(object_property));
}

/**
 * @brief Test Analog Input limit property get/set API
 */
#endif
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ai_tests, testAnalogInput_Limits)
#else
static void testAnalogInput_Limits(void)
#endif
{
#if defined(INTRINSIC_REPORTING)
    const uint32_t instance = 123;
    const uint32_t invalid_instance = instance + 1;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    bool status = false;

    Analog_Input_Init();
    zassert_not_equal(Analog_Input_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* a newly created object starts with the limits disabled and cleared */
    zassert_true(fabsf(Analog_Input_High_Limit(instance)) < FLT_EPSILON, NULL);
    zassert_true(fabsf(Analog_Input_Low_Limit(instance)) < FLT_EPSILON, NULL);
    zassert_true(fabsf(Analog_Input_Deadband(instance)) < FLT_EPSILON, NULL);
    zassert_equal(Analog_Input_Limit_Enable(instance), 0, NULL);

    /* set/get round trip, including negative and fractional values */
    status = Analog_Input_High_Limit_Set(instance, 90.0f);
    zassert_true(status, NULL);
    zassert_true(
        fabsf(Analog_Input_High_Limit(instance) - 90.0f) < FLT_EPSILON, NULL);
    status = Analog_Input_Low_Limit_Set(instance, -12.5f);
    zassert_true(status, NULL);
    zassert_true(
        fabsf(Analog_Input_Low_Limit(instance) - (-12.5f)) < FLT_EPSILON, NULL);
    status = Analog_Input_Deadband_Set(instance, 1.5f);
    zassert_true(status, NULL);
    zassert_true(
        fabsf(Analog_Input_Deadband(instance) - 1.5f) < FLT_EPSILON, NULL);

    /* limit-enable accepts each bit alone and both bits together */
    status = Analog_Input_Limit_Enable_Set(instance, EVENT_LOW_LIMIT_ENABLE);
    zassert_true(status, NULL);
    zassert_equal(
        Analog_Input_Limit_Enable(instance), EVENT_LOW_LIMIT_ENABLE, NULL);
    status = Analog_Input_Limit_Enable_Set(
        instance, EVENT_LOW_LIMIT_ENABLE | EVENT_HIGH_LIMIT_ENABLE);
    zassert_true(status, NULL);
    zassert_equal(
        Analog_Input_Limit_Enable(instance),
        EVENT_LOW_LIMIT_ENABLE | EVENT_HIGH_LIMIT_ENABLE, NULL);
    /* bits outside of the limit-enable bits are rejected, value unchanged */
    status = Analog_Input_Limit_Enable_Set(instance, (BACNET_LIMIT_ENABLE)0x04);
    zassert_false(status, NULL);
    zassert_equal(
        Analog_Input_Limit_Enable(instance),
        EVENT_LOW_LIMIT_ENABLE | EVENT_HIGH_LIMIT_ENABLE, NULL);
    status = Analog_Input_Limit_Enable_Set(instance, EVENT_HIGH_LIMIT_ENABLE);
    zassert_true(status, NULL);
    zassert_equal(
        Analog_Input_Limit_Enable(instance), EVENT_HIGH_LIMIT_ENABLE, NULL);

    /* the setters store the values used by the object properties */
    Analog_Input_Property_Decode(instance, PROP_HIGH_LIMIT, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_REAL, NULL);
    zassert_true(fabsf(value.type.Real - 90.0f) < FLT_EPSILON, NULL);
    Analog_Input_Property_Decode(instance, PROP_LOW_LIMIT, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_REAL, NULL);
    zassert_true(fabsf(value.type.Real - (-12.5f)) < FLT_EPSILON, NULL);
    Analog_Input_Property_Decode(instance, PROP_DEADBAND, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_REAL, NULL);
    zassert_true(fabsf(value.type.Real - 1.5f) < FLT_EPSILON, NULL);
    Analog_Input_Property_Decode(instance, PROP_LIMIT_ENABLE, &value);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_BIT_STRING, NULL);
    zassert_equal(bitstring_bits_used(&value.type.Bit_String), 2, NULL);
    zassert_false(bitstring_bit(&value.type.Bit_String, 0), NULL);
    zassert_true(bitstring_bit(&value.type.Bit_String, 1), NULL);

    /* unknown instance: getters return defaults and setters fail */
    zassert_true(
        fabsf(Analog_Input_High_Limit(invalid_instance)) < FLT_EPSILON, NULL);
    zassert_true(
        fabsf(Analog_Input_Low_Limit(invalid_instance)) < FLT_EPSILON, NULL);
    zassert_true(
        fabsf(Analog_Input_Deadband(invalid_instance)) < FLT_EPSILON, NULL);
    zassert_equal(Analog_Input_Limit_Enable(invalid_instance), 0, NULL);
    zassert_false(Analog_Input_High_Limit_Set(invalid_instance, 90.0f), NULL);
    zassert_false(Analog_Input_Low_Limit_Set(invalid_instance, -12.5f), NULL);
    zassert_false(Analog_Input_Deadband_Set(invalid_instance, 1.5f), NULL);
    zassert_false(
        Analog_Input_Limit_Enable_Set(
            invalid_instance, EVENT_HIGH_LIMIT_ENABLE),
        NULL);

    status = Analog_Input_Delete(instance);
    zassert_true(status, NULL);
    Analog_Input_Cleanup();
#else
    ztest_test_skip();
#endif
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(ai_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        ai_tests, ztest_unit_test(testAnalogInput),
        ztest_unit_test(testAnalogInput_Writable_Properties),
        ztest_unit_test(testAnalogInput_Limits));

    ztest_run_test_suite(ai_tests);
}
#endif
