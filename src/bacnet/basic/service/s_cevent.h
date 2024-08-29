/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic ConfirmedEventNotification service send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_CONFIRMED_EVENT_NOTIFICATION_H
#define SEND_CONFIRMED_EVENT_NOTIFICATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"
#include "bacnet/event.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint8_t Send_CEvent_Notify(uint32_t device_id,
                           const BACNET_EVENT_NOTIFICATION_DATA* data);
BACNET_STACK_EXPORT
uint8_t Send_CEvent_Notify_Address(uint8_t *pdu, uint16_t pdu_size,
    const BACNET_EVENT_NOTIFICATION_DATA *data, BACNET_ADDRESS *dest);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
