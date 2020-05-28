/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 John Minack

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/timestamp.h"
#include "bacnet/bacdevobjpropref.h"

/** @file bacdevobjpropref.c  BACnet Application Device Object (Property)
 * Reference */

int bacapp_encode_context_device_obj_property_ref(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(&apdu[apdu_len], tag_number);
    apdu_len += len;

    len = bacapp_encode_device_obj_property_ref(&apdu[apdu_len], value);
    apdu_len += len;

    len = encode_closing_tag(&apdu[apdu_len], tag_number);
    apdu_len += len;

    return apdu_len;
}

/* BACnetDeviceObjectPropertyReference ::= SEQUENCE {
    object-identifier       [0] BACnetObjectIdentifier,
    property-identifier     [1] BACnetPropertyIdentifier,
    property-array-index    [2] Unsigned OPTIONAL,
        -- used only with array datatype
        -- if omitted with an array then
        -- the entire array is referenced
    device-identifier       [3] BACnetObjectIdentifier OPTIONAL
}
*/
int bacapp_encode_device_obj_property_ref(
    uint8_t *apdu, BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int len;
    int apdu_len = 0;

    /* object-identifier       [0] BACnetObjectIdentifier */
    len = encode_context_object_id(&apdu[apdu_len], 0,
        value->objectIdentifier.type, value->objectIdentifier.instance);
    apdu_len += len;
    /* property-identifier     [1] BACnetPropertyIdentifier */
    len = encode_context_enumerated(
        &apdu[apdu_len], 1, value->propertyIdentifier);
    apdu_len += len;
    /* property-array-index    [2] Unsigned OPTIONAL */
    /* Check if needed before inserting */
    if (value->arrayIndex != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(&apdu[apdu_len], 2, value->arrayIndex);
        apdu_len += len;
    }
    /* device-identifier       [3] BACnetObjectIdentifier OPTIONAL */
    /* Likewise, device id is optional so see if needed
     * (set type to BACNET_NO_DEV_TYPE or something other than OBJECT_DEVICE to
     * omit */
    if (value->deviceIdentifier.type == OBJECT_DEVICE) {
        len = encode_context_object_id(&apdu[apdu_len], 3,
            value->deviceIdentifier.type,
            value->deviceIdentifier.instance);
        apdu_len += len;
    }
    return apdu_len;
}

/* BACnetDeviceObjectPropertyReference ::= SEQUENCE {
    object-identifier       [0] BACnetObjectIdentifier,
    property-identifier     [1] BACnetPropertyIdentifier,
    property-array-index    [2] Unsigned OPTIONAL,
        -- used only with array datatype
        -- if omitted with an array then
        -- the entire array is referenced
    device-identifier       [3] BACnetObjectIdentifier OPTIONAL
}
*/
int bacapp_decode_device_obj_property_ref(
    uint8_t *apdu, BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int len;
    int apdu_len = 0;
    uint32_t enumValue;

    /* object-identifier       [0] BACnetObjectIdentifier */
    if (-1 ==
        (len = decode_context_object_id(&apdu[apdu_len], 0,
             &value->objectIdentifier.type,
             &value->objectIdentifier.instance))) {
        return -1;
    }
    apdu_len += len;
    /* property-identifier     [1] BACnetPropertyIdentifier */
    if (-1 ==
        (len = decode_context_enumerated(&apdu[apdu_len], 1, &enumValue))) {
        return -1;
    }
    value->propertyIdentifier = (BACNET_PROPERTY_ID)enumValue;
    apdu_len += len;
    /* property-array-index    [2] Unsigned OPTIONAL */
    if (decode_is_context_tag(&apdu[apdu_len], 2) &&
        !decode_is_closing_tag(&apdu[apdu_len])) {
        if (-1 ==
            (len = decode_context_unsigned(
                 &apdu[apdu_len], 2, &value->arrayIndex))) {
            return -1;
        }
        apdu_len += len;
    } else {
        value->arrayIndex = BACNET_ARRAY_ALL;
    }
    /* device-identifier       [3] BACnetObjectIdentifier OPTIONAL */
    if (decode_is_context_tag(&apdu[apdu_len], 3) &&
        !decode_is_closing_tag(&apdu[apdu_len])) {
        if (-1 ==
            (len = decode_context_object_id(&apdu[apdu_len], 3,
                 &value->deviceIdentifier.type,
                 &value->deviceIdentifier.instance))) {
            return -1;
        }
        apdu_len += len;
    } else {
        value->deviceIdentifier.type = BACNET_NO_DEV_TYPE;
        value->deviceIdentifier.instance = BACNET_NO_DEV_ID;
    }

    return apdu_len;
}

int bacapp_decode_context_device_obj_property_ref(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_length =
            bacapp_decode_device_obj_property_ref(&apdu[len], value);

        if (section_length == -1) {
            len = -1;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                len = -1;
            }
        }
    } else {
        len = -1;
    }
    return len;
}

/* BACnetDeviceObjectReference ::= SEQUENCE {
    device-identifier [0] BACnetObjectIdentifier OPTIONAL,
    object-identifier [1] BACnetObjectIdentifier
}*/
int bacapp_encode_context_device_obj_ref(
    uint8_t *apdu, uint8_t tag_number, BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(&apdu[apdu_len], tag_number);
    apdu_len += len;

    len = bacapp_encode_device_obj_ref(&apdu[apdu_len], value);
    apdu_len += len;

    len = encode_closing_tag(&apdu[apdu_len], tag_number);
    apdu_len += len;

    return apdu_len;
}

/* BACnetDeviceObjectReference ::= SEQUENCE {
    device-identifier [0] BACnetObjectIdentifier OPTIONAL,
    object-identifier [1] BACnetObjectIdentifier
}*/
int bacapp_encode_device_obj_ref(
    uint8_t *apdu, BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int len;
    int apdu_len = 0;

    /* device-identifier [0] BACnetObjectIdentifier OPTIONAL */
    /* Device id is optional so see if needed
     * (set type to BACNET_NO_DEV_TYPE or something other than OBJECT_DEVICE to
     * omit */
    if (value->deviceIdentifier.type == OBJECT_DEVICE) {
        len = encode_context_object_id(&apdu[apdu_len], 0,
            value->deviceIdentifier.type,
            value->deviceIdentifier.instance);
        apdu_len += len;
    }
    /* object-identifier [1] BACnetObjectIdentifier */
    len = encode_context_object_id(&apdu[apdu_len], 1,
        value->objectIdentifier.type, value->objectIdentifier.instance);
    apdu_len += len;

    return apdu_len;
}

/* BACnetDeviceObjectReference ::= SEQUENCE {
    device-identifier [0] BACnetObjectIdentifier OPTIONAL,
    object-identifier [1] BACnetObjectIdentifier
}*/
int bacapp_decode_device_obj_ref(
    uint8_t *apdu, BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int len;
    int apdu_len = 0;

    /* device-identifier [0] BACnetObjectIdentifier OPTIONAL */
    if (decode_is_context_tag(&apdu[apdu_len], 0) &&
        !decode_is_closing_tag(&apdu[apdu_len])) {
        if (-1 ==
            (len = decode_context_object_id(&apdu[apdu_len], 0,
                 &value->deviceIdentifier.type,
                 &value->deviceIdentifier.instance))) {
            return -1;
        }
        apdu_len += len;
    } else {
        value->deviceIdentifier.type = BACNET_NO_DEV_TYPE;
        value->deviceIdentifier.instance = BACNET_NO_DEV_ID;
    }
    /* object-identifier [1] BACnetObjectIdentifier */
    if (-1 ==
        (len = decode_context_object_id(&apdu[apdu_len], 1,
             &value->objectIdentifier.type,
             &value->objectIdentifier.instance))) {
        return -1;
    }
    apdu_len += len;

    return apdu_len;
}

int bacapp_decode_context_device_obj_ref(
    uint8_t *apdu, uint8_t tag_number, BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_length = bacapp_decode_device_obj_ref(&apdu[len], value);

        if (section_length == -1) {
            len = -1;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                len = -1;
            }
        }
    } else {
        len = -1;
    }
    return len;
}

#ifdef BAC_TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"
static void testDevObjPropRef(
    Test *pTest, BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *inData)
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
    ct_test(pTest, outLen == inLen);
    ct_test(pTest,
        inData->objectIdentifier.instance == outData.objectIdentifier.instance);
    ct_test(
        pTest, inData->objectIdentifier.type == outData.objectIdentifier.type);
    ct_test(pTest, inData->propertyIdentifier == outData.propertyIdentifier);
    if (inData->arrayIndex != BACNET_ARRAY_ALL) {
        ct_test(pTest, inData->arrayIndex == outData.arrayIndex);
    } else {
        ct_test(pTest, outData.arrayIndex == BACNET_ARRAY_ALL);
    }
    if (inData->deviceIdentifier.type == OBJECT_DEVICE) {
        ct_test(pTest,
            inData->deviceIdentifier.instance ==
                outData.deviceIdentifier.instance);
        ct_test(pTest,
            inData->deviceIdentifier.type == outData.deviceIdentifier.type);
    } else {
        ct_test(pTest, outData.deviceIdentifier.instance == BACNET_NO_DEV_ID);
        ct_test(pTest, outData.deviceIdentifier.type == BACNET_NO_DEV_TYPE);
    }
}

static void testDevIdPropRef(Test *pTest)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE inData;

    /* everything encoded */
    inData.objectIdentifier.instance = 0x1234;
    inData.objectIdentifier.type = 15;
    inData.propertyIdentifier = 25;
    inData.arrayIndex = 0x5678;
    inData.deviceIdentifier.instance = 0x4343;
    inData.deviceIdentifier.type = OBJECT_DEVICE;
    testDevObjPropRef(pTest, &inData);
    /* optional array */
    inData.objectIdentifier.instance = 0x1234;
    inData.objectIdentifier.type = 15;
    inData.propertyIdentifier = 25;
    inData.arrayIndex = BACNET_ARRAY_ALL;
    inData.deviceIdentifier.instance = 0x4343;
    inData.deviceIdentifier.type = OBJECT_DEVICE;
    testDevObjPropRef(pTest, &inData);
    /* optional device ID */
    inData.objectIdentifier.instance = 0x1234;
    inData.objectIdentifier.type = 15;
    inData.propertyIdentifier = 25;
    inData.arrayIndex = 1;
    inData.deviceIdentifier.instance = 0;
    inData.deviceIdentifier.type = BACNET_NO_DEV_TYPE;
    testDevObjPropRef(pTest, &inData);
    /* optional array + optional device ID */
    inData.objectIdentifier.instance = 0x1234;
    inData.objectIdentifier.type = 15;
    inData.propertyIdentifier = 25;
    inData.arrayIndex = BACNET_ARRAY_ALL;
    inData.deviceIdentifier.instance = 0;
    inData.deviceIdentifier.type = BACNET_NO_DEV_TYPE;
    testDevObjPropRef(pTest, &inData);
}

static void testDevIdRef(Test *pTest)
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
    ct_test(pTest, outLen == inLen);
    ct_test(pTest,
        inData.deviceIdentifier.instance == outData.deviceIdentifier.instance);
    ct_test(
        pTest, inData.deviceIdentifier.type == outData.deviceIdentifier.type);
}

void testBACnetDeviceObjectPropertyReference(Test *pTest)
{
    bool rc;

    /* individual tests */
    rc = ct_addTestFunction(pTest, testDevIdPropRef);
    assert(rc);
    rc = ct_addTestFunction(pTest, testDevIdRef);
    assert(rc);
}

#ifdef TEST_DEV_ID_PROP_REF
#include <assert.h>
int main(void)
{
    Test *pTest;

    pTest = ct_create("BACnet Prop Ref", NULL);
    testBACnetDeviceObjectPropertyReference(pTest);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}

#endif /* TEST_DEV_ID_PROP_REF */
#endif /* BAC_TEST */
