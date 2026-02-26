/**
 * @file
 * @brief API for BACnet WritePropertyMultiple service encoder and decoder
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_WRITE_PROPERTY_MULTIPLE_H
#define BACNET_WRITE_PROPERTY_MULTIPLE_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct BACnet_Write_Access_Data;
typedef struct BACnet_Write_Access_Data {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    /* simple linked list of values */
    BACNET_PROPERTY_VALUE *listOfProperties;
    struct BACnet_Write_Access_Data *next;
} BACNET_WRITE_ACCESS_DATA;

/* decode the service request only */
BACNET_STACK_EXPORT
int wpm_decode_object_id(
    const uint8_t *apdu, uint16_t apdu_len, BACNET_WRITE_PROPERTY_DATA *wpdata);

BACNET_STACK_EXPORT
int wpm_decode_object_property(
    const uint8_t *apdu, uint16_t apdu_len, BACNET_WRITE_PROPERTY_DATA *wpdata);

/* encode objects */
BACNET_STACK_EXPORT
int wpm_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id);
BACNET_STACK_EXPORT
int wpm_encode_apdu_object_begin(
    uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t object_instance);
BACNET_STACK_EXPORT
int wpm_encode_apdu_object_property(
    uint8_t *apdu, const BACNET_WRITE_PROPERTY_DATA *wpdata);
BACNET_STACK_EXPORT
int wpm_encode_apdu_object_end(uint8_t *apdu);

BACNET_STACK_EXPORT
int write_property_multiple_request_encode(
    uint8_t *apdu, BACNET_WRITE_ACCESS_DATA *data);
BACNET_STACK_EXPORT
size_t write_property_multiple_request_service_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_WRITE_ACCESS_DATA *data);

BACNET_STACK_EXPORT
int wpm_encode_apdu(
    uint8_t *apdu,
    size_t max_apdu,
    uint8_t invoke_id,
    BACNET_WRITE_ACCESS_DATA *write_access_data);

/* encode service */
BACNET_STACK_EXPORT
int wpm_ack_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id);

BACNET_STACK_EXPORT
int wpm_error_ack_service_encode(
    uint8_t *apdu, const BACNET_WRITE_PROPERTY_DATA *wp_data);
BACNET_STACK_EXPORT
int wpm_error_ack_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    const BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
int wpm_error_ack_decode_apdu(
    const uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
void wpm_write_access_data_link_array(
    BACNET_WRITE_ACCESS_DATA *base, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DSWP Data Sharing - Write Property Multiple Service (DS-WPM)
 * @ingroup DataShare
 * 15.10 WriteProperty Multiple Service <br>
 * The WritePropertyMultiple service is used by a client BACnet-user
 * to modify the value of one or more specified properties of a BACnet object.
 * This service potentially allows write access to any property of any object,
 * whether a BACnet-defined object or not.
 * Properties shall be modified by the WritePropertyMultiple service
 * in the order specified in the 'List of Write Access Specifications'
 * parameter, and execution of the service shall continue until all of the
 * specified properties have been written to or a property is encountered that
 * for some reason cannot be modified as requested.
 * Some implementers may wish to restrict write access to certain properties
 * of certain objects. In such cases, an attempt to modify a restricted property
 * shall result in the return of an error of 'Error Class' PROPERTY and 'Error
 * Code' WRITE_ACCESS_DENIED. Note that these restricted properties may be
 * accessible through the use of Virtual Terminal services or other means at the
 * discretion of the implementer.
 */
#endif
