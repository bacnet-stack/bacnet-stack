/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
* Multimedia Timer contribution by Cameron Crothers, 2008
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
*
*********************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#define STRICT 1
#include <windows.h>
#include <MMSystem.h>
#include "timer.h"

/* counter for the various timers */
static volatile uint32_t Millisecond_Counter[MAX_MILLISECOND_TIMERS];

/*************************************************************************
* Description: simulate the gettimeofday Linux function
* Returns: none
* Notes: none
*************************************************************************/
int gettimeofday(
    struct timeval *tp,
    void *tzp)
{
    struct _timeb timebuffer;

    _ftime(&timebuffer);
    tp->tv_sec = timebuffer.time;
    tp->tv_usec = timebuffer.millitm * 1000;

    return 0;
}

/*************************************************************************
* Description: returns the current millisecond count
* Returns: none
* Notes: none
*************************************************************************/
uint32_t timer_milliseconds(
    unsigned index)
{
    uint32_t now = timeGetTime();
    uint32_t delta_time = 0;


    if (index < MAX_MILLISECOND_TIMERS) {
        if (Millisecond_Counter[index] <= now) {
            delta_time = now - Millisecond_Counter[index];
        } else {
            delta_time = (UINT32_MAX - Millisecond_Counter[index]) + now + 1;
        }
    }

    return delta_time;
}

/*************************************************************************
* Description: compares the current time count with a value
* Returns: true if the time has elapsed
* Notes: none
*************************************************************************/
bool timer_elapsed_milliseconds(
    unsigned index,
    uint32_t value)
{
    return (timer_milliseconds(index) >= value);
}

/*************************************************************************
* Description: compares the current time count with a value
* Returns: true if the time has elapsed
* Notes: none
*************************************************************************/
bool timer_elapsed_seconds(
    unsigned index,
    uint32_t seconds)
{
    return ((timer_milliseconds(index) / 1000) >= seconds);
}

/*************************************************************************
* Description: compares the current time count with a value
* Returns: true if the time has elapsed
* Notes: none
*************************************************************************/
bool timer_elapsed_minutes(
    unsigned index,
    uint32_t minutes)
{
    return ((timer_milliseconds(index) / (1000 * 60)) >= minutes);
}

/*************************************************************************
* Description: Sets the timer counter to zero.
* Returns: none
* Notes: none
*************************************************************************/
uint32_t timer_reset(
    unsigned index)
{
    uint32_t timer_value = timer_milliseconds(index);
    if (index < MAX_MILLISECOND_TIMERS) {
        Millisecond_Counter[index] = timeGetTime();
    }
    return timer_value;
}

/*************************************************************************
* Description: Initialization for timer
* Returns: none
* Notes: none
*************************************************************************/
void timer_init(
    void)
{
    TIMECAPS tc;
    uint32_t period;

    /* set timer resolution */
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
        fprintf(stderr, "Failed to get timer resolution parameters\n");
    }
    /* configure for 1ms resolution - if possible */
    period = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
    timeBeginPeriod(period);
}
