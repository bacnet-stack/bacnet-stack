/**
 * @file
 * @brief Header file for a basic GetAlarmSummary service handler
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @date 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_HANDLER_GET_ALARM_SUMMARY_H
#define BACNET_BASIC_SERVICE_HANDLER_GET_ALARM_SUMMARY_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/get_alarm_sum.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void handler_get_alarm_summary_set(
    BACNET_OBJECT_TYPE object_type, get_alarm_summary_function pFunction);

BACNET_STACK_EXPORT
void handler_get_alarm_summary(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
