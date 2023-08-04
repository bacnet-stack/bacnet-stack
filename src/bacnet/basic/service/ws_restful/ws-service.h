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

#ifndef __WS__SRV__INCLUDED__
#define __WS__SRV__INCLUDED__

#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"

#define BACNET_WS_SERVICE_MAX_OUT_BUFFER_LEN 1024

typedef enum
{
  BACNET_WS_SERVICE_METHOD_GET = 1,
  BACNET_WS_SERVICE_METHOD_POST = 2,
  BACNET_WS_SERVICE_METHOD_PUT = 4,
  BACNET_WS_SERVICE_METHOD_DELETE = 8,
} BACNET_WS_SERVICE_METHOD;

typedef enum 
{
  BACNET_WS_SERVICE_SUCCESS = 0,
  BACNET_WS_SERVICE_FAIL = 1,
  BACNET_WS_SERVICE_NO_RESOURCES = 2,
  BACNET_WS_SERVICE_BAD_PARAM = 3,
  BACNET_WS_SERVICE_INVALID_OPERATION = 4,
  BACNET_WS_SERVICE_HAS_DATA = 5
} BACNET_WS_SERVICE_RET;

typedef enum
{
  BACNET_WS_ALT_XML = 0,
  BACNET_WS_ALT_JSON = 1,
  BACNET_WS_ALT_PLAIN = 2,
  BACNET_WS_ALT_MEDIA = 3,
  BACNET_WS_ALT_ERROR = 0xff,
} BACNET_WS_ALT;


struct bacnet_ws_connect_ctx;

typedef BACNET_WS_SERVICE_RET (*BACNET_WS_SERVICE_CALLBACK)(
                                              struct bacnet_ws_connect_ctx *ctx,
                                              uint8_t* in, size_t in_len,
                                              uint8_t* out, size_t *out_len);

typedef BACNET_WS_SERVICE_RET (*BACNET_WS_SERVICE_BODY)(uint8_t* in, size_t in_len);

typedef struct
{
  uint32_t hash; /* TODO: think about collisions! */
  const char *uri; /* null terminated string without leading and trailing '/' */
  unsigned int ws_method_mask;
  bool https_only;
  BACNET_WS_SERVICE_CALLBACK func;
  BACNET_WS_SERVICE_BODY set_body;
  void *next;
} BACNET_WS_SERVICE;

struct bacnet_ws_connect_ctx
{
    BACNET_WS_SERVICE *s;
    BACNET_WS_SERVICE_METHOD m;
    BACNET_WS_ALT alt;
    void *context;
};

typedef struct bacnet_ws_connect_ctx BACNET_WS_CONNECT_CTX;

#define BACNET_WS_DECLARE_SERVICE(service, uri, methods, https_only, callback, set_body) \
   static BACNET_WS_SERVICE service##_s = { 0, uri, (unsigned int) methods, https_only, callback, set_body, NULL}; \
   static BACNET_WS_SERVICE *service = &service##_s

typedef void* BACNET_WS_SERVER;

BACNET_WS_SERVICE_RET ws_service_registry(BACNET_WS_SERVICE* s);

BACNET_WS_SERVICE_RET ws_server_start(uint16_t http_port,
                                      uint16_t https_port,
                                      char*    http_iface,
                                      char*    https_iface,
                                      uint8_t* ca_cert,
                                      size_t   ca_cert_size,
                                      uint8_t* cert,
                                      size_t   cert_size,
                                      uint8_t* key,
                                      size_t   key_size,
                                      size_t   timeout_s);

void ws_server_stop(void);

int ws_http_parameter_get(void *context, char *name, char *buffer, size_t len);

int ws_service_info_registry(void);


#endif