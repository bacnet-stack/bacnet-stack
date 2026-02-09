/**
 * @file
 * @brief API for BACnet WriteProperty service encoder and decoder
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_WRITE_PROPERTY_H
#define BACNET_WRITE_PROPERTY_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"

/** @note: write property can have application tagged data, or context tagged
   data, or even complex data types (i.e. opening and closing tag around data).
   It could also have more than one value or element.  */

typedef struct BACnet_Write_Property_Data {
    /* number type first to avoid enum cast warning on = { 0 } */
    uint32_t object_instance;
    BACNET_OBJECT_TYPE object_type;
    BACNET_PROPERTY_ID object_property;
    /* use BACNET_ARRAY_ALL when not setting */
    BACNET_ARRAY_INDEX array_index;
    uint8_t application_data[MAX_APDU];
    int application_data_len;
    uint8_t priority; /* use BACNET_NO_PRIORITY if no priority */
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_WRITE_PROPERTY_DATA;

/** Attempts to write a new value to one property for this object type
 *  of a given instance.
 * A function template; @see device.c for assignment to object types.
 * @ingroup ObjHelpers
 *
 * @param wp_data [in] Pointer to the BACnet_Write_Property_Data structure,
 *                     which is packed with the information from the WP request.
 * @return The length of the apdu encoded or -1 for error or
 *         -2 for abort message.
 */
typedef bool (*write_property_function)(BACNET_WRITE_PROPERTY_DATA *wp_data);

/**
 * @brief API for setting a BACnet Unsigned Integer property value
 * @param object_instance [in] Object instance number
 * @param value [in] New value to set
 * @return true if successful, else false
 */
typedef bool (*bacnet_property_unsigned_setter)(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

/**
 * @brief API to see if an object property is a member of this object instance
 * @param object_type - object type of the object
 * @param object_instance - object-instance number of the object
 * @param object_property - object-property to be checked
 * @return true if the property is a member of this object instance
 */
typedef bool (*write_property_member_of_object)(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property);

/**
 * @brief API to get the list of writable properties for an object instance
 * @param object_instance - object-instance number of the object
 * @param properties - pointer to the list of writable properties
 */
typedef void (*writable_property_list_function)(
    uint32_t object_instance, const int32_t **properties);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* encode service */
BACNET_STACK_EXPORT
size_t writeproperty_apdu_encode(
    uint8_t *apdu, const BACNET_WRITE_PROPERTY_DATA *data);
BACNET_STACK_EXPORT
size_t writeproperty_service_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_WRITE_PROPERTY_DATA *data);
BACNET_STACK_EXPORT
int wp_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    const BACNET_WRITE_PROPERTY_DATA *wp_data);

/* decode the service request only */
BACNET_STACK_EXPORT
int wp_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_len,
    BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool write_property_type_valid(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    const BACNET_APPLICATION_DATA_VALUE *value,
    uint8_t expected_tag);
BACNET_STACK_EXPORT
bool write_property_string_valid(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    const BACNET_APPLICATION_DATA_VALUE *value,
    size_t len_max);
BACNET_STACK_EXPORT
bool write_property_empty_string_valid(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    const BACNET_APPLICATION_DATA_VALUE *value,
    size_t len_max);
BACNET_STACK_EXPORT
bool write_property_bacnet_array_valid(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool write_property_unsigned_decode(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    BACNET_APPLICATION_DATA_VALUE *value,
    bacnet_property_unsigned_setter setter,
    BACNET_UNSIGNED_INTEGER maximum);

BACNET_STACK_EXPORT
bool write_property_relinquish_bypass(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    write_property_member_of_object member_of_object);

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DSWP Data Sharing - Write Property Service (DS-WP)
 * @ingroup DataShare
 * 15.9 WriteProperty Service <br>
 * The WriteProperty service is used by a client BACnet-user to modify the
 * value of a single specified property of a BACnet object. This service
 * potentially allows write access to any property of any object, whether a
 * BACnet-defined object or not. Some implementers may wish to restrict write
 * access to certain properties of certain objects. In such cases, an attempt
 * to modify a restricted property shall result in the return of an error of
 * 'Error Class' PROPERTY and 'Error Code' WRITE_ACCESS_DENIED.
 */
#endif
