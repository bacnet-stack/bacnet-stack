/**
 * @file
 * @brief Header file for a basic GetEventInformation service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_HANDLER_GET_EVENT_INFORMATION_H
#define BACNET_BASIC_SERVICE_HANDLER_GET_EVENT_INFORMATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/event.h"
#include "bacnet/getevent.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void handler_get_event_information_set(
        BACNET_OBJECT_TYPE object_type,
        get_event_info_function pFunction);

    BACNET_STACK_EXPORT
    void handler_get_event_information(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    BACNET_STACK_EXPORT
    void ge_ack_print_data(BACNET_GET_EVENT_INFORMATION_DATA* data,
                           uint32_t device_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
