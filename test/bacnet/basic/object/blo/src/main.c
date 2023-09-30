/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date September 2023
 *
 * SPDX-License-Identifier: MIT
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
    BACNET_APPLICATION_DATA_VALUE value = {0};
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    const uint32_t instance = 123;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    bool status = false;
    unsigned index;
    uint16_t milliseconds = 10;

    Binary_Lighting_Output_Init();
    Binary_Lighting_Output_Create(instance);
    status = Binary_Lighting_Output_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Binary_Lighting_Output_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_LIGHTING_OUTPUT;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Binary_Lighting_Output_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) >= 0) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Binary_Lighting_Output_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, 
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len >= 0) {
            test_len = bacapp_decode_known_property(rpdata.application_data,
                len, &value, rpdata.object_type, rpdata.object_property);
            if (rpdata.object_property != PROP_PRIORITY_ARRAY) {
                zassert_equal(len, test_len, "property '%s': failed to decode!\n",
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
                zassert_not_equal(wpdata.error_code,
                    ERROR_CODE_UNKNOWN_PROPERTY,
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
        zassert_not_equal(len, BACNET_STATUS_ERROR, 
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len > 0) {
            test_len = bacapp_decode_application_data(rpdata.application_data,
                (uint8_t)rpdata.application_data_len, &value);
            zassert_equal(len, test_len, "property '%s': failed to decode!\n",
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
                zassert_not_equal(wpdata.error_code,
                    ERROR_CODE_UNKNOWN_PROPERTY,
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
    /* check the dimming/ramping/stepping engine*/
    Binary_Lighting_Output_Timer(instance, milliseconds);
    /* check the delete function */
    status = Binary_Lighting_Output_Delete(instance);
    zassert_true(status, NULL);

    return;
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(blo_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(blo_tests,
     ztest_unit_test(testBinaryLightingOutput)
     );

    ztest_run_test_suite(blo_tests);
}
#endif
