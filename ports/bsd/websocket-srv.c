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

#if BACNET_WEBSOCKET_SERVER_DEBUG_ENABLED
static bool bws_srv_debug = true;
#else
static bool bws_srv_debug = false;
#endif

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

static int bws_websocket_event(struct lws *wsi,
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
} BACNET_WEBSOCKET_CONNECTION;

static struct lws_protocols bws_srv_protos[] = {
    { BACNET_WEBSOCKET_HUB_PROTOCOL, bws_websocket_event, 0, 0, 0, NULL, 0 },
    { BACNET_WEBSOCKET_DIRECT_CONNECTION_PROTOCOL, bws_websocket_event, 0, 0, 0,
        NULL, 0 },
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

static bool bws_init_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    memset(e, 0, sizeof(*e));

    if (pthread_cond_init(&e->cond, NULL) != 0) {
        return false;
    }
    e->h = BACNET_WEBSOCKET_INVALID_HANDLE;
    return true;
}

static void bws_deinit_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    if (e) {
        pthread_cond_destroy(&e->cond);
    }
}

static void bws_enqueue_accept_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    if (bws_accept_head == NULL && bws_accept_tail == NULL) {
        bws_accept_head = e;
        bws_accept_tail = e;
        e->next = NULL;
        e->last = NULL;
    } else if (bws_accept_head == bws_accept_tail) {
        e->last = bws_accept_head;
        e->next = NULL;
        bws_accept_head->next = e;
        bws_accept_tail = e;
    } else {
        e->last = bws_accept_tail;
        e->next = NULL;
        bws_accept_tail->next = e;
    }
}

static void bws_dequeue_accept_operation(void)
{
    if (bws_accept_head == bws_accept_tail) {
        bws_accept_head = NULL;
        bws_accept_tail = NULL;
    } else {
        bws_accept_head->next->last = NULL;
        bws_accept_head = bws_accept_head->next;
    }
}

static void bws_dequeue_all_accept_operations(void)
{
    while (bws_accept_head) {
        bws_accept_head->retcode = BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
        bws_accept_head->processed = 1;
        pthread_cond_signal(&bws_accept_head->cond);
        bws_dequeue_accept_operation();
    }
}

static void bws_enqueue_recv_operation(
    BACNET_WEBSOCKET_CONNECTION *c, BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    if (c->recv_head == NULL && c->recv_tail == NULL) {
        c->recv_head = e;
        c->recv_tail = e;
        e->next = NULL;
        e->last = NULL;
    } else if (c->recv_head == c->recv_tail) {
        e->last = c->recv_head;
        e->next = NULL;
        c->recv_head->next = e;
        c->recv_tail = e;
    } else {
        e->last = c->recv_tail;
        e->next = NULL;
        c->recv_tail->next = e;
    }
}

static void bws_dequeue_recv_operation(BACNET_WEBSOCKET_CONNECTION *c)
{
    if (c->recv_head == c->recv_tail) {
        c->recv_head = NULL;
        c->recv_tail = NULL;
    } else {
        c->recv_head->next->last = NULL;
        c->recv_head = c->recv_head->next;
    }
}

static void bws_dequeue_all_recv_operations(BACNET_WEBSOCKET_CONNECTION *c)
{
    while (c->recv_head) {
        c->recv_head->retcode = BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
        c->recv_head->processed = 1;
        pthread_cond_signal(&c->recv_head->cond);
        bws_dequeue_recv_operation(c);
    }
}

static void bws_enqueue_send_operation(
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
        c->send_head->next = e;
        c->send_tail = e;
    } else {
        e->last = c->send_tail;
        e->next = NULL;
        c->send_tail->next = e;
    }
}

static void bws_dequeue_send_operation(BACNET_WEBSOCKET_CONNECTION *c)
{
    if (c->send_head == c->send_tail) {
        c->send_head = NULL;
        c->send_tail = NULL;
    } else {
        c->send_head->next->last = NULL;
        c->send_head = c->send_head->next;
    }
}

static void bws_dequeue_all_send_operations(BACNET_WEBSOCKET_CONNECTION *c)
{
    while (c->send_head) {
        c->send_head->retcode = BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
        c->send_head->processed = 1;
        pthread_cond_signal(&c->send_head->cond);
        bws_dequeue_send_operation(c);
    }
}

static BACNET_WEBSOCKET_HANDLE bws_alloc_connection(void)
{
    int ret;
    int i;

    for (int i = 0; i < BACNET_SERVER_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_srv_conn[i].state == BACNET_WEBSOCKET_STATE_IDLE) {
            memset(&bws_srv_conn[i], 0, sizeof(bws_srv_conn[i]));
            if (pthread_cond_init(&bws_srv_conn[i].cond, NULL) != 0) {
                return BACNET_WEBSOCKET_INVALID_HANDLE;
            }
            FIFO_Init(&bws_srv_conn[i].in_data, bws_srv_conn[i].in_data_buf,
                sizeof(bws_srv_conn[i].in_data_buf));
            return i;
        }
    }
    return BACNET_WEBSOCKET_INVALID_HANDLE;
}

static void bws_free_connection(BACNET_WEBSOCKET_HANDLE h)
{
    if (h >= 0 && h < BACNET_SERVER_WEBSOCKETS_MAX_NUM) {
        if (bws_srv_conn[h].state != BACNET_WEBSOCKET_STATE_IDLE) {
            bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_IDLE;
            pthread_cond_destroy(&bws_srv_conn[h].cond);
            bws_srv_conn[h].ws = NULL;
        }
    }
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

static int bws_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BACNET_WEBSOCKET_HANDLE h;
    int written;
    int ret = 0;

    pthread_mutex_lock(&bws_srv_mutex);

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            h = bws_alloc_connection();
            if (h == BACNET_WEBSOCKET_INVALID_HANDLE) {
                pthread_mutex_unlock(&bws_srv_mutex);
                return -1;
            }
            bws_srv_conn[h].ws = wsi;
            bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_CONNECTING;
            // wakeup worker to process pending vent
            lws_cancel_service(bws_srv_ctx);
            break;
        }
        case LWS_CALLBACK_CLOSED: {
            h = bws_find_connnection(wsi);
            if (h != BACNET_WEBSOCKET_INVALID_HANDLE) {
                bws_dequeue_all_recv_operations(&bws_srv_conn[h]);
                bws_dequeue_all_send_operations(&bws_srv_conn[h]);
                if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_DISCONNECTING) {
                    bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_DISCONNECTED;
                    pthread_cond_signal(&bws_srv_conn[h].cond);
                } else if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_CONNECTED) {
                    bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_DISCONNECTED;
                } else if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_CONNECTING) {
                    bws_free_connection(h);
                }
            }
            break;
        }
        case LWS_CALLBACK_RECEIVE: {
            if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTED) {
                if (FIFO_Add(&bws_srv_conn[h].in_data, in, len)) {
                    // wakeup worker to process incoming data
                    lws_cancel_service(bws_srv_ctx);
                } else {
                    // TODO: add logginng here
                }
            }
            break;
        }
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            h = bws_find_connnection(wsi);
            if (h != BACNET_WEBSOCKET_INVALID_HANDLE) {
                if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_DISCONNECTING) {
                    return -1;
                } else if (bws_srv_conn[h].state ==
                    BACNET_WEBSOCKET_STATE_CONNECTED) {
                    if (bws_srv_conn[h].send_head) {
                        written = lws_write(bws_srv_conn[h].ws,
                            &bws_srv_conn[h].send_head->payload[LWS_PRE],
                            bws_srv_conn[h].send_head->payload_size,
                            LWS_WRITE_BINARY);
                        if (written <
                            (int)bws_srv_conn[h].send_head->payload_size) {
                            bws_srv_conn[h].send_head->retcode =
                                BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
                            ret = -1;
                        } else {
                            bws_srv_conn[h].send_head->retcode =
                                BACNET_WEBSOCKET_SUCCESS;
                        }
                        bws_srv_conn[h].send_head->processed = 1;
                        pthread_cond_signal(&bws_srv_conn[h].send_head->cond);
                        bws_dequeue_send_operation(&bws_srv_conn[h]);
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
    return ret;
}

static void *bws_srv_worker(void *arg)
{
    int i;

    while (1) {
        pthread_mutex_lock(&bws_srv_mutex);

        if (bws_srv_stop_worker) {
            bws_dequeue_all_accept_operations();
            for (i = 0; i < BACNET_SERVER_WEBSOCKETS_MAX_NUM; i++) {
                bws_dequeue_all_recv_operations(&bws_srv_conn[i]);
                bws_dequeue_all_send_operations(&bws_srv_conn[i]);
                if (bws_srv_conn[i].state ==
                    BACNET_WEBSOCKET_STATE_DISCONNECTING) {
                    bws_srv_conn[i].state = BACNET_WEBSOCKET_STATE_DISCONNECTED;
                    pthread_cond_signal(&bws_srv_conn[i].cond);
                } else {
                    bws_free_connection(i);
                }
            }
            lws_context_destroy(bws_srv_ctx);
            bws_srv_ctx = NULL;
            bws_srv_stop_worker = false;
            pthread_mutex_unlock(&bws_srv_mutex);
            break;
        }

        for (i = 0; i < BACNET_SERVER_WEBSOCKETS_MAX_NUM; i++) {
            if (bws_srv_conn[i].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
                if (bws_accept_head != NULL) {
                    bws_accept_head->retcode = BACNET_WEBSOCKET_SUCCESS;
                    bws_accept_head->processed = 1;
                    bws_accept_head->h = i;
                    pthread_cond_signal(&bws_accept_head->cond);
                    bws_dequeue_accept_operation();
                }
            } else if (bws_srv_conn[i].state ==
                BACNET_WEBSOCKET_STATE_DISCONNECTING) {
                lws_callback_on_writable(bws_srv_conn[i].ws);
            } else if (bws_srv_conn[i].state ==
                BACNET_WEBSOCKET_STATE_CONNECTED) {
                if (bws_srv_conn[i].send_head) {
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
                    bws_dequeue_recv_operation(&bws_srv_conn[i]);
                }
            }
        }
        pthread_mutex_unlock(&bws_srv_mutex);
        lws_service(bws_srv_ctx, 0);
    }

    return NULL;
}

BACNET_WEBSOCKET_RET bws_srv_start(int port,
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size)
{
    struct lws_context_creation_info info = { 0 };
    int ret;

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (port < 0 || port > 65535) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_debug) {
        lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);
    }

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

    bws_srv_ctx = lws_create_context(&info);

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    ret =
        pthread_create(&bws_srv_worker_thread_id, NULL, &bws_srv_worker, NULL);

    if (ret != 0) {
        lws_context_destroy(bws_srv_ctx);
        bws_srv_ctx = NULL;
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    pthread_mutex_unlock(&bws_srv_mutex);
    return BACNET_WEBSOCKET_SUCCESS;
}

BACNET_WEBSOCKET_RET bws_srv_accept(BACNET_WEBSOCKET_HANDLE *out_handle)
{
    BACNET_WEBSOCKET_OPERATION_ENTRY e;

    *out_handle = BACNET_WEBSOCKET_INVALID_HANDLE;
    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_init_operation(&e)) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    bws_enqueue_accept_operation(&e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_srv_ctx);

    // now wait for a new client connection

    while (!e.processed) {
        pthread_cond_wait(&e.cond, &bws_srv_mutex);
    }

    *out_handle = e.h;
    pthread_mutex_unlock(&bws_srv_mutex);
    bws_deinit_operation(&e);
    return e.retcode;
}

BACNET_WEBSOCKET_RET bws_srv_disconnect(BACNET_WEBSOCKET_HANDLE h)
{
    if (h < 0 || h >= BACNET_SERVER_WEBSOCKETS_MAX_NUM) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    } else if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    } else if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        // some thread has already started disconnect process.
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    } else if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        bws_free_connection(h);
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    } else {
        bws_srv_conn[h].state = BACNET_WEBSOCKET_STATE_DISCONNECTING;
        // signal worker to process change of connection state
        lws_cancel_service(bws_srv_ctx);
        // let's wait while worker thread processes changes
        while (bws_srv_conn[h].state != BACNET_WEBSOCKET_STATE_DISCONNECTED) {
            pthread_cond_wait(&bws_srv_conn[h].cond, &bws_srv_mutex);
        }
        bws_free_connection(h);
    }

    pthread_mutex_unlock(&bws_srv_mutex);
    return BACNET_WEBSOCKET_SUCCESS;
}

BACNET_WEBSOCKET_RET bws_srv_send(
    BACNET_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size)
{
    BACNET_WEBSOCKET_OPERATION_ENTRY e;

    if (h < 0 || h >= BACNET_SERVER_WEBSOCKETS_MAX_NUM) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!payload || !payload_size) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        bws_free_connection(h);
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    }

    if (!bws_init_operation(&e)) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload = malloc(payload_size + LWS_PRE);

    if (!e.payload) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload_size = payload_size;
    memcpy(&e.payload[LWS_PRE], payload, payload_size);
    bws_enqueue_send_operation(&bws_srv_conn[h], &e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_srv_ctx);

    // now wait until libwebsockets runloop processes write request

    while (!e.processed) {
        pthread_cond_wait(&e.cond, &bws_srv_mutex);
    }

    pthread_mutex_unlock(&bws_srv_mutex);
    free(e.payload);
    bws_deinit_operation(&e);
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

    if (h < 0 || h >= BACNET_SERVER_WEBSOCKETS_MAX_NUM) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!buf || !bufsize || !bytes_received) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_srv_mutex);

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    if (bws_srv_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        bws_free_connection(h);
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    }

    e.payload = buf;
    e.payload_size = bufsize;
    bws_enqueue_recv_operation(&bws_srv_conn[h], &e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_srv_ctx);

    // now wait until libwebsockets runloop processes write request
    clock_gettime(CLOCK_REALTIME, &to);

    to.tv_sec = to.tv_sec + timeout / 1000;
    to.tv_nsec = to.tv_nsec + (timeout % 1000) * 1000000;
    to.tv_sec += to.tv_nsec / 1000000000;
    to.tv_nsec %= 1000000000;

    while (!e.processed) {
        if (pthread_cond_timedwait(&e.cond, &bws_srv_mutex, &to) == ETIMEDOUT) {
            pthread_mutex_unlock(&bws_srv_mutex);
            bws_deinit_operation(&e);
            return BACNET_WEBSOCKET_TIMEDOUT;
        }
    }

    pthread_mutex_unlock(&bws_srv_mutex);

    if (e.retcode == BACNET_WEBSOCKET_SUCCESS) {
        *bytes_received = e.payload_size;
    }

    bws_deinit_operation(&e);
    return e.retcode;
}

BACNET_WEBSOCKET_RET bws_srv_stop(void)
{
    pthread_mutex_lock(&bws_srv_mutex);

    if (!bws_srv_ctx) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    if (bws_srv_stop_worker) {
        pthread_mutex_unlock(&bws_srv_mutex);
        return BACNET_WEBSOCKET_INVALID_OPERATION;
    }

    bws_srv_stop_worker = true;

    // wake up libwebsockets runloop
    lws_cancel_service(bws_srv_ctx);

    pthread_mutex_unlock(&bws_srv_mutex);

    // wailt while worker terminates
    pthread_join(bws_srv_worker_thread_id, NULL);
    return BACNET_WEBSOCKET_SUCCESS;
}

static BACNET_WEBSOCKET_SERVER bws_srv = { bws_srv_start, bws_srv_accept,
    bws_srv_disconnect, bws_srv_send, bws_srv_recv, bws_srv_stop };

BACNET_WEBSOCKET_SERVER *bws_srv_get(void)
{
    return &bws_srv;
}