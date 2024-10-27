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
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include "bacnet/datalink/bsc/bsc-event.h"

LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

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
    LOG_INF("bsc_event_wait() >>> ev = %p v %d", ev, ev->v);
    LOG_INF("bsc_event_wait() counter before %zu", ev->counter);
    ev->counter++;
    LOG_INF("bsc_event_wait() counter %zu", ev->counter);
    while (!ev->v) {
        k_condvar_wait(&ev->cond, &ev->mutex, K_FOREVER);
    }
    LOG_INF("bsc_event_wait() ev = %p", ev);
    LOG_INF("bsc_event_wait() before counter %zu", ev->counter);
    ev->counter--;
    LOG_INF("bsc_event_wait() counter %zu", ev->counter);
    if (!ev->counter) {
        ev->v = false;
        LOG_INF("bsc_event_wait() reset ev");
    } else {
        k_condvar_broadcast(&ev->cond);
    }
    LOG_INF("bsc_event_wait() <<< ev = %p", ev);
    k_mutex_unlock(&ev->mutex);
}

bool bsc_event_timedwait(BSC_EVENT *ev, unsigned int ms_timeout)
{
    bool timedout = false;

    k_mutex_lock(&ev->mutex, K_FOREVER);
    LOG_INF("bsc_event_timedwait() >>> ev = %p, v %d", ev, ev->v);
    LOG_INF("bsc_event_timedwait() counter before %zu", ev->counter);
    ev->counter++;
    LOG_INF("bsc_event_timedwait() counter %zu", ev->counter);

    while (!ev->v) {
        if (k_condvar_wait(&ev->cond, &ev->mutex, Z_TIMEOUT_MS(ms_timeout)) ==
            -EAGAIN) {
            timedout = true;
            break;
        }
    }

    LOG_INF("bsc_event_timedwait() ev = %p", ev);
    LOG_INF("bsc_event_timedwait() before counter %zu", ev->counter);
    ev->counter--;
    LOG_INF("bsc_event_timedwait() counter %zu", ev->counter);
    if (!timedout && !ev->counter) {
        ev->v = false;
        LOG_INF("bsc_event_timedwait() reset ev");
    } else {
        k_condvar_broadcast(&ev->cond);
    }

    LOG_INF("bsc_event_timedwait() <<< ev = %p", ev);
    k_mutex_unlock(&ev->mutex);
    return !timedout;
}

void bsc_event_signal(BSC_EVENT *ev)
{
    LOG_INF("bsc_event_signal() >>> ev = %p", ev);
    k_mutex_lock(&ev->mutex, K_FOREVER);
    ev->v = true;
    k_condvar_broadcast(&ev->cond);
    k_mutex_unlock(&ev->mutex);
    LOG_INF("bsc_event_signal() <<< ev = %p", ev);
}

void bsc_wait(int seconds)
{
    k_msleep(seconds * 1000);
}

void bsc_wait_ms(int mseconds)
{
    k_msleep(mseconds);
}
