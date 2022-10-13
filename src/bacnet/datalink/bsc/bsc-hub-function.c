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
#include "bacnet/datalink/bsc/bsc-mutex.h"
#include "bacnet/datalink/bsc/bsc-runloop.h"
#include "bacnet/datalink/bsc/bsc-hub-function.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"

static BSC_SOCKET *hub_function_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac);
static BSC_SOCKET *hub_function_find_connection_for_uuid(BACNET_SC_UUID *uuid);
static void hub_function_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BSC_SC_RET err,
    uint8_t *pdu,
    uint16_t pdu_len);
static void hub_function_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);

typedef enum {
    BSC_HUB_FUNCTION_STATE_IDLE = 0,
    BSC_HUB_FUNCTION_STATE_STARTING = 1,
    BSC_HUB_FUNCTION_STATE_STARTED = 2,
    BSC_HUB_FUNCTION_STATE_STOPPING = 3
} BSC_HUB_FUNCTION_STATE;

typedef struct BSC_Hub_Connector {
    BSC_SOCKET_CTX ctx;
    BSC_CONTEXT_CFG cfg;
    BSC_SOCKET sock[BSC_CONF_HUB_FUNCTION_CONNECTIONS_NUM];
    BSC_HUB_FUNCTION_STATE state;
    HUB_FUNCTION_EVENT event_func;
    BSC_SC_RET error;
} BSC_HUB_FUNCTION;

static BSC_HUB_FUNCTION bsc_hub_function = { 0 };

static BSC_SOCKET_CTX_FUNCS bsc_hub_function_ctx_funcs = {
    hub_function_find_connection_for_vmac,
    hub_function_find_connection_for_uuid, hub_function_socket_event,
    hub_function_context_event
};

static BSC_SOCKET *hub_function_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac)
{
    int i;
    bsc_global_mutex_lock();
    for (i = 0; i < sizeof(bsc_hub_function.sock) / sizeof(BSC_SOCKET); i++) {
        if (bsc_hub_function.sock[i].state == BSC_SOCK_STATE_CONNECTED &&
            !memcmp(&vmac->address[0],
                &bsc_hub_function.sock[i].vmac->address[0],
                sizeof(vmac->address))) {
            bsc_global_mutex_unlock();
            return &bsc_hub_function.sock[i];
        }
    }
    bsc_global_mutex_unlock();
    return NULL;
}

static BSC_SOCKET *hub_function_find_connection_for_uuid(BACNET_SC_UUID *uuid)
{
    int i;
    bsc_global_mutex_lock();
    for (i = 0; i < sizeof(bsc_hub_function.sock) / sizeof(BSC_SOCKET); i++) {
        if (bsc_hub_function.sock[i].state == BSC_SOCK_STATE_CONNECTED &&
            !memcmp(&uuid->uuid[0], &bsc_hub_function.sock[i].uuid->uuid[0],
                sizeof(uuid->uuid))) {
            bsc_global_mutex_unlock();
            return &bsc_hub_function.sock[i];
        }
    }
    bsc_global_mutex_unlock();
    return NULL;
}

static void hub_function_socket_event(BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BSC_SC_RET err,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    unsigned int len;
    BSC_SOCKET *dst;
    BSC_SC_RET ret;
    int i;
    uint8_t *p = (uint8_t *)decoded_pdu;
    bsc_global_mutex_lock();
    if (ev == BSC_SOCKET_EVENT_RECEIVED) {
        // double check that received message does not contain
        // originating virtual address and contains dest vaddr
        // although such kind of check is already i bsc-socket.c
        if (!decoded_pdu->hdr.origin && decoded_pdu->hdr.dest) {
            if (bsc_is_vmac_broadcast(decoded_pdu->hdr.dest)) {
                for (i = 0;
                     i < sizeof(bsc_hub_function.sock) / sizeof(BSC_SOCKET);
                     i++) {
                    if (&bsc_hub_function.sock[i] != c) {
                        // decoded_pdu is used as a buffer for extending of the
                        // header of pdu copy common bvlc header
                        memcpy(p, pdu, 4);
                        // adding originating virtual address
                        p[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
                        memcpy(&p[4], c->vmac, sizeof(c->vmac));
                        // send data as 2 buffers, first contains modified
                        // common hdr + orig vaddr,the second other part
                        // this allows us to reduce copying operations
                        ret = bsc_send2(&bsc_hub_function.sock[i], p,
                            4 + sizeof(c->vmac), &pdu[4], pdu_len - 4);
                        if (ret != BSC_SC_SUCCESS) {
                            debug_printf("sending of reconstructed pdu failed, "
                                         "err = %d\n",
                                ret);
                        }
                    }
                }
            } else {
                dst = hub_function_find_connection_for_vmac(
                    decoded_pdu->hdr.dest);
                if (!dst) {
                    debug_printf("can not find socket, hub dropped pdu of size "
                                 "%d for dest vmac %s\n",
                        pdu_len, bsc_vmac_to_string(decoded_pdu->hdr.dest));
                } else {
                    bvlc_sc_remove_dest_set_orig(pdu, pdu_len, c->vmac);
                    ret = bsc_send(dst, pdu, pdu_len);
                    if (ret != BSC_SC_SUCCESS) {
                        debug_printf(
                            "sending of pdu of %d bytes failed, err = %d\n",
                            pdu_len, ret);
                    }
                }
            }
        }
    } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED &&
        err == BSC_SC_DUPLICATED_VMAC) {
        bsc_hub_function.state = BSC_HUB_FUNCTION_STATE_STOPPING;
        bsc_hub_function.error = BSC_SC_DUPLICATED_VMAC;
        bsc_deinit_ctx(&bsc_hub_function.ctx);
    }
    bsc_global_mutex_unlock();
}

static void hub_connector_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev)
{
    bsc_global_mutex_lock();
    if (ev == BSC_CTX_INITIALIZED) {
        bsc_hub_function.state = BSC_HUB_FUNCTION_STATE_STARTED;
        bsc_hub_function.event_func(BSC_HUBF_EVENT_STARTED, BSC_SC_SUCCESS);
    } else if (ev == BSC_CTX_DEINITIALIZED) {
        bsc_hub_function.state = BSC_HUB_FUNCTION_STATE_IDLE;
        bsc_hub_function.event_func(
            BSC_HUBF_EVENT_STOPPED, bsc_hub_function.error);
    }
    bsc_global_mutex_unlock();
}

BACNET_STACK_EXPORT
BSC_SC_RET bsc_hub_function_start(uint8_t *ca_cert_chain,
    size_t ca_cert_chain_size,
    uint8_t *cert_chain,
    size_t cert_chain_size,
    uint8_t *key,
    size_t key_size,
    int port,
    BACNET_SC_UUID *local_uuid,
    BACNET_SC_VMAC_ADDRESS *local_vmac,
    uint16_t max_local_bvlc_len,
    uint16_t max_local_ndpu_len,
    unsigned int connect_timeout_s,
    unsigned int heartbeat_timeout_s,
    unsigned int disconnect_timeout_s,
    HUB_FUNCTION_EVENT event_func)
{
    BSC_SC_RET ret;
    debug_printf("bsc_hub_function_start() >>>\n");

    if (!ca_cert_chain || !ca_cert_chain_size || !cert_chain ||
        !cert_chain_size || !key || !key_size || !local_uuid || !local_vmac ||
        !max_local_npdu_len || !max_local_bvlc_len || !connect_timeout_s ||
        !heartbeat_timeout_s || !disconnect_timeout_s || !event_func) {
        debug_printf("bsc_hub_function_start() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    bsc_global_mutex_lock();
    if (bsc_hub_function.state != BSC_HUB_FUNCTION_STATE_IDLE) {
        bsc_global_mutex_unlock();
        debug_printf(
            "bsc_hub_function_start() <<< ret = BSC_SC_INVALID_OPERATION\n");
        return BSC_SC_INVALID_OPERATION;
    }

    bsc_hub_function.error = BSC_SC_SUCCESS;

    bsc_init_ctx_cfg(BSC_SOCKET_CTX_ACCEPTOR, &bsc_hub_function.cfg,
        BSC_WEBSOCKET_HUB_PROTOCOL, port, ca_cert_chain, ca_cert_chain_size,
        cert_chain, cert_chain_size, key, key_size, local_uuid, local_vmac,
        max_local_bvlc_len, max_local_ndpu_len, connect_timeout_s,
        heartbeat_timeout_s, disconnect_timeout_s);

    ret = bsc_init_сtx(&bsc_hub_function.ctx, &bsc_hub_function.cfg,
        &bsc_hub_function_ctx_funcs, bsc_hub_function.sock,
        sizeof(bsc_hub_function.sock) / sizeof(BSC_SOCKET));

    if (ret == BSC_SC_SUCCESS) {
        bsc_hub_function.state = BSC_HUB_FUNCTION_STATE_STARTING;
    }

    bsc_global_mutex_unlock();
    debug_printf("bsc_hub_function_start() << ret = %d\n", ret);
}

BACNET_STACK_EXPORT
void bsc_hub_function_stop(void)
{
    debug_printf("bsc_hub_function_stop() >>>\n");
    bsc_global_mutex_lock();
    if (bsc_hub_connector.state != BSC_HUB_FUNCTION_STATE_IDLE &&
        bsc_hub_connector.state != BSC_HUB_FUNCTION_STATE_STOPPING) {
        bsc_hub_connector.state = BSC_HUB_CONNECTOR_STATE_WAIT_FOR_CTX_STOPPING;
        bsc_deinit_ctx(&bsc_hub_function.ctx);
    }
    bsc_global_mutex_unlock();
    debug_printf("bsc_hub_functionn_stop() <<<\n");
}