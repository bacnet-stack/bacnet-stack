/**
 * @file
 * @brief BACnetAcknowledgeAlarmInfo service encode and decode
 * @author John Minack <minack@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_ALARM_ACK_H
#define BACNET_ALARM_ACK_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/timestamp.h"

typedef struct BACnetAcknowledgeAlarmInfo {
    uint32_t ackProcessIdentifier;
    BACNET_OBJECT_ID eventObjectIdentifier;
    BACNET_EVENT_STATE eventStateAcked;
    BACNET_TIMESTAMP eventTimeStamp;
    BACNET_CHARACTER_STRING ackSource;
    BACNET_TIMESTAMP ackTimeStamp;
} BACNET_ALARM_ACK_DATA;

/* return +1 if alarm was acknowledged
   return -1 if any error occurred
   return -2 abort */
typedef int (*alarm_ack_function)(
    BACNET_ALARM_ACK_DATA *alarmack_data, BACNET_ERROR_CODE *error_code);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int alarm_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_ALARM_ACK_DATA *data);

BACNET_STACK_EXPORT
int alarm_ack_encode_service_request(
    uint8_t *apdu, const BACNET_ALARM_ACK_DATA *data);

BACNET_STACK_EXPORT
size_t bacnet_acknowledge_alarm_info_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_ALARM_ACK_DATA *data);

BACNET_STACK_EXPORT
int alarm_ack_decode_service_request(
    const uint8_t *apdu, unsigned apdu_len, BACNET_ALARM_ACK_DATA *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
