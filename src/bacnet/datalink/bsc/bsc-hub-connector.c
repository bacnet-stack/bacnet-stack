/**
 * @file
 * @brief BACNet hub connector API.
 * @author Kirill Neznamov
 * @date July 2022
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
#include "bacnet/datalink/bsc/bsc-hub-connector.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"

typedef enum {
    BSC_HUB_CONN_PRIMARY = 0,
    BSC_HUB_CONN_FAILOVER = 1
} BSC_HUB_CONN_TYPE;

static BSC_SOCKET *hub_connector_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg);

static BSC_SOCKET *hub_connector_find_connection_for_uuid(
    BACNET_SC_UUID *uuid, void *user_arg);

static void hub_connector_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BSC_SC_RET err,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu);

static void hub_connector_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);

static uint16_t hub_connector_get_next_message_id(void *user_arg);

typedef enum {
    BSC_HUB_CONNECTOR_STATE_IDLE = 0,
    BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY = 1,
    BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER = 2,
    BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY = 3,
    BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER = 4,
    BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT = 5,
    BSC_HUB_CONNECTOR_STATE_WAIT_FOR_CTX_DEINIT = 6
} BSC_HUB_CONNECTOR_STATE;

typedef struct BSC_Hub_Connector {
    BSC_SOCKET_CTX ctx;
    BSC_CONTEXT_CFG cfg;
    BSC_SOCKET sock[2];
    BSC_HUB_CONNECTOR_STATE state;
    unsigned int reconnect_timeout_s;
    uint8_t primary_url[BSC_WSURL_MAX_LEN + 1];
    uint8_t failover_url[BSC_WSURL_MAX_LEN + 1];
    struct mstimer t;
    BSC_HUB_CONNECTOR_EVENT_FUNC event_func;
    void *user_arg;
    bool used;
} BSC_HUB_CONNECTOR;

static BSC_HUB_CONNECTOR bsc_hub_connector[BSC_CONF_HUB_CONNECTORS_NUM] = { 0 };
static bool bsc_runloop_registered;

static BSC_SOCKET_CTX_FUNCS bsc_hub_connector_ctx_funcs = {
    hub_connector_find_connection_for_vmac,
    hub_connector_find_connection_for_uuid, hub_connector_socket_event,
    hub_connector_context_event
};

static BSC_SOCKET *hub_connector_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg)
{
    return NULL;
}

static BSC_SOCKET *hub_connector_find_connection_for_uuid(
    BACNET_SC_UUID *uuid, void *user_arg)
{
    return NULL;
}

static BSC_HUB_CONNECTOR *hub_connector_alloc(void)
{
    int i;
    for (i = 0; i < BSC_CONF_HUB_CONNECTORS_NUM; i++) {
        if (!bsc_hub_connector[i].used) {
            bsc_hub_connector[i].used = true;
            debug_printf(
                "hub_connector_alloc() ret = %p\n", &bsc_hub_connector[i]);
            return &bsc_hub_connector[i];
        }
    }
    debug_printf("hub_connector_alloc() ret = %p\n", &bsc_hub_connector[i]);
    return NULL;
}

static void hub_connector_free(BSC_HUB_CONNECTOR *c)
{
    int i;
    c->used = false;
}

static void hub_connector_connect(BSC_HUB_CONNECTOR *p, BSC_HUB_CONN_TYPE type)
{
    BSC_SC_RET ret;
    p->state = (type == BSC_HUB_CONN_PRIMARY)
        ? BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY
        : BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER;

    ret = bsc_connect(&p->ctx, &p->sock[type],
        (type == BSC_HUB_CONN_PRIMARY) ? (char *)p->primary_url
                                       : (char *)p->failover_url);

    if (ret != BSC_SC_SUCCESS) {
        debug_printf("hub_connector_connect() got error while "
                     "connecting to hub type %d, err = %d\n",
            type, ret);
    }
}

static void hub_connector_process_state(void *ctx)
{
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)ctx;
    int i;

    bsc_global_mutex_lock();
    if (c->state == BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT) {
        if (mstimer_expired(&c->t)) {
            hub_connector_connect(c, BSC_HUB_CONN_PRIMARY);
        }
    }
    bsc_global_mutex_unlock();
}

static void hub_connector_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BSC_SC_RET err,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    BSC_SC_RET ret;
    BSC_HUB_CONNECTOR *hc;
    uint8_t **ppdu = &pdu;

    bsc_global_mutex_lock();
    hc = (BSC_HUB_CONNECTOR *)c->ctx->user_arg;
    debug_printf("hub_connector_socket_event() >>> hub_connector = %p, socket "
                 "= %p, ev = %d, err = %d, "
                 "pdu = %p, pdu_len = %d\n",
        hc, c, ev, err, pdu, pdu_len);
    if (ev == BSC_SOCKET_EVENT_CONNECTED) {
        if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY) {
            hc->state = BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY;
            hc->event_func(BSC_HUBC_EVENT_CONNECTED_PRIMARY, hc, hc->user_arg,
                NULL, 0, NULL);
        } else if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER) {
            hc->state = BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER;
            hc->event_func(BSC_HUBC_EVENT_CONNECTED_FAILOVER, hc, hc->user_arg,
                NULL, 0, NULL);
        }
    } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
        if (err == BSC_SC_DUPLICATED_VMAC) {
            debug_printf("hub_connector_socket_event()"
                         "got BSC_SC_DUPLICATED_VMAC error\n");
            hc->event_func(BSC_HUBC_EVENT_ERROR_DUPLICATED_VMAC, hc,
                hc->user_arg, NULL, 0, NULL);
        }
        if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY) {
            hub_connector_connect(hc, BSC_HUB_CONN_FAILOVER);
        } else if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER) {
            debug_printf("hub_connector_socket_event() wait for %d seconds\n",
                hc->reconnect_timeout_s);
            hc->state = BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT;
            mstimer_set(&hc->t, hc->reconnect_timeout_s * 1000);
        } else if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY ||
            hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER) {
            hc->event_func(
                BSC_HUBC_EVENT_DISCONNECTED, hc, hc->user_arg, NULL, 0, NULL);
            hub_connector_connect(hc, BSC_HUB_CONN_PRIMARY);
        }
    } else if (ev == BSC_SOCKET_EVENT_RECEIVED) {
        if (decoded_pdu->hdr.origin == NULL) {
            pdu_len = bvlc_sc_set_orig(ppdu, pdu_len, &c->vmac);
        }
        hc->event_func(BSC_HUBC_EVENT_RECEIVED, hc, hc->user_arg, *ppdu,
            pdu_len, decoded_pdu);
    }
    bsc_global_mutex_unlock();
    debug_printf("hub_connector_context_event() <<<\n");
}

static void hub_connector_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev)
{
    BSC_HUB_CONNECTOR *c;

    debug_printf(
        "hub_connector_context_event() >>> ctx = %p, ev = %d\n", ctx, ev);

    if (ev == BSC_CTX_DEINITIALIZED) {
        bsc_global_mutex_lock();
        c = (BSC_HUB_CONNECTOR *)ctx->user_arg;
        if (c->state != BSC_HUB_CONNECTOR_STATE_IDLE) {
            c->state = BSC_HUB_CONNECTOR_STATE_IDLE;
            hub_connector_free(c);
            c->event_func(
                BSC_HUBC_EVENT_STOPPED, c, c->user_arg, NULL, 0, NULL);
        }
        bsc_global_mutex_unlock();
    }

    debug_printf("hub_connector_context_event() <<<\n");
}

BACNET_STACK_EXPORT
BSC_SC_RET bsc_hub_connector_start(uint8_t *ca_cert_chain,
    size_t ca_cert_chain_size,
    uint8_t *cert_chain,
    size_t cert_chain_size,
    uint8_t *key,
    size_t key_size,
    BACNET_SC_UUID *local_uuid,
    BACNET_SC_VMAC_ADDRESS *local_vmac,
    uint16_t max_local_bvlc_len,
    uint16_t max_local_npdu_len,
    unsigned int connect_timeout_s,
    unsigned int heartbeat_timeout_s,
    unsigned int disconnect_timeout_s,
    char *primaryURL,
    char *failoverURL,
    unsigned int reconnect_timeout_s,
    BSC_HUB_CONNECTOR_EVENT_FUNC event_func,
    void *user_arg,
    BSC_HUB_CONNECTOR_HANDLE *h)
{
    BSC_SC_RET ret = BSC_SC_SUCCESS;
    BSC_HUB_CONNECTOR *c;

    debug_printf("bsc_hub_connector_start() >>>\n");

    if (!ca_cert_chain || !ca_cert_chain_size || !cert_chain ||
        !cert_chain_size || !key || !key_size || !local_uuid || !local_vmac ||
        !max_local_npdu_len || !max_local_bvlc_len || !connect_timeout_s ||
        !heartbeat_timeout_s || !disconnect_timeout_s || !primaryURL ||
        !failoverURL || !reconnect_timeout_s || !event_func || !h) {
        debug_printf("bsc_hub_connector_start() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    if (strlen(primaryURL) > BSC_WSURL_MAX_LEN ||
        strlen(failoverURL) > BSC_WSURL_MAX_LEN) {
        debug_printf("bsc_hub_connector_start() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    bsc_global_mutex_lock();
    c = hub_connector_alloc();
    if (!c) {
        bsc_global_mutex_unlock();
        debug_printf(
            "bsc_hub_connector_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    c->reconnect_timeout_s = reconnect_timeout_s;
    c->primary_url[0] = 0;
    c->failover_url[0] = 0;
    c->user_arg = user_arg;
    strcpy((char *)c->primary_url, primaryURL);
    strcpy((char *)c->failover_url, failoverURL);
    c->event_func = event_func;

    bsc_init_ctx_cfg(BSC_SOCKET_CTX_INITIATOR, &c->cfg,
        BSC_WEBSOCKET_HUB_PROTOCOL, 0, NULL, ca_cert_chain, ca_cert_chain_size,
        cert_chain, cert_chain_size, key, key_size, local_uuid, local_vmac,
        max_local_bvlc_len, max_local_npdu_len, connect_timeout_s,
        heartbeat_timeout_s, disconnect_timeout_s);
    debug_printf("bsc_hub_connector_start() uuid = %s, vmac = %s\n",
        bsc_uuid_to_string(&c->cfg.local_uuid),
        bsc_vmac_to_string(&c->cfg.local_vmac));
    ret = bsc_runloop_reg(bsc_global_runloop(), c, hub_connector_process_state);

    if (ret == BSC_SC_SUCCESS) {
        ret = bsc_init_сtx(&c->ctx, &c->cfg, &bsc_hub_connector_ctx_funcs,
            c->sock, sizeof(c->sock) / sizeof(BSC_SOCKET), (void *)c);

        if (ret == BSC_SC_SUCCESS) {
            *h = (BSC_HUB_CONNECTOR_HANDLE)c;
            ret = bsc_connect(&c->ctx, &c->sock[BSC_HUB_CONN_PRIMARY],
                (char *)c->primary_url);
            if (ret == BSC_SC_SUCCESS) {
                c->state = BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY;
            } else {
                c->state = BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER;
                ret = bsc_connect(&c->ctx, &c->sock[BSC_HUB_CONN_FAILOVER],
                    (char *)c->failover_url);
                if (ret != BSC_SC_SUCCESS) {
                    bsc_runloop_unreg(bsc_global_runloop(), c);
                    bsc_deinit_ctx(&c->ctx);
                    hub_connector_free(c);
                    *h = NULL;
                }
            }
        }
    }

    bsc_global_mutex_unlock();
    debug_printf("bsc_hub_connector_start() <<< ret = %d\n", ret);
    return ret;
}

void bsc_hub_connector_stop(BSC_HUB_CONNECTOR_HANDLE h)
{
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)h;
    debug_printf("bsc_hub_connector_stop() h = %p>>>\n", h);
    bsc_global_mutex_lock();

    if (c->state != BSC_HUB_CONNECTOR_STATE_WAIT_FOR_CTX_DEINIT &&
        c->state != BSC_HUB_CONNECTOR_STATE_IDLE) {
        c->state = BSC_HUB_CONNECTOR_STATE_WAIT_FOR_CTX_DEINIT;
        bsc_runloop_unreg(bsc_global_runloop(), c);
        bsc_deinit_ctx(&c->ctx);
    }
    bsc_global_mutex_unlock();
    debug_printf("bsc_hub_connector_stop() <<<\n");
}

BACNET_STACK_EXPORT
BSC_SC_RET bsc_hub_connector_send(
    BSC_HUB_CONNECTOR_HANDLE h, uint8_t *pdu, unsigned pdu_len)
{
    BSC_SC_RET ret;
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)h;

    debug_printf(
        "bsc_hub_connector_send() >>> h = %p,  pdu = %p, pdu_len = %d\n", h,
        pdu, pdu_len);

    bsc_global_mutex_lock();

    if (c->state == BSC_HUB_CONNECTOR_STATE_IDLE ||
        (c->state != BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY &&
            c->state != BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER)) {
        debug_printf("bsc_hub_connector_send() pdu is dropped\n");
        debug_printf(
            "bsc_hub_connector_send() <<< ret = BSC_SC_INVALID_OPERATION\n");
        return BSC_SC_INVALID_OPERATION;
    }
    if (c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY) {
        ret = bsc_send(&c->sock[BSC_HUB_CONN_PRIMARY], pdu, pdu_len);
    } else {
        ret = bsc_send(&c->sock[BSC_HUB_CONN_FAILOVER], pdu, pdu_len);
    }
    bsc_global_mutex_unlock();
    debug_printf("bsc_hub_connector_send() <<< ret = %d\n", ret);
    return ret;
}

BACNET_STACK_EXPORT
bool bsc_hub_connector_stopped(BSC_HUB_CONNECTOR_HANDLE h)
{
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)h;
    bool ret = false;

    debug_printf("bsc_hub_connector_stopped() >>> h = %p\n", h);
    bsc_global_mutex_lock();
    if (c->state == BSC_HUB_CONNECTOR_STATE_IDLE) {
        ret = true;
    }
    bsc_global_mutex_unlock();
    debug_printf("bsc_hub_connector_stopped() <<< ret = %d\n", ret);
    return ret;
}

BACNET_STACK_EXPORT
BVLC_SC_HUB_CONNECTION_STATUS bsc_hub_connector_status(
    BSC_HUB_CONNECTOR_HANDLE h)
{
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)h;
    BVLC_SC_HUB_CONNECTION_STATUS ret = BVLC_SC_HUB_CONNECTION_ABSENT;
    bsc_global_mutex_lock();
    if (c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY) {
        ret = BVLC_SC_HUB_CONNECTION_PRIMARY_HUB_CONNECTED;
    } else if (c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER) {
        ret = BVLC_SC_HUB_CONNECTION_FAILOVER_HUB_CONNECTED;
    }
    bsc_global_mutex_unlock();
    return ret;
}
