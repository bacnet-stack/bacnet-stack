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
#include <bacnet/proplist.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */
static int32_t Proprietary_Properties[] = { 512, 513, -1 };
static uint8_t Proprietary_Serial_Number[16] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

/**
 * @brief Test implementation that returns the proprietary properties list.
 * @param object_type [in] The BACNET_OBJECT_TYPE of the object instance
 *  to fetch the property list for.
 * @param object_instance [in] The object instance number of the object
 *  to fetch the property list for.
 * @param pPropertyList [out] Pointer to a property list to be
 *  filled with the property list for this object instance.
 * @return True if the object instance is valid and the property list has been
 *  filled in, false if the object instance is not valid or the property list
 *  could not be filled in.
 */
static bool Property_List_Proprietary(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    const int32_t **property_list)
{
    (void)object_type;
    (void)object_instance;
    *property_list = Proprietary_Properties;
    return true;
}

/**
 * @brief WriteProperty handler for this objects proprietary properties.
 * For the given WriteProperty data, the application_data is loaded
 * or the error code and class are set and the return value is false.
 * @param  data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return false if an error is loaded, true if no errors
 */
static bool Write_Property_Proprietary(BACNET_WRITE_PROPERTY_DATA *data)
{
    bool status = false;
    int apdu_len = 0;
    uint8_t *apdu = NULL;
    size_t apdu_size = 0;
    BACNET_OCTET_STRING octet_value = { 0 };

    if ((data == NULL) || (data->application_data_len == 0)) {
        return false;
    }
    /* none of our proprietary properties are arrays */
    apdu = data->application_data;
    apdu_size = data->application_data_len;
    switch ((int)data->object_property) {
        case 512:
            apdu_len = bacnet_octet_string_application_decode(
                apdu, apdu_size, &octet_value);
            if (apdu_len > 0) {
                octetstring_copy_value(
                    Proprietary_Serial_Number,
                    sizeof(Proprietary_Serial_Number), &octet_value);
                status = true;
            } else if (apdu_len == 0) {
                data->error_class = ERROR_CLASS_PROPERTY;
                data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            } else {
                data->error_class = ERROR_CLASS_PROPERTY;
                data->error_code = ERROR_CODE_INVALID_DATA_ENCODING;
            }
            break;
        case 513:
        default:
            data->error_class = ERROR_CLASS_PROPERTY;
            data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return status;
}

/**
 * @brief ReadProperty handler for this objects proprietary properties.
 * For the given ReadProperty data, the application_data is loaded
 * or the error code and class are set and the return value is
 * BACNET_STATUS_ERROR.
 * @param  data - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return number of bytes in the reply 0..N, or BACNET_STATUS_ERROR
 */
static int Read_Property_Proprietary(BACNET_READ_PROPERTY_DATA *data)
{
    int apdu_len = 0;
    uint8_t *apdu = NULL;
    size_t apdu_size = 0;
    BACNET_OCTET_STRING octet_value = { 0 };
    BACNET_UNSIGNED_INTEGER unsigned_value = 42;

    if ((data == NULL) || (data->application_data == NULL) ||
        (data->application_data_len == 0)) {
        return 0;
    }
    /* none of our proprietary properties are arrays */
    if (data->array_index != BACNET_ARRAY_ALL) {
        data->error_class = ERROR_CLASS_PROPERTY;
        data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return BACNET_STATUS_ERROR;
    }
    apdu = data->application_data;
    apdu_size = data->application_data_len;
    switch ((int)data->object_property) {
        case 512:
            octetstring_init(
                &octet_value, Proprietary_Serial_Number,
                sizeof(Proprietary_Serial_Number));
            apdu_len = bacnet_octet_string_application_encode(
                apdu, apdu_size, &octet_value);
            break;
        case 513:
            apdu_len = bacnet_unsigned_application_encode(
                apdu, apdu_size, unsigned_value);
            break;
        default:
            data->error_class = ERROR_CLASS_PROPERTY;
            data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/**
 * @brief Test ReadProperty API
 */
static void test_Device_Data_Sharing(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    /* for decode value data */
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    struct special_property_list_t property_list = { 0 };
    const int32_t *pRequired = NULL;
    const int32_t *pOptional = NULL;
    const int32_t *pProprietary = NULL;
    unsigned count = 0;
    bool status = false;

    Device_Init(NULL);
    count = Device_Count();
    zassert_true(count > 0, NULL);

    /* add some proprietary properties */
    Device_Property_List_Proprietary_Callback_Set(Property_List_Proprietary);
    Device_Read_Property_Proprietary_Callback_Set(Read_Property_Proprietary);
    Device_Write_Property_Proprietary_Callback_Set(Write_Property_Proprietary);
    /* configure for ReadProperty test */
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = Device_Index_To_Instance(0);
    /* get the property lists */
    Device_Objects_Property_List(
        OBJECT_DEVICE, rpdata.object_instance, &property_list);
    pRequired = property_list.Required.pList;
    pOptional = property_list.Optional.pList;
    pProprietary = property_list.Proprietary.pList;
    /* test the ReadProperty and WriteProperty handling for every property */
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
    while ((*pProprietary) != -1) {
        rpdata.object_property = *pProprietary;
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
        pProprietary++;
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
    unsigned i, count, writable;
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    const int32_t *properties;
    struct special_property_list_t property_list;

    Device_Init(NULL);
    count = Device_Count();
    zassert_true(count > 0, NULL);
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

    count = Device_Object_List_Count();
    for (i = 1; i <= count; i++) {
        bool valid = false;

        valid =
            Device_Object_List_Identifier(i, &object_type, &object_instance);
        zassert_true(valid, "object-list[%u] is not valid", i);
        valid = Device_Valid_Object_Id(object_type, object_instance);
        zassert_true(valid, NULL);
        Device_Objects_Writable_Property_List(
            object_type, object_instance, &properties);
        zassert_not_null(properties, NULL);
        writable = property_list_count(properties);
        zassert_true(
            writable > 0, "%s-%u has no writable properties",
            bactext_object_type_name(object_type), object_instance);
        Device_Objects_Property_List(
            object_type, object_instance, &property_list);
        zassert_true(property_list.Required.count > 0, NULL);
    }

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
