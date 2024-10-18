/**************************************************************************
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
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
    return timeGetTime();
}

/**
 * @brief Initialization for timer
 */
void mstimer_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &start);
}
