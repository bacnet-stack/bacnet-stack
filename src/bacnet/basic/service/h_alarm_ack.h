/**
 * @file
 * @brief Header file a basic Alarm Acknowledgment service Handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_ALARM_ACK_H
#define BACNET_BASIC_SERVICE_ALARM_ACK_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/alarm_ack.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void handler_alarm_ack(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    BACNET_STACK_EXPORT
    void handler_alarm_ack_set(
        BACNET_OBJECT_TYPE object_type,
        alarm_ack_function pFunction);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
