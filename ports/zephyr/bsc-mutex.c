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

static K_MUTEX_DEFINE(bsc_global_mutex);

struct BSC_Mutex {
    struct k_mutex mutex;
};

BSC_MUTEX *bsc_mutex_init(void)
{
    BSC_MUTEX *ret;

    ret = (BSC_MUTEX *)malloc(sizeof(BSC_MUTEX));

    if (!ret) {
        return NULL;
    }

    if (k_mutex_init(&ret->mutex) != 0) {
        free(ret);
        return NULL;
    }

    return ret;
}

void bsc_mutex_deinit(BSC_MUTEX *mutex)
{
    free(mutex);
}

void bsc_mutex_lock(BSC_MUTEX *mutex)
{
    k_mutex_lock(&mutex->mutex, K_FOREVER);
}

void bsc_mutex_unlock(BSC_MUTEX *mutex)
{
    k_mutex_unlock(&mutex->mutex);
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
        file, line, bsc_lock_cnt, (void*)k_current_get());
    k_mutex_lock(&bsc_global_mutex, K_FOREVER);
    printf("bsc_global_mutex_lock() call from %s:%d op=lock lock_cnt = %ld tid "
           "= %p\n",
        file, line, bsc_lock_cnt, (void*)k_current_get());
    bsc_lock_cnt++;
}

void bsc_global_mutex_unlock_dbg(char *file, int line)
{
    file = filename_without_full_path(file);
    bsc_lock_cnt--;
    printf("bsc_global_mutex_unlock() call from %s:%d op=unlock lock_cnt = %ld "
           " tid = %p\n",
        file, line, bsc_lock_cnt, (void*)k_current_get());
    k_mutex_unlock(&bsc_global_mutex);
}
#else
void bsc_global_mutex_lock(void)
{
    k_mutex_lock(&bsc_global_mutex, K_FOREVER);
}

void bsc_global_mutex_unlock(void)
{
    k_mutex_unlock(&bsc_global_mutex);
}
#endif

void* bsc_mutex_native(BSC_MUTEX *mutex)
{
    return (void *)&mutex->mutex;
}
