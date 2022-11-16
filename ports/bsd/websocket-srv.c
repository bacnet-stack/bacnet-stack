/**
 * @file
 * @brief Implementation of server websocket interface.
 * @author Kirill Neznamov
 * @date June 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/basic/sys/debug.h"

#define DEBUG_WEBSOCKET_SERVER 0

#if DEBUG_WEBSOCKET_SERVER == 1
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF(...)
#endif

#if (LWS_LIBRARY_VERSION_MAJOR >= 4) && (LWS_LIBRARY_VERSION_MINOR > 2)
#error \
    "Unsupported version of libwebsockets. Check details here: bacnet_stack/ports/bsd/websocket-srv.c:27"
/*
  Current version of libwebsockets has an issue under macosx.
  Websockets test shows leakage of file descriptors (pipes) inside
  libwebsockets library. If you want to use that version of the
  libwebsockets you must ensure that the issue is fixed.

  You can check it in the following way:

  1. Build websockets test in bacnet_stack/test/bacnet/datalink/websockets.
  2. Run the test in background mode 'test_websocket &' and remember pid
  3. Run 'lsof -p your_pid' and check that used file descriptors number
    are not constatly growing"
*/

#endif

#define BSC_INITIAL_BUFFER_LEN 512

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

// Some forward function declarations

static int bws_srv_websocket_event(struct lws *wsi,
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
    int fragment_buffer_len;
} BSC_WEBSOCKET_CONNECTION;

static const lws_retry_bo_t retry = {
    .secs_since_valid_ping = 3,
    .secs_since_valid_hangup = 10,
};

static pthread_mutex_t bws_global_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

static pthread_mutex_t bws_srv_direct_mutex[BSC_CONF_WEBSOCKET_SERVERS_NUM] = {
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER
};

static pthread_mutex_t bws_srv_hub_mutex[BSC_CONF_WEBSOCKET_SERVERS_NUM] = {
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER
};

static BSC_WEBSOCKET_CONNECTION bws_hub_conn[BSC_CONF_WEBSOCKET_SERVERS_NUM]
                                            [BSC_SERVER_HUB_WEBSOCKETS_MAX_NUM];
static BSC_WEBSOCKET_CONNECTION
    bws_direct_conn[BSC_CONF_WEBSOCKET_SERVERS_NUM]
                   [BSC_SERVER_DIRECT_WEBSOCKETS_MAX_NUM];

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

static BSC_WEBSOCKET_CONTEXT bws_hub_ctx[BSC_WEBSOCKET_PROTOCOLS_AMOUNT] = {
    0
};
static BSC_WEBSOCKET_CONTEXT bws_direct_ctx[BSC_WEBSOCKET_PROTOCOLS_AMOUNT] = {
    0
};

static BSC_WEBSOCKET_CONTEXT *bws_alloc_server_ctx(BSC_WEBSOCKET_PROTOCOL proto)
{
    int i;
    BSC_WEBSOCKET_CONTEXT *ctx = (proto == BSC_WEBSOCKET_HUB_PROTOCOL)
        ? &bws_hub_ctx[0]
        : &bws_direct_ctx[0];

    pthread_mutex_lock(&bws_global_mutex);

    for (i = 0; i < BSC_CONF_WEBSOCKET_SERVERS_NUM; i++) {
        if (!ctx[i].used) {
            ctx[i].used = true;
            if (proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
                ctx[i].mutex = &bws_srv_hub_mutex[i];
                ctx[i].conn = &bws_hub_conn[i][0];
            } else {
                ctx[i].mutex = &bws_srv_direct_mutex[i];
                ctx[i].conn = &bws_direct_conn[i][0];
            }
            pthread_mutex_unlock(&bws_global_mutex);
            return &ctx[i];
        }
    }
    pthread_mutex_unlock(&bws_global_mutex);
    return NULL;
}

static void bws_free_server_ctx(BSC_WEBSOCKET_CONTEXT *ctx)
{
    pthread_mutex_lock(&bws_global_mutex);
    ctx->used = false;
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
    int ret;
    int i;

    DEBUG_PRINTF("bws_srv_alloc_connection() >>> ctx = %p\n", ctx);

    for (int i = 0; i < bws_srv_get_max_sockets(ctx->proto); i++) {
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

static void bws_srv_free_connection(
    BSC_WEBSOCKET_CONTEXT *ctx, BSC_WEBSOCKET_HANDLE h)
{
    DEBUG_PRINTF("bws_srv_free_connection() >>> ctx = %p, h = %d\n", ctx, h);

    if (h >= 0 && h < bws_srv_get_max_sockets(ctx->proto)) {
        if (ctx->conn[h].state != BSC_WEBSOCKET_STATE_IDLE) {
            if (ctx->conn[h].fragment_buffer) {
                free(ctx->conn[h].fragment_buffer);
                ctx->conn[h].fragment_buffer_len = 0;
                ctx->conn[h].fragment_buffer_size = 0;
            }
            ctx->conn[h].state = BSC_WEBSOCKET_STATE_IDLE;
            ctx->conn[h].ws = NULL;
        }
    }
    DEBUG_PRINTF("bws_srv_free_connection() <<<\n");
}

static BSC_WEBSOCKET_HANDLE bws_find_connnection(
    BSC_WEBSOCKET_CONTEXT *ctx, struct lws *ws)
{
    for (int i = 0; i < bws_srv_get_max_sockets(ctx->proto); i++) {
        if (ctx->conn[i].ws == ws &&
            ctx->conn[i].state != BSC_WEBSOCKET_STATE_IDLE) {
            return i;
        }
    }
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static int bws_srv_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BSC_WEBSOCKET_HANDLE h;
    BSC_WEBSOCKET_CONTEXT *ctx =
        (BSC_WEBSOCKET_CONTEXT *)lws_context_user(lws_get_context(wsi));
    int written;
    int ret = 0;
    BSC_WEBSOCKET_SRV_DISPATCH dispatch_func;
    void *user_param;
    bool stop_worker;

    DEBUG_PRINTF(
        "bws_srv_websocket_event() >>> ctx = %p, proto = %d, wsi = %p, "
        "reason = %d, in = %p, len = %d\n",
        ctx, ctx->proto, wsi, reason, in, len);

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
            dispatch_func = ctx->dispatch_func;
            user_param = ctx->user_param;
            pthread_mutex_unlock(ctx->mutex);
            dispatch_func((BSC_WEBSOCKET_SRV_HANDLE)ctx, h,
                BSC_WEBSOCKET_CONNECTED, NULL, 0, user_param);
            // wakeup worker to process pending event
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
                    "bws_srv_websocket_event() ctx %p proto %d state of "
                    "socket %d is %d\n",
                    ctx, ctx->proto, h, ctx->conn[h].state);
                dispatch_func = ctx->dispatch_func;
                user_param = ctx->user_param;
                stop_worker = ctx->stop_worker;
                bws_srv_free_connection(ctx, h);
                pthread_mutex_unlock(ctx->mutex);
                if (!stop_worker) {
                    dispatch_func((BSC_WEBSOCKET_SRV_HANDLE)ctx, h,
                        BSC_WEBSOCKET_DISCONNECTED, NULL, 0, user_param);
                }
            }
            break;
        }
        case LWS_CALLBACK_RECEIVE: {
            pthread_mutex_lock(ctx->mutex);
            h = bws_find_connnection(ctx, wsi);
            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                pthread_mutex_unlock(ctx->mutex);
            } else {
                DEBUG_PRINTF("bws_srv_websocket_event() ctx %p proto %d "
                             "received %d bytes of "
                             "data for websocket %d\n",
                    ctx, ctx->proto, len, h);
                if (!lws_frame_is_binary(wsi)) {
                    // According AB.7.5.3 BACnet/SC BVLC Message Exchange,
                    // if a received data frame is not binary,
                    // the WebSocket connection shall be closed with a
                    // status code of 1003 -WEBSOCKET_DATA_NOT_ACCEPTED.
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
                            malloc(len <= BSC_INITIAL_BUFFER_LEN
                                    ? BSC_INITIAL_BUFFER_LEN
                                    : len);
                        if (!ctx->conn[h].fragment_buffer) {
                            lws_close_reason(wsi,
                                LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE, NULL, 0);
                            pthread_mutex_unlock(ctx->mutex);
                            DEBUG_PRINTF("bws_srv_websocket_event() <<< ret = "
                                         "-1, allocation of %d bytes failed\n",
                                len <= BSC_INITIAL_BUFFER_LEN
                                    ? BSC_INITIAL_BUFFER_LEN
                                    : len);
                            return -1;
                        }
                        ctx->conn[h].fragment_buffer_len = 0;
                        ctx->conn[h].fragment_buffer_size = len;
                    }
                    if (ctx->conn[h].fragment_buffer_len + len >
                        ctx->conn[h].fragment_buffer_size) {
                        ctx->conn[h].fragment_buffer =
                            realloc(ctx->conn[h].fragment_buffer,
                                ctx->conn[h].fragment_buffer_len + len);
                        if (!ctx->conn[h].fragment_buffer) {
                            lws_close_reason(wsi,
                                LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE, NULL, 0);
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
                        "socket %d\n",
                        len, h);
                    memcpy(
                        &ctx->conn[h]
                             .fragment_buffer[ctx->conn[h].fragment_buffer_len],
                        in, len);
                    ctx->conn[h].fragment_buffer_len += len;
                    if (lws_is_final_fragment(wsi) && !ctx->stop_worker) {
                        dispatch_func = ctx->dispatch_func;
                        user_param = ctx->user_param;
                        pthread_mutex_unlock(ctx->mutex);
                        dispatch_func((BSC_WEBSOCKET_SRV_HANDLE)ctx, h,
                            BSC_WEBSOCKET_RECEIVED,
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
                DEBUG_PRINTF("bws_srv_websocket_event() ctx %p proto %d socket "
                             "%d state = %d\n",
                    ctx, ctx->proto, h, ctx->conn[h].state);
                if (ctx->conn[h].state == BSC_WEBSOCKET_STATE_DISCONNECTING) {
                    DEBUG_PRINTF("bws_srv_websocket_event() <<< ret = -1\n");
                    pthread_mutex_unlock(ctx->mutex);
                    return -1;
                } else if (ctx->conn[h].state ==
                        BSC_WEBSOCKET_STATE_CONNECTED &&
                    !ctx->stop_worker && ctx->conn[h].want_send_data) {
                    ctx->conn[h].can_send_data = true;
                    dispatch_func = ctx->dispatch_func;
                    user_param = ctx->user_param;
                    stop_worker = ctx->stop_worker;
                    pthread_mutex_unlock(ctx->mutex);
                    if (!stop_worker) {
                        dispatch_func((BSC_WEBSOCKET_SRV_HANDLE)ctx, h,
                            BSC_WEBSOCKET_SENDABLE, NULL, 0, user_param);
                    }
                    pthread_mutex_lock(ctx->mutex);
                    ctx->conn[h].want_send_data = false;
                    ctx->conn[h].can_send_data = false;
                    pthread_mutex_unlock(ctx->mutex);
                    // wakeup worker to process internal state
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
        "bws_srv_worker() started for ctx %p proto %d\n", ctx, ctx->proto);

    pthread_mutex_lock(ctx->mutex);
    ctx->dispatch_func((BSC_WEBSOCKET_SRV_HANDLE)ctx, 0,
        BSC_WEBSOCKET_SERVER_STARTED, NULL, 0, ctx->user_param);
    pthread_mutex_unlock(ctx->mutex);

    while (1) {
        DEBUG_PRINTF(
            "bws_srv_worker() ctx %p proto %d blocked\n", ctx, ctx->proto);
        pthread_mutex_lock(ctx->mutex);

        if (ctx->stop_worker) {
            DEBUG_PRINTF("bws_srv_worker() ctx %p proto %d going to stop\n",
                ctx, ctx->proto);
            lws_context_destroy(ctx->wsctx);
            ctx->wsctx = NULL;
            ctx->stop_worker = false;
            DEBUG_PRINTF(
                "bws_srv_worker() ctx %p proto %d emitting stop event\n", ctx,
                ctx->proto);
            dispatch_func = ctx->dispatch_func;
            user_param = ctx->user_param;
            pthread_mutex_unlock(ctx->mutex);
            dispatch_func(
                ctx, 0, BSC_WEBSOCKET_SERVER_STOPPED, NULL, 0, user_param);
            bws_free_server_ctx(ctx);
            DEBUG_PRINTF(
                "bws_srv_worker() ctx %p proto %d stopped\n", ctx, ctx->proto);
            break;
        }

        for (i = 0; i < bws_srv_get_max_sockets(ctx->proto); i++) {
            DEBUG_PRINTF(
                "bws_srv_worker() ctx %p proto %d socket %d(%p) state = %d\n",
                ctx, ctx->proto, i, &ctx->conn[i], ctx->conn[i].state);
            if (ctx->conn[i].state == BSC_WEBSOCKET_STATE_CONNECTED) {
                if (ctx->conn[i].want_send_data) {
                    DEBUG_PRINTF("bws_srv_worker() process request for sending "
                                 "data on socket %d\n",
                        i);
                    lws_callback_on_writable(ctx->conn[i].ws);
                }
            } else if (ctx->conn[i].state ==
                BSC_WEBSOCKET_STATE_DISCONNECTING) {
                DEBUG_PRINTF("bws_srv_worker() process disconnecting event on "
                             "socket %d\n",
                    i);
                lws_callback_on_writable(ctx->conn[i].ws);
            }
        }

        DEBUG_PRINTF(
            "bws_srv_worker() ctx %p proto %d unblocked\n", ctx, ctx->proto);
        pthread_mutex_unlock(ctx->mutex);

        DEBUG_PRINTF("bws_srv_worker() ctx %p proto %d going to block on "
                     "lws_service() call\n",
            ctx, ctx->proto);
        lws_service(ctx->wsctx, 0);
    }

    return NULL;
}

BSC_WEBSOCKET_RET bws_srv_start(BSC_WEBSOCKET_PROTOCOL proto,
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
    int ret;
    BSC_WEBSOCKET_CONTEXT *ctx;

    struct lws_protocols protos[] = {
        { (proto == BSC_WEBSOCKET_HUB_PROTOCOL)
                ? BSC_WEBSOCKET_HUB_PROTOCOL_STR
                : BSC_WEBSOCKET_DIRECT_PROTOCOL_STR,
            bws_srv_websocket_event, 0, 0, 0, NULL, 0 },
        LWS_PROTOCOL_LIST_TERM
    };
    DEBUG_PRINTF("bws_srv_start() >>> proto = %d port = %d\n", proto, port);

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

    pthread_mutex_lock(ctx->mutex);

#if DEBUG_ENABLED == 1
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG |
            LLL_PARSER | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY |
            LLL_USER | LLL_THREAD,
        NULL);
#else
    lws_set_log_level(0, NULL);
#endif

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
    info.timeout_secs = timeout_s;
    info.connect_timeout_secs = timeout_s;
    info.user = ctx;
    ctx->wsctx = lws_create_context(&info);

    if (!ctx->wsctx) {
        pthread_mutex_unlock(ctx->mutex);
        bws_free_server_ctx(ctx);
        DEBUG_PRINTF("bws_srv_start() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    ret = pthread_create(&thread_id, NULL, &bws_srv_worker, ctx);

    if (ret != 0) {
        lws_context_destroy(ctx->wsctx);
        ctx->wsctx = NULL;
        pthread_mutex_unlock(ctx->mutex);
        bws_free_server_ctx(ctx);
        DEBUG_PRINTF(
            "bws_srv_start() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    ctx->dispatch_func = dispatch_func;
    ctx->user_param = dispatch_func_user_param;
    ctx->proto = proto;
    pthread_mutex_unlock(ctx->mutex);
    *sh = (BSC_WEBSOCKET_SRV_HANDLE)ctx;
    DEBUG_PRINTF("bws_srv_start() <<< ret = BSC_WEBSOCKET_SUCCESS\n");
    return BSC_WEBSOCKET_SUCCESS;
}

BSC_WEBSOCKET_RET bws_srv_stop(BSC_WEBSOCKET_SRV_HANDLE sh)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;

    DEBUG_PRINTF("bws_srv_stop() >>> ctx = %p\n", ctx);

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
    // wake up libwebsockets runloop
    lws_cancel_service(ctx->wsctx);
    pthread_mutex_unlock(ctx->mutex);

    DEBUG_PRINTF("bws_srv_stop() <<< ret = BSC_WEBSOCKET_SUCCESS\n");
    return BSC_WEBSOCKET_SUCCESS;
}

void bws_srv_disconnect(BSC_WEBSOCKET_SRV_HANDLE sh, BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;
    debug_printf("bws_srv_disconnect() >>> sh = %p h = %d\n", sh, h);

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
            // tell worker to process change of connection state
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
        // tell worker to process send request
        ctx->conn[h].want_send_data = true;
        lws_cancel_service(ctx->wsctx);
    }
    pthread_mutex_unlock(ctx->mutex);
    DEBUG_PRINTF("bws_srv_send() <<<\n");
}

BSC_WEBSOCKET_RET bws_srv_dispatch_send(BSC_WEBSOCKET_SRV_HANDLE sh,
    BSC_WEBSOCKET_HANDLE h,
    uint8_t *payload,
    size_t payload_size)
{
    int written;
    uint8_t *tmp_buf;
    BSC_WEBSOCKET_RET ret;
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;

    DEBUG_PRINTF("bws_srv_dispatch_send() >>> ctx = %p h = %d payload %p "
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

    // malloc() and copying is evil, but libwesockets wants some space before
    // actual payload.

    tmp_buf = malloc(payload_size + LWS_PRE);

    if (!tmp_buf) {
        DEBUG_PRINTF(
            "bws_srv_dispatch_send() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        pthread_mutex_unlock(ctx->mutex);
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    memcpy(&tmp_buf[LWS_PRE], payload, payload_size);

    written = lws_write(
        ctx->conn[h].ws, &tmp_buf[LWS_PRE], payload_size, LWS_WRITE_BINARY);

    DEBUG_PRINTF("bws_srv_dispatch_send() %d bytes is sent\n", written);

    if (written < (int)payload_size) {
        DEBUG_PRINTF(
            "bws_srv_dispatch_send() websocket connection is broken(closed)\n");
        // tell worker to process change of connection state
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