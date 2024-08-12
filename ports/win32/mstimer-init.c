/**************************************************************************
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
 * Multimedia Timer contribution by Cameron Crothers, 2008
 *
 * SPDX-License-Identifier: MIT
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
