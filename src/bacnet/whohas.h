/**
 * @file
 * @brief BACnet WhoHas-Request encode and decode API header file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_WHO_HAS_H
#define BACNET_WHO_HAS_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"

/**
 * @defgroup DMDOB Device Management-Dynamic Object Binding (DM-DOB)
 * @ingroup RDMS
 *
 * 16.9 Who-Has and I-Have Services <br>
 *
 * The Who-Has service is used by a sending BACnet-user to identify the device
 * object identifiers and network addresses of other BACnet devices whose local
 * databases contain an object with a given Object_Name or a given
 * Object_Identifier.
 *
 * The I-Have service is used to respond to Who-Has service requests or to
 * advertise the existence of an object with a given Object_Name or
 * Object_Identifier. The I-Have service request may be issued at any time and
 * does not need to be preceded by the receipt of a Who-Has service request.
 * The Who-Has and I-Have services are unconfirmed services.
 */

typedef struct BACnet_Who_Has_Data {
    /* deviceInstanceRange - use -1 for limit if you want unlimited */
    int32_t low_limit;
    int32_t high_limit;
    /* true if a string */
    bool is_object_name;
    union {
        BACNET_OBJECT_ID identifier;
        BACNET_CHARACTER_STRING name;
    } object;
} BACNET_WHO_HAS_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int whohas_encode_apdu(uint8_t *apdu, const BACNET_WHO_HAS_DATA *data);

BACNET_STACK_EXPORT
int bacnet_who_has_request_encode(
    uint8_t *apdu, const BACNET_WHO_HAS_DATA *data);
BACNET_STACK_EXPORT
size_t bacnet_who_has_service_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_WHO_HAS_DATA *data);

BACNET_STACK_EXPORT
int whohas_decode_service_request(
    const uint8_t *apdu, unsigned apdu_size, BACNET_WHO_HAS_DATA *data);

/* defined in unit test */
BACNET_STACK_EXPORT
int whohas_decode_apdu(
    const uint8_t *apdu, unsigned apdu_size, BACNET_WHO_HAS_DATA *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
