/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic AlarmAcknowledgement service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_ALARM_ACKNOWLEDGEMENT_H
#define SEND_ALARM_ACKNOWLEDGEMENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"
#include "bacnet/alarm_ack.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint8_t Send_Alarm_Acknowledgement_Address(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_ALARM_ACK_DATA *data,
    BACNET_ADDRESS *dest);

BACNET_STACK_EXPORT
uint8_t Send_Alarm_Acknowledgement(
    uint32_t device_id, const BACNET_ALARM_ACK_DATA *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
