/**
 * @file
 * @brief Implementation of websocket client interface.
 * @author Kirill Neznamov
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#define _GNU_SOURCE
#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/basic/sys/debug.h"
#include "websocket-mutex.h"

#define DEBUG_WEBSOCKET_CLIENT 0

#if DEBUG_WEBSOCKET_CLIENT == 1
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF(...)
#endif

#ifndef LWS_PROTOCOL_LIST_TERM
#define LWS_PROTOCOL_LIST_TERM       \
    {                                \
        NULL, NULL, 0, 0, 0, NULL, 0 \
    }
#endif

#define BSC_INITIAL_BUFFER_LEN 512

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
    uint8_t *fragment_buffer;
    size_t fragment_buffer_size;
    int fragment_buffer_len;
    char err_desc[BSC_WEBSOCKET_ERR_DESC_STR_MAX_LEN];
    BACNET_ERROR_CODE err_code;
} BSC_WEBSOCKET_CONNECTION;

/* Some forward function declarations */

static int bws_cli_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len);

static const char *bws_hub_protocol = BSC_WEBSOCKET_HUB_PROTOCOL_STR;
static const char *bws_direct_protocol = BSC_WEBSOCKET_DIRECT_PROTOCOL_STR;

static pthread_mutex_t bws_cli_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* Websockets protocol defined in BACnet/SC \S AB.7.1.  */

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

static lws_retry_bo_t bws_retry;

static BSC_WEBSOCKET_CONNECTION bws_cli_conn[BSC_CLIENT_WEBSOCKETS_MAX_NUM] = {
    0
};

static BSC_WEBSOCKET_HANDLE bws_cli_alloc_connection(void)
{
    int i;

    for (i = 0; i < BSC_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
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
            if (bws_cli_conn[h].fragment_buffer) {
                free(bws_cli_conn[h].fragment_buffer);
                bws_cli_conn[h].fragment_buffer_len = 0;
                bws_cli_conn[h].fragment_buffer_size = 0;
            }
            bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_IDLE;
            bws_cli_conn[h].ws = NULL;
            bws_cli_conn[h].ctx = NULL;
        }
    }
}

static BSC_WEBSOCKET_HANDLE bws_cli_find_connnection(struct lws *ws)
{
    int i;
    for (i = 0; i < BSC_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_cli_conn[i].ws == ws &&
            bws_cli_conn[i].state != BSC_WEBSOCKET_STATE_IDLE) {
            return i;
        }
    }
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static void bws_set_err_desc(BSC_WEBSOCKET_HANDLE h, char *err_desc)
{
    int len;
    if (bws_cli_conn[h].err_code == ERROR_CODE_SUCCESS) {
        len = strlen(err_desc) >= sizeof(bws_cli_conn[h].err_desc)
            ? sizeof(bws_cli_conn[h].err_desc) - 1
            : strlen(err_desc);

        memcpy(bws_cli_conn[h].err_desc, err_desc, len);
        bws_cli_conn[h].err_desc[len] = 0;

        if (strstr(err_desc, "tls:")) {
            bws_cli_conn[h].err_code = ERROR_CODE_TLS_ERROR;
        } else {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_ERROR;
        }
    }
}

static void bws_set_disconnect_reason(BSC_WEBSOCKET_HANDLE h, uint16_t err_code)
{
    bws_cli_conn[h].err_desc[0] = 0;
    switch (err_code) {
        case LWS_CLOSE_STATUS_NORMAL: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER;
            break;
        }
        case LWS_CLOSE_STATUS_GOINGAWAY: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_ENDPOINT_LEAVES;
            break;
        }
        case LWS_CLOSE_STATUS_PROTOCOL_ERR: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_PROTOCOL_ERROR;
            break;
        }
        case LWS_CLOSE_STATUS_UNACCEPTABLE_OPCODE: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_DATA_NOT_ACCEPTED;
            break;
        }
        case LWS_CLOSE_STATUS_NO_STATUS:
        case LWS_CLOSE_STATUS_RESERVED:
        {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_ERROR;
            break;
        }
        case LWS_CLOSE_STATUS_ABNORMAL_CLOSE: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_DATA_NOT_ACCEPTED;
            break;
        }
        case LWS_CLOSE_STATUS_INVALID_PAYLOAD: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_DATA_INCONSISTENT;
            break;
        }
        case LWS_CLOSE_STATUS_POLICY_VIOLATION: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_DATA_AGAINST_POLICY;
            break;
        }
        case LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_FRAME_TOO_LONG;
            break;
        }
        case LWS_CLOSE_STATUS_EXTENSION_REQUIRED: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_EXTENSION_MISSING;
            break;
        }
        case LWS_CLOSE_STATUS_UNEXPECTED_CONDITION: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_REQUEST_UNAVAILABLE;
            break;
        }
        default: {
            bws_cli_conn[h].err_code = ERROR_CODE_WEBSOCKET_ERROR;
            break;
        }
    }
}

static int bws_cli_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BSC_WEBSOCKET_HANDLE h;
    BSC_WEBSOCKET_CLI_DISPATCH dispatch_func;
    void *user_param;
    uint8_t err_code[2];

    (void)user;

    DEBUG_PRINTF(
        "bws_cli_websocket_event() >>> reason = %d, user = %p, in = %p\n",
        reason, user, in);

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: {
            pthread_mutex_lock(&bws_cli_mutex);
            h = bws_cli_find_connnection(wsi);

            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                DEBUG_PRINTF(
                    "bws_cli_websocket_event() can not find websocket handle "
                    "for wsi %p\n",
                    wsi);
                pthread_mutex_unlock(&bws_cli_mutex);
                DEBUG_PRINTF("bws_cli_websocket_event() <<< ret = 0\n");
                return 0;
            }

            DEBUG_PRINTF("bws_cli_websocket_event() connection established\n");
            bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_CONNECTED;
            dispatch_func = bws_cli_conn[h].dispatch_func;
            user_param = bws_cli_conn[h].user_param;
            pthread_mutex_unlock(&bws_cli_mutex);
            dispatch_func(
                h, BSC_WEBSOCKET_CONNECTED, 0, NULL, NULL, 0, user_param);
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            pthread_mutex_lock(&bws_cli_mutex);
            h = bws_cli_find_connnection(wsi);

            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                DEBUG_PRINTF(
                    "bws_cli_websocket_event() can not find websocket handle "
                    "for wsi %p\n",
                    wsi);
                pthread_mutex_unlock(&bws_cli_mutex);
                DEBUG_PRINTF("bws_cli_websocket_event() <<< ret = 0\n");
                return 0;
            }
            DEBUG_PRINTF(
                "bws_cli_websocket_event() received %d bytes of data\n", len);
            if (!lws_frame_is_binary(wsi)) {
                /* According AB.7.5.3 BACnet/SC BVLC Message Exchange,
                   if a received data frame is not binary,
                   the WebSocket connection shall be closed with a
                   status code of 1003 -WEBSOCKET_DATA_NOT_ACCEPTED.
                */
                DEBUG_PRINTF("bws_cli_websocket_event() got non-binary frame, "
                             "close connection for socket %d\n",
                    h);
                lws_close_reason(
                    wsi, LWS_CLOSE_STATUS_UNACCEPTABLE_OPCODE, NULL, 0);
                pthread_mutex_unlock(&bws_cli_mutex);
                DEBUG_PRINTF("bws_cli_websocket_event() <<< ret = -1\n");
                return -1;
            }
            if (bws_cli_conn[h].state != BSC_WEBSOCKET_STATE_CONNECTED) {
                pthread_mutex_unlock(&bws_cli_mutex);
            } else {
                if (!bws_cli_conn[h].fragment_buffer) {
                    DEBUG_PRINTF("bws_cli_websocket_event() alloc %d bytes for "
                                 "socket %d\n",
                        len, h);
                    bws_cli_conn[h].fragment_buffer = malloc(
                        len <= BSC_INITIAL_BUFFER_LEN ? BSC_INITIAL_BUFFER_LEN
                                                      : len);
                    if (!bws_cli_conn[h].fragment_buffer) {
                        lws_close_reason(
                            wsi, LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE, NULL, 0);
                        pthread_mutex_unlock(&bws_cli_mutex);
                        DEBUG_PRINTF("bws_cli_websocket_event() <<< ret = -1, "
                                     "allocation of %d bytes failed\n",
                            len <= BSC_INITIAL_BUFFER_LEN
                                ? BSC_INITIAL_BUFFER_LEN
                                : len);
                        return -1;
                    }
                    bws_cli_conn[h].fragment_buffer_len = 0;
                    bws_cli_conn[h].fragment_buffer_size = len;
                }
                if (bws_cli_conn[h].fragment_buffer_len + len >
                    bws_cli_conn[h].fragment_buffer_size) {
                    DEBUG_PRINTF("bws_cli_websocket_event() realloc %d bytes "
                                 "for socket %d\n",
                        bws_cli_conn[h].fragment_buffer_len + len, h);
                    bws_cli_conn[h].fragment_buffer =
                        realloc(bws_cli_conn[h].fragment_buffer,
                            bws_cli_conn[h].fragment_buffer_len + len);
                    if (!bws_cli_conn[h].fragment_buffer) {
                        lws_close_reason(
                            wsi, LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE, NULL, 0);
                        pthread_mutex_unlock(&bws_cli_mutex);
                        DEBUG_PRINTF("bws_cli_websocket_event() <<< ret = -1, "
                                     "re-allocation of %d bytes failed\n",
                            bws_cli_conn[h].fragment_buffer_len + len);
                        return -1;
                    }
                    bws_cli_conn[h].fragment_buffer_size =
                        bws_cli_conn[h].fragment_buffer_len + len;
                }
                DEBUG_PRINTF("bws_cli_websocket_event() got next %d bytes for "
                             "socket %d\n",
                    len, h);
                memcpy(
                    &bws_cli_conn[h]
                         .fragment_buffer[bws_cli_conn[h].fragment_buffer_len],
                    in, len);
                bws_cli_conn[h].fragment_buffer_len += len;

                if (lws_is_final_fragment(wsi)) {
                    DEBUG_PRINTF(
                        "bws_cli_websocket_event() last fragment received\n");
                    dispatch_func = bws_cli_conn[h].dispatch_func;
                    user_param = bws_cli_conn[h].user_param;
                    pthread_mutex_unlock(&bws_cli_mutex);
                    dispatch_func(h, BSC_WEBSOCKET_RECEIVED, 0, NULL,
                        bws_cli_conn[h].fragment_buffer,
                        bws_cli_conn[h].fragment_buffer_len, user_param);
                    pthread_mutex_lock(&bws_cli_mutex);
                    bws_cli_conn[h].fragment_buffer_len = 0;
                    pthread_mutex_unlock(&bws_cli_mutex);
                } else {
                    pthread_mutex_unlock(&bws_cli_mutex);
                }
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            pthread_mutex_lock(&bws_cli_mutex);
            h = bws_cli_find_connnection(wsi);

            if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
                DEBUG_PRINTF(
                    "bws_cli_websocket_event() can not find websocket handle "
                    "for wsi %p\n",
                    wsi);
                pthread_mutex_unlock(&bws_cli_mutex);
                DEBUG_PRINTF("bws_cli_websocket_event() <<< ret = 0\n");
                return 0;
            }

            DEBUG_PRINTF("bws_cli_websocket_event() can write, state = %d\n",
                bws_cli_conn[h].state);
            DEBUG_PRINTF("bws_cli_websocket_event() ws = %d, cs = %d\n",
                bws_cli_conn[h].want_send_data, bws_cli_conn[h].can_send_data);
            if (bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED &&
                bws_cli_conn[h].want_send_data) {
                bws_cli_conn[h].can_send_data = true;
                dispatch_func = bws_cli_conn[h].dispatch_func;
                user_param = bws_cli_conn[h].user_param;
                pthread_mutex_unlock(&bws_cli_mutex);
                dispatch_func(
                    h, BSC_WEBSOCKET_SENDABLE, 0, NULL, NULL, 0, user_param);
                pthread_mutex_lock(&bws_cli_mutex);
                bws_cli_conn[h].want_send_data = false;
                bws_cli_conn[h].can_send_data = false;
                DEBUG_PRINTF(
                    "bws_cli_websocket_event() was send , ws = %d, cs = %d\n",
                    bws_cli_conn[h].want_send_data,
                    bws_cli_conn[h].can_send_data);
                pthread_mutex_unlock(&bws_cli_mutex);
                /* wakeup worker to process internal state */
                lws_cancel_service(bws_cli_conn[h].ctx);
            } else {
                bws_cli_conn[h].want_send_data = false;
                DEBUG_PRINTF(
                    "bws_cli_websocket_event() no send, ws = %d, cs = %d\n",
                    bws_cli_conn[h].want_send_data,
                    bws_cli_conn[h].can_send_data);
                pthread_mutex_unlock(&bws_cli_mutex);
            }
            break;
        }
        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: {
            pthread_mutex_lock(&bws_cli_mutex);
            h = bws_cli_find_connnection(wsi);
            if (h != BSC_WEBSOCKET_INVALID_HANDLE && len >= 2) {
                err_code[0] = ((uint8_t *)in)[1];
                err_code[1] = ((uint8_t *)in)[0];
                bws_set_disconnect_reason(h, *((uint16_t *)&err_code));
            }
            pthread_mutex_unlock(&bws_cli_mutex);
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
            pthread_mutex_lock(&bws_cli_mutex);
            h = bws_cli_find_connnection(wsi);
            if (h != BSC_WEBSOCKET_INVALID_HANDLE) {
                bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
                if (reason == LWS_CALLBACK_CLIENT_CONNECTION_ERROR && in) {
                    bws_set_err_desc(h, (char *)in);
                }
                pthread_mutex_unlock(&bws_cli_mutex);
                /* wakeup worker to process pending event */
                lws_cancel_service(bws_cli_conn[h].ctx);
            } else {
                pthread_mutex_unlock(&bws_cli_mutex);
            }
            break;
        }
        default: {
            break;
        }
    }
    DEBUG_PRINTF("bws_cli_websocket_event() <<< ret = 0\n");
    return 0;
}

static void *bws_cli_worker(void *arg)
{
    BSC_WEBSOCKET_CONNECTION *conn = (BSC_WEBSOCKET_CONNECTION *)arg;
    BSC_WEBSOCKET_HANDLE h = conn - &bws_cli_conn[0];
    BSC_WEBSOCKET_CLI_DISPATCH dispatch_func;
    void *user_param;
    char err_desc[BSC_WEBSOCKET_ERR_DESC_STR_MAX_LEN];
    uint16_t err_code;

    while (1) {
        DEBUG_PRINTF("bws_cli_worker() try mutex h = %d\n", h);
        pthread_mutex_lock(&bws_cli_mutex);
        DEBUG_PRINTF("bws_cli_worker() mutex locked h = %d\n", h);
        if (conn->state == BSC_WEBSOCKET_STATE_CONNECTED) {
            if (conn->want_send_data) {
                DEBUG_PRINTF(
                    "bws_cli_worker() process request for sending data\n");
                lws_callback_on_writable(conn->ws);
            }
        } else if (conn->state == BSC_WEBSOCKET_STATE_DISCONNECTING) {
            DEBUG_PRINTF("bws_cli_worker() process disconnecting event\n");
            DEBUG_PRINTF("bws_cli_worker() destroy ctx %p\n", conn->ctx);
            /* TRICKY: This is ridiculus but lws_context_destroy()
                       does't seem to be
                       thread safe. More over, on different platforms the
                       function behaves in different ways. Call of
                       lws_context_destroy() leads to several calls of
                       bws_cli_websocket_event() callback (LWS_CALLBACK_CLOSED,
                       etc..). But under some OS (MacOSx) that callback is
                       called from context of the bws_cli_worker() thread and
                       under some other OS (linux) the callback is called from
                       internal libwebsockets lib thread. That's why
                       bws_cli_mutex must be unlocked before
                       lws_context_destroy() call. To ensure that nobody calls
                       lws_context_destroy() from some parallel thread it is
                       protected by global websocket mutex.
            */
            pthread_mutex_unlock(&bws_cli_mutex);
            bsc_websocket_global_lock();
            lws_context_destroy(conn->ctx);
            bsc_websocket_global_unlock();
            pthread_mutex_lock(&bws_cli_mutex);
            dispatch_func = conn->dispatch_func;
            user_param = conn->user_param;
            err_code = conn->err_code;
            if (err_code != ERROR_CODE_SUCCESS) {
                memcpy(err_desc, conn->err_desc, sizeof(err_desc));
            }
            bws_cli_free_connection(h);
            pthread_mutex_unlock(&bws_cli_mutex);
            DEBUG_PRINTF("bws_cli_worker() unlock mutex\n");
            dispatch_func(h, BSC_WEBSOCKET_DISCONNECTED, err_code,
                err_code != ERROR_CODE_SUCCESS ? err_desc : NULL, NULL, 0, user_param);
            return NULL;
        }
        DEBUG_PRINTF("bws_cli_worker() unlock mutex\n");
        pthread_mutex_unlock(&bws_cli_mutex);
        DEBUG_PRINTF("bws_cli_worker() going to block on lws_service() call\n");
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
    int len;
    int st;

    DEBUG_PRINTF("bws_cli_connect() >>> proto = %d, url = %s\n", proto, url);

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size ||
        !url || !out_handle || !timeout_s || !dispatch_func) {
        DEBUG_PRINTF("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    *out_handle = BSC_WEBSOCKET_INVALID_HANDLE;

    if (proto != BSC_WEBSOCKET_HUB_PROTOCOL &&
        proto != BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        DEBUG_PRINTF("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    len = strlen(url) >= sizeof(tmp_url) ? sizeof(tmp_url) - 1 : strlen(url);
    memcpy(tmp_url, url, len);
    tmp_url[len] = 0;

    bsc_websocket_global_lock();
#if DEBUG_ENABLED == 1
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG |
            LLL_PARSER | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY |
            LLL_USER | LLL_THREAD,
        NULL);
#else
    lws_set_log_level(0, NULL);
#endif
    bsc_websocket_global_unlock();

    pthread_mutex_lock(&bws_cli_mutex);
    st = lws_parse_uri(tmp_url, &prot, &addr, &port, &path);

    if (st || port == -1 || !prot || !addr || !path) {
        pthread_mutex_unlock(&bws_cli_mutex);
        DEBUG_PRINTF("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (strcmp(prot, "wss") != 0) {
        pthread_mutex_unlock(&bws_cli_mutex);
        DEBUG_PRINTF("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    h = bws_cli_alloc_connection();

    if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
        pthread_mutex_unlock(&bws_cli_mutex);
        DEBUG_PRINTF(
            "bws_cli_connect() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }
    bws_cli_conn[h].fragment_buffer = NULL;
    bws_cli_conn[h].fragment_buffer_len = 0;
    bws_cli_conn[h].fragment_buffer_size = 0;
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
    info.options |= LWS_SERVER_OPTION_FAIL_UPON_UNABLE_TO_BIND;
    info.timeout_secs = timeout_s;
    info.connect_timeout_secs = timeout_s;

    /* TRICKY: check comments related to lws_context_destroy() call */

    pthread_mutex_unlock(&bws_cli_mutex);
    bsc_websocket_global_lock();
    bws_cli_conn[h].ctx = lws_create_context(&info);
    bsc_websocket_global_unlock();
    pthread_mutex_lock(&bws_cli_mutex);
    DEBUG_PRINTF("bws_cli_connect() created ctx %p\n", bws_cli_conn[h].ctx);

    if (!bws_cli_conn[h].ctx) {
        bws_cli_free_connection(h);
        pthread_mutex_unlock(&bws_cli_mutex);
        DEBUG_PRINTF(
            "bws_cli_connect() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    ret = pthread_create(&thread_id, NULL, &bws_cli_worker, &bws_cli_conn[h]);

    if (ret != 0) {
        /* TRICKY: This is ridiculus but lws_context_destroy()
                   does't seem to be
                   thread safe. More over, on different platforms the
                   function behaves in different ways. Call of
                   lws_context_destroy() leads to several calls of
                   bws_cli_websocket_event() callback (LWS_CALLBACK_CLOSED,
                   etc..). But under some OS (MacOSx) that callback is
                   called from context of the bws_cli_worker() thread and
                   under some other OS (linux) the callback is called from
                   internal libwebsockets lib thread. That's why
                   bws_cli_mutex must be unlocked before
                   lws_context_destroy() call. To ensure that nobody calls
                   lws_context_destroy() from some parallel thread it is
                   protected by global websocket mutex.
        */
        pthread_mutex_unlock(&bws_cli_mutex);
        bsc_websocket_global_lock();
        lws_context_destroy(bws_cli_conn[h].ctx);
        bsc_websocket_global_unlock();
        pthread_mutex_lock(&bws_cli_mutex);
        bws_cli_free_connection(h);
        pthread_mutex_unlock(&bws_cli_mutex);
        DEBUG_PRINTF(
            "bws_cli_connect() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }
    bws_cli_conn[h].ws = NULL;
    cinfo.context = bws_cli_conn[h].ctx;
    cinfo.address = addr;
    cinfo.origin = cinfo.address;
    cinfo.host = cinfo.address;
    cinfo.port = port;
    cinfo.path = path;
    cinfo.pwsi = &bws_cli_conn[h].ws;
    cinfo.alpn = "h2;http/1.1";
    bws_retry.secs_since_valid_ping = 3;
    bws_retry.secs_since_valid_hangup = 10;
    cinfo.retry_and_idle_policy = &bws_retry;
    cinfo.ssl_connection = LCCSCF_USE_SSL |
        LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK | LCCSCF_ALLOW_SELFSIGNED;

    if (proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
        cinfo.protocol = bws_hub_protocol;
    } else {
        cinfo.protocol = bws_direct_protocol;
    }

    bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_CONNECTING;
    bws_cli_conn[h].err_code = ERROR_CODE_SUCCESS;
    *out_handle = h;
    lws_client_connect_via_info(&cinfo);
    pthread_mutex_unlock(&bws_cli_mutex);

    DEBUG_PRINTF("bws_cli_connect() <<< ret = %d\n", BSC_WEBSOCKET_SUCCESS);
    return ret;
}

void bws_cli_disconnect(BSC_WEBSOCKET_HANDLE h)
{
    DEBUG_PRINTF("bws_cli_disconnect() >>> h = %d\n", h);

    if (h >= 0 && h < BSC_CLIENT_WEBSOCKETS_MAX_NUM) {
        pthread_mutex_lock(&bws_cli_mutex);

        if (bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTING ||
            bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
            /* tell worker to process change of connection state */
            bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
            lws_cancel_service(bws_cli_conn[h].ctx);
        }

        pthread_mutex_unlock(&bws_cli_mutex);
    }

    DEBUG_PRINTF("bws_cli_disconnect() <<<\n");
}

void bws_cli_send(BSC_WEBSOCKET_HANDLE h)
{
    DEBUG_PRINTF("bws_cli_send() >>> h = %d\n", h);

    if (h >= 0 && h < BSC_CLIENT_WEBSOCKETS_MAX_NUM) {
        pthread_mutex_lock(&bws_cli_mutex);

        if (bws_cli_conn[h].state == BSC_WEBSOCKET_STATE_CONNECTED) {
            /* tell worker to process send request */
            bws_cli_conn[h].want_send_data = true;
            DEBUG_PRINTF("bws_cli_send() cs = 1\n");
            lws_cancel_service(bws_cli_conn[h].ctx);
        }

        pthread_mutex_unlock(&bws_cli_mutex);
    }

    DEBUG_PRINTF("bws_cli_send() <<<\n");
}

BSC_WEBSOCKET_RET bws_cli_dispatch_send(
    BSC_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size)
{
    int written;
    uint8_t *tmp_buf;
    BSC_WEBSOCKET_RET ret;

    DEBUG_PRINTF(
        "bws_cli_dispatch_send() >>> h = %d, payload = %p, payload_size = %d\n",
        h, payload, payload_size);

    if (h < 0 || h >= BSC_CLIENT_WEBSOCKETS_MAX_NUM) {
        DEBUG_PRINTF(
            "bws_cli_dispatch_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_cli_mutex);

    if ((bws_cli_conn[h].state != BSC_WEBSOCKET_STATE_CONNECTED) ||
        !bws_cli_conn[h].want_send_data || !bws_cli_conn[h].can_send_data) {
        DEBUG_PRINTF("bws_cli_dispatch_send() state = %d, ws = %d, cs = %d\n",
            bws_cli_conn[h].state, bws_cli_conn[h].want_send_data,
            bws_cli_conn[h].can_send_data);
        DEBUG_PRINTF("bws_cli_dispatch_send() <<< ret = "
                     "BSC_WEBSOCKET_INVALID_OPERATION\n");
        pthread_mutex_unlock(&bws_cli_mutex);
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    /* malloc() and copying is evil, but libwesockets wants some space before
       actual payload.
    */

    tmp_buf = malloc(payload_size + LWS_PRE);

    if (!tmp_buf) {
        DEBUG_PRINTF(
            "bws_cli_dispatch_send() <<< ret = BSC_WEBSOCKET_NO_RESOURCES\n");
        pthread_mutex_unlock(&bws_cli_mutex);
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    memcpy(&tmp_buf[LWS_PRE], payload, payload_size);

    written = lws_write(
        bws_cli_conn[h].ws, &tmp_buf[LWS_PRE], payload_size, LWS_WRITE_BINARY);

    DEBUG_PRINTF("bws_cli_dispatch_send() %d bytes is sent\n", written);

    if (written < (int)payload_size) {
        DEBUG_PRINTF(
            "bws_cli_dispatch_send() websocket connection is broken(closed)\n");
        /* tell worker to process change of connection state */
        bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_DISCONNECTING;
        lws_cancel_service(bws_cli_conn[h].ctx);
        ret = BSC_WEBSOCKET_INVALID_OPERATION;
    } else {
        ret = BSC_WEBSOCKET_SUCCESS;
    }

    pthread_mutex_unlock(&bws_cli_mutex);
    free(tmp_buf);
    DEBUG_PRINTF("bws_cli_dispatch_send() <<< ret = %d\n", ret);
    return ret;
}
