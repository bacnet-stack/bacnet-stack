/**
 * @file
 * @brief Client/Server generic websocket interfaces API.
 * @author Kirill Neznamov
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BACNET__WEBSOCKET__INCLUDED__
#define __BACNET__WEBSOCKET__INCLUDED__

#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"

#define BACNET_WEBSOCKET_DEBUG_ENABLED 1
#define BACNET_CLIENT_WEBSOCKETS_MAX_NUM 4
/* constant must be a power of 2 */
#define BACNET_CLIENT_WEBSOCKET_RX_BUFFER_SIZE 4096

#define BACNET_SERVER_WEBSOCKETS_MAX_NUM 4
#define BACNET_SERVER_WEBSOCKET_RX_BUFFER_SIZE 4096
#define BACNET_WSURL_MAX_LEN 256

typedef int BACNET_WEBSOCKET_HANDLE;
#define BACNET_WEBSOCKET_INVALID_HANDLE (-1)

// Websockets protocol defined in BACnet/SC \S AB.7.1.
#define BACNET_WEBSOCKET_HUB_PROTOCOL "hub.bsc.bacnet.org"
#define BACNET_WEBSOCKET_DIRECT_CONNECTION_PROTOCOL "dc.bsc.bacnet.org"

typedef enum {
    BACNET_WEBSOCKET_HUB_CONNECTION = 0,
    BACNET_WEBSOCKET_DIRECT_CONNECTION = 1
} BACNET_WEBSOCKET_CONNECTION_TYPE;

typedef enum {
    BACNET_WEBSOCKET_SUCCESS = 0,
    BACNET_WEBSOCKET_WEBSOCKET_CLOSED = 1,
    BACNET_WEBSOCKET_NO_RESOURCES = 2,
    BACNET_WEBSOCKET_BUFFER_TOO_SMALL = 3,
    BACNET_WEBSOCKET_BUFFER_TOO_BIG = 4,
    BACNET_WEBSOCKET_OPERATION_IN_PROGRESS = 5,
    BACNET_WEBSOCKET_BAD_PARAM = 6,
    BACNET_WEBSOCKET_TIMEDOUT = 7,
    BACNET_WEBSOCKET_INVALID_OPERATION = 8
} BACNET_WEBSOCKET_RET;

typedef struct BACNetWebsocketClient {
    /**
     * @brief Blocking bws_сonnect() function opens a new connection with
     * specified websocket server. In a case if some other thread calls
     * bws_disconnect() or bws_deinit() when the function call is in progress,
     * one of following retcodes is returned: BACNET_WEBSOCKET_NOT_INITIALIZED
     *           BACNET_WEBSOCKET_OPERATION_IN_PROGRESS
     *           BACNET_WEBSOCKET_CLOSED.
     *        If user
     *        Other possible error codes are pretty general:
     *          BACNET_WEBSOCKET_SUCCESS if the function succeded.
     *          BACNET_WEBSOCKET_BAD_PARAM if a user passed some incorrect
     * parameters. BACNET_WEBSOCKET_NO_RESOURCES if client has already opened
     * more than BACNET_CLIENT_WEBSOCKETS_MAX_NUM sockets, or if some mem
     * allocations failed or some allocations of resources failed like mutex,
     * thread, or etc..
     *
     * @param type - type of BACNet/SC connection, check
     * BACNET_WEBSOCKET_CONNECTION_TYPE enum. According BACNet standard
     * different type of connections require different websocket protocols.
     * @param url - BACNet/SC server URL.
     * @param out_handle - pointer to opened websocket handle in a case of
     * successfull connect.
     * @return error code from BACNET_WEBSOCKET_RET enum
     */

    BACNET_WEBSOCKET_RET (*bws_connect)
    (BACNET_WEBSOCKET_CONNECTION_TYPE type,
        char *url,
        uint8_t *ca_cert,
        size_t ca_cert_size,
        uint8_t *cert,
        size_t cert_size,
        uint8_t *key,
        size_t key_size,
        BACNET_WEBSOCKET_HANDLE *out_handle);

    BACNET_WEBSOCKET_RET (*bws_disconnect)(BACNET_WEBSOCKET_HANDLE h);

    BACNET_WEBSOCKET_RET (*bws_send)
    (BACNET_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size);

    BACNET_WEBSOCKET_RET (*bws_recv)
    (BACNET_WEBSOCKET_HANDLE h,
        uint8_t *buf,
        size_t bufsize,
        size_t *bytes_received,
        int timeout);
} BACNET_WEBSOCKET_CLIENT;

typedef struct BACNetWebsocketServer {
  BACNET_WEBSOCKET_RET (*bws_start)(
                                    int port,
                                    uint8_t *ca_cert,
                                    size_t ca_cert_size,
                                    uint8_t *cert,
                                    size_t cert_size,
                                    uint8_t *key,
                                    size_t key_size);
  // TODO: write note that user must check if out_handle is used.
  BACNET_WEBSOCKET_RET (*bws_accept)(BACNET_WEBSOCKET_HANDLE* out_handle);
  BACNET_WEBSOCKET_RET (*bws_disconnect)(BACNET_WEBSOCKET_HANDLE h);
  BACNET_WEBSOCKET_RET (*bws_send)(BACNET_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size);

  BACNET_WEBSOCKET_RET (*bws_recv)
  (BACNET_WEBSOCKET_HANDLE h,
      uint8_t *buf,
      size_t bufsize,
      size_t *bytes_received,
      int timeout);

  BACNET_WEBSOCKET_RET (*bws_stop)(void);
} BACNET_WEBSOCKET_SERVER;

BACNET_STACK_EXPORT
BACNET_WEBSOCKET_CLIENT *bws_cli_get(void);

BACNET_STACK_EXPORT
BACNET_WEBSOCKET_SERVER *bws_srv_get(void);

#endif