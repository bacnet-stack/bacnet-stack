/**
 * @file
 * @brief BACnetWeeklySchedule complex data type encode and decode
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @date September 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_WEEKLY_SCHEDULE_H
#define BACNET_WEEKLY_SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/dailyschedule.h"
#include "bacnet/bactimevalue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct BACnet_Weekly_Schedule {
        BACNET_DAILY_SCHEDULE weeklySchedule[7];
        bool singleDay;
    } BACNET_WEEKLY_SCHEDULE;

    /** Decode WeeklySchedule */
    BACNET_STACK_EXPORT
    int bacnet_weeklyschedule_decode(
        const uint8_t * apdu,
        int apdu_size,
        BACNET_WEEKLY_SCHEDULE * value);

    /** Encode WeeklySchedule */
    BACNET_STACK_EXPORT
    int bacnet_weeklyschedule_encode(
        uint8_t * apdu,
        const BACNET_WEEKLY_SCHEDULE * value);

    BACNET_STACK_EXPORT
    int bacnet_weeklyschedule_context_encode(
        uint8_t *apdu, uint8_t tag_number,
        const BACNET_WEEKLY_SCHEDULE *value);

    BACNET_STACK_EXPORT
    int bacnet_weeklyschedule_context_decode(
        const uint8_t *apdu, int apdu_size, uint8_t tag_number,
        BACNET_WEEKLY_SCHEDULE *value);

    BACNET_STACK_EXPORT
    bool bacnet_weeklyschedule_same(
        const BACNET_WEEKLY_SCHEDULE *value1,
        const BACNET_WEEKLY_SCHEDULE *value2);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
