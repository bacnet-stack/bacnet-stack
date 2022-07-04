#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bacnet/datalink/websocket.h"
#include "bacnet/basic/sys/fifo.h"

typedef enum {
    BACNET_WEBSOCKET_STATE_IDLE = 0,
    BACNET_WEBSOCKET_STATE_CONNECTING,
    BACNET_WEBSOCKET_STATE_CONNECTED,
    BACNET_WEBSOCKET_STATE_DISCONNECTING,
    BACNET_WEBSOCKET_STATE_DISCONNECTED,
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
    FIFO_BUFFER in_data;
    FIFO_DATA_STORE(in_data_buf, BACNET_CLIENT_WEBSOCKET_RX_BUFFER_SIZE);
    BACNET_WEBSOCKET_EVENTS events;
    BACNET_WEBSOCKET_CONNECTION_TYPE type;
} BACNET_WEBSOCKET_CONNECTION;

#if BACNET_WEBSOCKET_CLIENT_DEBUG_ENABLED
static bool bws_cli_debug = true;
#else
static bool bws_cli_debug = false;
#endif
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

static pthread_mutex_t bws_cli_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

static struct lws_protocols bws_cli_protos[] = {
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
    bws_cli_conn[BACNET_CLIENT_WEBSOCKETS_MAX_NUM] = { 0 };

static bool bws_init_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    memset(e, 0, sizeof(*e));

    if (pthread_cond_init(&e->cond, NULL) != 0) {
        return false;
    }

    return true;
}

static void bws_deinit_operation(BACNET_WEBSOCKET_OPERATION_ENTRY *e)
{
    if (e) {
        pthread_cond_destroy(&e->cond);
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

static BACNET_WEBSOCKET_HANDLE bws_alloc_connection(void)
{
    int ret;
    int i;

    for (int i = 0; i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_cli_conn[i].state == BACNET_WEBSOCKET_STATE_IDLE) {
            memset(&bws_cli_conn[i], 0, sizeof(bws_cli_conn[i]));
            if (pthread_cond_init(&bws_cli_conn[i].cond, NULL) != 0) {
                return BACNET_WEBSOCKET_INVALID_HANDLE;
            }

            FIFO_Init(&bws_cli_conn[i].in_data, bws_cli_conn[i].in_data_buf,
                sizeof(bws_cli_conn[i].in_data_buf));
            return i;
        }
    }
    return BACNET_WEBSOCKET_INVALID_HANDLE;
}

static void bws_free_connection_ptr(BACNET_WEBSOCKET_CONNECTION *c)
{
    if (c->state != BACNET_WEBSOCKET_STATE_IDLE) {
        c->state = BACNET_WEBSOCKET_STATE_IDLE;
        pthread_cond_destroy(&c->cond);
        c->ws = NULL;
    }
}

static void bws_free_connection(BACNET_WEBSOCKET_HANDLE h)
{
    if (h >= 0 && h < BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        bws_free_connection_ptr(&bws_cli_conn[h]);
    }
}

static BACNET_WEBSOCKET_HANDLE bws_find_connnection(struct lws *ws)
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

static int bws_websocket_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BACNET_WEBSOCKET_HANDLE h;
    int written;

    pthread_mutex_lock(&bws_cli_mutex);
    h = bws_find_connnection(wsi);

    if (h == BACNET_WEBSOCKET_INVALID_HANDLE) {
        pthread_mutex_unlock(&bws_cli_mutex);
        return 0;
    }

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: {
            bws_cli_conn[h].events.established = 1;
            // wakeup worker to process incoming connection event
            lws_cancel_service(bws_cli_conn[h].ctx);
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTED) {
                if (FIFO_Add(&bws_cli_conn[h].in_data, in, len)) {
                    bws_cli_conn[h].events.data_received = 1;
                    // wakeup worker to process incoming data
                    lws_cancel_service(bws_cli_conn[h].ctx);
                } else {
                    // TODO: add logginng here
                }
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTED) {
                if (bws_cli_conn[h].send_head) {
                    written = lws_write(bws_cli_conn[h].ws,
                        &bws_cli_conn[h].send_head->payload[LWS_PRE],
                        bws_cli_conn[h].send_head->payload_size,
                        LWS_WRITE_BINARY);

                    if (written <
                        (int)bws_cli_conn[h].send_head->payload_size) {
                        bws_cli_conn[h].events.closed = 1;
                        bws_cli_conn[h].send_head->retcode =
                            BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
                    } else {
                        bws_cli_conn[h].send_head->retcode =
                            BACNET_WEBSOCKET_SUCCESS;
                    }

                    bws_cli_conn[h].send_head->processed = 1;
                    pthread_cond_signal(&bws_cli_conn[h].send_head->cond);
                    bws_dequeue_send_operation(&bws_cli_conn[h]);
                    // wakeup worker to process internal state
                    lws_cancel_service(bws_cli_conn[h].ctx);
                }
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
            bws_cli_conn[h].events.closed = 1;
            // wakeup worker to process pending vent
            lws_cancel_service(bws_cli_conn[h].ctx);
            break;
        }
        case LWS_CALLBACK_EVENT_WAIT_CANCELLED: {
            break;
        }
        default: {
            break;
        }
    }
    pthread_mutex_unlock(&bws_cli_mutex);
    return 0;
}

static void *bws_cli_worker(void *arg)
{
    BACNET_WEBSOCKET_CONNECTION *conn = (BACNET_WEBSOCKET_CONNECTION *)arg;
    BACNET_WEBSOCKET_STATE old_state;

    while (1) {
        pthread_mutex_lock(&bws_cli_mutex);

        if (conn->events.closed ||
            conn->state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
            conn->events.closed = 0;
            old_state = conn->state;
            conn->state = BACNET_WEBSOCKET_STATE_DISCONNECTED;
            bws_dequeue_all_send_operations(conn);
            bws_dequeue_all_recv_operations(conn);
            // Note! lws_conext_destroy() implicitly calls bws_websocket_event()
            // callback. So connection state must be changed before that call.
            lws_context_destroy(conn->ctx);
            // if websocket is in BACNET_WEBSOCKET_STATE_CONNECTING or in
            // BACNET_WEBSOCKET_STATE_DISCONNECTING it means that some thread
            // (or even threads) is (are) blocked and waiting for result. That's
            // why pthread_cond_broadcast() used there.
            pthread_cond_broadcast(&conn->cond);

            if (old_state == BACNET_WEBSOCKET_STATE_CONNECTED) {
                bws_free_connection_ptr(conn);
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
            if (conn->send_head) {
                lws_callback_on_writable(conn->ws);
            }
            while (conn->recv_head && !FIFO_Empty(&conn->in_data)) {
                conn->recv_head->payload_size = FIFO_Pull(&conn->in_data,
                    conn->recv_head->payload, conn->recv_head->payload_size);
                conn->recv_head->processed = 1;
                pthread_cond_signal(&conn->recv_head->cond);
                bws_dequeue_recv_operation(conn);
            }
        }
        pthread_mutex_unlock(&bws_cli_mutex);
        lws_service(conn->ctx, 0);
    }

    return NULL;
}

BACNET_WEBSOCKET_RET bws_connect(BACNET_WEBSOCKET_CONNECTION_TYPE type,
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
    const char *prot, *addr, *path;
    int port;
    BACNET_WEBSOCKET_HANDLE h;
    struct lws_client_connect_info cinfo = { 0 };
    BACNET_WEBSOCKET_RET ret;

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size ||
        !url || !out_handle) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (type != BACNET_WEBSOCKET_HUB_CONNECTION &&
        type != BACNET_WEBSOCKET_DIRECT_CONNECTION) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    strncpy(tmp_url, url, BACNET_WSURL_MAX_LEN);

    if (strcmp(prot, "wss") != 0) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_cli_mutex);

    if (bws_cli_debug) {
        lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);
    }

    if (lws_parse_uri(tmp_url, &prot, &addr, &port, &path) != 0) {
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    h = bws_alloc_connection();

    if (h == BACNET_WEBSOCKET_INVALID_HANDLE) {
        pthread_mutex_unlock(&bws_cli_mutex);
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

    bws_cli_conn[h].ctx = lws_create_context(&info);

    if (!bws_cli_conn[h].ctx) {
        bws_free_connection(h);
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    ret = pthread_create(
        &bws_cli_conn[h].thread_id, NULL, &bws_cli_worker, bws_cli_conn[h].ctx);

    if (ret != 0) {
        lws_context_destroy(bws_cli_conn[h].ctx);
        pthread_mutex_unlock(&bws_cli_mutex);
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
    cinfo.ssl_connection =
        LCCSCF_USE_SSL | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;

    if (type == BACNET_WEBSOCKET_HUB_CONNECTION) {
        cinfo.protocol = bws_hub_protocol;
    } else {
        cinfo.protocol = bws_direct_protocol;
    }

    bws_cli_conn[h].type = type;
    bws_cli_conn[h].state = BACNET_WEBSOCKET_STATE_CONNECTING;
    lws_client_connect_via_info(&cinfo);

    // let's unblock worker thread and wait for connection over websocket
    while (bws_cli_conn[h].state != BACNET_WEBSOCKET_STATE_CONNECTED &&
        bws_cli_conn[h].state != BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        pthread_cond_wait(&bws_cli_conn[h].cond, &bws_cli_mutex);
    }

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_CONNECTED) {
        *out_handle = h;
        ret = BACNET_WEBSOCKET_SUCCESS;
    } else {
        bws_free_connection(h);
        *out_handle = BACNET_WEBSOCKET_INVALID_HANDLE;
        ret = BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    }

    pthread_mutex_unlock(&bws_cli_mutex);
    return ret;
}

BACNET_WEBSOCKET_RET bws_disconnect(BACNET_WEBSOCKET_HANDLE h)
{
    if (h < 0 || h >= BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_cli_mutex);

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE) {
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    } else if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING ||
        bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        // some thread has already started disconnect process.
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    } else {
        bws_cli_conn[h].state = BACNET_WEBSOCKET_STATE_DISCONNECTING;
        // signal worker to process change of connection state
        lws_cancel_service(bws_cli_conn[h].ctx);
        // let's wait while worker thread processes changes
        while (bws_cli_conn[h].state != BACNET_WEBSOCKET_STATE_DISCONNECTED) {
            pthread_cond_wait(&bws_cli_conn[h].cond, &bws_cli_mutex);
        }
        bws_free_connection(h);
    }
    pthread_mutex_unlock(&bws_cli_mutex);
    return BACNET_WEBSOCKET_SUCCESS;
}

BACNET_WEBSOCKET_RET bws_send(
    BACNET_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size)
{
    BACNET_WEBSOCKET_OPERATION_ENTRY e;

    if (h < 0 || h >= BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!payload || !payload_size) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_cli_mutex);

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE ||
        bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    }

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    // user is allowed to send data if websocket connection is already
    // established or it is in a process of establishing

    if (!bws_init_operation(&e)) {
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload = malloc(payload_size + LWS_PRE);

    if (!e.payload) {
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    e.payload_size = payload_size;
    memcpy(&e.payload[LWS_PRE], payload, payload_size);
    bws_enqueue_send_operation(&bws_cli_conn[h], &e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_cli_conn[h].ctx);

    // now wait until libwebsockets runloop processes write request

    while (!e.processed) {
        pthread_cond_wait(&e.cond, &bws_cli_mutex);
    }

    pthread_mutex_unlock(&bws_cli_mutex);
    free(e.payload);
    bws_deinit_operation(&e);
    return e.retcode;
}

BACNET_WEBSOCKET_RET bws_recv(BACNET_WEBSOCKET_HANDLE h,
    uint8_t *buf,
    size_t bufsize,
    size_t *bytes_received,
    int timeout)
{
    BACNET_WEBSOCKET_OPERATION_ENTRY e;
    struct timespec to;

    if (h < 0 || h >= BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!buf || !bufsize || !bytes_received) {
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    pthread_mutex_lock(&bws_cli_mutex);

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_IDLE ||
        bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_WEBSOCKET_CLOSED;
    }

    if (bws_cli_conn[h].state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        pthread_mutex_unlock(&bws_cli_mutex);
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    // user is allowed to recv data if websocket connection is already
    // established or it is in a process of establishing

    e.payload = buf;
    e.payload_size = bufsize;
    bws_enqueue_recv_operation(&bws_cli_conn[h], &e);

    // wake up libwebsockets runloop
    lws_cancel_service(bws_cli_conn[h].ctx);

    // now wait until libwebsockets runloop processes write request
    clock_gettime(CLOCK_REALTIME, &to);

    to.tv_sec = to.tv_sec + timeout / 1000;
    to.tv_nsec = to.tv_nsec + (timeout % 1000) * 1000000;
    to.tv_sec += to.tv_nsec / 1000000000;
    to.tv_nsec %= 1000000000;

    while (!e.processed) {
        if (pthread_cond_timedwait(&e.cond, &bws_cli_mutex, &to) == ETIMEDOUT) {
            pthread_mutex_unlock(&bws_cli_mutex);
            bws_deinit_operation(&e);
            return BACNET_WEBSOCKET_TIMEDOUT;
        }
    }

    pthread_mutex_unlock(&bws_cli_mutex);

    if (e.retcode == BACNET_WEBSOCKET_SUCCESS) {
        *bytes_received = e.payload_size;
    }

    bws_deinit_operation(&e);
    return e.retcode;
}

static BACNET_WEBSOCKET_CLIENT bws_cli = { bws_connect, bws_send, bws_recv,
    bws_disconnect };

BACNET_WEBSOCKET_CLIENT *bws_cli_get(void)
{
    return &bws_cli;
}
