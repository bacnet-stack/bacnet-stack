/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic GetAlarmSummary service send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_GET_ALARM_SUMMARY_H
#define SEND_GET_ALARM_SUMMARY_H

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
uint8_t Send_Get_Alarm_Summary_Address(BACNET_ADDRESS *dest, uint16_t max_apdu);
BACNET_STACK_EXPORT
uint8_t Send_Get_Alarm_Summary(uint32_t device_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
