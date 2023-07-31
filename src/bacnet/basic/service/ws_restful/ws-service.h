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
  BACNET_WS_SERVICE_INVALID_OPERATION = 4
} BACNET_WS_SERVICE_RET;

typedef BACNET_WS_SERVICE_RET (*BACNET_WS_SERVICE_CALLBACK)(BACNET_WS_SERVICE_METHOD m,
                                                     uint8_t* in, size_t in_len,
                                                     uint8_t* out, size_t *out_len);

typedef struct
{
  uint32_t hash; /* TODO: think about collisions! */
  const char *uri; /* null terminated string without leading and trailing '/' */
  unsigned int ws_method_mask;
  BACNET_WS_SERVICE_CALLBACK func;
  void *next;
} BACNET_WS_SERVICE;

#define BACNET_WS_DECLARE_SERVICE(service, uri, methods, callback) \
   static BACNET_WS_SERVICE service##_s = { 0, uri, (unsigned int) methods, callback, NULL}; \
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

#endif