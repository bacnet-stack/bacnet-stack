/**
 * @file
 * @brief Implementation of websocket client interface for Zephyr
 * @author Mikhail Antropov
 * @date Aug 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include <zephyr.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <kernel/thread.h>
#include <fcntl.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <net/websocket.h>
#include <net/tls_credentials.h>
#include <sys_clock.h>
#include <posix/sys/eventfd.h>
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/basic/sys/fifo.h"
#include "bacnet/basic/sys/debug.h"

/* Logging module registration is already done in ports/zephyr/main.c */
#include <logging/log.h>
#include <logging/log_ctrl.h>

LOG_MODULE_DECLARE(bacnet, LOG_LEVEL_DBG);

typedef enum {
    BACNET_WEBSOCKET_STATE_IDLE = 0,
    BACNET_WEBSOCKET_STATE_CONNECTING = 1,
    BACNET_WEBSOCKET_STATE_CONNECTED = 2,
    BACNET_WEBSOCKET_STATE_DISCONNECTING = 3,
    BACNET_WEBSOCKET_STATE_DISCONNECTED = 4
} BACNET_WEBSOCKET_STATE;

#define WEBSOCKET_CLOSE_CODE_1000   {0x03, 0xe8}

#define CONTEXT_PORT_NO_LISTEN  0
#define TLS_PEER_HOSTNAME "localhost"

typedef enum {
    CA_CERTIFICATE_TAG = 0,
    SERVER_CERTIFICATE = 1,
    PRIVATE_KEY = 2,
} TLS_CREDENTIAL_TAGS;

#define TLS_CREDENTIAL_MAXIMUM  10

/* We need to allocate bigger buffer for the websocket data we receive so that
 * the websocket header fits into it.
 */
#define EXTRA_BUF_SPACE 30

#define MAX_RECV_BUF_LEN 1024

typedef struct BACNetWebsocketTask {
    uint16_t queue_reserved;
    uint8_t *payload;
    size_t payload_size;
    size_t received;
    struct k_condvar cond;
    BACNET_WEBSOCKET_RET retcode;
} BACNET_WEBSOCKET_TASK;

typedef struct {
    int sock;
    int websock;
    struct k_queue sends;
    struct k_queue recvs;
    uint8_t buf[MAX_RECV_BUF_LEN + EXTRA_BUF_SPACE];
    BACNET_WEBSOCKET_STATE state;
} BACNET_WEBSOCKET_CONNECTION;

BUILD_ASSERT(BACNET_CLIENT_WEBSOCKETS_MAX_NUM < CONFIG_NET_SOCKETS_POLL_MAX,
    "BACNET_CLIENT_WEBSOCKETS_MAX_NUM must be greater than CONFIG_NET_SOCKETS_POLL_MAX");

static BACNET_WEBSOCKET_CONNECTION
    bws_cli_conn[BACNET_CLIENT_WEBSOCKETS_MAX_NUM] = { 0 };
static int fds_num = 0;
static struct zsock_pollfd fds[BACNET_CLIENT_WEBSOCKETS_MAX_NUM +1] = { 0 };

#define EVENT_ID_CHANGED_LIST   1
#define EVENT_ID_SEND           10

static int worker_event_fd = -1;

#define STACKSIZE (4096 + CONFIG_TEST_EXTRA_STACKSIZE)

K_MUTEX_DEFINE(bws_cli_mutex);

static BACNET_WEBSOCKET_HANDLE bws_cli_alloc_connection(void)
{
    int i;
    for (i = 0; i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_cli_conn[i].state == BACNET_WEBSOCKET_STATE_IDLE) {
            memset(&bws_cli_conn[i], 0, sizeof(bws_cli_conn[i]));
            k_queue_init(&bws_cli_conn[i].sends);
            k_queue_init(&bws_cli_conn[i].recvs);
            bws_cli_conn[i].state = BACNET_WEBSOCKET_STATE_CONNECTING;
            break;
        }
    }

    return i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM ?
        i : BACNET_WEBSOCKET_INVALID_HANDLE;
}

static int setup_socket(uint16_t family, const char *server, int port,
            int *sock, struct sockaddr *addr, socklen_t addr_len,
            BACNET_WEBSOCKET_HANDLE h)
{
    const char *family_str = family == AF_INET ? "IPv4" : "IPv6";
    int ret = 0;

    memset(addr, 0, addr_len);

    if (family == AF_INET) {
        net_sin(addr)->sin_family = AF_INET;
        net_sin(addr)->sin_port = htons(port);
        inet_pton(family, server, &net_sin(addr)->sin_addr);
    } else {
        net_sin6(addr)->sin6_family = AF_INET6;
        net_sin6(addr)->sin6_port = htons(port);
        inet_pton(family, server, &net_sin6(addr)->sin6_addr);
    }

    if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
        sec_tag_t sec_tag_list[] = {
            CA_CERTIFICATE_TAG + TLS_CREDENTIAL_MAXIMUM * h,
            SERVER_CERTIFICATE + TLS_CREDENTIAL_MAXIMUM * h,
            PRIVATE_KEY + TLS_CREDENTIAL_MAXIMUM * h
        };

        *sock = zsock_socket(family, SOCK_STREAM, IPPROTO_TLS_1_2);
        if (*sock >= 0) {
            ret = zsock_setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST,
                     sec_tag_list, sizeof(sec_tag_list));
            if (ret < 0) {
                LOG_ERR("Failed to set %s secure option (%d)",
                    family_str, -errno);
                ret = -errno;
                goto fail;
            }

            ret = zsock_setsockopt(*sock, SOL_TLS, TLS_HOSTNAME,
                     NULL /*server*/, 0 /*strlen(server)*/);
            if (ret < 0) {
                LOG_ERR("Failed to set %s TLS_HOSTNAME "
                    "option (%d)", family_str, -errno);
                ret = -errno;
                goto fail;
            }
        }
    } else {
        *sock = zsock_socket(family, SOCK_STREAM, IPPROTO_TCP);
    }

    if (*sock < 0) {
        LOG_ERR("Failed to create %s HTTP socket (%d)", family_str,
            -errno);
    }

    return ret;

fail:
    if (*sock >= 0) {
        zsock_close(*sock);
        *sock = -1;
    }

    return ret;
}

static int connect_socket(uint16_t family, const char *server, int port,
              int *sock, struct sockaddr *addr, socklen_t addr_len,
              BACNET_WEBSOCKET_HANDLE h)
{
    int ret;

    ret = setup_socket(family, server, port, sock, addr, addr_len, h);
    if (ret < 0 || *sock < 0) {
        return -1;
    }

    ret = zsock_connect(*sock, addr, addr_len);
    if (ret < 0) {
        LOG_ERR("Cannot connect to %s remote (%d)",
            family == AF_INET ? "IPv4" : "IPv6",
            -errno);
        ret = -errno;
    }

    return ret;
}

static void renumber_fds(void)
{
    int i;
    fds_num = 1;
    for (i = 0; i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_cli_conn[i].state == BACNET_WEBSOCKET_STATE_CONNECTED) {
            LOG_INF("Add socket %d for listening", bws_cli_conn[i].sock);
            fds[fds_num].fd = bws_cli_conn[i].sock;
            fds[fds_num].events = ZSOCK_POLLIN;
            fds[fds_num].revents = 0;
            fds_num++;
        }
    }
}

static BACNET_WEBSOCKET_CONNECTION *find_context(int fd)
{
    int i;
    for (i = 0; i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if ((bws_cli_conn[i].state == BACNET_WEBSOCKET_STATE_CONNECTED) &&
            (bws_cli_conn[i].sock == fd)) {
                return &bws_cli_conn[i];
        }
    }
    return NULL;
}

static void worker_event(uint8_t event_id)
{
    uint8_t value = event_id;
    LOG_INF("run worker event %d", event_id);
    zsock_send(worker_event_fd, &value, sizeof(value), 0);
}

static int connect_cb(int websock, struct http_request *req, void *user_data)
{
    int i;
    k_mutex_lock(&bws_cli_mutex, K_FOREVER);
    for (i = 0; i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        if (bws_cli_conn[i].buf == req->recv_buf) {
            bws_cli_conn[i].websock = websock;
            bws_cli_conn[i].state = BACNET_WEBSOCKET_STATE_CONNECTED;
            LOG_INF("Connect %d successed", i);
            break;
        }
    }

    k_mutex_unlock(&bws_cli_mutex);
    return i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM ? 0 :
        BACNET_WEBSOCKET_INVALID_HANDLE;
}

static BACNET_WEBSOCKET_RET bacnet_websocket_retcode(int ret)
{
    return ret >= 0 ? BACNET_WEBSOCKET_SUCCESS
                     : BACNET_WEBSOCKET_INVALID_OPERATION;
}

static int parse_uri(char *p, const char **prot, const char **ads, int *port,
          const char **path)
{
    const char *end;
    char unix_skt = 0;

    /* cut up the location into address, port and path */
    *prot = p;
    while (*p && (*p != ':' || p[1] != '/' || p[2] != '/'))
        p++;
    if (!*p) {
        end = p;
        p = (char *)*prot;
        *prot = end;
    } else {
        *p = '\0';
        p += 3;
    }
    if (*p == '+') /* unix skt */
        unix_skt = 1;

    *ads = p;
    if (!strcmp(*prot, "http") || !strcmp(*prot, "ws"))
        *port = 80;
    else if (!strcmp(*prot, "https") || !strcmp(*prot, "wss"))
        *port = 443;

    if (*p == '[') {
        ++(*ads);
        while (*p && *p != ']')
            p++;
        if (*p)
            *p++ = '\0';
    } else
        while (*p && *p != ':' && (unix_skt || *p != '/'))
            p++;

    if (*p == ':') {
        *p++ = '\0';
        *port = atoi(p);
        while (*p && *p != '/')
            p++;
    }
    *path = "/";
    if (*p) {
        *p++ = '\0';
        if (*p)
            *path = p;
    }

    return 0;
}

static BACNET_WEBSOCKET_RET bws_cli_init(
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size,
    BACNET_WEBSOCKET_HANDLE *out_handle)
{
    BACNET_WEBSOCKET_HANDLE h;
    BACNET_WEBSOCKET_RET retcode = BACNET_WEBSOCKET_SUCCESS;
    int ret;

    LOG_INF("bws_cli_init >>>");
    k_mutex_lock(&bws_cli_mutex, K_FOREVER);

    h = bws_cli_alloc_connection();
    if (h == BACNET_WEBSOCKET_INVALID_HANDLE) {
        LOG_ERR("Cannot allocate connection context");
        retcode = BACNET_WEBSOCKET_NO_RESOURCES;
        goto exit;
    }

    ret = tls_credential_add(CA_CERTIFICATE_TAG + TLS_CREDENTIAL_MAXIMUM * h,
        TLS_CREDENTIAL_CA_CERTIFICATE, ca_cert, ca_cert_size);
    if (ret < 0) {
        LOG_ERR("Failed to register public certificate: %d", ret);
        retcode = BACNET_WEBSOCKET_BAD_PARAM;
        goto exit;
    }

    ret = tls_credential_add(SERVER_CERTIFICATE + TLS_CREDENTIAL_MAXIMUM * h,
        TLS_CREDENTIAL_SERVER_CERTIFICATE, cert, cert_size);
    if (ret < 0) {
        LOG_ERR("Failed to register server certificate: %d", ret);
        retcode = BACNET_WEBSOCKET_BAD_PARAM;
        goto exit;
    }

    ret = tls_credential_add(PRIVATE_KEY + TLS_CREDENTIAL_MAXIMUM * h,
                 TLS_CREDENTIAL_PRIVATE_KEY, key, key_size);
    if (ret < 0) {
        LOG_ERR("Failed to register key certificate: %d", ret);
        retcode = BACNET_WEBSOCKET_BAD_PARAM;
        goto exit;
    }

    *out_handle = h;

exit:
    if ((retcode != BACNET_WEBSOCKET_SUCCESS) &&
        (h != BACNET_WEBSOCKET_INVALID_HANDLE))
    {
        bws_cli_conn[h].state = BACNET_WEBSOCKET_STATE_IDLE;
    }
    k_mutex_unlock(&bws_cli_mutex);
    LOG_INF("bws_cli_init <<< %d", retcode);

    return retcode;
}

static BACNET_WEBSOCKET_RET bws_cli_connect(
    BACNET_WEBSOCKET_PROTOCOL type,
    char *url,
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size,
    BACNET_WEBSOCKET_HANDLE *out_handle)
{
    int sock = -1;
    int websock = -1;
    int32_t timeout = 3 * MSEC_PER_SEC;
    struct sockaddr_in addr_in;
    BACNET_WEBSOCKET_HANDLE h;
    struct websocket_request req = {0};
    BACNET_WEBSOCKET_RET retcode = BACNET_WEBSOCKET_SUCCESS;
    char tmp_url[BACNET_WSURL_MAX_LEN];
    const char *prot = NULL, *addr = NULL, *path = NULL;
    int port = -1;
    int ret;

    char protocol[60];
    const char *extra_headers[] = {
        protocol,
        NULL
    };

    LOG_INF("bws_cli_connect() >>>");

    strncpy(tmp_url, url, BACNET_WSURL_MAX_LEN);
    ret = parse_uri(tmp_url, &prot, &addr, &port, &path);
    if (port == -1 || !prot || !addr || !path) {
        LOG_ERR("bws_cli_connect() <<< BACNET_WEBSOCKET_BAD_PARAM");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    retcode = bws_cli_init(ca_cert, ca_cert_size, cert, cert_size, key,
        key_size, &h);
    if (retcode != BACNET_WEBSOCKET_SUCCESS) {
        return retcode;
    }

    (void)connect_socket(AF_INET, addr, port,
        &sock, (struct sockaddr *)&addr_in, sizeof(addr_in), h);
    if (sock < 0) {
        LOG_ERR("bws_cli_connect() <<< Cannot create HTTP connection");
        return BACNET_WEBSOCKET_NO_RESOURCES;
    }

    bws_cli_conn[h].sock = sock;

    snprintf(protocol, sizeof(protocol),"Sec-WebSocket-Protocol: %s\r\n",
        type == BACNET_WEBSOCKET_HUB_PROTOCOL ?
            BACNET_WEBSOCKET_HUB_PROTOCOL_STR :
            BACNET_WEBSOCKET_DIRECT_PROTOCOL_STR);
    LOG_INF("Websocket protocol = %s", protocol);

    req.host = addr;
    req.url = path;
    req.optional_headers = extra_headers;
    req.cb = connect_cb;
    req.tmp_buf = bws_cli_conn[h].buf;
    req.tmp_buf_len = sizeof(bws_cli_conn[h].buf);

    websock = websocket_connect(sock, &req, timeout, "IPv4");
    if (websock < 0) {
        LOG_ERR("Cannot connect to %s:%d", addr, port);
        zsock_close(sock);
        return BACNET_WEBSOCKET_CLOSED;
    }

    worker_event(EVENT_ID_CHANGED_LIST);

    *out_handle = h;

    LOG_INF("bws_cli_connect() <<<");
    return BACNET_WEBSOCKET_SUCCESS;
}

static void bws_cli_disconnect_impl(BACNET_WEBSOCKET_CONNECTION *ctx)
{
    BACNET_WEBSOCKET_TASK *task;

    while (!k_queue_is_empty(&ctx->recvs)) {
        task = k_queue_get(&ctx->recvs, K_NO_WAIT);
        if (task != NULL) {
            task->retcode = BACNET_WEBSOCKET_CLOSED;
            k_condvar_signal(&task->cond);
        }
    }

    while (!k_queue_is_empty(&ctx->sends)) {
        task = k_queue_get(&ctx->sends, K_NO_WAIT);
        if (task != NULL) {
            task->retcode = BACNET_WEBSOCKET_CLOSED;
            k_condvar_signal(&task->cond);
        }
    }

    websocket_disconnect(ctx->websock);
    ctx->state = BACNET_WEBSOCKET_STATE_DISCONNECTED;
}

static BACNET_WEBSOCKET_RET bws_cli_disconnect(BACNET_WEBSOCKET_HANDLE h)
{
    BACNET_WEBSOCKET_RET ret = BACNET_WEBSOCKET_SUCCESS;
    uint8_t reason[2] = WEBSOCKET_CLOSE_CODE_1000;
    BACNET_WEBSOCKET_CONNECTION *ctx;
    bool need_send_close;

    LOG_INF("bws_cli_disconnect() >>> h = %d", h);

    if (h < 0 || h >= BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        LOG_ERR("bws_cli_disconnect() <<< ret = BACNET_WEBSOCKET_BAD_PARAM");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    k_mutex_lock(&bws_cli_mutex, K_FOREVER);

    ctx = &bws_cli_conn[h];
    if (ctx->state == BACNET_WEBSOCKET_STATE_DISCONNECTING)
        ret = BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    if (ctx->state == BACNET_WEBSOCKET_STATE_IDLE ||
        ctx->state == BACNET_WEBSOCKET_STATE_DISCONNECTED)
        ret = BACNET_WEBSOCKET_CLOSED;
    if (ret != BACNET_WEBSOCKET_SUCCESS)
        goto exit;

    // here the state is conected or connecting
    need_send_close = ctx->state == BACNET_WEBSOCKET_STATE_CONNECTED;
    ctx->state = BACNET_WEBSOCKET_STATE_DISCONNECTING;
    if (need_send_close) {
        websocket_send_msg(ctx->websock, reason, sizeof(reason),
                           WEBSOCKET_OPCODE_CLOSE, true, true, SYS_FOREVER_MS);
    }
    bws_cli_disconnect_impl(ctx);
    k_mutex_unlock(&bws_cli_mutex);

    worker_event(EVENT_ID_CHANGED_LIST);

exit:
    LOG_INF("bws_cli_disconnect() <<< ret = %d", ret);
    return ret;
}

void bws_cli_worker(void *p1, void *p2, void *p3)
{
    int ret = 0;
    int received, len, i;
    uint64_t remaining = ULLONG_MAX;
    uint32_t message_type;
    BACNET_WEBSOCKET_CONNECTION *ctx;
    BACNET_WEBSOCKET_TASK *task;
    bool need_renumber_fds = false;
    uint8_t value;
    int spair[2];

    ret = zsock_socketpair(AF_UNIX, SOCK_STREAM, 0, spair);
    if (ret == 0) {
        worker_event_fd = spair[0];
        fds[0].fd = spair[1];
        fds[0].events = ZSOCK_POLLIN;
        fds[0].revents = 0;
        fds_num = 1;
    } else {
        LOG_ERR("socketpair failed: %d", errno);
        return;
    }

   while (1) {
        LOG_INF("zsock_polling fds_num = %d", fds_num);
        ret = zsock_poll(fds, fds_num, -1);
        LOG_INF("zsock_polled: %d", ret);

        need_renumber_fds = false;

        if (ret == -1) {
            LOG_ERR("zsock_poll error: %d", errno);
            need_renumber_fds = true;
        }

        k_mutex_lock(&bws_cli_mutex, K_FOREVER);

        // receive
        for (i = 1; i < fds_num; i++) {
            LOG_INF("fds %d revents %d", i, fds[i].revents);
            if (fds[i].revents & (ZSOCK_POLLHUP | ZSOCK_POLLERR)) {
                need_renumber_fds = true;
            }
            if (!(fds[i].revents & ZSOCK_POLLIN)) {
                continue;
            }
            ctx = find_context(fds[i].fd);
            if (ctx == NULL) {
                need_renumber_fds = true;
                continue;
            }
            fds[i].revents = 0;
            received = 0;
            remaining = 1;
            message_type = 0;
            while ((remaining > 0) && (received < sizeof(ctx->buf))){
                //remaining = 0;
                ret = websocket_recv_msg(ctx->websock, ctx->buf + received,
                    sizeof(ctx->buf) - received, &message_type, &remaining, 0);
                LOG_DBG("websocket_recv_msg : remaining %lld, ret: %d",
                    remaining, ret);
                if (ret <= 0)
                    break;
                received += ret;
            }

            if (ret < 0) {
                LOG_ERR("Error websocket received: %d", ret);
                continue;
            }

            LOG_DBG("Receive message type: %d, length: %d", message_type,
                received);

            if (message_type & WEBSOCKET_FLAG_PING) {
                ret = websocket_send_msg(ctx->websock, ctx->buf, received,
                    WEBSOCKET_OPCODE_PONG, false, true, SYS_FOREVER_MS);
                LOG_INF("Sent PONG, status %d", ret);
            }

            if (message_type & WEBSOCKET_FLAG_CLOSE) {
                LOG_DBG("Receive message Close");
                bws_cli_disconnect_impl(ctx);
                need_renumber_fds = true;
            }

            if (message_type & (WEBSOCKET_FLAG_TEXT |
                                WEBSOCKET_FLAG_BINARY)) {
                LOG_DBG("Receive message Data");
                if (!k_queue_is_empty(&ctx->recvs)) {
                    task = k_queue_peek_head(&ctx->recvs);
                    len = MIN(received, task->payload_size);
                    memcpy(task->payload, ctx->buf, len);
                    task->received = len;
                    task->retcode = len < received ?
                        BACNET_WEBSOCKET_BUFFER_TOO_SMALL :
                        BACNET_WEBSOCKET_SUCCESS;
                    LOG_DBG("Data len %d received %d retcode %d", len,
                        received, task->retcode);
                    k_condvar_signal(&task->cond);
                }
            }
        }

        // send
        LOG_INF("fds 0 revents %d", fds[0].revents);
        if (fds[0].revents & ZSOCK_POLLIN) {
            fds[0].revents = 0;
            value = 0;
            zsock_recv(fds[0].fd, &value, sizeof(value), MSG_DONTWAIT);
            LOG_INF("worker event happend, value %d", value);

            switch (value) {
                case EVENT_ID_SEND:
                    for (i = 0; i < BACNET_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
                        ctx = &bws_cli_conn[i];
                        if (ctx->state != BACNET_WEBSOCKET_STATE_CONNECTED) {
                            continue;
                        }

                        while (!k_queue_is_empty(&ctx->sends)) {
                            task = k_queue_get(&ctx->sends, K_NO_WAIT);
                            if (task != NULL) {
                                task->retcode = bacnet_websocket_retcode(
                                    websocket_send_msg(ctx->websock, task->payload,
                                        task->payload_size, WEBSOCKET_OPCODE_DATA_BINARY,
                                        false, true, SYS_FOREVER_MS));
                                k_condvar_signal(&task->cond);
                            }
                        }
                    }
                    break;
                case EVENT_ID_CHANGED_LIST:
                    need_renumber_fds = true;
                    break;
            }
        }

        if (need_renumber_fds) {
            renumber_fds();
        }

        k_mutex_unlock(&bws_cli_mutex);
    }
}

static BACNET_WEBSOCKET_RET bws_cli_send(
    BACNET_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size)
{
    BACNET_WEBSOCKET_CONNECTION *ctx;
    BACNET_WEBSOCKET_TASK e;

    LOG_INF("bws_cli_send() >>> h = %d, payload = %p, payload_size = %d",
        h, payload, payload_size);

    if (h < 0 || h >= BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        LOG_INF("bws_cli_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!payload || !payload_size) {
        LOG_INF("bws_cli_send() <<< ret = BACNET_WEBSOCKET_BAD_PARAM");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    k_mutex_lock(&bws_cli_mutex, K_FOREVER);

    ctx = &bws_cli_conn[h];

    if (ctx->state == BACNET_WEBSOCKET_STATE_IDLE ||
        ctx->state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        k_mutex_unlock(&bws_cli_mutex);
        LOG_INF("bws_cli_send() <<< ret = BACNET_WEBSOCKET_CLOSED");
        return BACNET_WEBSOCKET_CLOSED;
    }

    if (ctx->state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        k_mutex_unlock(&bws_cli_mutex);
        LOG_INF("bws_cli_send() <<< ret = "
                     "BACNET_WEBSOCKET_OPERATION_IN_PROGRESS");
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    e.payload = payload;
    e.payload_size = payload_size;
    e.retcode = BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    k_condvar_init(&e.cond);
    k_queue_append(&ctx->sends, &e);

    worker_event(EVENT_ID_SEND);

    while (e.retcode == BACNET_WEBSOCKET_OPERATION_IN_PROGRESS) {
        k_condvar_wait(&e.cond, &bws_cli_mutex, K_FOREVER);
    }

    k_queue_remove(&ctx->sends, &e);
    k_mutex_unlock(&bws_cli_mutex);

    LOG_INF("bws_cli_send() <<< ret = %d", e.retcode);
    return e.retcode;
}

static BACNET_WEBSOCKET_RET bws_cli_recv(BACNET_WEBSOCKET_HANDLE h,
    uint8_t *buf,
    size_t bufsize,
    size_t *bytes_received,
    int timeout)
{
    BACNET_WEBSOCKET_CONNECTION *ctx;
    BACNET_WEBSOCKET_TASK e;
    int ret = 0;

    LOG_INF(
        "bws_cli_recv() >>> h = %d, buf = %p, bufsize = %d, timeout = %d", h,
        buf, bufsize, timeout);
    if (h < 0 || h >= BACNET_CLIENT_WEBSOCKETS_MAX_NUM) {
        LOG_INF("bws_cli_recv() <<< ret = BACNET_WEBSOCKET_BAD_PARAM");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    if (!buf || !bufsize || !bytes_received) {
        LOG_INF("bws_cli_recv() <<< ret = BACNET_WEBSOCKET_BAD_PARAM");
        return BACNET_WEBSOCKET_BAD_PARAM;
    }

    k_mutex_lock(&bws_cli_mutex, K_FOREVER);

    ctx = &bws_cli_conn[h];

    if (ctx->state == BACNET_WEBSOCKET_STATE_IDLE ||
        ctx->state == BACNET_WEBSOCKET_STATE_DISCONNECTED) {
        k_mutex_unlock(&bws_cli_mutex);
        LOG_INF("bws_cli_recv() <<< ret = BACNET_WEBSOCKET_CLOSED");
        return BACNET_WEBSOCKET_CLOSED;
    }

    if (ctx->state == BACNET_WEBSOCKET_STATE_DISCONNECTING) {
        k_mutex_unlock(&bws_cli_mutex);
        LOG_INF("bws_cli_recv() <<< ret = "
                     "BACNET_WEBSOCKET_OPERATION_IN_PROGRESS");
        return BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    }

    e.payload = buf;
    e.payload_size = bufsize;
    e.received = 0;
    e.retcode = BACNET_WEBSOCKET_OPERATION_IN_PROGRESS;
    k_condvar_init(&e.cond);
    k_queue_append(&ctx->recvs, &e);

    while (e.retcode == BACNET_WEBSOCKET_OPERATION_IN_PROGRESS) {
        ret = k_condvar_wait(&e.cond, &bws_cli_mutex, K_MSEC(timeout));
    }
    if (ret == ETIMEDOUT) {
        e.retcode = BACNET_WEBSOCKET_TIMEDOUT;
    }

    k_queue_remove(&ctx->recvs, &e);
    k_mutex_unlock(&bws_cli_mutex);

    if (e.retcode == BACNET_WEBSOCKET_SUCCESS ||
        e.retcode == BACNET_WEBSOCKET_BUFFER_TOO_SMALL) {
        *bytes_received = e.payload_size;
    }

    LOG_INF("bws_cli_recv() <<< ret = %d, bytes_received = %d",
        e.retcode, *bytes_received);
    return e.retcode;
}

static BACNET_WEBSOCKET_CLIENT bws_cli = { bws_cli_connect, bws_cli_disconnect,
    bws_cli_send, bws_cli_recv };

BACNET_WEBSOCKET_CLIENT *bws_cli_get(void)
{
    return &bws_cli;
}

K_THREAD_DEFINE(worker_thread_id, STACKSIZE, bws_cli_worker, NULL, NULL, NULL,
        -1, K_USER | K_INHERIT_PERMS, 0);
