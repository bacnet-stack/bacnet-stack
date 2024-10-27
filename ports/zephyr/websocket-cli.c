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

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/sys_clock.h>
#include <zephyr/net/websocket.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/basic/sys/debug.h"

LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

#if CONFIG_BACNETSTACK_LOG_LEVEL == LOG_LEVEL_DBG
#define DUMP_BUFFER debug_printf_hex
#else
#define DUMP_BUFFER(...)
#endif

enum websocket_close_status {
    WEBSOCKET_CLOSE_STATUS_NOSTATUS = 0,
    WEBSOCKET_CLOSE_STATUS_NORMAL = 1000,
    WEBSOCKET_CLOSE_STATUS_GOINGAWAY = 1001,
    WEBSOCKET_CLOSE_STATUS_PROTOCOL_ERR = 1002,
    WEBSOCKET_CLOSE_STATUS_UNACCEPTABLE_OPCODE = 1003,
    WEBSOCKET_CLOSE_STATUS_RESERVED = 1004,
    WEBSOCKET_CLOSE_STATUS_NO_STATUS = 1005,
    WEBSOCKET_CLOSE_STATUS_ABNORMAL_CLOSE = 1006,
    WEBSOCKET_CLOSE_STATUS_INVALID_PAYLOAD = 1007,
    WEBSOCKET_CLOSE_STATUS_POLICY_VIOLATION = 1008,
    WEBSOCKET_CLOSE_STATUS_MESSAGE_TOO_LARGE = 1009,
    WEBSOCKET_CLOSE_STATUS_EXTENSION_REQUIRED = 1010,
    WEBSOCKET_CLOSE_STATUS_UNEXPECTED_CONDITION = 1011,
    WEBSOCKET_CLOSE_STATUS_TLS_FAILURE = 1015,
    WEBSOCKET_CLOSE_STATUS_CLIENT_TRANSACTION_DONE = 2000,
    WEBSOCKET_CLOSE_STATUS_NOSTATUS_CONTEXT_DESTROY = 9999,
};

typedef enum {
    BSC_WEBSOCKET_STATE_IDLE = 0,
    BSC_WEBSOCKET_STATE_CONFIGURING = 1,
    BSC_WEBSOCKET_STATE_TCP_CONNECTING = 2,
    BSC_WEBSOCKET_STATE_WEB_CONNECTING = 3,
    BSC_WEBSOCKET_STATE_CONNECTED = 4,
    BSC_WEBSOCKET_STATE_DISCONNECTED = 5
} BSC_WEBSOCKET_STATE;

typedef enum {
    WORKER_ID_CONNECT = 0,
    WORKER_ID_DISCONNECT = 1,
    WORKER_ID_SEND = 2,
    WORKER_ID_ERROR = 3,
} WORKER_ID_EVENT;

#define CONTEXT_PORT_NO_LISTEN 0

typedef enum {
    CA_CERTIFICATE_TAG = 0,
    SERVER_CERTIFICATE = 1,
    PRIVATE_KEY = 2,
} TLS_CREDENTIAL_TAGS;

#define TLS_CREDENTIAL_MAXIMUM 4

typedef struct {
    BSC_WEBSOCKET_PROTOCOL proto;
    const char *prot;
    const char *addr;
    const char *path;
    int port;
    char url[BSC_WSURL_MAX_LEN];
} BSC_WEBSOCKET_CONNECTION_PARAM;

/* We need to allocate bigger buffer for the websocket data we receive so that
 * the websocket header fits into it.
 * This buffer is used for pass sever parameters (see
 * BSC_WEBSOCKET_CONNECTION_PARAM) befor start HTTP request into the websocket
 * connect and for saving the security accept key during HTTP request.
 */
BUILD_ASSERT(sizeof(BSC_WEBSOCKET_CONNECTION_PARAM) <= BVLC_SC_NPDU_SIZE);

#define STACKSIZE 4096
#define MIN_BUF_SIZE 64

typedef struct {
    int sock;
    int websock;
    size_t timeout;
    uint64_t connect_deadline;
    BSC_WEBSOCKET_STATE state;
    bool sendable;
    BSC_WEBSOCKET_CLI_DISPATCH dispatch;
    void *user_param;
    size_t length;
    uint8_t *buf;
    size_t buf_size;
    uint8_t ws_buf[BVLC_SC_NPDU_SIZE];
    int event_fd;
    k_tid_t thread_id;
    struct k_thread worker_thr;
} BSC_WEBSOCKET_CONNECTION;

#ifndef CONFIG_NET_SOCKETS_POLL_MAX
#define CONFIG_NET_SOCKETS_POLL_MAX BSC_CLIENT_WEBSOCKETS_MAX_NUM
#endif

K_KERNEL_STACK_ARRAY_DEFINE(
    worker_stack, BSC_CLIENT_WEBSOCKETS_MAX_NUM, STACKSIZE);

BUILD_ASSERT(BSC_CLIENT_WEBSOCKETS_MAX_NUM <= CONFIG_NET_SOCKETS_POLL_MAX);

static BSC_WEBSOCKET_CONNECTION bws_cli_conn[BSC_CLIENT_WEBSOCKETS_MAX_NUM] = {
    0
};

K_MUTEX_DEFINE(bws_cli_mutex);

static uint8_t *k_realloc(uint8_t *buf, size_t old_size, size_t new_size)
{
    uint8_t *p = k_malloc(new_size);
    if (p) {
        memcpy(p, buf, old_size);
    }
    k_free(buf);
    return p;
}

static BACNET_ERROR_CODE websocket_close_status_to_error_code(uint16_t status)
{
    switch (status) {
        case WEBSOCKET_CLOSE_STATUS_NOSTATUS:
            return ERROR_CODE_OTHER;
        case WEBSOCKET_CLOSE_STATUS_NORMAL:
            return ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER;
        case WEBSOCKET_CLOSE_STATUS_GOINGAWAY:
            return ERROR_CODE_WEBSOCKET_ENDPOINT_LEAVES;
        case WEBSOCKET_CLOSE_STATUS_PROTOCOL_ERR:
            return ERROR_CODE_WEBSOCKET_PROTOCOL_ERROR;
        case WEBSOCKET_CLOSE_STATUS_UNACCEPTABLE_OPCODE:
            return ERROR_CODE_WEBSOCKET_DATA_NOT_ACCEPTED;
        case WEBSOCKET_CLOSE_STATUS_RESERVED:
            return ERROR_CODE_WEBSOCKET_ERROR;
        case WEBSOCKET_CLOSE_STATUS_NO_STATUS:
            return ERROR_CODE_WEBSOCKET_ERROR;
        case WEBSOCKET_CLOSE_STATUS_ABNORMAL_CLOSE:
            return ERROR_CODE_WEBSOCKET_DATA_NOT_ACCEPTED;
        case WEBSOCKET_CLOSE_STATUS_INVALID_PAYLOAD:
            return ERROR_CODE_WEBSOCKET_DATA_INCONSISTENT;
        case WEBSOCKET_CLOSE_STATUS_POLICY_VIOLATION:
            return ERROR_CODE_WEBSOCKET_DATA_AGAINST_POLICY;
        case WEBSOCKET_CLOSE_STATUS_MESSAGE_TOO_LARGE:
            return ERROR_CODE_WEBSOCKET_FRAME_TOO_LONG;
        case WEBSOCKET_CLOSE_STATUS_EXTENSION_REQUIRED:
            return ERROR_CODE_WEBSOCKET_EXTENSION_MISSING;
        case WEBSOCKET_CLOSE_STATUS_UNEXPECTED_CONDITION:
            return ERROR_CODE_WEBSOCKET_REQUEST_UNAVAILABLE;
        case WEBSOCKET_CLOSE_STATUS_TLS_FAILURE:
            return ERROR_CODE_TLS_ERROR;
    }
    return ERROR_CODE_WEBSOCKET_ERROR;
}

static const char *err_desc(BACNET_ERROR_CODE err_code)
{
    switch (err_code) {
        case ERROR_CODE_OTHER:
            return "Error";
        case ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER:
            return "Closed by peer";
        case ERROR_CODE_WEBSOCKET_ENDPOINT_LEAVES:
            return "Endpoint leaves";
        case ERROR_CODE_WEBSOCKET_PROTOCOL_ERROR:
            return "Protocol error";
        case ERROR_CODE_WEBSOCKET_DATA_NOT_ACCEPTED:
            return "Connect not accepted";
        case ERROR_CODE_WEBSOCKET_DATA_INCONSISTENT:
            return "Data inconsistent";
        case ERROR_CODE_WEBSOCKET_DATA_AGAINST_POLICY:
            return "Data Againsst policy";
        case ERROR_CODE_WEBSOCKET_FRAME_TOO_LONG:
            return "Frame too long";
        case ERROR_CODE_WEBSOCKET_EXTENSION_MISSING:
            return "Extension missing";
        case ERROR_CODE_WEBSOCKET_REQUEST_UNAVAILABLE:
            return "Request unavailable";
        case ERROR_CODE_TLS_ERROR:
            return "TLS error";
        case ERROR_CODE_WEBSOCKET_ERROR:
            return "Websocket error";
        default:
    }
    return "Unknown error";
}

// p1 - index BSC_WEBSOCKET_CONNECTION
static void bws_cli_worker(void *p1, void *p2, void *p3);

static BSC_WEBSOCKET_HANDLE bws_cli_alloc_connection(void)
{
    BSC_WEBSOCKET_CONNECTION *ctx;
    int i;
    LOG_INF("bws_cli_alloc_connection() >>>");
    for (i = 0; i < BSC_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        ctx = &bws_cli_conn[i];
        if (ctx->state == BSC_WEBSOCKET_STATE_IDLE) {
            memset(ctx, 0, sizeof(*ctx));
            ctx->sock = -1;
            ctx->websock = -1;
            ctx->event_fd = -1;
            ctx->state = BSC_WEBSOCKET_STATE_CONFIGURING;
            ctx->buf = k_malloc(MIN_BUF_SIZE);
            if (!ctx->buf) {
                LOG_ERR("bws_cli_alloc_connection() Error: no memory");
                return BSC_WEBSOCKET_INVALID_HANDLE;
            }
            ctx->buf_size = MIN_BUF_SIZE;
            LOG_INF("bws_cli_alloc_connection() <<<  h %d", i);
            return i;
        }
    }

    LOG_ERR("bws_cli_alloc_connection() Error: no free handle");
    return BSC_WEBSOCKET_INVALID_HANDLE;
}

static void bws_cli_free_connection(BSC_WEBSOCKET_HANDLE h)
{
    LOG_INF("bws_cli_free_connection() h %d", h);
    tls_credential_delete(
        CA_CERTIFICATE_TAG + TLS_CREDENTIAL_MAXIMUM * h,
        TLS_CREDENTIAL_CA_CERTIFICATE);
    tls_credential_delete(
        SERVER_CERTIFICATE + TLS_CREDENTIAL_MAXIMUM * h,
        TLS_CREDENTIAL_SERVER_CERTIFICATE);
    tls_credential_delete(
        PRIVATE_KEY + TLS_CREDENTIAL_MAXIMUM * h, TLS_CREDENTIAL_PRIVATE_KEY);

    bws_cli_conn[h].buf_size = 0;
    k_free(bws_cli_conn[h].buf);
}

static void setup_addr(
    uint16_t family,
    const char *server,
    int port,
    struct sockaddr *addr,
    socklen_t addr_len)
{
    memset(addr, 0, addr_len);

    if (family == AF_INET) {
        net_sin(addr)->sin_family = AF_INET;
        net_sin(addr)->sin_port = htons(port);
        zsock_inet_pton(family, server, &net_sin(addr)->sin_addr);
    } else {
        net_sin6(addr)->sin6_family = AF_INET6;
        net_sin6(addr)->sin6_port = htons(port);
        zsock_inet_pton(family, server, &net_sin6(addr)->sin6_addr);
    }
}

static int setup_socket(
    uint16_t family, int *sock, BSC_WEBSOCKET_HANDLE h, size_t timeout_s)
{
    const char *family_str = family == AF_INET ? "IPv4" : "IPv6";
    int ret = 0;
    struct timeval timeout = { 0 };
    (void)h;

    if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
        sec_tag_t sec_tag_list[] = {
            CA_CERTIFICATE_TAG + TLS_CREDENTIAL_MAXIMUM * h,
            SERVER_CERTIFICATE + TLS_CREDENTIAL_MAXIMUM * h,
            PRIVATE_KEY + TLS_CREDENTIAL_MAXIMUM * h
        };

        *sock = zsock_socket(family, SOCK_STREAM, IPPROTO_TLS_1_2);
        if (*sock >= 0) {
            ret = zsock_setsockopt(
                *sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
                sizeof(sec_tag_list));
            if (ret < 0) {
                LOG_ERR(
                    "setup_socket() "
                    "Failed to set %s secure option (%d)",
                    family_str, -errno);
                ret = -errno;
                goto SETUP_SOCKET_FAIL;
            }

            ret = zsock_setsockopt(
                *sock, SOL_TLS, TLS_HOSTNAME, NULL /*server*/,
                0 /*strlen(server)*/);
            if (ret < 0) {
                LOG_ERR(
                    "setup_socket() Failed to set %s TLS_HOSTNAME option (%d)",
                    family_str, -errno);
                ret = -errno;
                goto SETUP_SOCKET_FAIL;
            }

            timeout.tv_sec = timeout_s;
            ret = zsock_setsockopt(
                *sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
            if (ret < 0) {
                LOG_ERR(
                    "setup_socket() Failed to set %s SO_SNDTIMEO option (%d)",
                    family_str, -errno);
                ret = -errno;
                goto SETUP_SOCKET_FAIL;
            }
        }
    } else {
        *sock = zsock_socket(family, SOCK_STREAM, IPPROTO_TCP);
    }

    if (*sock < 0) {
        LOG_ERR(
            "setup_socket() Failed to create %s HTTP socket (%d)", family_str,
            -errno);
    }

    return ret;

SETUP_SOCKET_FAIL:
    if (*sock >= 0) {
        zsock_close(*sock);
        *sock = -1;
    }

    return ret;
}

static int calc_timeout(uint64_t deadline)
{
    return k_ticks_to_ms_floor64(deadline - sys_clock_tick_get());
}

// return poll timeout (msec)
static int prepare_poll(BSC_WEBSOCKET_CONNECTION *ctx, struct zsock_pollfd *fds)
{
    int timeout = -1;

    switch (ctx->state) {
        case BSC_WEBSOCKET_STATE_CONNECTED:
            fds->fd = ctx->sock;
            fds->events = ZSOCK_POLLIN;
            if (ctx->sendable) {
                fds->events |= ZSOCK_POLLOUT;
            }
            break;
        case BSC_WEBSOCKET_STATE_TCP_CONNECTING:
            fds->fd = ctx->sock;
            fds->events = ZSOCK_POLLIN;
            timeout = calc_timeout(ctx->connect_deadline);
            break;
        case BSC_WEBSOCKET_STATE_WEB_CONNECTING:
            fds->fd = ctx->sock;
            fds->events = ZSOCK_POLLIN;
            break;
        default:
            fds->fd = -1;
            fds->events = 0;
            break;
    }
    fds->revents = 0;

    return timeout;
}

static void emit_worker_event(
    uint8_t event_id, BSC_WEBSOCKET_HANDLE h, uint16_t event_status)
{
    static struct k_mutex mutex = Z_MUTEX_INITIALIZER(mutex);

    k_mutex_lock(&mutex, K_FOREVER);
    LOG_INF("Worker event %d happend for %d connect", event_id, h);
    if (bws_cli_conn[h].event_fd >= 0) {
        zsock_send(bws_cli_conn[h].event_fd, &event_id, sizeof(event_id), 0);
        if (event_id == WORKER_ID_ERROR) {
            zsock_send(
                bws_cli_conn[h].event_fd, &event_status, sizeof(event_status),
                0);
        }
    } else {
        LOG_ERR("Worker connect %d is not run yet", h);
    }
    k_mutex_unlock(&mutex);
}

static int parse_uri(
    char *p, const char **prot, const char **ads, int *port, const char **path)
{
    const char *end;
    char unix_skt = 0;

    /* cut up the location into address, port and path */
    *prot = p;
    while (*p && (*p != ':' || p[1] != '/' || p[2] != '/')) {
        p++;
    }
    if (!*p) {
        end = p;
        p = (char *)*prot;
        *prot = end;
    } else {
        *p = '\0';
        p += 3;
    }
    if (*p == '+') { /* unix skt */
        unix_skt = 1;
    }

    *ads = p;
    if (!strcmp(*prot, "http") || !strcmp(*prot, "ws")) {
        *port = 80;
    } else if (!strcmp(*prot, "https") || !strcmp(*prot, "wss")) {
        *port = 443;
    }

    if (*p == '[') {
        ++(*ads);
        while (*p && *p != ']') {
            p++;
        }
        if (*p) {
            *p++ = '\0';
        }
    } else {
        while (*p && *p != ':' && (unix_skt || *p != '/')) {
            p++;
        }
    }

    if (*p == ':') {
        *p++ = '\0';
        *port = atoi(p);
        while (*p && *p != '/') {
            p++;
        }
    }
    *path = "/";
    if (*p) {
        *p++ = '\0';
        if (*p) {
            *path = p;
        }
    }

    return 0;
}

static BSC_WEBSOCKET_RET bws_cli_init(
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size,
    BSC_WEBSOCKET_HANDLE *out_handle)
{
    BSC_WEBSOCKET_HANDLE h;
    BSC_WEBSOCKET_RET retcode = BSC_WEBSOCKET_SUCCESS;
    int ret;

    LOG_INF("bws_cli_init >>>");

    k_mutex_lock(&bws_cli_mutex, K_FOREVER);
    h = bws_cli_alloc_connection();
    k_mutex_unlock(&bws_cli_mutex);
    if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
        LOG_ERR("Cannot allocate connection context");
        retcode = BSC_WEBSOCKET_NO_RESOURCES;
        goto CONNECT_INIT_FAIL;
    }

    ret = tls_credential_add(
        CA_CERTIFICATE_TAG + TLS_CREDENTIAL_MAXIMUM * h,
        TLS_CREDENTIAL_CA_CERTIFICATE, ca_cert, ca_cert_size);
    if (ret < 0) {
        LOG_ERR("Failed to register public certificate: %d", ret);
        retcode = BSC_WEBSOCKET_BAD_PARAM;
        goto CONNECT_INIT_FAIL;
    }

    ret = tls_credential_add(
        SERVER_CERTIFICATE + TLS_CREDENTIAL_MAXIMUM * h,
        TLS_CREDENTIAL_SERVER_CERTIFICATE, cert, cert_size);
    if (ret < 0) {
        LOG_ERR("Failed to register server certificate: %d", ret);
        retcode = BSC_WEBSOCKET_BAD_PARAM;
        goto CONNECT_INIT_FAIL;
    }

    ret = tls_credential_add(
        PRIVATE_KEY + TLS_CREDENTIAL_MAXIMUM * h, TLS_CREDENTIAL_PRIVATE_KEY,
        key, key_size);
    if (ret < 0) {
        LOG_ERR("Failed to register key certificate: %d", ret);
        retcode = BSC_WEBSOCKET_BAD_PARAM;
        goto CONNECT_INIT_FAIL;
    }

    *out_handle = h;

CONNECT_INIT_FAIL:
    LOG_INF("bws_cli_init <<< %d", retcode);

    return retcode;
}

BSC_WEBSOCKET_RET bws_cli_connect(
    BSC_WEBSOCKET_PROTOCOL proto,
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
    BSC_WEBSOCKET_HANDLE h = 0;
    BSC_WEBSOCKET_RET ret;
    BSC_WEBSOCKET_CONNECTION *ctx = NULL;
    BSC_WEBSOCKET_CONNECTION_PARAM *param;
    int spair[2] = { -1, -1 };

    LOG_INF("bws_cli_connect() >>> url=%s", url);

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size ||
        !url || !out_handle || !timeout_s || !dispatch_func) {
        LOG_INF("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (proto != BSC_WEBSOCKET_HUB_PROTOCOL &&
        proto != BSC_WEBSOCKET_DIRECT_PROTOCOL) {
        LOG_INF("bws_cli_connect() <<< ret = BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    ret =
        bws_cli_init(ca_cert, ca_cert_size, cert, cert_size, key, key_size, &h);
    if (ret != BSC_WEBSOCKET_SUCCESS) {
        goto CONNECT_FAIL;
    }

    ctx = &bws_cli_conn[h];
    ctx->timeout = MSEC_PER_SEC * timeout_s;
    ctx->connect_deadline =
        sys_clock_timeout_end_calc(Z_TIMEOUT_MS(ctx->timeout));

    param = (BSC_WEBSOCKET_CONNECTION_PARAM *)ctx->ws_buf;
    param->proto = proto;
    strncpy(param->url, url, BSC_WSURL_MAX_LEN);
    (void)parse_uri(
        param->url, &param->prot, &param->addr, &param->port, &param->path);
    if (param->port == -1 || !param->prot || !param->addr || !param->path ||
        (strcmp(param->prot, "wss") != 0)) {
        ret = BSC_WEBSOCKET_BAD_PARAM;
        goto CONNECT_FAIL;
    }

    setup_socket(AF_INET, &ctx->sock, h, timeout_s);
    if (ctx->sock < 0) {
        ret = BSC_WEBSOCKET_NO_RESOURCES;
        goto CONNECT_FAIL;
    }

    ret = zsock_socketpair(AF_UNIX, SOCK_STREAM, 0, spair);
    if (ret != 0) {
        ret = BSC_WEBSOCKET_NO_RESOURCES;
        goto CONNECT_FAIL;
    }

    ctx->event_fd = spair[0];
    ctx->dispatch = dispatch_func;
    ctx->user_param = dispatch_func_user_param;

    ctx->thread_id = k_thread_create(
        &ctx->worker_thr, worker_stack[h], STACKSIZE, bws_cli_worker, (void *)h,
        (void *)spair[1], NULL, -1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
    if (ctx->thread_id == NULL) {
        ret = BSC_WEBSOCKET_NO_RESOURCES;
        goto CONNECT_FAIL;
    }

    emit_worker_event(WORKER_ID_CONNECT, h, 0);

    *out_handle = h;

    LOG_INF("bws_cli_connect() <<< h = %d", h);
    return BSC_WEBSOCKET_SUCCESS;

CONNECT_FAIL:
    if (ctx) {
        if (ctx->sock >= 0) {
            zsock_close(ctx->sock);
        }
        if (spair[0] >= 0) {
            zsock_close(spair[0]);
        }
        if (spair[1] >= 0) {
            zsock_close(spair[1]);
        }

        k_mutex_lock(&bws_cli_mutex, K_FOREVER);
        ctx->state = BSC_WEBSOCKET_STATE_IDLE;
        ctx->sock = -1;
        if (ctx->thread_id != NULL) {
            k_thread_abort(ctx->thread_id);
        }
        bws_cli_free_connection(h);
        k_mutex_unlock(&bws_cli_mutex);
    }
    LOG_ERR("bws_cli_connect() <<< error %d", ret);
    return ret;
}

static void worker_zsock_connect(BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONNECTION *ctx = &bws_cli_conn[h];
    BSC_WEBSOCKET_CONNECTION_PARAM *param =
        (BSC_WEBSOCKET_CONNECTION_PARAM *)ctx->ws_buf;
    struct sockaddr_in addr_in;
    int ret;

    LOG_INF("bws_cli_zsock_connect() >>> h %d", h);

    setup_addr(
        AF_INET, param->addr, param->port, (struct sockaddr *)&addr_in,
        sizeof(addr_in));

    ctx->state = BSC_WEBSOCKET_STATE_TCP_CONNECTING;
    ret =
        zsock_connect(ctx->sock, (struct sockaddr *)&addr_in, sizeof(addr_in));
    if (ret < 0) {
        LOG_ERR("Cannot zsock connect to remote %d (%d)", h, -errno);
        ret = -errno;
        emit_worker_event(WORKER_ID_ERROR, h, WEBSOCKET_CLOSE_STATUS_NO_STATUS);
    } else {
        emit_worker_event(WORKER_ID_CONNECT, h, 0);
    }
    LOG_INF("bws_cli_zsock_connect() <<< h %d", h);
}

static int
websocket_connect_cb(int websock, struct http_request *req, void *user_data)
{
    BSC_WEBSOCKET_HANDLE h = (BSC_WEBSOCKET_HANDLE)user_data;
    BSC_WEBSOCKET_CONNECTION *ctx;

    if ((h >= 0) && (h < BSC_CLIENT_WEBSOCKETS_MAX_NUM) &&
        (bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_WEB_CONNECTING)) {
        LOG_INF("Connect %d successed", h);
        ctx = &bws_cli_conn[h];
        ctx->websock = websock;
        ctx->length = 0;
        ctx->state = BSC_WEBSOCKET_STATE_CONNECTED;
        emit_worker_event(WORKER_ID_CONNECT, h, 0);
    }

    return h < BSC_CLIENT_WEBSOCKETS_MAX_NUM ? 0 : -ENOENT;
}

static void worker_websocket_connect(BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONNECTION *ctx = &bws_cli_conn[h];
    BSC_WEBSOCKET_CONNECTION_PARAM *param =
        (BSC_WEBSOCKET_CONNECTION_PARAM *)ctx->ws_buf;
    struct websocket_request req = { 0 };
    char protocol[60];
    const char *extra_headers[] = { protocol, NULL };
    int32_t timeout;
    int ret;

    LOG_INF("bws_cli_websocket_connect() >>> h %d", h);

    snprintf(
        protocol, sizeof(protocol), "Sec-WebSocket-Protocol: %s\r\n",
        param->proto == BSC_WEBSOCKET_HUB_PROTOCOL
            ? BSC_WEBSOCKET_HUB_PROTOCOL_STR
            : BSC_WEBSOCKET_DIRECT_PROTOCOL_STR);
    LOG_INF("Websocket protocol = %s", protocol);

    req.host = param->addr;
    req.url = param->path;
    req.optional_headers = extra_headers;
    req.cb = websocket_connect_cb;
    req.tmp_buf = ctx->ws_buf;
    req.tmp_buf_len = sizeof(ctx->ws_buf);

    ctx->state = BSC_WEBSOCKET_STATE_WEB_CONNECTING;
    timeout = calc_timeout(ctx->connect_deadline);
    ret = websocket_connect(ctx->sock, &req, timeout, (void *)h);
    if (ret < 0) {
        LOG_ERR(
            "Cannot websocket connect to remote socket h %d (%d)", h, -errno);
        emit_worker_event(
            WORKER_ID_ERROR, h, WEBSOCKET_CLOSE_STATUS_PROTOCOL_ERR);
        return;
    }

    LOG_INF("bws_cli_websocket_connect() <<< h %d", h);
}

static void worker_disconnect(
    BSC_WEBSOCKET_HANDLE h, BACNET_ERROR_CODE reason, char *reason_desc)
{
    BSC_WEBSOCKET_CONNECTION *ctx = &bws_cli_conn[h];

    if (ctx->websock >= 0) {
        websocket_disconnect(ctx->websock);
    } else if (ctx->sock >= 0) {
        zsock_close(ctx->sock);
    }
    ctx->websock = ctx->sock = -1;
    ctx->state = BSC_WEBSOCKET_STATE_DISCONNECTED;
    ctx->dispatch(
        h, BSC_WEBSOCKET_DISCONNECTED, reason, reason_desc, NULL, 0,
        ctx->user_param);
}

void bws_cli_disconnect(BSC_WEBSOCKET_HANDLE h)
{
    LOG_INF("bws_cli_disconnect() >>> h %d", h);
    emit_worker_event(WORKER_ID_DISCONNECT, h, 0);
    LOG_INF("bws_cli_disconnect() <<< h %d", h);
}

// p1 - handle of BSC_WEBSOCKET_CONNECTION
// p2 - event socket
static void bws_cli_worker(void *p1, void *p2, void *p3)
{
    int ret = 0;
    uint64_t remaining = ULLONG_MAX;
    uint32_t message_type;
    BSC_WEBSOCKET_HANDLE h = (BSC_WEBSOCKET_HANDLE)p1;
    BSC_WEBSOCKET_CONNECTION *ctx = &bws_cli_conn[h];
    uint8_t event;
    uint16_t event_status;
    int timeout;
    struct zsock_pollfd fds[2] = { 0 }; // first - connect, second - event

    LOG_INF("bws_cli_worker() >>> h %d event_fd %d", h, (int)p2);

    fds[1].fd = (int)p2;
    fds[1].events = ZSOCK_POLLIN;
    fds[1].revents = 0;

    while (1) {
        // check if the worker is needed
        if (ctx->state == BSC_WEBSOCKET_STATE_DISCONNECTED) {
            zsock_close(fds[1].fd);
            zsock_close(ctx->event_fd);
            ctx->event_fd = -1;
            k_mutex_lock(&bws_cli_mutex, K_FOREVER);
            bws_cli_free_connection(h);
            ctx->state = BSC_WEBSOCKET_STATE_IDLE;
            k_mutex_unlock(&bws_cli_mutex);
            break; // close worker
        }

        timeout = prepare_poll(ctx, &fds[0]);
        // LOG_INF("zsock_polling timeout %d", timeout);
        ret = zsock_poll(fds, 2, timeout);
        // LOG_INF("zsock_polled: %d errno %d", ret, errno);

        // worker events
        if (fds[1].revents & ZSOCK_POLLIN) {
            fds[1].revents = 0;
            zsock_recv(fds[1].fd, &event, sizeof(event), ZSOCK_MSG_DONTWAIT);
            LOG_INF("Worker event happend, h %d, id %d", h, event);

            switch (event) {
                case WORKER_ID_CONNECT:
                    switch (ctx->state) {
                        case BSC_WEBSOCKET_STATE_CONFIGURING:
                            worker_zsock_connect(h);
                            break;
                        case BSC_WEBSOCKET_STATE_TCP_CONNECTING:
                            worker_websocket_connect(h);
                            break;
                        case BSC_WEBSOCKET_STATE_CONNECTED:
                            ctx->dispatch(
                                h, BSC_WEBSOCKET_CONNECTED, 0, NULL, NULL, 0,
                                ctx->user_param);
                            break;
                        default:
                            break;
                    }
                    break;
                case WORKER_ID_DISCONNECT:
                    worker_disconnect(h, ERROR_CODE_OTHER, NULL);
                    break;
                case WORKER_ID_SEND:
                    ctx->sendable = true;
                    break;
                case WORKER_ID_ERROR: {
                    BACNET_ERROR_CODE err_code;
                    zsock_recv(
                        fds[1].fd, &event_status, sizeof(event_status),
                        ZSOCK_MSG_DONTWAIT);
                    err_code =
                        websocket_close_status_to_error_code(event_status);
                    worker_disconnect(h, err_code, (char *)err_desc(err_code));
                } break;
            }
        }

        // error socket
        if ((fds[0].revents & ZSOCK_POLLERR) &&
            (ctx->state != BSC_WEBSOCKET_STATE_DISCONNECTED)) {
            worker_disconnect(
                h, ERROR_CODE_WEBSOCKET_CLOSED_ABNORMALLY,
                "Websocket closed abnormally");
            continue;
        }

        // receive & connect
        if ((fds[0].revents & ZSOCK_POLLIN) &&
            (ctx->state == BSC_WEBSOCKET_STATE_CONNECTED)) {
            LOG_INF("connect h %d pollin", h);
            remaining = 1;
            while (remaining > 0) {
                ret = websocket_recv_msg(
                    ctx->websock, ctx->buf + ctx->length,
                    ctx->buf_size - ctx->length, &message_type, &remaining, 0);
                if (ret <= 0) {
                    break;
                }
                ctx->length += ret;
                if (remaining > ctx->buf_size - ctx->length) {
                    size_t new_size = ctx->length + remaining;
                    ctx->buf = k_realloc(ctx->buf, ctx->buf_size, new_size);
                    if (!ctx->buf) {
                        worker_disconnect(
                            h, ERROR_CODE_OUT_OF_MEMORY, "Out of memory");
                        message_type = 0;
                        break;
                    }
                    ctx->buf_size = new_size;
                }
            }

            LOG_INF(
                "websocket_recv_msg ret %d, type %d, remaining %lld", ret,
                message_type, remaining);
            // -ENOTCONN from current zephyr websocket client means no received
            // data in real socket
            if ((ret < 0) && (ret != -EAGAIN) && (ret != -ENOTCONN)) {
                LOG_ERR("Error websocket received: %d h %d", ret, h);
                worker_disconnect(
                    h, ERROR_CODE_WEBSOCKET_CLOSED_ABNORMALLY,
                    "Websocket closed abnormally");
                continue;
            }

            if (message_type & WEBSOCKET_FLAG_PING) {
                ret = websocket_send_msg(
                    ctx->websock, ctx->buf, ret, WEBSOCKET_OPCODE_PONG, false,
                    true, ctx->timeout);
                LOG_INF("Sent PONG, status %d", ret);
            }

            if (message_type & WEBSOCKET_FLAG_CLOSE) {
                LOG_DBG("Receive message Close");
                worker_disconnect(
                    h, ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER,
                    "Receive message Close");
            }

            if ((message_type & WEBSOCKET_FLAG_FINAL) &&
                (message_type &
                 (WEBSOCKET_FLAG_TEXT | WEBSOCKET_FLAG_BINARY))) {
                LOG_DBG("Receive message Data, len %d h %d", ctx->length, h);
                DUMP_BUFFER(0, ctx->buf, ctx->length, "Client receive");
                ctx->dispatch(
                    h, BSC_WEBSOCKET_RECEIVED, 0, NULL, ctx->buf, ctx->length,
                    ctx->user_param);
                ctx->length = 0;
            }
        }

        if ((fds[0].revents & ZSOCK_POLLOUT) && ctx->sendable) {
            LOG_INF("connect h %d pollout - sendable", h);
            ctx->dispatch(
                h, BSC_WEBSOCKET_SENDABLE, 0, NULL, NULL, 0, ctx->user_param);
            ctx->sendable = false;
        }

        if (fds[0].revents & ZSOCK_POLLHUP) {
            LOG_INF("connect h %d pollhup - disconnect", h);
            worker_disconnect(h, ERROR_CODE_OTHER, NULL);
            continue;
        }
    }
    LOG_INF("bws_cli_worker() <<< h %d", h);
}

void bws_cli_send(BSC_WEBSOCKET_HANDLE h)
{
    LOG_INF("bws_cli_send() >>> h =  %d", h);
    emit_worker_event(WORKER_ID_SEND, h, 0);
    LOG_INF("bws_cli_send() >>>");
}

BSC_WEBSOCKET_RET bws_cli_dispatch_send(
    BSC_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size)
{
    BSC_WEBSOCKET_CONNECTION *ctx;
    BSC_WEBSOCKET_RET ret;

    LOG_INF(
        "bws_cli_dispatch_send() >>> h = %d, payload = %p, size = %d", h,
        payload, payload_size);

    if (h < 0 || h >= BSC_CLIENT_WEBSOCKETS_MAX_NUM) {
        ret = BSC_WEBSOCKET_BAD_PARAM;
        goto DISPATCH_SEND_EXIT;
    }

    if (!payload || !payload_size) {
        ret = BSC_WEBSOCKET_BAD_PARAM;
        goto DISPATCH_SEND_EXIT;
    }

    ctx = &bws_cli_conn[h];
    if (ctx->state != BSC_WEBSOCKET_STATE_CONNECTED) {
        ret = BSC_WEBSOCKET_NO_RESOURCES;
        goto DISPATCH_SEND_EXIT;
    }

    if (z_current_get() != ctx->thread_id) {
        ret = BSC_WEBSOCKET_INVALID_OPERATION;
        goto DISPATCH_SEND_EXIT;
    }

    DUMP_BUFFER(0, payload, payload_size, "Client send");
    ret = websocket_send_msg(
              ctx->websock, payload, payload_size, WEBSOCKET_OPCODE_DATA_BINARY,
              false, true, ctx->timeout) >= 0
        ? BSC_WEBSOCKET_SUCCESS
        : BSC_WEBSOCKET_INVALID_OPERATION;

DISPATCH_SEND_EXIT:
    LOG_INF("bws_cli_dispatch_send() <<< ret = %d", ret);
    return ret;
}
