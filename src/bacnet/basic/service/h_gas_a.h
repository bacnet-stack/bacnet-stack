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
 * @brief Callback for a GetAlarmSummary ACK
 * @details The handler validates the whole ACK before it calls the callback.
 *  - error_code is ERROR_CODE_SUCCESS and data is not NULL:
 *    one decoded entry. The callback is called once for each entry.
 *  - error_code is ERROR_CODE_SUCCESS and data is NULL:
 *    the ACK is valid and has no entries.
 *  - error_code is not ERROR_CODE_SUCCESS and data is NULL:
 *    the ACK is malformed or truncated. No entries are reported.
 * @param src [in] BACNET_ADDRESS of the source of the ACK
 * @param invoke_id [in] invoke ID of the ACK
 * @param data [in] the decoded entry, or NULL if there is no entry
 * @param error_code [in] ERROR_CODE_SUCCESS, or the reason for the failure
 */
typedef void (*get_alarm_summary_ack_callback_t)(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    const BACNET_GET_ALARM_SUMMARY_DATA *data,
    BACNET_ERROR_CODE error_code);

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
