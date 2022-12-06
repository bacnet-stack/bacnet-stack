/**
 * @file
 * @brief BACNet secure connect API.
 * @author Kirill Neznamov
 * @date September 2022
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
#include <errno.h>
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bsc-runloop.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bsc-mutex.h"

#define DEBUG_BSC_RUNLOOP 0

#if DEBUG_BSC_RUNLOOP == 1
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF(...)
#endif

#define BSC_DEFAULT_RUNLOOP_TIMEOUT_MS (1 * 1000)

typedef struct {
    void *ctx;
    void (*runloop_func)(void *ctx);
} BSC_RUNLOOP_CTX;

struct BSC_RunLoop {
    bool used;
    BSC_RUNLOOP_CTX runloop_ctx[BSC_RUNLOOP_CALLBACKS_NUM];
    bool started;
    bool process;
    bool changed;
    pthread_mutex_t *mutex;
    pthread_cond_t cond;
    pthread_t thread_id;
};

static pthread_mutex_t bsc_mutex_global = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static BSC_RUNLOOP bsc_runloop_global = { true, { 0 }, false, false, false,
    &bsc_mutex_global, { 0 }, 0 };
static BSC_RUNLOOP bsc_runloop_local[BSC_RUNLOOP_LOCAL_NUM];
static pthread_mutex_t bsc_mutex_local[BSC_RUNLOOP_LOCAL_NUM] = {
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER
};

BSC_RUNLOOP *bsc_global_runloop(void)
{
    return &bsc_runloop_global;
}

BSC_RUNLOOP *bsc_local_runloop_alloc(void)
{
    int i;
    pthread_mutex_lock(bsc_global_runloop()->mutex);
    for (i = 0; i < BSC_RUNLOOP_LOCAL_NUM; i++) {
        if (!bsc_runloop_local[i].used) {
            bsc_runloop_local[i].mutex = &bsc_mutex_local[i];
            bsc_runloop_local[i].used = true;
            pthread_mutex_unlock(bsc_global_runloop()->mutex);
            return &bsc_runloop_local[i];
        }
    }
    pthread_mutex_unlock(bsc_global_runloop()->mutex);
    return NULL;
}

void bsc_local_runloop_free(BSC_RUNLOOP *runloop)
{
    pthread_mutex_lock(bsc_global_runloop()->mutex);
    runloop->used = false;
    pthread_mutex_unlock(bsc_global_runloop()->mutex);
}

static void *bsc_runloop_worker(void *arg)
{
    BSC_RUNLOOP *rl = (BSC_RUNLOOP *)arg;
    BSC_RUNLOOP_CTX local[BSC_RUNLOOP_CALLBACKS_NUM];
    int i;
    struct timespec to;

    DEBUG_PRINTF("bsc_runloop_worker() >>>\n");

    pthread_mutex_lock(rl->mutex);
    memcpy(local, rl->runloop_ctx, sizeof(local));
    pthread_mutex_unlock(rl->mutex);

    while (1) {
        // DEBUG_PRINTF("bsc_runloop_worker() waiting for next iteration\n");
        pthread_mutex_lock(rl->mutex);
        clock_gettime(CLOCK_REALTIME, &to);
        to.tv_sec = to.tv_sec + BSC_DEFAULT_RUNLOOP_TIMEOUT_MS / 1000;
        to.tv_nsec =
            to.tv_nsec + (BSC_DEFAULT_RUNLOOP_TIMEOUT_MS % 1000) * 1000000;
        to.tv_sec += to.tv_nsec / 1000000000;
        to.tv_nsec %= 1000000000;

        while (!rl->process) {
            if (pthread_cond_timedwait(&rl->cond, rl->mutex, &to) ==
                ETIMEDOUT) {
                break;
            }
        }
        // DEBUG_PRINTF("bsc_runloop_worker() processing started\n");

        rl->process = false;

        if (rl->changed) {
            DEBUG_PRINTF("bsc_runloop_worker() processing context changes\n");
            rl->changed = false;
            memcpy(local, rl->runloop_ctx, sizeof(local));
        }

        if (!rl->started) {
            DEBUG_PRINTF("bsc_runloop_worker() runloop is stopped\n");
            pthread_mutex_unlock(rl->mutex);
            break;
        } else {
            pthread_mutex_unlock(rl->mutex);
            for (i = 0; i < BSC_RUNLOOP_CALLBACKS_NUM; i++) {
                if (local[i].ctx) {
                    local[i].runloop_func(local[i].ctx);
                }
            }
        }
    }
    DEBUG_PRINTF("bsc_runloop_worker() <<<\n");
    return NULL;
}

BSC_SC_RET bsc_runloop_start(BSC_RUNLOOP *runloop)
{
    int ret;

    if (runloop == bsc_global_runloop()) {
        DEBUG_PRINTF("bsc_runloop_start() >>> runloop global(%p)\n", runloop);
    } else {
        DEBUG_PRINTF("bsc_runloop_start() >>> runloop = %p\n", runloop);
    }

    pthread_mutex_lock(runloop->mutex);

    if (runloop->started) {
        pthread_mutex_unlock(runloop->mutex);
        return BSC_SC_INVALID_OPERATION;
    }

    if (pthread_cond_init(&runloop->cond, NULL) != 0) {
        DEBUG_PRINTF("bsc_runloop_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    ret =
        pthread_create(&runloop->thread_id, NULL, &bsc_runloop_worker, runloop);

    if (ret != 0) {
        pthread_cond_destroy(&runloop->cond);
        DEBUG_PRINTF("bsc_runloop_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    memset(runloop->runloop_ctx, 0, sizeof(runloop->runloop_ctx));
    runloop->started = true;
    pthread_mutex_unlock(runloop->mutex);
    DEBUG_PRINTF("bsc_runloop_start() >>> ret = BSC_SC_SUCCESS\n");
    return BSC_SC_SUCCESS;
}

BSC_SC_RET bsc_runloop_reg(
    BSC_RUNLOOP *runloop, void *ctx, void (*runloop_func)(void *ctx))
{
    int i;

    if (runloop == bsc_global_runloop()) {
        DEBUG_PRINTF(
            "bsc_runloop_reg() >>> runloop global (%p), ctx = %p, func = %p\n",
            runloop, ctx, runloop_func);
    } else {
        DEBUG_PRINTF(
            "bsc_runloop_reg() >>> runloop = %p, ctx = %p, func = %p\n",
            runloop, ctx, runloop_func);
    }

    pthread_mutex_lock(runloop->mutex);

    if (runloop->started) {
        for (i = 0; i < BSC_RUNLOOP_CALLBACKS_NUM; i++) {
            if (!runloop->runloop_ctx[i].ctx) {
                runloop->runloop_ctx[i].ctx = ctx;
                runloop->runloop_ctx[i].runloop_func = runloop_func;
                runloop->changed = true;
                pthread_mutex_unlock(runloop->mutex);
                DEBUG_PRINTF(
                    "bsc_runloop_schedule() <<< ret = BSC_SC_SUCCESS\n");
                return BSC_SC_SUCCESS;
            }
        }
        pthread_mutex_unlock(runloop->mutex);
        DEBUG_PRINTF("bsc_runloop_schedule() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    pthread_mutex_unlock(runloop->mutex);
    DEBUG_PRINTF("bsc_runloop_schedule() <<< ret = BSC_SC_INVALID_OPERATION\n");
    return BSC_SC_INVALID_OPERATION;
}

void bsc_runloop_schedule(BSC_RUNLOOP *runloop)
{
    if (runloop == bsc_global_runloop()) {
        DEBUG_PRINTF(
            "bsc_runloop_schedule() >>> runloop global (%p)\n", runloop);
    } else {
        DEBUG_PRINTF("bsc_runloop_schedule() >>> runloop = %p\n", runloop);
    }

    pthread_mutex_lock(runloop->mutex);
    if (runloop->started) {
        runloop->process = 1;
        pthread_cond_signal(&runloop->cond);
    }
    pthread_mutex_unlock(runloop->mutex);
    DEBUG_PRINTF("bsc_runloop_schedule() <<<\n");
}

void bsc_runloop_unreg(BSC_RUNLOOP *runloop, void *ctx)
{
    int i;
    if (runloop == bsc_global_runloop()) {
        DEBUG_PRINTF("bsc_runloop_unreg() >>> runloop global (%p), ctx = %p\n",
            runloop, ctx);
    } else {
        DEBUG_PRINTF(
            "bsc_runloop_unreg() >>> runloop = %p, ctx = %p\n", runloop, ctx);
    }

    pthread_mutex_lock(runloop->mutex);
    for (i = 0; i < BSC_RUNLOOP_CALLBACKS_NUM; i++) {
        if (runloop->runloop_ctx[i].ctx == ctx) {
            runloop->runloop_ctx[i].ctx = NULL;
            runloop->runloop_ctx[i].runloop_func = NULL;
            runloop->changed = true;
            break;
        }
    }
    pthread_mutex_unlock(runloop->mutex);
    DEBUG_PRINTF("bsc_runloop_unreg() <<<\n");
}

void bsc_runloop_stop(BSC_RUNLOOP *runloop)
{
    if (runloop == bsc_global_runloop()) {
        DEBUG_PRINTF("bsc_runloop_stop() >>> runloop global (%p)\n", runloop);
    } else {
        DEBUG_PRINTF("bsc_runloop_stop() >>> runloop = %p\n", runloop);
    }
    pthread_mutex_lock(runloop->mutex);
    if (runloop->started) {
        runloop->started = false;
        pthread_cond_signal(&runloop->cond);
        pthread_mutex_unlock(runloop->mutex);
        pthread_join(runloop->thread_id, NULL);
#if DEBUG_ENABLED == 1
        // Ensure that on the moment of stop all callbacks are un-registered
        {
            int i;
            for (i = 0; i < BSC_RUNLOOP_CALLBACKS_NUM; i++) {
                if (runloop->runloop_ctx[i].ctx != NULL) {
                    debug_printf(
                        "bsc_runloop_stop() ctx %p is not un-registered\n",
                        runloop->runloop_ctx[i].ctx);
                }
            }
        }
#endif
        DEBUG_PRINTF("bsc_runloop_stop() <<<\n");
        return;
    }
    pthread_mutex_unlock(runloop->mutex);
    DEBUG_PRINTF("bsc_runloop_stop() <<<\n");
}