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
 *
 *********************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include "bacport.h"
#include "bacnet/basic/sys/mstimer.h"

/** @file linux/mstimer.c  Provides Linux-specific time and timer functions. */

/* counter for the various timers */
static volatile unsigned long Millisecond_Counter;

/* start time for the clock */
static struct timespec start;

/** @brief The timeGetTime function retrieves the system time, in milliseconds.
 *  The system time is the time elapsed since Windows was started.
 */
static unsigned long timeGetTime(void)
{
    struct timespec now;
    unsigned long ticks;

    clock_gettime(CLOCK_MONOTONIC, &now);

    ticks = (now.tv_sec - start.tv_sec) * 1000 +
        (now.tv_nsec - start.tv_nsec) / 1000000;

    return ticks;
}

/**
 * @brief returns the current millisecond count
 * @return millisecond counter
 */
unsigned long mstimer_now(void)
{
    unsigned long now = timeGetTime();
    unsigned long delta_time = 0;

    if (Millisecond_Counter <= now) {
        delta_time = now - Millisecond_Counter;
    } else {
        delta_time = (ULONG_MAX - Millisecond_Counter) + now + 1;
    }

    return delta_time;
}

/**
 * @brief Initialization for timer
 */
void mstimer_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &start);
}
