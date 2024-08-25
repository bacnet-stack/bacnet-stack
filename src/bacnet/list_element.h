/**
 * @file
 * @brief API for AddListElement and RemoveListElement service codec
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_LIST_ELEMENT_H
#define BACNET_LIST_ELEMENT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"

/**
 *  AddListElement-Request ::= SEQUENCE {
 *      object-identifier       [0] BACnetObjectIdentifier,
 *      property-identifier     [1] BACnetPropertyIdentifier,
 *      property-array-index    [2] Unsigned OPTIONAL,
 *      -- used only with array datatype
 *      list-of-elements        [3] ABSTRACT-SYNTAX.&Type
 *  }
 *  RemoveListElement-Request ::= SEQUENCE {
 *      object-identifier       [0] BACnetObjectIdentifier,
 *      property-identifier     [1] BACnetPropertyIdentifier,
 *      property-array-index    [2] Unsigned OPTIONAL,
 *      -- used only with array datatypes
 *      list-of-elements        [3] ABSTRACT-SYNTAX.&Type
 *  }
 */

typedef struct BACnet_List_Element_Data {
    /* note: number type first to avoid enum cast warning on = { 0 } */
    uint32_t object_instance;
    BACNET_OBJECT_TYPE object_type;
    BACNET_PROPERTY_ID object_property;
    BACNET_ARRAY_INDEX array_index;
    uint8_t *application_data;
    int application_data_len;
    BACNET_UNSIGNED_INTEGER first_failed_element_number;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_LIST_ELEMENT_DATA;

/**
 * @brief AddListElement or RemoveListElement from an object list property
 * @ingroup ObjHelpers
 * @param list_element [in] Pointer to the BACnet_List_Element_Data structure,
 * which is packed with the information from the request.
 * @return The length of the apdu encoded or #BACNET_STATUS_ERROR or
 * #BACNET_STATUS_ABORT or #BACNET_STATUS_REJECT.
 */
typedef int (*list_element_function)(BACNET_LIST_ELEMENT_DATA *list_element);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int list_element_encode_service_request(
    uint8_t *apdu, BACNET_LIST_ELEMENT_DATA *list_element);
BACNET_STACK_EXPORT
size_t list_element_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_LIST_ELEMENT_DATA *list_element);
BACNET_STACK_EXPORT
int list_element_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_LIST_ELEMENT_DATA *list_element);
BACNET_STACK_EXPORT
int list_element_error_ack_encode(
    uint8_t *apdu, BACNET_LIST_ELEMENT_DATA *list_element);
BACNET_STACK_EXPORT
int list_element_error_ack_decode(
    uint8_t *apdu, uint16_t apdu_size, BACNET_LIST_ELEMENT_DATA *list_element);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
