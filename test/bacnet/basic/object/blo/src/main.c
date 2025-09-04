/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date September 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/blo.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testBinaryLightingOutput)
#else
static void testBinaryLightingOutput(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    const uint32_t instance = 123;
    uint32_t test_instance = 0;
    bool status = false;
    unsigned index;
    const char *test_name = NULL;
    char *sample_name = "sample";

    Binary_Lighting_Output_Init();
    test_instance = Binary_Lighting_Output_Create(instance);
    zassert_equal(test_instance, instance, NULL);
    status = Binary_Lighting_Output_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Binary_Lighting_Output_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_BINARY_LIGHTING_OUTPUT;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Binary_Lighting_Output_Property_Lists(
        &pRequired, &pOptional, &pProprietary);
    while ((*pRequired) >= 0) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Binary_Lighting_Output_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            if (rpdata.object_property != PROP_PRIORITY_ARRAY) {
                zassert_equal(
                    len, test_len, "property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Binary_Lighting_Output_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Binary_Lighting_Output_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            zassert_equal(
                len, test_len, "property '%s': failed to decode!\n",
                bactext_property_name(rpdata.object_property));
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Binary_Lighting_Output_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        }
        pOptional++;
    }
    /* check for unsupported property - use ALL */
    rpdata.object_property = PROP_ALL;
    len = Binary_Lighting_Output_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    status = Binary_Lighting_Output_Write_Property(&wpdata);
    zassert_false(status, NULL);
    /* test the ASCII name get/set */
    status = Binary_Lighting_Output_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Binary_Lighting_Output_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Binary_Lighting_Output_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Binary_Lighting_Output_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);
    /* check the delete function */
    status = Binary_Lighting_Output_Delete(instance);
    zassert_true(status, NULL);

    return;
}

static struct {
    uint32_t object_instance;
    BACNET_BINARY_LIGHTING_PV old_pv;
    BACNET_BINARY_LIGHTING_PV pv;
    uint32_t count;
} BLO_Value;
static void Binary_Lighting_Output_Write_Value_Handler(
    uint32_t object_instance,
    BACNET_BINARY_LIGHTING_PV old_value,
    BACNET_BINARY_LIGHTING_PV value)
{
    BLO_Value.object_instance = object_instance;
    BLO_Value.old_pv = old_value;
    BLO_Value.pv = value;
    BLO_Value.count++;
}

static struct {
    uint32_t object_instance;
    uint32_t count;
} BLO_Blink;
static void Binary_Lighting_Output_Blink_Warn_Handler(uint32_t object_instance)
{
    BLO_Blink.object_instance = object_instance;
    BLO_Blink.count++;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testBinaryLightingOutputBlink)
#else
static void testBinaryLightingOutputBlink(void)
#endif
{
    const uint32_t object_instance = 123;
    bool status = false;
    uint16_t milliseconds, milliseconds_elapsed;
    BACNET_BINARY_LIGHTING_PV pv, test_pv, expect_pv;
    unsigned test_priority;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };

    Binary_Lighting_Output_Init();
    Binary_Lighting_Output_Create(object_instance);
    status = Binary_Lighting_Output_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    Binary_Lighting_Output_Write_Value_Callback_Set(
        Binary_Lighting_Output_Write_Value_Handler);
    Binary_Lighting_Output_Blink_Warn_Callback_Set(
        Binary_Lighting_Output_Blink_Warn_Handler);
    /* check the blink warning engine at defaults */
    milliseconds_elapsed = 100;
    expect_pv = BINARY_LIGHTING_PV_OFF;
    Binary_Lighting_Output_Timer(object_instance, milliseconds_elapsed);
    zassert_equal(BLO_Blink.count, 0, "count=%u", BLO_Blink.count);
    zassert_equal(BLO_Value.count, 0, "count=%u", BLO_Value.count);
    zassert_equal(BLO_Value.pv, expect_pv, "value=%u", BLO_Value.pv);

    /* check WriteProperty properties */
    wpdata.object_type = OBJECT_BINARY_LIGHTING_OUTPUT;
    wpdata.object_instance = object_instance;
    wpdata.object_property = PROP_PRESENT_VALUE;
    wpdata.priority = BACNET_MAX_PRIORITY;
    wpdata.array_index = BACNET_ARRAY_ALL;
    wpdata.error_class = ERROR_CLASS_PROPERTY;
    wpdata.error_code = ERROR_CODE_SUCCESS;
    /* ON */
    pv = BINARY_LIGHTING_PV_ON;
    expect_pv = BINARY_LIGHTING_PV_ON;
    wpdata.application_data_len =
        encode_application_enumerated(wpdata.application_data, pv);
    status = Binary_Lighting_Output_Write_Property(&wpdata);
    zassert_true(status, NULL);
    milliseconds = 2000;
    milliseconds_elapsed = 100;
    while (milliseconds) {
        Binary_Lighting_Output_Timer(object_instance, milliseconds_elapsed);
        test_pv = Binary_Lighting_Output_Present_Value(object_instance);
        zassert_equal(expect_pv, test_pv, NULL);
        test_priority =
            Binary_Lighting_Output_Present_Value_Priority(object_instance);
        zassert_equal(
            wpdata.priority, test_priority, "priority=%u test_priority=%u",
            wpdata.priority, test_priority);
        zassert_equal(BLO_Blink.count, 0, NULL);
        zassert_equal(BLO_Value.count, 1, "count=%u", BLO_Value.count);
        zassert_equal(BLO_Value.pv, expect_pv, NULL);
        milliseconds -= milliseconds_elapsed;
    }
    /* OFF */
    pv = BINARY_LIGHTING_PV_OFF;
    expect_pv = BINARY_LIGHTING_PV_OFF;
    wpdata.application_data_len =
        encode_application_enumerated(wpdata.application_data, pv);
    status = Binary_Lighting_Output_Write_Property(&wpdata);
    zassert_true(status, NULL);
    milliseconds = 2000;
    milliseconds_elapsed = 100;
    while (milliseconds) {
        Binary_Lighting_Output_Timer(object_instance, milliseconds_elapsed);
        test_pv = Binary_Lighting_Output_Present_Value(object_instance);
        zassert_equal(expect_pv, test_pv, NULL);
        test_priority =
            Binary_Lighting_Output_Present_Value_Priority(object_instance);
        zassert_equal(
            wpdata.priority, test_priority, "priority=%u test_priority=%u",
            wpdata.priority, test_priority);
        zassert_equal(BLO_Blink.count, 0, NULL);
        zassert_equal(BLO_Value.count, 2, "count=%u", BLO_Value.count);
        zassert_equal(BLO_Value.pv, expect_pv, NULL);
        milliseconds -= milliseconds_elapsed;
    }
    /* WARN - already off */
    pv = BINARY_LIGHTING_PV_WARN;
    expect_pv = BINARY_LIGHTING_PV_OFF;
    wpdata.application_data_len =
        encode_application_enumerated(wpdata.application_data, pv);
    status = Binary_Lighting_Output_Write_Property(&wpdata);
    zassert_true(status, NULL);
    milliseconds = 2000;
    milliseconds_elapsed = 100;
    while (milliseconds) {
        Binary_Lighting_Output_Timer(object_instance, milliseconds_elapsed);
        test_pv = Binary_Lighting_Output_Present_Value(object_instance);
        zassert_equal(expect_pv, test_pv, "pv=%u", test_pv);
        test_priority =
            Binary_Lighting_Output_Present_Value_Priority(object_instance);
        zassert_equal(
            wpdata.priority, test_priority, "priority=%u test_priority=%u",
            wpdata.priority, test_priority);
        zassert_equal(BLO_Blink.count, 0, NULL);
        zassert_equal(BLO_Value.count, 2, "count=%u", BLO_Value.count);
        zassert_equal(BLO_Value.pv, expect_pv, NULL);
        milliseconds -= milliseconds_elapsed;
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(blo_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        blo_tests, ztest_unit_test(testBinaryLightingOutput),
        ztest_unit_test(testBinaryLightingOutputBlink));

    ztest_run_test_suite(blo_tests);
}
#endif
