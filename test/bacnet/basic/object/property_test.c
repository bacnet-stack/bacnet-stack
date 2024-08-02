/**
 * @file
 * @brief Unit test for object property read/write
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2024
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/rp.h>
#include <bacnet/rpm.h>
#include <bacnet/wp.h>

/**
 * @brief Perform a read/write test on a property
 * @param rpdata [in,out] The structure to hold the read property request
 * @param read_property [in] The function to read the property
 * @param write_property [in] The function to write the property
 * @param skip_fail_property_list [in] The list of properties that
 *  are known to fail to decode after reading
 * @return true if the property was written successfully, false if not
 */
bool bacnet_object_property_write_test(
    BACNET_WRITE_PROPERTY_DATA *wpdata,
    write_property_function write_property,
    const int *skip_fail_property_list)
{
    bool status = false;
    int len = 0;
    int test_len = 0;

    (void)skip_fail_property_list;
    if (wpdata && write_property) {
        status = write_property(wpdata);
        if (!status) {
            /* verify WriteProperty property is known */
            zassert_not_equal(
                wpdata->error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                "property '%s': WriteProperty Unknown!\n",
                bactext_property_name(wpdata->object_property));
        }
    }

    return status;
}

/**
 * @brief Initialize the write property parameter structure data from the
 * read property parameter structure data and the length of the property value
 * @param wpdata [in,out] The structure to hold the write property request
 * @param rpdata [in] The structure to hold the read property request
 */
void bacnet_object_property_write_parameter_init(
    BACNET_WRITE_PROPERTY_DATA *wpdata,
    BACNET_READ_PROPERTY_DATA *rpdata,
    int len)
{
    if (wpdata && rpdata) {
        /* WriteProperty parameters */
        wpdata->object_type = rpdata->object_type;
        wpdata->object_instance = rpdata->object_instance;
        wpdata->object_property = rpdata->object_property;
        wpdata->array_index = rpdata->array_index;
        memcpy(&wpdata->application_data, rpdata->application_data, MAX_APDU);
        wpdata->application_data_len = len;
        wpdata->error_code = ERROR_CODE_SUCCESS;
    }
}

/**
 * @brief Perform a read/write test on a property
 * @param rpdata [in,out] The structure to hold the read property request
 * @param read_property [in] The function to read the property
 * @param write_property [in] The function to write the property
 * @param skip_fail_property_list [in] The list of properties that
 *  are known to fail to decode after reading
 * @return length of the property value that was read
 */
int bacnet_object_property_read_test(
    BACNET_READ_PROPERTY_DATA *rpdata,
    read_property_function read_property,
    const int *skip_fail_property_list)
{
    bool status = false;
    int len = 0;
    int test_len = 0;
    int apdu_len = 0;
    int read_len = 0;
    uint8_t *apdu;
    BACNET_ARRAY_INDEX array_index = 0;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    read_len = read_property(rpdata);
    if ((read_len == BACNET_STATUS_ERROR) &&
        (rpdata->error_class == ERROR_CLASS_PROPERTY) &&
        (rpdata->error_code == ERROR_CODE_READ_ACCESS_DENIED)) {
        /* read-only is a valid response for some properties */
    } else if (read_len > 0) {
        /* validate the data from the read request */
        apdu = rpdata->application_data;
        apdu_len = read_len;
        while (apdu_len) {
            len = bacapp_decode_known_property(
                apdu, apdu_len, &value, rpdata->object_type,
                rpdata->object_property);
            if (len > 0) {
                test_len += len;
                if ((len < apdu_len) &&
                    (rpdata->array_index == BACNET_ARRAY_ALL)) {
                    /* more data, therefore, this is an array */
                    array_index = 1;
                }
                if (array_index > 0) {
                    apdu += len;
                    apdu_len -= len;
                    array_index++;
                } else {
                    break;
                }
            } else {
                printf(
                    "property '%s': failed to decode! len=%d\n",
                    bactext_property_name(rpdata->object_property), len);
                break;
            }
        }
        if (read_len != test_len) {
            printf(
                "property '%s': failed to decode! %d!=%d\n",
                bactext_property_name(rpdata->object_property), test_len,
                read_len);
        }
        if (property_list_member(
                skip_fail_property_list, rpdata->object_property)) {
            /* FIXME: known fail to decode */
            test_len = read_len;
        }
        zassert_true(test_len == read_len, NULL);
    } else if (read_len == 0) {
        /* empty response is valid for some properties */
    } else {
        zassert_not_equal(
            read_len, BACNET_STATUS_ERROR, "property '%s': failed to read!\n",
            bactext_property_name(rpdata->object_property));
    }

    return len;
}

/**
 * @brief Test all the properties of an object for read/write
 *
 * @param object_type The type of object to test
 * @param object_instance The instance number of the object to test
 * @param property_list The function to get the list of properties
 * @param read_property The function to read the property
 * @param write_property The function to write the property
 * @param skip_fail_property_list The list of properties that
 *  are known to fail to decode after reading
 */
void bacnet_object_properties_read_write_test(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    rpm_property_lists_function property_list,
    read_property_function read_property,
    write_property_function write_property,
    const int *skip_fail_property_list)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    unsigned count = 0;
    int len = 0;
    bool status = false;

    /* ReadProperty parameters */
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = object_type;
    property_list(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
        rpdata.object_instance = object_instance;
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = bacnet_object_property_read_test(
            &rpdata, read_property, skip_fail_property_list);
        bacnet_object_property_write_parameter_init(&wpdata, &rpdata, len);
        bacnet_object_property_write_test(
            &wpdata, write_property, skip_fail_property_list);
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        bacnet_object_property_read_test(
            &rpdata, read_property, skip_fail_property_list);
        bacnet_object_property_write_parameter_init(&wpdata, &rpdata, len);
        bacnet_object_property_write_test(
            &wpdata, write_property, skip_fail_property_list);
        pOptional++;
    }
    while ((*pProprietary) != -1) {
        rpdata.object_property = *pProprietary;
        rpdata.array_index = BACNET_ARRAY_ALL;
        bacnet_object_property_read_test(
            &rpdata, read_property, skip_fail_property_list);
        bacnet_object_property_write_parameter_init(&wpdata, &rpdata, len);
        bacnet_object_property_write_test(
            &wpdata, write_property, skip_fail_property_list);
        pProprietary++;
    }
    /* check for unsupported property - use ALL */
    rpdata.object_property = PROP_ALL;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = read_property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    wpdata.object_property = PROP_ALL;
    wpdata.array_index = BACNET_ARRAY_ALL;
    if (write_property) {
        status = write_property(&wpdata);
        zassert_false(status, NULL);
    }
}
