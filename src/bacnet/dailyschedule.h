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
#ifndef DAILYSCHEDULE_H
#define DAILYSCHEDULE_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bactimevalue.h"

/* arbitrary value, shall be unlimited for B-OWS but we don't care, 640k shall be enough */
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
        uint16_t TV_Count;      /* the number of time values actually used */
    } BACNET_DAILY_SCHEDULE;

    /** Decode DailySchedule (sequence of times and values) */
    BACNET_STACK_EXPORT
    int bacnet_dailyschedule_decode(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_DAILY_SCHEDULE * day);

    /** Encode DailySchedule (sequence of times and values) */
    BACNET_STACK_EXPORT
    int bacnet_dailyschedule_encode(
        uint8_t * apdu,
        BACNET_DAILY_SCHEDULE * day);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DAILYSCHEDULE_H */
