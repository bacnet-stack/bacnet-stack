/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic UnconfirmedEventNotification service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_UNCONFIRMED_EVENT_NOTIFICATION_H
#define SEND_UNCONFIRMED_EVENT_NOTIFICATION_H

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
int Send_UEvent_Notify(
    uint8_t *buffer,
    const BACNET_EVENT_NOTIFICATION_DATA *data,
    BACNET_ADDRESS *dest);

BACNET_STACK_EXPORT
int Send_UEvent_Notify_Device(
    uint8_t *buffer,
    const BACNET_EVENT_NOTIFICATION_DATA *data,
    uint32_t device_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
