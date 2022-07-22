/**
 * @file
 * @brief Implementation of websocket client interface for MAC OS.
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
#include "bacnet/datalink/websocket.h"
#include "bacnet/basic/sys/fifo.h"
#include "bacnet/basic/sys/debug.h"

typedef enum {
    BACNET_WEBSOCKET_STATE_IDLE = 0,
    BACNET_WEBSOCKET_STATE_CONNECTING = 1,
    BACNET_WEBSOCKET_STATE_CONNECTED = 2,
    BACNET_WEBSOCKET_STATE_DISCONNECTING = 3,
    BACNET_WEBSOCKET_STATE_DISCONNECTED = 4
} BACNET_WEBSOCKET_STATE;

typedef struct BACNetWebsocketOperationListEntry {
    struct BACNetWebsocketOperationListEntry *next;
    struct BACNetWebsocketOperationListEntry *last;
    BACNET_WEBSOCKET_RET retcode;
    uint8_t *payload;
    size_t payload_size;
    pthread_cond_t cond;
    bool processed;
} BACNET_WEBSOCKET_OPERATION_ENTRY;

typedef struct {
    int established : 1;
    int closed : 1;
    int data_received : 1;
} BACNET_WEBSOCKET_EVENTS;

typedef struct {
    struct lws_context *ctx;
    struct lws *ws;
    BACNET_WEBSOCKET_STATE state;
    pthread_t thread_id;
    BACNET_WEBSOCKET_OPERATION_ENTRY *send_head;
    BACNET_WEBSOCKET_OPERATION_ENTRY *send_tail;
    BACNET_WEBSOCKET_OPERATION_ENTRY *recv_head;
    BACNET_WEBSOCKET_OPERATION_ENTRY *recv_tail;
    pthread_cond_t cond;
    FIFO_BUFFER in_size;
    // It is assumed that typical minimal BACNet/SC packet size is 16 bytes.
    // So size of the fifo that holds lengths of received datagrams calculated
    // as BACNET_CLIENT_WEBSOCKET_RX_BUFFER_SIZE / 16
    FIFO_DATA_STORE(in_size_buf, BACNET_CLIENT_WEBSOCKET_RX_BUFFER_SIZE / 16);
    FIFO_BUFFER in_data;
    FIFO_DATA_STORE(in_data_buf, BACNET_CLIENT_WEBSOCKET_RX_BUFFER_SIZE);
    BACNET_WEBSOCKET_EVENTS events;
    BACNET_WEBSOCKET_CONNECTION_TYPE type;
    int wait_threads_cnt;
} BACNET_WEBSOCKET_CONNECTION;

// Some forward function declarations

static int bws_cli_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len);

// Websockets protocol defined in BACnet/SC \S AB.7.1.
static const char *bws_hub_protocol = BACNET_WEBSOCKET_HUB_PROTOCOL;
static const char *bws_direct_protocol =
    BACNET_WEBSOCKET_DIRECT_CONNECTION_PROTOCOL;

static pthread_mutex_t bws_cli_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static struct lws_protocols bws_cli_protos[] = {
    { BACNET_WEBSOCKET_HUB_PROTOCOL, bws_cli_websocket_event, 0, 0, 0, NULL,
        0 },
    { BACNET_WEBSOCKET_DIRECT_CONNECTION_PROTOCOL, bws_cli_websocket_event, 0,
        0, 0, NULL, 0 },
    LWS_PROTOCOL_LIST_TERM
};

static const lws_retry_bo_t retry = {
    .secs_since_valid_ping = 3,
    .secs_since_valid_hangup = 10,
};

static BACNET_WEBSOCKET_CONNECTION
    bws_cli_conn[BACNET_CLIENT_WEBSOCKETS_MAX_NUM] = { 0 };

static bool bws_cli_init_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    debug_printf("bws_cli_init_operation() >>> e = %p\n", e);

    memset(e, 0, sizeof(*e));

    if (pthread_cond_init(&e->cond, NULL) != 0) {
        debug_printf("bws_cli_init_operation() <<< ret = false\n");
        return false;
    }

    debug_printf("bws_cli_init_operation() <<< ret = true\n");
    return true;
}

static void bws_cli_deinit_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    debug_printf("bws_cli_deinit_operation() >>> e = %p\n", e);
    if (e) {
        pthread_cond_destroy(&e->cond);
    }
    debug_printf("bws_cli_deinit_operation() <<<\n");
}

static void bws_cli_remove_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e,
    BACNET_WEBSOCKET_OPERATION_ENTRY **head,
    BACNET_WEBSOCKET_OPERATION_ENTRY **tail)
{
    debug_printf(
        "bws_cli_remove_operation() >>> e = %p, head = %p, tail = %p\n", e,
        head, tail);
    if (*head == *tail) {
        *head = NULL;
        *tail = NULL;
    } else if (!e->last) {
        *head = e->next;
        (*head)->last = NULL;
    } else if (!e->next) {
        *tail = e->last;
        (*tail)->next = NULL;
    } else {
        e->next->last = e->last;
        e->last->next = e->next;
    }
    debug_printf("bws_cli_remove_operation() <<<\n");
}

static void bws_cli_enqueue_send_operation(
    BACNET_WEBSOCKET_CONNECTION *c, BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    if (c->send_head == NULL && c->send_tail == NULL) {
        c->send_head = e;
        c->send_tail = e;
        e->next = NULL;
        e->last = NULL;
    } else if (c->send_head == c->send_tail) {
        e->last = c->send_head;
        e->next = NULL;
        c->send_tail = e;
        c->send_head->next = e;
    } else {
        e->last = c->send_tail;
        e->next = NULL;
        c->send_tail->next = e;
        c->send_tail = e;
    }
}

static void bws_cli_dequeue_send_operation(BACNET_WEBSOCKET_CONNECTION *c)
{
    bws_cli_remove_operation(c->send_head, &c->send_head, &c->send_tail);
}

static void bws_cli_dequeue_all_send_operations(BACNET_WEBSOCKET_CONNECTION *c)
{
    debug_printf("bws_cli_dequeue_all_send_operations() >>> c = %p\n", c);
    while (c->send_head) {
        c->send_head->retcode = BACNET_WEBSOCKET_CLOSED;
        c->send_head->processed = 1;
        pthread_cond_signal(&c->send_head->cond);
        bws_cli_dequeue_send_operation(c);
    }
    debug_printf("bws_cli_dequeue_all_send_operations() <<<\n");
}

static void bws_cli_enqueue_recv_operation(
    BACNET_WEBSOCKET_CONNECTION *c, BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    debug_printf("bws_cli_enqueue_recv_operation() >>> c = %p, e = %p\n", c, e);
    if (c->recv_head == NULL && c->recv_tail == NULL) {
        c->recv_head = e;
        c->recv_tail = e;
        e->next = NULL;
        e->last = NULL;
    } else if (c->recv_head == c->recv_tail) {
        e->last = c->recv_head;
        e->next = NULL;
        c->recv_tail = e;
        c->recv_head->next = e;
    } else {
        e->last = c->recv_tail;
        e->next = NULL;
        c->recv_tail->next = e;
        c->recv_tail = e;
    }
    debug_printf("bws_cli_enqueue_recv_operation() <<<\n");
}

static void bws_cli_dequeue_recv_operation(BACNET_WEBSOCKET_CONNECTION *c)
{
    debug_printf("bws_cli_dequeue_recv_operation() >>> c = %p\n", c);
    bws_cli_remove_operation(c->recv_head, &c->recv_head, &c->recv_tail);
    debug_printf("bws_cli_dequeue_recv_operation() <<<\n");
}

static void bws_cli_dequeue_all_recv_operations(BACNET_WEBSOCKET_CONNECTION *c)
{
    debug_printf("bws_cli_dequeue_all_recv_operations() >>> c = %p\n", c);
    while (c->recv_head) {
        c->recv_head->retcode = BACNET_WEBSOCKET_CLOSED;
        c->recv_head->processed = 1;
        pthread_cond_signal(&c->recv_head->cond);
        bws_cli_dequeue_recv_operation(c);
    }
    debug_printf("bws_cli_dequeue_all_recv_operations() <<<\n");
}

static BACNET_WEBSOCKET_HANDLE bws_cli_alloc_connection(void)
{
    int ret;
    int i;

    for (int i = 0; i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_cli_conn[i].state == BACNET_WEBSOCKET_STATE_IDLE) {
            memset(&bws_cli_conn[i], 0, sizeof(bws_cli_conn[i]));
            if (pthread_cond_init(&bws_cli_conn[i].cond, NULL) != 0) {
                return BACNET_WEBSOCKET_INVALID_HANDLE;
            }
            FIFO_Init(&bws_cli_conn[i].in_size, bws_cli_conn[i].in_size_buf,
                sizeof(bws_cli_conn[i].in_size_buf));
            FIFO_Init(&bws_cli_conn[i].in_data, bws_cli_conn[i].in_data_buf,
                sizeof(bws_cli_conn[i].in_data_buf));
            return i;
        }
    }
    return BACNET_WEBSOCKET_INVALID_HANDLE;
}

static void bws_cli_free_connection_ptr(BACNET_WEBSOCKET_CONNECTION *c)
{
    if (c->state != BACNET_WEBSOCKET_STATE_IDLE) {
        c->state = BACNET_WEBSOCKET_STATE_IDLE;
        pthread_cond_destroy(&c->cond);
        c->ws = NULL;
    }
}

static void bws_cli_free_connection(BACNET_WEBSOCKET_HANDLE h)
{
    if (h >= 0 && h < BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        bws_cli_free_connection_ptr(&bws_cli_conn[h]);
    }
}

static BACNET_WEBSOCKET_HANDLE bws_cli_find_connnection(struct lws *ws)
{
    for (int i = 0; i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_cli_conn[i].ws == ws &&
            bws_cli_conn[i].state != BACNET_WEBSOCKET_STATE_DISCONNECTED &&
            bws_cli_conn[i].state != BACNET_WEBSOCKET_STATE_IDLE) {
            return i;
        }
    }
    return BACNET_WEBSOCKET_INVALID_HANDLE;
}

static int bws_cli_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BACNET_WEBSOCKET_HANDLE h;
    int written;

    debug_printf("bws_cli_websocket_event() >>> reason = %d\n", reason);
    pthread_mutex_lock(&bws_cli_mutex);
    h = bws_cli_find_connnection(wsi);

    if (h == BACNET_WEBSOCKET_INVALID_HANDLE) {
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
            bws_cli_conn[h].events.established = 1;
            // wakeup worker to process incoming connection event
            lws_cancel_service(bws_cli_conn[h].ctx);
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            debug_printf(
                "bws_cli_websocket_event() received %d bytes of data\n", len);
            if(!lws_frame_is_binary(wsi)) {
               // According AB.7.5.3 BACnet/SC BVLC Message Exchange,
               // if a received data frame is not binary,
               // the WebSocket connection shall be closed with a
               // status code of 1003 -WEBSOCKET_DATA_NOT_ACCEPTED.
                debug_printf(
                  "bws_cli_websocket_event() got non-binary frame, "\
                  "close connection for socket %d\n", h);
                lws_close_reason(wsi,
                                 LWS_CLOSE_STATUS_UNACCEPTABLE_OPCODE,
                                 NULL, 0 );
                pthread_mutex_unlock(&bws_cli_mutex);
                debug_printf("bws_cli_websocket_event() <<< ret = -1\n");
                return -1;
            }
            if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTED ||
                bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
                if (len <= 65535 &&
                    FIFO_Available(&bws_cli_conn[h].in_data, len) &&
                    FIFO_Available(
                        &bws_cli_conn[h].in_size, sizeof(uint16_t))) {
                    uint16_t packet_len = len;
                    FIFO_Add(&bws_cli_conn[h].in_size, (uint8_t *)&packet_len,
                        sizeof(packet_len));
                    FIFO_Add(&bws_cli_conn[h].in_data, in, len);
                    bws_cli_conn[h].events.data_received = 1;
                    // wakeup worker to process incoming data
                    lws_cancel_service(bws_cli_conn[h].ctx);
                }
#if DEBUG_ENABLED == 1
                else {
                    debug_printf(
                        "bws_cli_websocket_event() drop %d bytes of data "
                        "for socket in state %d\n",
                        len, bws_cli_conn[h].state);
                }
#endif
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            debug_printf("bws_cli_websocket_event() can write\n");
            if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTED ||
                bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
                if (bws_cli_conn[h].send_head) {
                    debug_printf(
                        "bws_cli_websocket_event() going to send %d bytes\n",
                        bws_cli_conn[h].send_head->payload_size);
                    written = lws_write(bws_cli_conn[h].ws,
                        &bws_cli_conn[h].send_head->payload[LWS_PRE],
                        bws_cli_conn[h].send_head->payload_size,
                        LWS_WRITE_BINARY);
                    debug_printf(
                        "bws_cli_websocket_event() %d bytes sent\n", written);
                    if (written <
                        (int)bws_cli_conn[h].send_head->payload_size) {
                        bws_cli_conn[h].events.closed = 1;
                        bws_cli_conn[h].send_head->retcode =
                            BACNET_WEBSOCKET_CLOSED;
                    } else {
                        bws_cli_conn[h].send_head->retcode =
                            BACNET_WEBSOCKET_SUCCESS;
                    }
                    bws_cli_conn[h].send_head->processed = 1;
                    pthread_cond_signal(&bws_cli_conn[h].send_head->cond);
                    bws_cli_dequeue_send_operation(&bws_cli_conn[h]);
                    // wakeup worker to process internal state
                    lws_cancel_service(bws_cli_conn[h].ctx);
                }
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
            if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
                debug_printf(
                    "got disconnect while connecting reason = %d\n", reason);
                debug_printf("desc = %s\n", (char *)in);
            }

            debug_printf(
                "bws_cli_websocket_event() connection closed, reason = %d\n",
                reason);
            bws_cli_conn[h].events.closed = 1;
            // wakeup worker to process pending vent
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
    BACNET_WEBSOCKET_HANDLE h = *((int *)arg);
    BACNET_WEBSOCKET_CONNECTION *conn = &bws_cli_conn[h];
    BACNET_WEBSOCKET_STATE old_state;

    while (1) {
        debug_printf("bws_cli_worker() lock mutex\n");
        pthread_mutex_lock(&bws_cli_mutex);
        debug_printf("bws_cli_worker() unlock mutex\n");
        debug_printf(
            "bws_cli_worker() unblocked, socket = %d, socket state = %d\n", h,
            conn->state);
        if (conn->events.closed ||
            conn->state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
            conn->events.closed = 0;
            old_state = conn->state;
            conn->state = BACNET_WEBSOCKET_STATE_DISCONNECTED;
            bws_cli_dequeue_all_send_operations(conn);
            bws_cli_dequeue_all_recv_operations(conn);
            // Note! lws_conext_destroy() implicitly calls bws_websocket_event()
            // callback. So connection state must be changed before that call.
            lws_context_destroy(conn->ctx);
            // if websocket is in BACNET_WEBSOCKET_STATE_CONNECTING or in
            // BACNET_WEBSOCKET_STATE_DISCONNECTING it means that some thread
            // (or even threads) is (are) blocked and waiting for result. That's
            // why pthread_cond_broadcast() used there.
            pthread_cond_broadcast(&conn->cond);

            if (old_state == BACNET_WEBSOCKET_STATE_CONNECTED) {
                if (bws_cli_conn[h].wait_threads_cnt == 0) {
                    bws_cli_free_connection_ptr(conn);
                }
            }
            pthread_mutex_unlock(&bws_cli_mutex);
            break;
        } else if (conn->state == BACNET_WEBSOCKET_STATE_CONNECTING) {
            if (conn->events.established) {
                conn->events.established = 0;
                conn->state = BACNET_WEBSOCKET_STATE_CONNECTED;
                pthread_cond_signal(&conn->cond);
            }
        } else if (conn->state == BACNET_WEBSOCKET_STATE_CONNECTED) {
            debug_printf(
                "bws_cli_worker() process BACNET_WEBSOCKET_STATE_CONNECTED\n");
            if (conn->send_head) {
                debug_printf(
                    "bws_cli_worker() request callback for sending data\n");
                lws_callback_on_writable(conn->ws);
            }
            while (conn->recv_head && !FIFO_Empty(&conn->in_size)) {
                uint16_t packet_len;
                FIFO_Pull(
                    &conn->in_size, (uint8_t *)&packet_len, sizeof(packet_len));
                debug_printf("bws_cli_worker() packet_len = %d\n", packet_len);
                debug_printf(
                    "bws_cli_worker() conn->recv_head->payload_size = %d\n",
                    conn->recv_head->payload_size);

                if (conn->recv_head->payload_size < packet_len) {
                    FIFO_Pull(&conn->in_data, conn->recv_head->payload,
                        conn->recv_head->payload_size);
                    // remove part of datagram which does not fit into user
                    // buffer
                    packet_len -= conn->recv_head->payload_size;
                    while (packet_len > 0) {
                        FIFO_Get(&conn->in_data);
                        packet_len--;
                    }
                    conn->recv_head->retcode =
                        BACNET_WEBSOCKET_BUFFER_TOO_SMALL;
                } else {
                    conn->recv_head->retcode = BACNET_WEBSOCKET_SUCCESS;
                    FIFO_Pull(
                        &conn->in_data, conn->recv_head->payload, packet_len);
                    conn->recv_head->payload_size = packet_len;
                }
                debug_printf("bws_cli_worker() signal that %d bytes received "
                             "on websocket %d\n",
                    conn->recv_head->payload_size, h);
                conn->recv_head->processed = 1;
                pthread_cond_signal(&conn->recv_head->cond);
                bws_cli_dequeue_recv_operation(conn);
            }
        }
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_worker() going to block on lws_service() call\n");
        lws_service(conn->ctx, 0);
    }

    return NULL;
}

static BACNET_WEBSOCKET_RET bws_cli_connect(
    BACNET_WEBSOCKET_CONNECTION_TYPE type,
    char *url,
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size,
    BACNET_WEBSOCKET_HANDLE *out_handle)
{
    struct lws_context_creation_info info = { 0 };
    char tmp_url[BACNET_WSURL_MAX_LEN];
    const char *prot = NULL, *addr = NULL, *path = NULL;
    int port = -1;
    BACNET_WEBSOCKET_HANDLE h;
    struct lws_client_connect_info cinfo = { 0 };
    BACNET_WEBSOCKET_RET ret;

    debug_printf("bws_cli_connect() >>> type = %d, url = %s\n", type, url);

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size ||
        !url || !out_handle) {
        debug_printf(
            "bws_cli_connect() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (type != BACNET_WEBSOCKET_HUB_CONNECTION &&
        type != BACNET_WEBSOCKET_DIRECT_CONNECTION) {
        debug_printf(
            "bws_cli_connect() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    strncpy(tmp_url, url, BACNET_WSURL_MAX_LEN);

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
        debug_printf(
            "bws_cli_connect() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (strcmp(prot, "wss") != 0) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_connect() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    h = bws_cli_alloc_connection();

    if (h == BACNET_WEBSOCKET_INVALID_HANDLE) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_connect() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = bws_cli_protos;
    info.gid = -1;
    info.uid = -1;
    info.client_ssl_cert_mem = cert;
    info.client_ssl_cert_mem_len = cert_size;
    info.client_ssl_ca_mem = ca_cert;
    info.client_ssl_ca_mem_len = ca_cert_size;
    info.client_ssl_key_mem = key;
    info.client_ssl_key_mem_len = key_size;
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.timeout_secs = BACNET_WEBSOCKET_TIMEOUT_SECONDS;
    info.connect_timeout_secs = BACNET_WEBSOCKET_TIMEOUT_SECONDS;

    bws_cli_conn[h].ctx = lws_create_context(&info);

    if (!bws_cli_conn[h].ctx) {
        bws_cli_free_connection(h);
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_connect() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    ret = pthread_create(&bws_cli_conn[h].thread_id, NULL, &bws_cli_worker, &h);

    if (ret != 0) {
        bws_cli_free_connection(h);
        lws_context_destroy(bws_cli_conn[h].ctx);
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_connect() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    // from that moment worker thread will be blocked until we unlock
    // bws_cli_mutex

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

    if (type == BACNET_WEBSOCKET_HUB_CONNECTION) {
        cinfo.protocol = bws_hub_protocol;
    } else {
        cinfo.protocol = bws_direct_protocol;
    }

    bws_cli_conn[h].type = type;
    bws_cli_conn[h].state = BACNET_WEBSOCKET_STATE_CONNECTING;
    lws_client_connect_via_info(&cinfo);

    // let's unblock worker thread and wait for connection over websocket
    debug_printf("bws_cli_connect() going to block on pthread_cond_wait()\n");
    bws_cli_conn[h].wait_threads_cnt++;
    while (bws_cli_conn[h].state != BACNET_WEBSOCKET_STATE_CONNECTED &&
        bws_cli_conn[h].state != BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        pthread_cond_wait(&bws_cli_conn[h].cond, &bws_cli_mutex);
    }

    debug_printf("bws_cli_connect() unblocked\n");
    bws_cli_conn[h].wait_threads_cnt--;

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTED) {
        *out_handle = h;
        ret = BACNET_WEBSOCKET_SUCCESS;
    } else {
        debug_printf("socket %d state = %d\n", h, bws_cli_conn[h].state);
        if (bws_cli_conn[h].wait_threads_cnt == 0) {
            bws_cli_free_connection(h);
        }
        *out_handle = BACNET_WEBSOCKET_INVALID_HANDLE;
        ret = BACNET_WEBSOCKET_CLOSED;
    }

    pthread_mutex_unlock(&bws_cli_mutex);
    debug_printf("bws_cli_connect() <<< ret = %d\n", ret);
    return ret;
}

static BACNET_WEBSOCKET_RET bws_cli_disconnect(BACNET_WEBSOCKET_HANDLE h)
{
    debug_printf("bws_cli_disconnect() >>> h = %d\n", h);

    if (h < 0 || h >= BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        debug_printf(
            "bws_cli_disconnect() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_cli_mutex);

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_disconnect() <<< ret = BACNET_WEBSOCKET_CLOSED\n");
        return BACNET_WEBSOCKET_CLOSED;
    } else if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING ||
        bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        // some thread has already started disconnect process.
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_disconnect() <<< ret = "
                     "BACNET_WEBSOCKET_OPERATION_IN_PROGRESS\n");
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    } else {
        bws_cli_conn[h].state = BACNET_WEBSOCKET_STATE_DISCONNECTING;
        // signal worker to process change of connection state
        lws_cancel_service(bws_cli_conn[h].ctx);
        // let's wait while worker thread processes changes
        debug_printf(
            "bws_cli_disconnect() going to block on pthread_cond_wait()\n");
        bws_cli_conn[h].wait_threads_cnt++;
        while (bws_cli_conn[h].state != BACNET_WEBSOCKET_STATE_DISCONNECTED) {
            pthread_cond_wait(&bws_cli_conn[h].cond, &bws_cli_mutex);
        }
        bws_cli_conn[h].wait_threads_cnt--;
        debug_printf("bws_cli_disconnect() unblocked\n");
        if (bws_cli_conn[h].wait_threads_cnt == 0) {
            bws_cli_free_connection(h);
        }
    }
    pthread_mutex_unlock(&bws_cli_mutex);
    debug_printf("bws_cli_disconnect() <<< ret = BACNET_WEBSOCKET_SUCCESS\n");
    return BACNET_WEBSOCKET_SUCCESS;
}

static BACNET_WEBSOCKET_RET bws_cli_send(
    BACNET_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size)
{
    BACNET_WEBSOCKET_OPERATION_ENTRY e;
    debug_printf("bws_cli_send() >>> h = %d, payload = %p, payload_size = %d\n",
        h, payload, payload_size);

    if (h < 0 || h >= BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        debug_printf("bws_cli_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!payload || !payload_size) {
        debug_printf("bws_cli_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_cli_mutex);

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE ||
        bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_send() <<< ret = BACNET_WEBSOCKET_CLOSED\n");
        return BACNET_WEBSOCKET_CLOSED;
    }

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_send() <<< ret = "
                     "BACNET_WEBSOCKET_OPERATION_IN_PROGRESS\n");
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    // user is allowed to send data if websocket connection is already
    // established or it is in a process of establishing

    if (!bws_cli_init_operation(&e)) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_send() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload = malloc(payload_size + LWS_PRE);

    if (!e.payload) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_send() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload_size = payload_size;
    memcpy(&e.payload[LWS_PRE], payload, payload_size);
    bws_cli_enqueue_send_operation(&bws_cli_conn[h], &e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_cli_conn[h].ctx);

    // now wait until libwebsockets runloop processes write request

    debug_printf("bws_cli_send() going to block on pthread_cond_wait()\n");
    while (!e.processed) {
        pthread_cond_wait(&e.cond, &bws_cli_mutex);
    }
    debug_printf("bws_cli_send() unblocked()\n");
    pthread_mutex_unlock(&bws_cli_mutex);
    free(e.payload);
    bws_cli_deinit_operation(&e);
    debug_printf("bws_cli_send() <<< ret = %d\n", e.retcode);
    return e.retcode;
}

static BACNET_WEBSOCKET_RET bws_cli_recv(BACNET_WEBSOCKET_HANDLE h,
    uint8_t *buf,
    size_t bufsize,
    size_t *bytes_received,
    int timeout)
{
    BACNET_WEBSOCKET_OPERATION_ENTRY e;
    struct timespec to;
    debug_printf(
        "bws_cli_recv() >>> h = %d, buf = %p, bufsize = %d, timeout = %d\n", h,
        buf, bufsize, timeout);
    if (h < 0 || h >= BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        debug_printf("bws_cli_recv() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!buf || !bufsize || !bytes_received) {
        debug_printf("bws_cli_recv() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_cli_mutex);

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE ||
        bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_recv() <<< ret = BACNET_WEBSOCKET_CLOSED\n");
        return BACNET_WEBSOCKET_CLOSED;
    }

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf("bws_cli_recv() <<< ret = "
                     "BACNET_WEBSOCKET_OPERATION_IN_PROGRESS\n");
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    // user is allowed to recv data if websocket connection is already
    // established or it is in a process of establishing

    if (!bws_cli_init_operation(&e)) {
        pthread_mutex_unlock(&bws_cli_mutex);
        debug_printf(
            "bws_cli_recv() <<< ret =  BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload = buf;
    e.payload_size = bufsize;
    bws_cli_enqueue_recv_operation(&bws_cli_conn[h], &e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_cli_conn[h].ctx);

    // now wait until libwebsockets runloop processes write request
    clock_gettime(CLOCK_REALTIME, &to);

    to.tv_sec = to.tv_sec + timeout / 1000;
    to.tv_nsec = to.tv_nsec + (timeout % 1000) * 1000000;
    to.tv_sec += to.tv_nsec / 1000000000;
    to.tv_nsec %= 1000000000;
    debug_printf("bws_cli_recv() websocket state %d\n", bws_cli_conn[h].state);
    debug_printf("bws_cli_recv() going to block on pthread_cond_timedwait()\n");

    while (!e.processed) {
        if (pthread_cond_timedwait(&e.cond, &bws_cli_mutex, &to) == ETIMEDOUT) {
            bws_cli_remove_operation(
                &e, &bws_cli_conn[h].recv_head, &bws_cli_conn[h].recv_tail);
            pthread_mutex_unlock(&bws_cli_mutex);
            bws_cli_deinit_operation(&e);
            debug_printf(
                "bws_cli_recv() <<< ret = BACNET_WEBSOCKET_TIMEDOUT\n");
            return BACNET_WEBSOCKET_TIMEDOUT;
        }
    }
    debug_printf("bws_cli_recv() unblocked\n");
    pthread_mutex_unlock(&bws_cli_mutex);

    if (e.retcode == BACNET_WEBSOCKET_SUCCESS ||
        e.retcode == BACNET_WEBSOCKET_BUFFER_TOO_SMALL) {
        *bytes_received = e.payload_size;
    }

    bws_cli_deinit_operation(&e);
    debug_printf("bws_cli_recv() <<< ret = %d, bytes_received = %d\n",
        e.retcode, *bytes_received);
    return e.retcode;
}

static BACNET_WEBSOCKET_CLIENT bws_cli = { bws_cli_connect, bws_cli_disconnect,
    bws_cli_send, bws_cli_recv };

BACNET_WEBSOCKET_CLIENT *bws_cli_get(void)
{
    return &bws_cli;
}
