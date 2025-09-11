/**
 * @file
 * @brief test BACnet Device object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/device.h>
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test ReadProperty API
 */
static void test_Device_Data_Sharing(void)
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = 1234, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Device_Init(NULL);
    count = Device_Count();
    zassert_true(count > 0, NULL);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = Device_Index_To_Instance(0);
    Device_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Device_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len > 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value, rpdata.object_type, rpdata.object_property);
            if ((test_len < len) && (rpdata.array_index == BACNET_ARRAY_ALL)) {
                /* this is one element of an array - assume it is accurate */
                len = test_len;
            }
            zassert_equal(
                test_len, len, "property '%s': ReadProperty decode failure!\n",
                bactext_property_name(rpdata.object_property));
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Device_Write_Property(&wpdata);
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
        len = Device_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len > 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value, rpdata.object_type, rpdata.object_property);
            zassert_equal(
                test_len, len, "property '%s': ReadProperty decode failure!\n",
                bactext_property_name(rpdata.object_property));
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Device_Write_Property(&wpdata);
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
}

/**
 * @brief Test basic API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(device_tests, testDevice)
#else
static void testDevice(void)
#endif
{
    bool status = false;
    const char *name = "Patricia";
    BACNET_REINITIALIZE_DEVICE_DATA rd_data;

    status = Device_Set_Object_Instance_Number(0);
    zassert_equal(Device_Object_Instance_Number(), 0, NULL);
    zassert_true(status, NULL);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    zassert_equal(Device_Object_Instance_Number(), BACNET_MAX_INSTANCE, NULL);
    zassert_true(status, NULL);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE / 2);
    zassert_equal(
        Device_Object_Instance_Number(), (BACNET_MAX_INSTANCE / 2), NULL);
    zassert_true(status, NULL);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE + 1);
    zassert_not_equal(
        Device_Object_Instance_Number(), (BACNET_MAX_INSTANCE + 1), NULL);
    zassert_false(status, NULL);

    Device_Set_System_Status(STATUS_NON_OPERATIONAL, true);
    zassert_equal(Device_System_Status(), STATUS_NON_OPERATIONAL, NULL);

    zassert_equal(Device_Vendor_Identifier(), BACNET_VENDOR_ID, NULL);

    Device_Set_Model_Name(name, strlen(name));
    zassert_equal(strcmp(Device_Model_Name(), name), 0, NULL);

    /* Reinitialize with no device password */
    rd_data.error_class = ERROR_CLASS_DEVICE;
    rd_data.error_code = ERROR_CODE_SUCCESS;
    rd_data.state = BACNET_REINIT_COLDSTART;
    characterstring_init_ansi(&rd_data.password, NULL);
    status = Device_Reinitialize_Password_Set(NULL);
    status = Device_Reinitialize(&rd_data);
    zassert_true(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_DEVICE, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_SUCCESS, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));
    /* Reinitialize with device valid password, service no password */
    status = Device_Reinitialize_Password_Set("valid");
    zassert_true(status, NULL);
    status = characterstring_init_ansi(&rd_data.password, NULL);
    zassert_true(status, NULL);
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SECURITY, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_PASSWORD_FAILURE, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));
    /* Reinitialize with device valid password, service invalid password */
    status = characterstring_init_ansi(&rd_data.password, "invalid");
    zassert_true(status, NULL);
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SECURITY, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_PASSWORD_FAILURE, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));
    /* Reinitialize with device valid password, service valid password */
    characterstring_init_ansi(&rd_data.password, "valid");
    status = Device_Reinitialize(&rd_data);
    zassert_true(status, NULL);
    /* Reinitialize with device valid password, service too long password */
    characterstring_init_ansi(&rd_data.password, "abcdefghijklmnopqrstuvwxyz");
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SERVICES, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_PARAMETER_OUT_OF_RANGE, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));
    /* Reinitialize with device no password, unsupported state */
    status = Device_Reinitialize_Password_Set(NULL);
    zassert_true(status, NULL);
    rd_data.state = BACNET_REINIT_MAX;
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SERVICES, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_PARAMETER_OUT_OF_RANGE, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(device_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        device_tests, ztest_unit_test(testDevice),
        ztest_unit_test(test_Device_Data_Sharing));

    ztest_run_test_suite(device_tests);
}
#endif
