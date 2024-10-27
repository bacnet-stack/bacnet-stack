/**
 * @file
 * @brief Implementation of server websocket interface Zephyr.
 * @author Mikhail Antropov
 * @date Feb 2023
 * @section LICENSE
 *
 * Copyright (C) 2023 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#define _FCNTL_H
#include "mongoose.h"
#undef _FCNTL_H
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/basic/sys/debug.h"
#include "websocket-global.h"

LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

#if CONFIG_BACNETSTACK_LOG_LEVEL == LOG_LEVEL_DBG
#define DUMP_BUFFER debug_printf_hex
#else
#define DUMP_BUFFER(...)
#endif

typedef enum {
    BSC_WEBSOCKET_STATE_IDLE = 0,
    BSC_WEBSOCKET_STATE_CONNECTING = 1,
    BSC_WEBSOCKET_STATE_CONNECTED = 2,
    BSC_WEBSOCKET_STATE_DISCONNECTING = 3
} BSC_WEBSOCKET_STATE;

typedef struct {
    struct mg_connection *ws;
    BSC_WEBSOCKET_STATE state;
    bool want_send_data;
    bool want_close;
} BSC_WEBSOCKET_CONNECTION;

static BSC_WEBSOCKET_CONNECTION
    bws_hub_conn[BSC_CONF_WEBSOCKET_SERVERS_NUM]
                [BSC_SERVER_HUB_WEBSOCKETS_MAX_NUM] = { 0 };
static BSC_WEBSOCKET_CONNECTION
    bws_direct_conn[BSC_CONF_WEBSOCKET_SERVERS_NUM]
                   [BSC_SERVER_DIRECT_WEBSOCKETS_MAX_NUM] = { 0 };

static K_MUTEX_DEFINE(bws_global_mutex);

typedef enum {
    BSC_WEBSOCKET_SERVER_STATE_IDLE = 0,
    BSC_WEBSOCKET_SERVER_STATE_START = 1,
    BSC_WEBSOCKET_SERVER_STATE_RUN = 2,
    BSC_WEBSOCKET_SERVER_STATE_STOPPING = 3,
    BSC_WEBSOCKET_SERVER_STATE_STOPPED = 4
} BSC_WEBSOCKET_SERVER_STATE;

typedef struct BACNetWebsocketServerContext {
    BSC_WEBSOCKET_SERVER_STATE state;
    struct mg_mgr mgr;
    uint8_t *ca_cert;
    uint8_t *cert;
    uint8_t *key;
    char url[100];
    BSC_WEBSOCKET_PROTOCOL proto;
    BSC_WEBSOCKET_CONNECTION *conn;
    BSC_WEBSOCKET_SRV_DISPATCH dispatch_func;
    struct k_mutex mutex;
    void *user_param;
    k_tid_t thread_id;
    struct k_thread worker_thr;
    k_thread_stack_t *stack;
} BSC_WEBSOCKET_CONTEXT;

static BSC_WEBSOCKET_CONTEXT bws_hub_ctx[BSC_CONF_WEBSOCKET_SERVERS_NUM] = {
    0
};
static BSC_WEBSOCKET_CONTEXT bws_direct_ctx[BSC_CONF_WEBSOCKET_SERVERS_NUM] = {
    0
};

#define STACKSIZE 4096

K_KERNEL_STACK_ARRAY_DEFINE(
    bws_hub_ctx_stack, BSC_CONF_WEBSOCKET_SERVERS_NUM, STACKSIZE);
K_KERNEL_STACK_ARRAY_DEFINE(
    bws_direct_ctx_stack, BSC_CONF_WEBSOCKET_SERVERS_NUM, STACKSIZE);

static BSC_WEBSOCKET_CONTEXT *bws_alloc_server_ctx(BSC_WEBSOCKET_PROTOCOL proto)
{
    int i;
    BSC_WEBSOCKET_CONTEXT *ctx = (proto == BSC_WEBSOCKET_HUB_PROTOCOL)
        ? &bws_hub_ctx[0]
        : &bws_direct_ctx[0];

    k_mutex_lock(&bws_global_mutex, K_FOREVER);
    LOG_INF("bws_alloc_server_ctx() >>> proto = %d", proto);

    for (i = 0; i < BSC_CONF_WEBSOCKET_SERVERS_NUM; i++) {
        if (ctx[i].state == BSC_WEBSOCKET_SERVER_STATE_IDLE) {
            if (proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
                ctx[i].conn = &bws_hub_conn[i][0];
                ctx[i].stack = bws_hub_ctx_stack[i];
            } else {
                ctx[i].conn = &bws_direct_conn[i][0];
                ctx[i].stack = bws_direct_ctx_stack[i];
            }
            if (k_mutex_init(&ctx[i].mutex) != 0) {
                LOG_WRN("bws_alloc_server_ctx() <<< ret = %p", &ctx[i]);
                k_mutex_unlock(&bws_global_mutex);
                return NULL;
            }
            ctx[i].state = BSC_WEBSOCKET_SERVER_STATE_START;
            LOG_WRN("bws_alloc_server_ctx() <<< ret = %p", &ctx[i]);
            k_mutex_unlock(&bws_global_mutex);
            return &ctx[i];
        }
    }
    LOG_INF("bws_alloc_server_ctx() <<< ret = NULL");
    k_mutex_unlock(&bws_global_mutex);
    return NULL;
}

static void bws_free_server_ctx(BSC_WEBSOCKET_CONTEXT *ctx)
{
    k_mutex_lock(&bws_global_mutex, K_FOREVER);
    LOG_INF("bws_free_server_ctx() >>> ctx = %p", ctx);
    mg_mgr_free(&ctx->mgr);
    ctx->state = BSC_WEBSOCKET_SERVER_STATE_IDLE;
    ctx->conn = NULL;
    ctx->dispatch_func = NULL;
    ctx->user_param = NULL;
    LOG_INF("bws_free_server_ctx() <<< ");
    k_mutex_unlock(&bws_global_mutex);
}

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

static bool bws_open_connect_number(BSC_WEBSOCKET_CONTEXT *ctx)
{
    int k = 0;
    int i;
    for (i = 0; i < bws_srv_get_max_sockets(ctx->proto); i++) {
        if (ctx->conn[i].state != BSC_WEBSOCKET_STATE_IDLE) {
            k++;
        }
    }
    return k;
}

static bool bws_validate_ctx_pointer(BSC_WEBSOCKET_CONTEXT *ctx)
{
    return ctx->state != BSC_WEBSOCKET_SERVER_STATE_IDLE;
}

static BSC_WEBSOCKET_CONTEXT *bws_server_find(const struct mg_connection *ws)
{
    static BSC_WEBSOCKET_CONTEXT *contexts[2] = { bws_hub_ctx, bws_direct_ctx };
    BSC_WEBSOCKET_CONTEXT *ctx;
    int i;
    int k;

    for (k = 0; k < sizeof(contexts) / sizeof(BSC_WEBSOCKET_CONTEXT *); k++) {
        ctx = contexts[k];
        for (i = 0; i < BSC_CONF_WEBSOCKET_SERVERS_NUM; i++) {
            if (&ctx[i].mgr == ws->mgr) {
                return &ctx[i];
            }
        }
    }
    return NULL;
}

static const char *bws_srv_get_proto_str(BSC_WEBSOCKET_PROTOCOL proto)
{
    if (proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
        return BSC_WEBSOCKET_HUB_PROTOCOL_STR;
    } else if (proto == BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        return BSC_WEBSOCKET_DIRECT_PROTOCOL_STR;
    }
    return NULL;
}

static void bws_call_dispatch_func(
    BSC_WEBSOCKET_CONTEXT *ctx,
    BSC_WEBSOCKET_HANDLE h,
    BSC_WEBSOCKET_EVENT ev,
    BACNET_ERROR_CODE ws_reason,
    char *ws_reason_desc,
    uint8_t *buf,
    size_t size)
{
    BSC_WEBSOCKET_SRV_DISPATCH dispatch_func;
    void *user_param;

    dispatch_func = ctx->dispatch_func;
    user_param = ctx->user_param;
    k_mutex_unlock(&ctx->mutex);
    dispatch_func(
        (BSC_WEBSOCKET_SRV_HANDLE)ctx, h, ev, ws_reason, ws_reason_desc, buf,
        size, user_param);
    k_mutex_lock(&ctx->mutex, K_FOREVER);
}

static void bws_server_stop(BSC_WEBSOCKET_CONTEXT *ctx)
{
    LOG_INF("bws_server_stop() >>> ctx %p", ctx);
    k_mutex_lock(&ctx->mutex, K_FOREVER);
    ctx->state = BSC_WEBSOCKET_SERVER_STATE_STOPPED;
    k_mutex_unlock(&ctx->mutex);

    bws_call_dispatch_func(
        ctx, 0, BSC_WEBSOCKET_SERVER_STOPPED, 0, NULL, NULL, 0);
    bws_free_server_ctx(ctx);
    LOG_INF("bws_server_stop() <<<");
}

static BSC_WEBSOCKET_HANDLE bws_srv_alloc_connection(BSC_WEBSOCKET_CONTEXT *ctx)
{
    int i;

    LOG_INF("bws_srv_alloc_connection() >>> ctx = %p", ctx);

    for (i = 0; i < bws_srv_get_max_sockets(ctx->proto); i++) {
        if (ctx->conn[i].state == BSC_WEBSOCKET_STATE_IDLE) {
            memset(&ctx->conn[i], 0, sizeof(ctx->conn[i]));
            LOG_WRN("bws_srv_alloc_connection() <<< ret = %d", i);
            return i;
        }
    }

    LOG_INF("bws_srv_alloc_connection() <<< ret = "
            "BSC_WEBSOCKET_INVALID_HANDLE");
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static void
bws_srv_free_connection(BSC_WEBSOCKET_CONTEXT *ctx, BSC_WEBSOCKET_HANDLE h)
{
    LOG_INF("bws_srv_free_connection() >>> ctx = %p, h = %d", ctx, h);

    if (h >= 0 && h < bws_srv_get_max_sockets(ctx->proto)) {
        if (ctx->conn[h].state != BSC_WEBSOCKET_STATE_IDLE) {
            memset(&ctx->conn[h], 0, sizeof(ctx->conn[h]));
            ctx->conn[h].state = BSC_WEBSOCKET_STATE_IDLE;
        }
    }
    LOG_INF("bws_srv_free_connection() <<<");
}

static bool bws_find_connnection(
    const struct mg_connection *ws,
    BSC_WEBSOCKET_CONTEXT **pctx,
    BSC_WEBSOCKET_HANDLE *h)
{
    BSC_WEBSOCKET_CONTEXT *ctx;
    int i;

    ctx = bws_server_find(ws);
    if (!ctx) {
        return false;
    }

    for (i = 0; i < bws_srv_get_max_sockets(ctx->proto); i++) {
        if (ctx->conn[i].ws == ws &&
            ctx->conn[i].state != BSC_WEBSOCKET_STATE_IDLE) {
            *pctx = ctx;
            *h = i;
            return true;
        }
    }
    return false;
}

static void bws_srv_websocket_event(
    struct mg_connection *ws, int ev, void *ev_data, void *fn_data)
{
    BSC_WEBSOCKET_HANDLE h = BSC_WEBSOCKET_INVALID_HANDLE;
    BSC_WEBSOCKET_CONTEXT *ctx = NULL;

    if (!bws_find_connnection(ws, &ctx, &h) && (ev != MG_EV_ACCEPT) &&
        (ev != MG_EV_ERROR) && (ev != MG_EV_POLL)) {
        LOG_INF(
            "bws_srv_websocket_event() >>> unknown ctx = %p, ev = %d", ctx, ev);
    }

    if (ctx) {
        k_mutex_lock(&ctx->mutex, K_FOREVER);
    }

    // LOG_INF("bws_srv_websocket_event() >>> ws = %p, ctx = %p, ev = %d, proto
    // = %d",
    //         ws, ctx, ev, (ctx ? ctx->proto : -1));

    switch (ev) {
        case MG_EV_ERROR: {
            LOG_ERR("bws_srv_websocket_event() error = %s", (char *)ev_data);
            // todo dispatch error

            break;
        }
        case MG_EV_OPEN:
            break;

        case MG_EV_ACCEPT: {
            LOG_INF("bws_srv_websocket_event() accept connection");
            ctx = bws_server_find(ws);
            if (!ctx) {
                LOG_DBG("bws_srv_websocket_event() server matching error, "
                        "dropping incoming connection");
                ws->is_draining = 1;
                break;
            }

            h = BSC_WEBSOCKET_INVALID_HANDLE;
            k_mutex_lock(&ctx->mutex, K_FOREVER);
            if (ctx->state == BSC_WEBSOCKET_SERVER_STATE_RUN) {
                h = bws_srv_alloc_connection(ctx);
            }
            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                LOG_DBG("bws_srv_websocket_event() no free sockets, "
                        "dropping incoming connection");
                ws->is_draining = 1;
                break;
            }
            LOG_DBG(
                "bws_srv_websocket_event() ctx %p proto %d set state of"
                " socket %d to BACNET_WEBSOCKET_STATE_CONNECTING",
                ctx, ctx->proto, h);
            ctx->conn[h].ws = ws;
            ctx->conn[h].state = BSC_WEBSOCKET_STATE_CONNECTING;

            struct mg_tls_opts opts = { .ca = ctx->ca_cert,
                                        .cert = ctx->cert,
                                        .certkey = ctx->key };
            mg_tls_init(ws, &opts);
            break;
        }
        case MG_EV_CLOSE: {
            LOG_INF("bws_srv_websocket_event() closed connection ctx %p", ctx);
            if (ctx) {
                LOG_INF(
                    "proto %d state of socket %d is %d", ctx->proto, h,
                    ctx->conn[h].state);
                bws_srv_free_connection(ctx, h);
                bws_call_dispatch_func(
                    ctx, h, BSC_WEBSOCKET_DISCONNECTED, 0, NULL, NULL, 0);
            }
            break;
        }
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *)ev_data;
            struct mg_str *wsproto =
                mg_http_get_header(hm, "Sec-WebSocket-Protocol");

            if (strncmp(
                    bws_srv_get_proto_str(ctx->proto), wsproto->ptr,
                    wsproto->len) != 0) {
                mg_http_reply(ws, 426, "", "Unknown WS protocol");
                ws->is_draining = 1;
            } else {
                // Upgrade to websocket. From now on, a connection is a
                // full-duplex Websocket connection, which will receive
                // MG_EV_WS_MSG events.
                mg_ws_upgrade(ws, hm, NULL);
            }
            break;
        }
        case MG_EV_WS_OPEN: {
            ctx->conn[h].state = BSC_WEBSOCKET_STATE_CONNECTED;
            bws_call_dispatch_func(
                ctx, h, BSC_WEBSOCKET_CONNECTED, 0, NULL, NULL, 0);
            break;
        }
        case MG_EV_WS_MSG: {
            struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
            LOG_DBG(
                "bws_srv_websocket_event() ctx %p proto %d "
                "received %d bytes of data for websocket %d",
                ctx, ctx->proto, wm->data.len, h);
            DUMP_BUFFER(
                0, (uint8_t *)wm->data.ptr, wm->data.len, "Server receive");
            bws_call_dispatch_func(
                ctx, h, BSC_WEBSOCKET_RECEIVED, 0, NULL,
                (uint8_t *)wm->data.ptr, wm->data.len);
            break;
        }
        case MG_EV_WS_CTL: {
            struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
            if ((wm->flags & 0x0f) == WEBSOCKET_OP_CLOSE) {
                LOG_DBG("bws_srv_websocket_event() ctx %p stopping", ctx);
            }
            break;
        }
        default:
            break;
    }

    if (ctx) {
        k_mutex_unlock(&ctx->mutex);
    }

    // LOG_DBG("bws_srv_websocket_event() <<<");
}

// p1 - pointer of BSC_WEBSOCKET_CONTEXT
// p2 - poll timeout
static void bws_srv_worker(void *p1, void *p2, void *p3)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)p1;
    int timeout = (int)p2;
    BSC_WEBSOCKET_SERVER_STATE state = BSC_WEBSOCKET_SERVER_STATE_RUN;
    struct mg_connection *srv_conn;
    int h;

    // check status
    k_mutex_lock(&ctx->mutex, K_FOREVER);
    state = ctx->state;
    k_mutex_unlock(&ctx->mutex);
    if (state != BSC_WEBSOCKET_SERVER_STATE_START) {
        goto exit;
    }

    // init
    mg_mgr_init(&ctx->mgr);
    srv_conn =
        mg_http_listen(&ctx->mgr, ctx->url, bws_srv_websocket_event, NULL);
    if (!srv_conn) {
        LOG_ERR("bws_srv_worker() server %p cannot start", ctx);
        goto exit;
    }

    LOG_INF("bws_srv_worker() start server %p", ctx);
    k_mutex_lock(&ctx->mutex, K_FOREVER);
    bws_call_dispatch_func(
        ctx, 0, BSC_WEBSOCKET_SERVER_STARTED, 0, NULL, NULL, 0);
    if (state == BSC_WEBSOCKET_SERVER_STATE_START) {
        ctx->state = BSC_WEBSOCKET_SERVER_STATE_RUN;
        state = ctx->state;
    }
    k_mutex_unlock(&ctx->mutex);

    // run loop
    while ((state == BSC_WEBSOCKET_SERVER_STATE_RUN) ||
           (state == BSC_WEBSOCKET_SERVER_STATE_STOPPING)) {
        mg_mgr_poll(&ctx->mgr, timeout);

        if ((state == BSC_WEBSOCKET_SERVER_STATE_STOPPING) &&
            (bws_open_connect_number(ctx) == 0)) {
            break;
        }

        k_mutex_lock(&ctx->mutex, K_FOREVER);
        state = ctx->state;

        for (h = 0; h < bws_srv_get_max_sockets(ctx->proto); h++) {
            if ((ctx->conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) &&
                (ctx->conn[h].want_send_data)) {
                ctx->conn[h].want_send_data = false;
                bws_call_dispatch_func(
                    ctx, h, BSC_WEBSOCKET_SENDABLE, 0, NULL, NULL, 0);
            }
            if (ctx->conn[h].want_close && ctx->conn[h].ws) {
                ctx->conn[h].ws->is_draining = 1;
            }
        }

        k_mutex_unlock(&ctx->mutex);
    }

exit:
    LOG_INF("bws_srv_worker() stop server %p", ctx);
    bws_server_stop(ctx);
}

static void search_iface_name_cb(struct net_if *iface, void *user_data)
{
    char **ud = (char **)user_data;
    uint32_t ip;
    LOG_DBG("Iface name: %s", iface->if_dev->dev->name);
    if (strcmp(ud[0], iface->if_dev->dev->name) == 0) {
        ip = ntohl(iface->config.ip.ipv4->gw.s_addr);
        sprintf(
            ud[1], "%d.%d.%d.%d", ip & 0xff000000, ip & 0x00ff0000,
            ip & 0x0000ff00, ip & 0x000000ff);
    }
}

static const char *iface_to_ipv4(char *iface)
{
    static char str[80];
    char *net_if_foreach_data[2] = { iface, str };
    if (!iface) {
        return "0.0.0.0";
    }

    str[0] = 0;
    net_if_foreach(search_iface_name_cb, net_if_foreach_data);
    return str;
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
    BSC_WEBSOCKET_CONTEXT *ctx;
    k_tid_t thread_id;

    LOG_DBG(
        "bws_srv_start() >>> proto = %d port = %d "
        "dispatch_func_user_param = %p",
        proto, port, dispatch_func_user_param);

    if (proto != BSC_WEBSOCKET_HUB_PROTOCOL &&
        proto != BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        LOG_DBG("bws_srv_start() <<< bad protocol, ret = "
                "BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size ||
        !dispatch_func || !timeout_s || !sh) {
        LOG_DBG("bws_srv_start() <<< ret = BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (port < 0 || port > 65535) {
        LOG_DBG("bws_srv_start() <<< ret = BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    ctx = bws_alloc_server_ctx(proto);

    if (!ctx) {
        LOG_DBG(
            "bws_srv_start() <<< maximum amount of servers for "
            "server proto %d is to small, ret = BSC_WEBSOCKET_NO_RESOURCES",
            proto);
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    k_mutex_lock(&ctx->mutex, K_FOREVER);

    ctx->ca_cert = ca_cert;
    ctx->cert = cert;
    ctx->key = key;
    ctx->proto = proto;
    ctx->dispatch_func = dispatch_func;
    ctx->user_param = dispatch_func_user_param;
    ctx->mgr.userdata = ctx;
    snprintf(
        ctx->url, sizeof(ctx->url), "wss://%s:%d", iface_to_ipv4(iface), port);

    // enums log level in mongoose and zephyr are similar
    mg_log_set((int)CONFIG_BACNETSTACK_LOG_LEVEL);

    thread_id = k_thread_create(
        &ctx->worker_thr, ctx->stack, STACKSIZE, bws_srv_worker, ctx,
        (void *)50, NULL, -1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
    ctx->thread_id = thread_id;
    k_mutex_unlock(&ctx->mutex);

    if (thread_id == 0) {
        bws_free_server_ctx(ctx);
        LOG_DBG("bws_srv_start() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    *sh = (BSC_WEBSOCKET_SRV_HANDLE)ctx;
    LOG_DBG("bws_srv_start() <<< ret = BSC_WEBSOCKET_SUCCESS");
    return BSC_WEBSOCKET_SUCCESS;
}

BSC_WEBSOCKET_RET bws_srv_stop(BSC_WEBSOCKET_SRV_HANDLE sh)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;
    int h;

    LOG_INF(
        "bws_srv_stop() >>> ctx = %p user_param = %p", ctx, ctx->user_param);

    if (!bws_validate_ctx_pointer(ctx)) {
        LOG_INF("bws_srv_stop() <<< bad websocket handle, ret = "
                "BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    k_mutex_lock(&ctx->mutex, K_FOREVER);

    if ((ctx->state == BSC_WEBSOCKET_SERVER_STATE_STOPPING) ||
        (ctx->state == BSC_WEBSOCKET_SERVER_STATE_STOPPED)) {
        k_mutex_unlock(&ctx->mutex);
        LOG_INF("bws_srv_stop() <<< ret = BSC_WEBSOCKET_INVALID_OPERATION");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    ctx->state = BSC_WEBSOCKET_SERVER_STATE_STOPPING;
    LOG_INF("bws_srv_stop() BSC_WEBSOCKET_SERVER_STATE_STOPPING");
    for (h = 0; h < bws_srv_get_max_sockets(ctx->proto); h++) {
        if ((ctx->conn[h].state == BSC_WEBSOCKET_STATE_CONNECTING) ||
            (ctx->conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED)) {
            // tell worker to process change of connection state
            ctx->conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
            ctx->conn[h].want_close = true;
        }
    }
    k_mutex_unlock(&ctx->mutex);

    LOG_INF("bws_srv_stop() <<< ret = BSC_WEBSOCKET_SUCCESS");
    return BSC_WEBSOCKET_SUCCESS;
}

void bws_srv_disconnect(BSC_WEBSOCKET_SRV_HANDLE sh, BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;
    LOG_INF("bws_srv_disconnect() >>> sh = %p h = %d", sh, h);

    if (!bws_validate_ctx_pointer(ctx)) {
        LOG_INF("bws_srv_disconnect() <<< bad websocket handle");
        return;
    }

    k_mutex_lock(&ctx->mutex, K_FOREVER);
    if (h >= 0 && h < bws_srv_get_max_sockets(ctx->proto) &&
        (ctx->state != BSC_WEBSOCKET_SERVER_STATE_STOPPING) &&
        (ctx->state != BSC_WEBSOCKET_SERVER_STATE_STOPPED)) {
        if (ctx->conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
            // tell worker to process change of connection state
            ctx->conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
            ctx->conn[h].want_close = true;
        }
    }
    k_mutex_unlock(&ctx->mutex);
    LOG_INF("bws_srv_disconnect() <<<");
}

void bws_srv_send(BSC_WEBSOCKET_SRV_HANDLE sh, BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;

    LOG_INF("bws_srv_send() >>> ctx = %p h = %d", ctx, h);

    if (!bws_validate_ctx_pointer(ctx)) {
        LOG_INF("bws_srv_send() <<< bad websocket handle");
        return;
    }

    k_mutex_lock(&ctx->mutex, K_FOREVER);
    if (h >= 0 && h < bws_srv_get_max_sockets(ctx->proto) &&
        ctx->conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
        ctx->conn[h].want_send_data = true;
    }
    k_mutex_unlock(&ctx->mutex);

    LOG_INF("bws_srv_send() <<<");
}

// call from bws_srv_worker
BSC_WEBSOCKET_RET bws_srv_dispatch_send(
    BSC_WEBSOCKET_SRV_HANDLE sh,
    BSC_WEBSOCKET_HANDLE h,
    uint8_t *payload,
    size_t payload_size)
{
    int written;
    BSC_WEBSOCKET_RET ret;
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;

    LOG_INF(
        "bws_srv_dispatch_send() >>> ctx = %p h = %d payload %p "
        "payload_size %d",
        ctx, h, payload, payload_size);

    if (!bws_validate_ctx_pointer(ctx)) {
        LOG_INF("bws_srv_dispatch_send() <<< bad websocket handle, ret = "
                "BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (h < 0 || h >= bws_srv_get_max_sockets(ctx->proto)) {
        LOG_INF("bws_srv_dispatch_send() <<< BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (!payload || !payload_size) {
        LOG_INF("bws_srv_dispatch_send() <<< BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (ctx->state != BSC_WEBSOCKET_SERVER_STATE_RUN) {
        k_mutex_unlock(&ctx->mutex);
        LOG_INF("bws_srv_dispatch_send() <<< ret = "
                "BSC_WEBSOCKET_INVALID_OPERATION");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    if (ctx->conn[h].state != BSC_WEBSOCKET_STATE_CONNECTED) {
        LOG_INF("bws_srv_dispatch_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM");
        k_mutex_unlock(&ctx->mutex);
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    DUMP_BUFFER(0, payload, payload_size, "Server send");
    written =
        mg_ws_send(ctx->conn[h].ws, payload, payload_size, WEBSOCKET_OP_BINARY);

    LOG_INF("bws_srv_dispatch_send() %d bytes is sent", written);

    if (written < (int)payload_size) {
        LOG_INF(
            "bws_srv_dispatch_send() websocket connection is broken(closed)");
        // tell worker to process change of connection state
        ctx->conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
        ctx->conn[h].ws->is_draining = 1;
        ret = BSC_WEBSOCKET_INVALID_OPERATION;
    } else {
        ret = BSC_WEBSOCKET_SUCCESS;
    }

    LOG_INF("bws_srv_dispatch_send() <<< ret = %d", ret);
    return ret;
}

char *bws_ntoa(const struct mg_addr *addr, char *buf, size_t len)
{
    if (addr->is_ip6) {
        uint16_t *p = (uint16_t *)addr->ip6;
        mg_snprintf(
            buf, len, "%x:%x:%x:%x:%x:%x:%x:%x", mg_htons(p[0]), mg_htons(p[1]),
            mg_htons(p[2]), mg_htons(p[3]), mg_htons(p[4]), mg_htons(p[5]),
            mg_htons(p[6]), mg_htons(p[7]));
    } else {
        uint8_t p[4];
        memcpy(p, &addr->ip, sizeof(p));
        mg_snprintf(
            buf, len, "%d.%d.%d.%d", (int)p[0], (int)p[1], (int)p[2],
            (int)p[3]);
    }
    return buf;
}

bool bws_srv_get_peer_ip_addr(
    BSC_WEBSOCKET_SRV_HANDLE sh,
    BSC_WEBSOCKET_HANDLE h,
    uint8_t *ip_str,
    size_t ip_str_len,
    uint16_t *port)
{
    BSC_WEBSOCKET_CONTEXT *ctx = (BSC_WEBSOCKET_CONTEXT *)sh;
    bool ret = false;

    if (!ctx || h < 0 || !ip_str || !ip_str_len || !port ||
        h >= bws_srv_get_max_sockets(ctx->proto)) {
        return ret;
    }

    k_mutex_lock(&ctx->mutex, K_FOREVER);

    if (ctx->conn[h].ws != NULL) {
        bws_ntoa(&ctx->conn[h].ws->rem, ip_str, ip_str_len);
        *port = ctx->conn[h].ws->rem.port;
        ret = true;
    }

    k_mutex_unlock(&ctx->mutex);

    return ret;
}
