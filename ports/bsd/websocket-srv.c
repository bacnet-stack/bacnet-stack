/**
 * @file
 * @brief Implementation of server websocket interface MAC OS / BSD.
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
#include "bacnet/basic/sys/fifo.h"
#include "bacnet/basic/sys/debug.h"

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

static int bws_srv_websocket_direct_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len);

static int bws_srv_websocket_hub_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len);

static int bws_srv_websocket_event(BSC_WEBSOCKET_PROTOCOL proto,
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
} BSC_WEBSOCKET_CONNECTION;

static struct lws_protocols bws_srv_direct_protos[] = {
    { BSC_WEBSOCKET_DIRECT_PROTOCOL_STR, bws_srv_websocket_direct_event, 0, 0,
        0, NULL, 0 },
    LWS_PROTOCOL_LIST_TERM
};

static struct lws_protocols bws_srv_hub_protos[] = {
    { BSC_WEBSOCKET_HUB_PROTOCOL_STR, bws_srv_websocket_hub_event, 0, 0, 0,
        NULL, 0 },
    LWS_PROTOCOL_LIST_TERM
};

static const lws_retry_bo_t retry = {
    .secs_since_valid_ping = 3,
    .secs_since_valid_hangup = 10,
};

static pthread_mutex_t bws_srv_direct_mutex =
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static pthread_mutex_t bws_srv_hub_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static BSC_WEBSOCKET_CONNECTION bws_hub_conn[BSC_SERVER_HUB_WEBSOCKETS_MAX_NUM];
static BSC_WEBSOCKET_CONNECTION
    bws_direct_conn[BSC_SERVER_DIRECT_WEBSOCKETS_MAX_NUM];

typedef struct BACNetWebsocketServerContext {
    struct lws_context *wsctx;
    pthread_mutex_t *mutex;
    BSC_WEBSOCKET_CONNECTION *conn;
    int conn_size;
    BSC_WEBSOCKET_SRV_DISPATCH dispatch_func;
    bool stop_worker;
} BSC_WEBSOCKET_CONTEXT;

static BSC_WEBSOCKET_CONTEXT bws_ctx[BSC_WEBSOCKET_PROTOCOLS_AMOUNT] = {
    { NULL, &bws_srv_hub_mutex, bws_hub_conn, BSC_SERVER_HUB_WEBSOCKETS_MAX_NUM,
        NULL, false },
    { NULL, &bws_srv_direct_mutex, bws_direct_conn,
        BSC_SERVER_DIRECT_WEBSOCKETS_MAX_NUM, NULL, false }
};

static BSC_WEBSOCKET_PROTOCOL bws_srv_proto_by_ctx(BSC_WEBSOCKET_CONTEXT *ctx)
{
    if (ctx == &bws_ctx[BSC_WEBSOCKET_HUB_PROTOCOL]) {
        return BSC_WEBSOCKET_HUB_PROTOCOL;
    } else {
        return BSC_WEBSOCKET_DIRECT_PROTOCOL;
    }
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

static BSC_WEBSOCKET_HANDLE bws_srv_alloc_connection(
    BSC_WEBSOCKET_PROTOCOL proto)
{
    int ret;
    int i;

    debug_printf("bws_srv_alloc_connection() >>> proto = %d\n", proto);

    for (int i = 0; i < bws_srv_get_max_sockets(proto); i++) {
        if (bws_ctx[proto].conn[i].state == BSC_WEBSOCKET_STATE_IDLE) {
            memset(&bws_ctx[proto].conn[i], 0, sizeof(bws_ctx[proto].conn[i]));
            debug_printf("bws_srv_alloc_connection() <<< ret = %d\n", i);
            return i;
        }
    }

    debug_printf("bws_srv_alloc_connection() <<< ret = "
                 "BSC_WEBSOCKET_INVALID_HANDLE\n");
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static void bws_srv_free_connection(
    BSC_WEBSOCKET_PROTOCOL proto, BSC_WEBSOCKET_HANDLE h)
{
    debug_printf(
        "bws_srv_free_connection() >>> proto = %d, h = %d\n", proto, h);

    if (h >= 0 && h < bws_srv_get_max_sockets(proto)) {
        if (bws_ctx[proto].conn[h].state != BSC_WEBSOCKET_STATE_IDLE) {
            bws_ctx[proto].conn[h].state = BSC_WEBSOCKET_STATE_IDLE;
            bws_ctx[proto].conn[h].ws = NULL;
        }
    }
    debug_printf("bws_srv_free_connection() <<<\n");
}

static BSC_WEBSOCKET_HANDLE bws_find_connnection(
    BSC_WEBSOCKET_PROTOCOL proto, struct lws *ws)
{
    for (int i = 0; i < bws_srv_get_max_sockets(proto); i++) {
        if (bws_ctx[proto].conn[i].ws == ws &&
            bws_ctx[proto].conn[i].state != BSC_WEBSOCKET_STATE_IDLE) {
            return i;
        }
    }
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static int bws_srv_websocket_direct_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    return bws_srv_websocket_event(
        BSC_WEBSOCKET_DIRECT_PROTOCOL, wsi, reason, user, in, len);
}

static int bws_srv_websocket_hub_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    return bws_srv_websocket_event(
        BSC_WEBSOCKET_HUB_PROTOCOL, wsi, reason, user, in, len);
}

static int bws_srv_websocket_event(BSC_WEBSOCKET_PROTOCOL proto,
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BSC_WEBSOCKET_HANDLE h;
    int written;
    int ret = 0;

    debug_printf("bws_srv_websocket_event() >>> proto = %d, wsi = %p, "
                 "reason = %d, in = %p, len = %d\n",
        proto, wsi, reason, in, len);

    pthread_mutex_lock(bws_ctx[proto].mutex);

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            debug_printf("bws_srv_websocket_event() established connection\n");
            h = bws_srv_alloc_connection(proto);
            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                debug_printf("bws_srv_websocket_event() no free sockets, "
                             "dropping incoming connection\n");
                pthread_mutex_unlock(bws_ctx[proto].mutex);
                return -1;
            }
            debug_printf("bws_srv_websocket_event() proto %d set state of"
                         " socket %d to BACNET_WEBSOCKET_STATE_CONNECTING\n",
                proto, h);
            bws_ctx[proto].conn[h].ws = wsi;
            bws_ctx[proto].conn[h].state = BSC_WEBSOCKET_STATE_CONNECTED;
            bws_ctx[proto].dispatch_func(
                proto, h, BSC_WEBSOCKET_CONNECTED, NULL, 0);
            // wakeup worker to process pending event
            lws_cancel_service(bws_ctx[proto].wsctx);
            break;
        }
        case LWS_CALLBACK_CLOSED: {
            debug_printf("bws_srv_websocket_event() closed connection\n");
            h = bws_find_connnection(proto, wsi);
            if (h != BSC_WEBSOCKET_INVALID_HANDLE) {
                debug_printf("bws_srv_websocket_event() proto %d state of "
                             "socket %d is %d\n",
                    proto, h, bws_ctx[proto].conn[h].state);
                bws_srv_free_connection(proto, h);
                if (!bws_ctx[proto].stop_worker) {
                    bws_ctx[proto].dispatch_func(
                        proto, h, BSC_WEBSOCKET_DISCONNECTED, NULL, 0);
                }
            }
            break;
        }
        case LWS_CALLBACK_RECEIVE: {
            h = bws_find_connnection(proto, wsi);
            if (h != BSC_WEBSOCKET_INVALID_HANDLE) {
                debug_printf(
                    "bws_srv_websocket_event() proto %d received %d bytes of "
                    "data for websocket %d\n",
                    proto, len, h);
                if (!lws_frame_is_binary(wsi)) {
                    // According AB.7.5.3 BACnet/SC BVLC Message Exchange,
                    // if a received data frame is not binary,
                    // the WebSocket connection shall be closed with a
                    // status code of 1003 -WEBSOCKET_DATA_NOT_ACCEPTED.
                    debug_printf("bws_srv_websocket_event() proto %d got "
                                 "non-binary frame, "
                                 "close websocket %d\n",
                        proto, h);
                    lws_close_reason(
                        wsi, LWS_CLOSE_STATUS_UNACCEPTABLE_OPCODE, NULL, 0);
                    pthread_mutex_unlock(bws_ctx[proto].mutex);
                    debug_printf("bws_srv_websocket_event() <<< ret = -1\n");
                    return -1;
                }
                if (bws_ctx[proto].conn[h].state ==
                    BSC_WEBSOCKET_STATE_CONNECTED) {
                    if (!bws_ctx[proto].stop_worker) {
                        bws_ctx[proto].dispatch_func(
                            proto, h, BSC_WEBSOCKET_RECEIVED, in, len);
                    }
                }
            }
            break;
        }
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            debug_printf(
                "bws_srv_websocket_event() proto %d can write\n", proto);
            h = bws_find_connnection(proto, wsi);
            if (h != BSC_WEBSOCKET_INVALID_HANDLE) {
                debug_printf(
                    "bws_srv_websocket_event() proto %d socket %d state = %d\n",
                    proto, h, bws_ctx[proto].conn[h].state);
                if (bws_ctx[proto].conn[h].state ==
                    BSC_WEBSOCKET_STATE_DISCONNECTING) {
                    debug_printf("bws_srv_websocket_event() <<< ret = -1\n");
                    pthread_mutex_unlock(bws_ctx[proto].mutex);
                    return -1;
                } else if (bws_ctx[proto].conn[h].state ==
                        BSC_WEBSOCKET_STATE_CONNECTED &&
                    !bws_ctx[proto].stop_worker &&
                    bws_ctx[proto].conn[h].want_send_data) {
                    bws_ctx[proto].conn[h].can_send_data = true;
                    bws_ctx[proto].dispatch_func(
                        proto, h, BSC_WEBSOCKET_SENDABLE, NULL, 0);
                    bws_ctx[proto].conn[h].want_send_data = false;
                    bws_ctx[proto].conn[h].can_send_data = false;
                    // wakeup worker to process internal state
                    lws_cancel_service(bws_ctx[proto].wsctx);
                } else {
                    bws_ctx[proto].conn[h].want_send_data = false;
                }
            }
            break;
        }
        default: {
            break;
        }
    }
    pthread_mutex_unlock(bws_ctx[proto].mutex);
    debug_printf("bws_srv_websocket_event() <<< ret = %d\n", ret);
    return ret;
}

static void *bws_srv_worker(void *arg)
{
    BSC_WEBSOCKET_PROTOCOL proto =
        bws_srv_proto_by_ctx((BSC_WEBSOCKET_CONTEXT *)arg);
    int i;

    debug_printf("bws_srv_worker() started for proto %d\n", proto);

    pthread_mutex_lock(bws_ctx[proto].mutex);
    bws_ctx[proto].dispatch_func(
        proto, 0, BSC_WEBSOCKET_SERVER_STARTED, NULL, 0);
    pthread_mutex_unlock(bws_ctx[proto].mutex);

    while (1) {
        debug_printf("bws_srv_worker() proto %d blocked\n", proto);
        pthread_mutex_lock(bws_ctx[proto].mutex);

        if (bws_ctx[proto].stop_worker) {
            debug_printf("bws_srv_worker() proto %d going to stop\n", proto);
            lws_context_destroy(bws_ctx[proto].wsctx);
            bws_ctx[proto].wsctx = NULL;
            bws_ctx[proto].stop_worker = false;
            debug_printf(
                "bws_srv_worker() proto %d emitting stop event\n", proto);
            bws_ctx[proto].dispatch_func(
                proto, 0, BSC_WEBSOCKET_SERVER_STOPPED, NULL, 0);
            pthread_mutex_unlock(bws_ctx[proto].mutex);
            debug_printf("bws_srv_worker() proto %d stopped\n", proto);
            break;
        }

        for (i = 0; i < bws_srv_get_max_sockets(proto); i++) {
            debug_printf("bws_srv_worker() proto %d socket %d state = %d\n",
                proto, i, bws_ctx[proto].conn[i].state);
            if (bws_ctx[proto].conn[i].state == BSC_WEBSOCKET_STATE_CONNECTED) {
                if (bws_ctx[proto].conn[i].want_send_data) {
                    debug_printf("bws_srv_worker() process request for sending "
                                 "data on socket %d\n",
                        i);
                    lws_callback_on_writable(bws_ctx[proto].conn[i].ws);
                }
            } else if (bws_ctx[proto].conn[i].state ==
                BSC_WEBSOCKET_STATE_DISCONNECTING) {
                debug_printf("bws_srv_worker() process disconnecting event on "
                             "socket %d\n",
                    i);
                lws_callback_on_writable(bws_ctx[proto].conn[i].ws);
            }
        }

        debug_printf("bws_srv_worker() proto %d unblocked\n", proto);
        pthread_mutex_unlock(bws_ctx[proto].mutex);

        debug_printf(
            "bws_srv_worker() proto %d going to block on lws_service() call\n",
            proto);
        lws_service(bws_ctx[proto].wsctx, 0);
    }

    return NULL;
}

BSC_WEBSOCKET_RET bws_srv_start(BSC_WEBSOCKET_PROTOCOL proto,
    int port,
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size,
    size_t timeout_s,
    BSC_WEBSOCKET_SRV_DISPATCH dispatch_func)
{
    pthread_t thread_id;
    struct lws_context_creation_info info = { 0 };
    int ret;

    debug_printf("bws_srv_start() >>> proto = %d port = %d\n", proto, port);

    if (proto != BSC_WEBSOCKET_HUB_PROTOCOL &&
        proto != BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        debug_printf("bws_srv_start() <<< bad protocol, ret = "
                     "BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (bws_ctx[proto].conn_size == 0) {
        debug_printf(
            "bws_srv_start() <<< too small amount of sockets configured for "
            "server proto %d, ret = BSC_WEBSOCKET_NO_RESOURCES\n",
            proto);
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size ||
        !dispatch_func || !timeout_s) {
        debug_printf("bws_srv_start() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (port < 0 || port > 65535) {
        debug_printf("bws_srv_start() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(bws_ctx[proto].mutex);

    if (bws_ctx[proto].stop_worker) {
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        debug_printf(
            "bws_srv_start() <<< ret = BSC_WEBSOCKET_INVALID_OPERATION\n");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_ctx[proto].wsctx) {
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        debug_printf(
            "bws_srv_start() <<< ret = BSC_WEBSOCKET_INVALID_OPERATION\n");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

#if DEBUG_ENABLED == 1
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG |
            LLL_PARSER | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY |
            LLL_USER | LLL_THREAD,
        NULL);
#else
    lws_set_log_level(0, NULL);
#endif

    info.port = port;

    if (proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
        info.protocols = bws_srv_hub_protos;
    } else if (proto == BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        info.protocols = bws_srv_direct_protos;
    }

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
    bws_ctx[proto].wsctx = lws_create_context(&info);

    if (!bws_ctx[proto].wsctx) {
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        debug_printf("bws_srv_start() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    ret = pthread_create(&thread_id, NULL, &bws_srv_worker, &bws_ctx[proto]);

    if (ret != 0) {
        lws_context_destroy(bws_ctx[proto].wsctx);
        bws_ctx[proto].wsctx = NULL;
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        debug_printf(
            "bws_srv_start() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    bws_ctx[proto].dispatch_func = dispatch_func;
    pthread_mutex_unlock(bws_ctx[proto].mutex);
    debug_printf("bws_srv_start() <<< ret = BSC_WEBSOCKET_SUCCESS\n");
    return BSC_WEBSOCKET_SUCCESS;
}

BSC_WEBSOCKET_RET bws_srv_stop(BSC_WEBSOCKET_PROTOCOL proto)
{
    debug_printf("bws_srv_stop() >>> proto = %d\n", proto);

    if (proto != BSC_WEBSOCKET_HUB_PROTOCOL &&
        proto != BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        debug_printf("bws_srv_disconnect() <<< bad protocol, ret = "
                     "BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(bws_ctx[proto].mutex);

    if (bws_ctx[proto].stop_worker) {
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        debug_printf(
            "bws_srv_stop() <<< ret = BSC_WEBSOCKET_INVALID_OPERATION\n");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_ctx[proto].wsctx) {
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        debug_printf(
            "bws_srv_stop() <<< ret = BSC_WEBSOCKET_INVALID_OPERATION\n");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    bws_ctx[proto].stop_worker = true;
    // wake up libwebsockets runloop
    lws_cancel_service(bws_ctx[proto].wsctx);
    pthread_mutex_unlock(bws_ctx[proto].mutex);

    debug_printf("bws_srv_stop() <<< ret = BSC_WEBSOCKET_SUCCESS\n");
    return BSC_WEBSOCKET_SUCCESS;
}

void bws_srv_disconnect(BSC_WEBSOCKET_PROTOCOL proto, BSC_WEBSOCKET_HANDLE h)
{
    debug_printf("bws_srv_disconnect() >>> proto = %d h = %d\n", proto, h);

    if (proto == BSC_WEBSOCKET_HUB_PROTOCOL ||
        proto == BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        pthread_mutex_lock(bws_ctx[proto].mutex);

        if (h >= 0 && h < bws_srv_get_max_sockets(proto) &&
            !bws_ctx[proto].stop_worker && bws_ctx[proto].wsctx) {
            if (bws_ctx[proto].conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
                // tell worker to process change of connection state
                bws_ctx[proto].conn[h].state =
                    BSC_WEBSOCKET_STATE_DISCONNECTING;
                lws_cancel_service(bws_ctx[proto].wsctx);
            }
        }
        pthread_mutex_unlock(bws_ctx[proto].mutex);
    }

    debug_printf("bws_srv_disconnect() <<<\n");
}

void bws_srv_send(BSC_WEBSOCKET_PROTOCOL proto, BSC_WEBSOCKET_HANDLE h)
{
    debug_printf("bws_srv_send() >>> proto = %d h = %d\n", proto, h);
    if (proto == BSC_WEBSOCKET_HUB_PROTOCOL ||
        proto == BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        pthread_mutex_lock(bws_ctx[proto].mutex);
        if (bws_ctx[proto].conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
            // tell worker to process send request
            bws_ctx[proto].conn[h].want_send_data = true;
            lws_cancel_service(bws_ctx[proto].wsctx);
        }
        pthread_mutex_unlock(bws_ctx[proto].mutex);
    }
    debug_printf("bws_srv_send() <<<\n");
}

BSC_WEBSOCKET_RET bws_srv_dispatch_send(BSC_WEBSOCKET_PROTOCOL proto,
    BSC_WEBSOCKET_HANDLE h,
    uint8_t *payload,
    size_t payload_size)
{
    int written;
    uint8_t *tmp_buf;
    BSC_WEBSOCKET_RET ret;

    debug_printf("bws_srv_dispatch_send() >>> proto = %d h = %d payload %d "
                 "payload_size %d\n",
        proto, h, payload, payload_size);

    if (proto != BSC_WEBSOCKET_HUB_PROTOCOL &&
        proto != BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        debug_printf("bws_srv_dispatch_send() <<< bad protocol, ret = "
                     "BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (h < 0 || h >= bws_srv_get_max_sockets(proto)) {
        debug_printf("bws_srv_dispatch_send() <<< BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (!payload || !payload_size) {
        debug_printf("bws_srv_dispatch_send() <<< BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(bws_ctx[proto].mutex);

    if (bws_ctx[proto].stop_worker) {
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        debug_printf("bws_srv_dispatch_send() <<< ret = "
                     "BSC_WEBSOCKET_INVALID_OPERATION\n");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_ctx[proto].wsctx) {
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        debug_printf("bws_srv_dispatch_send() <<< ret = "
                     "BSC_WEBSOCKET_INVALID_OPERATION\n");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    if ((bws_ctx[proto].conn[h].state != BSC_WEBSOCKET_STATE_CONNECTED) ||
        !bws_ctx[proto].conn[h].want_send_data ||
        !bws_ctx[proto].conn[h].can_send_data) {
        debug_printf(
            "bws_srv_dispatch_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    bws_ctx[proto].conn[h].want_send_data = false;
    bws_ctx[proto].conn[h].can_send_data = false;

    // malloc() and copying is evil, but libwesockets wants some space before
    // actual payload.

    tmp_buf = malloc(payload_size + LWS_PRE);

    if (!tmp_buf) {
        debug_printf(
            "bws_srv_dispatch_send() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        pthread_mutex_unlock(bws_ctx[proto].mutex);
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    memcpy(&tmp_buf[LWS_PRE], payload, payload_size);

    written = lws_write(bws_ctx[proto].conn[h].ws, &tmp_buf[LWS_PRE],
        payload_size, LWS_WRITE_BINARY);

    debug_printf("bws_srv_dispatch_send() %d bytes is sent\n", written);

    if (written < (int)payload_size) {
        debug_printf(
            "bws_srv_dispatch_send() websocket connection is broken(closed)\n");
        // tell worker to process change of connection state
        bws_ctx[proto].conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
        lws_cancel_service(bws_ctx[proto].wsctx);
        ret = BSC_WEBSOCKET_INVALID_OPERATION;
    } else {
        ret = BSC_WEBSOCKET_SUCCESS;
    }

    pthread_mutex_unlock(bws_ctx[proto].mutex);
    debug_printf("bws_srv_dispatch_send() <<< ret = %d\n", ret);
    return ret;
}