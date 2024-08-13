/**
 * @file
 * @brief BACnet GetEventNotification encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_GET_EVENT_H
#define BACNET_GET_EVENT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/timestamp.h"
#include "bacnet/event.h"

struct BACnet_Get_Event_Information_Data;
typedef struct BACnet_Get_Event_Information_Data {
    BACNET_OBJECT_ID objectIdentifier;
    BACNET_EVENT_STATE eventState;
    BACNET_BIT_STRING acknowledgedTransitions;
    BACNET_TIMESTAMP eventTimeStamps[3];
    BACNET_NOTIFY_TYPE notifyType;
    BACNET_BIT_STRING eventEnable;
    uint32_t eventPriorities[3];
    struct BACnet_Get_Event_Information_Data *next;
} BACNET_GET_EVENT_INFORMATION_DATA;

/* return 0 if no active event at this index
   return -1 if end of list
   return +1 if active event */
typedef int (
    *get_event_info_function) (
    unsigned index,
    BACNET_GET_EVENT_INFORMATION_DATA * getevent_data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int getevent_apdu_encode(
        uint8_t *apdu,
        BACNET_OBJECT_ID *lastReceivedObjectIdentifier);

    BACNET_STACK_EXPORT
    int getevent_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_OBJECT_ID * lastReceivedObjectIdentifier);

    BACNET_STACK_EXPORT
    size_t getevent_service_request_encode(
        uint8_t *apdu, size_t apdu_size, 
        BACNET_OBJECT_ID *data);

    BACNET_STACK_EXPORT
    int getevent_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_OBJECT_ID * object_id);

    BACNET_STACK_EXPORT
    int getevent_ack_encode_apdu_init(
        uint8_t * apdu,
        size_t max_apdu,
        uint8_t invoke_id);

    BACNET_STACK_EXPORT
    int getevent_ack_encode_apdu_data(
        uint8_t * apdu,
        size_t max_apdu,
        BACNET_GET_EVENT_INFORMATION_DATA * get_event_data);

    BACNET_STACK_EXPORT
    int getevent_ack_encode_apdu_end(
        uint8_t * apdu,
        size_t max_apdu,
        bool moreEvents);

    BACNET_STACK_EXPORT
    int getevent_ack_decode_service_request(
        uint8_t * apdu,
        int apdu_len,   /* total length of the apdu */
        BACNET_GET_EVENT_INFORMATION_DATA * get_event_data,
        bool * moreEvents);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
