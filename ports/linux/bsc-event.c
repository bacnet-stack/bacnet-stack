/**
 * @file
 * @brief  Implementation of mutex abstraction used in BACNet secure connect.
 * @author Kirill Neznamov
 * @date August 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bsc-event.h"

#define DEBUG_BSC_EVENT 0

#if DEBUG_BSC_EVENT == 1
#define DEBUG_PRINTF printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF debug_printf_disabled
#endif

struct BSC_Event {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool v;
    size_t counter;
};

BSC_EVENT *bsc_event_init(void)
{
    BSC_EVENT *ev;
    pthread_mutexattr_t attr;

    if (pthread_mutexattr_init(&attr) != 0) {
        return NULL;
    }

    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
        pthread_mutexattr_destroy(&attr);
        return NULL;
    }

    ev = malloc(sizeof(BSC_EVENT));

    if (!ev) {
        pthread_mutexattr_destroy(&attr);
        return NULL;
    }

    if (pthread_mutex_init(&ev->mutex, &attr) != 0) {
        pthread_mutexattr_destroy(&attr);
        free(ev);
        return NULL;
    }

    pthread_mutexattr_destroy(&attr);

    if (pthread_cond_init(&ev->cond, NULL) != 0) {
        pthread_mutex_destroy(&ev->mutex);
        free(ev);
        return NULL;
    }

    ev->v = false;
    ev->counter = 0;
    return ev;
}

void bsc_event_deinit(BSC_EVENT *ev)
{
    pthread_mutex_destroy(&ev->mutex);
    pthread_cond_destroy(&ev->cond);
    free(ev);
}

void bsc_event_wait(BSC_EVENT *ev)
{
    pthread_mutex_lock(&ev->mutex);
    DEBUG_PRINTF("bsc_event_wait() >>> ev = %p\n", ev);
    DEBUG_PRINTF("bsc_event_wait() counter before %zu\n", ev->counter);
    ev->counter++;
    DEBUG_PRINTF("bsc_event_wait() counter %zu\n", ev->counter);
    while (!ev->v) {
        pthread_cond_wait(&ev->cond, &ev->mutex);
    }
    DEBUG_PRINTF("bsc_event_wait() ev = %p\n", ev);
    DEBUG_PRINTF("bsc_event_wait() before counter %zu\n", ev->counter);
    ev->counter--;
    DEBUG_PRINTF("bsc_event_wait() counter %zu\n", ev->counter);
    if (!ev->counter) {
        ev->v = false;
        DEBUG_PRINTF("bsc_event_wait() reset ev\n");
    } else {
        DEBUG_PRINTF("bsc_event_wait() wake up other waiting threads\n");
        pthread_cond_broadcast(&ev->cond);
    }
    DEBUG_PRINTF("bsc_event_wait() <<< ev = %p\n", ev);
    pthread_mutex_unlock(&ev->mutex);
}

bool bsc_event_timedwait(BSC_EVENT *ev, unsigned int ms_timeout)
{
    struct timespec to;
    int r = 0;

    clock_gettime(CLOCK_REALTIME, &to);
    to.tv_sec = to.tv_sec + ms_timeout / 1000;
    to.tv_nsec = to.tv_nsec + (ms_timeout % 1000) * 1000000;
    to.tv_sec += to.tv_nsec / 1000000000;
    to.tv_nsec %= 1000000000;

    DEBUG_PRINTF(
        "bsc_event_timedwait() >>> before lock ev = %p ev->v = %d\n", ev,
        ev->v);

    pthread_mutex_lock(&ev->mutex);

    DEBUG_PRINTF(
        "bsc_event_timedwait() >>> after lock ev = %p ev->v = %d\n", ev, ev->v);

    ev->counter++;
    DEBUG_PRINTF("bsc_event_timedwait() counter %zu\n", ev->counter);

    while (!ev->v) {
        r = pthread_cond_timedwait(&ev->cond, &ev->mutex, &to);
        if (r != 0) {
            break;
        }
    }

    if (ev->v) {
        if (r != 0) {
            DEBUG_PRINTF("Fired!!! r = %d\n", r);
        }
        r = 0;
    }

    ev->counter--;
    DEBUG_PRINTF("bsc_event_timedwait() counter %zu\n", ev->counter);

    if (!ev->counter) {
        DEBUG_PRINTF("bsc_event_timedwait() event is reset, err = %d\n", r);
        ev->v = false;
    } else {
        DEBUG_PRINTF("bsc_event_timedwait() wake up other waiting threads\n");
        pthread_cond_broadcast(&ev->cond);
    }

    DEBUG_PRINTF(
        "bsc_event_timedwait() <<< ret = %d, ev = %p ev->v = %d r = %d\n",
        r == 0, ev, ev->v, r);
    pthread_mutex_unlock(&ev->mutex);
    return r == 0;
}

void bsc_event_signal(BSC_EVENT *ev)
{
    DEBUG_PRINTF("bsc_event_signal() >>> ev = %p\n", ev);
    pthread_mutex_lock(&ev->mutex);
    DEBUG_PRINTF("bsc_event_signal() >>> ev = %p\n", ev);
    ev->v = true;
    pthread_cond_broadcast(&ev->cond);
    DEBUG_PRINTF("bsc_event_signal() <<< ev = %p\n", ev);
    pthread_mutex_unlock(&ev->mutex);
    DEBUG_PRINTF("bsc_event_signal() <<< ev = %p\n", ev);
}

void bsc_wait(int seconds)
{
    sleep(seconds);
}

void bsc_wait_ms(int mseconds)
{
    usleep(mseconds * 1000);
}
