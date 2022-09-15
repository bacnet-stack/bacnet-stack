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
//#include <net/websocket.h>
#include <net/tls_credentials.h>
#include <sys_clock.h>
#include <websocket.h>
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/basic/sys/debug.h"

/* Logging module registration is already done in ports/zephyr/main.c */
#include <logging/log.h>
#include <logging/log_ctrl.h>

LOG_MODULE_DECLARE(bacnet, LOG_LEVEL_DBG);

typedef enum {
    BSC_WEBSOCKET_STATE_IDLE = 0,
    BSC_WEBSOCKET_STATE_CONFIGURING = 1,
    BSC_WEBSOCKET_STATE_TCP_CONNECTING = 2,
    BSC_WEBSOCKET_STATE_WEB_CONNECTING = 3,
    BSC_WEBSOCKET_STATE_CONNECTED = 4,
    BSC_WEBSOCKET_STATE_DISCONNECTING = 5,
    BSC_WEBSOCKET_STATE_DISCONNECTED = 6
} BSC_WEBSOCKET_STATE;

typedef enum {
    WORKER_ID_CONNECT = 0,
    WORKER_ID_DISCONNECT = 1,
    WORKER_ID_SEND = 2,
    WORKER_ID_ERROR = 3,
} WORKER_ID_EVENT;

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
 * This buffer is used for pass sever parameters (see
 * BSC_WEBSOCKET_CONNECTION_PARAM) befor start HTTP request into the websocket 
 * connect and for saving the security accept key during HTTP request.
 */
#define MAX_RECV_BUF_LEN 276

typedef struct {
    BSC_WEBSOCKET_PROTOCOL proto;
    const char *prot;
    const char *addr;
    const char *path;
    int port;
    char url[BSC_WSURL_MAX_LEN];
} BSC_WEBSOCKET_CONNECTION_PARAM;

typedef struct {
    int sock;
    int websock;
    uint64_t connect_deadline;
    BSC_WEBSOCKET_STATE state;
    BSC_WEBSOCKET_CLI_DISPATCH dispatch;
    void* user_param;
    uint8_t buf[MAX_RECV_BUF_LEN];
} BSC_WEBSOCKET_CONNECTION;

BUILD_ASSERT(BSC_CLIENT_WEBSOCKETS_MAX_NUM < CONFIG_NET_SOCKETS_POLL_MAX,
    "BSC_CLIENT_WEBSOCKETS_MAX_NUM must be greater than CONFIG_NET_SOCKETS_POLL_MAX");

BUILD_ASSERT(sizeof(BSC_WEBSOCKET_CONNECTION_PARAM) <= MAX_RECV_BUF_LEN,
    "Connection buffer must be greater then size of BSC_WEBSOCKET_CONNECTION_PARAM");

static BSC_WEBSOCKET_CONNECTION
    bws_cli_conn[BSC_CLIENT_WEBSOCKETS_MAX_NUM] = { 0 };
static struct zsock_pollfd fds[BSC_CLIENT_WEBSOCKETS_MAX_NUM +1] = { 0 };

#define EVENT_FDS_INDEX BSC_CLIENT_WEBSOCKETS_MAX_NUM

#define STACKSIZE (4096 + CONFIG_TEST_EXTRA_STACKSIZE)
static int worker_event_fd = -1;
static void bws_cli_worker(void *p1, void *p2, void *p3);

K_THREAD_DEFINE(worker_thread, STACKSIZE, bws_cli_worker, NULL, NULL, NULL,
        -1, K_USER | K_INHERIT_PERMS, 0);

static void setblocking(int fd, bool val)
{
    int fl, res;

    fl = zsock_fcntl(fd, F_GETFL, 0);
    if (fl == -1) {
        LOG_ERR("fcntl(F_GETFL): %d", errno);
    }

    if (val) {
        fl &= ~O_NONBLOCK;
    } else {
        fl |= O_NONBLOCK;
    }

    res = zsock_fcntl(fd, F_SETFL, fl);
    if (fl == -1) {
        LOG_ERR("fcntl(F_SETFL): %d", errno);
    }
}

static BSC_WEBSOCKET_HANDLE bws_cli_alloc_connection(void)
{
    BSC_WEBSOCKET_CONNECTION *ctx;
    int i;
    for (i = 0; i < BSC_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        ctx = &bws_cli_conn[i];
        if (ctx->state == BSC_WEBSOCKET_STATE_IDLE) {
            memset(ctx, 0, sizeof(*ctx));
            ctx->sock = -1;
            ctx->websock = -1;
            ctx->state = BSC_WEBSOCKET_STATE_CONFIGURING;
            break;
        }
    }

    return i < BSC_CLIENT_WEBSOCKETS_MAX_NUM ?
        i : BSC_WEBSOCKET_INVALID_HANDLE;
}

static void setup_addr(uint16_t family, const char *server, int port,
            struct sockaddr *addr, socklen_t addr_len)
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

static int setup_socket(uint16_t family, int *sock, BSC_WEBSOCKET_HANDLE h)
{
    const char *family_str = family == AF_INET ? "IPv4" : "IPv6";
    int ret = 0;

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

/*
 * Calcualte timeout in msec
 * connect_timeout - connect deadline in tick
 * current_timeout - current timeout in msec
 * return: timeout in msec
 */
static int calc_timeout(uint64_t connect_timeout, int current_timeout)
{
    uint64_t t = k_ticks_to_ms_floor64(connect_timeout - sys_clock_tick_get());
    if (current_timeout < 0)
        return (int)t;
    return MIN(t, current_timeout);
}

// return poll timeout (msec)
static int renumber_fds(void)
{
    int timeout = -1;
    int i;

    for (i = 0; i < BSC_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
        switch(bws_cli_conn[i].state) {
            case BSC_WEBSOCKET_STATE_CONNECTED:
                fds[i].fd = bws_cli_conn[i].sock;
                fds[i].events = ZSOCK_POLLIN;
                break;
            case BSC_WEBSOCKET_STATE_TCP_CONNECTING:
                fds[i].fd = bws_cli_conn[i].sock;
                fds[i].events = ZSOCK_POLLIN | ZSOCK_POLLOUT;
                timeout = calc_timeout(bws_cli_conn[i].connect_deadline, timeout);
                break;
            case BSC_WEBSOCKET_STATE_WEB_CONNECTING:
                fds[i].fd = bws_cli_conn[i].sock;
                fds[i].events = ZSOCK_POLLIN;
                //timeout = calc_timeout(bws_cli_conn[i].connect_deadline, timeout);
                break;
            default:
                fds[i].fd = -1;
                fds[i].events = 0;
                break;
        }
        fds[i].revents = 0;
    }
    return timeout;
}

static void emit_worker_event(uint8_t event_id, BSC_WEBSOCKET_HANDLE h)
{
    uint8_t event[2] = {event_id, h};
    LOG_INF("run worker event %d for %d", event_id, h);
    zsock_send(worker_event_fd, event, sizeof(event), 0);
}

static BSC_WEBSOCKET_RET bsc_websocket_retcode(int ret)
{
    return ret >= 0 ? BSC_WEBSOCKET_SUCCESS
                     : BSC_WEBSOCKET_INVALID_OPERATION;
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

    h = bws_cli_alloc_connection();
    if (h == BSC_WEBSOCKET_INVALID_HANDLE) {
        LOG_ERR("Cannot allocate connection context");
        retcode = BSC_WEBSOCKET_NO_RESOURCES;
        goto exit;
    }

    ret = tls_credential_add(CA_CERTIFICATE_TAG + TLS_CREDENTIAL_MAXIMUM * h,
        TLS_CREDENTIAL_CA_CERTIFICATE, ca_cert, ca_cert_size);
    if (ret < 0) {
        LOG_ERR("Failed to register public certificate: %d", ret);
        retcode = BSC_WEBSOCKET_BAD_PARAM;
        goto exit;
    }

    ret = tls_credential_add(SERVER_CERTIFICATE + TLS_CREDENTIAL_MAXIMUM * h,
        TLS_CREDENTIAL_SERVER_CERTIFICATE, cert, cert_size);
    if (ret < 0) {
        LOG_ERR("Failed to register server certificate: %d", ret);
        retcode = BSC_WEBSOCKET_BAD_PARAM;
        goto exit;
    }

    ret = tls_credential_add(PRIVATE_KEY + TLS_CREDENTIAL_MAXIMUM * h,
                 TLS_CREDENTIAL_PRIVATE_KEY, key, key_size);
    if (ret < 0) {
        LOG_ERR("Failed to register key certificate: %d", ret);
        retcode = BSC_WEBSOCKET_BAD_PARAM;
        goto exit;
    }

    *out_handle = h;

exit:
    if ((retcode != BSC_WEBSOCKET_SUCCESS) &&
        (h != BSC_WEBSOCKET_INVALID_HANDLE))
    {
        bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_IDLE;
    }
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
    void* dispatch_func_user_param,
    BSC_WEBSOCKET_HANDLE *out_handle)
{
    BSC_WEBSOCKET_HANDLE h;
    BSC_WEBSOCKET_RET retcode;
    BSC_WEBSOCKET_CONNECTION *ctx;
    BSC_WEBSOCKET_CONNECTION_PARAM *param;

    LOG_INF("bws_cli_connect() >>>");

    retcode = bws_cli_init(ca_cert, ca_cert_size, cert, cert_size, key,
        key_size, &h);
    if (retcode != BSC_WEBSOCKET_SUCCESS) {
        return retcode;
    }

    ctx = &bws_cli_conn[h];
    ctx->connect_deadline = sys_clock_timeout_end_calc(Z_TIMEOUT_MS(timeout_s));

    param = (BSC_WEBSOCKET_CONNECTION_PARAM*)ctx->buf;
    param->proto = proto;
    strncpy(param->url, url, BSC_WSURL_MAX_LEN);
    (void)parse_uri(param->url, &param->prot, &param->addr, &param->port,
        &param->path);
    if (param->port == -1 || !param->prot || !param->addr || !param->path) {
        ctx->state = BSC_WEBSOCKET_STATE_IDLE;
        LOG_ERR("bws_cli_connect() <<< BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    ctx->dispatch = dispatch_func;
    ctx->user_param = dispatch_func_user_param;
    setup_socket(AF_INET, &ctx->sock, h);
    if (ctx->sock < 0) {
        ctx->state = BSC_WEBSOCKET_STATE_IDLE;
        LOG_ERR("bws_cli_connect() <<< Cannot create socket");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    setblocking(ctx->sock, false);
    emit_worker_event(WORKER_ID_CONNECT, h);

    *out_handle = h;

    LOG_INF("bws_cli_connect() <<<");
    return BSC_WEBSOCKET_SUCCESS;
}

static void worker_zsock_connect(BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONNECTION *ctx = &bws_cli_conn[h];
    BSC_WEBSOCKET_CONNECTION_PARAM *param = 
        (BSC_WEBSOCKET_CONNECTION_PARAM*)ctx->buf;
    struct sockaddr_in addr_in;
    int ret;

    LOG_INF("bws_cli_zsock_connect() >>> %d", h);

    setup_addr(AF_INET, param->addr, param->port, (struct sockaddr *)&addr_in,
        sizeof(addr_in));

    ctx->state = BSC_WEBSOCKET_STATE_TCP_CONNECTING;
    ret = zsock_connect(ctx->sock, (struct sockaddr *)&addr_in,
        sizeof(addr_in));
    if (ret < 0) {
        LOG_ERR("Cannot connect to remote (%d)", -errno);
        ret = -errno;
        emit_worker_event(WORKER_ID_ERROR, h);
    } else {
        emit_worker_event(WORKER_ID_CONNECT, h);
    }
    LOG_INF("bws_cli_zsock_connect() <<<");
}

static int websocket_connect_cb(int websock, struct http_request *req,
    void *user_data)
{
    BSC_WEBSOCKET_HANDLE h = (BSC_WEBSOCKET_HANDLE)user_data;

    if ((h >= 0) && (h < BSC_CLIENT_WEBSOCKETS_MAX_NUM) && 
        (bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_WEB_CONNECTING)) {
            LOG_INF("Connect %d successed", h);
            bws_cli_conn[h].websock = websock;
            bws_cli_conn[h].state = BSC_WEBSOCKET_STATE_CONNECTED;
            setblocking(bws_cli_conn[h].sock, true);
            emit_worker_event(WORKER_ID_CONNECT, h);
    }

    return h < BSC_CLIENT_WEBSOCKETS_MAX_NUM ? 0 :
        BSC_WEBSOCKET_INVALID_HANDLE;
}

static void worker_websocket_connect(BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONNECTION *ctx = &bws_cli_conn[h];
    BSC_WEBSOCKET_CONNECTION_PARAM *param = 
        (BSC_WEBSOCKET_CONNECTION_PARAM*)ctx->buf;
    struct websocket_request req = {0};
    char protocol[60];
    const char *extra_headers[] = {
        protocol,
        NULL
    };
    int32_t timeout;
    int ret;

    LOG_INF("bws_cli_websocket_connect() >>> %d", h);

    snprintf(protocol, sizeof(protocol),"Sec-WebSocket-Protocol: %s\r\n",
        param->proto == BSC_WEBSOCKET_HUB_PROTOCOL ?
            BSC_WEBSOCKET_HUB_PROTOCOL_STR :
            BSC_WEBSOCKET_DIRECT_PROTOCOL_STR);
    LOG_INF("Websocket protocol = %s", protocol);

    req.host = param->addr;
    req.url = param->path;
    req.optional_headers = extra_headers;
    req.cb = websocket_connect_cb;
    req.tmp_buf = ctx->buf;
    req.tmp_buf_len = sizeof(ctx->buf);

    ctx->state = BSC_WEBSOCKET_STATE_WEB_CONNECTING;
    timeout = calc_timeout(ctx->connect_deadline, -1);
    ret = websocket_connect(ctx->sock, &req, timeout, (void*)h);
    LOG_INF("websocket_connect() return %d", ret);
    if ((ret < 0) && (ret != -EAGAIN)) {
        LOG_ERR("Cannot connect to %s:%d", param->addr, param->port);
        emit_worker_event(WORKER_ID_ERROR, h);
        return;
    }

    LOG_INF("bws_cli_websocket_connect() <<<");
}

static void worker_websocket_connect_wait_data(BSC_WEBSOCKET_HANDLE h)
{
    BSC_WEBSOCKET_CONNECTION *ctx = &bws_cli_conn[h];
    BSC_WEBSOCKET_CONNECTION_PARAM *param = 
        (BSC_WEBSOCKET_CONNECTION_PARAM*)ctx->buf;
    int ret;
    struct websocket_request req = {0};

    LOG_INF("worker_websocket_connect_wait_data() >>> %d", h);

    // Fill mock HTTP request for responce processing
    req.cb = websocket_connect_cb;
    req.tmp_buf = ctx->buf;
    req.tmp_buf_len = sizeof(ctx->buf);

    ret = websocket_connect_wait_data(ctx->sock, &req, (void*)h);
    if (ret < 0) {
        LOG_ERR("Cannot connect to %s:%d (%d)", param->addr, param->port, ret);
        emit_worker_event(WORKER_ID_ERROR, h);
    }

    LOG_INF("worker_websocket_connect_wait_data() <<<");
}

static void worker_disconnect(BSC_WEBSOCKET_CONNECTION *ctx)
{
    if (ctx->state == BSC_WEBSOCKET_STATE_CONNECTED) {
        websocket_send_msg(ctx->websock, NULL, 0,
            WEBSOCKET_OPCODE_CLOSE, false, true, SYS_FOREVER_MS);
    }
    if (ctx->websock >= 0)
        websocket_disconnect(ctx->websock);
    else
        zsock_close(ctx->sock);
    ctx->state = BSC_WEBSOCKET_STATE_DISCONNECTED;
}

void bws_cli_disconnect(BSC_WEBSOCKET_HANDLE h)
{
    LOG_INF("bws_cli_disconnect() >>> h = %d", h);
    emit_worker_event(WORKER_ID_DISCONNECT, h);
    LOG_INF("bws_cli_disconnect() <<<");
}

static void bws_cli_worker(void *p1, void *p2, void *p3)
{
    int ret = 0;
    int i;
    uint64_t remaining = ULLONG_MAX;
    uint32_t message_type;
    BSC_WEBSOCKET_CONNECTION *ctx;
    bool need_renumber_fds = false;
    uint8_t event[2];
    int spair[2];
    int timeout;

    ret = zsock_socketpair(AF_UNIX, SOCK_STREAM, 0, spair);
    if (ret == 0) {
        worker_event_fd = spair[0];
        fds[EVENT_FDS_INDEX].fd = spair[1];
        fds[EVENT_FDS_INDEX].events = ZSOCK_POLLIN;
        fds[EVENT_FDS_INDEX].revents = 0;
    } else {
        LOG_ERR("socketpair failed: %d", errno);
        return;
    }
    
    timeout = renumber_fds();

    while (1) {
        LOG_INF("zsock_polling timeout %d", timeout);
        ret = zsock_poll(fds, BSC_CLIENT_WEBSOCKETS_MAX_NUM +1, timeout);
        LOG_INF("zsock_polled: %d", ret);

        if ((ret == -1) || (ret == 0)) {
            if (ret == -1)
                LOG_ERR("zsock_poll error: %d", errno);
            timeout = renumber_fds();
            continue;
        }

        need_renumber_fds = false;

        // receive & connect
        for (i = 0; i < BSC_CLIENT_WEBSOCKETS_MAX_NUM; i++) {
            if (fds[i].revents & (ZSOCK_POLLHUP | ZSOCK_POLLERR)) {
                need_renumber_fds = true;
            }

            ctx = &bws_cli_conn[i];

            if (!(fds[i].revents & ZSOCK_POLLIN)) {
                continue;
            }

            LOG_INF("connect %d revents %d", i, fds[i].revents);

            if (ctx->state == BSC_WEBSOCKET_STATE_WEB_CONNECTING) {
                worker_websocket_connect_wait_data(i);
                need_renumber_fds = true;
                continue;
            }

            fds[i].revents = 0;
            remaining = 1;
            while (remaining > 0) {
                ret = websocket_recv_msg(ctx->websock, ctx->buf,
                    sizeof(ctx->buf), &message_type, &remaining, 0);
                if (ret <= 0) {
                    LOG_ERR("Error websocket received: %d", ret);
                    break;
                }

                LOG_INF("websocket_recv_msg ret %d, type %d, remaining %lld",
                    ret, message_type, remaining);

                if (message_type & WEBSOCKET_FLAG_PING) {
                    ret = websocket_send_msg(ctx->websock, ctx->buf, ret,
                        WEBSOCKET_OPCODE_PONG, false, true, SYS_FOREVER_MS);
                    LOG_INF("Sent PONG, status %d", ret);
                }

                if (message_type & WEBSOCKET_FLAG_CLOSE) {
                    LOG_DBG("Receive message Close");
                    worker_disconnect(ctx);
                    ctx->dispatch(event[1], BSC_WEBSOCKET_DISCONNECTED, NULL, 0,
                        ctx->user_param);
                    need_renumber_fds = true;
                }

                if (message_type & (WEBSOCKET_FLAG_TEXT |
                                    WEBSOCKET_FLAG_BINARY)) {
                    LOG_DBG("Receive message Data, len %d", ret);
                    ctx->dispatch(i, BSC_WEBSOCKET_RECEIVED, ctx->buf, ret,
                        ctx->user_param);
                }
            }
        }

        // worker evets
        if (fds[EVENT_FDS_INDEX].revents & ZSOCK_POLLIN) {
            fds[EVENT_FDS_INDEX].revents = 0;
            zsock_recv(fds[EVENT_FDS_INDEX].fd, event, sizeof(event),
                ZSOCK_MSG_DONTWAIT);
            LOG_INF("worker event happend, value %d idx %d", event[0], event[1]);
            ctx = &bws_cli_conn[event[1]];

            switch (event[0]) {
                case WORKER_ID_CONNECT:
                    switch(ctx->state)
                    {
                        case BSC_WEBSOCKET_STATE_CONFIGURING:
                            worker_zsock_connect(event[1]);
                            break;
                        case BSC_WEBSOCKET_STATE_TCP_CONNECTING:
                            worker_websocket_connect(event[1]);
                            break;
                        case BSC_WEBSOCKET_STATE_CONNECTED:
                            ctx->dispatch(event[1], BSC_WEBSOCKET_CONNECTED,
                                NULL, 0, ctx->user_param);
                            break;
                        default:
                            break;
                    }
                    need_renumber_fds = true;
                    break;
                case WORKER_ID_DISCONNECT:
                    worker_disconnect(ctx);
                    need_renumber_fds = true;
                    ctx->dispatch(event[1], BSC_WEBSOCKET_DISCONNECTED, NULL, 0,
                        ctx->user_param);
                    break;
                case WORKER_ID_SEND:
                    ctx->dispatch(event[1], BSC_WEBSOCKET_SENDABLE, NULL, 0,
                        ctx->user_param);
                    break;
                case WORKER_ID_ERROR:
                    worker_disconnect(ctx);
                    need_renumber_fds = true;
                    ctx->dispatch(event[1], BSC_WEBSOCKET_DISCONNECTED, NULL, 0,
                        ctx->user_param);
                    break;
            }
        }

        if (need_renumber_fds) {
            timeout = renumber_fds();
        }
    }
}

void bws_cli_send(BSC_WEBSOCKET_HANDLE h)
{
    LOG_INF("bws_cli_send() >>> h =  %d", h);
    emit_worker_event(WORKER_ID_SEND, h);
    LOG_INF("bws_cli_send() >>>");
}

BSC_WEBSOCKET_RET bws_cli_dispatch_send(BSC_WEBSOCKET_HANDLE h,
    uint8_t *payload, size_t payload_size)
{
    BSC_WEBSOCKET_CONNECTION *ctx;
    BSC_WEBSOCKET_RET ret;

    LOG_INF("bws_cli_dispatch_send() >>> h = %d, payload = %p, payload_size = %d",
        h, payload, payload_size);

    if (h < 0 || h >= BSC_CLIENT_WEBSOCKETS_MAX_NUM) {
        LOG_INF("bws_cli_dispatch_send() <<< ret = BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (!payload || !payload_size) {
        LOG_INF("bws_cli_dispatch_send() <<< ret = BSC_WEBSOCKET_BAD_PARAM");
        return BSC_WEBSOCKET_BAD_PARAM;
    }

    if (z_current_get() != worker_thread) {
        LOG_ERR("bws_cli_dispatch_send() <<< ret = BSC_WEBSOCKET_INVALID_OPER");
        return BSC_WEBSOCKET_INVALID_OPERATION;
    }

    ctx = &bws_cli_conn[h];
    if (ctx->state != BSC_WEBSOCKET_STATE_CONNECTED) {
        LOG_ERR("bws_cli_dispatch_send() <<< ret = BSC_WEBSOCKET_NO_RESOURCES");
        return BSC_WEBSOCKET_NO_RESOURCES;
    }

    ret = bsc_websocket_retcode(
        websocket_send_msg(ctx->websock, payload, payload_size,
            WEBSOCKET_OPCODE_DATA_BINARY, false, true, SYS_FOREVER_MS));

    LOG_INF("bws_cli_dispatch_send() <<< ret = %d", ret);
    return ret;
}
