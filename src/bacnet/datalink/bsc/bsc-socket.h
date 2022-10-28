/**
 * @file
 * @brief BACNet secure connect API.
 * @author Kirill Neznamov
 * @date July 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BSC__SECURE__CONNECTION__INCLUDED__
#define __BSC__SECURE__CONNECTION__INCLUDED__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bsc-conf.h"
#include "bacnet/basic/sys/mstimer.h"

#ifndef BSC_CONF_RX_BUFFER_SIZE
#define BSC_RX_BUFFER_SIZE 4096
#else
#define BSC_RX_BUFFER_SIZE BSC_CONF_RX_BUFFER_SIZE
#endif

#ifndef BSC_CONF_TX_BUFFER_SIZE
#define BSC_TX_BUFFER_SIZE 4096
#else
#define BSC_TX_BUFFER_SIZE BSC_CONF_TX_BUFFER_SIZE
#endif

struct BSC_Socket;
typedef struct BSC_Socket BSC_SOCKET;

struct BSC_SocketContext;
typedef struct BSC_SocketContext BSC_SOCKET_CTX;

struct BSC_SocketContextFuncs;
typedef struct BSC_SocketContextFuncs BSC_SOCKET_CTX_FUNCS;

struct BSC_ContextCFG;
typedef struct BSC_ContextCFG BSC_CONTEXT_CFG;

typedef enum {
   BSC_SOCKET_CTX_INITIATOR = 1,
   BSC_SOCKET_CTX_ACCEPTOR = 2
}  BSC_SOCKET_CTX_TYPE;

typedef enum {
    BSC_SOCKET_EVENT_CONNECTED = 0,
    BSC_SOCKET_EVENT_DISCONNECTED = 1,
    BSC_SOCKET_EVENT_RECEIVED = 2
} BSC_SOCKET_EVENT;

typedef enum {
    BSC_CTX_INITIALIZED = 0,
    BSC_CTX_DEINITIALIZED = 1
} BSC_CTX_EVENT;

typedef enum {
  BSC_CTX_STATE_IDLE = 0,
  BSC_CTX_STATE_INITIALIZING = 1,
  BSC_CTX_STATE_INITIALIZED = 2,
  BSC_CTX_STATE_DEINITIALIZING = 3
} BSC_CTX_STATE;

typedef enum {
    BSC_SOCK_STATE_IDLE = 0,
    BSC_SOCK_STATE_AWAITING_WEBSOCKET = 1,
    BSC_SOCK_STATE_AWAITING_REQUEST = 2,
    BSC_SOCK_STATE_AWAITING_ACCEPT = 3,
    BSC_SOCK_STATE_CONNECTED = 4,
    BSC_SOCK_STATE_DISCONNECTING = 5,
    BSC_SOCK_STATE_ERROR = 6,
    BSC_SOCK_STATE_ERROR_FLUSH_TX = 7
} BSC_SOCKET_STATE;

struct BSC_Socket {
    BSC_SOCKET_CTX *ctx;
    BSC_WEBSOCKET_HANDLE wh;
    BSC_SOCKET_STATE state;
    BSC_SC_RET disconnect_reason;
    struct mstimer t;
    struct mstimer heartbeat;
    BACNET_SC_VMAC_ADDRESS vmac; // VMAC address of the requesting node.
    BACNET_SC_UUID uuid;
    uint16_t message_id;
    
    // Regarding max_bvlc_len and max_npdu_len:
    // These are the datalink limits and are passed up the stack to let
    // the application layer know one of the several numbers that go into computing
    // how big an NPDU/APDU can be.

    uint16_t max_bvlc_len; // remote peer max bvlc len
    uint16_t max_npdu_len; // remote peer max npdu len

    uint16_t expected_connect_accept_message_id;
    uint16_t expected_disconnect_message_id;
    uint16_t expected_heartbeat_message_id;

    BVLC_SC_DECODED_MESSAGE dm;
    // 2 bytes is packet len
    uint8_t rx_buf[BSC_RX_BUFFER_SIZE + 2]; 
    size_t rx_buf_size;
    // 2 bytes is packet len
    uint8_t tx_buf[BSC_TX_BUFFER_SIZE + 2]; 
    size_t tx_buf_size;
};

struct BSC_ContextCFG
{
    BSC_SOCKET_CTX_TYPE type;
    BSC_WEBSOCKET_PROTOCOL proto;
    uint16_t port;
    uint8_t *ca_cert_chain;
    size_t ca_cert_chain_size;
    uint8_t *cert_chain;
    size_t cert_chain_size;
    uint8_t *priv_key;
    size_t priv_key_size;
    BACNET_SC_VMAC_ADDRESS local_vmac;
    BACNET_SC_UUID local_uuid;
    uint16_t max_bvlc_len; // local peer max bvlc len
    uint16_t max_ndpu_len; // local peer max npdu len

    // According AB.6.2 BACnet/SC Connection Establishment and Termination
    // recommended default value for establishing of connection 10 seconds
    unsigned long connect_timeout_s;
    unsigned long disconnect_timeout_s;

    // According 12.56.Y10 SC_Heartbeat_Timeout
    // (http://www.bacnet.org/Addenda/Add-135-2020cc.pdf) the recommended default
    // value is 300 seconds.
    unsigned long heartbeat_timeout_s;
};

struct BSC_SocketContextFuncs {

    BSC_SOCKET* (*find_connection_for_vmac)(BACNET_SC_VMAC_ADDRESS *vmac,
                                            void* user_arg);
    BSC_SOCKET* (*find_connection_for_uuid)(BACNET_SC_UUID *uuid,
                                            void* user_arg);
    // We always reserve BSC_PRE bytes before BVLC message header
    // to avoid copying of packet payload during manipulation with
    // origin and dest addresses (add them to received PDU)
    // That's why pdu pointer has always reserved BSC_PRE bytes behind
    void (*socket_event)(BSC_SOCKET*c, BSC_SOCKET_EVENT ev,
                         BSC_SC_RET err, uint8_t *pdu, uint16_t pdu_len,
                         BVLC_SC_DECODED_MESSAGE *decoded_pdu);
    void (*context_event)(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);
};


struct BSC_SocketContext {
    BSC_CTX_STATE state;
    BSC_WEBSOCKET_SRV_HANDLE sh;
    BSC_SOCKET *sock;
    size_t  sock_num;
    BSC_SOCKET_CTX_FUNCS* funcs;
    BSC_CONTEXT_CFG *cfg;
    bool deinit_in_progress;
    void* user_arg;
};

// max_local_bvlc_len - The maximum BVLC message size int bytes that can be
//                      received and processed by BSC/SC datalink.
// max_local_ndpu_len - The maximum NPDU message size in bytes hat can be
//                      handled by BSC/SC datalink.

BACNET_STACK_EXPORT
void bsc_init_ctx_cfg(BSC_SOCKET_CTX_TYPE type,
                      BSC_CONTEXT_CFG* cfg,
                      BSC_WEBSOCKET_PROTOCOL proto,
                      uint16_t port,
                      uint8_t *ca_cert_chain,
                      size_t ca_cert_chain_size,
                      uint8_t *cert_chain,
                      size_t cert_chain_size,
                      uint8_t *key,
                      size_t key_size,
                      BACNET_SC_UUID *local_uuid,
                      BACNET_SC_VMAC_ADDRESS *local_vmac,
                      uint16_t max_local_bvlc_len,
                      uint16_t max_local_ndpu_len,
                      unsigned int connect_timeout_s,
                      unsigned int heartbeat_timeout_s,
                      unsigned int disconnect_timeout_s);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_init_сtx(BSC_SOCKET_CTX *ctx,
                        BSC_CONTEXT_CFG* cfg,
                        BSC_SOCKET_CTX_FUNCS* funcs,
                        BSC_SOCKET* sockets,
                        size_t sockets_num,
                        void* user_arg);

BACNET_STACK_EXPORT
void bsc_deinit_ctx(BSC_SOCKET_CTX *ctx);

/**
 * @brief  bsc_connect() function starts connection operation for a 
 *         specified BACNet socket. The function call be called only
 *         for initiator context otherwise BSC_SC_INVALID_OPERATION
 *         error is returned.
 *
 * @param ctx - socket context.
 * @param c - BACNet socket descriptor .
 * @param url - url to connect to. For example: wss://legrand.com:8080.
 *
 * @return error code from BSC_SC_RET enum.
 *  The following error codes can be returned:
 *    BSC_SC_BAD_PARAM - In a case if some input parameter is
 *                          incorrect.
 *    BSC_SC_INVALID_OPERATION - if socket is not in opened state,
             or disconnect operation is in progress using
             bsc_disconnect() or bsc_deinit_ctx().
 *    BSC_SC_SUCCESS - operation has succeded.
 *    BSC_SC_NO_RESOURCES - there are not resources (memory, etc.. )
 *                          to send data
 */

BACNET_STACK_EXPORT
BSC_SC_RET bsc_connect(
    BSC_SOCKET_CTX *ctx,
    BSC_SOCKET *c,
    char *url);

BACNET_STACK_EXPORT
void bsc_disconnect(BSC_SOCKET *c);

/**
 * @brief  bsc_send() function schedules transmitting of pdu to
 *         another BACNet socket. The function may be used only
 *         when the socket is in a connected state
 *         otherwise BSC_SC_INVALID_OPERATION error is returned.
 *
 * @param c - BACNet socket descriptor initialized by bsc_accept() or
 *            bsc_connect() calls.
 * @param pdu - pointer to a data to send.
 * @param pdu_len - size in bytes of data to send.
 *
 * @return error code from BSC_SC_RET enum.
 *  The following error codes can be returned:
 *    BSC_SC_BAD_PARAM - In a case if some input parameter is
 *                          incorrect.
 *    BSC_SC_INVALID_OPERATION - if socket is not in opened state,
             or disconnect operation is in progress using
             bsc_disconnect() or bsc_deinit_ctx().
 *    BSC_SC_SUCCESS - operation has succeded.
 *    BSC_SC_NO_RESOURCES - there are not resources (memory, etc.. )
 *                          to send data
 */

BACNET_STACK_EXPORT
BSC_SC_RET bsc_send(BSC_SOCKET *c, uint8_t *pdu, uint16_t pdu_len);

BACNET_STACK_EXPORT
void bsc_get_remote_bvlc(BSC_SOCKET *c, uint16_t *p_val);

BACNET_STACK_EXPORT
void bsc_get_remote_npdu(BSC_SOCKET *c, uint16_t *p_val);

#endif