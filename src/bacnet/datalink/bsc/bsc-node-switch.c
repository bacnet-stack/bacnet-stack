/**
 * @file
 * @brief BACNet secure connect node switch function API.
 * @author Kirill Neznamov
 * @date October 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-mutex.h"
#include "bacnet/datalink/bsc/bsc-runloop.h"
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
#define DEBUG_PRINTF(...)
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

static void node_switch_acceptor_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BSC_SC_RET err,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu);

static void node_switch_acceptor_context_event(
    BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);

static void node_switch_initiator_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BSC_SC_RET err,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu);

static void node_switch_initiator_context_event(
    BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);

typedef struct BSC_Node_Switch_Acceptor {
    BSC_SOCKET_CTX ctx;
    BSC_CONTEXT_CFG cfg;
    BSC_SOCKET sock[BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM];
    BSC_NODE_SWITCH_STATE state;
} BSC_NODE_SWITCH_ACCEPTOR;

typedef struct {
    uint8_t utf8_urls[BSC_CONF_NODE_MAX_URIS_NUM_IN_ADDRESS_RESOLUTION_ACK]
                     [BSC_CONF_NODE_MAX_URI_SIZE_IN_ADDRESS_RESOLUTION_ACK + 1];
    size_t urls_cnt;
    size_t url_elem;
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

static BSC_NODE_SWITCH_CTX bsc_node_switch[BSC_CONF_NODE_SWITCHES_NUM] = { false };

static BSC_SOCKET_CTX_FUNCS bsc_node_switch_acceptor_ctx_funcs = {
    node_switch_acceptor_find_connection_for_vmac,
    node_switch_acceptor_find_connection_for_uuid,
    node_switch_acceptor_socket_event, node_switch_acceptor_context_event
};

static BSC_SOCKET_CTX_FUNCS bsc_node_switch_initiator_ctx_funcs = { NULL, NULL,
    node_switch_initiator_socket_event, node_switch_initiator_context_event };

static BSC_NODE_SWITCH_CTX *node_switch_alloc(void)
{
    int i;
    for (i = 0; i < BSC_CONF_NODE_SWITCHES_NUM; i++) {
        if (!bsc_node_switch[i].used) {
            bsc_node_switch[i].used = true;
            memset(&bsc_node_switch[i].initiator, 0,
                sizeof(bsc_node_switch[i].initiator));
            memset(&bsc_node_switch[i].acceptor, 0,
                sizeof(bsc_node_switch[i].acceptor));
            return &bsc_node_switch[i];
        }
    }
    return NULL;
}

static void node_switch_free(BSC_NODE_SWITCH_CTX *ctx)
{
    ctx->used = false;
}

static void copy_urls(
    BSC_NODE_SWITCH_CTX *ctx, int index, BSC_ADDRESS_RESOLUTION *r)
{
    int i;
    for (i = 0; i < r->urls_num; i++) {
        ctx->initiator.urls[index].utf8_urls[i][0] = 0;
        strcpy((char *)&ctx->initiator.urls[index].utf8_urls[i][0],
            (char *)&r->utf8_urls[i][0]);
    }
    ctx->initiator.urls[index].urls_cnt = r->urls_num;
}

static void copy_urls2(
    BSC_NODE_SWITCH_CTX *ctx, int index, char **urls, size_t urls_cnt)
{
    size_t i;
    for (i = 0; i < urls_cnt; i++) {
        ctx->initiator.urls[index].utf8_urls[i][0] = 0;
        strcpy((char *)&ctx->initiator.urls[index].utf8_urls[i][0], urls[i]);
    }
    ctx->initiator.urls[index].urls_cnt = urls_cnt;
}

static BSC_SOCKET *node_switch_acceptor_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg)
{
    int i;
    BSC_NODE_SWITCH_CTX *c;
    bsc_global_mutex_lock();
    c = (BSC_NODE_SWITCH_CTX *)user_arg;
    for (i = 0; i < sizeof(c->acceptor.sock) / sizeof(BSC_SOCKET); i++) {
        DEBUG_PRINTF("ns = %p, sock %p, state = %d, vmac = %s\n", c,
            &c->acceptor.sock[i], c->acceptor.sock[i].state,
            bsc_vmac_to_string(&c->acceptor.sock[i].vmac));
        if (c->acceptor.sock[i].state != BSC_SOCK_STATE_IDLE &&
            !memcmp(&vmac->address[0], &c->acceptor.sock[i].vmac.address[0],
                sizeof(vmac->address))) {
            bsc_global_mutex_unlock();
            return &c->acceptor.sock[i];
        }
    }
    bsc_global_mutex_unlock();
    return NULL;
}

static BSC_SOCKET *node_switch_acceptor_find_connection_for_uuid(
    BACNET_SC_UUID *uuid, void *user_arg)
{
    int i;
    BSC_NODE_SWITCH_CTX *c;
    bsc_global_mutex_lock();
    c = (BSC_NODE_SWITCH_CTX *)user_arg;
    for (i = 0; i < sizeof(c->acceptor.sock) / sizeof(BSC_SOCKET); i++) {
        DEBUG_PRINTF("ns = %p, sock %p, state = %d, uuid = %s\n", c,
            &c->acceptor.sock[i], c->acceptor.sock[i].state,
            bsc_uuid_to_string(&c->acceptor.sock[i].uuid));
        if (c->acceptor.sock[i].state != BSC_SOCK_STATE_IDLE &&
            !memcmp(&uuid->uuid[0], &c->acceptor.sock[i].uuid.uuid[0],
                sizeof(uuid->uuid))) {
            DEBUG_PRINTF("found\n");
            bsc_global_mutex_unlock();
            return &c->acceptor.sock[i];
        }
    }
    bsc_global_mutex_unlock();
    return NULL;
}

static void node_switch_acceptor_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BSC_SC_RET err,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    uint8_t **ppdu = &pdu;
    BSC_NODE_SWITCH_CTX *ctx;

    bsc_global_mutex_lock();
    ctx = (BSC_NODE_SWITCH_CTX *)c->ctx->user_arg;

    // Node switch does not take care about incoming connection status,
    // so just route PDUs to upper layer

    if (ctx->acceptor.state == BSC_NODE_SWITCH_STATE_STARTED) {
        if (ev == BSC_SOCKET_EVENT_RECEIVED) {
            pdu_len = bvlc_sc_set_orig(ppdu, pdu_len, &c->vmac);
            ctx->event_func(BSC_NODE_SWITCH_EVENT_RECEIVED, ctx, ctx->user_arg,
                NULL, *ppdu, pdu_len, decoded_pdu);
        } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED &&
            err == BSC_SC_DUPLICATED_VMAC) {
            ctx->event_func(BSC_NODE_SWITCH_EVENT_DUPLICATED_VMAC, ctx,
                ctx->user_arg, NULL, NULL, 0, NULL);
        }
    }
    bsc_global_mutex_unlock();
}

static void node_switch_context_deinitialized(BSC_NODE_SWITCH_CTX *ns)
{
    if (ns->initiator.state == BSC_NODE_SWITCH_STATE_IDLE &&
        ns->acceptor.state == BSC_NODE_SWITCH_STATE_IDLE) {
        node_switch_free(ns);
        ns->event_func(BSC_NODE_SWITCH_EVENT_STOPPED, ns, ns->user_arg, NULL,
            NULL, 0, NULL);
    }
}

static void node_switch_acceptor_context_event(
    BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev)
{
    BSC_NODE_SWITCH_CTX *ns;

    bsc_global_mutex_lock();
    DEBUG_PRINTF("node_switch_acceptor_context_event() >>> ctx = %p, ev = %d, "
                 "user_arg = %p\n",
        ctx, ev, ctx->user_arg);
    ns = (BSC_NODE_SWITCH_CTX *)ctx->user_arg;
    if (ev == BSC_CTX_INITIALIZED) {
        if (ns->acceptor.state == BSC_NODE_SWITCH_STATE_STARTING) {
            ns->acceptor.state = BSC_NODE_SWITCH_STATE_STARTED;
            ns->event_func(BSC_NODE_SWITCH_EVENT_STARTED, ns, ns->user_arg,
                NULL, NULL, 0, NULL);
        }
    } else if (ev == BSC_CTX_DEINITIALIZED) {
        ns->acceptor.state = BSC_NODE_SWITCH_STATE_IDLE;
        node_switch_context_deinitialized(ns);
    }
    DEBUG_PRINTF("node_switch_acceptor_context_event() <<<\n");
    bsc_global_mutex_unlock();
}

static int node_switch_initiator_find_connection_index_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, BSC_NODE_SWITCH_CTX *ctx)
{
    int i;

    for (i = 0; i < sizeof(ctx->initiator.sock) / sizeof(BSC_SOCKET); i++) {
        if (ctx->initiator.sock_state[i] != BSC_NODE_SWITCH_CONNECTION_STATE_IDLE &&
            !memcmp(&ctx->initiator.dest_vmac[i].address[0], &vmac->address[0],
                sizeof(vmac->address))) {
            return i;
        }
    }
    return -1;
}

static int node_switch_initiator_get_index(
    BSC_NODE_SWITCH_CTX *ctx, BSC_SOCKET *c)
{
    return ((c - &ctx->initiator.sock[0]) >= 0) ? (c - &ctx->initiator.sock[0])
                                                : -1;
}

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
            ret = bsc_connect(&ctx->initiator.ctx, &ctx->initiator.sock[index],
                (char *)&ctx->initiator.urls[index]
                    .utf8_urls[ctx->initiator.urls[index].url_elem][0]);
            ctx->initiator.urls[index].url_elem++;
        }
    }
}

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
            memcpy(&ns->initiator.dest_vmac[sock_index].address[0],
                &dest->address[0], BVLC_SC_VMAC_SIZE);
            mstimer_set(&ns->initiator.t[sock_index],
                ns->address_resolution_timeout_s * 1000);
            (void) bsc_node_send_address_resolution(
                ns->user_arg, &ns->initiator.dest_vmac[sock_index]);
        }
    }
}

static void node_switch_initiator_runloop(void *ctx)
{
    int i;
    BSC_NODE_SWITCH_CTX *ns = (BSC_NODE_SWITCH_CTX *)ctx;

    // DEBUG_PRINTF("node_switch_initiator_runloop() >>> ctx = %p\n", ctx);
    bsc_global_mutex_lock();

    for (i = 0; i < sizeof(ns->initiator.sock) / sizeof(BSC_SOCKET); i++) {
        // DEBUG_PRINTF("node_switch_initiator_runloop() socket %d state %d\n",
        // i, ns->initiator.sock_state[i]);
        if (ns->initiator.sock_state[i] ==
            BSC_NODE_SWITCH_CONNECTION_STATE_DELAYING) {
            if (mstimer_expired(&ns->initiator.t[i])) {
                ns->initiator.urls[i].url_elem = 0;
                ns->initiator.sock_state[i] =
                    BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_CONNECTION;
                node_switch_connect_or_delay(
                    ns, &ns->initiator.dest_vmac[i], i);
            }
        } else if (ns->initiator.sock_state[i] ==
            BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_RESOLUTION) {
            if (mstimer_expired(&ns->initiator.t[i])) {
                ns->initiator.sock_state[i] =
                    BSC_NODE_SWITCH_CONNECTION_STATE_DELAYING;
                mstimer_set(
                    &ns->initiator.t[i], ns->reconnect_timeout_s * 1000);
            }
        }
    }

    bsc_global_mutex_unlock();
    // DEBUG_PRINTF("node_switch_initiator_runloop() <<< ctx = %p\n", ctx);
}

static void node_switch_initiator_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BSC_SC_RET err,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    int index;
    uint8_t **ppdu = &pdu;
    BSC_NODE_SWITCH_CTX *ns;

    DEBUG_PRINTF(
        "node_switch_initiator_socket_event() >>> c %p, ev = %d\n", c, ev);

    bsc_global_mutex_lock();

    ns = (BSC_NODE_SWITCH_CTX *)c->ctx->user_arg;

    if (ns->initiator.state == BSC_NODE_SWITCH_STATE_STARTED) {
        if (ev == BSC_SOCKET_EVENT_DISCONNECTED &&
            err == BSC_SC_DUPLICATED_VMAC) {
            ns->event_func(BSC_NODE_SWITCH_EVENT_DUPLICATED_VMAC, ns,
                ns->user_arg, NULL, NULL, 0, NULL);
        } else if (ev == BSC_SOCKET_EVENT_RECEIVED) {
            pdu_len = bvlc_sc_set_orig(ppdu, pdu_len, &c->vmac);
            ns->event_func(BSC_NODE_SWITCH_EVENT_RECEIVED, ns, ns->user_arg,
                NULL, *ppdu, pdu_len, decoded_pdu);
        }

        index = node_switch_initiator_get_index(ns, c);

        if (index > -1) {
            if (ns->initiator.sock_state[index] ==
                BSC_NODE_SWITCH_CONNECTION_STATE_WAIT_CONNECTION) {
                if (ev == BSC_SOCKET_EVENT_CONNECTED) {
                    ns->initiator.sock_state[index] =
                        BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED;
                    // if user provides url instead of dest in
                    // bsc_node_switch_connect() ns->initiator.dest_vmac[index]
                    // is unset. that's why we always set it from socket
                    // descriptor
                    memcpy(&ns->initiator.dest_vmac[index].address[0],
                        &c->vmac.address[0], sizeof(c->vmac.address));
                    ns->event_func(BSC_NODE_SWITCH_EVENT_CONNECTED, ns,
                        ns->user_arg, &ns->initiator.dest_vmac[index], NULL, 0,
                        NULL);
                } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
                    connect_next_url(ns, index);
                }
            } else if (ns->initiator.sock_state[index] ==
                BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED) {
                if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
                    ns->event_func(BSC_NODE_SWITCH_EVENT_DISCONNECTED, ns,
                        ns->user_arg, &ns->initiator.dest_vmac[index], NULL, 0,
                        NULL);
                    ns->initiator.urls[index].url_elem = 0;
                    connect_next_url(ns, index);
                }
            } else if (ns->initiator.sock_state[index] ==
                BSC_NODE_SWITCH_CONNECTION_STATE_LOCAL_DISCONNECT) {
                if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
                    ns->initiator.sock_state[index] =
                        BSC_NODE_SWITCH_CONNECTION_STATE_IDLE;
                    ns->event_func(BSC_NODE_SWITCH_EVENT_DISCONNECTED, ns,
                        ns->user_arg, &ns->initiator.dest_vmac[index], NULL, 0,
                        NULL);
                }
            }
        }
    }

    bsc_global_mutex_unlock();
    DEBUG_PRINTF("node_switch_initiator_socket_event() <<<\n");
}

static void node_switch_initiator_context_event(
    BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev)
{
    BSC_NODE_SWITCH_CTX *ns;
    int i;

    bsc_global_mutex_lock();
    DEBUG_PRINTF("node_switch_initiator_context_event () >>> ctx = %p, ev = "
                 "%d, user_arg = %p\n",
        ctx, ev, ctx->user_arg);
    ns = (BSC_NODE_SWITCH_CTX *)ctx->user_arg;
    if (ev == BSC_CTX_DEINITIALIZED) {
        for (i = 0; i < sizeof(ns->initiator.sock) / sizeof(BSC_SOCKET); i++) {
            if (ns->initiator.sock_state[i] ==
                BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED) {
                ns->initiator.sock_state[i] =
                    BSC_NODE_SWITCH_CONNECTION_STATE_IDLE;
                ns->event_func(BSC_NODE_SWITCH_EVENT_DISCONNECTED, ns,
                    ns->user_arg, &ns->initiator.dest_vmac[i], NULL, 0, NULL);
            }
        }
        ns->initiator.state = BSC_NODE_SWITCH_STATE_IDLE;
        node_switch_context_deinitialized(ns);
    }
    DEBUG_PRINTF("node_switch_initiator_context_event() <<<\n");
    bsc_global_mutex_unlock();
}

BSC_SC_RET bsc_node_switch_start(uint8_t *ca_cert_chain,
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
    unsigned int connect_timeout_s,
    unsigned int heartbeat_timeout_s,
    unsigned int disconnect_timeout_s,
    unsigned int reconnnect_timeout_s,
    unsigned int address_resolution_timeout_s,
    bool direct_connect_accept_enable,
    bool direct_connect_initiate_enable,
    BSC_NODE_SWITCH_EVENT_FUNC event_func,
    void *user_arg,
    BSC_NODE_SWITCH_HANDLE *h)
{
    BSC_SC_RET ret;
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
    bsc_global_mutex_lock();
    ns = node_switch_alloc();

    if (!ns) {
        bsc_global_mutex_unlock();
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

    ret = bsc_runloop_reg(
        bsc_global_runloop(), ns, node_switch_initiator_runloop);

    if (ret != BSC_SC_SUCCESS) {
        node_switch_free(ns);
        bsc_global_mutex_unlock();
        DEBUG_PRINTF("bsc_node_switch_start() <<< ret = %d\n", ret);
        return ret;
    }

    if (direct_connect_initiate_enable) {
        bsc_init_ctx_cfg(BSC_SOCKET_CTX_INITIATOR, &ns->initiator.cfg,
            BSC_WEBSOCKET_DIRECT_PROTOCOL, 0, NULL, ca_cert_chain,
            ca_cert_chain_size, cert_chain, cert_chain_size, key, key_size,
            local_uuid, local_vmac, max_local_bvlc_len, max_local_npdu_len,
            connect_timeout_s, heartbeat_timeout_s, disconnect_timeout_s);
        ret = bsc_init_ctx(&ns->initiator.ctx, &ns->initiator.cfg,
            &bsc_node_switch_initiator_ctx_funcs, ns->initiator.sock,
            sizeof(ns->initiator.sock) / sizeof(BSC_SOCKET), ns);
        if (ret == BSC_SC_SUCCESS) {
            ns->initiator.state = BSC_NODE_SWITCH_STATE_STARTED;
        }
    }

    if (ret == BSC_SC_SUCCESS && direct_connect_accept_enable) {
        bsc_init_ctx_cfg(BSC_SOCKET_CTX_ACCEPTOR, &ns->acceptor.cfg,
            BSC_WEBSOCKET_DIRECT_PROTOCOL, port, iface, ca_cert_chain,
            ca_cert_chain_size, cert_chain, cert_chain_size, key, key_size,
            local_uuid, local_vmac, max_local_bvlc_len, max_local_npdu_len,
            connect_timeout_s, heartbeat_timeout_s, disconnect_timeout_s);
        ret = bsc_init_ctx(&ns->acceptor.ctx, &ns->acceptor.cfg,
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
        bsc_runloop_unreg(bsc_global_runloop(), ns);
        node_switch_free(ns);
    } else {
        *h = ns;
        if (direct_connect_initiate_enable && !direct_connect_accept_enable) {
            ns->event_func(BSC_NODE_SWITCH_EVENT_STARTED, ns, ns->user_arg,
                NULL, NULL, 0, NULL);
        }
    }
    bsc_global_mutex_unlock();
    DEBUG_PRINTF("bsc_node_switch_start() <<< ret = %d\n", ret);
    return ret;
}

void bsc_node_switch_stop(BSC_NODE_SWITCH_HANDLE h)
{
    BSC_NODE_SWITCH_CTX *ns;
    DEBUG_PRINTF("bsc_node_switch_stop() >>> h = %p \n", h);
    bsc_global_mutex_lock();
    ns = (BSC_NODE_SWITCH_CTX *)h;
    if (ns) {
        DEBUG_PRINTF("bsc_node_switch_stop() unreg loop\n");
        bsc_runloop_unreg(bsc_global_runloop(), ns);
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
    bsc_global_mutex_unlock();
    DEBUG_PRINTF("bsc_node_switch_stop() <<<\n");
}

bool bsc_node_switch_stopped(BSC_NODE_SWITCH_HANDLE h)
{
    BSC_NODE_SWITCH_CTX *ns = (BSC_NODE_SWITCH_CTX *)h;
    bool ret = false;

    DEBUG_PRINTF("bsc_node_switch_stopped() h = %p >>>\n", h);
    bsc_global_mutex_lock();
    if (ns && ns->acceptor.state == BSC_NODE_SWITCH_STATE_IDLE &&
        ns->initiator.state == BSC_NODE_SWITCH_STATE_IDLE) {
        ret = true;
    }
    bsc_global_mutex_unlock();
    DEBUG_PRINTF("bsc_node_switch_stoped() <<< ret = %d\n", ret);
    return ret;
}

bool bsc_node_switch_started(BSC_NODE_SWITCH_HANDLE h)
{
    BSC_NODE_SWITCH_CTX *ns = (BSC_NODE_SWITCH_CTX *)h;
    bool ret = false;

    DEBUG_PRINTF("bsc_node_switch_started() h = %p >>>\n", h);
    bsc_global_mutex_lock();
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
    bsc_global_mutex_unlock();
    DEBUG_PRINTF("bsc_node_switch_started() <<< ret = %d\n", ret);
    return ret;
}

BSC_SC_RET bsc_node_switch_connect(BSC_NODE_SWITCH_HANDLE h,
    BACNET_SC_VMAC_ADDRESS *dest,
    char **urls,
    size_t urls_cnt)
{
    BSC_NODE_SWITCH_CTX *ns;
    BSC_SC_RET ret = BSC_SC_INVALID_OPERATION;
    int i;

    DEBUG_PRINTF("bsc_node_switch_connect() >>> h = %p, dest = %p, urls = %p, "
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

    bsc_global_mutex_lock();
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
    bsc_global_mutex_unlock();
    DEBUG_PRINTF("bsc_node_switch_connect() <<< ret = %d\n", ret);
    return ret;
}

void bsc_node_switch_process_address_resolution(
    BSC_NODE_SWITCH_HANDLE h, BSC_ADDRESS_RESOLUTION *r)
{
    BSC_NODE_SWITCH_CTX *ns;
    int i;

    DEBUG_PRINTF("bsc_node_switch_process_address_resolution() >>> h = %p, r = "
                 "%p (%s)\n",
        h, r, bsc_vmac_to_string(&r->vmac));
    bsc_global_mutex_lock();

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

    bsc_global_mutex_unlock();
    DEBUG_PRINTF("bsc_node_switch_process_address_resolution() <<<\n");
}

void bsc_node_switch_disconnect(
    BSC_NODE_SWITCH_HANDLE h, BACNET_SC_VMAC_ADDRESS *dest)
{
    BSC_NODE_SWITCH_CTX *ns;
    BSC_SOCKET *c;
    int i;

    DEBUG_PRINTF("bsc_node_switch_disconnect() >>> h = %p, dest = %p (%s)\n", h,
        dest, bsc_vmac_to_string(dest));
    bsc_global_mutex_lock();
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
                    ns->event_func(BSC_NODE_SWITCH_EVENT_DISCONNECTED, ns,
                        ns->user_arg, &ns->initiator.dest_vmac[i], NULL, 0,
                        NULL);
                }
            }
        }
    }

    bsc_global_mutex_unlock();
    DEBUG_PRINTF("bsc_node_switch_disconnect() <<<\n");
}

BSC_SC_RET bsc_node_switch_send(
    BSC_NODE_SWITCH_HANDLE h, uint8_t *pdu, unsigned pdu_len)
{
    BSC_NODE_SWITCH_CTX *ns;
    BSC_SC_RET ret = BSC_SC_SUCCESS;
    BSC_SOCKET *c = NULL;
    BACNET_SC_VMAC_ADDRESS dest;
    uint8_t **ppdu = &pdu;
    int i;

    DEBUG_PRINTF(
        "bsc_node_switch_send() >>> pdu = %p, pdu_len = %u\n", pdu, pdu_len);
    bsc_global_mutex_lock();
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
    bsc_global_mutex_unlock();
    DEBUG_PRINTF("bsc_node_switch_send() <<< ret = %d\n", ret);
    return ret;
}

BACNET_STACK_EXPORT
bool bsc_node_switch_connected(BSC_NODE_SWITCH_HANDLE h,
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

    bsc_global_mutex_lock();
    ns = (BSC_NODE_SWITCH_CTX *)h;
    if (ns->direct_connect_initiate_enable) {
        if (dest) {
            i = node_switch_initiator_find_connection_index_for_vmac(dest, ns);
            if (i != -1) {
                if (ns->initiator.sock_state[i] ==
                    BSC_NODE_SWITCH_CONNECTION_STATE_CONNECTED) {
                    ret = true;
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
                                    if (!memcmp((char *)&ns->initiator.urls[j]
                                                    .utf8_urls[k][0],
                                            urls[i], strlen(urls[i]))) {
                                        ret = true;
                                        bsc_global_mutex_unlock();
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
    bsc_global_mutex_unlock();
    return ret;
}