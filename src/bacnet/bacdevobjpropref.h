/**
 * @file
 * @brief BACnetDeviceObjectPropertyReference structure, encode, decode
 * @author John Minack <minack@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DEV_PROP_REF_H_
#define BACNET_DEV_PROP_REF_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacint.h"

typedef struct BACnetDeviceObjectPropertyReference {
    /* number type first to avoid enum cast warning on = { 0 } */
    BACNET_UNSIGNED_INTEGER arrayIndex;
    BACNET_OBJECT_ID objectIdentifier;
    BACNET_PROPERTY_ID propertyIdentifier;
    BACNET_OBJECT_ID deviceIdentifier;
} BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE;

/** BACnetDeviceObjectReference structure.
 * If the optional deviceIdentifier is not provided, then this refers
 * to an object inside this Device.
 */
typedef struct BACnetDeviceObjectReference {
    BACNET_OBJECT_ID deviceIdentifier; /**< Optional, for external device. */
    BACNET_OBJECT_ID objectIdentifier;
} BACNET_DEVICE_OBJECT_REFERENCE;

/**
 *  BACnetObjectPropertyReference ::= SEQUENCE {
 *      object-identifier [0] BACnetObjectIdentifier,
 *      property-identifier [1] BACnetPropertyIdentifier,
 *      property-array-index [2] Unsigned OPTIONAL
 *      -- used only with array datatype
 *      -- if omitted with an array the entire array is referenced
 * }
 */
typedef struct BACnet_Object_Property_Reference {
    /* note: use type = OBJECT_NONE for unused reference */
    BACNET_OBJECT_ID object_identifier;
    BACNET_PROPERTY_ID property_identifier;
    /* optional array index - use BACNET_ARRAY_ALL when not used */
    BACNET_ARRAY_INDEX property_array_index;
} BACNET_OBJECT_PROPERTY_REFERENCE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacapp_encode_device_obj_property_ref(
    uint8_t *apdu, const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
int bacapp_encode_context_device_obj_property_ref(
    uint8_t *apdu,
    uint8_t tag_number,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_DEPRECATED(
    "Use bacnet_device_object_property_reference_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_device_obj_property_ref(
    const uint8_t *apdu, BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_DEPRECATED(
    "Use bacnet_device_object_property_reference_context_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_context_device_obj_property_ref(
    const uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
int bacapp_encode_device_obj_ref(
    uint8_t *apdu, const BACNET_DEVICE_OBJECT_REFERENCE *value);

BACNET_STACK_EXPORT
int bacapp_encode_context_device_obj_ref(
    uint8_t *apdu,
    uint8_t tag_number,
    const BACNET_DEVICE_OBJECT_REFERENCE *value);

BACNET_STACK_EXPORT
int bacapp_decode_device_obj_ref(
    const uint8_t *apdu, BACNET_DEVICE_OBJECT_REFERENCE *value);

BACNET_STACK_EXPORT
int bacapp_decode_context_device_obj_ref(
    const uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_REFERENCE *value);

BACNET_STACK_EXPORT
int bacapp_encode_obj_property_ref(
    uint8_t *apdu, const BACNET_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
int bacapp_encode_context_obj_property_ref(
    uint8_t *apdu,
    uint8_t tag_number,
    const BACNET_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
int bacapp_decode_obj_property_ref(
    const uint8_t *apdu,
    uint16_t apdu_len_max,
    BACNET_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
int bacapp_decode_context_obj_property_ref(
    const uint8_t *apdu,
    uint16_t apdu_len_max,
    uint8_t tag_number,
    BACNET_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
int bacnet_device_object_property_reference_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value);
BACNET_STACK_EXPORT
int bacnet_device_object_property_reference_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
int bacnet_device_object_reference_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_DEVICE_OBJECT_REFERENCE *value);
BACNET_STACK_EXPORT
int bacnet_device_object_reference_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_DEVICE_OBJECT_REFERENCE *value);

BACNET_STACK_EXPORT
bool bacnet_device_object_property_reference_same(
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value1,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value2);

BACNET_STACK_EXPORT
bool bacnet_device_object_reference_same(
    const BACNET_DEVICE_OBJECT_REFERENCE *value1,
    const BACNET_DEVICE_OBJECT_REFERENCE *value2);

BACNET_STACK_EXPORT
bool bacnet_object_property_reference_same(
    const BACNET_OBJECT_PROPERTY_REFERENCE *value1,
    const BACNET_OBJECT_PROPERTY_REFERENCE *value2);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
