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

#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bacnet/basic/sys/keylist.h"
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

static int ws_http_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len);

static const struct lws_protocols ws_http_protocol = { "http", ws_http_event,
    /*sizeof(struct pss)*/ 0, 0, 0, NULL, 0 };

static const struct lws_protocols *ws_protocols[] = { &ws_http_protocol, NULL };

static const struct lws_http_mount ws_mount = {
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
    /* .mountpoint_len */ 4, /* char count */
    /* .basic_auth_login_file */ NULL
};

typedef struct {
    struct lws_context *ctx;
    pthread_mutex_t* mutex;
    bool stop_worker;
    bool used;
} WS_SERVER;

static pthread_mutex_t ws_srv_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static WS_SERVER ws_srv = { NULL, &ws_srv_mutex, false, false };

static const char *const param_names[] = { "z", "send", NULL, NULL, NULL };

enum enum_param_names { EPN_TEXT1, EPN_SEND };

static uint32_t djb2_hash(uint8_t *str)
{
    uint32_t h = 5381;
    uint8_t c;
    while ((c = *str++))
        h = ((h << 5) + h) + c;
    return h;
}

static int ws_http_event(struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
#if 0
  uint8_t buf[LWS_PRE + 2048], *start = &buf[LWS_PRE], *p = start,
    *end = &buf[sizeof(buf) - 1];
  time_t t;
  int n;
  int m;
  WS_SERVICE_CTX* ctx = (WS_SERVICE_CTX *)lws_context_user(lws_get_context(wsi));

    static const char * const methods_names[] = {
      "GET", "POST","PUT", "DELETE"
    };

    static const unsigned char methods[] = {
      WSI_TOKEN_GET_URI,
      WSI_TOKEN_POST_URI,
      WSI_TOKEN_PUT_URI,
      WSI_TOKEN_DELETE_URI
    };

#if defined(LWS_HAVE_CTIME_R)
  char date[32];
#endif

  printf("reason = %d\n", reason);
  switch (reason) {
  case LWS_CALLBACK_HTTP:

    /*
     * If you want to know the full url path used, you can get it
     * like this
     *
     * n = lws_hdr_copy(wsi, buf, sizeof(buf), WSI_TOKEN_GET_URI);
     *
     * The base path is the first (n - strlen((const char *)in))
     * chars in buf.
     */


    lws_get_peer_simple(wsi, (char *)buf, sizeof(buf));
    printf("%s: HTTP: connection %s, path %s\n", __func__,
        (const char *)buf, (char*)in);

      for (m = 0; m < (int)LWS_ARRAY_SIZE(methods); m++)
        if (lws_hdr_total_length(wsi, methods[m]) >=
            len) {
         printf("%s\n", methods_names[m]);
          break;
        }

    /*
     * Demonstrates how to retreive a urlarg x=value
     */

    {
      char value[100];
      int z = lws_get_urlarg_by_name_safe(wsi, "x", value,
             sizeof(value) - 1);

      if (z >= 0)
        lwsl_hexdump_notice(value, (size_t)z);
    }
#if 0
    /*
     * prepare and write http headers... with regards to content-
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
    if (lws_add_http_common_headers(wsi, HTTP_STATUS_OK,
        "text/html",
        LWS_ILLEGAL_HTTP_CONTENT_LEN, /* no content len */
        &p, end))
      return 1;
    if (lws_finalize_write_http_header(wsi, start, &p, end))
      return 1;

    /* write the body separately */
    lws_callback_on_writable(wsi);
#endif
    return 0;
  case LWS_CALLBACK_HTTP_BODY:
    printf("LWS_CALLBACK_HTTP_BODY\n");
    /* create the POST argument parser if not already existing */

    if (!ctx->spa) {
      ctx->spa = lws_spa_create(wsi, param_names,
          LWS_ARRAY_SIZE(param_names), 1024,
          NULL, NULL); /* no file upload */
      if (!ctx->spa)
        return -1;
    }

    /* let it parse the POST data */
    printf("in = %s\n", in);
    if (lws_spa_process(ctx->spa, in, (int)len))
      return -1;
    break;

  case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
      printf("LWS_CALLBACK_CLOSED_CLIENT_HTTP\n");

    if (ctx->spa && lws_spa_destroy(ctx->spa))
      return -1;
    break;

  case LWS_CALLBACK_HTTP_BODY_COMPLETION:

    /* inform the spa no more payload data coming */

    printf("LWS_CALLBACK_HTTP_BODY_COMPLETION\n");

    /* we just dump the decoded things to the log */

    if (ctx->spa)
    lws_spa_finalize(ctx->spa);
      for (n = 0; n < (int)LWS_ARRAY_SIZE(param_names); n++) {
        if (!lws_spa_get_string(ctx->spa, n))
          printf("%s: undefined\n", param_names[n]);
        else
          printf("%s: (len %d) '%s'\n",
              param_names[n],
              lws_spa_get_length(ctx->spa, n),
              lws_spa_get_string(ctx->spa, n));
      }


    break;

  case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
  printf("LWS_CALLBACK_HTTP_DROP_PROTOCOL\n");
    /* called when our wsi user_space is going to be destroyed */
    if (ctx->spa) {
      lws_spa_destroy(ctx->spa);
      ctx->spa = NULL;
    }
    break;
  case LWS_CALLBACK_HTTP_WRITEABLE:


    /*
     * We send a large reply in pieces of around 2KB each.
     *
     * For http/1, it's possible to send a large buffer at once,
     * but lws will malloc() up a temp buffer to hold any data
     * that the kernel didn't accept in one go.  This is expensive
     * in memory and cpu, so it's better to stage the creation of
     * the data to be sent each time.
     *
     * For http/2, large data frames would block the whole
     * connection, not just the stream and are not allowed.  Lws
     * will call back on writable when the stream both has transmit
     * credit and the round-robin fair access for sibling streams
     * allows it.
     *
     * For http/2, we must send the last part with
     * LWS_WRITE_HTTP_FINAL to close the stream representing
     * this transaction.
     */

      t = time(NULL);
      /*
       * to work with http/2, we must take care about LWS_PRE
       * valid behind the buffer we will send.
       */
      p += lws_snprintf((char *)p, lws_ptr_diff_size_t(end, p), "<html>"
        "<head><meta charset=utf-8 "
        "http-equiv=\"Content-Language\" "
        "content=\"en\"/></head><body>"
        "<img src=\"/libwebsockets.org-logo.svg\">"
        "<br>Dynamic content for '%s' from mountpoint."
        "<br>Time: %s<br><br>"
        "</body></html>", NULL,
#if defined(LWS_HAVE_CTIME_R)
        ctime_r(&t, date));
#else
        ctime(&t);
#endif



    if (lws_write(wsi, (uint8_t *)start, lws_ptr_diff_size_t(p, start), (enum lws_write_protocol)n) !=
        lws_ptr_diff(p, start))
      return 1;
     if (lws_http_transaction_completed(wsi))
        return -1;
    /*
     * HTTP/1.0 no keepalive: close network connection
     * HTTP/1.1 or HTTP1.0 + KA: wait / process next transaction
     * HTTP/2: stream ended, parent connection remains up
     */
    if (n == LWS_WRITE_HTTP_FINAL) {
        if (lws_http_transaction_completed(wsi))
      return -1;
    } else
      lws_callback_on_writable(wsi);

    return 0;

  default:
    break;
  }
#endif
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
    int ret;
    WS_SERVER *ctx = &ws_srv;
    pthread_attr_t attr;
    int r;
    struct lws_context *c = NULL;

    DEBUG_PRINTF(
        "ws_server_start() >>> http_port= %d, https_port = %d, http_iface = %s, "
        "https_iface = %s, ca_cert = %p, ca_cert_size = %d, cert = %p, "
        "cert_size = %d, key = %p, key_size = %d, timeout_s = %d\n",
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
    info.options |= LWS_SERVER_OPTION_FAIL_UPON_UNABLE_TO_BIND;
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

    // http server setup

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


    // https server setup

    info.port = https_port;
    info.iface = https_iface;
    info.vhost_name = "https";
    info.server_ssl_cert_mem = cert;
    info.server_ssl_cert_mem_len = cert_size;
    info.server_ssl_ca_mem = ca_cert;
    info.server_ssl_ca_mem_len = ca_cert_size;
    info.server_ssl_private_key_mem = key;
    info.server_ssl_private_key_mem_len = key_size;

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

    bsc_websocket_global_unlock();
    pthread_mutex_lock(ctx->mutex);
    ctx->ctx = c;
    ctx->stop_worker = false;

    r = pthread_attr_init(&attr);

    if (!r) {
        r = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    if (!r) {
        r = pthread_create(&thread_id, &attr, &ws_service_srv_worker, &ctx);
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
  DEBUG_PRINTF("ws_service_registry() >>> s = %p\n", s);
  pthread_mutex_lock(ctx->mutex);
  if(!ctx->used || !ctx->ctx) {
    ret = BACNET_WS_SERVICE_INVALID_OPERATION;
  }
  else {
    // TODO!
  }
  pthread_mutex_unlock(ctx->mutex);
  DEBUG_PRINTF("ws_service_registry() <<< ret = %d\n",ret);
  return ret;
}