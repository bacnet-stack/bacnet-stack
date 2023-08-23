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
#ifndef BACNET_SPECIAL_EVENT_H
#define BACNET_SPECIAL_EVENT_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bactimevalue.h"
#include "bacnet/calendar_entry.h"
#include "bacnet/dailyschedule.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


    typedef enum BACnet_SpecialEventPeriod_Tags {
        BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY = 0,
        BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE = 1,
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
