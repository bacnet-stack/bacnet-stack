/**
 * @file
 * @brief BACnet GetAlarmSummary encode and decode functions
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_GET_ALARM_SUMMARY_H_
#define BACNET_GET_ALARM_SUMMARY_H_

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/timestamp.h"

struct BACnet_Get_Alarm_Summary_Data;
typedef struct BACnet_Get_Alarm_Summary_Data {
    BACNET_OBJECT_ID objectIdentifier;
    BACNET_EVENT_STATE alarmState;
    BACNET_BIT_STRING acknowledgedTransitions;
    struct BACnet_Get_Alarm_Summary_Data *next;
} BACNET_GET_ALARM_SUMMARY_DATA;


/* return 0 if no active alarm at this index
   return -1 if end of list
   return +1 if active alarm */
typedef int (
    *get_alarm_summary_function) (
    unsigned index,
    BACNET_GET_ALARM_SUMMARY_DATA * getalarm_data);


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int get_alarm_summary_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id);

    /* encode service */
    BACNET_STACK_EXPORT
    int get_alarm_summary_ack_encode_apdu_init(
        uint8_t * apdu,
        uint8_t invoke_id);

    BACNET_STACK_EXPORT
    int get_alarm_summary_ack_encode_apdu_data(
        uint8_t * apdu,
        size_t max_apdu,
        BACNET_GET_ALARM_SUMMARY_DATA * get_alarm_data);

    BACNET_STACK_EXPORT
    int get_alarm_summary_ack_decode_apdu_data(
        uint8_t * apdu,
        size_t max_apdu,
        BACNET_GET_ALARM_SUMMARY_DATA * get_alarm_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup ALMEVNT Alarm and Event Management BIBBs
 * @ingroup ALMEVNT
 * 13.1 ConfirmedCOVNotification Service <br>
 * The GetAlarmSummary service is used by a client BACnet-user to obtain
 * a summary of "active alarms." The term "active alarm" refers to
 * BACnet standard objects that have an Event_State property whose value
 * is not equal to NORMAL and a Notify_Type property whose value is ALARM.
 * The GetEnrollmentSummary service provides a more sophisticated approach
 * with various kinds of filters.
 */
#endif /* BACNET_GET_ALARM_SUM_H_ */
