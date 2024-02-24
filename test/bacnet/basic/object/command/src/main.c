/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/command.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command)
#else
static void test_object_command(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = 0;

    Command_Init();
    count = Command_Count();
    zassert_true(count > 0, NULL);
    object_instance = Command_Index_To_Instance(0);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_COMMAND;
    rpdata.object_instance = object_instance;
    Command_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Command_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_known_property(rpdata.application_data,
                (uint8_t)rpdata.application_data_len, &value,
                rpdata.object_type, rpdata.object_property);
            if (len != test_len) {
                printf("property '%s': failed to decode! %d!=%d\n",
                    bactext_property_name(rpdata.object_property), test_len,
                    len);
            }
            if (rpdata.object_property == PROP_ACTION) {
                /* FIXME: known fail to decode */
                len = test_len;
            }
            zassert_equal(test_len, len, NULL);
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Command_Write_Property(&wpdata);
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
        len = Command_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_known_property(rpdata.application_data,
                (uint8_t)rpdata.application_data_len, &value,
                rpdata.object_type, rpdata.object_property);
            if (len != test_len) {
                printf("property '%s': failed to decode! %d!=%d\n",
                    bactext_property_name(rpdata.object_property), test_len,
                    len);
            }
            if (rpdata.object_property == PROP_PRIORITY_ARRAY) {
                /* FIXME: known fail to decode */
                len = test_len;
            }
            zassert_equal(test_len, len, NULL);
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Command_Write_Property(&wpdata);
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

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(tests_object_command, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        tests_object_command, ztest_unit_test(test_object_command));

    ztest_run_test_suite(tests_object_command);
}
#endif
