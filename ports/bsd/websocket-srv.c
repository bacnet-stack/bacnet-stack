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
#include "bacnet/datalink/websocket.h"
#include "bacnet/basic/sys/fifo.h"
#include "bacnet/basic/sys/debug.h"

typedef enum {
    BACNET_WEBSOCKET_STATE_IDLE = 0,
    BACNET_WEBSOCKET_STATE_CONNECTING,
    BACNET_WEBSOCKET_STATE_CONNECTED,
    BACNET_WEBSOCKET_STATE_DISCONNECTING,
    BACNET_WEBSOCKET_STATE_DISCONNECTED
} BACNET_WEBSOCKET_STATE;

typedef struct BACNetWebsocketOperationListEntry {
    struct BACNetWebsocketOperationListEntry *next;
    struct BACNetWebsocketOperationListEntry *last;
    BACNET_WEBSOCKET_RET retcode;
    uint8_t *payload;
    size_t payload_size;
    pthread_cond_t cond;
    bool processed;
    BACNET_WEBSOCKET_HANDLE h;
} BACNET_WEBSOCKET_OPERATION_ENTRY;

// Some forward function declarations

static int bws_srv_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len);

// Websockets protocol defined in BACnet/SC \S AB.7.1.
static const char *bws_hub_protocol = BACNET_WEBSOCKET_HUB_PROTOCOL;
static const char *bws_direct_protocol =
    BACNET_WEBSOCKET_DIRECT_CONNECTION_PROTOCOL;

static struct lws_context *bws_srv_ctx = NULL;
static pthread_mutex_t bws_srv_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static pthread_t bws_srv_worker_thread_id;
static bool bws_srv_stop_worker = false;

typedef struct {
    struct lws *ws;
    BACNET_WEBSOCKET_STATE state;
    BACNET_WEBSOCKET_OPERATION_ENTRY *send_head;
    BACNET_WEBSOCKET_OPERATION_ENTRY *send_tail;
    BACNET_WEBSOCKET_OPERATION_ENTRY *recv_head;
    BACNET_WEBSOCKET_OPERATION_ENTRY *recv_tail;
    pthread_cond_t cond;
    FIFO_BUFFER in_data;
    FIFO_DATA_STORE(in_data_buf, BACNET_SERVER_WEBSOCKET_RX_BUFFER_SIZE);
    BACNET_WEBSOCKET_CONNECTION_TYPE type;
    int wait_threads_cnt;
} BACNET_WEBSOCKET_CONNECTION;

static struct lws_protocols bws_srv_protos[] = {
    { BACNET_WEBSOCKET_HUB_PROTOCOL, bws_srv_websocket_event, 0, 0, 0, NULL,
        0 },
    { BACNET_WEBSOCKET_DIRECT_CONNECTION_PROTOCOL, bws_srv_websocket_event, 0,
        0, 0, NULL, 0 },
    LWS_PROTOCOL_LIST_TERM
};

static const lws_retry_bo_t retry = {
    .secs_since_valid_ping = 3,
    .secs_since_valid_hangup = 10,
};

static BACNET_WEBSOCKET_CONNECTION
    bws_srv_conn[BACNET_SERVER_WEBSOCKETS_MAX_NUM] = { 0 };

static BACNET_WEBSOCKET_OPERATION_ENTRY *bws_accept_head = NULL;
static BACNET_WEBSOCKET_OPERATION_ENTRY *bws_accept_tail = NULL;

static bool bws_srv_init_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    debug_printf("bws_srv_init_operation() >>> e = %p\n", e);
    memset(e, 0, sizeof(*e));

    if (pthread_cond_init(&e->cond, NULL) != 0) {
        debug_printf("bws_srv_init_operation() <<< ret = false\n");
        return false;
    }

    e->h = BACNET_WEBSOCKET_INVALID_HANDLE;
    debug_printf("bws_srv_init_operation() <<< ret = true\n");
    return true;
}

static void bws_srv_deinit_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    debug_printf("bws_srv_deinit_operation() >>> e = %p\n", e);
    if (e) {
        pthread_cond_destroy(&e->cond);
    }
    debug_printf("bws_srv_deinit_operation() <<<\n");
}

static void bws_srv_enqueue_accept_operation(
    BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    debug_printf("bws_srv_enqueue_accept_operation() >>> e = %p\n", e);
    if (bws_accept_head == NULL && bws_accept_tail == NULL) {
        bws_accept_head = e;
        bws_accept_tail = e;
        e->next = NULL;
        e->last = NULL;
    } else if (bws_accept_head == bws_accept_tail) {
        e->last = bws_accept_head;
        e->next = NULL;
        bws_accept_tail = e;
        bws_accept_head->next = e;
    } else {
        e->last = bws_accept_tail;
        e->next = NULL;
        bws_accept_tail->next = e;
        bws_accept_tail = e;
    }
    debug_printf("bws_srv_enqueue_accept_operation() <<<\n");
}

static void bws_srv_dequeue_accept_operation(void)
{
    debug_printf(
        "bws_srv_dequeue_accept_operation() >>> e = %p\n", bws_accept_head);

    if (bws_accept_head == bws_accept_tail) {
        bws_accept_head = NULL;
        bws_accept_tail = NULL;
    } else {
        bws_accept_head->next->last = NULL;
        bws_accept_head = bws_accept_head->next;
    }

    debug_printf("bws_srv_dequeue_accept_operation() <<<\n");
}

static void bws_srv_dequeue_all_accept_operations(void)
{
    debug_printf("bws_srv_dequeue_all_accept_operations() >>>\n");
    while (bws_accept_head) {
        bws_accept_head->retcode = BACNET_WEBSOCKET_CLOSED;
        bws_accept_head->processed = 1;
        pthread_cond_signal(&bws_accept_head->cond);
        bws_srv_dequeue_accept_operation();
    }
    debug_printf("bws_srv_dequeue_all_accept_operations() <<<\n");
}

static void bws_srv_enqueue_recv_operation(
    BACNET_WEBSOCKET_CONNECTION *c, BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    debug_printf("bws_srv_enqueue_recv_operation() >>> c = %p, e = %p\n", c, e);
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
    debug_printf("bws_srv_enqueue_recv_operation() <<< \n");
}

static void bws_srv_dequeue_recv_operation(BACNET_WEBSOCKET_CONNECTION *c)
{
    debug_printf("bws_srv_dequeue_recv_operation() >>> c = %p\n", c);
    if (c->recv_head == c->recv_tail) {
        c->recv_head = NULL;
        c->recv_tail = NULL;
    } else {
        c->recv_head->next->last = NULL;
        c->recv_head = c->recv_head->next;
    }
    debug_printf("bws_srv_dequeue_recv_operation() <<< \n");
}

static void bws_srv_dequeue_all_recv_operations(BACNET_WEBSOCKET_CONNECTION *c)
{
    debug_printf("bws_srv_dequeue_all_recv_operations( >>> c = %p\n", c);
    while (c->recv_head) {
        c->recv_head->retcode = BACNET_WEBSOCKET_CLOSED;
        c->recv_head->processed = 1;
        pthread_cond_signal(&c->recv_head->cond);
        bws_srv_dequeue_recv_operation(c);
    }
    debug_printf("bws_srv_dequeue_all_recv_operations() <<<\n", c);
}

static void bws_srv_enqueue_send_operation(
    BACNET_WEBSOCKET_CONNECTION *c, BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    debug_printf("bws_srv_enqueue_send_operation() >>> c = %p, e = %p\n", c, e);
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
    debug_printf("bws_srv_enqueue_send_operation() <<<\n");
}

static void bws_srv_dequeue_send_operation(BACNET_WEBSOCKET_CONNECTION *c)
{
    debug_printf("bws_srv_dequeue_send_operation() >>> c = %p\n", c);
    if (c->send_head == c->send_tail) {
        c->send_head = NULL;
        c->send_tail = NULL;
    } else {
        c->send_head->next->last = NULL;
        c->send_head = c->send_head->next;
    }
    debug_printf("bws_srv_dequeue_send_operation() <<<\n");
}

static void bws_srv_dequeue_all_send_operations(BACNET_WEBSOCKET_CONNECTION *c)
{
    debug_printf("bws_srv_dequeue_all_send_operations() >>> c = %p\n", c);
    while (c->send_head) {
        c->send_head->retcode = BACNET_WEBSOCKET_CLOSED;
        c->send_head->processed = 1;
        pthread_cond_signal(&c->send_head->cond);
        bws_srv_dequeue_send_operation(c);
    }
    debug_printf("bws_srv_dequeue_all_send_operations() <<<\n");
}

static BACNET_WEBSOCKET_HANDLE bws_srv_alloc_connection(void)
{
    int ret;
    int i;
    debug_printf("bws_srv_alloc_connection() >>>\n");

    for (int i = 0; i < BACNET_SERVER_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_srv_conn[i].state == BACNET_WEBSOCKET_STATE_IDLE) {
            memset(&bws_srv_conn[i], 0, sizeof(bws_srv_conn[i]));
            if (pthread_cond_init(&bws_srv_conn[i].cond, NULL) != 0) {
                return BACNET_WEBSOCKET_INVALID_HANDLE;
            }
            FIFO_Init(&bws_srv_conn[i].in_data, bws_srv_conn[i].in_data_buf,
                sizeof(bws_srv_conn[i].in_data_buf));
            debug_printf("bws_srv_alloc_connection() <<< ret = %d, cond = %p\n",
                i, &bws_srv_conn[i].cond);
            return i;
        }
    }
    debug_printf("bws_srv_alloc_connection() <<< ret = "
                 "BACNET_WEBSOCKET_INVALID_HANDLE\n");
    return BACNET_WEBSOCKET_INVALID_HANDLE;
}

static void bws_srv_free_connection(BACNET_WEBSOCKET_HANDLE h)
{
    debug_printf("bws_srv_free_connection() >>> h = %d\n", h);
    if (h >= 0 && h < BACNET_SERVER_WEBSOCKETS_MAX_NUM) {
        if (bws_srv_conn[h].state != BACNET_WEBSOCKET_STATE_IDLE) {
            bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_IDLE;
            pthread_cond_destroy(&bws_srv_conn[h].cond);
            bws_srv_conn[h].ws = NULL;
        }
    }
    debug_printf("bws_srv_free_connection() <<<\n");
}

static BACNET_WEBSOCKET_HANDLE bws_find_connnection(struct lws *ws)
{
    for (int i = 0; i < BACNET_SERVER_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_srv_conn[i].ws == ws &&
            bws_srv_conn[i].state != BACNET_WEBSOCKET_STATE_DISCONNECTED &&
            bws_srv_conn[i].state != BACNET_WEBSOCKET_STATE_IDLE) {
            return i;
        }
    }
    return BACNET_WEBSOCKET_INVALID_HANDLE;
}

static int bws_srv_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BACNET_WEBSOCKET_HANDLE h;
    int written;
    int ret = 0;
    debug_printf("bws_srv_websocket_event() >>> wsi = %p, reason = %d, in = "
                 "%p, len = %d\n",
        wsi, reason, in, len);
    pthread_mutex_lock(&bws_srv_mutex);

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            debug_printf("bws_srv_websocket_event() established connection\n");
            h = bws_srv_alloc_connection();
            if (h == BACNET_WEBSOCKET_INVALID_HANDLE) {
                pthread_mutex_unlock(&bws_srv_mutex);
                return -1;
            }
            debug_printf("bws_srv_websocket_event() set state of socket %d to "
                         "BACNET_WEBSOCKET_STATE_CONNECTING\n",
                h);
            bws_srv_conn[h].ws = wsi;
            bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_CONNECTING;
            // wakeup worker to process pending vent
            lws_cancel_service(bws_srv_ctx);
            break;
        }
        case LWS_CALLBACK_CLOSED: {
            debug_printf("bws_srv_websocket_event() closed connection\n");
            h = bws_find_connnection(wsi);
            if (h != BACNET_WEBSOCKET_INVALID_HANDLE) {
                debug_printf(
                    "bws_srv_websocket_event() state of socket %d is %d\n", h,
                    bws_srv_conn[h].state);
                bws_srv_dequeue_all_recv_operations(&bws_srv_conn[h]);
                bws_srv_dequeue_all_send_operations(&bws_srv_conn[h]);
                if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_DISCONNECTING) {
                    bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_DISCONNECTED;
                    debug_printf("bws_srv_websocket_event() set state %d for "
                                 "socket %d\n",
                        bws_srv_conn[h].state, h);
                    lws_cancel_service(bws_srv_ctx);
                } else if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_CONNECTED) {
                    bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_DISCONNECTED;
                } else if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_CONNECTING) {
                    bws_srv_free_connection(h);
                }
            }
            break;
        }
        case LWS_CALLBACK_RECEIVE: {
            h = bws_find_connnection(wsi);
            if (h != BACNET_WEBSOCKET_INVALID_HANDLE) {
                debug_printf("bws_srv_websocket_event() received %d bytes of "
                             "data for websocket %d\n",
                    len, h);
                if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTED) {
                    if (FIFO_Add(&bws_srv_conn[h].in_data, in, len)) {
                        // wakeup worker to process incoming data
                        lws_cancel_service(bws_srv_ctx);
                    }
#ifdef DEBUG_ENABLED
                    else {
                        debug_printf("bws_srv_websocket_event() drop %d bytes "
                                     "of data on socket %d\n",
                            len, h);
                    }
#endif
                }
            }
            break;
        }
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            debug_printf("bws_srv_websocket_event() can write\n");
            h = bws_find_connnection(wsi);
            if (h != BACNET_WEBSOCKET_INVALID_HANDLE) {
                debug_printf("bws_srv_websocket_event() socket %d state = %d\n",
                    h, bws_srv_conn[h].state);
                if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_DISCONNECTING) {
                    debug_printf("bws_srv_websocket_event() <<< ret = -1\n");
                    pthread_mutex_unlock(&bws_srv_mutex);
                    return -1;
                } else if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_CONNECTED) {
                    if (bws_srv_conn[h].send_head) {
                        debug_printf("bws_srv_websocket_event() going to send "
                                     "%d bytes\n",
                            bws_srv_conn[h].send_head->payload_size);
                        written = lws_write(bws_srv_conn[h].ws,
                            &bws_srv_conn[h].send_head->payload[LWS_PRE],
                            bws_srv_conn[h].send_head->payload_size,
                            LWS_WRITE_BINARY);
                        debug_printf(
                            "bws_srv_websocket_event() %d bytes sent\n",
                            written);
                        if (written <
                            (int)bws_srv_conn[h].send_head->payload_size) {
                            bws_srv_conn[h].send_head->retcode =
                                BACNET_WEBSOCKET_CLOSED;
                            ret = -1;
                        } else {
                            bws_srv_conn[h].send_head->retcode =
                                BACNET_WEBSOCKET_SUCCESS;
                        }
                        bws_srv_conn[h].send_head->processed = 1;
                        debug_printf("bws_srv_websocket_event() unblock send "
                                     "function\n");
                        pthread_cond_signal(&bws_srv_conn[h].send_head->cond);
                        bws_srv_dequeue_send_operation(&bws_srv_conn[h]);
                        // wakeup worker to process internal state
                        lws_cancel_service(bws_srv_ctx);
                    }
                }
            }
            break;
        }
        case LWS_CALLBACK_EVENT_WAIT_CANCELLED: {
            break;
        }
        default: {
            break;
        }
    }
    pthread_mutex_unlock(&bws_srv_mutex);
    debug_printf("bws_srv_websocket_event() <<< ret = %d\n", ret);
    return ret;
}

static void *bws_srv_worker(void *arg)
{
    int i;

    while (1) {
        pthread_mutex_lock(&bws_srv_mutex);
        debug_printf("bws_srv_worker() unblocked\n");

        if (bws_srv_stop_worker) {
            debug_printf("bws_srv_worker() going to stop\n");
            bws_srv_dequeue_all_accept_operations();
            for (i = 0; i < BACNET_SERVER_WEBSOCKETS_MAX_NUM; i++) {
                bws_srv_dequeue_all_recv_operations(&bws_srv_conn[i]);
                bws_srv_dequeue_all_send_operations(&bws_srv_conn[i]);
                if (bws_srv_conn[i].state ==
                    BACNET_WEBSOCKET_STATE_DISCONNECTING) {
                    bws_srv_conn[i].state = BACNET_WEBSOCKET_STATE_DISCONNECTED;
                    debug_printf(
                        "bws_srv_worker() signal socket %d to unblock\n", i);
                    pthread_cond_broadcast(&bws_srv_conn[i].cond);
                } else {
                    if (bws_srv_conn[i].wait_threads_cnt == 0) {
                        bws_srv_free_connection(i);
                    } else {
                        pthread_cond_broadcast(&bws_srv_conn[i].cond);
                    }
                }
            }
            lws_context_destroy(bws_srv_ctx);
            bws_srv_ctx = NULL;
            bws_srv_stop_worker = false;
            pthread_mutex_unlock(&bws_srv_mutex);
            debug_printf("bws_srv_worker() stopped\n");
            break;
        }

        for (i = 0; i < BACNET_SERVER_WEBSOCKETS_MAX_NUM; i++) {
            debug_printf("bws_srv_worker() socket %d state = %d\n", i,
                bws_srv_conn[i].state);
            if (bws_srv_conn[i].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
                if (bws_accept_head != NULL) {
                    bws_accept_head->retcode = BACNET_WEBSOCKET_SUCCESS;
                    bws_accept_head->processed = 1;
                    bws_accept_head->h = i;
                    bws_srv_conn[i].state = BACNET_WEBSOCKET_STATE_CONNECTED;
                    debug_printf("bws_srv_worker() signal socket %d to unblock "
                                 "on accept\n",
                        i);
                    pthread_cond_signal(&bws_accept_head->cond);
                    bws_srv_dequeue_accept_operation();
                }
            } else if (bws_srv_conn[i].state ==
                BACNET_WEBSOCKET_STATE_DISCONNECTING) {
                debug_printf("bws_srv_worker() schedule callback to disconnect "
                             "on socket %i\n",
                    i);
                lws_callback_on_writable(bws_srv_conn[i].ws);
            } else if (bws_srv_conn[i].state ==
                BACNET_WEBSOCKET_STATE_DISCONNECTED) {
                debug_printf(
                    "bws_srv_worker() signal to unblock socket %d cond = %p\n",
                    i, &bws_srv_conn[i].cond);
                pthread_cond_broadcast(&bws_srv_conn[i].cond);
            } else if (bws_srv_conn[i].state ==
                BACNET_WEBSOCKET_STATE_CONNECTED) {
                if (bws_srv_conn[i].send_head) {
                    debug_printf("bws_srv_worker() schedule callback to send "
                                 "data on socket %i\n",
                        i);
                    lws_callback_on_writable(bws_srv_conn[i].ws);
                }
                while (bws_srv_conn[i].recv_head &&
                    !FIFO_Empty(&bws_srv_conn[i].in_data)) {
                    bws_srv_conn[i].recv_head->payload_size =
                        FIFO_Pull(&bws_srv_conn[i].in_data,
                            bws_srv_conn[i].recv_head->payload,
                            bws_srv_conn[i].recv_head->payload_size);
                    bws_srv_conn[i].recv_head->processed = 1;
                    pthread_cond_signal(&bws_srv_conn[i].recv_head->cond);
                    bws_srv_dequeue_recv_operation(&bws_srv_conn[i]);
                }
            }
        }
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_worker() going to block on lws_service() call\n");
        lws_service(bws_srv_ctx, 0);
    }

    return NULL;
}

static BACNET_WEBSOCKET_RET bws_srv_start(int port,
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size)
{
    struct lws_context_creation_info info = { 0 };
    int ret;

    debug_printf("bws_srv_start() >>> port = %d\n", port);

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size) {
        debug_printf("bws_srv_start() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (port < 0 || port > 65535) {
        debug_printf("bws_srv_start() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_start() <<< ret = BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_start() <<< ret = BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
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
    info.protocols = bws_srv_protos;
    info.gid = -1;
    info.uid = -1;
    info.server_ssl_cert_mem = cert;
    info.server_ssl_cert_mem_len = cert_size;
    info.server_ssl_ca_mem = ca_cert;
    info.server_ssl_ca_mem_len = ca_cert_size;
    info.server_ssl_private_key_mem = key;
    info.server_ssl_private_key_mem_len = key_size;
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    bws_srv_ctx = lws_create_context(&info);

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_start() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    ret =
        pthread_create(&bws_srv_worker_thread_id, NULL, &bws_srv_worker, NULL);

    if (ret != 0) {
        lws_context_destroy(bws_srv_ctx);
        bws_srv_ctx = NULL;
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_start() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    pthread_mutex_unlock(&bws_srv_mutex);
    debug_printf("bws_srv_start() <<< ret = BACNET_WEBSOCKET_SUCCESS\n");
    return BACNET_WEBSOCKET_SUCCESS;
}

static BACNET_WEBSOCKET_RET bws_srv_accept(BACNET_WEBSOCKET_HANDLE *out_handle)
{
    BACNET_WEBSOCKET_OPERATION_ENTRY e;
    debug_printf("bws_srv_accept() >>> out_handle = %p\n");

    if (!out_handle) {
        debug_printf("bws_srv_accept() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    *out_handle = BACNET_WEBSOCKET_INVALID_HANDLE;

    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_accept() <<< ret = BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_accept() <<< ret = BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_srv_init_operation(&e)) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_accept() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    bws_srv_enqueue_accept_operation(&e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_srv_ctx);

    // now wait for a new client connection
    debug_printf("bws_srv_accept() going to block on pthread_cond_wait()\n");

    while (!e.processed) {
        pthread_cond_wait(&e.cond, &bws_srv_mutex);
    }

    debug_printf("bws_srv_accept() unblocked\n");
    *out_handle = e.h;
    pthread_mutex_unlock(&bws_srv_mutex);
    bws_srv_deinit_operation(&e);
    debug_printf("bws_srv_accept() ret = %d\n", e.retcode);
    return e.retcode;
}

BACNET_WEBSOCKET_RET bws_srv_disconnect(BACNET_WEBSOCKET_HANDLE h)
{
    debug_printf("bws_srv_disconnect() >>> h = %d\n", h);

    if (h < 0 || h >= BACNET_SERVER_WEBSOCKETS_MAX_NUM) {
        debug_printf(
            "bws_srv_disconnect() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_disconnect() <<< ret = "
                     "BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_disconnect() <<< ret = "
                     "BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_disconnect() <<< ret = BACNET_WEBSOCKET_CLOSED\n");
        return BACNET_WEBSOCKET_CLOSED;
    } else if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_disconnect() <<< ret = "
                     "BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    } else if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        // some thread has already started disconnect process.
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_disconnect() <<< ret = "
                     "BACNET_WEBSOCKET_OPERATION_IN_PROGRESS\n");
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    } else if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        if (bws_srv_conn[h].wait_threads_cnt == 0) {
            bws_srv_free_connection(h);
        }
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_disconnect() <<< ret = BACNET_WEBSOCKET_CLOSED\n");
        return BACNET_WEBSOCKET_CLOSED;
    } else {
        bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_DISCONNECTING;
        // signal worker to process change of connection state
        lws_cancel_service(bws_srv_ctx);
        // let's wait while worker thread processes changes
        debug_printf(
            "bws_srv_disconnect() going to block on pthread_cond_wait()\n");
        bws_srv_conn[h].wait_threads_cnt++;
        while (bws_srv_conn[h].state != BACNET_WEBSOCKET_STATE_DISCONNECTED) {
            debug_printf(
                "bws_srv_disconnect() block socket %d state %d cond = %p\n", h,
                bws_srv_conn[h].state, &bws_srv_conn[h].cond);
            pthread_cond_wait(&bws_srv_conn[h].cond, &bws_srv_mutex);
            debug_printf("bws_srv_disconnect() unblocked socket %d state %d\n",
                h, bws_srv_conn[h].state);
        }
        debug_printf("bws_srv_disconnect() unblocked\n");
        bws_srv_conn[h].wait_threads_cnt--;
        if (bws_srv_conn[h].wait_threads_cnt == 0) {
            bws_srv_free_connection(h);
        }
    }

    pthread_mutex_unlock(&bws_srv_mutex);
    debug_printf("bws_srv_disconnect() <<< ret = BACNET_WEBSOCKET_SUCCESS\n");
    return BACNET_WEBSOCKET_SUCCESS;
}

BACNET_WEBSOCKET_RET bws_srv_send(
    BACNET_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size)
{
    BACNET_WEBSOCKET_OPERATION_ENTRY e;
    debug_printf("bws_srv_send() >>> h = %d, payload = %p, size = %d\n", h,
        payload, payload_size);

    if (h < 0 || h >= BACNET_SERVER_WEBSOCKETS_MAX_NUM) {
        debug_printf("bws_srv_send() <<< BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!payload || !payload_size) {
        debug_printf("bws_srv_send() <<< BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_send() <<< BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_send() <<< BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_send() <<< BACNET_WEBSOCKET_CLOSED\n");
        return BACNET_WEBSOCKET_CLOSED;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_send() <<< BACNET_WEBSOCKET_OPERATION_IN_PROGRESS\n");
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_send() <<< BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_send() <<< BACNET_WEBSOCKET_CLOSED\n");
        return BACNET_WEBSOCKET_CLOSED;
    }

    if (!bws_srv_init_operation(&e)) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_send() <<< BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload = malloc(payload_size + LWS_PRE);

    if (!e.payload) {
        pthread_mutex_unlock(&bws_srv_mutex);
        bws_srv_deinit_operation(&e);
        debug_printf("bws_srv_send() <<< BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload_size = payload_size;
    memcpy(&e.payload[LWS_PRE], payload, payload_size);
    bws_srv_enqueue_send_operation(&bws_srv_conn[h], &e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_srv_ctx);

    // now wait until libwebsockets runloop processes write request
    debug_printf("bws_srv_send() going to block on pthread_cond_wait\n");

    while (!e.processed) {
        pthread_cond_wait(&e.cond, &bws_srv_mutex);
    }

    debug_printf("bws_srv_send() unblocked\n");
    pthread_mutex_unlock(&bws_srv_mutex);
    free(e.payload);
    bws_srv_deinit_operation(&e);
    debug_printf("bws_srv_send() <<< ret = %d\n", e.retcode);
    return e.retcode;
}

BACNET_WEBSOCKET_RET bws_srv_recv(BACNET_WEBSOCKET_HANDLE h,
    uint8_t *buf,
    size_t bufsize,
    size_t *bytes_received,
    int timeout)
{
    BACNET_WEBSOCKET_OPERATION_ENTRY e;
    struct timespec to;

    debug_printf(
        "bws_srv_recv() >>> h = %d, buf = %p, bufsize = %d, timeout = %d\n", h,
        buf, bufsize, timeout);

    if (h < 0 || h >= BACNET_SERVER_WEBSOCKETS_MAX_NUM) {
        debug_printf("bws_srv_recv() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!buf || !bufsize || !bytes_received) {
        debug_printf("bws_srv_recv() <<< ret = BACNET_WEBSOCKET_BAD_PARAM\n");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_recv() <<< ret = BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_recv() <<< ret = BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_recv() <<< ret = BACNET_WEBSOCKET_CLOSED\n");
        return BACNET_WEBSOCKET_CLOSED;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_recv() <<< ret = BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_recv() <<< ret = "
                     "BACNET_WEBSOCKET_OPERATION_IN_PROGRESS\n");
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        if (bws_srv_conn[h].wait_threads_cnt == 0) {
            bws_srv_free_connection(h);
        }
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf("bws_srv_recv() <<< ret = BACNET_WEBSOCKET_CLOSED\n");
        return BACNET_WEBSOCKET_CLOSED;
    }

    if (!bws_srv_init_operation(&e)) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_recv() <<< ret = BACNET_WEBSOCKET_NO_RESOURCES\n");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload = buf;
    e.payload_size = bufsize;
    bws_srv_enqueue_recv_operation(&bws_srv_conn[h], &e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_srv_ctx);

    // now wait until libwebsockets runloop processes write request
    clock_gettime(CLOCK_REALTIME, &to);

    to.tv_sec = to.tv_sec + timeout / 1000;
    to.tv_nsec = to.tv_nsec + (timeout % 1000) * 1000000;
    to.tv_sec += to.tv_nsec / 1000000000;
    to.tv_nsec %= 1000000000;
    debug_printf("bws_srv_recv() going to block on pthread_cond_timedwait()\n");

    while (!e.processed) {
        if (pthread_cond_timedwait(&e.cond, &bws_srv_mutex, &to) == ETIMEDOUT) {
            pthread_mutex_unlock(&bws_srv_mutex);
            bws_srv_deinit_operation(&e);
            debug_printf(
                "bws_srv_recv() <<< ret = BACNET_WEBSOCKET_TIMEDOUT\n");
            return BACNET_WEBSOCKET_TIMEDOUT;
        }
    }

    debug_printf("bws_srv_recv() unblocked\n");
    pthread_mutex_unlock(&bws_srv_mutex);

    if (e.retcode == BACNET_WEBSOCKET_SUCCESS) {
        *bytes_received = e.payload_size;
    }

    bws_srv_deinit_operation(&e);
    debug_printf("bws_srv_recv() <<< ret = %d\n", e.retcode);
    return e.retcode;
}

BACNET_WEBSOCKET_RET bws_srv_stop(void)
{
    debug_printf("bws_srv_stop() >>>\n");
    pthread_mutex_lock(&bws_srv_mutex);

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_stop() <<< ret = BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        debug_printf(
            "bws_srv_stop() <<< ret = BACNET_WEBSOCKET_INVALID_OPERATION\n");
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    bws_srv_stop_worker = true;

    // wake up libwebsockets runloop
    lws_cancel_service(bws_srv_ctx);

    pthread_mutex_unlock(&bws_srv_mutex);

    // wait while worker terminates
    debug_printf("bws_srv_stop() waiting while worker thread terminates\n");
    pthread_join(bws_srv_worker_thread_id, NULL);

    debug_printf("bws_srv_stop() <<< ret = BACNET_WEBSOCKET_SUCCESS\n");
    return BACNET_WEBSOCKET_SUCCESS;
}

static BACNET_WEBSOCKET_SERVER bws_srv = { bws_srv_start, bws_srv_accept,
    bws_srv_disconnect, bws_srv_send, bws_srv_recv, bws_srv_stop };

BACNET_WEBSOCKET_SERVER *bws_srv_get(void)
{
    return &bws_srv;
}