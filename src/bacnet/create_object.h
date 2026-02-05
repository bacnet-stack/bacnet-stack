/**
 * @file
 * @brief API for CreateObject service encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_CREATE_OBJECT_H
#define BACNET_CREATE_OBJECT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/delete_object.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifndef BACNET_CREATE_OBJECT_LIST_VALUES_ENABLED
#define BACNET_CREATE_OBJECT_LIST_VALUES_ENABLED 1
#endif

/**
 *  CreateObject-Request ::= SEQUENCE {
 *      object-specifier [0] CHOICE {
 *          object-type [0] BACnetObjectType,
 *          object-identifier [1] BACnetObjectIdentifier
 *      },
 *      list-of-initial-values [1] SEQUENCE OF BACnetPropertyValue OPTIONAL
 *  }
 */
typedef struct BACnet_Create_Object_Data {
    /* note: use BACNET_MAX_INSTANCE to choose CHOICE=[0] object_type */
    uint32_t object_instance;
    BACNET_OBJECT_TYPE object_type;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
#if BACNET_CREATE_OBJECT_LIST_VALUES_ENABLED
    /* list of values similar to WriteProperty - decoded later */
    uint8_t application_data[MAX_APDU];
#endif
    int application_data_len;
    /* This parameter, of type Unsigned, shall convey the numerical
       position, starting at 1, of the offending 'Initial Value' in the
       'List of Initial Values' parameter received in the request.
       If the request is considered invalid for reasons other than the 'List of
       Initial Values' parameter, the 'First Failed Element Number' shall
       be equal to zero. */
    BACNET_UNSIGNED_INTEGER first_failed_element_number;
} BACNET_CREATE_OBJECT_DATA;

/**
 * BACnetPropertyValue ::= SEQUENCE {
 *      property-identifier [0] BACnetPropertyIdentifier,
 *      property-array-index [1] Unsigned OPTIONAL,
 *      -- used only with array datatypes
 *      -- if omitted with an array the entire array is referenced
 *      property-value [2] ABSTRACT-SYNTAX.&Type,
 *      -- any datatype appropriate for the specified property
 *      priority [3] Unsigned (1..16) OPTIONAL
 *      -- used only when property is commandable
 *  }
 */
typedef struct BACnet_Create_Object_Property_Value {
    BACNET_PROPERTY_ID propertyIdentifier;
    BACNET_ARRAY_INDEX propertyArrayIndex;
    const uint8_t *application_data;
    int application_data_len;
    uint8_t priority;
} BACNET_CREATE_OBJECT_PROPERTY_VALUE;

/**
 * @brief CreateObject service handler for an object
 * @ingroup ObjHelpers
 * @param object_instance [in] instance number of the object to create,
 *  or BACNET_MAX_INSTANCE to create the next free object instance
 * @return object instance number created, or BACNET_MAX_INSTANCE if not
 */
typedef uint32_t (*create_object_function)(uint32_t object_instance);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int create_object_encode_initial_value(
    uint8_t *apdu, int offset, const BACNET_PROPERTY_VALUE *value);
BACNET_STACK_EXPORT
int create_object_encode_initial_value_data(
    uint8_t *apdu, int offset, BACNET_CREATE_OBJECT_PROPERTY_VALUE *value);
BACNET_STACK_EXPORT
int create_object_decode_initial_value(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_CREATE_OBJECT_PROPERTY_VALUE *value);

BACNET_STACK_EXPORT
size_t create_object_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_encode_service_request(
    uint8_t *apdu, BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_encode_service_ack_encode(
    uint8_t *apdu, BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_decode_service_request(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_CREATE_OBJECT_DATA *data);

BACNET_STACK_EXPORT
int create_object_ack_service_encode(
    uint8_t *apdu, const BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_ack_service_decode(
    const uint8_t *apdu, uint16_t apdu_size, BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_ack_encode(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_CREATE_OBJECT_DATA *data);

BACNET_STACK_EXPORT
int create_object_error_ack_service_encode(
    uint8_t *apdu, const BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_error_ack_service_decode(
    const uint8_t *apdu, uint16_t apdu_size, BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_error_ack_encode(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_CREATE_OBJECT_DATA *data);

BACNET_STACK_EXPORT
bool create_object_initializer_list_process(
    BACNET_CREATE_OBJECT_DATA *data, write_property_function write_property);
BACNET_STACK_EXPORT
bool create_object_process(
    BACNET_CREATE_OBJECT_DATA *data,
    bool object_supported,
    bool object_exists,
    create_object_function create_object,
    delete_object_function delete_object,
    write_property_function write_property);
BACNET_STACK_EXPORT
int create_object_writable_properties_encode(
    uint8_t *apdu,
    size_t apdu_size,
    BACNET_CREATE_OBJECT_DATA *data,
    const int32_t *required_properties,
    const int32_t *optional_properties,
    const int32_t *proprietary_properties,
    const int32_t *writable_properties,
    read_property_function read_property);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
