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
#include "bacnet/datalink/bsc/bsc-mutex.h"
#include "bacnet/datalink/bsc/bsc-event.h"

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
    ev->counter++;
    while (!ev->v) {
        pthread_cond_wait(&ev->cond, &ev->mutex);
    }
    ev->counter--;
    if (!ev->counter) {
        ev->v = false;
    }
    pthread_mutex_unlock(&ev->mutex);
}

void bsc_event_signal(BSC_EVENT *ev)
{
    pthread_mutex_lock(&ev->mutex);
    ev->v = true;
    pthread_cond_broadcast(&ev->cond);
    pthread_mutex_unlock(&ev->mutex);
}

void bsc_wait(int seconds)
{
    sleep(seconds);
}
