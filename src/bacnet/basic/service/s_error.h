/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic Error message send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_ERROR_MESSAGE_H
#define SEND_ERROR_MESSAGE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int Send_Error_To_Network(
    uint8_t *buffer,
    BACNET_ADDRESS *dest,
    uint8_t invoke_id,
    BACNET_CONFIRMED_SERVICE service,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code);

BACNET_STACK_EXPORT
int error_encode_pdu(
    uint8_t *buffer,
    BACNET_ADDRESS *dest,
    BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t invoke_id,
    BACNET_CONFIRMED_SERVICE service,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
