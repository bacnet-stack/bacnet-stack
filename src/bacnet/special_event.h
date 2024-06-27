/**
 * @file
 * @brief API for BACnetSpecialEvent complex data type encode and decode
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SPECIAL_EVENT_H
#define BACNET_SPECIAL_EVENT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactimevalue.h"
#include "bacnet/calendar_entry.h"
#include "bacnet/dailyschedule.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


    typedef enum BACnet_SpecialEventPeriod_Tags {
        BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY = 0,
        BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE = 1
    } BACNET_SPECIAL_EVENT_PERIOD_TAG;

    typedef struct BACnet_Special_Event {
        BACNET_SPECIAL_EVENT_PERIOD_TAG periodTag;
        union {
            BACNET_CALENDAR_ENTRY calendarEntry;
            BACNET_OBJECT_ID calendarReference;
        } period;
        /* We reuse the daily schedule struct and its encoding/decoding - it's identical */
        BACNET_DAILY_SCHEDULE timeValues;
        uint8_t priority;
    } BACNET_SPECIAL_EVENT;

    /** Decode Special Event */
    BACNET_STACK_EXPORT
    int bacnet_special_event_decode(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_SPECIAL_EVENT * value);

    /** Encode Special Event */
    BACNET_STACK_EXPORT
    int bacnet_special_event_encode(
        uint8_t * apdu,
        BACNET_SPECIAL_EVENT * value);

    BACNET_STACK_EXPORT
    int bacnet_special_event_context_encode(
        uint8_t *apdu, uint8_t tag_number, BACNET_SPECIAL_EVENT *value);

    BACNET_STACK_EXPORT
    int bacnet_special_event_context_decode(
        uint8_t *apdu, int max_apdu_len, uint8_t tag_number,
        BACNET_SPECIAL_EVENT *value);

    BACNET_STACK_EXPORT
    bool bacnet_special_event_same(
        BACNET_SPECIAL_EVENT *value1, BACNET_SPECIAL_EVENT *value2);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* BACNET_SPECIAL_EVENT_H */
