/**
 * @file
 * @brief BACnet secure connect socket API.
 *        In general, user should not use that API directly,
 *        BACnet/SC datalink API should be used.
 * @author Kirill Neznamov <kirill\.neznamov@dsr-corporation\.com>
 * @date December 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DATALINK_BSC_SOCKET_H
#define BACNET_DATALINK_BSC_SOCKET_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bsc-conf.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/object/sc_netport.h"

#define BSC_RX_BUFFER_SIZE BSC_CONF_SOCK_RX_BUFFER_SIZE
#define BSC_TX_BUFFER_SIZE BSC_CONF_SOCK_TX_BUFFER_SIZE

#define BSC_SOCKET_CTX_NUM                                           \
    (BSC_CONF_NODES_NUM *                                            \
     (BSC_CONF_HUB_CONNECTORS_NUM + 2 * BSC_CONF_NODE_SWITCHES_NUM + \
      BSC_CONF_HUB_FUNCTIONS_NUM))

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
} BSC_SOCKET_CTX_TYPE;

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
    BACNET_ERROR_CODE reason;
    struct mstimer t;
    struct mstimer heartbeat;
    BACNET_SC_VMAC_ADDRESS vmac; /* VMAC address of the requesting node. */
    BACNET_SC_UUID uuid;

    /* Regarding max_bvlc_len and max_npdu_len: */
    /* These are the datalink limits and are passed up the stack to let */
    /* the application layer know one of the several numbers that go into
     * computing */
    /* how big an NPDU/APDU can be. */

    uint16_t max_bvlc_len; /* remote peer max bvlc len */
    uint16_t max_npdu_len; /* remote peer max npdu len */

    uint16_t expected_connect_accept_message_id;
    uint16_t expected_disconnect_message_id;
    uint16_t expected_heartbeat_message_id;

    uint8_t tx_buf[BSC_TX_BUFFER_SIZE];
    size_t tx_buf_size;
};

struct BSC_ContextCFG {
    BSC_SOCKET_CTX_TYPE type;
    BSC_WEBSOCKET_PROTOCOL proto;
    uint16_t port;
    char *iface;
    uint8_t *ca_cert_chain;
    size_t ca_cert_chain_size;
    uint8_t *cert_chain;
    size_t cert_chain_size;
    uint8_t *priv_key;
    size_t priv_key_size;
    BACNET_SC_VMAC_ADDRESS local_vmac;
    BACNET_SC_UUID local_uuid;
    uint16_t max_bvlc_len; /* local peer max bvlc len */
    uint16_t max_ndpu_len; /* local peer max npdu len */

    /* According AB.6.2 BACnet/SC Connection Establishment and Termination */
    /* recommended default value for establishing of connection 10 seconds */
    uint16_t connect_timeout_s;
    uint16_t disconnect_timeout_s;

    /* According 12.56.Y10 SC_Heartbeat_Timeout */
    /* (http://www.bacnet.org/Addenda/Add-135-2020cc.pdf) the recommended
     * default */
    /* value is 300 seconds. */
    uint16_t heartbeat_timeout_s;
};

struct BSC_SocketContextFuncs {
    BSC_SOCKET *(*find_connection_for_vmac)(
        BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg);
    BSC_SOCKET *(*find_connection_for_uuid)(
        BACNET_SC_UUID *uuid, void *user_arg);
    /* We always reserve BSC_PRE bytes before BVLC message header */
    /* to avoid copying of packet payload during manipulation with */
    /* origin and dest addresses (e.g. adding them to received PDU) */
    /* That's why pdu pointer has always reserved BSC_PRE bytes behind */
    /* The params disconnect_reason and disconnect_reason_desc are meanfull */
    /* only for disconnect events, e.g. when ev == BSC_SOCKET_EVENT_DISCONNECTED
     */

    void (*socket_event)(
        BSC_SOCKET *c,
        BSC_SOCKET_EVENT ev,
        BACNET_ERROR_CODE disconnect_reason,
        const char *disconnect_reason_desc,
        uint8_t *pdu,
        size_t pdu_len,
        BVLC_SC_DECODED_MESSAGE *decoded_pdu);

    void (*context_event)(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);
    void (*failed_request)(
        BSC_SOCKET_CTX *ctx,
        BSC_SOCKET *c,
        BACNET_SC_VMAC_ADDRESS *vmac,
        BACNET_SC_UUID *uuid,
        BACNET_ERROR_CODE error,
        const char *error_desc);
};

struct BSC_SocketContext {
    BSC_CTX_STATE state;
    BSC_WEBSOCKET_SRV_HANDLE sh;
    BSC_SOCKET *sock;
    size_t sock_num;
    BSC_SOCKET_CTX_FUNCS *funcs;
    BSC_CONTEXT_CFG *cfg;
    bool deinit_in_progress;
    void *user_arg;
};

/* max_local_bvlc_len - The maximum BVLC message size int bytes that can be */
/*                      received and processed by BSC/SC datalink. */
/* max_local_ndpu_len - The maximum NPDU message size in bytes hat can be */
/*                      handled by BSC/SC datalink. */

BACNET_STACK_EXPORT
void bsc_init_ctx_cfg(
    BSC_SOCKET_CTX_TYPE type,
    BSC_CONTEXT_CFG *cfg,
    BSC_WEBSOCKET_PROTOCOL proto,
    uint16_t port,
    char *iface,
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
BSC_SC_RET bsc_init_ctx(
    BSC_SOCKET_CTX *ctx,
    BSC_CONTEXT_CFG *cfg,
    BSC_SOCKET_CTX_FUNCS *funcs,
    BSC_SOCKET *sockets,
    size_t sockets_num,
    void *user_arg);

BACNET_STACK_EXPORT
void bsc_deinit_ctx(BSC_SOCKET_CTX *ctx);

/**
 * @brief  bsc_connect() function starts connect operation for a
 *         specified BACnet socket. The function call be called only
 *         for initiator context otherwise BSC_SC_INVALID_OPERATION
 *         error is returned. As a result if bsc_connect() was
 *         succeeded for given param c, that leads to emitting of
 *         BSC_SOCKET_EVENT_CONNECTED or BSC_SOCKET_EVENT_DISCONNECTED
 *         events depending on the result of connect operation.
 *         If connect operation is failed, BSC_SOCKET_EVENT_DISCONNECTED
 *         event is emitted and user can determine the reason why it
 *         happened using disconnect_reason and disconnect_reason_desc
 *         parameters in callback function.
 *         If connect operation succeeded, BSC_SOCKET_EVENT_CONNECTED
 *         event is emitted.
 *
 * @param ctx - socket context.
 * @param c - BACnet socket descriptor .
 * @param url - url to connect to. For example: wss://example.com:8080.
 *
 * @return error code from BSC_SC_RET enum.
 *  The following error codes can be returned:
 *    BSC_SC_BAD_PARAM - In a case if some input parameter is
 *                          incorrect.
 *    BSC_SC_INVALID_OPERATION - if socket is not in opened state,
             or disconnect operation is in progress using
             bsc_disconnect() or bsc_deinit_ctx().
 *    BSC_SC_SUCCESS - operation has succeeded.
 *    BSC_SC_NO_RESOURCES - there are not resources (memory, etc.. )
 *                          to send data
 */

BACNET_STACK_EXPORT
BSC_SC_RET bsc_connect(BSC_SOCKET_CTX *ctx, BSC_SOCKET *c, char *url);

BACNET_STACK_EXPORT
void bsc_disconnect(BSC_SOCKET *c);

/**
 * @brief  bsc_send() function schedules transmitting of pdu to
 *         another BACnet socket. The function may be used only
 *         when the socket is in a connected state
 *         otherwise BSC_SC_INVALID_OPERATION error is returned.
 *
 * @param c - BACnet socket descriptor initialized by bsc_accept() or
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
 *    BSC_SC_SUCCESS - operation has succeeded.
 *    BSC_SC_NO_RESOURCES - there are not resources (memory, etc.. )
 *                          to send data
 */

BACNET_STACK_EXPORT
BSC_SC_RET bsc_send(BSC_SOCKET *c, uint8_t *pdu, size_t pdu_len);

BACNET_STACK_EXPORT
uint16_t bsc_get_next_message_id(void);

BACNET_STACK_EXPORT
void bsc_socket_maintenance_timer(uint16_t seconds);

/**
 * @brief  bsc_socket_get_peer_addr() function gets information
 *         about remote peer address only for socket with acceptor cotext.
 *
 * @param c - BACnet socket descriptor initialized by bsc_accept() call.
 * @param data - pointer to a struct holding address information.
 * @param pdu_len - size in bytes of data to send.
 *
 * @return false if socket is not acceptor or bad parameter was passed or
 *               if getting of address information failed.
 *         true  if function succeeds.
 */

BACNET_STACK_EXPORT
bool bsc_socket_get_peer_addr(BSC_SOCKET *c, BACNET_HOST_N_PORT_DATA *data);

BACNET_STACK_EXPORT
uint8_t *bsc_socket_get_global_buf(void);

BACNET_STACK_EXPORT
size_t bsc_socket_get_global_buf_size(void);
#endif
