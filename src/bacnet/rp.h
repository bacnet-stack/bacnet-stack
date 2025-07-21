/**
 * @file
 * @brief BACnet ReadProperty-Request encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_READ_PROPERTY_H
#define BACNET_READ_PROPERTY_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

typedef struct BACnet_Read_Property_Data {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_PROPERTY_ID object_property;
    BACNET_ARRAY_INDEX array_index;
    uint8_t *application_data;
    int application_data_len;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_READ_PROPERTY_DATA;

/* Forward declaration of RPM-style data structure */
struct BACnet_Read_Access_Data;

/** Reads one property for this object type of a given instance.
 * A function template; @see device.c for assignment to object types.
 * @ingroup ObjHelpers
 *
 * @param rp_data [in] Pointer to the BACnet_Read_Property_Data structure,
 *                     which is packed with the information from the RP request.
 * @return The length of the apdu encoded or -1 for error or
 *         -2 for abort message.
 */
typedef int (*read_property_function)(BACNET_READ_PROPERTY_DATA *rp_data);

/**
 * @brief Process a ReadProperty-ACK message
 * @param device_id [in] The device ID of the source of the message
 * @param rp_data [in] The contents of the ReadProperty-ACK message
 */
typedef void (*read_property_ack_process)(
    uint32_t device_id, BACNET_READ_PROPERTY_DATA *rp_data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int read_property_request_encode(
    uint8_t *apdu, const BACNET_READ_PROPERTY_DATA *data);

BACNET_STACK_EXPORT
size_t read_property_request_service_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_READ_PROPERTY_DATA *data);

BACNET_STACK_EXPORT
int read_property_ack_encode(
    uint8_t *apdu, const BACNET_READ_PROPERTY_DATA *data);

BACNET_STACK_EXPORT
size_t read_property_ack_service_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_READ_PROPERTY_DATA *data);

BACNET_STACK_EXPORT
int rp_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_READ_PROPERTY_DATA *rpdata);

/* decode the service request only */
BACNET_STACK_EXPORT
int rp_decode_service_request(
    const uint8_t *apdu, unsigned apdu_len, BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool read_property_bacnet_array_valid(BACNET_READ_PROPERTY_DATA *data);

/* method to encode the ack without extra buffer */
BACNET_STACK_EXPORT
int rp_ack_encode_apdu_init(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
int rp_ack_encode_apdu_object_property_end(uint8_t *apdu);

/* method to encode the ack using extra buffer */
BACNET_STACK_EXPORT
int rp_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
int rp_ack_decode_service_request(
    uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    BACNET_READ_PROPERTY_DATA *rpdata);

/* Decode instead to RPM-style data structure. */
BACNET_STACK_EXPORT
int rp_ack_fully_decode_service_request(
    uint8_t *apdu,
    int apdu_len,
    struct BACnet_Read_Access_Data *read_access_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DataShare Data Sharing BIBBs
 * These BIBBs prescribe the BACnet capabilities required to interoperably
 * perform the data sharing functions enumerated in 22.2.1.1 for the BACnet
 * devices defined therein.
 */

/** @defgroup DSRP Data Sharing -Read Property Service (DS-RP)
 * @ingroup DataShare
 * 15.5 ReadProperty Service <br>
 * The ReadProperty service is used by a client BACnet-user to request the
 * value of one property of one BACnet Object. This service allows read access
 * to any property of any object, whether a BACnet-defined object or not.
 */
#endif
