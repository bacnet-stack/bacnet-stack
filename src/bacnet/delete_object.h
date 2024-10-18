/**
 * @file
 * @brief API for DeleteObject service encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DELETE_OBJECT_H
#define BACNET_DELETE_OBJECT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"

/**
 *  DeleteObject-Request ::= SEQUENCE {
 *      object-identifier BACnetObjectIdentifier
 */
typedef struct BACnet_Delete_Object_Data {
    uint32_t object_instance;
    BACNET_OBJECT_TYPE object_type;
    /* application layer stores specific abort/reject/error */
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_DELETE_OBJECT_DATA;

/**
 * @brief DeleteObject service handler for an object
 * @ingroup ObjHelpers
 * @param object_instance [in] instance number of the object to delete
 * @return true if the object instance is deleted
 */
typedef bool (*delete_object_function)(uint32_t object_instance);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int delete_object_encode_service_request(
    uint8_t *apdu, const BACNET_DELETE_OBJECT_DATA *data);

BACNET_STACK_EXPORT
int delete_object_service_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_DELETE_OBJECT_DATA *data);

BACNET_STACK_EXPORT
int delete_object_decode_service_request(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_DELETE_OBJECT_DATA *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
