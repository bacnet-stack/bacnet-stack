/**
* @file
* @brief Header file for a basic GetAlarmSummary-Ack service handler
* @author Steve Karg <skarg@users.sourceforge.net>
* @date 2013
* @copyright SPDX-License-Identifier: MIT
*/
#ifndef BACNET_BASIC_SERVICE_GET_ALARM_SUMMARY_ACK_HANDLER_H
#define BACNET_BASIC_SERVICE_GET_ALARM_SUMMARY_ACK_HANDLER_H
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
    void get_alarm_summary_ack_handler(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
