/**
 * @file
 * @brief  Implementation of mutex abstraction used in BACNet secure connect.
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date August 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
        NULL, // default security attributes
        FALSE, // initially not owned
        NULL); // unnamed mutex

    if (ret->mutex == NULL) {
        free(ret);
        return NULL;
    }

    ret->event = CreateEvent(
        NULL, // default security attributes
        TRUE, // manual-reset event
        FALSE, // initial state is nonsignaled
        NULL // unnamed event
    );

    if (ret->event == NULL) {
        CloseHandle(ret->mutex);
        free(ret);
        return NULL;
    }

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
    DEBUG_PRINTF("bsc_event_wait() >>> ev = %p\n", ev);
    WaitForSingleObject(ev->mutex, INFINITE);
    DEBUG_PRINTF("bsc_event_wait() counter before %zu\n", ev->counter);
    ev->counter++;
    ReleaseMutex(ev->mutex);
    WaitForSingleObject(ev->event, INFINITE);
    WaitForSingleObject(ev->mutex, INFINITE);
    DEBUG_PRINTF("bsc_event_wait() before counter %zu\n", ev->counter);
    ev->counter--;
    DEBUG_PRINTF("bsc_event_wait() counter %zu\n", ev->counter);
    if (!ev->counter) {
        DEBUG_PRINTF("bsc_event_wait() reset event\n");
        ReleaseMutex(ev->mutex);
        ResetEvent(ev->event);
    } else {
        DEBUG_PRINTF("bsc_event_wait() set event\n");
        ReleaseMutex(ev->mutex);
    }
    DEBUG_PRINTF("bsc_event_wait() <<< ev = %p\n", ev);
}

bool bsc_event_timedwait(BSC_EVENT *ev, unsigned int ms_timeout)
{
    bool timedout = false;
    DWORD ret;

    DEBUG_PRINTF("bsc_event_timedwait() >>> ev = %p\n", ev);
    WaitForSingleObject(ev->mutex, INFINITE);
    DEBUG_PRINTF("bsc_event_timedwait() counter before %zu\n", ev->counter);
    ev->counter++;
    DEBUG_PRINTF("bsc_event_timedwait() counter %zu\n", ev->counter);
    ReleaseMutex(ev->mutex);

    ret = WaitForSingleObject(ev->event, ms_timeout);
    WaitForSingleObject(ev->mutex, INFINITE);

    DEBUG_PRINTF("bsc_event_timedwait() before counter %zu\n", ev->counter);
    ev->counter--;
    DEBUG_PRINTF("bsc_event_timedwait() counter %zu\n", ev->counter);
    if (!ev->counter) {
        ReleaseMutex(ev->mutex);
        if (ret == WAIT_OBJECT_0) {
            ResetEvent(ev->event);
        }
    } else {
        ReleaseMutex(ev->mutex);
    }
    DEBUG_PRINTF(
        "bsc_event_timedwait() <<< ret = %d\n",
        ret == WAIT_OBJECT_0 ? true : false);
    return ret == WAIT_OBJECT_0 ? true : false;
}

void bsc_event_signal(BSC_EVENT *ev)
{
    DEBUG_PRINTF("bsc_event_signal() >>> ev = %p\n", ev);
    SetEvent(ev->event);
    DEBUG_PRINTF("bsc_event_signal() <<< ev = %p\n", ev);
}

void bsc_wait(int seconds)
{
    Sleep(seconds * 1000);
}

void bsc_wait_ms(int mseconds)
{
    Sleep(mseconds);
}
