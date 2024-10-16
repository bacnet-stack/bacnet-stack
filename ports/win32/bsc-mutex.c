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
#include <stdio.h>
#include "bacnet/datalink/bsc/bsc-mutex.h"

static HANDLE bsc_global_mutex = NULL;

struct BSC_Mutex {
    HANDLE mutex;
};

BSC_MUTEX *bsc_mutex_init(void)
{
    BSC_MUTEX *ret;

    ret = (BSC_MUTEX *)malloc(sizeof(BSC_MUTEX));
    if (!ret) {
        return NULL;
    }

    ret->mutex = CreateMutex(
        NULL, // default security attributes
        FALSE, // initially not owned
        NULL); // unnamed mutex
    if (ret->mutex == NULL) {
        printf("CreateMutex error: %d\n", GetLastError());
        free(ret);
        return NULL;
    }

    return ret;
}

void bsc_mutex_deinit(BSC_MUTEX *mutex)
{
    CloseHandle(mutex->mutex);
    free(mutex);
}

void bsc_mutex_lock(BSC_MUTEX *mutex)
{
    WaitForSingleObject(mutex->mutex, INFINITE);
}

void bsc_mutex_unlock(BSC_MUTEX *mutex)
{
    ReleaseMutex(mutex->mutex);
}

#if BSC_MUTEX_DEBUG == 1

static long bsc_lock_cnt = 0;

static char *filename_without_full_path(char *file)
{
    int i;
    for (i = strlen(file); i >= 0; i--) {
        if (file[i] == '/' || file[i] == '\\') {
            return &file[i + 1];
        }
    }
    return file;
}

void bsc_global_mutex_lock_dbg(char *file, int line)
{
    if (bsc_global_mutex == NULL) {
        bsc_global_mutex = CreateMutex(NULL, FALSE, NULL);
    }

    file = filename_without_full_path(file);
    printf(
        "bsc_global_mutex_lock() call from %s:%d op=try_lock lock_cnt = %ld "
        "tid = %d\n",
        file, line, bsc_lock_cnt, GetCurrentThreadId());
    WaitForSingleObject(bsc_global_mutex, INFINITE);
    printf(
        "bsc_global_mutex_lock() call from %s:%d op=lock lock_cnt = %ld tid "
        "= %d\n",
        file, line, bsc_lock_cnt, GetCurrentThreadId());
    bsc_lock_cnt++;
}

void bsc_global_mutex_unlock_dbg(char *file, int line)
{
    file = filename_without_full_path(file);
    bsc_lock_cnt--;
    printf(
        "bsc_global_mutex_unlock() call from %s:%d op=unlock lock_cnt = %ld "
        " tid = %p\n",
        file, line, bsc_lock_cnt, (void *)pthread_self());
    ReleaseMutex(bsc_global_mutex);
}
#else
void bsc_global_mutex_lock(void)
{
    if (bsc_global_mutex == NULL) {
        bsc_global_mutex = CreateMutex(NULL, FALSE, NULL);
    }

    WaitForSingleObject(bsc_global_mutex, INFINITE);
}

void bsc_global_mutex_unlock(void)
{
    ReleaseMutex(bsc_global_mutex);
}
#endif

void *bsc_mutex_native(BSC_MUTEX *mutex)
{
    return (void *)mutex->mutex;
}
