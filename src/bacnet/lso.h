/**
 * @file
 * @brief API for BACnetLifeSafetyOperation encoder and decoder
 * @author John Minack <minack@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_LIFE_SAFETY_OPERATION_H
#define BACNET_LIFE_SAFETY_OPERATION_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"

typedef struct {
    uint32_t processId;
    BACNET_CHARACTER_STRING requestingSrc;
    BACNET_LIFE_SAFETY_OPERATION operation;
    BACNET_OBJECT_ID targetObject;
    bool use_target:1;
} BACNET_LSO_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int lso_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_LSO_DATA * data);

    BACNET_STACK_EXPORT
    int life_safety_operation_encode(
        uint8_t *apdu, 
        BACNET_LSO_DATA *data);

    BACNET_STACK_EXPORT
    size_t life_safety_operation_request_encode(
        uint8_t *apdu, 
        size_t apdu_size, 
        BACNET_LSO_DATA *data);

    BACNET_STACK_EXPORT
    int lso_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_LSO_DATA * data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
