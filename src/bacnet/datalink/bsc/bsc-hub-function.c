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
#include "bacnet/datalink/bsc/bsc-conf.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-hub-function.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"

#define DEBUG_BSC_HUB_FUNCTION 0

#if DEBUG_BSC_HUB_FUNCTION == 1
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF debug_printf_disabled
#endif

static BSC_SOCKET *hub_function_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg);

static BSC_SOCKET *hub_function_find_connection_for_uuid(
    BACNET_SC_UUID *uuid, void *user_arg);

static void hub_function_failed_request(BSC_SOCKET_CTX *ctx,
    BSC_SOCKET *c,
    BACNET_SC_VMAC_ADDRESS *vmac,
    BACNET_SC_UUID *uuid,
    BACNET_ERROR_CODE error,
    const char *error_desc);

static void hub_function_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE reason,
    const char *reason_desc,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu);

static void hub_function_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);

typedef enum {
    BSC_HUB_FUNCTION_STATE_IDLE = 0,
    BSC_HUB_FUNCTION_STATE_STARTING = 1,
    BSC_HUB_FUNCTION_STATE_STARTED = 2,
    BSC_HUB_FUNCTION_STATE_STOPPING = 3
} BSC_HUB_FUNCTION_STATE;

typedef struct BSC_Hub_Connector {
    bool used;
    BSC_SOCKET_CTX ctx;
    BSC_CONTEXT_CFG cfg;
    BSC_SOCKET sock[BSC_CONF_HUB_FUNCTION_CONNECTIONS_NUM];
    BSC_HUB_FUNCTION_STATE state;
    BSC_HUB_EVENT_FUNC event_func;
    void *user_arg;
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *status;
} BSC_HUB_FUNCTION;

static BSC_HUB_FUNCTION bsc_hub_function[BSC_CONF_HUB_FUNCTIONS_NUM] = { 0 };
static BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS
    bsc_hub_status[BSC_CONF_HUB_FUNCTIONS_NUM]
                  [BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM];
static bool bsc_hub_status_initialized[BSC_CONF_HUB_FUNCTIONS_NUM] = { 0 };

static BSC_SOCKET_CTX_FUNCS bsc_hub_function_ctx_funcs = {
    hub_function_find_connection_for_vmac,
    hub_function_find_connection_for_uuid, hub_function_socket_event,
    hub_function_context_event, hub_function_failed_request
};

static BSC_HUB_FUNCTION *hub_function_alloc(void)
{
    int i, j;
    for (i = 0; i < BSC_CONF_HUB_FUNCTIONS_NUM; i++) {
        if (!bsc_hub_function[i].used) {
            bsc_hub_function[i].used = true;
            bsc_hub_function[i].status = &bsc_hub_status[i][0];

            /* Start/stop cycles of a hub function must not make an influence to
             * history related to connection status */
            /* That's why bsc_hub_function[i].status array is initialized only
             * once */

            if (!bsc_hub_status_initialized[i]) {
                for (j = 0; j < BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM;
                     j++) {
                    memset(&bsc_hub_function[i].status[j], 0,
                        sizeof(bsc_hub_function[i].status[j]));
                    /* set timestamps to unspecified values */
                    memset(&bsc_hub_function[i].status[j].Disconnect_Timestamp,
                        0xFF,
                        sizeof(bsc_hub_function[i]
                                   .status[j]
                                   .Disconnect_Timestamp));
                    memset(&bsc_hub_function[i].status[j].Connect_Timestamp,
                        0xFF,
                        sizeof(
                            bsc_hub_function[i].status[j].Connect_Timestamp));
                }
                bsc_hub_status_initialized[i] = true;
            }
            return &bsc_hub_function[i];
        }
    }
    return NULL;
}

static void hub_function_free(BSC_HUB_FUNCTION *p)
{
    p->used = false;
}

static BSC_SOCKET *hub_function_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg)
{
    int i;
    BSC_HUB_FUNCTION *f;

    bws_dispatch_lock();
    f = (BSC_HUB_FUNCTION *)user_arg;
    DEBUG_PRINTF("hubf = %p local_vmac = %s\n", f,
        bsc_vmac_to_string(&f->cfg.local_vmac));
    for (i = 0; i < sizeof(f->sock) / sizeof(BSC_SOCKET); i++) {
        DEBUG_PRINTF("hubf = %p, sock %p, state = %d, vmac = %s\n", f,
            &f->sock[i], f->sock[i].state,
            bsc_vmac_to_string(&f->sock[i].vmac));
        if (f->sock[i].state != BSC_SOCK_STATE_IDLE &&
            !memcmp(&vmac->address[0], &f->sock[i].vmac.address[0],
                sizeof(vmac->address))) {
            bws_dispatch_unlock();
            return &f->sock[i];
        }
    }
    bws_dispatch_unlock();
    return NULL;
}

static BSC_SOCKET *hub_function_find_connection_for_uuid(
    BACNET_SC_UUID *uuid, void *user_arg)
{
    int i;
    BSC_HUB_FUNCTION *f;

    bws_dispatch_lock();
    f = (BSC_HUB_FUNCTION *)user_arg;
    for (i = 0; i < sizeof(f->sock) / sizeof(BSC_SOCKET); i++) {
        DEBUG_PRINTF("hubf = %p, sock %p, state = %d, uuid = %s\n", f,
            &f->sock[i], f->sock[i].state,
            bsc_uuid_to_string(&f->sock[i].uuid));
        if (f->sock[i].state != BSC_SOCK_STATE_IDLE &&
            !memcmp(
                &uuid->uuid[0], &f->sock[i].uuid.uuid[0], sizeof(uuid->uuid))) {
            bws_dispatch_unlock();
            DEBUG_PRINTF("found socket\n");
            return &f->sock[i];
        }
    }
    bws_dispatch_unlock();
    return NULL;
}

static BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *
hub_function_find_status_for_vmac(
    BSC_HUB_FUNCTION *f, BACNET_SC_VMAC_ADDRESS *vmac)
{
    int i;
    int non_connected_index = -1;
    BACNET_DATE_TIME timestamp;

    for (i = 0; i < BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM; i++) {
        if (!memcmp(&f->status[i].Peer_VMAC[0], &vmac->address[0],
                BVLC_SC_VMAC_SIZE)) {
            return &f->status[i];
        }
        if (f->status[i].State != BACNET_CONNECTED) {
            if (non_connected_index < 0) {
                non_connected_index = i;
                memcpy(&timestamp, &f->status[i].Disconnect_Timestamp,
                    sizeof(timestamp));
            } else {
                if (datetime_is_valid(&f->status[i].Disconnect_Timestamp.date,
                        &f->status[i].Disconnect_Timestamp.time) &&
                    datetime_is_valid(&timestamp.date, &timestamp.time)) {
                    if (datetime_compare(&f->status[i].Disconnect_Timestamp,
                            &timestamp) < 0) {
                        non_connected_index = i;
                        memcpy(&timestamp, &f->status[i].Disconnect_Timestamp,
                            sizeof(timestamp));
                    }
                }
            }
        }
    }

    return non_connected_index < 0 ? NULL : &f->status[non_connected_index];
}

static void hub_function_update_status(BSC_HUB_FUNCTION *f,
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE disconnect_reason,
    const char *disconnect_reason_desc)
{
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *s;

    s = hub_function_find_status_for_vmac(f, &c->vmac);

    if (s) {
        memcpy(s->Peer_VMAC, &c->vmac.address[0], BVLC_SC_VMAC_SIZE);
        memcpy(
            &s->Peer_UUID.uuid.uuid128[0], &c->uuid.uuid[0], BVLC_SC_UUID_SIZE);
        if (!bsc_socket_get_peer_addr(c, &s->Peer_Address)) {
            memset(&s->Peer_Address, 0, sizeof(s->Peer_Address));
        }
        if (disconnect_reason_desc) {
            bsc_copy_str(s->Error_Details, disconnect_reason_desc,
                sizeof(s->Error_Details));
        } else {
            s->Error_Details[0] = 0;
        }
        s->Error = ERROR_CODE_OTHER;
        if (ev == BSC_SOCKET_EVENT_CONNECTED) {
            s->State = BACNET_CONNECTED;
            bsc_set_timestamp(&s->Connect_Timestamp);
        } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
            bsc_set_timestamp(&s->Disconnect_Timestamp);
            if (disconnect_reason == ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER ||
                disconnect_reason == ERROR_CODE_SUCCESS) {
                s->State = BACNET_NOT_CONNECTED;
            } else {
                s->State = BACNET_DISCONNECTED_WITH_ERRORS;
                s->Error = disconnect_reason;
            }
        }
    }
}

static void hub_function_failed_request(BSC_SOCKET_CTX *ctx,
    BSC_SOCKET *c,
    BACNET_SC_VMAC_ADDRESS *vmac,
    BACNET_SC_UUID *uuid,
    BACNET_ERROR_CODE error,
    const char *error_desc)
{
    BSC_HUB_FUNCTION *f;
    BACNET_HOST_N_PORT_DATA peer;
    (void)ctx;

    bws_dispatch_lock();
    f = (BSC_HUB_FUNCTION *)c->ctx->user_arg;
    if (f->user_arg) {
        if (bsc_socket_get_peer_addr(c, &peer)) {
            bsc_node_store_failed_request_info(
                (BSC_NODE *)f->user_arg, &peer, vmac, uuid, error, error_desc);
        }
    }
    bws_dispatch_unlock();
}

static void hub_function_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE reason,
    const char *reason_desc,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    BSC_SOCKET *dst;
    BSC_SC_RET ret;
    int i;
    uint8_t **ppdu = &pdu;
    BSC_HUB_FUNCTION *f;

    bws_dispatch_lock();
    f = (BSC_HUB_FUNCTION *)c->ctx->user_arg;
    if (ev == BSC_SOCKET_EVENT_RECEIVED) {
        /* double check that received message does not contain */
        /* originating virtual address and contains dest vaddr */
        /* although such kind of check is already in bsc-socket.c */
        if (!decoded_pdu->hdr.origin && decoded_pdu->hdr.dest) {
            if (bvlc_sc_is_vmac_broadcast(decoded_pdu->hdr.dest)) {
                for (i = 0; i < sizeof(f->sock) / sizeof(BSC_SOCKET); i++) {
                    if (&f->sock[i] != c &&
                        f->sock[i].state == BSC_SOCK_STATE_CONNECTED) {
                        /* change origin address if presented or add origin */
                        /* address into pdu by extending of it's header */
                        pdu_len = bvlc_sc_set_orig(ppdu, pdu_len, &c->vmac);
                        ret = bsc_send(&f->sock[i], *ppdu, pdu_len);
                        (void)ret;
#if DEBUG_ENABLED == 1
                        if (ret != BSC_SC_SUCCESS) {
                            DEBUG_PRINTF("sending of reconstructed pdu failed, "
                                         "err = %d\n",
                                ret);
                        }
#endif
                    }
                }
            } else {
                dst = hub_function_find_connection_for_vmac(
                    decoded_pdu->hdr.dest, (void *)f);
                if (!dst) {
                    DEBUG_PRINTF("can not find socket, hub dropped pdu of size "
                                 "%d for dest vmac %s\n",
                        pdu_len, bsc_vmac_to_string(decoded_pdu->hdr.dest));
                } else {
                    bvlc_sc_remove_dest_set_orig(pdu, pdu_len, &c->vmac);
                    ret = bsc_send(dst, pdu, pdu_len);
                    (void)ret;
#if DEBUG_ENABLED == 1
                    if (ret != BSC_SC_SUCCESS) {
                        DEBUG_PRINTF(
                            "sending of pdu of %d bytes failed, err = %d\n",
                            pdu_len, ret);
                    }
#endif
                }
            }
        }
    } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
        hub_function_update_status(f, c, ev, reason, reason_desc);
        if (reason == ERROR_CODE_NODE_DUPLICATE_VMAC) {
            f->event_func(BSC_HUBF_EVENT_ERROR_DUPLICATED_VMAC,
                (BSC_HUB_FUNCTION_HANDLE)f, f->user_arg);
        }
    } else if (ev == BSC_SOCKET_EVENT_CONNECTED) {
        hub_function_update_status(f, c, ev, reason, reason_desc);
    }
    bws_dispatch_unlock();
}

static void hub_function_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev)
{
    BSC_HUB_FUNCTION *f;
    bws_dispatch_lock();
    f = (BSC_HUB_FUNCTION *)ctx->user_arg;
    if (ev == BSC_CTX_INITIALIZED) {
        f->state = BSC_HUB_FUNCTION_STATE_STARTED;
        f->event_func(
            BSC_HUBF_EVENT_STARTED, (BSC_HUB_FUNCTION_HANDLE)f, f->user_arg);
    } else if (ev == BSC_CTX_DEINITIALIZED) {
        f->state = BSC_HUB_FUNCTION_STATE_IDLE;
        hub_function_free(f);
        f->event_func(
            BSC_HUBF_EVENT_STOPPED, (BSC_HUB_FUNCTION_HANDLE)f, f->user_arg);
    }
    bws_dispatch_unlock();
}

BSC_SC_RET bsc_hub_function_start(uint8_t *ca_cert_chain,
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
    BSC_HUB_EVENT_FUNC event_func,
    void *user_arg,
    BSC_HUB_FUNCTION_HANDLE *h)
{
    BSC_SC_RET ret;
    BSC_HUB_FUNCTION *f;

    DEBUG_PRINTF("bsc_hub_function_start() >>>\n");

    if (!ca_cert_chain || !ca_cert_chain_size || !cert_chain ||
        !cert_chain_size || !key || !key_size || !local_uuid || !local_vmac ||
        !max_local_npdu_len || !max_local_bvlc_len || !connect_timeout_s ||
        !heartbeat_timeout_s || !disconnect_timeout_s || !event_func || !h) {
        DEBUG_PRINTF("bsc_hub_function_start() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    *h = NULL;

    bws_dispatch_lock();
    f = hub_function_alloc();

    if (!f) {
        bws_dispatch_unlock();
        DEBUG_PRINTF(
            "bsc_hub_function_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }

    f->user_arg = user_arg;
    f->event_func = event_func;

    bsc_init_ctx_cfg(BSC_SOCKET_CTX_ACCEPTOR, &f->cfg,
        BSC_WEBSOCKET_HUB_PROTOCOL, port, iface, ca_cert_chain,
        ca_cert_chain_size, cert_chain, cert_chain_size, key, key_size,
        local_uuid, local_vmac, max_local_bvlc_len, max_local_npdu_len,
        connect_timeout_s, heartbeat_timeout_s, disconnect_timeout_s);

    ret = bsc_init_ctx(&f->ctx, &f->cfg, &bsc_hub_function_ctx_funcs, f->sock,
        sizeof(f->sock) / sizeof(BSC_SOCKET), f);

    if (ret == BSC_SC_SUCCESS) {
        f->state = BSC_HUB_FUNCTION_STATE_STARTING;
        *h = (BSC_HUB_FUNCTION_HANDLE)f;
    } else {
        hub_function_free(f);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_hub_function_start() << ret = %d\n", ret);
    return ret;
}

void bsc_hub_function_stop(BSC_HUB_FUNCTION_HANDLE h)
{
    BSC_HUB_FUNCTION *f = (BSC_HUB_FUNCTION *)h;
    DEBUG_PRINTF("bsc_hub_function_stop() >>> h = %p\n", h);
    bws_dispatch_lock();
    if (f && f->state != BSC_HUB_FUNCTION_STATE_IDLE &&
        f->state != BSC_HUB_FUNCTION_STATE_STOPPING) {
        f->state = BSC_HUB_FUNCTION_STATE_STOPPING;
        bsc_deinit_ctx(&f->ctx);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_hub_function_stop() <<<\n");
}

bool bsc_hub_function_stopped(BSC_HUB_FUNCTION_HANDLE h)
{
    BSC_HUB_FUNCTION *f = (BSC_HUB_FUNCTION *)h;
    bool ret = false;

    DEBUG_PRINTF("bsc_hub_function_stopped() >>> h = %p\n", h);
    bws_dispatch_lock();
    if (f && f->state == BSC_HUB_FUNCTION_STATE_IDLE) {
        ret = true;
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_hub_function_stopped() <<< ret = %d\n", ret);
    return ret;
}

bool bsc_hub_function_started(BSC_HUB_FUNCTION_HANDLE h)
{
    BSC_HUB_FUNCTION *f = (BSC_HUB_FUNCTION *)h;
    bool ret = false;

    DEBUG_PRINTF("bsc_hub_function_started() >>> h = %p\n", h);
    bws_dispatch_lock();
    if (f && f->state == BSC_HUB_FUNCTION_STATE_STARTED) {
        ret = true;
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_hub_function_started() <<< ret = %d\n", ret);
    return ret;
}

BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *bsc_hub_function_status(
    BSC_HUB_FUNCTION_HANDLE h, size_t *cnt)
{
    BSC_HUB_FUNCTION *f = (BSC_HUB_FUNCTION *)h;
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *ret = NULL;

    bws_dispatch_lock();
    if (f && f->state == BSC_HUB_FUNCTION_STATE_STARTED) {
        ret = f->status;
        *cnt = BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM;
    }
    bws_dispatch_unlock();
    return ret;
}
