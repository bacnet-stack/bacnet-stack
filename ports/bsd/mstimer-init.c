/**************************************************************************
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "bacnet/basic/sys/mstimer.h"
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif
/** @file bsd/timer.c  Provides BSD-specific time and timer functions. */

/* start time for the clock */
static struct timespec start;
/* The timeGetTime function retrieves the system time, in milliseconds.
   The system time is the time elapsed since the OS was started. */
static unsigned long timeGetTime(void)
{
    struct timespec now;
    unsigned long ticks;

#ifdef __MACH__
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    now.tv_sec = mts.tv_sec;
    now.tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_MONOTONIC, &now);
#endif
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
#ifdef __MACH__
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    start.tv_sec = mts.tv_sec;
    start.tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif
}
