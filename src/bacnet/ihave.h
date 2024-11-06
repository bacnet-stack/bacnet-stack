/**
 * @file
 * @brief API for I-Have service encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_I_HAVE_H
#define BACNET_I_HAVE_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"

typedef struct BACnet_I_Have_Data {
    BACNET_OBJECT_ID device_id;
    BACNET_OBJECT_ID object_id;
    BACNET_CHARACTER_STRING object_name;
} BACNET_I_HAVE_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int ihave_encode_apdu(uint8_t *apdu, const BACNET_I_HAVE_DATA *data);

BACNET_STACK_EXPORT
int ihave_decode_service_request(
    const uint8_t *apdu, unsigned apdu_len, BACNET_I_HAVE_DATA *data);

BACNET_STACK_EXPORT
int ihave_decode_apdu(
    const uint8_t *apdu, unsigned apdu_len, BACNET_I_HAVE_DATA *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
