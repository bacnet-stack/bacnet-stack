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

/**
 * Encode a property reference for the device object.
 * Add an opening tag, encode the property and finally
 * add a closing tag as well.
 *
 * @param apdu  Pointer to the buffer to encode to.
 * @param tag_number  Tag number.
 * @param value  Pointer to the object property reference,
 *               used for encoding.
 *
 * @return Bytes encoded or 0 on failure.
 */
int bacapp_encode_context_device_obj_property_ref(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int len;
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;

        len = bacapp_encode_device_obj_property_ref(&apdu[apdu_len], value);
        apdu_len += len;

        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * Encode a property reference for the device object.
 *
 * BACnetDeviceObjectPropertyReference ::= SEQUENCE {
 *  object-identifier       [0] BACnetObjectIdentifier,
 *  property-identifier     [1] BACnetPropertyIdentifier,
 *  property-array-index    [2] Unsigned OPTIONAL,
 *      -- used only with array datatype
 *      -- if omitted with an array then
 *      -- the entire array is referenced
 *  device-identifier       [3] BACnetObjectIdentifier OPTIONAL
 * }
 *
 * @param apdu  Pointer to the buffer to encode to.
 * @param value  Pointer to the object property reference,
 *               used for encoding.
 *
 * @return Bytes encoded.
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
            value->deviceIdentifier.type, value->deviceIdentifier.instance);
        apdu_len += len;
    }
    return apdu_len;
}

/**
 * Decode a property reference of a device object.
 *
 * BACnetDeviceObjectPropertyReference ::= SEQUENCE {
 *  object-identifier       [0] BACnetObjectIdentifier,
 *  property-identifier     [1] BACnetPropertyIdentifier,
 *  property-array-index    [2] Unsigned OPTIONAL,
 *      -- used only with array datatype
 *      -- if omitted with an array then
 *      -- the entire array is referenced
 *  device-identifier       [3] BACnetObjectIdentifier OPTIONAL
 * }
 *
 * @param apdu  Pointer to the buffer containing the data to decode.
 * @param value  Pointer to the object property reference,
 *               used to decode the data into.
 *
 * @return Bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacapp_decode_device_obj_property_ref(
    uint8_t *apdu, BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int len;
    int apdu_len = 0;
    uint32_t enumValue = 0;

    /* object-identifier       [0] BACnetObjectIdentifier */
    len =
        decode_context_object_id(&apdu[apdu_len], 0,
            &value->objectIdentifier.type,
            &value->objectIdentifier.instance);
    if (len == BACNET_STATUS_ERROR) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    /* property-identifier     [1] BACnetPropertyIdentifier */
    len =
        decode_context_enumerated(&apdu[apdu_len], 1, &enumValue);
    if (len == BACNET_STATUS_ERROR) {
        return BACNET_STATUS_ERROR;
    }
    value->propertyIdentifier = (BACNET_PROPERTY_ID)enumValue;
    apdu_len += len;
    /* property-array-index    [2] Unsigned OPTIONAL */
    if (decode_is_context_tag(&apdu[apdu_len], 2) &&
        !decode_is_closing_tag(&apdu[apdu_len])) {
        len = decode_context_unsigned(
                &apdu[apdu_len], 2, &value->arrayIndex);
        if (len == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
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

/**
 * Decode the opening tag and the property reference of
 * an device object and check for the closing tag as well.
 *
 * @param apdu  Pointer to the buffer containing the data to decode.
 * @param tag_number  Tag number
 * @param value  Pointer to the object property reference,
 *               used to decode the data into.
 *
 * @return Bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacapp_decode_context_device_obj_property_ref(
    uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_length =
            bacapp_decode_device_obj_property_ref(&apdu[len], value);
        if (section_length == BACNET_STATUS_ERROR) {
            len = BACNET_STATUS_ERROR;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                len = BACNET_STATUS_ERROR;
            }
        }
    } else {
        len = BACNET_STATUS_ERROR;
    }
    return len;
}

/**
 * Encode the opening tag and the device object reference
 * and finally for the closing tag as well.
 *
 * BACnetDeviceObjectReference ::= SEQUENCE {
 * device-identifier [0] BACnetObjectIdentifier OPTIONAL,
 * object-identifier [1] BACnetObjectIdentifier
 * }
 *
 * @param apdu  Pointer to the buffer used for encoding.
 * @param tag_number  Tag number
 * @param value  Pointer to the device object reference,
 *               used to encode.
 *
 * @return Bytes encoded or 0 on failure.
 */
int bacapp_encode_context_device_obj_ref(
    uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int len;
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;

        len = bacapp_encode_device_obj_ref(&apdu[apdu_len], value);
        apdu_len += len;

        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * Encode the device object reference.
 *
 * BACnetDeviceObjectReference ::= SEQUENCE {
 *  device-identifier [0] BACnetObjectIdentifier OPTIONAL,
 *  object-identifier [1] BACnetObjectIdentifier
 * }
 *
 * @param apdu  Pointer to the buffer used for encoding.
 * @param value  Pointer to the device object reference,
 *               used to encode.
 *
 * @return Bytes encoded or 0 on failure.
 */
int bacapp_encode_device_obj_ref(
    uint8_t * apdu,
    BACNET_DEVICE_OBJECT_REFERENCE * value)
{
    int len;
    int apdu_len = 0;

    /* device-identifier [0] BACnetObjectIdentifier OPTIONAL */
    /* Device id is optional so see if needed
     * (set type to BACNET_NO_DEV_TYPE or something other than OBJECT_DEVICE to
     * omit */
    if (value->deviceIdentifier.type == OBJECT_DEVICE) {
        len = encode_context_object_id(&apdu[apdu_len], 0,
            value->deviceIdentifier.type, value->deviceIdentifier.instance);
        apdu_len += len;
    }
    /* object-identifier [1] BACnetObjectIdentifier */
    len =
        encode_context_object_id(&apdu[apdu_len], 1,
            value->objectIdentifier.type, value->objectIdentifier.instance);
    apdu_len += len;

    return apdu_len;
}

/**
 * Decode the device object reference.
 *
 * BACnetDeviceObjectReference ::= SEQUENCE {
 *  device-identifier [0] BACnetObjectIdentifier OPTIONAL,
 *  object-identifier [1] BACnetObjectIdentifier
 * }
 *
 * @param apdu  Pointer to the buffer containing the data to decode.
 * @param value  Pointer to the object object reference,
 *               that shall be decoded.
 *
 * @return Bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacapp_decode_device_obj_ref(
    uint8_t * apdu,
    BACNET_DEVICE_OBJECT_REFERENCE * value)
{
    int len;
    int apdu_len = 0;

    /* device-identifier [0] BACnetObjectIdentifier OPTIONAL */
    if (decode_is_context_tag(&apdu[apdu_len], 0) &&
        !decode_is_closing_tag(&apdu[apdu_len])) {
        len =
            decode_context_object_id(&apdu[apdu_len], 0,
                &value->deviceIdentifier.type,
                &value->deviceIdentifier.instance);
        if (len == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else {
        value->deviceIdentifier.type = BACNET_NO_DEV_TYPE;
        value->deviceIdentifier.instance = BACNET_NO_DEV_ID;
    }
    /* object-identifier [1] BACnetObjectIdentifier */
    len =
        decode_context_object_id(&apdu[apdu_len], 1,
            &value->objectIdentifier.type,
            &value->objectIdentifier.instance);
    if (len == BACNET_STATUS_ERROR) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * Decode the context device object reference. Check for
 * an opening tag and a closing tag as well.
 *
 * @param apdu  Pointer to the buffer containing the data to decode.
 * @param tag_number  Tag number
 * @param value  Pointer to the context device object reference,
 *               that shall be decoded.
 *
 * @return Bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacapp_decode_context_device_obj_ref(
    uint8_t * apdu,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_REFERENCE * value)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_length = bacapp_decode_device_obj_ref(&apdu[len], value);
        if (section_length == BACNET_STATUS_ERROR) {
            len = BACNET_STATUS_ERROR;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                len = BACNET_STATUS_ERROR;
            }
        }
    } else {
        len = BACNET_STATUS_ERROR;
    }
    return len;
}
