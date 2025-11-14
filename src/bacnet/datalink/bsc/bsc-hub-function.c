/**
 * @file
 * @brief BACNet hub function API.
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date July 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
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
#include "bacnet/bactext.h"
#undef DEBUG_PRINTF
#if DEBUG_BSC_HUB_FUNCTION
#define DEBUG_PRINTF debug_printf
#if (DEBUG_BSC_HUB_FUNCTION == 2)
#define DEBUG_PRINTF_VERBOSE debug_printf
#else
#define DEBUG_PRINTF_VERBOSE debug_printf_disabled
#endif
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF debug_printf_disabled
#define DEBUG_PRINTF_VERBOSE debug_printf_disabled
#endif

static BSC_SOCKET *hub_function_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg);

static BSC_SOCKET *
hub_function_find_connection_for_uuid(BACNET_SC_UUID *uuid, void *user_arg);

static void hub_function_failed_request(
    BSC_SOCKET_CTX *ctx,
    BSC_SOCKET *c,
    BACNET_SC_VMAC_ADDRESS *vmac,
    BACNET_SC_UUID *uuid,
    BACNET_ERROR_CODE error,
    const char *error_desc);

static void hub_function_socket_event(
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE reason,
    const char *reason_desc,
    uint8_t *pdu,
    size_t pdu_len,
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
} BSC_HUB_FUNCTION;

#if BSC_CONF_HUB_FUNCTIONS_NUM > 0
static BSC_HUB_FUNCTION bsc_hub_function[BSC_CONF_HUB_FUNCTIONS_NUM] = { 0 };
#else
static BSC_HUB_FUNCTION *bsc_hub_function = NULL;
#endif

static BSC_SOCKET_CTX_FUNCS bsc_hub_function_ctx_funcs = {
    hub_function_find_connection_for_vmac,
    hub_function_find_connection_for_uuid, hub_function_socket_event,
    hub_function_context_event, hub_function_failed_request
};

/**
 * @brief Allocate a hub function
 * @return pointer to the hub function
 */
static BSC_HUB_FUNCTION *hub_function_alloc(void)
{
    int i;
    for (i = 0; i < BSC_CONF_HUB_FUNCTIONS_NUM; i++) {
        if (!bsc_hub_function[i].used) {
            bsc_hub_function[i].used = true;
            return &bsc_hub_function[i];
        }
    }
    return NULL;
}

/**
 * @brief Free a hub function
 * @param p - pointer to the hub function
 */
static void hub_function_free(BSC_HUB_FUNCTION *p)
{
    p->used = false;
}

/**
 * @brief find a hub function connection for a specific VMAC address
 * @param vmac - pointer to the VMAC address
 * @param user_arg - pointer to the user argument
 * @return pointer to the socket, or NULL if not found
 */
static BSC_SOCKET *hub_function_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac, void *user_arg)
{
    int i;
    BSC_HUB_FUNCTION *f;

    bws_dispatch_lock();
    f = (BSC_HUB_FUNCTION *)user_arg;
    DEBUG_PRINTF(
        "hubf = %p local_vmac = %s\n", f,
        bsc_vmac_to_string(&f->cfg.local_vmac));
    for (i = 0; i < sizeof(f->sock) / sizeof(BSC_SOCKET); i++) {
        DEBUG_PRINTF(
            "hubf = %p, sock %p, state = %d, vmac = %s\n", f, &f->sock[i],
            f->sock[i].state, bsc_vmac_to_string(&f->sock[i].vmac));
        if (f->sock[i].state != BSC_SOCK_STATE_IDLE &&
            !memcmp(
                &vmac->address[0], &f->sock[i].vmac.address[0],
                sizeof(vmac->address))) {
            bws_dispatch_unlock();
            return &f->sock[i];
        }
    }
    bws_dispatch_unlock();
    return NULL;
}

/**
 * @brief find a hub function connection for a specific UUID
 * @param uuid - pointer to the UUID
 * @param user_arg - pointer to the user argument
 * @return pointer to the socket, or NULL if not found
 */
static BSC_SOCKET *
hub_function_find_connection_for_uuid(BACNET_SC_UUID *uuid, void *user_arg)
{
    int i;
    BSC_HUB_FUNCTION *f;

    bws_dispatch_lock();
    f = (BSC_HUB_FUNCTION *)user_arg;
    for (i = 0; i < sizeof(f->sock) / sizeof(BSC_SOCKET); i++) {
        DEBUG_PRINTF_VERBOSE(
            "hubf = %p, sock %p, state = %d, uuid = %s\n", f, &f->sock[i],
            f->sock[i].state, bsc_uuid_to_string(&f->sock[i].uuid));
        if (f->sock[i].state != BSC_SOCK_STATE_IDLE &&
            !memcmp(
                &uuid->uuid[0], &f->sock[i].uuid.uuid[0], sizeof(uuid->uuid))) {
            bws_dispatch_unlock();
            DEBUG_PRINTF_VERBOSE("found socket\n");
            return &f->sock[i];
        }
    }
    bws_dispatch_unlock();
    return NULL;
}

/**
 * @brief update the status of the hub function
 * @param f - pointer to the hub function
 * @param c - pointer to the socket
 * @param ev - event
 * @param disconnect_reason - disconnect reason
 * @param disconnect_reason_desc - disconnect reason description
 */
static void hub_function_update_status(
    BSC_HUB_FUNCTION *f,
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE disconnect_reason,
    const char *disconnect_reason_desc)
{
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *s;

    if (f->user_arg) {
        s = bsc_node_find_hub_status_for_vmac(f->user_arg, &c->vmac);

        if (s) {
            memcpy(s->Peer_VMAC, &c->vmac.address[0], BVLC_SC_VMAC_SIZE);
            memcpy(
                &s->Peer_UUID.uuid.uuid128[0], &c->uuid.uuid[0],
                BVLC_SC_UUID_SIZE);
            if (!bsc_socket_get_peer_addr(c, &s->Peer_Address)) {
                memset(&s->Peer_Address, 0, sizeof(s->Peer_Address));
            }
            if (disconnect_reason_desc) {
                bsc_copy_str(
                    s->Error_Details, disconnect_reason_desc,
                    sizeof(s->Error_Details));
            } else {
                s->Error_Details[0] = 0;
            }
            s->Error = ERROR_CODE_DEFAULT;
            if (ev == BSC_SOCKET_EVENT_CONNECTED) {
                s->State = BACNET_SC_CONNECTION_STATE_CONNECTED;
                bsc_set_timestamp(&s->Connect_Timestamp);
                memset(
                    &s->Disconnect_Timestamp, 0xff,
                    sizeof(s->Disconnect_Timestamp));
            } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
                bsc_set_timestamp(&s->Disconnect_Timestamp);
                if (disconnect_reason == ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER ||
                    disconnect_reason == ERROR_CODE_SUCCESS) {
                    s->State = BACNET_SC_CONNECTION_STATE_NOT_CONNECTED;
                } else {
                    s->State =
                        BACNET_SC_CONNECTION_STATE_DISCONNECTED_WITH_ERRORS;
                    s->Error = disconnect_reason;
                }
            }
        }
    }
}

/**
 * @brief Handle a hub function failed request
 * @param ctx - pointer to the socket context
 * @param c - pointer to the socket
 * @param vmac - pointer to the VMAC address
 * @param uuid - pointer to the UUID
 * @param error - error code
 * @param error_desc - pointer to the error description
 */
static void hub_function_failed_request(
    BSC_SOCKET_CTX *ctx,
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

/**
 * @brief Handle a hub function socket event
 * @param c - pointer to the socket
 * @param ev - event
 * @param reason - disconnect reason
 * @param reason_desc - disconnect reason description
 * @param pdu - pointer to the PDU
 * @param pdu_len - PDU length
 * @param decoded_pdu - decoded PDU
 */
static void hub_function_socket_event(
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE reason,
    const char *reason_desc,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    BSC_SOCKET *dst;
    BSC_SC_RET ret;
    int i;
    uint8_t *p_pdu;
    BSC_HUB_FUNCTION *f;
    size_t len;

    DEBUG_PRINTF_VERBOSE(
        "hub_function_socket_event() >>> c = %p, ev = %d, reason = "
        "%s, desc = %p, pdu = %p, pdu_len = %d, decoded_pdu = %p\n",
        c, ev, bactext_error_code_name(reason), reason_desc, pdu,
        pdu_len, decoded_pdu);

    bws_dispatch_lock();
    f = (BSC_HUB_FUNCTION *)c->ctx->user_arg;
    if (ev == BSC_SOCKET_EVENT_RECEIVED) {
        DEBUG_PRINTF(
            "BSC-HUB: socket event %s\n", bsc_socket_event_to_string(ev));
        /* double check that received message does not contain */
        /* originating virtual address and contains dest vaddr */
        /* although such kind of check is already in bsc-socket.c */
        if (!decoded_pdu->hdr.origin && decoded_pdu->hdr.dest) {
            if (bvlc_sc_is_vmac_broadcast(decoded_pdu->hdr.dest)) {
                if (bsc_socket_get_global_buf_size() >= pdu_len) {
                    p_pdu = bsc_socket_get_global_buf();
                    len = pdu_len;
                    memcpy(p_pdu, pdu, len);
                    for (i = 0; i < sizeof(f->sock) / sizeof(BSC_SOCKET); i++) {
                        if ((&f->sock[i] != c) &&
                            (f->sock[i].state == BSC_SOCK_STATE_CONNECTED)) {
                            /* change origin address if presented or add origin
                             */
                            /* address into pdu by extending of it's header */
                            len = (uint16_t)bvlc_sc_set_orig(
                                &p_pdu, len, &c->vmac);
                            ret = bsc_send(&f->sock[i], p_pdu, len);
                            if (ret == BSC_SC_SUCCESS) {
                                DEBUG_PRINTF(
                                    "BSC-HUB: "
                                    "sent pdu of %d bytes\n",
                                    len);
                            } else {
                                DEBUG_PRINTF(
                                    "BSC-HUB: "
                                    "sending of reconstructed pdu failed, "
                                    "err = %s\n",
                                    bsc_return_code_to_string(ret));
                            }
                        }
                    }
                } else {
                    DEBUG_PRINTF(
                        "BSC-HUB: received pdu with len = %d is dropped\n",
                        pdu_len);
                }
            } else {
                dst = hub_function_find_connection_for_vmac(
                    decoded_pdu->hdr.dest, (void *)f);
                if (!dst) {
                    DEBUG_PRINTF(
                        "BSC-HUB: cannot find socket. Hub dropped pdu of size "
                        "%d for dest vmac %s\n",
                        pdu_len, bsc_vmac_to_string(decoded_pdu->hdr.dest));
                } else {
                    bvlc_sc_remove_dest_set_orig(pdu, pdu_len, &c->vmac);
                    ret = bsc_send(dst, pdu, pdu_len);
                    if (ret == BSC_SC_SUCCESS) {
                        DEBUG_PRINTF(
                            "BSC-HUB: "
                            "sent pdu of %d bytes\n",
                            pdu_len);
                    } else {
                        DEBUG_PRINTF(
                            "BSC-HUB: sending of pdu of %d bytes failed."
                            " err = %s\n",
                            pdu_len, bsc_return_code_to_string(ret));
                    }
                }
            }
        }
    } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
        hub_function_update_status(f, c, ev, reason, reason_desc);
        if (reason == ERROR_CODE_NODE_DUPLICATE_VMAC) {
            f->event_func(
                BSC_HUBF_EVENT_ERROR_DUPLICATED_VMAC,
                (BSC_HUB_FUNCTION_HANDLE)f, f->user_arg);
        }
        if (reason != ERROR_CODE_SUCCESS) {
            DEBUG_PRINTF(
                "BSC-HUB: event=%s reason=%s \"%s\"\n",
                bsc_socket_event_to_string(ev), bactext_error_code_name(reason),
                reason_desc);
        }
    } else if (ev == BSC_SOCKET_EVENT_CONNECTED) {
        hub_function_update_status(f, c, ev, reason, reason_desc);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF_VERBOSE("hub_function_socket_event() <<<\n");
}

/**
 * @brief Handle a hub function context event
 * @param ctx - pointer to the socket context
 * @param ev - event
 */
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

/**
 * @brief Start a BACnet hub function
 * @param ca_cert_chain - pointer to the CA certificate chain
 * @param ca_cert_chain_size - size of the CA certificate chain
 * @param cert_chain - pointer to the certificate chain
 * @param cert_chain_size - size of the certificate chain
 * @param key - pointer to the private key
 * @param key_size - size of the private key
 * @param port - port number
 * @param iface - pointer to the interface
 * @param local_uuid - pointer to the local UUID
 * @param local_vmac - pointer to the local VMAC address
 * @param max_local_bvlc_len - maximum local BVLC length
 * @param max_local_npdu_len - maximum local NPDU length
 * @param connect_timeout_s - connection timeout in seconds
 * @param heartbeat_timeout_s - heartbeat timeout in seconds
 * @param disconnect_timeout_s - disconnect timeout in seconds
 * @param event_func - pointer to the event function
 * @param user_arg - pointer to the user argument
 * @param h - pointer to the hub function handle
 * @return BACnet/SC status
 */
BSC_SC_RET bsc_hub_function_start(
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
    unsigned int connect_timeout_s,
    unsigned int heartbeat_timeout_s,
    unsigned int disconnect_timeout_s,
    BSC_HUB_EVENT_FUNC event_func,
    void *user_arg,
    BSC_HUB_FUNCTION_HANDLE *h)
{
    BSC_SC_RET ret;
    BSC_HUB_FUNCTION *f;

    DEBUG_PRINTF_VERBOSE("bsc_hub_function_start() >>>\n");

    if (!ca_cert_chain || !ca_cert_chain_size || !cert_chain ||
        !cert_chain_size || !key || !key_size || !local_uuid || !local_vmac ||
        !max_local_npdu_len || !max_local_bvlc_len || !connect_timeout_s ||
        !heartbeat_timeout_s || !disconnect_timeout_s || !event_func || !h) {
        ret = BSC_SC_BAD_PARAM;
        DEBUG_PRINTF(
            "BSC-HUB: start failed. err=%s\n", bsc_return_code_to_string(ret));
        return ret;
    }
    *h = NULL;
    bws_dispatch_lock();
    f = hub_function_alloc();

    if (!f) {
        bws_dispatch_unlock();
        ret = BSC_SC_NO_RESOURCES;
        DEBUG_PRINTF(
            "BSC-HUB: alloc failed. err=%s\n", bsc_return_code_to_string(ret));
        return ret;
    }
    f->user_arg = user_arg;
    f->event_func = event_func;
    bsc_init_ctx_cfg(
        BSC_SOCKET_CTX_ACCEPTOR, &f->cfg, BSC_WEBSOCKET_HUB_PROTOCOL, port,
        iface, ca_cert_chain, ca_cert_chain_size, cert_chain, cert_chain_size,
        key, key_size, local_uuid, local_vmac, max_local_bvlc_len,
        max_local_npdu_len, connect_timeout_s, heartbeat_timeout_s,
        disconnect_timeout_s);

    ret = bsc_init_ctx(
        &f->ctx, &f->cfg, &bsc_hub_function_ctx_funcs, f->sock,
        sizeof(f->sock) / sizeof(BSC_SOCKET), f);

    if (ret == BSC_SC_SUCCESS) {
        f->state = BSC_HUB_FUNCTION_STATE_STARTING;
        *h = (BSC_HUB_FUNCTION_HANDLE)f;
    } else {
        DEBUG_PRINTF(
            "BSC-HUB: context initialization failed. err=%s\n",
            bsc_return_code_to_string(ret));
        hub_function_free(f);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF_VERBOSE("bsc_hub_function_start() << ret = %d\n", ret);
    return ret;
}

/**
 * @brief Stop a BACnet hub function
 * @param h - pointer to the hub function handle
 */
void bsc_hub_function_stop(BSC_HUB_FUNCTION_HANDLE h)
{
    BSC_HUB_FUNCTION *f = (BSC_HUB_FUNCTION *)h;
    DEBUG_PRINTF_VERBOSE("bsc_hub_function_stop() >>> h = %p\n", h);
    bws_dispatch_lock();
    if (f && f->state != BSC_HUB_FUNCTION_STATE_IDLE &&
        f->state != BSC_HUB_FUNCTION_STATE_STOPPING) {
        f->state = BSC_HUB_FUNCTION_STATE_STOPPING;
        bsc_deinit_ctx(&f->ctx);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF_VERBOSE("bsc_hub_function_stop() <<<\n");
}

/**
 * @brief Check if the hub function is stopped
 * @param h - pointer to the hub function handle
 * @return true if the hub function is stopped, false otherwise
 */
bool bsc_hub_function_stopped(BSC_HUB_FUNCTION_HANDLE h)
{
    BSC_HUB_FUNCTION *f = (BSC_HUB_FUNCTION *)h;
    bool ret = false;

    DEBUG_PRINTF_VERBOSE("bsc_hub_function_stopped() >>> h = %p\n", h);
    bws_dispatch_lock();
    if (f && f->state == BSC_HUB_FUNCTION_STATE_IDLE) {
        ret = true;
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF_VERBOSE(
        "bsc_hub_function_stopped() <<< ret = %s\n", ret ? "true" : "false");

    return ret;
}

/**
 * @brief Check if the hub function is started
 * @param h - pointer to the hub function handle
 * @return true if the hub function is started, false otherwise
 */
bool bsc_hub_function_started(BSC_HUB_FUNCTION_HANDLE h)
{
    BSC_HUB_FUNCTION *f = (BSC_HUB_FUNCTION *)h;
    bool ret = false;

    DEBUG_PRINTF_VERBOSE("bsc_hub_function_started() >>> h = %p\n", h);
    bws_dispatch_lock();
    if (f && f->state == BSC_HUB_FUNCTION_STATE_STARTED) {
        ret = true;
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF_VERBOSE(
        "bsc_hub_function_started() <<< ret = %s\n", ret ? "true" : "false");

    return ret;
}
