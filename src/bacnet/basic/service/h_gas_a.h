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
#include "bacnet/get_alarm_sum.h"

/**
 * @brief Callback for each decoded GetAlarmSummary ACK entry
 * @param src [in] BACNET_ADDRESS of the source of the ACK
 * @param invoke_id [in] invoke ID of the ACK
 * @param data [in] the decoded entry
 */
typedef void (*get_alarm_summary_ack_callback_t)(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    const BACNET_GET_ALARM_SUMMARY_DATA *data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void get_alarm_summary_ack_callback_set(get_alarm_summary_ack_callback_t cb);

BACNET_STACK_EXPORT
void get_alarm_summary_ack_print_data(
    const BACNET_GET_ALARM_SUMMARY_DATA *data);

BACNET_STACK_EXPORT
void get_alarm_summary_ack_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
