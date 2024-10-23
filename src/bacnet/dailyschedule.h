/**
 * @file
 * @brief BACnetDailySchedule encode and decode functions
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DAILY_SCHEDULE_H
#define BACNET_DAILY_SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactimevalue.h"

/* arbitrary value, shall be unlimited for B-OWS but we don't care, 640k shall
 * be enough */
/* however we try not to boost the bacnet application value structure size,  */
/* so 7 x (this value) x sizeof(BACNET_TIME_VALUE) fits. */
#define MAX_DAY_SCHEDULE_VALUES 40

/*
    BACnetDailySchedule ::= SEQUENCE {
        day-schedule [0] SEQUENCE OF BACnetTimeValue
    }
*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct BACnet_Daily_Schedule {
    BACNET_TIME_VALUE Time_Values[MAX_DAY_SCHEDULE_VALUES];
    uint16_t TV_Count; /* the number of time values actually used */
} BACNET_DAILY_SCHEDULE;

/** Decode DailySchedule (sequence of times and values) */
BACNET_STACK_EXPORT
int bacnet_dailyschedule_context_decode(
    const uint8_t *apdu,
    int max_apdu_len,
    uint8_t tag_number,
    BACNET_DAILY_SCHEDULE *day);

/** Encode DailySchedule (sequence of times and values) */
BACNET_STACK_EXPORT
int bacnet_dailyschedule_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_DAILY_SCHEDULE *day);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DAILYSCHEDULE_H */
