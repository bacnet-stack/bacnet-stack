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
#include <zephyr.h>
#include <stdlib.h>
#include "bacnet/datalink/bsc/bsc-mutex.h"
#include "bacnet/datalink/bsc/bsc-event.h"

#define DEBUG_BSC_EVENT 0

#if DEBUG_BSC_EVENT == 1
#define DEBUG_PRINTF printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF(...)
#endif

struct BSC_Event {
    struct k_mutex mutex;
    struct k_condvar cond;
    bool v;
    size_t counter;
};

BSC_EVENT *bsc_event_init(void)
{
    BSC_EVENT *ev;

    ev = malloc(sizeof(BSC_EVENT));

    if (!ev) {
        return NULL;
    }

    if (k_mutex_init(&ev->mutex) != 0) {
        free(ev);
        return NULL;
    }

    if (k_condvar_init(&ev->cond) != 0) {
        free(ev);
        return NULL;
    }

    ev->v = false;
    ev->counter = 0;
    return ev;
}

void bsc_event_deinit(BSC_EVENT *ev)
{
    free(ev);
}

void bsc_event_wait(BSC_EVENT *ev)
{
    k_mutex_lock(&ev->mutex, K_FOREVER);
    DEBUG_PRINTF("bsc_event_wait() >>> ev = %p\n", ev);
    DEBUG_PRINTF("bsc_event_wait() counter before %zu\n", ev->counter);
    ev->counter++;
    DEBUG_PRINTF("bsc_event_wait() counter %zu\n", ev->counter);
    while (!ev->v) {
        k_condvar_wait(&ev->cond, &ev->mutex, K_FOREVER);
    }
    DEBUG_PRINTF("bsc_event_wait() ev = %p\n", ev);
    DEBUG_PRINTF("bsc_event_wait() before counter %zu\n", ev->counter);
    ev->counter--;
    DEBUG_PRINTF("bsc_event_wait() counter %zu\n", ev->counter);
    if (!ev->counter) {
        ev->v = false;
        DEBUG_PRINTF("bsc_event_wait() reset ev\n");
    }
    DEBUG_PRINTF("bsc_event_wait() <<< ev = %p\n", ev);
    k_mutex_unlock(&ev->mutex);
}

bool bsc_event_timedwait(BSC_EVENT *ev, unsigned int ms_timeout)
{
    bool timedout = false;

    k_mutex_lock(&ev->mutex, K_FOREVER);
    DEBUG_PRINTF("bsc_event_timedwait() >>> ev = %p\n", ev);
    DEBUG_PRINTF("bsc_event_timedwait() counter before %zu\n", ev->counter);
    ev->counter++;
    DEBUG_PRINTF("bsc_event_timedwait() counter %zu\n", ev->counter);

    while (!ev->v) {
        if (k_condvar_wait(&ev->cond, &ev->mutex, Z_TIMEOUT_MS(ms_timeout)) ==
             -EAGAIN) {
            timedout = true;
            break;
        }
    }

    DEBUG_PRINTF("bsc_event_timedwait() ev = %p\n", ev);
    DEBUG_PRINTF("bsc_event_timedwait() before counter %zu\n", ev->counter);
    ev->counter--;
    DEBUG_PRINTF("bsc_event_timedwait() counter %zu\n", ev->counter);
    if (!timedout && !ev->counter) {
        ev->v = false;
        DEBUG_PRINTF("bsc_event_timedwait() reset ev\n");
    }
    DEBUG_PRINTF("bsc_event_timedwait() <<< ev = %p\n", ev);
    k_mutex_unlock(&ev->mutex);
    return !timedout;
}

void bsc_event_signal(BSC_EVENT *ev)
{
    DEBUG_PRINTF("bsc_event_signal() >>> ev = %p\n", ev);
    k_mutex_lock(&ev->mutex, K_FOREVER);
    ev->v = true;
    k_condvar_broadcast(&ev->cond);
    k_mutex_unlock(&ev->mutex);
    DEBUG_PRINTF("bsc_event_signal() <<< ev = %p\n", ev);
}

void bsc_event_reset(BSC_EVENT *ev)
{
    k_mutex_lock(&ev->mutex, K_FOREVER);
    ev->v = false;
    ev->counter = 0;
    k_mutex_unlock(&ev->mutex);
}

void bsc_wait(int seconds)
{
    k_msleep(seconds * 1000);
}
