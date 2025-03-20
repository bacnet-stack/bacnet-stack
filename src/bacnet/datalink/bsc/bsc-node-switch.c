/**
 * @file
 * @brief BACNet secure connect node switch function API.
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date October 2022
 * @copyright SPDX-License-Identifier: GPL-2\.0-or-later WITH GCC-exception-2.0
 */
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-node-switch.h"
#include "bacnet/datalink/bsc/bsc-hub-connector.h"
#include "bacnet/datalink/bsc/bsc-node.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"

#define DEBUG_BSC_NODE_SWITCH 0

#if DEBUG_BSC_NODE_SWITCH == 1
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF debug_printf_disabled
#endif

typedef enum {
    BSC_NODE_SWITCH_STATE_IDLE = 0,
    BSC_NODE_SWITCH_STATE_STARTING = 1,
    BSC_NODE_SWITCH_STATE_STARTED = 2,
    BSC_NODE_SWITCH_STATE_STOPPING = 3
} BSC_NODE_SWITCH_STATE;

typedef enum {
    BSC_NODE_SWITCH_CONNECTION_STATE_IDLE = 0,
    BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_CONNECTION = 1,
    BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_RESOLUTION = 2,
    BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED = 3,
    BSC_NODE_SWITCH_CONNECTION_STATE_DELAYING = 4,
    BSC_NODE_SWITCH_CONNECTION_STATE_LOCAL_DISCONNECT = 5
} BSC_NODE_SWITCH_CONNECTION_STATE;

static BSC_SOCKET *node_switch_acceptor_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg);

static BSC_SOCKET *node_switch_acceptor_find_connection_for_uuid(
    BACNET_SC_UUID *uuid, void *user_arg);

static void node_switch_acceptor_failed_request(
    BSC_SOCKET_CTX *ctx,
    BSC_SOCKET *c,
    BACNET_SC_VMAC_ADDRESS *vmac,
    BACNET_SC_UUID *uuid,
    BACNET_ERROR_CODE error,
    const char *error_desc);

static void node_switch_acceptor_socket_event(
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE reason,
    const char *reason_desc,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu);

static void
node_switch_acceptor_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);

static void node_switch_initiator_socket_event(
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE reason,
    const char *reason_desc,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu);

static void
node_switch_initiator_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);

typedef struct BSC_Node_Switch_Acceptor {
    BSC_SOCKET_CTX ctx;
    BSC_CONTEXT_CFG cfg;
    BSC_SOCKET sock[BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM];
    BSC_NODE_SWITCH_STATE state;
    BACNET_SC_DIRECT_CONNECTION_STATUS *status;
} BSC_NODE_SWITCH_ACCEPTOR;

typedef struct {
    uint8_t utf8_urls[BSC_CONF_NODE_MAX_URIS_NUM_IN_ADDRESS_RESOLUTION_ACK]
                     [BSC_CONF_NODE_MAX_URI_SIZE_IN_ADDRESS_RESOLUTION_ACK + 1];
    size_t urls_cnt;
    int url_elem;
} BSC_NODE_SWITCH_URLS;

typedef struct BSC_Node_Switch_Initiator {
    BSC_SOCKET_CTX ctx;
    BSC_CONTEXT_CFG cfg;
    BSC_SOCKET sock[BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM];
    BSC_NODE_SWITCH_CONNECTION_STATE
    sock_state[BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM];
    BACNET_SC_VMAC_ADDRESS dest_vmac[BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM];
    struct mstimer t[BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM];
    BSC_NODE_SWITCH_URLS urls[BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM];
    BSC_NODE_SWITCH_STATE state;
} BSC_NODE_SWITCH_INITIATOR;

typedef struct {
    bool used;
    BSC_NODE_SWITCH_ACCEPTOR acceptor;
    BSC_NODE_SWITCH_INITIATOR initiator;
    BSC_NODE_SWITCH_EVENT_FUNC event_func;
    unsigned int reconnect_timeout_s;
    unsigned int address_resolution_timeout_s;
    bool direct_connect_accept_enable;
    bool direct_connect_initiate_enable;
    void *user_arg;
} BSC_NODE_SWITCH_CTX;

#if BSC_CONF_NODE_SWITCHES_NUM > 0
static BSC_NODE_SWITCH_CTX bsc_node_switch[BSC_CONF_NODE_SWITCHES_NUM] = { 0 };
#else
static BSC_NODE_SWITCH_CTX *bsc_node_switch = NULL;
#endif

static BSC_SOCKET_CTX_FUNCS bsc_node_switch_acceptor_ctx_funcs = {
    node_switch_acceptor_find_connection_for_vmac,
    node_switch_acceptor_find_connection_for_uuid,
    node_switch_acceptor_socket_event, node_switch_acceptor_context_event,
    node_switch_acceptor_failed_request
};

static BSC_SOCKET_CTX_FUNCS bsc_node_switch_initiator_ctx_funcs = {
    NULL, NULL, node_switch_initiator_socket_event,
    node_switch_initiator_context_event, NULL
};

static int node_switch_acceptor_find_connection_index_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, BSC_NODE_SWITCH_CTX *ctx);

/**
 * @brief Allocate a node switch context
 * @return pointer to the allocated node switch context
 */
static BSC_NODE_SWITCH_CTX *node_switch_alloc(void)
{
    int i;

    for (i = 0; i < BSC_CONF_NODE_SWITCHES_NUM; i++) {
        if (!bsc_node_switch[i].used) {
            bsc_node_switch[i].used = true;
            memset(
                &bsc_node_switch[i].initiator, 0,
                sizeof(bsc_node_switch[i].initiator));
            memset(
                &bsc_node_switch[i].acceptor, 0,
                sizeof(bsc_node_switch[i].acceptor));
            return &bsc_node_switch[i];
        }
    }
    return NULL;
}

/**
 * @brief Free the node switch context
 */
static void node_switch_free(BSC_NODE_SWITCH_CTX *ctx)
{
    ctx->used = false;
}

/**
 * @brief Update the status of the node switch
 * @param ctx - pointer to the node switch context
 * @param initiator - true if the initiator, otherwise false
 * @param failed_to_connect - true if failed to connect, otherwise false
 * @param URI - pointer to the URI
 * @param c - pointer to the socket
 * @param ev - socket event
 * @param reason - error code
 * @param reason_desc - pointer to the reason description
 */
static void node_switch_update_status(
    BSC_NODE_SWITCH_CTX *ctx,
    bool initiator,
    bool failed_to_connect,
    const char *URI,
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE reason,
    const char *reason_desc)
{
    BACNET_SC_DIRECT_CONNECTION_STATUS *s;

    DEBUG_PRINTF(
        "node_switch_update_status() >>> ctx = %p, initiator = %d, "
        "failed_to_connect = %d, URI = %p, c = %p, ev = %d, reason = "
        "%d, reason_desc = %p\n",
        ctx, initiator, failed_to_connect, URI, c, ev, reason, reason_desc);

    if (ctx->user_arg) {
        s = bsc_node_find_direct_status_for_vmac(ctx->user_arg, &c->vmac);

        if (s) {
            DEBUG_PRINTF(
                "found existent status entry %p for vmac = %s\n", s,
                bsc_vmac_to_string(&c->vmac));
            if (!initiator) {
                s->URI[0] = 0;
            } else if (URI) {
                bsc_copy_str(s->URI, URI, sizeof(s->URI));
            }
            memcpy(s->Peer_VMAC, &c->vmac.address[0], BVLC_SC_VMAC_SIZE);
            memcpy(
                &s->Peer_UUID.uuid.uuid128[0], &c->uuid.uuid[0],
                BVLC_SC_UUID_SIZE);
            if (reason_desc) {
                bsc_copy_str(
                    s->Error_Details, reason_desc, sizeof(s->Error_Details));
            } else {
                s->Error_Details[0] = 0;
            }
            s->Error = reason;
            if (ev == BSC_SOCKET_EVENT_CONNECTED) {
                DEBUG_PRINTF("set status state to CONNECTED\n");
                if (!bsc_socket_get_peer_addr(c, &s->Peer_Address)) {
                    memset(&s->Peer_Address, 0, sizeof(s->Peer_Address));
                }
                s->State = BACNET_SC_CONNECTION_STATE_CONNECTED;
                bsc_set_timestamp(&s->Connect_Timestamp);
                memset(
                    &s->Disconnect_Timestamp, 0xFF,
                    sizeof(s->Disconnect_Timestamp));
            } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
                bsc_set_timestamp(&s->Disconnect_Timestamp);
                if (reason == ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER ||
                    reason == ERROR_CODE_SUCCESS) {
                    s->State = BACNET_SC_CONNECTION_STATE_NOT_CONNECTED;
                    DEBUG_PRINTF("set status state to NOT-CONNECTED\n");
                } else {
                    s->State =
                        BACNET_SC_CONNECTION_STATE_DISCONNECTED_WITH_ERRORS;
                    DEBUG_PRINTF(
                        "set status state to DISCONNECTED-WITH-ERRORS\n");
                }
                if (failed_to_connect) {
                    s->State = BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT;
                    DEBUG_PRINTF("set status state to FAILED-TO-CONNECT\n");
                }
            }
        }
    }

    DEBUG_PRINTF("node_switch_update_status() <<<\n");
}

/**
 * @brief Copy URLs from the address resolution to the node switch context
 * @param ctx - pointer to the node switch context
 * @param index - index of the URL
 * @param r - pointer to the address resolution
 */
static void
copy_urls(BSC_NODE_SWITCH_CTX *ctx, int index, BSC_ADDRESS_RESOLUTION *r)
{
    int i;
    for (i = 0; i < r->urls_num; i++) {
        ctx->initiator.urls[index].utf8_urls[i][0] = 0;
        strncpy(
            (char *)&ctx->initiator.urls[index].utf8_urls[i][0],
            (char *)&r->utf8_urls[i][0],
            sizeof(ctx->initiator.urls[index].utf8_urls[i]) - 1);
    }
    ctx->initiator.urls[index].urls_cnt = r->urls_num;
}

/**
 * @brief Copy URLs from the array to the node switch context
 * @param ctx - pointer to the node switch context
 * @param index - index of the URL
 * @param urls - pointer to the array of URLs
 * @param urls_cnt - number of URLs
 */
static void
copy_urls2(BSC_NODE_SWITCH_CTX *ctx, int index, char **urls, size_t urls_cnt)
{
    size_t i;
    for (i = 0; i < urls_cnt; i++) {
        ctx->initiator.urls[index].utf8_urls[i][0] = 0;
        strncpy(
            (char *)&ctx->initiator.urls[index].utf8_urls[i][0], urls[i],
            sizeof(ctx->initiator.urls[index].utf8_urls[i]) - 1);
    }
    ctx->initiator.urls[index].urls_cnt = urls_cnt;
}

/**
 * @brief Find the connection socket for a specific VMAC address
 * @param vmac - pointer to the VMAC address
 * @param user_arg - pointer to the user argument
 * @return pointer to the socket, or NULL if not found
 */
static BSC_SOCKET *node_switch_acceptor_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg)
{
    int i;
    BSC_NODE_SWITCH_CTX *c;
    bws_dispatch_lock();
    c = (BSC_NODE_SWITCH_CTX *)user_arg;
    for (i = 0; i < sizeof(c->acceptor.sock) / sizeof(BSC_SOCKET); i++) {
        DEBUG_PRINTF(
            "ns = %p, sock %p, state = %d, vmac = %s\n", c,
            &c->acceptor.sock[i], c->acceptor.sock[i].state,
            bsc_vmac_to_string(&c->acceptor.sock[i].vmac));
        if (c->acceptor.sock[i].state != BSC_SOCK_STATE_IDLE &&
            !memcmp(
                &vmac->address[0], &c->acceptor.sock[i].vmac.address[0],
                sizeof(vmac->address))) {
            bws_dispatch_unlock();
            return &c->acceptor.sock[i];
        }
    }
    bws_dispatch_unlock();
    return NULL;
}

/**
 * @brief Find the connection socket for a specific UUID
 * @param uuid - pointer to the UUID
 * @param user_arg - pointer to the user argument
 * @return pointer to the socket, or NULL if not found
 */
static BSC_SOCKET *node_switch_acceptor_find_connection_for_uuid(
    BACNET_SC_UUID *uuid, void *user_arg)
{
    int i;
    BSC_NODE_SWITCH_CTX *c;
    bws_dispatch_lock();
    c = (BSC_NODE_SWITCH_CTX *)user_arg;
    for (i = 0; i < sizeof(c->acceptor.sock) / sizeof(BSC_SOCKET); i++) {
        DEBUG_PRINTF(
            "ns = %p, sock %p, state = %d, uuid = %s\n", c,
            &c->acceptor.sock[i], c->acceptor.sock[i].state,
            bsc_uuid_to_string(&c->acceptor.sock[i].uuid));
        if (c->acceptor.sock[i].state != BSC_SOCK_STATE_IDLE &&
            !memcmp(
                &uuid->uuid[0], &c->acceptor.sock[i].uuid.uuid[0],
                sizeof(uuid->uuid))) {
            DEBUG_PRINTF("found\n");
            bws_dispatch_unlock();
            return &c->acceptor.sock[i];
        }
    }
    bws_dispatch_unlock();
    return NULL;
}

/**
 * @brief Handle a node switch acceptor failed request
 * @param ctx - pointer to the socket context
 * @param c - pointer to the socket
 * @param vmac - pointer to the VMAC address
 * @param uuid - pointer to the UUID
 * @param error - error code
 * @param error_desc - pointer to the error description
 */
static void node_switch_acceptor_failed_request(
    BSC_SOCKET_CTX *ctx,
    BSC_SOCKET *c,
    BACNET_SC_VMAC_ADDRESS *vmac,
    BACNET_SC_UUID *uuid,
    BACNET_ERROR_CODE error,
    const char *error_desc)
{
    BSC_NODE_SWITCH_CTX *ns;
    BACNET_HOST_N_PORT_DATA peer;
    (void)ctx;

    bws_dispatch_lock();
    ns = (BSC_NODE_SWITCH_CTX *)c->ctx->user_arg;
    if (ns->user_arg) {
        if (bsc_socket_get_peer_addr(c, &peer)) {
            bsc_node_store_failed_request_info(
                (BSC_NODE *)ns->user_arg, &peer, vmac, uuid, error, error_desc);
        }
    }
    bws_dispatch_unlock();
}

/**
 * @brief Handle a node switch acceptor socket event
 * @param c - pointer to the socket
 * @param ev - event
 * @param disconnect_reason - disconnect reason
 * @param disconnect_reason_desc - disconnect reason description
 * @param pdu - pointer to the PDU
 * @param pdu_len - PDU length
 * @param decoded_pdu - decoded PDU
 */
static void node_switch_acceptor_socket_event(
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE disconnect_reason,
    const char *disconnect_reason_desc,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    uint8_t *p_pdu;
    BSC_NODE_SWITCH_CTX *ctx;
    uint16_t error_class;
    uint16_t error_code;
    const char *err_desc;
    bool res = true;

    DEBUG_PRINTF(
        "node_switch_acceptor_socket_event() >>> c %p, ev = %d\n", c, ev);

    bws_dispatch_lock();
    ctx = (BSC_NODE_SWITCH_CTX *)c->ctx->user_arg;

    /* Node switch does not take care about incoming connection status, */
    /* so just route PDUs to upper layer */

    if (ctx->acceptor.state == BSC_NODE_SWITCH_STATE_STARTED) {
        if (ev == BSC_SOCKET_EVENT_RECEIVED) {
            if (bsc_socket_get_global_buf_size() >= pdu_len) {
                p_pdu = bsc_socket_get_global_buf();
                memcpy(p_pdu, pdu, pdu_len);
                pdu_len = bvlc_sc_set_orig(&p_pdu, pdu_len, &c->vmac);
                res = bvlc_sc_decode_message(
                    p_pdu, pdu_len, decoded_pdu, &error_code, &error_class,
                    &err_desc);
                if (res) {
                    ctx->event_func(
                        BSC_NODE_SWITCH_EVENT_RECEIVED, ctx, ctx->user_arg,
                        NULL, p_pdu, pdu_len, decoded_pdu);
                }
            }
        } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
            node_switch_update_status(
                ctx, false, false, NULL, c, ev, disconnect_reason,
                disconnect_reason_desc);
            if (disconnect_reason == ERROR_CODE_NODE_DUPLICATE_VMAC) {
                ctx->event_func(
                    BSC_NODE_SWITCH_EVENT_DUPLICATED_VMAC, ctx, ctx->user_arg,
                    NULL, NULL, 0, NULL);
            }
        } else if (ev == BSC_SOCKET_EVENT_CONNECTED) {
            node_switch_update_status(
                ctx, false, false, NULL, c, ev, ERROR_CODE_DEFAULT, NULL);
        }
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("node_switch_acceptor_socket_event() <<<\n");
}

/**
 * @brief Deinialize the node switch context
 * @param ns - pointer to the node switch context
 */
static void node_switch_context_deinitialized(BSC_NODE_SWITCH_CTX *ns)
{
    if (ns->initiator.state == BSC_NODE_SWITCH_STATE_IDLE &&
        ns->acceptor.state == BSC_NODE_SWITCH_STATE_IDLE) {
        node_switch_free(ns);
        ns->event_func(
            BSC_NODE_SWITCH_EVENT_STOPPED, ns, ns->user_arg, NULL, NULL, 0,
            NULL);
    }
}

/**
 * @brief Handle a node switch acceptor context event
 * @param ctx - pointer to the socket context
 * @param ev - event
 */
static void
node_switch_acceptor_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev)
{
    BSC_NODE_SWITCH_CTX *ns;

    bws_dispatch_lock();
    DEBUG_PRINTF(
        "node_switch_acceptor_context_event() >>> ctx = %p, ev = %d, "
        "user_arg = %p\n",
        ctx, ev, ctx->user_arg);
    ns = (BSC_NODE_SWITCH_CTX *)ctx->user_arg;
    if (ev == BSC_CTX_INITIALIZED) {
        if (ns->acceptor.state == BSC_NODE_SWITCH_STATE_STARTING) {
            ns->acceptor.state = BSC_NODE_SWITCH_STATE_STARTED;
            ns->event_func(
                BSC_NODE_SWITCH_EVENT_STARTED, ns, ns->user_arg, NULL, NULL, 0,
                NULL);
        }
    } else if (ev == BSC_CTX_DEINITIALIZED) {
        ns->acceptor.state = BSC_NODE_SWITCH_STATE_IDLE;
        node_switch_context_deinitialized(ns);
    }
    DEBUG_PRINTF("node_switch_acceptor_context_event() <<<\n");
    bws_dispatch_unlock();
}

/**
 * @brief find a node switch initiator connection index for a specific
 *  VMAC address
 * @param vmac - pointer to the VMAC address
 * @param ctx - pointer to the node switch context
 * @return connection index, or -1 if not found
 */
static int node_switch_initiator_find_connection_index_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, BSC_NODE_SWITCH_CTX *ctx)
{
    int i;

    for (i = 0; i < sizeof(ctx->initiator.sock) / sizeof(BSC_SOCKET); i++) {
        if (ctx->initiator.sock_state[i] !=
                BSC_NODE_SWITCH_CONNECTION_STATE_IDLE &&
            !memcmp(
                &ctx->initiator.dest_vmac[i].address[0], &vmac->address[0],
                sizeof(vmac->address))) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief find a node switch acceptor connection index for a specific VMAC
 * address
 * @param vmac - pointer to the VMAC address
 * @param ctx - pointer to the node switch context
 * @return connection index, or -1 if not found
 */
static int node_switch_acceptor_find_connection_index_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, BSC_NODE_SWITCH_CTX *ctx)
{
    int i;

    for (i = 0; i < sizeof(ctx->acceptor.sock) / sizeof(BSC_SOCKET); i++) {
        if (ctx->acceptor.sock[i].state == BSC_SOCK_STATE_CONNECTED &&
            !memcmp(
                &ctx->acceptor.sock[i].vmac.address[0], &vmac->address[0],
                sizeof(vmac->address))) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief find a node switch initiator connection index
 * @param ctx - pointer to the node switch context
 * @param c - pointer to the socket
 * @return node switch initiator index, or -1 if not found
 */
static int
node_switch_initiator_get_index(BSC_NODE_SWITCH_CTX *ctx, BSC_SOCKET *c)
{
    return ((c - &ctx->initiator.sock[0]) >= 0)
        ? (int)(c - &ctx->initiator.sock[0])
        : -1;
}

/**
 * @brief allocate a socket for the node switch initiator
 * @param ctx - pointer to the node switch context
 * @return socket index, or -1 if not found
 */
static int node_switch_initiator_alloc_sock(BSC_NODE_SWITCH_CTX *ctx)
{
    int i;
    for (i = 0; i < sizeof(ctx->initiator.sock) / sizeof(BSC_SOCKET); i++) {
        if (ctx->initiator.sock_state[i] ==
            BSC_NODE_SWITCH_CONNECTION_STATE_IDLE) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Connect to the next URL
 * @param ctx - pointer to the node switch context
 * @param index - index of the URL
 */
static void connect_next_url(BSC_NODE_SWITCH_CTX *ctx, int index)
{
    BSC_SC_RET ret = BSC_SC_BAD_PARAM;

    while (ret != BSC_SC_SUCCESS) {
        if (ctx->initiator.urls[index].url_elem >=
            ctx->initiator.urls[index].urls_cnt) {
            ctx->initiator.sock_state[index] =
                BSC_NODE_SWITCH_CONNECTION_STATE_DELAYING;
            mstimer_set(
                &ctx->initiator.t[index], ctx->reconnect_timeout_s * 1000);
            ctx->initiator.urls[index].url_elem = 0;
            break;
        } else {
            ctx->initiator.sock_state[index] =
                BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_CONNECTION;
            ret = bsc_connect(
                &ctx->initiator.ctx, &ctx->initiator.sock[index],
                (char *)&ctx->initiator.urls[index]
                    .utf8_urls[ctx->initiator.urls[index].url_elem][0]);
            ctx->initiator.urls[index].url_elem++;
        }
    }
}

/**
 * @brief Connect to the next URL or delay
 * @param ns - pointer to the node switch context
 * @param dest - pointer to the VMAC address
 * @param sock_index - socket index
 */
static void node_switch_connect_or_delay(
    BSC_NODE_SWITCH_CTX *ns, BACNET_SC_VMAC_ADDRESS *dest, int sock_index)
{
    BSC_ADDRESS_RESOLUTION *r;

    if (ns->initiator.urls[sock_index].urls_cnt > 0) {
        connect_next_url(ns, sock_index);
    } else if (dest) {
        r = bsc_node_get_address_resolution(ns->user_arg, dest);
        if (r && r->urls_num) {
            copy_urls(ns, sock_index, r);
            ns->initiator.urls[sock_index].url_elem = 0;
            connect_next_url(ns, sock_index);
        } else {
            ns->initiator.sock_state[sock_index] =
                BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_RESOLUTION;
            ns->initiator.urls[sock_index].urls_cnt = 0;
            memcpy(
                &ns->initiator.dest_vmac[sock_index].address[0],
                &dest->address[0], BVLC_SC_VMAC_SIZE);
            mstimer_set(
                &ns->initiator.t[sock_index],
                ns->address_resolution_timeout_s * 1000);
            (void)bsc_node_send_address_resolution(
                ns->user_arg, &ns->initiator.dest_vmac[sock_index]);
        }
    }
}

/**
 * @brief Process a node switch initiator runloop
 * @param ns - pointer to the node switch context
 */
static void node_switch_initiator_runloop(BSC_NODE_SWITCH_CTX *ns)
{
    int i;

    /* DEBUG_PRINTF("node_switch_initiator_runloop() >>> ctx = %p\n", ns); */

    for (i = 0; i < sizeof(ns->initiator.sock) / sizeof(BSC_SOCKET); i++) {
        /* DEBUG_PRINTF("node_switch_initiator_runloop() socket %d state %d\n",
         */
        /* i, ns->initiator.sock_state[i]); */
        if (ns->initiator.sock_state[i] ==
            BSC_NODE_SWITCH_CONNECTION_STATE_DELAYING) {
            if (mstimer_expired(&ns->initiator.t[i])) {
                ns->initiator.urls[i].url_elem = 0;
                ns->initiator.sock_state[i] =
                    BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_CONNECTION;
                node_switch_connect_or_delay(
                    ns, &ns->initiator.dest_vmac[i], i);
            }
        } else if (
            ns->initiator.sock_state[i] ==
            BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_RESOLUTION) {
            if (mstimer_expired(&ns->initiator.t[i])) {
                ns->initiator.sock_state[i] =
                    BSC_NODE_SWITCH_CONNECTION_STATE_DELAYING;
                mstimer_set(
                    &ns->initiator.t[i], ns->reconnect_timeout_s * 1000);
            }
        }
    }
}

/**
 * @brief Run the node switch maintenance timer
 * @param seconds - number of seconds elapsed from the previous call
 */
void bsc_node_switch_maintenance_timer(uint16_t seconds)
{
    int i;
    (void)seconds;

    bws_dispatch_lock();
    for (i = 0; i < BSC_CONF_NODE_SWITCHES_NUM; i++) {
        if (bsc_node_switch[i].used) {
            node_switch_initiator_runloop(&bsc_node_switch[i]);
        }
    }
    bws_dispatch_unlock();
}

/**
 * @brief Handle a node switch initiator socket event
 * @param c - pointer to the socket
 * @param ev - event
 * @param disconnect_reason - disconnect reason
 * @param disconnect_reason_desc - disconnect reason description
 * @param pdu - pointer to the PDU
 * @param pdu_len - PDU length
 * @param decoded_pdu - decoded PDU
 */
static void node_switch_initiator_socket_event(
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE disconnect_reason,
    const char *disconnect_reason_desc,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    int index;
    uint8_t *p_pdu = NULL;
    BSC_NODE_SWITCH_CTX *ns;
    int elem;
    uint16_t error_class;
    uint16_t error_code;
    const char *err_desc;
    bool res;

    DEBUG_PRINTF(
        "node_switch_initiator_socket_event() >>> c %p, ev = %d\n", c, ev);

    bws_dispatch_lock();

    ns = (BSC_NODE_SWITCH_CTX *)c->ctx->user_arg;

    if (ns->initiator.state == BSC_NODE_SWITCH_STATE_STARTED) {
        if (ev == BSC_SOCKET_EVENT_DISCONNECTED &&
            disconnect_reason == ERROR_CODE_NODE_DUPLICATE_VMAC) {
            ns->event_func(
                BSC_NODE_SWITCH_EVENT_DUPLICATED_VMAC, ns, ns->user_arg, NULL,
                NULL, 0, NULL);
        } else if (ev == BSC_SOCKET_EVENT_RECEIVED) {
            if (bsc_socket_get_global_buf_size() >= pdu_len) {
                p_pdu = bsc_socket_get_global_buf();
                memcpy(p_pdu, pdu, pdu_len);
                pdu_len = bvlc_sc_set_orig(&p_pdu, pdu_len, &c->vmac);
                res = bvlc_sc_decode_message(
                    p_pdu, pdu_len, decoded_pdu, &error_code, &error_class,
                    &err_desc);
                if (res) {
                    ns->event_func(
                        BSC_NODE_SWITCH_EVENT_RECEIVED, ns, ns->user_arg, NULL,
                        p_pdu, pdu_len, decoded_pdu);
                }
            }
        }

        index = node_switch_initiator_get_index(ns, c);

        if (index > -1) {
            elem = ns->initiator.urls[index].url_elem - 1;
            if (elem < 0 || elem >= ns->initiator.urls[index].urls_cnt) {
                elem = -1;
            }
            if (ns->initiator.sock_state[index] ==
                BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_CONNECTION) {
                if (ev == BSC_SOCKET_EVENT_CONNECTED) {
                    ns->initiator.sock_state[index] =
                        BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED;
                    /* if user provides url instead of dest in */
                    /* bsc_node_switch_connect() ns->initiator.dest_vmac[index]
                     */
                    /* is unset. that's why we always set it from socket */
                    /* descriptor */
                    memcpy(
                        &ns->initiator.dest_vmac[index].address[0],
                        &c->vmac.address[0], sizeof(c->vmac.address));
                    node_switch_update_status(
                        ns, true, false,
                        elem == -1 ? NULL
                                   : (const char *)&ns->initiator.urls[index]
                                         .utf8_urls[elem][0],
                        c, ev, ERROR_CODE_DEFAULT, NULL);
                    ns->event_func(
                        BSC_NODE_SWITCH_EVENT_CONNECTED, ns, ns->user_arg,
                        &ns->initiator.dest_vmac[index], NULL, 0, NULL);
                } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
                    node_switch_update_status(
                        ns, true, true,
                        elem == -1 ? NULL
                                   : (const char *)&ns->initiator.urls[index]
                                         .utf8_urls[elem][0],
                        c, ev, disconnect_reason, disconnect_reason_desc);
                    connect_next_url(ns, index);
                }
            } else if (
                ns->initiator.sock_state[index] ==
                BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED) {
                if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
                    node_switch_update_status(
                        ns, true, false,
                        elem == -1 ? NULL
                                   : (const char *)&ns->initiator.urls[index]
                                         .utf8_urls[elem][0],
                        c, ev, disconnect_reason, disconnect_reason_desc);
                    ns->event_func(
                        BSC_NODE_SWITCH_EVENT_DISCONNECTED, ns, ns->user_arg,
                        &ns->initiator.dest_vmac[index], NULL, 0, NULL);
                    ns->initiator.urls[index].url_elem = 0;
                    connect_next_url(ns, index);
                }
            } else if (
                ns->initiator.sock_state[index] ==
                BSC_NODE_SWITCH_CONNECTION_STATE_LOCAL_DISCONNECT) {
                if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
                    node_switch_update_status(
                        ns, true, false,
                        elem == -1 ? NULL
                                   : (const char *)&ns->initiator.urls[index]
                                         .utf8_urls[elem][0],
                        c, ev, ERROR_CODE_SUCCESS, NULL);
                    ns->initiator.sock_state[index] =
                        BSC_NODE_SWITCH_CONNECTION_STATE_IDLE;
                    ns->event_func(
                        BSC_NODE_SWITCH_EVENT_DISCONNECTED, ns, ns->user_arg,
                        &ns->initiator.dest_vmac[index], NULL, 0, NULL);
                }
            }
        }
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("node_switch_initiator_socket_event() <<<\n");
}

/**
 * @brief Handle a node switch initiator context event
 * @param ctx - pointer to the socket context
 * @param ev - event
 */
static void
node_switch_initiator_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev)
{
    BSC_NODE_SWITCH_CTX *ns;
    int i;

    bws_dispatch_lock();
    DEBUG_PRINTF(
        "node_switch_initiator_context_event () >>> ctx = %p, ev = "
        "%d, user_arg = %p\n",
        ctx, ev, ctx->user_arg);
    ns = (BSC_NODE_SWITCH_CTX *)ctx->user_arg;
    if (ev == BSC_CTX_DEINITIALIZED) {
        for (i = 0; i < sizeof(ns->initiator.sock) / sizeof(BSC_SOCKET); i++) {
            if (ns->initiator.sock_state[i] ==
                BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED) {
                ns->initiator.sock_state[i] =
                    BSC_NODE_SWITCH_CONNECTION_STATE_IDLE;
                ns->event_func(
                    BSC_NODE_SWITCH_EVENT_DISCONNECTED, ns, ns->user_arg,
                    &ns->initiator.dest_vmac[i], NULL, 0, NULL);
            }
        }
        ns->initiator.state = BSC_NODE_SWITCH_STATE_IDLE;
        node_switch_context_deinitialized(ns);
    }
    DEBUG_PRINTF("node_switch_initiator_context_event() <<<\n");
    bws_dispatch_unlock();
}

/**
 * @brief Start the node switch
 * @param ca_cert_chain - pointer to the CA certificate chain
 * @param ca_cert_chain_size - size of the CA certificate chain
 * @param cert_chain - pointer to the certificate chain
 * @param cert_chain_size - size of the certificate chain
 * @param key - pointer to the key
 * @param key_size - size of the key
 * @param port - port number
 * @param iface - pointer to the interface
 * @param local_uuid - pointer to the local UUID
 * @param local_vmac - pointer to the local VMAC address
 * @param max_local_bvlc_len - maximum local BVLC length
 * @param max_local_npdu_len - maximum local NPDU length
 * @param connect_timeout_s - connection timeout in seconds
 * @param heartbeat_timeout_s - heartbeat timeout in seconds
 * @param disconnect_timeout_s - disconnect timeout in seconds
 * @param reconnnect_timeout_s - reconnect timeout in seconds
 * @param address_resolution_timeout_s - address resolution timeout in seconds
 * @param direct_connect_accept_enable - true if direct connect accept is
 * enabled
 * @param direct_connect_initiate_enable - true if direct connect initiate is
 * enabled
 * @param event_func - pointer to the event function
 * @param user_arg - pointer to the user argument
 * @param h - pointer to the node switch handle
 * @return BACnet/SC status
 */
BSC_SC_RET bsc_node_switch_start(
    uint8_t *ca_cert_chain,
    size_t ca_cert_chain_size,
    uint8_t *cert_chain,
    size_t cert_chain_size,
    uint8_t *key,
    size_t key_size,
    int port,
    char *iface,
    BACNET_SC_UUID *local_uuid,
    BACNET_SC_VMAC_ADDRESS *local_vmac,
    uint16_t max_local_bvlc_len,
    uint16_t max_local_npdu_len,
    uint16_t connect_timeout_s,
    uint16_t heartbeat_timeout_s,
    uint16_t disconnect_timeout_s,
    uint16_t reconnnect_timeout_s,
    uint16_t address_resolution_timeout_s,
    bool direct_connect_accept_enable,
    bool direct_connect_initiate_enable,
    BSC_NODE_SWITCH_EVENT_FUNC event_func,
    void *user_arg,
    BSC_NODE_SWITCH_HANDLE *h)
{
    BSC_SC_RET ret = BSC_SC_SUCCESS;
    BSC_NODE_SWITCH_CTX *ns;

    DEBUG_PRINTF("bsc_node_switch_start() >>>\n");

    if (!address_resolution_timeout_s || !event_func ||
        (direct_connect_accept_enable && !port) || !h) {
        DEBUG_PRINTF("bsc_node_switch_start() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    if (!direct_connect_accept_enable && !direct_connect_initiate_enable) {
        DEBUG_PRINTF("bsc_node_switch_start() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    *h = NULL;
    bws_dispatch_lock();
    ns = node_switch_alloc();

    if (!ns) {
        bws_dispatch_unlock();
        DEBUG_PRINTF("bsc_node_switch_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    ns->event_func = event_func;
    ns->user_arg = user_arg;
    ns->reconnect_timeout_s = reconnnect_timeout_s;
    ns->address_resolution_timeout_s = address_resolution_timeout_s;
    ns->direct_connect_initiate_enable = direct_connect_initiate_enable;
    ns->direct_connect_accept_enable = direct_connect_accept_enable;
    ns->initiator.state = BSC_NODE_SWITCH_STATE_IDLE;
    ns->acceptor.state = BSC_NODE_SWITCH_STATE_IDLE;

    if (direct_connect_initiate_enable) {
        bsc_init_ctx_cfg(
            BSC_SOCKET_CTX_INITIATOR, &ns->initiator.cfg,
            BSC_WEBSOCKET_DIRECT_PROTOCOL, 0, NULL, ca_cert_chain,
            ca_cert_chain_size, cert_chain, cert_chain_size, key, key_size,
            local_uuid, local_vmac, max_local_bvlc_len, max_local_npdu_len,
            connect_timeout_s, heartbeat_timeout_s, disconnect_timeout_s);
        ret = bsc_init_ctx(
            &ns->initiator.ctx, &ns->initiator.cfg,
            &bsc_node_switch_initiator_ctx_funcs, ns->initiator.sock,
            sizeof(ns->initiator.sock) / sizeof(BSC_SOCKET), ns);
        if (ret == BSC_SC_SUCCESS) {
            ns->initiator.state = BSC_NODE_SWITCH_STATE_STARTED;
        }
    }

    if (ret == BSC_SC_SUCCESS && direct_connect_accept_enable) {
        bsc_init_ctx_cfg(
            BSC_SOCKET_CTX_ACCEPTOR, &ns->acceptor.cfg,
            BSC_WEBSOCKET_DIRECT_PROTOCOL, port, iface, ca_cert_chain,
            ca_cert_chain_size, cert_chain, cert_chain_size, key, key_size,
            local_uuid, local_vmac, max_local_bvlc_len, max_local_npdu_len,
            connect_timeout_s, heartbeat_timeout_s, disconnect_timeout_s);
        ret = bsc_init_ctx(
            &ns->acceptor.ctx, &ns->acceptor.cfg,
            &bsc_node_switch_acceptor_ctx_funcs, ns->acceptor.sock,
            sizeof(ns->acceptor.sock) / sizeof(BSC_SOCKET), ns);
        if (ret == BSC_SC_SUCCESS) {
            ns->acceptor.state = BSC_NODE_SWITCH_STATE_STARTING;
        }
    }

    if (ret != BSC_SC_SUCCESS) {
        if (ns->initiator.state == BSC_NODE_SWITCH_STATE_STARTED) {
            bsc_deinit_ctx(&ns->initiator.ctx);
            ns->initiator.state = BSC_NODE_SWITCH_STATE_IDLE;
        }
        node_switch_free(ns);
    } else {
        *h = ns;
        if (direct_connect_initiate_enable && !direct_connect_accept_enable) {
            ns->event_func(
                BSC_NODE_SWITCH_EVENT_STARTED, ns, ns->user_arg, NULL, NULL, 0,
                NULL);
        }
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_switch_start() <<< ret = %d\n", ret);
    return ret;
}

/**
 * @brief Stop the node switch
 * @param h - pointer to the node switch handle
 */
void bsc_node_switch_stop(BSC_NODE_SWITCH_HANDLE h)
{
    BSC_NODE_SWITCH_CTX *ns;
    DEBUG_PRINTF("bsc_node_switch_stop() >>> h = %p \n", h);
    bws_dispatch_lock();
    ns = (BSC_NODE_SWITCH_CTX *)h;
    if (ns) {
        DEBUG_PRINTF("bsc_node_switch_stop() unreg loop\n");
        if (ns->direct_connect_accept_enable &&
            ns->acceptor.state != BSC_NODE_SWITCH_STATE_IDLE) {
            ns->acceptor.state = BSC_NODE_SWITCH_STATE_STOPPING;
            bsc_deinit_ctx(&ns->acceptor.ctx);
        }
        if (ns->direct_connect_initiate_enable &&
            ns->initiator.state != BSC_NODE_SWITCH_STATE_IDLE) {
            ns->initiator.state = BSC_NODE_SWITCH_STATE_STOPPING;
            bsc_deinit_ctx(&ns->initiator.ctx);
        }
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_switch_stop() <<<\n");
}

/**
 * @brief Check if the node switch is stopped
 * @param h - pointer to the node switch handle
 * @return true if the node switch is stopped, false otherwise
 */
bool bsc_node_switch_stopped(BSC_NODE_SWITCH_HANDLE h)
{
    BSC_NODE_SWITCH_CTX *ns = (BSC_NODE_SWITCH_CTX *)h;
    bool ret = false;

    DEBUG_PRINTF("bsc_node_switch_stopped() h = %p >>>\n", h);
    bws_dispatch_lock();
    if (ns && ns->acceptor.state == BSC_NODE_SWITCH_STATE_IDLE &&
        ns->initiator.state == BSC_NODE_SWITCH_STATE_IDLE) {
        ret = true;
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_switch_stoped() <<< ret = %d\n", ret);
    return ret;
}

/**
 * @brief Check if the node switch is started
 * @param h - pointer to the node switch handle
 * @return true if the node switch is started, false otherwise
 */
bool bsc_node_switch_started(BSC_NODE_SWITCH_HANDLE h)
{
    BSC_NODE_SWITCH_CTX *ns = (BSC_NODE_SWITCH_CTX *)h;
    bool ret = false;

    DEBUG_PRINTF("bsc_node_switch_started() h = %p >>>\n", h);
    bws_dispatch_lock();
    if (ns) {
        ret = true;
        if (ns->direct_connect_initiate_enable &&
            ns->initiator.state != BSC_NODE_SWITCH_STATE_STARTED) {
            ret = false;
        }
        if (ns->direct_connect_accept_enable &&
            ns->acceptor.state != BSC_NODE_SWITCH_STATE_STARTED) {
            ret = false;
        }
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_switch_started() <<< ret = %d\n", ret);
    return ret;
}

/**
 * @brief Connect to a node switch
 * @param h - pointer to the node switch handle
 * @param dest - pointer to the VMAC address
 * @param urls - pointer to the URLs
 * @param urls_cnt - number of URLs
 * @return BACnet/SC status
 */
BSC_SC_RET bsc_node_switch_connect(
    BSC_NODE_SWITCH_HANDLE h,
    BACNET_SC_VMAC_ADDRESS *dest,
    char **urls,
    size_t urls_cnt)
{
    BSC_NODE_SWITCH_CTX *ns;
    BSC_SC_RET ret = BSC_SC_INVALID_OPERATION;
    int i;

    DEBUG_PRINTF(
        "bsc_node_switch_connect() >>> h = %p, dest = %p, urls = %p, "
        "urls_cnt = %u\n",
        h, dest, urls, urls_cnt);

    for (i = 0; i < urls_cnt; i++) {
        if (strlen(urls[i]) >
            BSC_CONF_NODE_MAX_URI_SIZE_IN_ADDRESS_RESOLUTION_ACK) {
            DEBUG_PRINTF(
                "bsc_node_switch_connect() <<< ret =  BSC_SC_BAD_PARAM\n");
            return BSC_SC_BAD_PARAM;
        }
    }

    if (!dest && (!urls || !urls_cnt)) {
        DEBUG_PRINTF("bsc_node_switch_connect() <<< ret =  BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    if (dest && (urls || urls_cnt)) {
        DEBUG_PRINTF("bsc_node_switch_connect() <<< ret =  BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    bws_dispatch_lock();
    ns = (BSC_NODE_SWITCH_CTX *)h;
    if (ns->direct_connect_initiate_enable) {
        if (urls && urls_cnt) {
            i = node_switch_initiator_alloc_sock(ns);
            if (i == -1) {
                ret = BSC_SC_NO_RESOURCES;
            } else {
                copy_urls2(ns, i, urls, urls_cnt);
                ns->initiator.urls[i].url_elem = 0;
                node_switch_connect_or_delay(ns, NULL, i);
                ret = BSC_SC_SUCCESS;
            }
        } else {
            i = node_switch_initiator_find_connection_index_for_vmac(dest, ns);
            if (i != -1) {
                ret = BSC_SC_SUCCESS;
            } else {
                i = node_switch_initiator_alloc_sock(ns);
                if (i == -1) {
                    ret = BSC_SC_NO_RESOURCES;
                } else {
                    ns->initiator.urls[i].urls_cnt = 0;
                    node_switch_connect_or_delay(ns, dest, i);
                    ret = BSC_SC_SUCCESS;
                }
            }
        }
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_switch_connect() <<< ret = %d\n", ret);
    return ret;
}

/**
 * @brief Process an address resolution
 * @param h - pointer to the node switch handle
 * @param r - pointer to the address resolution
 */
void bsc_node_switch_process_address_resolution(
    BSC_NODE_SWITCH_HANDLE h, BSC_ADDRESS_RESOLUTION *r)
{
    BSC_NODE_SWITCH_CTX *ns;
    int i;

    DEBUG_PRINTF(
        "bsc_node_switch_process_address_resolution() >>> h = %p, r = "
        "%p (%s)\n",
        h, r, bsc_vmac_to_string(&r->vmac));
    bws_dispatch_lock();

    ns = (BSC_NODE_SWITCH_CTX *)h;
    if (r && r->urls_num) {
        i = node_switch_initiator_find_connection_index_for_vmac(&r->vmac, ns);
        if (i != -1 &&
            ns->initiator.sock_state[i] ==
                BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_RESOLUTION) {
            copy_urls(ns, i, r);
            ns->initiator.urls[i].url_elem = 0;
            node_switch_connect_or_delay(ns, NULL, i);
        }
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_switch_process_address_resolution() <<<\n");
}

/**
 * @brief Disconnect from a node switch
 * @param h - pointer to the node switch handle
 * @param dest - pointer to the VMAC address
 */
void bsc_node_switch_disconnect(
    BSC_NODE_SWITCH_HANDLE h, BACNET_SC_VMAC_ADDRESS *dest)
{
    BSC_NODE_SWITCH_CTX *ns;
    BSC_SOCKET *c;
    int i;

    DEBUG_PRINTF(
        "bsc_node_switch_disconnect() >>> h = %p, dest = %p (%s)\n", h, dest,
        bsc_vmac_to_string(dest));
    bws_dispatch_lock();
    ns = (BSC_NODE_SWITCH_CTX *)h;
    if (ns->direct_connect_initiate_enable) {
        i = node_switch_initiator_find_connection_index_for_vmac(dest, ns);

        if (i != -1) {
            if (ns->initiator.sock_state[i] !=
                BSC_NODE_SWITCH_CONNECTION_STATE_LOCAL_DISCONNECT) {
                if (ns->initiator.sock_state[i] ==
                        BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED ||
                    ns->initiator.sock_state[i] ==
                        BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_CONNECTION) {
                    c = &ns->initiator.sock[i];
                    bsc_disconnect(c);
                    ns->initiator.sock_state[i] =
                        BSC_NODE_SWITCH_CONNECTION_STATE_LOCAL_DISCONNECT;
                } else {
                    ns->initiator.sock_state[i] =
                        BSC_NODE_SWITCH_CONNECTION_STATE_IDLE;
                    ns->event_func(
                        BSC_NODE_SWITCH_EVENT_DISCONNECTED, ns, ns->user_arg,
                        &ns->initiator.dest_vmac[i], NULL, 0, NULL);
                }
            }
        }
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_switch_disconnect() <<<\n");
}

/**
 * @brief Send a PDU to a node switch
 * @param h - pointer to the node switch handle
 * @param pdu - pointer to the PDU
 * @param pdu_len - PDU length
 * @return BACnet/SC status
 */
BSC_SC_RET
bsc_node_switch_send(BSC_NODE_SWITCH_HANDLE h, uint8_t *pdu, size_t pdu_len)
{
    BSC_NODE_SWITCH_CTX *ns;
    BSC_SC_RET ret = BSC_SC_SUCCESS;
    BSC_SOCKET *c = NULL;
    BACNET_SC_VMAC_ADDRESS dest;
    uint8_t **ppdu = &pdu;
    int i;

    DEBUG_PRINTF(
        "bsc_node_switch_send() >>> pdu = %p, pdu_len = %u\n", pdu, pdu_len);
    bws_dispatch_lock();
    ns = (BSC_NODE_SWITCH_CTX *)h;
    if (bvlc_sc_pdu_has_no_dest(pdu, pdu_len) ||
        bvlc_sc_pdu_has_dest_broadcast(pdu, pdu_len)) {
        ret = bsc_node_hub_connector_send(ns->user_arg, pdu, pdu_len);
    } else {
        if (bvlc_sc_pdu_get_dest(pdu, pdu_len, &dest)) {
            i = node_switch_initiator_find_connection_index_for_vmac(&dest, ns);
            if (i != -1 &&
                ns->initiator.sock_state[i] ==
                    BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED) {
                c = &ns->initiator.sock[i];
            }
            if (!c) {
                c = node_switch_acceptor_find_connection_for_vmac(&dest, ns);
            }
            if (c && c->state == BSC_SOCK_STATE_CONNECTED) {
                pdu_len = bvlc_sc_remove_orig_and_dest(ppdu, pdu_len);
                if (pdu_len > 0) {
                    ret = bsc_send(c, *ppdu, pdu_len);
                }
            } else {
                ret = bsc_node_hub_connector_send(ns->user_arg, pdu, pdu_len);
            }
        }
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_switch_send() <<< ret = %d\n", ret);
    return ret;
}

/**
 * @brief Check if a node switch is connected
 * @param h - pointer to the node switch handle
 * @param dest - pointer to the VMAC address
 * @param urls - pointer to the URLs
 * @param urls_cnt - number of URLs
 */
BACNET_STACK_EXPORT
bool bsc_node_switch_connected(
    BSC_NODE_SWITCH_HANDLE h,
    BACNET_SC_VMAC_ADDRESS *dest,
    char **urls,
    size_t urls_cnt)
{
    BSC_NODE_SWITCH_CTX *ns;
    bool ret = false;
    int i, j, k;

    if (!dest && (!urls || urls_cnt == 0)) {
        return false;
    }

    bws_dispatch_lock();
    ns = (BSC_NODE_SWITCH_CTX *)h;
    if (ns->direct_connect_initiate_enable) {
        if (dest) {
            i = node_switch_initiator_find_connection_index_for_vmac(dest, ns);
            if (i != -1) {
                if (ns->initiator.sock_state[i] ==
                    BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED) {
                    ret = true;
                    bws_dispatch_unlock();
                    return ret;
                }
            }
        } else {
            for (i = 0; i < urls_cnt; i++) {
                for (j = 0; j < sizeof(ns->initiator.sock) / sizeof(BSC_SOCKET);
                     j++) {
                    if (ns->initiator.sock_state[j] ==
                        BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED) {
                        if (ns->initiator.urls[j].urls_cnt > 0) {
                            for (k = 0; k < ns->initiator.urls[j].urls_cnt;
                                 k++) {
                                if (strlen((char *)&ns->initiator.urls[j]
                                               .utf8_urls[k][0]) ==
                                    strlen(urls[i])) {
                                    if (!memcmp(
                                            (char *)&ns->initiator.urls[j]
                                                .utf8_urls[k][0],
                                            urls[i], strlen(urls[i]))) {
                                        ret = true;
                                        bws_dispatch_unlock();
                                        return ret;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (ns->direct_connect_accept_enable) {
        if (dest) {
            i = node_switch_acceptor_find_connection_index_for_vmac(dest, ns);
            if (i != -1) {
                ret = true;
            }
        }
    }
    bws_dispatch_unlock();
    return ret;
}
