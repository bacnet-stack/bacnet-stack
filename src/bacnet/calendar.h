/**************************************************************************
 *
 * Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/
#ifndef CALENDAR_H
#define CALENDAR_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
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
int bacapp_encode_CalendarEntry(uint8_t *apdu, BACNET_CALENDAR_ENTRY *value);

BACNET_STACK_EXPORT
int bacapp_encode_context_CalendarEntry(
    uint8_t *apdu, uint8_t tag_number, BACNET_CALENDAR_ENTRY *value);

BACNET_STACK_EXPORT
int bacapp_decode_CalendarEntry(
    uint8_t *apdu, uint32_t len_value, BACNET_CALENDAR_ENTRY *value);

BACNET_STACK_EXPORT
int bacapp_decode_context_CalendarEntry(
    uint8_t *apdu, uint32_t len_value, uint8_t tag_number,
    BACNET_CALENDAR_ENTRY *value);

BACNET_STACK_EXPORT
bool bacapp_date_in_calendar_entry(BACNET_DATE *date,
    BACNET_CALENDAR_ENTRY *entry);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CALENDAR_H */
