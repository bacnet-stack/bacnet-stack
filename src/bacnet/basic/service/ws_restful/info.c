/**
 * @file
 * @brief HTTP/HTTPS info endpoint.
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
#include <bacnet/basic/object/device.h>

static BACNET_WS_SERVICE_RET info_vendor_id_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    int len;
    (void)in;
    (void)in_len;

    len = (int)(end - *out);

    if (ctx->alt == BACNET_WS_ALT_PLAIN) {
        *out += snprintf((char*)*out, len, "%d", Device_Vendor_Identifier());
    } else {
        *out += snprintf((char*)*out, len, "{ \"vendor-id\": %d }",
            Device_Vendor_Identifier());
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET info_vendor_name_handler(
    BACNET_WS_CONNECT_CTX *ctx, uint8_t* in, size_t in_len, uint8_t** out,
    uint8_t *end)
{
    int len;
    (void)in;
    (void)in_len;

    len = (int)(end - *out);

    if (ctx->alt == BACNET_WS_ALT_PLAIN) {
        *out += snprintf((char*)*out, len, "%s", Device_Vendor_Name());
    } else {
        *out += snprintf((char*)*out, len, "{ \"vendor-name\": \"%s\" }",
            Device_Vendor_Name());
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET info_model_name_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    int len;
    (void)in;
    (void)in_len;

    len = (int)(end - *out);

    if (ctx->alt == BACNET_WS_ALT_PLAIN) {
        *out += snprintf((char*)*out, len, "%s", Device_Model_Name());
    } else {
        *out += snprintf((char*)*out, len, "{ \"model-name\": \"%s\" }",
            Device_Model_Name());
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET info_software_version_handler(
    BACNET_WS_CONNECT_CTX *ctx, uint8_t* in, size_t in_len, uint8_t** out,
    uint8_t *end)
{
    int len;
    (void)in;
    (void)in_len;

    len = (int)(end - *out);

    if (ctx->alt == BACNET_WS_ALT_PLAIN) {
        *out += snprintf((char*)*out, len, "%s",
            Device_Application_Software_Version());
    } else {
        *out += snprintf((char*)*out, len,
            "{ \"software-version\": \"%s\" }",
            Device_Application_Software_Version());
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET info_protocol_version_handler(
    BACNET_WS_CONNECT_CTX *ctx, uint8_t* in, size_t in_len, uint8_t** out,
    uint8_t *end)
{
    int len;
    (void)in;
    (void)in_len;

    len = (int)(end - *out);

    if (ctx->alt == BACNET_WS_ALT_PLAIN) {
        *out += snprintf((char*)*out, len, "%d", Device_Protocol_Version());
    } else {
        *out += snprintf((char*)*out, len, "{ \"protocol-version\": %d }",
            Device_Protocol_Version());
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET info_protocol_revision_handler(
    BACNET_WS_CONNECT_CTX *ctx, uint8_t* in, size_t in_len, uint8_t** out,
    uint8_t *end)
{
    int len;
    (void)in;
    (void)in_len;

    len = (int)(end - *out);

    if (ctx->alt == BACNET_WS_ALT_PLAIN) {
        *out += snprintf((char*)*out, len, "%d", Device_Protocol_Revision());
    } else {
        *out += snprintf((char*)*out, len,
            "{ \"protocol-revision\": %d }", Device_Protocol_Revision());
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

static BACNET_WS_SERVICE_RET info_max_uri_handler(BACNET_WS_CONNECT_CTX *ctx,
    uint8_t* in, size_t in_len, uint8_t** out, uint8_t *end)
{
    int len;
    (void)in;
    (void)in_len;

    len = (int)(end - *out);

    // todo get max-uri from LWS

    if (ctx->alt == BACNET_WS_ALT_PLAIN) {
        *out += snprintf((char*)*out, len, "%d", 255);
    } else {
        *out += snprintf((char*)*out, len, "{ \"max-uri\": %d }", 255);
    }

    return BACNET_WS_SERVICE_SUCCESS;
}

BACNET_WS_DECLARE_SERVICE(info_vendor_id, ".info/vendor-identifier",
    BACNET_WS_SERVICE_METHOD_GET, false, info_vendor_id_handler);

BACNET_WS_DECLARE_SERVICE(info_vendor_name, ".info/vendor-name",
    BACNET_WS_SERVICE_METHOD_GET, false, info_vendor_name_handler);

BACNET_WS_DECLARE_SERVICE(info_model_name, ".info/model-name",
    BACNET_WS_SERVICE_METHOD_GET, false, info_model_name_handler);

BACNET_WS_DECLARE_SERVICE(info_software_version, ".info/software-version",
    BACNET_WS_SERVICE_METHOD_GET, false, info_software_version_handler);

BACNET_WS_DECLARE_SERVICE(info_protocol_version, ".info/protocol-version",
    BACNET_WS_SERVICE_METHOD_GET, false, info_protocol_version_handler);

BACNET_WS_DECLARE_SERVICE(info_protocol_revision, ".info/protocol-revision",
    BACNET_WS_SERVICE_METHOD_GET, false, info_protocol_revision_handler);

BACNET_WS_DECLARE_SERVICE(info_max_uri, ".info/max-uri",
    BACNET_WS_SERVICE_METHOD_GET, false, info_max_uri_handler);

int ws_service_info_registry(void)
{
    int ret;

    if ((ret = ws_service_registry(info_vendor_id)) != 0)
        return ret;
    if ((ret = ws_service_registry(info_vendor_name)) != 0)
        return ret;
    if ((ret = ws_service_registry(info_model_name)) != 0)
        return ret;
    if ((ret = ws_service_registry(info_software_version)) != 0)
        return ret;
    if ((ret = ws_service_registry(info_protocol_version)) != 0)
        return ret;
    if ((ret = ws_service_registry(info_protocol_revision)) != 0)
        return ret;
    if ((ret = ws_service_registry(info_max_uri)) != 0)
        return ret;

    return ret;
}
