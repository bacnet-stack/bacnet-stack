/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacdevobjpropref.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testDevObjPropRef(
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *inData)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE outData;
    uint8_t buffer[MAX_APDU] = { 0 };
    int inLen = 0;
    int outLen = 0;

    /* encode */
    inLen = bacapp_encode_device_obj_property_ref(buffer, inData);
    /* add a closing tag at the end of the buffer to verify proper handling
       when that is encountered in real packets */
    encode_closing_tag(&buffer[inLen], 3);
    /* decode */
    outLen = bacapp_decode_device_obj_property_ref(buffer, &outData);
    zassert_equal(outLen, inLen, NULL);
    zassert_equal(
        inData->objectIdentifier.instance, outData.objectIdentifier.instance, NULL);
    zassert_equal(
        inData->objectIdentifier.type, outData.objectIdentifier.type, NULL);
    zassert_equal(inData->propertyIdentifier, outData.propertyIdentifier, NULL);
    if (inData->arrayIndex != BACNET_ARRAY_ALL) {
        zassert_equal(inData->arrayIndex, outData.arrayIndex, NULL);
    } else {
        zassert_equal(outData.arrayIndex, BACNET_ARRAY_ALL, NULL);
    }
    if (inData->deviceIdentifier.type == OBJECT_DEVICE) {
        zassert_equal(
            inData->deviceIdentifier.instance,
                outData.deviceIdentifier.instance, NULL);
        zassert_equal(
            inData->deviceIdentifier.type, outData.deviceIdentifier.type, NULL);
    } else {
        zassert_equal(outData.deviceIdentifier.instance, BACNET_NO_DEV_ID, NULL);
        zassert_equal(outData.deviceIdentifier.type, BACNET_NO_DEV_TYPE, NULL);
    }
}

static void testDevIdPropRef(void)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE inData;

    /* everything encoded */
    inData.objectIdentifier.instance = 0x1234;
    inData.objectIdentifier.type = 15;
    inData.propertyIdentifier = 25;
    inData.arrayIndex = 0x5678;
    inData.deviceIdentifier.instance = 0x4343;
    inData.deviceIdentifier.type = OBJECT_DEVICE;
    testDevObjPropRef(&inData);
    /* optional array */
    inData.objectIdentifier.instance = 0x1234;
    inData.objectIdentifier.type = 15;
    inData.propertyIdentifier = 25;
    inData.arrayIndex = BACNET_ARRAY_ALL;
    inData.deviceIdentifier.instance = 0x4343;
    inData.deviceIdentifier.type = OBJECT_DEVICE;
    testDevObjPropRef(&inData);
    /* optional device ID */
    inData.objectIdentifier.instance = 0x1234;
    inData.objectIdentifier.type = 15;
    inData.propertyIdentifier = 25;
    inData.arrayIndex = 1;
    inData.deviceIdentifier.instance = 0;
    inData.deviceIdentifier.type = BACNET_NO_DEV_TYPE;
    testDevObjPropRef(&inData);
    /* optional array + optional device ID */
    inData.objectIdentifier.instance = 0x1234;
    inData.objectIdentifier.type = 15;
    inData.propertyIdentifier = 25;
    inData.arrayIndex = BACNET_ARRAY_ALL;
    inData.deviceIdentifier.instance = 0;
    inData.deviceIdentifier.type = BACNET_NO_DEV_TYPE;
    testDevObjPropRef(&inData);
}

static void testDevIdRef(void)
{
    BACNET_DEVICE_OBJECT_REFERENCE inData;
    BACNET_DEVICE_OBJECT_REFERENCE outData;
    uint8_t buffer[MAX_APDU];
    int inLen;
    int outLen;

    inData.deviceIdentifier.instance = 0x4343;
    inData.deviceIdentifier.type = OBJECT_DEVICE;
    inLen = bacapp_encode_device_obj_ref(buffer, &inData);
    outLen = bacapp_decode_device_obj_ref(buffer, &outData);
    zassert_equal(outLen, inLen, NULL);
    zassert_equal(
        inData.deviceIdentifier.instance, outData.deviceIdentifier.instance, NULL);
    zassert_equal(
        inData.deviceIdentifier.type, outData.deviceIdentifier.type, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(bacdevobjpropref_tests,
     ztest_unit_test(testDevIdPropRef),
     ztest_unit_test(testDevIdRef)
     );

    ztest_run_test_suite(bacdevobjpropref_tests);
}
