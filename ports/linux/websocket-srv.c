/**
 * @file
 * @brief Implementation of server websocket interface.
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date June 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#define _GNU_SOURCE
#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/basic/sys/debug.h"
#include "websocket-global.h"
#include <arpa/inet.h>

#undef DEBUG_PRINTF
#if DEBUG_WEBSOCKET_SERVER
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF debug_printf_disabled
#endif

#define BSC_RX_BUFFER_LEN BSC_WEBSOCKET_RX_BUFFER_LEN

#ifndef LWS_PROTOCOL_LIST_TERM
#define LWS_PROTOCOL_LIST_TERM       \
    {                                \
        NULL, NULL, 0, 0, 0, NULL, 0 \
    }
#endif

typedef enum {
    BSC_WEBSOCKET_STATE_IDLE = 0,
    BSC_WEBSOCKET_STATE_CONNECTED = 1,
    BSC_WEBSOCKET_STATE_DISCONNECTING = 2
} BSC_WEBSOCKET_STATE;

/* Some forward function declarations */

static int bws_srv_websocket_event(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len);

typedef struct {
    struct lws_context *ctx;
    struct lws *ws;
    BSC_WEBSOCKET_STATE state;
    bool want_send_data;
    bool can_send_data;
    uint8_t *fragment_buffer;
    size_t fragment_buffer_size;
    size_t fragment_buffer_len;
    BACNET_ERROR_CODE err_code;
} BSC_WEBSOCKET_CONNECTION;

#if BSC_CONF_WEBSOCKET_SERVERS_NUM < 1
#error "BSC_CONF_WEBSOCKET_SERVERS_NUM must be >= 1"
#endif

static pthread_mutex_t bws_global_mutex =
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t bws_srv_direct_mutex[BSC_CONF_WEBSOCKET_SERVERS_NUM];
static pthread_mutex_t bws_srv_hub_mutex[BSC_CONF_WEBSOCKET_SERVERS_NUM];

#if BSC_SERVER_HUB_WEBSOCKETS_MAX_NUM > 0
static BSC_WEBSOCKET_CONNECTION
    bws_hub_conn[BSC_CONF_WEBSOCKET_SERVERS_NUM]
                [BSC_SERVER_HUB_WEBSOCKETS_MAX_NUM] = { 0 };
#else
static BSC_WEBSOCKET_CONNECTION bws_hub_conn[1][1] = { 0 };
#endif

#if BSC_SERVER_DIRECT_WEBSOCKETS_MAX_NUM > 0
static BSC_WEBSOCKET_CONNECTION
    bws_direct_conn[BSC_CONF_WEBSOCKET_SERVERS_NUM]
                   [BSC_SERVER_DIRECT_WEBSOCKETS_MAX_NUM] = { 0 };
#else
static BSC_WEBSOCKET_CONNECTION bws_direct_conn[1][1] = { 0 };
#endif

typedef struct BACNetWebsocketServerContext {
    bool used;
    struct lws_context *wsctx;
    BSC_WEBSOCKET_PROTOCOL proto;
    BSC_WEBSOCKET_CONNECTION *conn;
    pthread_mutex_t *mutex;
    BSC_WEBSOCKET_SRV_DISPATCH dispatch_func;
    void *user_param;
    bool stop_worker;
} BSC_WEBSOCKET_CONTEXT;

static BSC_WEBSOCKET_CONTEXT bws_hub_ctx[BSC_CONF_WEBSOCKET_SERVERS_NUM] = {
    0
};
static BSC_WEBSOCKET_CONTEXT bws_direct_ctx[BSC_CONF_WEBSOCKET_SERVERS_NUM] = {
    0
};

static bool bws_mutex_init(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attr;

    if (pthread_mutexattr_init(&attr) != 0) {
        return false;
    }

    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
        pthread_mutexattr_destroy(&attr);
        return false;
    }

    if (pthread_mutex_init(mutex, &attr) != 0) {
        pthread_mutexattr_destroy(&attr);
        return false;
    }

    pthread_mutexattr_destroy(&attr);
    return true;
}

static BSC_WEBSOCKET_CONTEXT *bws_alloc_server_ctx(BSC_WEBSOCKET_PROTOCOL proto)
{
    int i;
    BSC_WEBSOCKET_CONTEXT *ctx = (proto == BSC_WEBSOCKET_HUB_PROTOCOL)
        ? &bws_hub_ctx[0]
        : &bws_direct_ctx[0];

    pthread_mutex_lock(&bws_global_mutex);
    DEBUG_PRINTF("bws_alloc_server_ctx() >>> proto = %d\n", proto);

    for (i = 0; i < BSC_CONF_WEBSOCKET_SERVERS_NUM; i++) {
        if (!ctx[i].used) {
            if (proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
                ctx[i].mutex = &bws_srv_hub_mutex[i];
                ctx[i].conn = &bws_hub_conn[i][0];
            } else {
                ctx[i].mutex = &bws_srv_direct_mutex[i];
                ctx[i].conn = &bws_direct_conn[i][0];
            }
            if (!bws_mutex_init(ctx[i].mutex)) {
                DEBUG_PRINTF("bws_alloc_server_ctx() <<< ret = %p\n", &ctx[i]);
                pthread_mutex_unlock(&bws_global_mutex);
                return NULL;
            }
            ctx[i].used = true;
            DEBUG_PRINTF("bws_alloc_server_ctx() <<< ret = %p\n", &ctx[i]);
            pthread_mutex_unlock(&bws_global_mutex);
            return &ctx[i];
        }
    }
    DEBUG_PRINTF("bws_alloc_server_ctx() <<< ret = %p\n", &ctx[i]);
    pthread_mutex_unlock(&bws_global_mutex);
    return NULL;
}

static void bws_set_disconnect_reason(
    BSC_WEBSOCKET_CONTEXT *ctx, BSC_WEBSOCKET_HANDLE h, uint16_t err_code)
{
    switch (err_code) {
        case LWS_CLOSE_STATUS_NORMAL: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER;
            break;
        }
        case LWS_CLOSE_STATUS_GOINGAWAY: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_ENDPOINT_LEAVES;
            break;
        }
        case LWS_CLOSE_STATUS_PROTOCOL_ERR: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_PROTOCOL_ERROR;
            break;
        }
        case LWS_CLOSE_STATUS_UNACCEPTABLE_OPCODE: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_DATA_NOT_ACCEPTED;
            break;
        }
        case LWS_CLOSE_STATUS_NO_STATUS:
        case LWS_CLOSE_STATUS_RESERVED: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_ERROR;
            break;
        }
        case LWS_CLOSE_STATUS_ABNORMAL_CLOSE: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_DATA_NOT_ACCEPTED;
            break;
        }
        case LWS_CLOSE_STATUS_INVALID_PAYLOAD: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_DATA_INCONSISTENT;
            break;
        }
        case LWS_CLOSE_STATUS_POLICY_VIOLATION: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_DATA_AGAINST_POLICY;
            break;
        }
        case LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_FRAME_TOO_LONG;
            break;
        }
        case LWS_CLOSE_STATUS_EXTENSION_REQUIRED: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_EXTENSION_MISSING;
            break;
        }
        case LWS_CLOSE_STATUS_UNEXPECTED_CONDITION: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_REQUEST_UNAVAILABLE;
            break;
        }
        default: {
            ctx->conn[h].err_code = ERROR_CODE_WEBSOCKET_ERROR;
            break;
        }
    }
}

static void bws_free_server_ctx(BSC_WEBSOCKET_CONTEXT *ctx)
{
    pthread_mutex_lock(&bws_global_mutex);
    DEBUG_PRINTF("bws_free_server_ctx() >>> ctx = %p\n", ctx);
    ctx->used = false;
    ctx->wsctx = NULL;
    ctx->conn = NULL;
    pthread_mutex_destroy(ctx->mutex);
    ctx->mutex = NULL;
    ctx->dispatch_func = NULL;
    ctx->user_param = NULL;
    DEBUG_PRINTF("bws_free_server_ctx() <<< \n");
    pthread_mutex_unlock(&bws_global_mutex);
}

#if DEBUG_ENABLED == 1
static bool bws_validate_ctx_pointer(BSC_WEBSOCKET_CONTEXT *ctx)
{
    bool is_validated = false;
    int i;

    pthread_mutex_lock(&bws_global_mutex);

    for (i = 0; i < BSC_CONF_WEBSOCKET_SERVERS_NUM; i++) {
        if (ctx == &bws_hub_ctx[i]) {
            is_validated = true;
            break;
        }
    }

    if (!is_validated) {
        for (i = 0; i < BSC_CONF_WEBSOCKET_SERVERS_NUM; i++) {
            if (ctx == &bws_direct_ctx[i]) {
                is_validated = true;
                break;
            }
        }
    }

    pthread_mutex_unlock(&bws_global_mutex);
    return is_validated;
}
#endif

static int bws_srv_get_max_sockets(BSC_WEBSOCKET_PROTOCOL proto)
{
    int max = 0;
    if (proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
        max = BSC_SERVER_HUB_WEBSOCKETS_MAX_NUM;
    } else if (proto == BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        max = BSC_SERVER_DIRECT_WEBSOCKETS_MAX_NUM;
    }
    return max;
}

static BSC_WEBSOCKET_HANDLE bws_srv_alloc_connection(BSC_WEBSOCKET_CONTEXT *ctx)
{
    int i;

    DEBUG_PRINTF("bws_srv_alloc_connection() >>> ctx = %p\n", ctx);

    for (i = 0; i < bws_srv_get_max_sockets(ctx->proto); i++) {
        if (ctx->conn[i].state == BSC_WEBSOCKET_STATE_IDLE) {
            memset(&ctx->conn[i], 0, sizeof(ctx->conn[i]));
            DEBUG_PRINTF("bws_srv_alloc_connection() <<< ret = %d\n", i);
            return i;
        }
    }

    DEBUG_PRINTF("bws_srv_alloc_connection() <<< ret = "
                 "BSC_WEBSOCKET_INVALID_HANDLE\n");
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static void
bws_srv_free_connection(BSC_WEBSOCKET_CONTEXT *ctx, BSC_WEBSOCKET_HANDLE h)
{
    DEBUG_PRINTF("bws_srv_free_connection() >>> ctx = %p, h = %d\n", ctx, h);

    if (h >= 0 && h < bws_srv_get_max_sockets(ctx->proto)) {
        if (ctx->conn[h].state != BSC_WEBSOCKET_STATE_IDLE) {
            if (ctx->conn[h].fragment_buffer) {
                free(ctx->conn[h].fragment_buffer);
                ctx->conn[h].fragment_buffer = NULL;
                ctx->conn[h].fragment_buffer_len = 0;
                ctx->conn[h].fragment_buffer_size = 0;
            }
            ctx->conn[h].state = BSC_WEBSOCKET_STATE_IDLE;
            ctx->conn[h].ws = NULL;
        }
    }
    DEBUG_PRINTF("bws_srv_free_connection() <<<\n");
}

static BSC_WEBSOCKET_HANDLE
bws_find_connnection(BSC_WEBSOCKET_CONTEXT *ctx, struct lws *ws)
{
    int i;
    for (i = 0; i < bws_srv_get_max_sockets(ctx->proto); i++) {
        if (ctx->conn[i].ws == ws &&
            ctx->conn[i].state != BSC_WEBSOCKET_STATE_IDLE) {
            return i;
        }
    }
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static int bws_srv_websocket_event(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BSC_WEBSOCKET_HANDLE h;
    BSC_WEBSOCKET_CONTEXT *ctx =
        (BSC_WEBSOCKET_CONTEXT *)lws_context_user(lws_get_context(wsi));
    int ret = 0;
    BSC_WEBSOCKET_SRV_DISPATCH dispatch_func;
    void *user_param;
    bool stop_worker;
    uint8_t err_code[2];
    uint16_t err;
    (void)user;

    DEBUG_PRINTF(
        "bws_srv_websocket_event() >>> ctx = %p, user_param = %p, "
        "proto = %d, wsi = %p, "
        "reason = %d, in = %p, len = %d\n",
        ctx, ctx->user_param, ctx->proto, wsi, reason, in, len);

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            pthread_mutex_lock(ctx->mutex);
            DEBUG_PRINTF("bws_srv_websocket_event() established connection\n");
            h = bws_srv_alloc_connection(ctx);
            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                DEBUG_PRINTF("bws_srv_websocket_event() no free sockets, "
                             "dropping incoming connection\n");
                pthread_mutex_unlock(ctx->mutex);
                return -1;
            }
            DEBUG_PRINTF(
                "bws_srv_websocket_event() ctx %p proto %d set state of"
                " socket %d to BACNET_WEBSOCKET_STATE_CONNECTING\n",
                ctx, ctx->proto, h);
            ctx->conn[h].ws = wsi;
            ctx->conn[h].state = BSC_WEBSOCKET_STATE_CONNECTED;
            ctx->conn[h].err_code = ERROR_CODE_SUCCESS;
            dispatch_func = ctx->dispatch_func;
            user_param = ctx->user_param;
            pthread_mutex_unlock(ctx->mutex);
            dispatch_func(
                (BSC_WEBSOCKET_SRV_HANDLE)ctx, h, BSC_WEBSOCKET_CONNECTED, 0,
                NULL, NULL, 0, user_param);
            /* wakeup worker to process pending event */
            lws_cancel_service(ctx->wsctx);
            break;
        }
        case LWS_CALLBACK_CLOSED: {
            DEBUG_PRINTF("bws_srv_websocket_event() closed connection\n");
            pthread_mutex_lock(ctx->mutex);
            h = bws_find_connnection(ctx, wsi);
            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                pthread_mutex_unlock(ctx->mutex);
            } else {
                DEBUG_PRINTF(
                    "bws_srv_websocket_event() ctx %p user_param = %p "
                    "proto %d state of "
                    "socket %d is %d\n",
                    ctx, ctx->user_param, ctx->proto, h, ctx->conn[h].state);
                dispatch_func = ctx->dispatch_func;
                user_param = ctx->user_param;
                stop_worker = ctx->stop_worker;
                err = ctx->conn[h].err_code;
                bws_srv_free_connection(ctx, h);
                pthread_mutex_unlock(ctx->mutex);
                if (!stop_worker) {
                    dispatch_func(
                        (BSC_WEBSOCKET_SRV_HANDLE)ctx, h,
                        BSC_WEBSOCKET_DISCONNECTED, err, NULL, NULL, 0,
                        user_param);
                }
            }
            break;
        }
        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: {
            pthread_mutex_lock(ctx->mutex);
            h = bws_find_connnection(ctx, wsi);
            if (h != BSC_WEBSOCKET_INVALID_HANDLE && len >= 2) {
                err_code[0] = ((uint8_t *)in)[1];
                err_code[1] = ((uint8_t *)in)[0];
                bws_set_disconnect_reason(ctx, h, *((uint16_t *)&err_code));
            }
            pthread_mutex_unlock(ctx->mutex);
            break;
        }
        case LWS_CALLBACK_RECEIVE: {
            pthread_mutex_lock(ctx->mutex);
            h = bws_find_connnection(ctx, wsi);
            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                pthread_mutex_unlock(ctx->mutex);
            } else {
                DEBUG_PRINTF(
                    "bws_srv_websocket_event() ctx %p proto %d "
                    "received %d bytes of "
                    "data for websocket %d\n",
                    ctx, ctx->proto, len, h);
                if (!lws_frame_is_binary(wsi)) {
                    /* According AB.7.5.3 BACnet/SC BVLC Message Exchange,
                       if a received data frame is not binary,
                       the WebSocket connection shall be closed with a
                       status code of 1003 -WEBSOCKET_DATA_NOT_ACCEPTED.
                    */
                    DEBUG_PRINTF(
                        "bws_srv_websocket_event() ctx %p proto %d got "
                        "non-binary frame, "
                        "close websocket %d\n",
                        ctx, ctx->proto, h);
                    lws_close_reason(
                        wsi, LWS_CLOSE_STATUS_UNACCEPTABLE_OPCODE, NULL, 0);
                    pthread_mutex_unlock(ctx->mutex);
                    DEBUG_PRINTF("bws_srv_websocket_event() <<< ret = -1\n");
                    return -1;
                }
                if (ctx->conn[h].state != BSC_WEBSOCKET_STATE_CONNECTED) {
                    pthread_mutex_unlock(ctx->mutex);
                } else {
                    if (!ctx->conn[h].fragment_buffer) {
                        ctx->conn[h].fragment_buffer =
                            malloc(BSC_RX_BUFFER_LEN);
                        if (!ctx->conn[h].fragment_buffer) {
                            lws_close_reason(
                                wsi, LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE, NULL,
                                0);
                            pthread_mutex_unlock(ctx->mutex);
                            DEBUG_PRINTF(
                                "bws_srv_websocket_event() <<< ret = "
                                "-1, allocation of %d bytes failed\n",
                                len <= BSC_RX_BUFFER_LEN ? BSC_RX_BUFFER_LEN
                                                         : len);
                            return -1;
                        }
                        ctx->conn[h].fragment_buffer_len = 0;
                        ctx->conn[h].fragment_buffer_size = BSC_RX_BUFFER_LEN;
                    }
                    if (ctx->conn[h].fragment_buffer_len + len >
                        ctx->conn[h].fragment_buffer_size) {
                        DEBUG_PRINTF(
                            "bws_srv_websocket_event() realloc buf of %d bytes"
                            "for socket %d to %d bytes\n",
                            ctx->conn[h].fragment_buffer_len, h,
                            ctx->conn[h].fragment_buffer_len + len);

                        ctx->conn[h].fragment_buffer = realloc(
                            ctx->conn[h].fragment_buffer,
                            ctx->conn[h].fragment_buffer_len + len);
                        if (!ctx->conn[h].fragment_buffer) {
                            lws_close_reason(
                                wsi, LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE, NULL,
                                0);
                            pthread_mutex_unlock(ctx->mutex);
                            DEBUG_PRINTF(
                                "bws_srv_websocket_event() <<< ret = -1, "
                                "re-allocation of %d bytes failed\n",
                                ctx->conn[h].fragment_buffer_len + len);
                            return -1;
                        }
                        ctx->conn[h].fragment_buffer_size =
                            ctx->conn[h].fragment_buffer_len + len;
                    }
                    DEBUG_PRINTF(
                        "bws_srv_websocket_event() got next %d bytes for "
                        "socket %d total_len %d\n",
                        len, h, ctx->conn[h].fragment_buffer_len);
                    memcpy(
                        &ctx->conn[h]
                             .fragment_buffer[ctx->conn[h].fragment_buffer_len],
                        in, len);
                    ctx->conn[h].fragment_buffer_len += len;
                    if (lws_is_final_fragment(wsi) && !ctx->stop_worker) {
                        dispatch_func = ctx->dispatch_func;
                        user_param = ctx->user_param;
                        pthread_mutex_unlock(ctx->mutex);
                        dispatch_func(
                            (BSC_WEBSOCKET_SRV_HANDLE)ctx, h,
                            BSC_WEBSOCKET_RECEIVED, 0, NULL,
                            ctx->conn[h].fragment_buffer,
                            ctx->conn[h].fragment_buffer_len, user_param);
                        pthread_mutex_lock(ctx->mutex);
                        ctx->conn[h].fragment_buffer_len = 0;
                        pthread_mutex_unlock(ctx->mutex);
                    } else {
                        pthread_mutex_unlock(ctx->mutex);
                    }
                }
            }
            break;
        }
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            pthread_mutex_lock(ctx->mutex);
            DEBUG_PRINTF(
                "bws_srv_websocket_event() ctx %p proto %d can write\n", ctx,
                ctx->proto);
            h = bws_find_connnection(ctx, wsi);
            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                pthread_mutex_unlock(ctx->mutex);
            } else {
                DEBUG_PRINTF(
                    "bws_srv_websocket_event() ctx %p proto %d socket "
                    "%d state = %d\n",
                    ctx, ctx->proto, h, ctx->conn[h].state);

                if (ctx->conn[h].state == BSC_WEBSOCKET_STATE_DISCONNECTING) {
                    /* if -1 returned from this callback libwebsocket closes
+                      websocket related to wsi */
                    ret = -1;
                }

                if ((ctx->conn[h].state != BSC_WEBSOCKET_STATE_IDLE) &&
                    !ctx->stop_worker && ctx->conn[h].want_send_data) {
                    ctx->conn[h].can_send_data = true;
                    dispatch_func = ctx->dispatch_func;
                    user_param = ctx->user_param;
                    stop_worker = ctx->stop_worker;
                    pthread_mutex_unlock(ctx->mutex);
                    if (!stop_worker) {
                        dispatch_func(
                            (BSC_WEBSOCKET_SRV_HANDLE)ctx, h,
                            BSC_WEBSOCKET_SENDABLE, 0, NULL, NULL, 0,
                            user_param);
                    }
                    pthread_mutex_lock(ctx->mutex);
                    ctx->conn[h].want_send_data = false;
                    ctx->conn[h].can_send_data = false;
                    pthread_mutex_unlock(ctx->mutex);
                    /* wakeup worker to process internal state */
                    lws_cancel_service(ctx->wsctx);
                } else {
                    ctx->conn[h].want_send_data = false;
                    pthread_mutex_unlock(ctx->mutex);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
    DEBUG_PRINTF("bws_srv_websocket_event() <<< ret = %d\n", ret);
    return ret;
}

static void *bws_srv_worker(void *arg)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)arg;
    int i;
    BSC_WEBSOCKET_SRV_DISPATCH dispatch_func;
    void *user_param;

    DEBUG_PRINTF(
        "bws_srv_worker() started for ctx %p proto %d user_param %p\n", ctx,
        ctx->proto, ctx->user_param);

    pthread_mutex_lock(ctx->mutex);
    dispatch_func = ctx->dispatch_func;
    user_param = ctx->user_param;
    pthread_mutex_unlock(ctx->mutex);

    dispatch_func(
        (BSC_WEBSOCKET_SRV_HANDLE)ctx, 0, BSC_WEBSOCKET_SERVER_STARTED, 0, NULL,
        NULL, 0, user_param);

    while (1) {
        DEBUG_PRINTF(
            "bws_srv_worker() ctx %p proto %d blocked user_param %p\n", ctx,
            ctx->proto, ctx->user_param);
        pthread_mutex_lock(ctx->mutex);

        if (ctx->stop_worker) {
            DEBUG_PRINTF(
                "bws_srv_worker() ctx %p user_param %p proto %d going "
                "to stop\n",
                ctx, ctx->user_param, ctx->proto);
            DEBUG_PRINTF(
                "bws_srv_worker() destroy wsctx %p, ctx = %p, "
                "user_param = %p\n",
                ctx->wsctx, ctx, ctx->user_param);
            /* TRICKY: Libwebsockets API is not designed to be used from
                       multipe service threads, as a result
               lws_context_destroy() is not thread safe.More over, on different
               platforms the function behaves in different ways. Call of
                       lws_context_destroy() leads to several calls of
                       bws_srv_websocket_event() callback (LWS_CALLBACK_CLOSED,
                       etc..). But under some OS (MacOSx) that callback is
                       called from context of the bws_cli_worker() thread and
                       under some other OS (linux) the callback is called from
                       internal libwebsockets lib thread. That's why
                       bws_cli_mutex must be unlocked before
                       lws_context_destroy() call. To ensure that nobody calls
                       lws_context_destroy() from some parallel thread it is
                       protected by global websocket mutex.
            */
            pthread_mutex_unlock(ctx->mutex);
            bsc_websocket_global_lock();
            lws_context_destroy(ctx->wsctx);
            bsc_websocket_global_unlock();
            pthread_mutex_lock(ctx->mutex);
            ctx->wsctx = NULL;
            DEBUG_PRINTF("bws_srv_worker() set wsctx %p\n", ctx->wsctx);
            ctx->stop_worker = false;
            DEBUG_PRINTF(
                "bws_srv_worker() ctx %p proto %d emitting stop event\n", ctx,
                ctx->proto);
            dispatch_func = ctx->dispatch_func;
            user_param = ctx->user_param;
            DEBUG_PRINTF(
                "bws_srv_worker() ctx %p user_param = %p\n", ctx, user_param);
            pthread_mutex_unlock(ctx->mutex);
            dispatch_func(
                ctx, 0, BSC_WEBSOCKET_SERVER_STOPPED, 0, NULL, NULL, 0,
                user_param);
            bws_free_server_ctx(ctx);
            DEBUG_PRINTF(
                "bws_srv_worker() ctx %p proto %d stopped\n", ctx, ctx->proto);
            return NULL;
        }

        for (i = 0; i < bws_srv_get_max_sockets(ctx->proto); i++) {
            DEBUG_PRINTF(
                "bws_srv_worker() ctx %p user_param %p proto %d "
                "socket %d(%p) state = %d\n",
                ctx, ctx->user_param, ctx->proto, i, &ctx->conn[i],
                ctx->conn[i].state);
            if (ctx->conn[i].state == BSC_WEBSOCKET_STATE_CONNECTED) {
                if (ctx->conn[i].want_send_data) {
                    DEBUG_PRINTF(
                        "bws_srv_worker() process request for sending "
                        "data on socket %d\n",
                        i);
                    lws_callback_on_writable(ctx->conn[i].ws);
                }
            } else if (
                ctx->conn[i].state == BSC_WEBSOCKET_STATE_DISCONNECTING) {
                DEBUG_PRINTF(
                    "bws_srv_worker() process disconnecting event on "
                    "socket %d\n",
                    i);
                lws_callback_on_writable(ctx->conn[i].ws);
            }
        }

        DEBUG_PRINTF(
            "bws_srv_worker() ctx %p proto %d unblocked\n", ctx, ctx->proto);
        pthread_mutex_unlock(ctx->mutex);

        DEBUG_PRINTF(
            "bws_srv_worker() ctx %p user_param %p proto %d going to block on "
            "lws_service() call\n",
            ctx, ctx->user_param, ctx->proto);
        lws_service(ctx->wsctx, 0);
    }

    return NULL;
}

BSC_WEBSOCKET_RET bws_srv_start(
    BSC_WEBSOCKET_PROTOCOL proto,
    int port,
    char *iface,
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size,
    size_t timeout_s,
    BSC_WEBSOCKET_SRV_DISPATCH dispatch_func,
    void *dispatch_func_user_param,
    BSC_WEBSOCKET_SRV_HANDLE *sh)
{
    pthread_t thread_id;
    struct lws_context_creation_info info = { 0 };
    BSC_WEBSOCKET_CONTEXT *ctx;
    pthread_attr_t attr;
    int r;
    struct lws_protocols protos[] = { { NULL, bws_srv_websocket_event, 0, 0, 0,
                                        NULL, 0 },
                                      LWS_PROTOCOL_LIST_TERM };
    protos[0].name = (proto == BSC_WEBSOCKET_HUB_PROTOCOL)
        ? BSC_WEBSOCKET_HUB_PROTOCOL_STR
        : BSC_WEBSOCKET_DIRECT_PROTOCOL_STR;

    DEBUG_PRINTF(
        "bws_srv_start() >>> proto = %d port = %d "
        "dispatch_func_user_param = %p\n",
        proto, port, dispatch_func_user_param);

    if (proto != BSC_WEBSOCKET_HUB_PROTOCOL &&
        proto != BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        DEBUG_PRINTF("bws_srv_start() <<< bad protocol, ret = "
                     "BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size ||
        !dispatch_func || !timeout_s || !sh) {
        DEBUG_PRINTF("bws_srv_start() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (port < 0 || port > 65535) {
        DEBUG_PRINTF("bws_srv_start() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    ctx = bws_alloc_server_ctx(proto);

    if (!ctx) {
        DEBUG_PRINTF(
            "bws_srv_start() <<< maximum amount of servers for "
            "server proto %d is to small, ret = BSC_WEBSOCKET_NO_RESOURCES\n",
            proto);
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    bsc_websocket_init_log();

    pthread_mutex_lock(ctx->mutex);
    info.port = port;
    info.iface = iface;
    info.protocols = protos;
    info.gid = -1;
    info.uid = -1;
    info.server_ssl_cert_mem = cert;
    info.server_ssl_cert_mem_len = cert_size;
    info.server_ssl_ca_mem = ca_cert;
    info.server_ssl_ca_mem_len = ca_cert_size;
    info.server_ssl_private_key_mem = key;
    info.server_ssl_private_key_mem_len = key_size;
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.options |= LWS_SERVER_OPTION_FAIL_UPON_UNABLE_TO_BIND;
    info.timeout_secs = timeout_s;
    info.connect_timeout_secs = timeout_s;
    info.user = ctx;

    /* TRICKY: check comments related to lws_context_destroy() call */

    pthread_mutex_unlock(ctx->mutex);
    bsc_websocket_global_lock();
    ctx->wsctx = lws_create_context(&info);
    bsc_websocket_global_unlock();
    pthread_mutex_lock(ctx->mutex);

    if (!ctx->wsctx) {
        pthread_mutex_unlock(ctx->mutex);
        bws_free_server_ctx(ctx);
        DEBUG_PRINTF("bws_srv_start() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    ctx->dispatch_func = dispatch_func;
    ctx->user_param = dispatch_func_user_param;
    ctx->proto = proto;
    r = pthread_attr_init(&attr);

    if (!r) {
        r = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    if (!r) {
        r = pthread_create(&thread_id, &attr, &bws_srv_worker, ctx);
    }

    if (r) {
        /* TRICKY: Libwebsockets API is not designed to be used from
                   multipe service threads, as a result lws_context_destroy()
                   is not thread safe. More over, on different platforms the
                   function behaves in different ways. Call of
                   lws_context_destroy() leads to several calls of
                   bws_srv_websocket_event() callback (LWS_CALLBACK_CLOSED,
                   etc..). But under some OS (MacOSx) that callback is
                   called from context of the bws_cli_worker() thread and
                   under some other OS (linux) the callback is called from
                   internal libwebsockets lib thread. That's why
                   bws_cli_mutex must be unlocked before
                   lws_context_destroy() call. To ensure that nobody calls
                   lws_context_destroy() from some parallel thread it is
                   protected by global websocket mutex.
        */
        pthread_mutex_unlock(ctx->mutex);
        bsc_websocket_global_lock();
        lws_context_destroy(ctx->wsctx);
        bsc_websocket_global_unlock();
        pthread_mutex_lock(ctx->mutex);
        ctx->wsctx = NULL;
        pthread_mutex_unlock(ctx->mutex);
        bws_free_server_ctx(ctx);
        DEBUG_PRINTF(
            "bws_srv_start() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }
    pthread_attr_destroy(&attr);
    pthread_mutex_unlock(ctx->mutex);
    *sh = (BSC_WEBSOCKET_SRV_HANDLE)ctx;
    DEBUG_PRINTF("bws_srv_start() <<< ret = BSC_WEBSOCKET_SUCCESS\n");
    return BSC_WEBSOCKET_SUCCESS;
}

BSC_WEBSOCKET_RET bws_srv_stop(BSC_WEBSOCKET_SRV_HANDLE sh)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;

    DEBUG_PRINTF(
        "bws_srv_stop() >>> ctx = %p user_param = %p\n", ctx, ctx->user_param);

#if DEBUG_ENABLED == 1
    if (!bws_validate_ctx_pointer(ctx)) {
        DEBUG_PRINTF("bws_srv_stop() <<< bad websocket handle, ret = "
                     "BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }
#endif

    pthread_mutex_lock(ctx->mutex);

    if (ctx->stop_worker) {
        pthread_mutex_unlock(ctx->mutex);
        DEBUG_PRINTF(
            "bws_srv_stop() <<< ret = BSC_WEBSOCKET_INVALID_OPERATION\n");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    ctx->stop_worker = true;
    /* wake up libwebsockets runloop */
    lws_cancel_service(ctx->wsctx);
    pthread_mutex_unlock(ctx->mutex);

    DEBUG_PRINTF("bws_srv_stop() <<< ret = BSC_WEBSOCKET_SUCCESS\n");
    return BSC_WEBSOCKET_SUCCESS;
}

void bws_srv_disconnect(BSC_WEBSOCKET_SRV_HANDLE sh, BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;
    DEBUG_PRINTF("bws_srv_disconnect() >>> sh = %p h = %d\n", sh, h);

#if DEBUG_ENABLED == 1
    if (!bws_validate_ctx_pointer(ctx)) {
        DEBUG_PRINTF("bws_srv_disconnect() <<< bad websocket handle\n");
        return;
    }
#endif

    pthread_mutex_lock(ctx->mutex);
    if (h >= 0 && h < bws_srv_get_max_sockets(ctx->proto) &&
        !ctx->stop_worker) {
        if (ctx->conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
            /* tell worker to process change of connection state */
            ctx->conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
            lws_cancel_service(ctx->wsctx);
        }
    }
    pthread_mutex_unlock(ctx->mutex);
    DEBUG_PRINTF("bws_srv_disconnect() <<<\n");
}

void bws_srv_send(BSC_WEBSOCKET_SRV_HANDLE sh, BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;

    DEBUG_PRINTF("bws_srv_send() >>> ctx = %p h = %d\n", ctx, h);

#if DEBUG_ENABLED == 1
    if (!bws_validate_ctx_pointer(ctx)) {
        DEBUG_PRINTF("bws_srv_send() <<< bad websocket handle\n");
        return;
    }
#endif

    pthread_mutex_lock(ctx->mutex);
    if (ctx->conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
        /* tell worker to process send request */
        ctx->conn[h].want_send_data = true;
        lws_cancel_service(ctx->wsctx);
    }
    pthread_mutex_unlock(ctx->mutex);
    DEBUG_PRINTF("bws_srv_send() <<<\n");
}

BSC_WEBSOCKET_RET bws_srv_dispatch_send(
    BSC_WEBSOCKET_SRV_HANDLE sh,
    BSC_WEBSOCKET_HANDLE h,
    uint8_t *payload,
    size_t payload_size)
{
    int written;
    BSC_WEBSOCKET_RET ret;
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;

    DEBUG_PRINTF(
        "bws_srv_dispatch_send() >>> ctx = %p h = %d payload %p "
        "payload_size %d\n",
        ctx, h, payload, payload_size);

#if DEBUG_ENABLED == 1
    if (!bws_validate_ctx_pointer(ctx)) {
        DEBUG_PRINTF("bws_srv_dispatch_send() <<< bad websocket handle, ret = "
                     "BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }
#endif

    if (h < 0 || h >= bws_srv_get_max_sockets(ctx->proto)) {
        DEBUG_PRINTF("bws_srv_dispatch_send() <<< BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (!payload || !payload_size) {
        DEBUG_PRINTF("bws_srv_dispatch_send() <<< BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(ctx->mutex);

    if (ctx->stop_worker) {
        pthread_mutex_unlock(ctx->mutex);
        DEBUG_PRINTF("bws_srv_dispatch_send() <<< ret = "
                     "BSC_WEBSOCKET_INVALID_OPERATION\n");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    if ((ctx->conn[h].state != BSC_WEBSOCKET_STATE_CONNECTED) ||
        !ctx->conn[h].want_send_data || !ctx->conn[h].can_send_data) {
        DEBUG_PRINTF(
            "bws_srv_dispatch_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        pthread_mutex_unlock(ctx->mutex);
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    written =
        lws_write(ctx->conn[h].ws, payload, payload_size, LWS_WRITE_BINARY);

    DEBUG_PRINTF("bws_srv_dispatch_send() %d bytes is sent\n", written);

    if (written < (int)payload_size) {
        DEBUG_PRINTF(
            "bws_srv_dispatch_send() websocket connection is broken(closed)\n");
        /* tell worker to process change of connection state */
        ctx->conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
        lws_cancel_service(ctx->wsctx);
        ret = BSC_WEBSOCKET_INVALID_OPERATION;
    } else {
        ret = BSC_WEBSOCKET_SUCCESS;
    }

    pthread_mutex_unlock(ctx->mutex);
    DEBUG_PRINTF("bws_srv_dispatch_send() <<< ret = %d\n", ret);
    return ret;
}

bool bws_srv_get_peer_ip_addr(
    BSC_WEBSOCKET_SRV_HANDLE sh,
    BSC_WEBSOCKET_HANDLE h,
    uint8_t *ip_str,
    size_t ip_str_len,
    uint16_t *port)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;
    int fd;
    struct sockaddr_storage addr;
    socklen_t len;
    const char *ret = NULL;

    if (!ctx || h < 0 || !ip_str || !ip_str_len || !port ||
        h >= bws_srv_get_max_sockets(ctx->proto)) {
        return false;
    }

    pthread_mutex_lock(ctx->mutex);

    if (ctx->conn[h].state != BSC_WEBSOCKET_STATE_IDLE &&
        ctx->conn[h].ws != NULL && !ctx->stop_worker) {
        len = sizeof(addr);
        fd = lws_get_socket_fd(ctx->conn[h].ws);
        if (fd != -1) {
            getpeername(fd, (struct sockaddr *)&addr, &len);
            if (addr.ss_family == AF_INET) {
                struct sockaddr_in *s = (struct sockaddr_in *)&addr;
                *port = ntohs(s->sin_port);
                ret = inet_ntop(
                    AF_INET, &s->sin_addr, (char *)ip_str, ip_str_len);
            } else {
                struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
                *port = ntohs(s->sin6_port);
                ret = inet_ntop(
                    AF_INET6, &s->sin6_addr, (char *)ip_str, ip_str_len);
            }
        }
    }

    pthread_mutex_unlock(ctx->mutex);

    if (ret) {
        return true;
    }

    return false;
}
