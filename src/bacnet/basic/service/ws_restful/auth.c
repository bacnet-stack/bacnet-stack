/**
 * @file
 * @brief HTTP/HTTPS auth endpoint.
 * @author Mikhail Antropov
 * @date Aug 2023
 * @section LICENSE
 *
 * Copyright (C) 2023 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <bacnet/basic/service/ws_restful/ws-service.h>
#include <bacnet/basic/service/ws_restful/oauth_server.h>
#include <bacnet/basic/service/ws_restful/base64.h>

/*
 * Endpoints ".auth/int"
 */

#define USER_NAME_LENGTH    16
#define PASSWORD_LENGTH     32
#define TOKEN_LENGTH        128
#define URI_MAX             256

static bool token_check(BACNET_WS_CONNECT_CTX *ctx)
{
    char token[TOKEN_LENGTH] = {0};
    int size;
    size = ws_http_parameter_get(ctx->context, "Token", token, sizeof(token));
    return oauth_token_check((uint8_t*)token, size);
}

static BACNET_WS_SERVICE_RET auth_int_user_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    int len;
    char user[USER_NAME_LENGTH] = {0};
    int ret;
    (void)in;
    (void)in_len;

    if (!token_check(ctx)) {
        ctx->http_retcode = HTTP_STATUS_UNAUTHORIZED;
        return BACNET_WS_SERVICE_SUCCESS;
    }

    ws_http_parameter_get(ctx->context, "User", user, sizeof(user));

    ret = oauth_user_set(user);
    if ( ret != 0) {
        ctx->http_retcode = HTTP_STATUS_NOT_ACCEPTABLE;
        ctx->alt = BACNET_WS_ALT_PLAIN;
        len = (int)(end - *out);
        *out += snprintf((char*)*out, len, "internal error: %d", ret);
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET auth_int_pass_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    int len;
    char pass[PASSWORD_LENGTH] = {0};
    int ret;
    (void)in;
    (void)in_len;

    if (!token_check(ctx)) {
        ctx->http_retcode = HTTP_STATUS_UNAUTHORIZED;
        return BACNET_WS_SERVICE_SUCCESS;
    }

    ws_http_parameter_get(ctx->context, "Password", pass, sizeof(pass));

    ret = oauth_pass_set(pass);
    if ( ret != 0) {
        ctx->http_retcode = HTTP_STATUS_NOT_ACCEPTABLE;
        ctx->alt = BACNET_WS_ALT_PLAIN;
        len = (int)(end - *out);
        *out += snprintf((char*)*out, len, "internal error: %d", ret);
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET auth_int_id_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    char id[USER_NAME_LENGTH] = {0};
    int ret;
    int len;
    (void)in;
    (void)in_len;

    if (!token_check(ctx)) {
        ctx->http_retcode = HTTP_STATUS_UNAUTHORIZED;
        return BACNET_WS_SERVICE_SUCCESS;
    }

    ws_http_parameter_get(ctx->context, "ID", id, sizeof(id));

    ret = oauth_id_set(id);
    if ( ret != 0) {
        ctx->http_retcode = HTTP_STATUS_NOT_ACCEPTABLE;
        ctx->alt = BACNET_WS_ALT_PLAIN;
        len = (int)(end - *out);
        *out += snprintf((char*)*out, len, "internal error: %d", ret);
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET auth_int_secret_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    int len;
    char secret[PASSWORD_LENGTH] = {0};
    int ret;
    (void)in;
    (void)in_len;

    if (!token_check(ctx)) {
        ctx->http_retcode = HTTP_STATUS_UNAUTHORIZED;
        return BACNET_WS_SERVICE_SUCCESS;
    }

    ws_http_parameter_get(ctx->context, "secret", secret, sizeof(secret));

    ret = oauth_secret_set(secret);
    if ( ret != 0) {
        ctx->http_retcode = HTTP_STATUS_NOT_ACCEPTABLE;
        ctx->alt = BACNET_WS_ALT_PLAIN;
        len = (int)(end - *out);
        *out += snprintf((char*)*out, len, "internal error: %d", ret);
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET auth_int_enable_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    char enable[10] = {0};
    int len;
    (void)in;
    (void)in_len;

    if (ctx->method == BACNET_WS_SERVICE_METHOD_GET) {
        len = (int)(end - *out);

        if (ctx->alt == BACNET_WS_ALT_PLAIN) {
            *out += snprintf((char*)*out, len, "%d", oauth_is_enable() ? 1 : 0);
        } else {
            *out += snprintf((char*)*out, len, "{ \"oauth_enable\": \"%s\" }",
                oauth_is_enable() ? "true" : "false");
        }
    } else {

        if (!token_check(ctx)) {
            ctx->http_retcode = HTTP_STATUS_UNAUTHORIZED;
        return BACNET_WS_SERVICE_SUCCESS;
        }

        ws_http_parameter_get(ctx->context, "enable", enable, sizeof(enable));

        if (strncmp(enable, "true", 4) == 0 || enable[0] == '1') {
            oauth_enable();
        } else {
            oauth_disable();
        }
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

/*
 * Endpoints ".auth/ext"
 */

#define RESPONCE_ERROR(ctx, out, end, err_str, ...)                         \
    ctx->http_retcode = HTTP_STATUS_NOT_ACCEPTABLE;                         \
    ctx->alt = BACNET_WS_ALT_PLAIN;                                         \
    *out += snprintf((char*)*out, (int)(end - *out), err_str, __VA_ARGS__);


static BACNET_WS_SERVICE_RET file_sender(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t** out, uint8_t *end, int (*get_data)(uint8_t **, size_t*))
{
    int ret;
    uint8_t *data;
    size_t data_size;
    size_t sent = (size_t)ctx->endpoint_data;
    int len;

    if (sent == 0) {
        ret = (*get_data)(&data, &data_size);
        if (ret != 0) {
            RESPONCE_ERROR(ctx, out, end, "internal error: %d", ret);
            return BACNET_WS_SERVICE_SUCCESS;
        }

        ctx->body_data_size = base64_encode_size(data_size);
        ctx->body_data = calloc(1, ctx->body_data_size);
        if (ret != 0) {
            RESPONCE_ERROR(ctx, out, end, "no memory: %d", ret);
            return BACNET_WS_SERVICE_SUCCESS;
        }

        ctx->base64_body = true;
        ctx->body_data_size = base64_encode(data, data_size, ctx->body_data);
    }

    len = (int)(end - *out);
    if (len > ctx->body_data_size - sent) {
        len = ctx->body_data_size - sent;
    }
    memcpy(*out, (uint8_t*)ctx->body_data + sent, len);
    *out += len;
    sent += len;
    ctx->endpoint_data = (void*)sent;
    if (sent < ctx->body_data_size) {
        return BACNET_WS_SERVICE_HAS_DATA;
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET file_receiver(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t** out, uint8_t *end, int (*set_data)(uint8_t *, size_t))
{
    int ret;
    size_t data_size;

    if (!token_check(ctx)) {
        ctx->http_retcode = HTTP_STATUS_UNAUTHORIZED;
        return BACNET_WS_SERVICE_SUCCESS;
    }

    data_size = base64_inplace_decode(ctx->body_data, ctx->body_data_size);
    ret = (*set_data)(ctx->body_data, data_size);
    if ( ret != 0) {
        RESPONCE_ERROR(ctx, out, end, "internal error: %d", ret);
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET auth_ext_pri_uri_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    char uri[URI_MAX] = {0};
    int ret;
    int len;
    (void)in;
    (void)in_len;

    if (ctx->method == BACNET_WS_SERVICE_METHOD_GET) {
        len = (int)(end - *out);

        if (ctx->alt == BACNET_WS_ALT_PLAIN) {
            *out += snprintf((char*)*out, len, "%s", oauth_pri_uri());
        } else {
            *out += snprintf((char*)*out, len, "{ \"PRI-URI\": \"%s\" }",
                oauth_pri_uri());
        }
    } else {

        if (!token_check(ctx)) {
            ctx->http_retcode = HTTP_STATUS_UNAUTHORIZED;
            return BACNET_WS_SERVICE_SUCCESS;
        }

        ws_http_parameter_get(ctx->context, "uri", uri, sizeof(uri));
        ret = oauth_pri_uri_set(uri);
        if ( ret != 0) {
            RESPONCE_ERROR(ctx, out, end, "internal error: %d", ret);
        }
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET auth_ext_pri_cert_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    (void)in;
    (void)in_len;

    if (ctx->method == BACNET_WS_SERVICE_METHOD_GET) {
        return file_sender(ctx, out, end, &oauth_pri_cert);
    }

    return file_receiver(ctx, out, end, &oauth_pri_cert_set);
}

static BACNET_WS_SERVICE_RET auth_ext_pri_pubkey_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    (void)in;
    (void)in_len;

    if (ctx->method == BACNET_WS_SERVICE_METHOD_GET) {
        return file_sender(ctx, out, end, &oauth_pri_pubkey);
    }

    return file_receiver(ctx, out, end, &oauth_pri_pubkey_set);
}

static BACNET_WS_SERVICE_RET auth_ext_sec_uri_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    char uri[URI_MAX] = {0};
    int ret;
    int len;
    (void)in;
    (void)in_len;

    if (ctx->method == BACNET_WS_SERVICE_METHOD_GET) {
        len = (int)(end - *out);

        if (ctx->alt == BACNET_WS_ALT_PLAIN) {
            *out += snprintf((char*)*out, len, "%s", oauth_sec_uri());
        } else {
            *out += snprintf((char*)*out, len, "{ \"SEC-URI\": \"%s\" }",
                oauth_sec_uri());
        }
    } else {

        if (!token_check(ctx)) {
            ctx->http_retcode = HTTP_STATUS_UNAUTHORIZED;
            return BACNET_WS_SERVICE_SUCCESS;
        }

        ws_http_parameter_get(ctx->context, "uri", uri, sizeof(uri));
        ret = oauth_sec_uri_set(uri);
        if ( ret != 0) {
            RESPONCE_ERROR(ctx, out, end, "internal error: %d", ret);
        }
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET auth_ext_sec_cert_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    (void)in;
    (void)in_len;

    if (ctx->method == BACNET_WS_SERVICE_METHOD_GET) {
        return file_sender(ctx, out, end, &oauth_sec_cert);
    }

    return file_receiver(ctx, out, end, &oauth_sec_cert_set);
}

static BACNET_WS_SERVICE_RET auth_ext_sec_pubkey_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    (void)in;
    (void)in_len;

    if (ctx->method == BACNET_WS_SERVICE_METHOD_GET) {
        return file_sender(ctx, out, end, &oauth_sec_pubkey);
    }

    return file_receiver(ctx, out, end, &oauth_sec_pubkey_set);
}
BACNET_WS_DECLARE_SERVICE(auth_int_user, ".auth/int/user",
    BACNET_WS_SERVICE_METHOD_POST | BACNET_WS_SERVICE_METHOD_PUT, false,
    auth_int_user_handler);

BACNET_WS_DECLARE_SERVICE(auth_int_pass, ".auth/int/pass",
    BACNET_WS_SERVICE_METHOD_POST | BACNET_WS_SERVICE_METHOD_PUT, false,
    auth_int_pass_handler);

BACNET_WS_DECLARE_SERVICE(auth_int_id, ".auth/int/id`",
    BACNET_WS_SERVICE_METHOD_POST | BACNET_WS_SERVICE_METHOD_PUT, false,
    auth_int_id_handler);

BACNET_WS_DECLARE_SERVICE(auth_int_secret, ".auth/int/secret",
    BACNET_WS_SERVICE_METHOD_POST | BACNET_WS_SERVICE_METHOD_PUT, false,
    auth_int_secret_handler);

BACNET_WS_DECLARE_SERVICE(auth_int_enable, ".auth/int/enable",
    BACNET_WS_SERVICE_METHOD_GET | BACNET_WS_SERVICE_METHOD_POST |
    BACNET_WS_SERVICE_METHOD_PUT, false, auth_int_enable_handler);


BACNET_WS_DECLARE_SERVICE(auth_ext_pri_uri, ".auth/ext/pri-uri",
    BACNET_WS_SERVICE_METHOD_GET | BACNET_WS_SERVICE_METHOD_POST |
    BACNET_WS_SERVICE_METHOD_PUT, false, auth_ext_pri_uri_handler);

BACNET_WS_DECLARE_SERVICE(auth_ext_pri_cert, ".auth/ext/pri-cert",
    BACNET_WS_SERVICE_METHOD_GET | BACNET_WS_SERVICE_METHOD_POST |
    BACNET_WS_SERVICE_METHOD_PUT, false, auth_ext_pri_cert_handler);

BACNET_WS_DECLARE_SERVICE(auth_ext_pri_pubkey, ".auth/ext/pri-pubkey",
    BACNET_WS_SERVICE_METHOD_GET | BACNET_WS_SERVICE_METHOD_POST |
    BACNET_WS_SERVICE_METHOD_PUT, false, auth_ext_pri_pubkey_handler);

BACNET_WS_DECLARE_SERVICE(auth_ext_sec_uri, ".auth/ext/sec-uri",
    BACNET_WS_SERVICE_METHOD_GET | BACNET_WS_SERVICE_METHOD_POST |
    BACNET_WS_SERVICE_METHOD_PUT, false, auth_ext_sec_uri_handler);

BACNET_WS_DECLARE_SERVICE(auth_ext_sec_cert, ".auth/ext/sec-cert",
    BACNET_WS_SERVICE_METHOD_GET | BACNET_WS_SERVICE_METHOD_POST |
    BACNET_WS_SERVICE_METHOD_PUT, false, auth_ext_sec_cert_handler);

BACNET_WS_DECLARE_SERVICE(auth_ext_sec_pubkey, ".auth/ext/sec-pubkey",
    BACNET_WS_SERVICE_METHOD_GET | BACNET_WS_SERVICE_METHOD_POST |
    BACNET_WS_SERVICE_METHOD_PUT, false, auth_ext_sec_pubkey_handler);



int ws_service_auth_registry(void)
{
    int ret;

    if ((ret = ws_service_registry(auth_int_user)) != 0)
        return ret;
    if ((ret = ws_service_registry(auth_int_pass)) != 0)
        return ret;
    if ((ret = ws_service_registry(auth_int_id)) != 0)
        return ret;
    if ((ret = ws_service_registry(auth_int_secret)) != 0)
        return ret;
    if ((ret = ws_service_registry(auth_int_enable)) != 0)
        return ret;

    if ((ret = ws_service_registry(auth_ext_pri_uri)) != 0)
        return ret;
    if ((ret = ws_service_registry(auth_ext_pri_cert)) != 0)
        return ret;
    if ((ret = ws_service_registry(auth_ext_pri_pubkey)) != 0)
        return ret;
    if ((ret = ws_service_registry(auth_ext_sec_uri)) != 0)
        return ret;
    if ((ret = ws_service_registry(auth_ext_sec_cert)) != 0)
        return ret;
    if ((ret = ws_service_registry(auth_ext_sec_pubkey)) != 0)
        return ret;

    return ret;
}
