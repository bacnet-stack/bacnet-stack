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

#include <zephyr.h>
#include <string.h>
#include <stdlib.h>
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
    bool exit;
    struct k_mutex *mutex;
    struct k_condvar cond;
    k_tid_t thread_id;
    struct k_thread thread;
    k_thread_stack_t *stack;
};

#define RUNLOOP_STACKSIZE 128

K_KERNEL_STACK_DEFINE(runloop_global_stack, RUNLOOP_STACKSIZE);
static K_MUTEX_DEFINE(bsc_mutex_global);
static BSC_RUNLOOP bsc_runloop_global = { true, {{0}}, false, false, false,
    false, &bsc_mutex_global, {{{{0}}}}, NULL, {{{{{0}}}}},
    runloop_global_stack };

K_KERNEL_STACK_ARRAY_DEFINE(runloop_stack, BSC_RUNLOOP_LOCAL_NUM,
    RUNLOOP_STACKSIZE);
static struct k_mutex bsc_mutex_local[BSC_RUNLOOP_LOCAL_NUM];
static BSC_RUNLOOP bsc_runloop_local[BSC_RUNLOOP_LOCAL_NUM] = {0};

BSC_RUNLOOP *bsc_global_runloop(void)
{
    return &bsc_runloop_global;
}

BSC_RUNLOOP *bsc_local_runloop_alloc(void)
{
    int i;
    k_mutex_lock(bsc_global_runloop()->mutex, K_FOREVER);
    for (i = 0; i < BSC_RUNLOOP_LOCAL_NUM; i++) {
        if (!bsc_runloop_local[i].used) {
            k_mutex_init(&bsc_mutex_local[i]);
            bsc_runloop_local[i].mutex = &bsc_mutex_local[i];
            bsc_runloop_local[i].stack = runloop_stack[i];
            bsc_runloop_local[i].used = true;
            k_mutex_unlock(bsc_global_runloop()->mutex);
            return &bsc_runloop_local[i];
        }
    }
    k_mutex_unlock(bsc_global_runloop()->mutex);
    return NULL;
}

void bsc_local_runloop_free(BSC_RUNLOOP *runloop)
{
    k_mutex_lock(bsc_global_runloop()->mutex, K_FOREVER);
    runloop->used = false;
    k_mutex_unlock(bsc_global_runloop()->mutex);
}

static void bsc_runloop_worker(void *p1, void *p2, void *p3)
{
    BSC_RUNLOOP *rl = (BSC_RUNLOOP *)p1;
    BSC_RUNLOOP_CTX local[BSC_RUNLOOP_CALLBACKS_NUM];
    int i;

    DEBUG_PRINTF("bsc_runloop_worker() >>>\n");

    k_mutex_lock(rl->mutex, K_FOREVER);
    memcpy(local, rl->runloop_ctx, sizeof(local));
    k_mutex_unlock(rl->mutex);

    while (1) {
        // DEBUG_PRINTF("bsc_runloop_worker() waiting for next iteration\n");
        k_mutex_lock(rl->mutex, K_FOREVER);
        while (!rl->process) {
            if (k_condvar_wait(&rl->cond, rl->mutex,
                    Z_TIMEOUT_MS(BSC_DEFAULT_RUNLOOP_TIMEOUT_MS)) == -EAGAIN) {
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

        if (rl->exit) {
            rl->exit = false;
            DEBUG_PRINTF("bsc_runloop_worker() runloop is stopped\n");
            k_mutex_unlock(rl->mutex);
            break;
        } else {
            k_mutex_unlock(rl->mutex);
            for (i = 0; i < BSC_RUNLOOP_CALLBACKS_NUM; i++) {
                if (local[i].ctx) {
                    local[i].runloop_func(local[i].ctx);
                }
            }
        }
    }
    DEBUG_PRINTF("bsc_runloop_worker() <<<\n");
}

BSC_SC_RET bsc_runloop_start(BSC_RUNLOOP *runloop)
{
    if (runloop == bsc_global_runloop()) {
        DEBUG_PRINTF("bsc_runloop_start() >>> runloop global(%p)\n", runloop);
    } else {
        DEBUG_PRINTF("bsc_runloop_start() >>> runloop = %p\n", runloop);
    }

    k_mutex_lock(runloop->mutex, K_FOREVER);

    if (runloop->started) {
        k_mutex_unlock(runloop->mutex);
        DEBUG_PRINTF("bsc_runloop_start() <<< ret = BSC_SC_INVALID_OPERATION\n");
        return BSC_SC_INVALID_OPERATION;
    }

    if (k_condvar_init(&runloop->cond) != 0) {
        k_mutex_unlock(runloop->mutex);
        DEBUG_PRINTF("bsc_runloop_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    runloop->thread_id = k_thread_create(&runloop->thread, runloop->stack,
        RUNLOOP_STACKSIZE, &bsc_runloop_worker, runloop, NULL, NULL, -1,
        K_USER | K_INHERIT_PERMS, K_NO_WAIT);

    if (runloop->thread_id == NULL) {
        //memset(&runloop->cond, 0, sizeof(runloop->cond));
        k_mutex_unlock(runloop->mutex);
        DEBUG_PRINTF("bsc_runloop_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }
    k_thread_start(runloop->thread_id);

    memset(runloop->runloop_ctx, 0, sizeof(runloop->runloop_ctx));
    runloop->started = true;
    k_mutex_unlock(runloop->mutex);
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

    k_mutex_lock(runloop->mutex, K_FOREVER);

    if (runloop->started) {
        for (i = 0; i < BSC_RUNLOOP_CALLBACKS_NUM; i++) {
            if (!runloop->runloop_ctx[i].ctx) {
                runloop->runloop_ctx[i].ctx = ctx;
                runloop->runloop_ctx[i].runloop_func = runloop_func;
                runloop->changed = true;
                k_mutex_unlock(runloop->mutex);
                DEBUG_PRINTF(
                    "bsc_runloop_schedule() <<< ret = BSC_SC_SUCCESS\n");
                return BSC_SC_SUCCESS;
            }
        }
        k_mutex_unlock(runloop->mutex);
        DEBUG_PRINTF("bsc_runloop_schedule() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    k_mutex_unlock(runloop->mutex);
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

    k_mutex_lock(runloop->mutex, K_FOREVER);
    if (runloop->started) {
        runloop->process = 1;
        k_condvar_broadcast(&runloop->cond);
    }
    k_mutex_unlock(runloop->mutex);
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

    k_mutex_lock(runloop->mutex, K_FOREVER);
    for (i = 0; i < BSC_RUNLOOP_CALLBACKS_NUM; i++) {
        if (runloop->runloop_ctx[i].ctx == ctx) {
            runloop->runloop_ctx[i].ctx = NULL;
            runloop->runloop_ctx[i].runloop_func = NULL;
            runloop->changed = true;
            break;
        }
    }
    k_mutex_unlock(runloop->mutex);
    DEBUG_PRINTF("bsc_runloop_unreg() <<<\n");
}

void bsc_runloop_stop(BSC_RUNLOOP *runloop)
{
    if (runloop == bsc_global_runloop()) {
        DEBUG_PRINTF("bsc_runloop_stop() >>> runloop global (%p)\n", runloop);
    } else {
        DEBUG_PRINTF("bsc_runloop_stop() >>> runloop = %p\n", runloop);
    }

    k_mutex_lock(runloop->mutex, K_FOREVER);
    if (!runloop->started) {
        k_mutex_unlock(runloop->mutex);
    }
    else {
        runloop->exit = true;
        k_condvar_broadcast(&runloop->cond);
        k_mutex_unlock(runloop->mutex);
        k_thread_join(&runloop->thread, K_FOREVER);
        k_mutex_lock(runloop->mutex, K_FOREVER);
        runloop->started = false;
        //memset(&runloop->cond, 0, sizeof(runloop->cond));
        k_mutex_unlock(runloop->mutex);

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
    }
    DEBUG_PRINTF("bsc_runloop_stop() <<<\n");
}
