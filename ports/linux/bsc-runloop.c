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
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bsc-runloop.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bsc-mutex.h"

#define BSC_DEFAULT_RUNLOOP_TIMEOUT_MS (1 * 1000)

typedef struct {
    BSC_SOCKET_CTX *ctx;
    void (*runloop_func)(BSC_SOCKET_CTX *ctx);
} BSC_RUNLOOP_CTX;

static BSC_RUNLOOP_CTX bsc_runloop_ctx[BSC_MAX_CONTEXTS_NUM];
static bool bsc_runloop_started = false;
static bool bsc_runloop_process = false;
static bool bsc_runloop_ctx_changed = false;
static pthread_mutex_t bsc_runloop_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_cond_t bsc_cond;
static pthread_t bsc_thread_id;

static void *bsc_runloop_worker(void *arg)
{
    BSC_RUNLOOP_CTX local[BSC_MAX_CONTEXTS_NUM];
    int i;
    struct timespec to;

    debug_printf("bsc_runloop_worker() >>>\n");

    pthread_mutex_lock(&bsc_runloop_mutex);
    memcpy(local, bsc_runloop_ctx, sizeof(local));
    pthread_mutex_unlock(&bsc_runloop_mutex);

    while (1) {
        // debug_printf("bsc_runloop_worker() waiting for next iteration\n");
        pthread_mutex_lock(&bsc_runloop_mutex);
        clock_gettime(CLOCK_REALTIME, &to);
        to.tv_sec = to.tv_sec + BSC_DEFAULT_RUNLOOP_TIMEOUT_MS / 1000;
        to.tv_nsec =
            to.tv_nsec + (BSC_DEFAULT_RUNLOOP_TIMEOUT_MS % 1000) * 1000000;
        to.tv_sec += to.tv_nsec / 1000000000;
        to.tv_nsec %= 1000000000;

        while (!bsc_runloop_process) {
            if (pthread_cond_timedwait(&bsc_cond, &bsc_runloop_mutex, &to) ==
                ETIMEDOUT) {
                break;
            }
        }
        // debug_printf("bsc_runloop_worker() processing started\n");

        bsc_runloop_process = false;

        if (bsc_runloop_ctx_changed) {
            debug_printf("bsc_runloop_worker() processing context changes\n");
            bsc_runloop_ctx_changed = false;
            memcpy(local, bsc_runloop_ctx, sizeof(local));
        }

        if (!bsc_runloop_started) {
            debug_printf("bsc_runloop_worker() runloop is stopped\n");
            pthread_mutex_unlock(&bsc_runloop_mutex);
            break;
        } else {
            pthread_mutex_unlock(&bsc_runloop_mutex);
            for (i = 0; i < BSC_MAX_CONTEXTS_NUM; i++) {
                if (local[i].ctx) {
                    local[i].runloop_func(local[i].ctx);
                }
            }
        }
    }
    debug_printf("bsc_runloop_worker() <<<\n");
    return NULL;
}

BSC_SC_RET bsc_runloop_start(void)
{
    int ret;

    debug_printf("bsc_runloop_start() >>>\n");

    pthread_mutex_lock(&bsc_runloop_mutex);

    if (bsc_runloop_started) {
        pthread_mutex_unlock(&bsc_runloop_mutex);
        return BSC_SC_INVALID_OPERATION;
    }

    if (pthread_cond_init(&bsc_cond, NULL) != 0) {
        debug_printf("bsc_runloop_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    ret = pthread_create(&bsc_thread_id, NULL, &bsc_runloop_worker, NULL);

    if (ret != 0) {
        pthread_cond_destroy(&bsc_cond);
        debug_printf("bsc_runloop_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    memset(bsc_runloop_ctx, 0, sizeof(bsc_runloop_ctx));
    bsc_runloop_started = true;
    pthread_mutex_unlock(&bsc_runloop_mutex);
    debug_printf("bsc_runloop_start() >>> ret = BSC_SC_SUCCESS\n");
    return BSC_SC_SUCCESS;
}

BSC_SC_RET bsc_runloop_reg(
    BSC_SOCKET_CTX *ctx, void (*runloop_func)(BSC_SOCKET_CTX *ctx))
{
    int i;
    debug_printf(
        "bsc_runloop_reg() >>> ctx = %p, func = %p\n", ctx, runloop_func);

    pthread_mutex_lock(&bsc_runloop_mutex);

    if (bsc_runloop_started) {
        for (i = 0; i < BSC_MAX_CONTEXTS_NUM; i++) {
            if (!bsc_runloop_ctx[i].ctx) {
                bsc_runloop_ctx[i].ctx = ctx;
                bsc_runloop_ctx[i].runloop_func = runloop_func;
                bsc_runloop_ctx_changed = true;
                pthread_mutex_unlock(&bsc_runloop_mutex);
                debug_printf(
                    "bsc_runloop_schedule() <<< ret = BSC_SC_SUCCESS\n");
                return BSC_SC_SUCCESS;
            }
        }
        pthread_mutex_unlock(&bsc_runloop_mutex);
        debug_printf("bsc_runloop_schedule() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    pthread_mutex_unlock(&bsc_runloop_mutex);
    debug_printf("bsc_runloop_schedule() <<< ret = BSC_SC_INVALID_OPERATION\n");
    return BSC_SC_INVALID_OPERATION;
}

void bsc_runloop_schedule(void)
{
    debug_printf("bsc_runloop_schedule() >>>\n");
    pthread_mutex_lock(&bsc_runloop_mutex);
    if (bsc_runloop_started) {
        bsc_runloop_process = 1;
        pthread_cond_signal(&bsc_cond);
    }
    pthread_mutex_unlock(&bsc_runloop_mutex);
    debug_printf("bsc_runloop_schedule() <<<\n");
}

void bsc_runloop_unreg(BSC_SOCKET_CTX *ctx)
{
    int i;
    debug_printf("bsc_runloop_stop() >>>\n");
    pthread_mutex_lock(&bsc_runloop_mutex);
    for (i = 0; i < BSC_MAX_CONTEXTS_NUM; i++) {
        if (bsc_runloop_ctx[i].ctx == ctx) {
            bsc_runloop_ctx[i].ctx = NULL;
            bsc_runloop_ctx[i].runloop_func = NULL;
            bsc_runloop_ctx_changed = true;
            break;
        }
    }
    pthread_mutex_unlock(&bsc_runloop_mutex);
    debug_printf("bsc_runloop_stop() <<<\n");
}

void bsc_runloop_stop(void)
{
    debug_printf("bsc_runloop_stop() >>>\n");
    pthread_mutex_lock(&bsc_runloop_mutex);
    if (bsc_runloop_started) {
        bsc_runloop_started = false;
        pthread_cond_signal(&bsc_cond);
        pthread_mutex_unlock(&bsc_runloop_mutex);
        pthread_join(bsc_thread_id, NULL);
        debug_printf("bsc_runloop_stop() <<<\n");
        return;
    }
    pthread_mutex_unlock(&bsc_runloop_mutex);
    debug_printf("bsc_runloop_stop() <<<\n");
}