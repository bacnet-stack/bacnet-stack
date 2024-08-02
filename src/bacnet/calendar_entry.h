/**
 * @file
 * @brief API for BACnetCalendarEntry complex data type encode and decode
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_CALENDAR_ENTRY_H
#define BACNET_CALENDAR_ENTRY_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactimevalue.h"
#include "bacnet/datetime.h"

/*
    BACnetCalendarEntry ::= CHOICE {
        date [0] Date,
        date-range [1] BACnetDateRange,
        weekNDay [2] BACnetWeekNDay
    }
*/

typedef enum BACnet_CalendarEntry_Tags {
    BACNET_CALENDAR_DATE = 0,
    BACNET_CALENDAR_DATE_RANGE = 1,
    BACNET_CALENDAR_WEEK_N_DAY = 2
} BACNET_CALENDAR_ENTRY_TAGS;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct BACnetCalendarEntry_T {
    uint8_t tag;
    union {
        BACNET_DATE Date;
        BACNET_DATE_RANGE DateRange;
        BACNET_WEEKNDAY WeekNDay;
    } type;
    /* simple linked list if needed */
    struct BACnetCalendarEntry_T *next;
} BACNET_CALENDAR_ENTRY;


BACNET_STACK_EXPORT
int bacnet_calendar_entry_encode(uint8_t *apdu, BACNET_CALENDAR_ENTRY *value);

BACNET_STACK_EXPORT
int bacnet_calendar_entry_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_CALENDAR_ENTRY *value);

BACNET_STACK_EXPORT
int bacnet_calendar_entry_decode(
    uint8_t *apdu, uint32_t apdu_max_len, BACNET_CALENDAR_ENTRY *entry);

BACNET_STACK_EXPORT
int bacnet_calendar_entry_context_decode(
    uint8_t *apdu, uint32_t apdu_max_len, uint8_t tag_number,
    BACNET_CALENDAR_ENTRY *value);

BACNET_STACK_EXPORT
bool bacapp_date_in_calendar_entry(BACNET_DATE *date,
    BACNET_CALENDAR_ENTRY *entry);

BACNET_STACK_EXPORT
bool bacnet_calendar_entry_same(
    BACNET_CALENDAR_ENTRY *value1, BACNET_CALENDAR_ENTRY *value2);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif

