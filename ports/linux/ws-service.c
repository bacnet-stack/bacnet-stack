/**
 * @file
 * @brief HTTP/HTTPS thread-safe BACNet/WS service API.
 * @author Kirill Neznamov
 * @date June 2023
 * @section LICENSE
 *
 * Copyright (C) 2023 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#define _GNU_SOURCE
#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bacnet/basic/sys/keylist.h"
#define HTTP_STATUS
#include "bacnet/basic/service/ws_restful/ws-service.h"
#include "bacnet/basic/sys/debug.h"
#include "websocket-global.h"

#define DEBUG_BACNET_WS_SERVICE 0

#if DEBUG_BACNET_WS_SERVICE == 1
#define DEBUG_PRINTF printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF debug_printf_disabled
#endif


BACNET_WS_DECLARE_SERVICE(root_service, "", 0x0f, false, NULL);

static int ws_http_event(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len);

static int http_headers_write(struct lws *wsi, int http_retcode, int alt,
    bool base64_hdr);

static const struct lws_protocols ws_http_protocol = { "http", ws_http_event,
    sizeof(BACNET_WS_CONNECT_CTX), 0, 0, NULL, 0 };
static const struct lws_protocols *ws_protocols[] = { &ws_http_protocol, NULL };

static const struct lws_http_mount ws_mount_https = {
    /* .mount_next */ NULL, /* linked-list "next" */
    /* .mountpoint */ "/", /* mountpoint URL */
    /* .origin */ NULL, /* protocol */
    /* .def */ NULL,
    /* .protocol */ "http",
    /* .cgienv */ NULL,
    /* .extra_mimetypes */ NULL,
    /* .interpret */ NULL,
    /* .cgi_timeout */ 0,
    /* .cache_max_age */ 0,
    /* .auth_mask */ 0,
    /* .cache_reusable */ 0,
    /* .cache_revalidate */ 0,
    /* .cache_intermediaries */ 0,
    /* .cache_no */ 0,
    /* .origin_protocol */ LWSMPRO_CALLBACK, /* dynamic */
    /* .mountpoint_len */ 1, /* char count */
    /* .basic_auth_login_file */ NULL
};

static const struct lws_http_mount ws_mount = {
    /* .mount_next */ &ws_mount_https, /* linked-list "next" */
    /* .mountpoint */ "/", /* mountpoint URL */
    /* .origin */ NULL, /* protocol */
    /* .def */ NULL,
    /* .protocol */ "http",
    /* .cgienv */ NULL,
    /* .extra_mimetypes */ NULL,
    /* .interpret */ NULL,
    /* .cgi_timeout */ 0,
    /* .cache_max_age */ 0,
    /* .auth_mask */ 0,
    /* .cache_reusable */ 0,
    /* .cache_revalidate */ 0,
    /* .cache_intermediaries */ 0,
    /* .cache_no */ 0,
    /* .origin_protocol */ LWSMPRO_CALLBACK, /* dynamic */
    /* .mountpoint_len */ 1, /* char count */
    /* .basic_auth_login_file */ NULL
};

typedef struct {
    struct lws_context *ctx;
    pthread_mutex_t* mutex;
    bool stop_worker;
    bool used;
    BACNET_WS_SERVICE *services;
} WS_SERVER;

static pthread_mutex_t ws_srv_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static WS_SERVER ws_srv = { NULL, &ws_srv_mutex, false, false, NULL };

static uint32_t djb2_hash(uint8_t *str)
{
    uint32_t h = 5381;
    uint8_t c;
    while ((c = *str++))
        h = ((h << 5) + h) + c;
    return h;
}

static BACNET_WS_SERVICE* ws_service_get(char *uri)
{
    WS_SERVER *ctx = &ws_srv;
    uint32_t hash = djb2_hash((uint8_t*)uri);
    BACNET_WS_SERVICE *s;

    pthread_mutex_lock(ctx->mutex);
    for (s = ctx->services; s != NULL && s->hash != hash; s = s->next)
        ;
    pthread_mutex_unlock(ctx->mutex);
    return s;
}

static uint8_t ws_get_method(struct lws *wsi)
{
    if (lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI))
        return BACNET_WS_SERVICE_METHOD_GET;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI))
        return BACNET_WS_SERVICE_METHOD_POST;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_PUT_URI))
        return BACNET_WS_SERVICE_METHOD_PUT;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_DELETE_URI))
        return BACNET_WS_SERVICE_METHOD_DELETE;
    return 0;
}

/*
 * Retreive an alt parameter, see BacNET spec Clause W.8.1
 */
static BACNET_WS_ALT ws_alt_get(struct lws *wsi)
{
    const char* alt_values[] = {"xml", "json", "plain", "media" };
    char alt[10] = {0};
    int i;

    if (ws_http_parameter_get(wsi, "alt", alt, sizeof(alt)) <= 0) {
        return BACNET_WS_ALT_JSON;
    }

    for (i = 0; i < LWS_ARRAY_SIZE(alt_values); ++i) {
        if (strcmp(alt_values[i], alt) == 0) {
            return (BACNET_WS_ALT)i;
        }
    }

    return BACNET_WS_ALT_ERROR;
}

static bool body_collect(BACNET_WS_CONNECT_CTX *ctx, uint8_t* in, size_t in_len)
{
    uint8_t *buf = NULL;

    if (ctx->body_data == NULL) {
        ctx->body_data = buf = calloc(1, in_len);
    } else {
        buf = calloc(1, in_len + ctx->body_data_size);
        if (buf != NULL) {
            memcpy(buf, ctx->body_data, ctx->body_data_size);
            free(ctx->body_data);
            ctx->body_data = buf;
            buf += ctx->body_data_size;
        }
    }

    if (buf == NULL) {
        return false;
    }
    memcpy(buf, in, in_len);
    ctx->body_data_size += in_len;

    return true;
}

/*
 * Note! If any endpoint use media content then it must set
 */
static char* alt_header[] =
    {"application/xml", "application/json", "text/plain", "media" };

#define WS_HTTP_RESPONCE_ERROR(http_error_code)                     \
    if (lws_add_http_common_headers(wsi, http_error_code,           \
            alt_header[BACNET_WS_ALT_PLAIN], 0,  &p, end)) {        \
        return 1;                                                   \
    }                                                               \
    (void)lws_finalize_write_http_header(wsi, start, &p, end);


static int ws_http_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    BACNET_WS_CONNECT_CTX *pss = (BACNET_WS_CONNECT_CTX*)user;
    char path[128];
    uint8_t buf[LWS_PRE + 2048], *start = &buf[LWS_PRE], *p = start,
      *end = &buf[sizeof(buf) - 1];
    BACNET_WS_SERVICE_RET ret;
    enum lws_write_protocol n;

    switch (reason) {
    case LWS_CALLBACK_HTTP:
        pss->context = wsi;
        pss->body_data = NULL;
        pss->body_data_size = 0;
        pss->endpoint_data = NULL;
        pss->http_retcode = HTTP_STATUS_OK;
        pss->base64_body = false;
        pss->headers_written = false;

        lws_snprintf(path, sizeof(path), "%s", (const char *)in);
        pss->service = ws_service_get(path);
        if (pss->service == NULL) {
            DEBUG_PRINTF("Error: unknown service %s\n", path);
            WS_HTTP_RESPONCE_ERROR(HTTP_STATUS_FORBIDDEN);
            return 1;
        }

        if (pss->service->https_only) {
            if (strncmp(lws_get_vhost_name(lws_get_vhost(wsi)),
                        "https", 5)!=0) {
                DEBUG_PRINTF("Error: https only\n");
                WS_HTTP_RESPONCE_ERROR(HTTP_STATUS_FORBIDDEN);
                return 1;
            }
        }

        pss->method = ws_get_method(wsi);
        if ((pss->method & pss->service->ws_method_mask) == 0) {
            DEBUG_PRINTF("Error: method %d is not allow\n", pss->method);
            WS_HTTP_RESPONCE_ERROR(HTTP_STATUS_FORBIDDEN);
            return 1;
        }

        pss->alt = ws_alt_get(wsi);
        if (pss->alt == BACNET_WS_ALT_ERROR) {
            DEBUG_PRINTF("Error: ALT is WS_ERR_PARAM_OUT_OF_RANGE\n");
            WS_HTTP_RESPONCE_ERROR(HTTP_STATUS_FORBIDDEN);
            return 1;
        }

        /* write the body separately */
        lws_callback_on_writable(wsi);
        return 0;

    case LWS_CALLBACK_HTTP_BODY:
        if (!pss || pss->service == NULL) {
            break;
        }

        if (!body_collect(pss, in, len)) {
            DEBUG_PRINTF("Error: set_body()\n");
            return 1;
        }
        break;

  case LWS_CALLBACK_HTTP_WRITEABLE:

        if (!pss || pss->service == NULL) {
            break;
        }

        if (pss->service->handle_cb == NULL) {
            return lws_callback_http_dummy(wsi, reason, user, in, len);
        }

        ret = pss->service->handle_cb(pss, in, len, &p, end);
        if ((ret != BACNET_WS_SERVICE_SUCCESS) &&
            (ret != BACNET_WS_SERVICE_HAS_DATA)) {
            DEBUG_PRINTF("Error: callback return %d\n", ret);
            return 1;
        }

        if (!pss->headers_written) {
            if (http_headers_write(wsi, pss->http_retcode, pss->alt,
                pss->base64_body)) {
                return 1;
            }
            pss->headers_written = true;
        }

        n = (ret == BACNET_WS_SERVICE_SUCCESS) ?
            LWS_WRITE_HTTP_FINAL :
            LWS_WRITE_HTTP;

        if (lws_write(wsi, (uint8_t *)start, lws_ptr_diff_size_t(p, start), n)
            != lws_ptr_diff(p, start)) {
            return 1;
        }

        /*
         * HTTP/1.0 no keepalive: close network connection
         * HTTP/1.1 or HTTP1.0 + KA: wait / process next transaction
         * HTTP/2: stream ended, parent connection remains up
         */
        if (ret == BACNET_WS_SERVICE_SUCCESS) {
            if (lws_http_transaction_completed(wsi)) {
                return -1;
            }
        } else {
          lws_callback_on_writable(wsi);
        }

        return 0;

    case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
        /* called when our wsi user_space is going to be destroyed */
        free(pss->body_data);
        pss->body_data = NULL;
        free(pss->endpoint_data);
        pss->endpoint_data = NULL;
        break;

    default:
        break;
    }

    return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static void *ws_service_srv_worker(void *arg)
{
    WS_SERVER *s = (WS_SERVER *)arg;
    struct lws_context *ctx;

    while (1) {
        pthread_mutex_lock(s->mutex);
        if (s->stop_worker) {
            ctx = s->ctx;
            s->ctx = NULL;
            pthread_mutex_unlock(s->mutex);
            bsc_websocket_global_lock();
            lws_context_destroy(ctx);
            bsc_websocket_global_unlock();
            pthread_mutex_lock(s->mutex);
            s->used = false;
            pthread_mutex_unlock(s->mutex);
            return NULL;
         }
        pthread_mutex_unlock(s->mutex);
        lws_service(s->ctx, 0);
    }

    return NULL;
}

BACNET_WS_SERVICE_RET ws_server_start(uint16_t http_port,
    uint16_t https_port,
    char *http_iface,
    char *https_iface,
    uint8_t *ca_cert,
    size_t ca_cert_size,
    uint8_t *cert,
    size_t cert_size,
    uint8_t *key,
    size_t key_size,
    size_t timeout_s)
{
    pthread_t thread_id;
    struct lws_context_creation_info info;
    WS_SERVER *ctx = &ws_srv;
    pthread_attr_t attr;
    int r;
    struct lws_context *c = NULL;

    DEBUG_PRINTF(
        "ws_server_start() >>> http_port= %d, https_port = %d, http_iface = %s, "
        "https_iface = %s, ca_cert = %p, ca_cert_size = %ld, cert = %p, "
        "cert_size = %ld, key = %p, key_size = %ld, timeout_s = %ld\n",
        http_port, https_port, http_iface, https_iface, ca_cert, ca_cert_size,
        cert, cert_size, key, key_size, timeout_s);

    bsc_websocket_init_log();

    if (!ca_cert || !ca_cert_size || !cert || !cert_size || !key || !key_size) {
        DEBUG_PRINTF(
            "ws_server_start) <<< ret = %d\n", BACNET_WS_SERVICE_BAD_PARAM);
        return BACNET_WS_SERVICE_BAD_PARAM;
    }

    pthread_mutex_lock(ctx->mutex);

    if(ctx->used || ctx->ctx)
    {
      pthread_mutex_unlock(ctx->mutex);
      DEBUG_PRINTF(
            "ws_server_start) <<< ret = %d\n", BACNET_WS_SERVICE_INVALID_OPERATION);
      return BACNET_WS_SERVICE_INVALID_OPERATION;
    }

    ctx->used = true;
    pthread_mutex_unlock(ctx->mutex);

    memset(&info, 0, sizeof(info));
    info.pprotocols = ws_protocols;
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    /*info.options |= LWS_SERVER_OPTION_FAIL_UPON_UNABLE_TO_BIND;*/
    info.options |= LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
    info.options |=
        LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    info.user = ctx;

    bsc_websocket_global_lock();
    c = lws_create_context(&info);
    bsc_websocket_global_unlock();

    if (!c) {
        pthread_mutex_lock(ctx->mutex);
        ctx->used = false;
        pthread_mutex_unlock(ctx->mutex);

        DEBUG_PRINTF("ws_server_start() lws_create_context() failed\n");
        DEBUG_PRINTF(
            "ws_server_start() <<< ret = %d\n", BACNET_WS_SERVICE_NO_RESOURCES);
        return BACNET_WS_SERVICE_NO_RESOURCES;
    }

    /* http server setup */

    info.gid = -1;
    info.uid = -1;
    info.port = http_port;
    info.iface = http_iface;
    info.mounts = &ws_mount;
    info.vhost_name = "http";
    info.timeout_secs = timeout_s;
    info.connect_timeout_secs = timeout_s;

    bsc_websocket_global_lock();

    if (!lws_create_vhost(c, &info)) {
        pthread_mutex_lock(ctx->mutex);
        ctx->used = false;
        pthread_mutex_unlock(ctx->mutex);
        lws_context_destroy(c);
        bsc_websocket_global_unlock();
        DEBUG_PRINTF(
            "ws_server_start() <<< ret = %d\n", BACNET_WS_SERVICE_NO_RESOURCES);
        return BACNET_WS_SERVICE_NO_RESOURCES;
    }

    /* https server setup */

    info.port = https_port;
    info.iface = https_iface;
    //info.mounts = &ws_mount_https;
    info.vhost_name = "https";
    info.server_ssl_cert_mem = cert;
    info.server_ssl_cert_mem_len = cert_size;
    info.server_ssl_ca_mem = ca_cert;
    info.server_ssl_ca_mem_len = ca_cert_size;
    info.server_ssl_private_key_mem = key;
    info.server_ssl_private_key_mem_len = key_size;
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    //bsc_websocket_global_lock();

    if (!lws_create_vhost(c, &info)) {
        pthread_mutex_lock(ctx->mutex);
        ctx->used = false;
        pthread_mutex_unlock(ctx->mutex);
        lws_context_destroy(c);
        bsc_websocket_global_unlock();
        DEBUG_PRINTF(
            "ws_server_start() <<< ret = %d\n", BACNET_WS_SERVICE_NO_RESOURCES);
        return BACNET_WS_SERVICE_NO_RESOURCES;
    }

    bsc_websocket_global_unlock();
    pthread_mutex_lock(ctx->mutex);
    ctx->ctx = c;
    ctx->stop_worker = false;

    r = pthread_attr_init(&attr);

    if (!r) {
        r = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    if (!r) {
        r = pthread_create(&thread_id, &attr, &ws_service_srv_worker, ctx);
    }

    if (r) {
        ctx->ctx = NULL;
        ctx->used = false;
        pthread_mutex_unlock(ctx->mutex);
        bsc_websocket_global_lock();
        lws_context_destroy(c);
        bsc_websocket_global_unlock();
        DEBUG_PRINTF(
            "ws_server_start() <<< ret = %d\n", BACNET_WS_SERVICE_NO_RESOURCES);
        return BACNET_WS_SERVICE_NO_RESOURCES;
    }
  
    pthread_mutex_unlock(ctx->mutex);
    pthread_attr_destroy(&attr);

    ws_service_registry(root_service);

    DEBUG_PRINTF("ws_server_start() <<< ret = %d\n", BACNET_WS_SERVICE_SUCCESS);
    return BACNET_WS_SERVICE_SUCCESS;
}

void ws_server_stop(void)
{
    WS_SERVER *ctx = &ws_srv;
    DEBUG_PRINTF("ws_server_stop() >>>\n");
    pthread_mutex_lock(ctx->mutex);
    if(ctx->used && ctx->ctx) {
        ctx->stop_worker = true;
        lws_cancel_service(ctx->ctx);
    }
    pthread_mutex_unlock(ctx->mutex);
    DEBUG_PRINTF("ws_server_stop() <<< \n");
}

BACNET_WS_SERVICE_RET ws_service_registry(BACNET_WS_SERVICE* s)
{
    WS_SERVER *ctx = &ws_srv;
    BACNET_WS_SERVICE_RET ret = BACNET_WS_SERVICE_SUCCESS;
    BACNET_WS_SERVICE* srv;

    DEBUG_PRINTF("ws_service_registry() >>> s = %p\n", (void*)s);
    pthread_mutex_lock(ctx->mutex);
    if(!ctx->used || !ctx->ctx) {
        ret = BACNET_WS_SERVICE_INVALID_OPERATION;
    } else {
        s->hash = djb2_hash((uint8_t*)s->uri);
        if (ctx->services == NULL) {
            ctx->services = s;
        } else {
            for(srv = ctx->services; srv->next != NULL; srv = srv->next)
                ;
          srv->next = s;
        }
    }
    pthread_mutex_unlock(ctx->mutex);
    DEBUG_PRINTF("ws_service_registry() <<< ret = %d\n",ret);
    return ret;
}

/*
 * TODO ws_http_parameter_get_mock() must be removed after implement
 * WS REST Client and rewrite rest endpoints tests
 */
#ifdef CONFIG_ZTEST

struct bacnet_http_parameter_mock
{
    char *name;
    char *value;
    size_t len;
};

struct bacnet_http_parameter_mock *ws_http_parameter_mocks = NULL;

static int ws_http_parameter_get_mock(
    void *context, char *name, char *buffer, size_t len)
{
    struct bacnet_http_parameter_mock* mock;
    (void)context;

    for (mock = ws_http_parameter_mocks; mock->name != NULL; mock++) {
        if (strcmp(mock->name, name) == 0) {
            memcpy(buffer, mock->value, mock->len);
            return mock->len;
        }
    }
    return -1;
}
#endif

/*
 * Retreive a urlarg x=value
 */
int ws_http_parameter_get(void *context, char *name, char *buffer, size_t len)
{
    struct lws *wsi = (struct lws *)context;

#ifdef CONFIG_ZTEST
    if (context == NULL) {
        return ws_http_parameter_get_mock(context, name, buffer, len);
    }
#endif

    return lws_get_urlarg_by_name_safe(wsi, name, buffer, len - 1);
}

/*
 * prepare http headers... with regards to content-
 * length, there are three approaches:
 *
 *  - http/1.0 or connection:close: no need, but no pipelining
 *  - http/1.1 or connected:keep-alive
 *     (keep-alive is default for 1.1): content-length required
 *  - http/2: no need, LWS_WRITE_HTTP_FINAL closes the stream
 *
 * giving the api below LWS_ILLEGAL_HTTP_CONTENT_LEN instead of
 * a content length forces the connection response headers to
 * send back "connection: close", disabling keep-alive.
 *
 * If you know the final content-length, it's always OK to give
 * it and keep-alive can work then if otherwise possible.  But
 * often you don't know it and avoiding having to compute it
 * at header-time makes life easier at the server.
 */
static int http_headers_write(struct lws *wsi, int http_retcode, int alt,
                                bool base64_hdr)
{
    uint8_t buf[LWS_PRE + 1024], *start = &buf[LWS_PRE], *p = start,
      *end = &buf[sizeof(buf) - 1];
    size_t len;

    if (lws_add_http_common_headers(wsi, http_retcode, alt_header[alt],
                                    LWS_ILLEGAL_HTTP_CONTENT_LEN, &p, end)) {
        return 1;
    }

    if (base64_hdr) {
        if (lws_add_http_header_by_name(wsi,
                        (const uint8_t *)"Content-Transfer-Encoding:",
                        (const uint8_t *)"base64",
                        6, &p, end)) {
            return 1;
        }
    }

    if (lws_finalize_http_header(wsi, &p, end)) {
        return 1;
    }

    len = lws_ptr_diff(p, start);

    if (lws_write(wsi, start, len, LWS_WRITE_HTTP_HEADERS) != len) {        
        return 1;
    }

    return 0;
}

#ifdef CONFIG_ZTEST
BACNET_WS_SERVICE *ws_service_root_get(void) {
    return ws_srv.services;
}

BACNET_WS_SERVICE *ws_service_get_debug(char *service_name)
{
    return ws_service_get(service_name);
}

#endif
