/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
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
#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdint.h>

/* Timer Module */
/* reserve the millisecond timer indexes as needed for each module */
#define TIMER_SILENCE 0
#define TIMER_DEBOUNCE 1
#define TIMER_LED_1 2
#define TIMER_LED_2 3
#define TIMER_LED_3 4
#define TIMER_LED_4 5
#define TIMER_DCC 6
#define MAX_MILLISECOND_TIMERS 7

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void timer_init(
        void);
    unsigned long timer_milliseconds(
        unsigned index);
    bool timer_elapsed_milliseconds(
        unsigned index,
        unsigned long value);
    bool timer_elapsed_seconds(
        unsigned index,
        unsigned long value);
    bool timer_elapsed_minutes(
        unsigned index,
        unsigned long seconds);
    unsigned long timer_milliseconds_set(
        unsigned index,
        unsigned long value);
    unsigned long timer_reset(
        unsigned index);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
