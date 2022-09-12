/**
 * @file
 * @brief Implementation of websocket client interface for MAC OS / BSD.
 * @author Kirill Neznamov
 * @date May 2022
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
    "Unsupported version of libwebsockets. Check details here: bacnet_stack/ports/bsd/websocket-cli.c:26"
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
    BSC_WEBSOCKET_STATE_CONNECTING = 1,
    BSC_WEBSOCKET_STATE_CONNECTED = 2,
    BSC_WEBSOCKET_STATE_DISCONNECTING = 3
} BSC_WEBSOCKET_STATE;

typedef struct {
    struct lws_context *ctx;
    struct lws *ws;
    BSC_WEBSOCKET_STATE state;
    bool want_send_data;
    bool can_send_data;
    BSC_WEBSOCKET_CLI_DISPATCH dispatch_func;
    void *user_param;
} BSC_WEBSOCKET_CONNECTION;

// Some forward function declarations

static int bws_cli_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len);

static const char *bws_hub_protocol = BSC_WEBSOCKET_HUB_PROTOCOL_STR;
static const char *bws_direct_protocol = BSC_WEBSOCKET_DIRECT_PROTOCOL_STR;

static pthread_mutex_t bws_cli_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

// Websockets protocol defined in BACnet/SC \S AB.7.1.

static struct lws_protocols bws_cli_direct_protocol[] = {
    { BSC_WEBSOCKET_DIRECT_PROTOCOL_STR, bws_cli_websocket_event, 0, 0, 0, NULL,
        0 },
    LWS_PROTOCOL_LIST_TERM
};

static struct lws_protocols bws_cli_hub_protocol[] = {
    { BSC_WEBSOCKET_HUB_PROTOCOL_STR, bws_cli_websocket_event, 0, 0, 0, NULL,
        0 },
    LWS_PROTOCOL_LIST_TERM
};

static const lws_retry_bo_t retry = {
    .secs_since_valid_ping = 3,
    .secs_since_valid_hangup = 10,
};

static BSC_WEBSOCKET_CONNECTION bws_cli_conn[BSC_CLIENT_WEBSOCKETS_MAX_NUM] = {
    0
};

static BSC_WEBSOCKET_HANDLE bws_cli_alloc_connection(void)
{
    int ret;
    int i;

    for (int i = 0; i < BSC_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_cli_conn[i].state == BSC_WEBSOCKET_STATE_IDLE) {
            memset(&bws_cli_conn[i], 0, sizeof(bws_cli_conn[i]));
            return i;
        }
    }
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static void bws_cli_free_connection(BSC_WEBSOCKET_HANDLE h)
{
    if (h >= 0 && h < BSC_CLIENT_WEBSOCKETS_MAX_NUM) {
        if (bws_cli_conn[h].state != BSC_WEBSOCKET_STATE_IDLE) {
            bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_IDLE;
            bws_cli_conn[h].ws = NULL;
        }
    }
}

static BSC_WEBSOCKET_HANDLE bws_cli_find_connnection(struct lws *ws)
{
    for (int i = 0; i < BSC_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_cli_conn[i].ws == ws &&
            bws_cli_conn[i].state != BSC_WEBSOCKET_STATE_IDLE) {
            return i;
        }
    }
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static int bws_cli_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BSC_WEBSOCKET_HANDLE h;
    int written;

    debug_printf("bws_cli_websocket_event() >>> reason = %d\n", reason);
    pthread_mutex_lock(&bws_cli_mutex);
    h = bws_cli_find_connnection(wsi);

    if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
        debug_printf("bws_cli_websocket_event() can not find websocket handle "
                     "for wsi %p\n",
            wsi);
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_websocket_event() <<< ret = 0\n");
        return 0;
    }

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: {
            debug_printf("bws_cli_websocket_event() connection established\n");
            bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_CONNECTED;
            bws_cli_conn[h].dispatch_func(h, BSC_WEBSOCKET_CONNECTED, NULL, 0,
                bws_cli_conn[h].user_param);
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            debug_printf(
                "bws_cli_websocket_event() received %d bytes of data\n", len);
            if (!lws_frame_is_binary(wsi)) {
                // According AB.7.5.3 BACnet/SC BVLC Message Exchange,
                // if a received data frame is not binary,
                // the WebSocket connection shall be closed with a
                // status code of 1003 -WEBSOCKET_DATA_NOT_ACCEPTED.
                debug_printf("bws_cli_websocket_event() got non-binary frame, "
                             "close connection for socket %d\n",
                    h);
                lws_close_reason(
                    wsi, LWS_CLOSE_STATUS_UNACCEPTABLE_OPCODE, NULL, 0);
                pthread_mutex_unlock(&bws_cli_mutex);
                debug_printf("bws_cli_websocket_event() <<< ret = -1\n");
                return -1;
            }
            if (bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
                bws_cli_conn[h].dispatch_func(h, BSC_WEBSOCKET_RECEIVED,
                    (uint8_t *)in, len, bws_cli_conn[h].user_param);
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            debug_printf("bws_cli_websocket_event() can write\n");
            if (bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED &&
                bws_cli_conn[h].want_send_data) {
                bws_cli_conn[h].can_send_data = true;
                bws_cli_conn[h].dispatch_func(h, BSC_WEBSOCKET_SENDABLE, NULL,
                    0, bws_cli_conn[h].user_param);
                bws_cli_conn[h].want_send_data = false;
                bws_cli_conn[h].can_send_data = false;
                // wakeup worker to process internal state
                lws_cancel_service(bws_cli_conn[h].ctx);
            } else {
                bws_cli_conn[h].want_send_data = false;
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
            bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
            // wakeup worker to process pending event
            lws_cancel_service(bws_cli_conn[h].ctx);
            break;
        }
        default: {
            break;
        }
    }
    pthread_mutex_unlock(&bws_cli_mutex);
    debug_printf("bws_cli_websocket_event() <<< ret = 0\n");
    return 0;
}

static void *bws_cli_worker(void *arg)
{
    BSC_WEBSOCKET_HANDLE h = *((int *)arg);
    BSC_WEBSOCKET_CONNECTION *conn = &bws_cli_conn[h];

    while (1) {
        debug_printf("bws_cli_worker() lock mutex\n");
        pthread_mutex_lock(&bws_cli_mutex);
        if (bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
            if (bws_cli_conn[h].want_send_data) {
                debug_printf(
                    "bws_cli_worker() process request for sending data\n");
                lws_callback_on_writable(conn->ws);
            }
        } else if (bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_DISCONNECTING) {
            debug_printf("bws_cli_worker() process disconnecting event\n");
            lws_context_destroy(conn->ctx);
            bws_cli_free_connection(h);
            bws_cli_conn[h].dispatch_func(h, BSC_WEBSOCKET_DISCONNECTED, NULL,
                0, bws_cli_conn[h].user_param);
            pthread_mutex_unlock(&bws_cli_mutex);
            break;
        }
        debug_printf("bws_cli_worker() unlock mutex\n");
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_worker() going to block on lws_service() call\n");
        lws_service(conn->ctx, 0);
    }

    return NULL;
}

BSC_WEBSOCKET_RET bws_cli_connect(BSC_WEBSOCKET_PROTOCOL proto,
    char *url,
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size,
    size_t timeout_s,
    BSC_WEBSOCKET_CLI_DISPATCH dispatch_func,
    void *dispatch_func_user_param,
    BSC_WEBSOCKET_HANDLE *out_handle)
{
    struct lws_context_creation_info info = { 0 };
    char tmp_url[BSC_WSURL_MAX_LEN];
    const char *prot = NULL, *addr = NULL, *path = NULL;
    int port = -1;
    BSC_WEBSOCKET_HANDLE h;
    struct lws_client_connect_info cinfo = { 0 };
    BSC_WEBSOCKET_RET ret;
    pthread_t thread_id;

    debug_printf("bws_cli_connect() >>> proto = %d, url = %s\n", proto, url);

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size ||
        !url || !out_handle || !timeout_s || !dispatch_func) {
        debug_printf("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    *out_handle = BSC_WEBSOCKET_INVALID_HANDLE;

    if (proto != BSC_WEBSOCKET_HUB_PROTOCOL &&
        proto != BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        debug_printf("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    strncpy(tmp_url, url, BSC_WSURL_MAX_LEN);

    pthread_mutex_lock(&bws_cli_mutex);

#if DEBUG_ENABLED == 1
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG |
            LLL_PARSER | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY |
            LLL_USER | LLL_THREAD,
        NULL);
#else
    lws_set_log_level(0, NULL);
#endif

    ret = lws_parse_uri(tmp_url, &prot, &addr, &port, &path);

    if (port == -1 || !prot || !addr || !path) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (strcmp(prot, "wss") != 0) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    h = bws_cli_alloc_connection();

    if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_connect() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }
    bws_cli_conn[h].dispatch_func = dispatch_func;
    bws_cli_conn[h].user_param = dispatch_func_user_param;
    info.port = CONTEXT_PORT_NO_LISTEN;
    if (proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
        info.protocols = bws_cli_hub_protocol;
    } else {
        info.protocols = bws_cli_direct_protocol;
    }
    info.gid = -1;
    info.uid = -1;
    info.client_ssl_cert_mem = cert;
    info.client_ssl_cert_mem_len = cert_size;
    info.client_ssl_ca_mem = ca_cert;
    info.client_ssl_ca_mem_len = ca_cert_size;
    info.client_ssl_key_mem = key;
    info.client_ssl_key_mem_len = key_size;
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.timeout_secs = timeout_s;
    info.connect_timeout_secs = timeout_s;
    bws_cli_conn[h].ctx = lws_create_context(&info);

    if (!bws_cli_conn[h].ctx) {
        bws_cli_free_connection(h);
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_connect() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    ret = pthread_create(&thread_id, NULL, &bws_cli_worker, &h);

    if (ret != 0) {
        bws_cli_free_connection(h);
        lws_context_destroy(bws_cli_conn[h].ctx);
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_connect() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    cinfo.context = bws_cli_conn[h].ctx;
    cinfo.address = addr;
    cinfo.origin = cinfo.address;
    cinfo.host = cinfo.address;
    cinfo.port = port;
    cinfo.path = path;
    cinfo.pwsi = &bws_cli_conn[h].ws;
    cinfo.alpn = "h2;http/1.1";
    cinfo.retry_and_idle_policy = &retry;
    cinfo.ssl_connection = LCCSCF_USE_SSL |
        LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK | LCCSCF_ALLOW_SELFSIGNED;

    if (proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
        cinfo.protocol = bws_hub_protocol;
    } else {
        cinfo.protocol = bws_direct_protocol;
    }

    bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_CONNECTING;
    *out_handle = h;
    lws_client_connect_via_info(&cinfo);
    pthread_mutex_unlock(&bws_cli_mutex);

    debug_printf("bws_cli_connect() <<< ret = %d\n", BSC_WEBSOCKET_SUCCESS);
    return ret;
}

void bws_cli_disconnect(BSC_WEBSOCKET_HANDLE h)
{
    debug_printf("bws_cli_disconnect() >>> h = %d\n", h);

    if (h >= 0 && h < BSC_CLIENT_WEBSOCKETS_MAX_NUM) {
        pthread_mutex_lock(&bws_cli_mutex);

        if (bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTING ||
            bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
            // tell worker to process change of connection state
            bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
            lws_cancel_service(bws_cli_conn[h].ctx);
        }

        pthread_mutex_unlock(&bws_cli_mutex);
    }

    debug_printf("bws_cli_disconnect() <<<\n");
}

void bws_cli_send(BSC_WEBSOCKET_HANDLE h)
{
    debug_printf("bws_cli_send() >>> h = %d\n", h);

    if (h >= 0 && h < BSC_CLIENT_WEBSOCKETS_MAX_NUM) {
        pthread_mutex_lock(&bws_cli_mutex);

        if (bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
            // tell worker to process send request
            bws_cli_conn[h].want_send_data = true;
            lws_cancel_service(bws_cli_conn[h].ctx);
        }

        pthread_mutex_unlock(&bws_cli_mutex);
    }

    debug_printf("bws_cli_send() <<<\n");
}

BSC_WEBSOCKET_RET bws_cli_dispatch_send(
    BSC_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size)
{
    int written;
    uint8_t *tmp_buf;
    BSC_WEBSOCKET_RET ret;

    debug_printf(
        "bws_cli_dispatch_send() >>> h = %d, payload = %p, payload_size = %d\n",
        h, payload, payload_size);

    if (h < 0 || h >= BSC_CLIENT_WEBSOCKETS_MAX_NUM) {
        debug_printf(
            "bws_cli_dispatch_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_cli_mutex);

    if ((bws_cli_conn[h].state != BSC_WEBSOCKET_STATE_CONNECTED) ||
        !bws_cli_conn[h].want_send_data || !bws_cli_conn[h].can_send_data) {
        debug_printf(
            "bws_cli_dispatch_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        pthread_mutex_unlock(&bws_cli_mutex);
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    bws_cli_conn[h].want_send_data = false;
    bws_cli_conn[h].can_send_data = false;

    // malloc() and copying is evil, but libwesockets wants some space before
    // actual payload.

    tmp_buf = malloc(payload_size + LWS_PRE);

    if (!tmp_buf) {
        debug_printf(
            "bws_cli_dispatch_send() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        pthread_mutex_unlock(&bws_cli_mutex);
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    memcpy(&tmp_buf[LWS_PRE], payload, payload_size);

    written = lws_write(
        bws_cli_conn[h].ws, &tmp_buf[LWS_PRE], payload_size, LWS_WRITE_BINARY);

    debug_printf("bws_cli_dispatch_send() %d bytes is sent\n", written);

    if (written < (int)payload_size) {
        debug_printf(
            "bws_cli_dispatch_send() websocket connection is broken(closed)\n");
        // tell worker to process change of connection state
        bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
        lws_cancel_service(bws_cli_conn[h].ctx);
        ret = BSC_WEBSOCKET_INVALID_OPERATION;
    } else {
        ret = BSC_WEBSOCKET_SUCCESS;
    }

    pthread_mutex_unlock(&bws_cli_mutex);
    free(tmp_buf);
    debug_printf("bws_cli_dispatch_send() <<< ret = %d\n", ret);
    return ret;
}