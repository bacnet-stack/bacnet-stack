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
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
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
    HANDLE mutex;
    HANDLE event;
    bool v;
    size_t counter;
};

BSC_EVENT *bsc_event_init(void)
{
    BSC_EVENT *ret;

    ret = (BSC_EVENT *)malloc(sizeof(BSC_EVENT));
    if (!ret) {
        return NULL;
    }

    ret->mutex = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex
    if (ret->mutex == NULL) {
        printf("CreateMutex error: %d\n", GetLastError());
        free(ret);
        return NULL;
    }

    ret->event = CreateEvent( 
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        NULL                // unnamed event
        ); 
    if (ret->event == NULL) {
        printf("CreateEvent error: %d\n", GetLastError());
        CloseHandle(ret->mutex);
        free(ret);
        return NULL;
    }

    ret->v = false;
    ret->counter = 0;

    return ret;
}

void bsc_event_deinit(BSC_EVENT *ev)
{
    CloseHandle(ev->mutex);
    CloseHandle(ev->event);
    free(ev);
}

void bsc_event_wait(BSC_EVENT *ev)
{
    bool v;
    DEBUG_PRINTF("bsc_event_wait() >>> ev = %p\n", ev);
    WaitForSingleObject(ev->mutex, INFINITE);
    DEBUG_PRINTF("bsc_event_wait() counter before %zu\n", ev->counter);
    ev->counter++;
    DEBUG_PRINTF("bsc_event_wait() counter %zu\n", ev->counter);
    v = ev->v;
    ReleaseMutex(ev->mutex);
    while (!v) {
        WaitForSingleObject(ev->event, INFINITE);
        WaitForSingleObject(ev->mutex, INFINITE);
        v = ev->v;
        ReleaseMutex(ev->mutex);
    }

    DEBUG_PRINTF("bsc_event_wait() ev = %p\n", ev);
    WaitForSingleObject(ev->mutex, INFINITE);
    DEBUG_PRINTF("bsc_event_wait() before counter %zu\n", ev->counter);
    ev->counter--;
    DEBUG_PRINTF("bsc_event_wait() counter %zu\n", ev->counter);
    if (!ev->counter) {
        ev->v = false;
        DEBUG_PRINTF("bsc_event_wait() reset ev\n");
    }
    DEBUG_PRINTF("bsc_event_wait() <<< ev = %p\n", ev);
    ReleaseMutex(ev->mutex);
}

bool bsc_event_timedwait(BSC_EVENT *ev, unsigned int ms_timeout)
{
    bool v;
    bool timedout = false;
    ULONGLONG to;
    DWORD st = WAIT_OBJECT_0;

    to = GetTickCount64() + ms_timeout;

    DEBUG_PRINTF("bsc_event_timedwait() >>> ev = %p\n", ev);
    WaitForSingleObject(ev->mutex, INFINITE);
    DEBUG_PRINTF("bsc_event_timedwait() counter before %zu\n", ev->counter);
    ev->counter++;
    DEBUG_PRINTF("bsc_event_timedwait() counter %zu\n", ev->counter);
    v = ev->v;
    ReleaseMutex(ev->mutex);

    while (!v && (st != WAIT_TIMEOUT)) {
        st = WaitForSingleObject(ev->event, to - GetTickCount64());
        WaitForSingleObject(ev->mutex, INFINITE);
        v = ev->v;
        ReleaseMutex(ev->mutex);
    }

    DEBUG_PRINTF("bsc_event_timedwait() ev = %p\n", ev);
    WaitForSingleObject(ev->mutex, INFINITE);
    DEBUG_PRINTF("bsc_event_timedwait() before counter %zu\n", ev->counter);
    ev->counter--;
    DEBUG_PRINTF("bsc_event_timedwait() counter %zu\n", ev->counter);
    if (!ev->counter && (st != WAIT_TIMEOUT)) {
        ev->v = false;
        DEBUG_PRINTF("bsc_event_timedwait() reset ev\n");
    }
    DEBUG_PRINTF("bsc_event_timedwait() <<< ev = %p\n", ev);
    ReleaseMutex(ev->mutex);

    return st != WAIT_TIMEOUT;
}

void bsc_event_signal(BSC_EVENT *ev)
{
    DEBUG_PRINTF("bsc_event_signal() >>> ev = %p\n", ev);
    WaitForSingleObject(ev->mutex, INFINITE);
    ev->v = true;
    SetEvent(ev->event);
    ReleaseMutex(ev->mutex);
    DEBUG_PRINTF("bsc_event_signal() <<< ev = %p\n", ev);
}

void bsc_event_reset(BSC_EVENT *ev)
{
    WaitForSingleObject(ev->mutex, INFINITE);
    ev->v = false;
    ev->counter = 0;
    ReleaseMutex(ev->mutex);
}

void bsc_wait(int seconds)
{
    sleep(seconds);
}
