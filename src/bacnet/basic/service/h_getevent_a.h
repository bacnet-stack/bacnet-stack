/**
 * @file
 * @brief Header file for a basic GetEventNotification-Ack service handler
 * @author Daniel Blazevic <daniel.blazevic@gmail.com>
 * @date 2014
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_GET_EVENT_ACK_HANDLER_H
#define BACNET_BASIC_SERVICE_GET_EVENT_ACK_HANDLER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void get_event_ack_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
