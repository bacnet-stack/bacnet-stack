/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/lo.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lo_tests, testLightingOutput)
#else
static void testLightingOutput(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    const uint32_t instance = 123;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    bool status = false;
    unsigned index, count;
    uint16_t milliseconds = 10;
    uint32_t test_instance;
    const char *test_name = NULL;
    char *sample_name = "sample";
    BACNET_LIGHTING_COMMAND lighting_command = { 0 };

    Lighting_Output_Init();
    Lighting_Output_Create(instance);
    status = Lighting_Output_Valid_Instance(instance);
    zassert_true(status, NULL);
    status = Lighting_Output_Valid_Instance(BACNET_MAX_INSTANCE);
    zassert_false(status, NULL);
    index = Lighting_Output_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    count = Lighting_Output_Count();
    zassert_equal(count, 1, NULL);
    test_instance = Lighting_Output_Index_To_Instance(0);
    zassert_equal(test_instance, instance, NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_LIGHTING_OUTPUT;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Lighting_Output_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) >= 0) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Lighting_Output_Read_Property(&rpdata);
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
            status = Lighting_Output_Write_Property(&wpdata);
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
        len = Lighting_Output_Read_Property(&rpdata);
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
            status = Lighting_Output_Write_Property(&wpdata);
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
    len = Lighting_Output_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    wpdata.object_property = PROP_ALL;
    status = Lighting_Output_Write_Property(&wpdata);
    zassert_false(status, NULL);
    /* check the dimming/ramping/stepping engine*/
    Lighting_Output_Timer(instance, milliseconds);
    /* test the ASCII name get/set */
    status = Lighting_Output_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Lighting_Output_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Lighting_Output_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Lighting_Output_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);
    /* test local control API */
    lighting_command.operation = BACNET_LIGHTS_NONE;
    do {
        status =
            Lighting_Output_Lighting_Command_Set(instance, &lighting_command);
        if (status) {
            zassert_true(status, NULL);
        } else {
            printf(
                "lighting-command operation[%d] %s not supported.\n",
                lighting_command.operation,
                bactext_lighting_operation_name(lighting_command.operation));
        }
        if (lighting_command.operation == BACNET_LIGHTS_PROPRIETARY_MIN) {
            /* skip to make testing faster */
            lighting_command.operation = BACNET_LIGHTS_PROPRIETARY_MAX;
        } else if (lighting_command.operation == BACNET_LIGHTS_RESERVED_MIN) {
            /* skip to make testing faster */
            lighting_command.operation = BACNET_LIGHTS_RESERVED_MAX;
        } else {
            lighting_command.operation++;
        }
    } while (lighting_command.operation <= BACNET_LIGHTS_PROPRIETARY_MAX);

    /* check the delete function */
    status = Lighting_Output_Delete(instance);
    zassert_true(status, NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(lo_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(lo_tests, ztest_unit_test(testLightingOutput));

    ztest_run_test_suite(lo_tests);
}
#endif
