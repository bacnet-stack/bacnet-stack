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
    /* simple linked list of values */
    BACNET_PROPERTY_VALUE *list_of_initial_values;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
    BACNET_UNSIGNED_INTEGER first_failed_element_number;
} BACNET_CREATE_OBJECT_DATA;

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
    uint8_t *apdu, uint32_t apdu_size, BACNET_CREATE_OBJECT_DATA *data);

BACNET_STACK_EXPORT
int create_object_ack_service_encode(
    uint8_t *apdu, BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_ack_service_decode(
    uint8_t *apdu, uint16_t apdu_size, BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_ack_encode(
    uint8_t *apdu, uint8_t invoke_id, BACNET_CREATE_OBJECT_DATA *data);

BACNET_STACK_EXPORT
int create_object_error_ack_service_encode(
    uint8_t *apdu, BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_error_ack_service_decode(
    uint8_t *apdu, uint16_t apdu_size, BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
int create_object_error_ack_encode(
    uint8_t *apdu, uint8_t invoke_id, BACNET_CREATE_OBJECT_DATA *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
