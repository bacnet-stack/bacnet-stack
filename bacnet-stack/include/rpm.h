/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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
#ifndef RPM_H
#define RPM_H

#include <stdint.h>
#include <stdbool.h>
#include "bacenum.h"
#include "bacdef.h"
#include "bacapp.h"

struct BACnet_Read_Access_Data;
typedef struct BACnet_Read_Access_Data {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    /* simple linked list of values */
    BACNET_PROPERTY_REFERENCE *listOfProperties;
    struct BACnet_Read_Access_Data *next;
} BACNET_READ_ACCESS_DATA;

/** Fetches the lists of properties (array of BACNET_PROPERTY_ID's) for this
 *  object type, grouped by Required, Optional, and Proprietary.
 * A function template; @see device.c for assignment to object types.
 * @ingroup ObjHelpers 
 *
 * @param pRequired [out] Pointer reference for the list of Required properties.
 * @param pOptional [out] Pointer reference for the list of Optional properties.
 * @param pProprietary [out] Pointer reference for the list of Proprietary
 *                           properties for this BACNET_OBJECT_TYPE.
 */
typedef void (
    *rpm_property_lists_function) (
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);

struct property_list_t {
    const int *pList;
    unsigned count;
};

struct special_property_list_t {
    struct property_list_t Required;
    struct property_list_t Optional;
    struct property_list_t Proprietary;
};

typedef void (
    *rpm_object_property_lists_function) (
    BACNET_OBJECT_TYPE object_type,
    struct special_property_list_t * pPropertyList);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* encode functions */
/* Start with the Init function, and then add an object,
 then add its properties, and then end the object.
 Continue to add objects and properties as needed
 until the APDU is full.*/

/* RPM */
    int rpm_encode_apdu_init(
        uint8_t * apdu,
        uint8_t invoke_id);

    int rpm_encode_apdu_object_begin(
        uint8_t * apdu,
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance);

    int rpm_encode_apdu_object_property(
        uint8_t * apdu,
        BACNET_PROPERTY_ID object_property,
        int32_t array_index);

    int rpm_encode_apdu_object_end(
        uint8_t * apdu);

    int rpm_encode_apdu(
        uint8_t * apdu,
        size_t max_apdu,
        uint8_t invoke_id,
        BACNET_READ_ACCESS_DATA * read_access_data);

/* decode the object portion of the service request only */
    int rpm_decode_object_id(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_OBJECT_TYPE * object_type,
        uint32_t * object_instance);

/* is this the end of this object property list? */
    int rpm_decode_object_end(
        uint8_t * apdu,
        unsigned apdu_len);

/* decode the object property portion of the service request only */
    int rpm_decode_object_property(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_PROPERTY_ID * object_property,
        int32_t * array_index);

/* RPM Ack - reply from server */
    int rpm_ack_encode_apdu_init(
        uint8_t * apdu,
        uint8_t invoke_id);

    int rpm_ack_encode_apdu_object_begin(
        uint8_t * apdu,
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance);

    int rpm_ack_encode_apdu_object_property(
        uint8_t * apdu,
        BACNET_PROPERTY_ID object_property,
        int32_t array_index);

    int rpm_ack_encode_apdu_object_property_value(
        uint8_t * apdu,
        uint8_t * application_data,
        unsigned application_data_len);

    int rpm_ack_encode_apdu_object_property_error(
        uint8_t * apdu,
        BACNET_ERROR_CLASS error_class,
        BACNET_ERROR_CODE error_code);

    int rpm_ack_encode_apdu_object_end(
        uint8_t * apdu);

    int rpm_ack_decode_object_id(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_OBJECT_TYPE * object_type,
        uint32_t * object_instance);
/* is this the end of the list of this objects properties values? */
    int rpm_ack_decode_object_end(
        uint8_t * apdu,
        unsigned apdu_len);
    int rpm_ack_decode_object_property(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_PROPERTY_ID * object_property,
        int32_t * array_index);
#ifdef TEST
#include "ctest.h"
    int rpm_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        uint8_t ** service_request,
        unsigned *service_request_len);

    int rpm_ack_decode_apdu(
        uint8_t * apdu,
        int apdu_len,   /* total length of the apdu */
        uint8_t * invoke_id,
        uint8_t ** service_request,
        unsigned *service_request_len);

    void testReadPropertyMultiple(
        Test * pTest);
    void testReadPropertyMultipleAck(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DSRPM Data Sharing -Read Property Multiple Service (DS-RPM)
 * @ingroup DataShare
 * 15.7 ReadPropertyMultiple Service <br>
 * The ReadPropertyMultiple service is used by a client BACnet-user to request 
 * the values of one or more specified properties of one or more BACnet Objects. 
 * This service allows read access to any property of any object, whether a 
 * BACnet-defined object or not. The user may read a single property of a single 
 * object, a list of properties of a single object, or any number of properties 
 * of any number of objects. 
 * A 'Read Access Specification' with the property identifier ALL can be used to 
 * learn the implemented properties of an object along with their values.
 */
#endif
