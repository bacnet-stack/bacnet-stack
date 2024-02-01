/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 John Minack
 Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>

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
 * @param apdu Pointer to the encode buffer, or NULL for length
 * @param tag_number Tag number.
 * @param value Pointer to the object property reference, used for encoding.
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
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = bacapp_encode_device_obj_property_ref(apdu, value);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
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
 * @param apdu  Pointer to the encode buffer, or NULL for length
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

    if (!value) {
        return apdu_len;
    }
    /* object-identifier       [0] BACnetObjectIdentifier */
    len = encode_context_object_id(apdu, 0, value->objectIdentifier.type,
        value->objectIdentifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* property-identifier     [1] BACnetPropertyIdentifier */
    len = encode_context_enumerated(apdu, 1, value->propertyIdentifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* property-array-index    [2] Unsigned OPTIONAL */
    /* Check if needed before inserting */
    if (value->arrayIndex != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 2, value->arrayIndex);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* device-identifier       [3] BACnetObjectIdentifier OPTIONAL */
    /* Likewise, device id is optional so see if needed
     * (set type to BACNET_NO_DEV_TYPE or something other than OBJECT_DEVICE to
     * omit */
    if (value->deviceIdentifier.type == OBJECT_DEVICE) {
        len = encode_context_object_id(apdu, 3, value->deviceIdentifier.type,
            value->deviceIdentifier.instance);
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
 * @param apdu  Pointer to the buffer containing the encoded value
 * @param apdu_size Size of the buffer containing the encoded value
 * @param value Pointer to the structure which contains the decoded value
 *
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacnet_device_object_property_reference_decode(uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int apdu_len = 0;
    int len = 0;
    uint32_t len_value_type = 0;
    BACNET_UNSIGNED_INTEGER array_index = 0;
    BACNET_OBJECT_TYPE object_type = 0;
    uint32_t object_instance = 0;
    uint32_t property_identifier = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* object-identifier [0] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(&apdu[apdu_len], apdu_size - apdu_len,
        0, &object_type, &object_instance);
    if (len > 0) {
        apdu_len += len;
        if (value) {
            value->objectIdentifier.instance = object_instance;
            value->objectIdentifier.type = object_type;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-identifier [1] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &property_identifier);
    if (len > 0) {
        apdu_len += len;
        if (value) {
            value->propertyIdentifier = property_identifier;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-array-index [2] Unsigned OPTIONAL */
    if (bacnet_is_context_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &len, &len_value_type)) {
        apdu_len += len;
        len = bacnet_unsigned_decode(&apdu[apdu_len], apdu_size - apdu_len,
            len_value_type, &array_index);
        if (len > 0) {
            apdu_len += len;
            if (value) {
                value->arrayIndex = array_index;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        /* OPTIONAL - skip apdu_len increment */
        if (value) {
            value->arrayIndex = BACNET_ARRAY_ALL;
        }
    }
    /* device-identifier [3] BACnetObjectIdentifier OPTIONAL */
    if (bacnet_is_context_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len, &len_value_type)) {
        apdu_len += len;
        len = bacnet_object_id_decode(&apdu[apdu_len], apdu_size - apdu_len,
            len_value_type, &object_type, &object_instance);
        if (len > 0) {
            apdu_len += len;
            if (value) {
                value->deviceIdentifier.type = object_type;
                value->deviceIdentifier.instance = object_instance;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        /* OPTIONAL - skip apdu_len increment */
        if (value) {
            value->deviceIdentifier.type = BACNET_NO_DEV_TYPE;
            value->deviceIdentifier.instance = BACNET_NO_DEV_ID;
        }
    }

    return apdu_len;
}

/**
 * Decode the opening tag and the property reference of
 * an device object and check for the closing tag as well.
 *
 * @param apdu Pointer to the buffer containing the encoded value
 * @param apdu_size Size of the buffer containing the encoded value
 * @param tag_number  Tag number
 * @param value Pointer to the structure which contains the decoded value
 *
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacnet_device_object_property_reference_context_decode(uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int apdu_len = 0;
    int len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
        len = bacnet_device_object_property_reference_decode(
            &apdu[apdu_len], apdu_size - apdu_len, value);
        if (len > 0) {
            apdu_len += len;
            if (bacnet_is_closing_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare the complex data of value1 and value2
 * @param value1 - value 1 structure
 * @param value2 - value 2 structure
 * @return true if the values are the same
 */
bool bacnet_device_object_property_reference_same(
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value1,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value2)
{
    bool status = false;

    if (value1 && value2) {
        if ((value1->arrayIndex == value2->arrayIndex) &&
            (value1->deviceIdentifier.instance ==
                value2->deviceIdentifier.instance) &&
            (value1->deviceIdentifier.type == value2->deviceIdentifier.type) &&
            (value1->objectIdentifier.instance ==
                value2->objectIdentifier.instance) &&
            (value1->objectIdentifier.type == value2->objectIdentifier.type) &&
            (value1->propertyIdentifier == value2->propertyIdentifier)) {
            status = true;
        }
    }

    return status;
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
 * @deprecated Use bacnet_device_object_property_reference_decode() instead
 */
int bacapp_decode_device_obj_property_ref(
    uint8_t *apdu, BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    return bacnet_device_object_property_reference_decode(
        apdu, MAX_APDU, value);
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
 * @deprecated Use bacnet_device_object_property_reference_context_decode()
 * instead
 */
int bacapp_decode_context_device_obj_property_ref(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    return bacnet_device_object_property_reference_context_decode(
        apdu, MAX_APDU, tag_number, value);
}

/**
 * Encode the opening tag and the device object reference
 * and finally for the closing tag as well.
 *
 * BACnetDeviceObjectReference ::= SEQUENCE {
 *   device-identifier [0] BACnetObjectIdentifier OPTIONAL,
 *   object-identifier [1] BACnetObjectIdentifier
 * }
 *
 * @param apdu Pointer to the encode buffer, or NULL for length
 * @param tag_number Tag number
 * @param value Pointer to the device object reference, used to encode.
 *
 * @return Bytes encoded or 0 on failure.
 */
int bacapp_encode_context_device_obj_ref(
    uint8_t *apdu, uint8_t tag_number, BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int len;
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = bacapp_encode_device_obj_ref(apdu, value);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * Encode the device object reference.
 *
 * BACnetDeviceObjectReference ::= SEQUENCE {
 *   device-identifier [0] BACnetObjectIdentifier OPTIONAL,
 *   object-identifier [1] BACnetObjectIdentifier
 * }
 *
 * @param apdu Pointer to the encode buffer, or NULL for length
 * @param value Pointer to the device object reference, used to encode.
 *
 * @return Bytes encoded or 0 on failure.
 */
int bacapp_encode_device_obj_ref(
    uint8_t *apdu, BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int len;
    int apdu_len = 0;

    if (value) {
        /* device-identifier [0] BACnetObjectIdentifier OPTIONAL */
        /* Device id is optional so determine if needed and
           set type to BACNET_NO_DEV_TYPE or something other
           than OBJECT_DEVICE to omit */
        if (value->deviceIdentifier.type == OBJECT_DEVICE) {
            len = encode_context_object_id(apdu, 0,
                value->deviceIdentifier.type, value->deviceIdentifier.instance);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
        /* object-identifier [1] BACnetObjectIdentifier */
        len = encode_context_object_id(apdu, 1, value->objectIdentifier.type,
            value->objectIdentifier.instance);
        apdu_len += len;
    }

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
 * @param apdu  Pointer to the buffer containing the encoded value
 * @param apdu_size Size of the buffer containing the encoded value
 * @param value Pointer to the structure containing the decoded value
 *
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacnet_device_object_reference_decode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int len;
    int apdu_len = 0;
    uint32_t len_value_type = 0;
    BACNET_OBJECT_TYPE object_type = 0;
    uint32_t object_instance = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* device-identifier [0] BACnetObjectIdentifier OPTIONAL */
    if (bacnet_is_context_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len, &len_value_type)) {
        apdu_len += len;
        len = bacnet_object_id_decode(&apdu[apdu_len], apdu_size - apdu_len,
            len_value_type, &object_type, &object_instance);
        if (len > 0) {
            apdu_len += len;
            if (value) {
                value->deviceIdentifier.instance = object_instance;
                value->deviceIdentifier.type = object_type;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        /* OPTIONAL - skip apdu_len increment */
        value->deviceIdentifier.type = BACNET_NO_DEV_TYPE;
        value->deviceIdentifier.instance = BACNET_NO_DEV_ID;
    }
    /* object-identifier [1] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(&apdu[apdu_len], apdu_size - apdu_len,
        1, &object_type, &object_instance);
    if (len > 0) {
        apdu_len += len;
        if (value) {
            value->objectIdentifier.instance = object_instance;
            value->objectIdentifier.type = object_type;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * Decode the context device object reference. Check for
 * an opening tag and a closing tag as well.
 *
 * @param apdu Pointer to the buffer containing the encoded value
 * @param apdu_size Size of the buffer containing the encoded value
 * @param tag_number Tag number
 * @param value Pointer to the structure containing the decoded value
 *
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacnet_device_object_reference_context_decode(uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int apdu_len = 0;
    int len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
        len = bacnet_device_object_reference_decode(
            &apdu[apdu_len], apdu_size - apdu_len, value);
        if (len > 0) {
            apdu_len += len;
            if (bacnet_is_closing_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare the complex data of value1 and value2
 * @param value1 - value 1 structure
 * @param value2 - value 2 structure
 * @return true if the values are the same
 */
bool bacnet_device_object_reference_same(BACNET_DEVICE_OBJECT_REFERENCE *value1,
    BACNET_DEVICE_OBJECT_REFERENCE *value2)
{
    bool status = false;

    if (value1 && value2) {
        if ((value1->deviceIdentifier.instance ==
                value2->deviceIdentifier.instance) &&
            (value1->deviceIdentifier.type == value2->deviceIdentifier.type) &&
            (value1->objectIdentifier.instance ==
                value2->objectIdentifier.instance) &&
            (value1->objectIdentifier.type == value2->objectIdentifier.type)) {
            status = true;
        }
    }

    return status;
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
 * @deprecated Use bacnet_device_object_reference_decode() instead.
 */
int bacapp_decode_device_obj_ref(
    uint8_t *apdu, BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    return bacnet_device_object_reference_decode(apdu, MAX_APDU, value);
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
 * @deprecated Use bacnet_device_object_reference_context_decode() instead.
 */
int bacapp_decode_context_device_obj_ref(
    uint8_t *apdu, uint8_t tag_number, BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    return bacnet_device_object_reference_context_decode(
        apdu, MAX_APDU, tag_number, value);
}

/**
 * @brief Encode a BACnetObjectPropertyReference
 *
 *   BACnetObjectPropertyReference ::= SEQUENCE {
 *       object-identifier    [0] BACnetObjectIdentifier,
 *       property-identifier  [1] BACnetPropertyIdentifier,
 *       property-array-index [2] Unsigned OPTIONAL
 *       -- used only with array datatype
 *       -- if omitted with an array the entire array is referenced
 *   }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param reference - BACnetObjectPropertyReference
 * @return length of the APDU buffer
 */
int bacapp_encode_obj_property_ref(
    uint8_t *apdu, BACNET_OBJECT_PROPERTY_REFERENCE *reference)
{
    int len = 0;
    int apdu_len = 0;

    if (!reference) {
        return 0;
    }
    if (reference->object_identifier.type == OBJECT_NONE) {
        return 0;
    }
    len = encode_context_object_id(apdu, 0, reference->object_identifier.type,
        reference->object_identifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_enumerated(apdu, 1, reference->property_identifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (reference->property_array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 2, reference->property_array_index);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the BACnetObjectPropertyReference as Context Tagged
 * @param apdu - the APDU buffer
 * @param tag_number - context tag number to be encoded
 * @param reference - BACnetObjectPropertyReference to encode
 * @return length of the APDU buffer
 */
int bacapp_encode_context_obj_property_ref(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_OBJECT_PROPERTY_REFERENCE *reference)
{
    int len = 0;
    int apdu_len = 0;

    if (reference && (reference->object_identifier.type == OBJECT_NONE)) {
        return 0;
    }
    len = encode_opening_tag(apdu, tag_number);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacapp_encode_obj_property_ref(apdu, reference);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, tag_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a BACnetObjectPropertyReference
 *
 *   BACnetObjectPropertyReference ::= SEQUENCE {
 *       object-identifier    [0] BACnetObjectIdentifier,
 *       property-identifier  [1] BACnetPropertyIdentifier,
 *       property-array-index [2] Unsigned OPTIONAL
 *       -- used only with array datatype
 *       -- if omitted with an array the entire array is referenced
 *   }
 *
 * @param apdu  Pointer to the buffer containing the encoded value
 * @param apdu_size Size of the buffer containing the encoded value
 * @param reference - BACnetObjectPropertyReference to decode into
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacapp_decode_obj_property_ref(uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_OBJECT_PROPERTY_REFERENCE *reference)
{
    int apdu_len = 0;
    int len = 0;
    BACNET_OBJECT_ID object_identifier;
    uint32_t property_identifier;
    BACNET_UNSIGNED_INTEGER unsigned_value;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* object-identifier    [0] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(&apdu[apdu_len], apdu_size - apdu_len,
        0, &object_identifier.type, &object_identifier.instance);
    if (len > 0) {
        apdu_len += len;
    } else if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    /* property-identifier  [1] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &property_identifier);
    if (len > 0) {
        apdu_len += len;
    } else if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (reference) {
        reference->object_identifier.type = object_identifier.type;
        reference->object_identifier.instance = object_identifier.instance;
        reference->property_identifier =
            (BACNET_PROPERTY_ID)property_identifier;
    }
    /* property-array-index [2] Unsigned OPTIONAL */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 2, NULL)) {
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (unsigned_value > UINT32_MAX) {
                return BACNET_STATUS_ERROR;
            }
            if (reference) {
                reference->property_array_index = unsigned_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        /* OPTIONAL - skip apdu_len increment */
        if (reference) {
            reference->property_array_index = BACNET_ARRAY_ALL;
        }
    }

    return apdu_len;
}

/**
 * Decode the context object property reference. Check for
 * an opening tag and a closing tag as well.
 *
 * @param apdu  Pointer to the buffer containing the encoded value
 * @param apdu_size Size of the buffer containing the encoded value
 * @param tag_number  Tag number
 * @param value  Pointer to the structure that shall be decoded into.
 *
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacapp_decode_context_obj_property_ref(uint8_t *apdu,
    uint16_t apdu_size,
    uint8_t tag_number,
    BACNET_OBJECT_PROPERTY_REFERENCE *value)
{
    int len = 0;
    int apdu_len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    len = bacapp_decode_obj_property_ref(
        &apdu[apdu_len], apdu_size - apdu_len, value);
    if (len > 0) {
        apdu_len += len;
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare the complex data of value1 and value2
 * @param value1 - value 1 structure
 * @param value2 - value 2 structure
 * @return true if the values are the same
 */
bool bacnet_object_property_reference_same(
    BACNET_OBJECT_PROPERTY_REFERENCE *value1,
    BACNET_OBJECT_PROPERTY_REFERENCE *value2)
{
    bool status = false;

    if (value1 && value2) {
        if ((value1->property_identifier == value2->property_identifier) &&
            (value1->object_identifier.instance ==
                value2->object_identifier.instance) &&
            (value1->object_identifier.type ==
                value2->object_identifier.type)) {
            status = true;
        }
    }

    return status;
}
