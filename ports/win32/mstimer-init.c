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
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "bacport.h"
#include "bacnet/basic/sys/mstimer.h"

/* Windows timer period - in milliseconds */
static unsigned long Timer_Period = 1;

/**
 * @brief returns the current millisecond count
 * @return millisecond counter
 */
unsigned long mstimer_now(void)
{
    return timeGetTime();
}

/**
 * @brief Shut down for timer
 */
static void timer_cleanup(void)
{
    timeEndPeriod(Timer_Period);
}

/**
 * @brief Initialization for timer
 */
void mstimer_init(void)
{
    TIMECAPS tc;

    /* set timer resolution */
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
        fprintf(stderr, "Failed to get timer resolution parameters\n");
    }
    /* configure for 1ms resolution - if possible */
    Timer_Period = min(max(tc.wPeriodMin, 1L), tc.wPeriodMax);
    if (Timer_Period != 1L) {
        fprintf(stderr,
            "Failed to set timer to 1ms.  "
            "Time period set to %ums\n",
            (unsigned)Timer_Period);
    }
    timeBeginPeriod(Timer_Period);
    atexit(timer_cleanup);
}
