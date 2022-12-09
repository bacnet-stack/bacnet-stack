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

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bacnet/datalink/bsc/bsc-mutex.h"

static pthread_mutex_t bsc_global_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

struct BSC_Mutex {
    pthread_mutex_t mutex;
};

BSC_MUTEX *bsc_mutex_init(void)
{
    pthread_mutexattr_t attr;
    BSC_MUTEX *ret;

    if (pthread_mutexattr_init(&attr) != 0) {
        return NULL;
    }

    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
        pthread_mutexattr_destroy(&attr);
        return NULL;
    }

    ret = (BSC_MUTEX *)malloc(sizeof(BSC_MUTEX));

    if (!ret) {
        pthread_mutexattr_destroy(&attr);
        return NULL;
    }

    if (pthread_mutex_init(&ret->mutex, &attr) != 0) {
        pthread_mutexattr_destroy(&attr);
        free(ret);
        return NULL;
    }

    pthread_mutexattr_destroy(&attr);
    return ret;
}

void bsc_mutex_deinit(BSC_MUTEX *mutex)
{
    pthread_mutex_destroy(&mutex->mutex);
    free(mutex);
}

void bsc_mutex_lock(BSC_MUTEX *mutex)
{
    pthread_mutex_lock(&mutex->mutex);
}

void bsc_mutex_unlock(BSC_MUTEX *mutex)
{
    pthread_mutex_unlock(&mutex->mutex);
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
    file = filename_without_full_path(file);
    printf("bsc_global_mutex_lock() call from %s:%d op=try_lock lock_cnt = %ld "
           "tid = %p\n",
        file, line, bsc_lock_cnt, (void*)pthread_self());
    pthread_mutex_lock(&bsc_global_mutex);
    printf("bsc_global_mutex_lock() call from %s:%d op=lock lock_cnt = %ld tid "
           "= %p\n",
        file, line, bsc_lock_cnt, (void*)pthread_self());
    bsc_lock_cnt++;
}

void bsc_global_mutex_unlock_dbg(char *file, int line)
{
    file = filename_without_full_path(file);
    bsc_lock_cnt--;
    printf("bsc_global_mutex_unlock() call from %s:%d op=unlock lock_cnt = %ld "
           " tid = %p\n",
        file, line, bsc_lock_cnt, (void*)pthread_self());
    pthread_mutex_unlock(&bsc_global_mutex);
}
#else
void bsc_global_mutex_lock(void)
{
    pthread_mutex_lock(&bsc_global_mutex);
}

void bsc_global_mutex_unlock(void)
{
    pthread_mutex_unlock(&bsc_global_mutex);
}
#endif

void* bsc_mutex_native(BSC_MUTEX *mutex)
{
    return (void *)&mutex->mutex;
}
