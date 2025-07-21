/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic I-Am service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_I_AM_H
#define SEND_I_AM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/bacapp.h"
#include "bacnet/npdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Send_I_Am_To_Network(
    BACNET_ADDRESS *target_address,
    uint32_t device_id,
    unsigned int max_apdu,
    int segmentation,
    uint16_t vendor_id);
BACNET_STACK_EXPORT
int iam_encode_pdu(
    uint8_t *buffer, BACNET_ADDRESS *dest, BACNET_NPDU_DATA *npdu_data);
BACNET_STACK_EXPORT
void Send_I_Am(uint8_t *buffer);
BACNET_STACK_EXPORT
void Send_I_Am_Broadcast(uint8_t *buffer);
BACNET_STACK_EXPORT
int iam_unicast_encode_pdu(
    uint8_t *buffer,
    const BACNET_ADDRESS *src,
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data);
BACNET_STACK_EXPORT
void Send_I_Am_Unicast(uint8_t *buffer, const BACNET_ADDRESS *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
