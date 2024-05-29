/**
 * @file
 * @brief Unit test for BACnetObjectPropertyReference and
 * BACnetDeviceObjectReference and BACnetDeviceObjectPropertyReference encode
 * and decode API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2023
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacdevobjpropref.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testDevObjPropRef(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *data)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE test_data;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    int null_len = 0;

    /* encode */
    null_len = bacapp_encode_device_obj_property_ref(NULL, data);
    len = bacapp_encode_device_obj_property_ref(apdu, data);
    zassert_equal(null_len, len, "null_len=%d len=%d", null_len, len);
    /* add a closing tag at the end of the apdu to verify proper handling
       when that is encountered in real packets */
    encode_closing_tag(&apdu[len], 3);
    /* decode */
    null_len = bacnet_device_object_property_reference_decode(apdu, len, NULL);
    test_len =
        bacnet_device_object_property_reference_decode(apdu, len, &test_data);
    zassert_equal(null_len, len, "null_len=%d len=%d", null_len, len);
    zassert_equal(test_len, len, "test_len=%d len=%d", test_len, len);
    zassert_equal(
        data->objectIdentifier.instance, test_data.objectIdentifier.instance,
        NULL);
    zassert_equal(
        data->objectIdentifier.type, test_data.objectIdentifier.type, NULL);
    zassert_equal(data->propertyIdentifier, test_data.propertyIdentifier, NULL);
    if (data->arrayIndex != BACNET_ARRAY_ALL) {
        zassert_equal(data->arrayIndex, test_data.arrayIndex, NULL);
    } else {
        zassert_equal(test_data.arrayIndex, BACNET_ARRAY_ALL, NULL);
    }
    if (data->deviceIdentifier.type == OBJECT_DEVICE) {
        zassert_equal(
            data->deviceIdentifier.instance,
            test_data.deviceIdentifier.instance, NULL);
        zassert_equal(
            data->deviceIdentifier.type, test_data.deviceIdentifier.type, NULL);
    } else {
        zassert_equal(
            test_data.deviceIdentifier.instance, BACNET_NO_DEV_ID, NULL);
        zassert_equal(
            test_data.deviceIdentifier.type, BACNET_NO_DEV_TYPE, NULL);
    }
    while (--len) {
        test_len = bacnet_device_object_property_reference_decode(
            apdu, len, &test_data);
        if ((len > 0) && (test_data.arrayIndex == BACNET_ARRAY_ALL)) {
            /* special case when optional portion is exactly missing */
        } else if (
            (len > 0) &&
            (test_data.deviceIdentifier.type == BACNET_NO_DEV_TYPE)) {
            /* special case when optional portion is exactly missing */
        } else {
            zassert_true(test_len <= 0, "test_len=%d len=%d", test_len, len);
        }
    }
    test_len = bacnet_device_object_property_reference_decode(
        NULL, sizeof(apdu), &test_data);
    zassert_true(test_len <= 0, "test_len=%d len=%d", test_len, len);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdevobjpropref_tests, testDevIdPropRef)
#else
static void testDevIdPropRef(void)
#endif
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE data;

    /* everything encoded */
    data.objectIdentifier.instance = 0x1234;
    data.objectIdentifier.type = 15;
    data.propertyIdentifier = 25;
    data.arrayIndex = 0x5678;
    data.deviceIdentifier.instance = 0x4343;
    data.deviceIdentifier.type = OBJECT_DEVICE;
    testDevObjPropRef(&data);
    /* optional array */
    data.objectIdentifier.instance = 0x1234;
    data.objectIdentifier.type = 15;
    data.propertyIdentifier = 25;
    data.arrayIndex = BACNET_ARRAY_ALL;
    data.deviceIdentifier.instance = 0x4343;
    data.deviceIdentifier.type = OBJECT_DEVICE;
    testDevObjPropRef(&data);
    /* optional device ID */
    data.objectIdentifier.instance = 0x1234;
    data.objectIdentifier.type = 15;
    data.propertyIdentifier = 25;
    data.arrayIndex = 1;
    data.deviceIdentifier.instance = 0;
    data.deviceIdentifier.type = BACNET_NO_DEV_TYPE;
    testDevObjPropRef(&data);
    /* optional array + optional device ID */
    data.objectIdentifier.instance = 0x1234;
    data.objectIdentifier.type = 15;
    data.propertyIdentifier = 25;
    data.arrayIndex = BACNET_ARRAY_ALL;
    data.deviceIdentifier.instance = 0;
    data.deviceIdentifier.type = BACNET_NO_DEV_TYPE;
    testDevObjPropRef(&data);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdevobjpropref_tests, testDevIdRef)
#else
static void testDevIdRef(void)
#endif
{
    BACNET_DEVICE_OBJECT_REFERENCE data;
    BACNET_DEVICE_OBJECT_REFERENCE test_data;
    uint8_t apdu[MAX_APDU];
    int len;
    int test_len;
    int null_len;

    data.deviceIdentifier.instance = 0x4343;
    data.deviceIdentifier.type = OBJECT_DEVICE;
    null_len = bacapp_encode_device_obj_ref(NULL, &data);
    len = bacapp_encode_device_obj_ref(apdu, &data);
    zassert_equal(null_len, len, NULL);
    test_len = bacnet_device_object_reference_decode(apdu, len, &test_data);
    zassert_equal(test_len, len, NULL);
    null_len = bacnet_device_object_reference_decode(apdu, len, NULL);
    zassert_equal(test_len, null_len, NULL);
    zassert_equal(
        data.deviceIdentifier.instance, test_data.deviceIdentifier.instance,
        NULL);
    zassert_equal(
        data.deviceIdentifier.type, test_data.deviceIdentifier.type, NULL);
    while (--len) {
        test_len = bacnet_device_object_reference_decode(apdu, len, &test_data);
        zassert_true(test_len <= 0, NULL);
    }
    test_len =
        bacnet_device_object_reference_decode(NULL, sizeof(apdu), &test_data);
    zassert_true(test_len <= 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdevobjpropref_tests, testObjPropRef)
#else
static void testObjPropRef(void)
#endif
{
    BACNET_OBJECT_PROPERTY_REFERENCE data;
    BACNET_OBJECT_PROPERTY_REFERENCE test_data;
    uint8_t apdu[MAX_APDU];
    uint8_t tag_number = 1;
    int len;
    int test_len;
    int null_len;

    data.object_identifier.instance = 12345;
    data.object_identifier.type = OBJECT_ANALOG_VALUE;
    data.property_identifier = PROP_PRESENT_VALUE;
    data.property_array_index = BACNET_ARRAY_ALL;
    null_len = bacapp_encode_obj_property_ref(NULL, &data);
    len = bacapp_encode_obj_property_ref(apdu, &data);
    zassert_equal(null_len, len, NULL);
    test_len = bacapp_decode_obj_property_ref(apdu, len, &test_data);
    zassert_equal(test_len, len, NULL);
    zassert_equal(
        data.object_identifier.type, test_data.object_identifier.type, NULL);
    zassert_equal(
        data.object_identifier.instance, test_data.object_identifier.instance,
        NULL);
    zassert_equal(
        data.property_identifier, test_data.property_identifier, NULL);
    zassert_equal(
        data.property_array_index, test_data.property_array_index, NULL);
    while (--len) {
        test_len = bacapp_decode_obj_property_ref(apdu, len, &test_data);
        zassert_true(test_len <= 0, NULL);
    }
    /* context */
    null_len = bacapp_encode_context_obj_property_ref(NULL, tag_number, &data);
    len = bacapp_encode_context_obj_property_ref(apdu, tag_number, &data);
    zassert_equal(null_len, len, NULL);
    test_len = bacapp_decode_context_obj_property_ref(
        apdu, len, tag_number, &test_data);
    zassert_equal(test_len, len, "len=%d test_len=%d", len, test_len);
    zassert_equal(
        data.object_identifier.type, test_data.object_identifier.type, NULL);
    zassert_equal(
        data.object_identifier.instance, test_data.object_identifier.instance,
        NULL);
    zassert_equal(
        data.property_identifier, test_data.property_identifier, NULL);
    zassert_equal(
        data.property_array_index, test_data.property_array_index, NULL);
    while (--len) {
        test_len = bacapp_decode_context_obj_property_ref(
            apdu, len, tag_number, &test_data);
        zassert_true(test_len <= 0, "test_len=%d len=%d", test_len, len);
    }
    null_len = bacapp_decode_context_obj_property_ref(
        NULL, sizeof(apdu), tag_number, &test_data);
    zassert_true(test_len <= 0, "test_len=%d len=%d", test_len, len);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacdevobjpropref_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacdevobjpropref_tests, ztest_unit_test(testDevIdPropRef),
        ztest_unit_test(testDevIdRef), ztest_unit_test(testObjPropRef));

    ztest_run_test_suite(bacdevobjpropref_tests);
}
#endif
