/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic UnconfirmedCOVNotification service send
 * and SubscribeCOV service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SERVICE_SEND_COV_H
#define SERVICE_SEND_COV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"
#include "bacnet/cov.h"
#include "bacnet/npdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int Send_UCOV_Notify(
    uint8_t *buffer, unsigned buffer_len, const BACNET_COV_DATA *cov_data);
BACNET_STACK_EXPORT
int ucov_notify_encode_pdu(
    uint8_t *buffer,
    unsigned buffer_len,
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    const BACNET_COV_DATA *cov_data);
BACNET_STACK_EXPORT
uint8_t Send_COV_Subscribe_Address(
    BACNET_ADDRESS *dest,
    uint16_t max_apdu,
    const BACNET_SUBSCRIBE_COV_DATA *cov_data);
BACNET_STACK_EXPORT
uint8_t Send_COV_Subscribe(
    uint32_t device_id, const BACNET_SUBSCRIBE_COV_DATA *cov_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
