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
#ifndef WEEKLYSCHEDULE_H
#define WEEKLYSCHEDULE_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
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
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_WEEKLY_SCHEDULE * value);

    /** Encode WeeklySchedule */
    BACNET_STACK_EXPORT
    int bacnet_weeklyschedule_encode(
        uint8_t * apdu,
        BACNET_WEEKLY_SCHEDULE * value);

    BACNET_STACK_EXPORT
    int bacnet_weeklyschedule_context_encode(
        uint8_t *apdu, uint8_t tag_number, BACNET_WEEKLY_SCHEDULE *value);

    BACNET_STACK_EXPORT
    int bacnet_weeklyschedule_context_decode(
        uint8_t *apdu, int max_apdu_len, uint8_t tag_number,
        BACNET_WEEKLY_SCHEDULE *value);

    BACNET_STACK_EXPORT
    bool bacnet_weeklyschedule_same(
        BACNET_WEEKLY_SCHEDULE *value1, BACNET_WEEKLY_SCHEDULE *value2);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* WEEKLYSCHEDULE_H */
