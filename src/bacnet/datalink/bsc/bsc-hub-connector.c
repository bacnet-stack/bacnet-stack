/**
 * @file
 * @brief BACnet hub connector API.
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date July 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-hub-connector.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/sc_netport.h"
#include "bacnet/bactext.h"

#define DEBUG_BSC_HUB_CONNECTOR 0

#undef DEBUG_PRINTF
#if DEBUG_BSC_HUB_CONNECTOR == 1
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_PRINTF
#define DEBUG_PRINTF debug_printf_disabled
#endif

typedef enum {
    BSC_HUB_CONN_PRIMARY = 0,
    BSC_HUB_CONN_FAILOVER = 1
} BSC_HUB_CONN_TYPE;

static void hub_connector_socket_event(
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE reason,
    const char *reason_desc,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu);

static void hub_connector_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev);

typedef enum {
    BSC_HUB_CONNECTOR_STATE_IDLE = 0,
    BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY = 1,
    BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER = 2,
    BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY = 3,
    BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER = 4,
    BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT = 5,
    BSC_HUB_CONNECTOR_STATE_WAIT_FOR_CTX_DEINIT = 6,
    BSC_HUB_CONNECTOR_STATE_DUPLICATED_VMAC = 7
} BSC_HUB_CONNECTOR_STATE;

typedef struct BSC_Hub_Connector {
    bool used;
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
    BACNET_SC_HUB_CONNECTION_STATUS primary_status;
    BACNET_SC_HUB_CONNECTION_STATUS failover_status;
} BSC_HUB_CONNECTOR;

#if BSC_CONF_HUB_CONNECTORS_NUM > 0
static BSC_HUB_CONNECTOR bsc_hub_connector[BSC_CONF_HUB_CONNECTORS_NUM] = { 0 };
#else
static BSC_HUB_CONNECTOR *bsc_hub_connector = NULL;
#endif

static BSC_SOCKET_CTX_FUNCS bsc_hub_connector_ctx_funcs = {
    NULL, NULL, hub_connector_socket_event, hub_connector_context_event, NULL
};

/**
 * @brief Initialize the BACnet hub connector status
 * @param s - pointer to the status structure
 */
static void hub_connector_reset_status(BACNET_SC_HUB_CONNECTION_STATUS *s)
{
    /* set timestamps to unspecified values */
    memset(&s->Connect_Timestamp, 0xFF, sizeof(s->Connect_Timestamp));
    memset(&s->Disconnect_Timestamp, 0xFF, sizeof(s->Disconnect_Timestamp));
    s->Error = ERROR_CODE_DEFAULT;
    s->Error_Details[0] = 0;
}

/**
 * @brief Allocate a hub connector
 * @return pointer to the allocated hub connector
 */
static BSC_HUB_CONNECTOR *hub_connector_alloc(void)
{
    int i;
    for (i = 0; i < BSC_CONF_HUB_CONNECTORS_NUM; i++) {
        if (!bsc_hub_connector[i].used) {
            memset(&bsc_hub_connector[i], 0, sizeof(bsc_hub_connector[i]));
            bsc_hub_connector[i].used = true;
            hub_connector_reset_status(&bsc_hub_connector[i].primary_status);
            hub_connector_reset_status(&bsc_hub_connector[i].failover_status);
            DEBUG_PRINTF(
                "hub_connector_alloc() ret = %p\n", &bsc_hub_connector[i]);
            return &bsc_hub_connector[i];
        }
    }
    DEBUG_PRINTF("hub_connector_alloc() ret = %p\n", &bsc_hub_connector[i]);
    return NULL;
}

/**
 * @brief Free a hub connector
 * @param c - pointer to the hub connector
 */
static void hub_connector_free(BSC_HUB_CONNECTOR *c)
{
    DEBUG_PRINTF("hub_connector_free() c = %p\n", c);
    c->used = false;
}

/**
 * @brief Update the BACnet hub connector status
 * @param s - pointer to the status structure
 * @param state - new state
 * @param err - error code
 * @param err_desc - error description
 */
static void hub_conector_update_status(
    BACNET_SC_HUB_CONNECTION_STATUS *s,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_ERROR_CODE err,
    const char *err_desc)
{
    s->State = state;
    if (state == BACNET_SC_CONNECTION_STATE_NOT_CONNECTED ||
        state == BACNET_SC_CONNECTION_STATE_DISCONNECTED_WITH_ERRORS) {
        bsc_set_timestamp(&s->Disconnect_Timestamp);
    } else if (
        state == BACNET_SC_CONNECTION_STATE_CONNECTED ||
        state == BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT) {
        bsc_set_timestamp(&s->Connect_Timestamp);
    }
    s->Error = err;
    s->Error_Details[0] = 0;
    if (err_desc) {
        bsc_copy_str(&s->Error_Details[0], err_desc, sizeof(s->Error_Details));
    }
}

/**
 * @brief Connect to a BACnet hub
 * @param p - pointer to the hub connector
 * @param type - connection type
 */
static void hub_connector_connect(BSC_HUB_CONNECTOR *p, BSC_HUB_CONN_TYPE type)
{
    BSC_SC_RET ret;
    char *url = (type == BSC_HUB_CONN_PRIMARY) ? (char *)p->primary_url
                                               : (char *)p->failover_url;

    p->state = (type == BSC_HUB_CONN_PRIMARY)
        ? BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY
        : BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER;

    DEBUG_PRINTF(
        "hub_connector_connect() hub = %p connecting to url %s\n", p, url);

    if (url[0] == 0) {
        mstimer_set(&p->t, p->reconnect_timeout_s * 1000);
        p->state = BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT;
        return;
    }

    ret = bsc_connect(&p->ctx, &p->sock[type], url);
    (void)ret;

#if DEBUG_ENABLED == 1
    if (ret != BSC_SC_SUCCESS) {
        DEBUG_PRINTF(
            "hub_connector_connect() got error while "
            "connecting to hub type %d, err = %d\n",
            type, ret);
    }
#endif
}

/**
 * @brief Process the hub connector state
 * @param c - pointer to the hub connector
 */
static void hub_connector_process_state(BSC_HUB_CONNECTOR *c)
{
    if (c->state == BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT) {
        if (mstimer_expired(&c->t)) {
            hub_connector_connect(c, BSC_HUB_CONN_PRIMARY);
        }
    }
}

/**
 * @brief Hub connector maintenance timer
 * @param seconds - number of elapsed seconds
 */
void bsc_hub_connector_maintenance_timer(uint16_t seconds)
{
    int i;
    (void)seconds;

    bws_dispatch_lock();
    for (i = 0; i < BSC_CONF_HUB_CONNECTORS_NUM; i++) {
        if (bsc_hub_connector[i].used) {
            hub_connector_process_state(&bsc_hub_connector[i]);
        }
    }
    bws_dispatch_unlock();
}

/**
 * @brief Hub connector socket event
 * @param c - pointer to the socket
 * @param ev - event
 * @param disconnect_reason - disconnect reason
 * @param disconnect_reason_desc - disconnect reason description
 * @param pdu - pointer to the PDU
 * @param pdu_len - PDU length
 * @param decoded_pdu - decoded PDU
 */
static void hub_connector_socket_event(
    BSC_SOCKET *c,
    BSC_SOCKET_EVENT ev,
    BACNET_ERROR_CODE disconnect_reason,
    const char *disconnect_reason_desc,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    BSC_HUB_CONNECTOR *hc;
    BACNET_SC_CONNECTION_STATE st =
        BACNET_SC_CONNECTION_STATE_DISCONNECTED_WITH_ERRORS;

    bws_dispatch_lock();
    hc = (BSC_HUB_CONNECTOR *)c->ctx->user_arg;
    DEBUG_PRINTF(
        "hub_connector_socket_event() >>> hub_connector = %p, socket "
        "= %p, ev = %d, reason = %d, reason_desc = %p,"
        "pdu = %p, pdu_len = %d\n",
        hc, c, ev, disconnect_reason, disconnect_reason_desc, pdu, pdu_len);
    DEBUG_PRINTF("hub_connector_socket_event() state = %d\n", hc->state);
    if (ev == BSC_SOCKET_EVENT_CONNECTED) {
        if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY) {
            DEBUG_PRINTF(
                "hub_connector_socket_event() hub_connector = %p "
                "connected primary\n",
                hc);
            hc->state = BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY;
            hub_conector_update_status(
                &hc->primary_status, BACNET_SC_CONNECTION_STATE_CONNECTED,
                ERROR_CODE_DEFAULT, NULL);
            hc->event_func(
                BSC_HUBC_EVENT_CONNECTED_PRIMARY, hc, hc->user_arg, NULL, 0,
                NULL);
        } else if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER) {
            DEBUG_PRINTF(
                "hub_connector_socket_event() hub_connector = %p "
                "connected failover\n",
                hc);
            hc->state = BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER;
            hub_conector_update_status(
                &hc->failover_status, BACNET_SC_CONNECTION_STATE_CONNECTED,
                ERROR_CODE_DEFAULT, NULL);
            hc->event_func(
                BSC_HUBC_EVENT_CONNECTED_FAILOVER, hc, hc->user_arg, NULL, 0,
                NULL);
        }
    } else if (ev == BSC_SOCKET_EVENT_DISCONNECTED) {
        if (disconnect_reason == ERROR_CODE_NODE_DUPLICATE_VMAC &&
            hc->state != BSC_HUB_CONNECTOR_STATE_WAIT_FOR_CTX_DEINIT) {
            DEBUG_PRINTF("hub_connector_socket_event() "
                         "got ERROR_CODE_NODE_DUPLICATE_VMAC error\n");
            if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY) {
                hub_conector_update_status(
                    &hc->primary_status,
                    BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT,
                    disconnect_reason, disconnect_reason_desc);
            } else if (
                hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER) {
                hub_conector_update_status(
                    &hc->failover_status,
                    BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT,
                    disconnect_reason, disconnect_reason_desc);
            }
            hc->state = BSC_HUB_CONNECTOR_STATE_DUPLICATED_VMAC;
            hc->event_func(
                BSC_HUBC_EVENT_ERROR_DUPLICATED_VMAC, hc, hc->user_arg, NULL, 0,
                NULL);
        } else if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY) {
            DEBUG_PRINTF("hub_connector_socket_event() try to connect to "
                         "failover hub\n");
            hub_conector_update_status(
                &hc->primary_status,
                BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT, disconnect_reason,
                disconnect_reason_desc);
            hub_connector_connect(hc, BSC_HUB_CONN_FAILOVER);
        } else if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER) {
            DEBUG_PRINTF(
                "hub_connector_socket_event() wait for %d seconds\n",
                hc->reconnect_timeout_s);
            hub_conector_update_status(
                &hc->failover_status,
                BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT, disconnect_reason,
                disconnect_reason_desc);
            hc->state = BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT;
            mstimer_set(&hc->t, hc->reconnect_timeout_s * 1000);
        } else if (
            hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY ||
            hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER) {
            if (disconnect_reason == ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER ||
                disconnect_reason == ERROR_CODE_SUCCESS) {
                st = BACNET_SC_CONNECTION_STATE_NOT_CONNECTED;
            }
            if (hc->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY) {
                hub_conector_update_status(
                    &hc->primary_status, st, ERROR_CODE_DEFAULT, NULL);
            } else {
                hub_conector_update_status(
                    &hc->failover_status, st, ERROR_CODE_DEFAULT, NULL);
            }
            DEBUG_PRINTF(
                "hub_connector_socket_event() try to connect to primary hub\n");
            hub_connector_connect(hc, BSC_HUB_CONN_PRIMARY);
        }
    } else if (ev == BSC_SOCKET_EVENT_RECEIVED) {
        DEBUG_PRINTF(
            "hub_connector_socket_event() hub_connector = %p pdu of "
            "%d len is received\n",
            hc, pdu_len);
        hc->event_func(
            BSC_HUBC_EVENT_RECEIVED, hc, hc->user_arg, pdu, pdu_len,
            decoded_pdu);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("hub_connector_socket_event() <<<\n");
}

/**
 * @brief Hub connector context event
 * @param ctx - pointer to the context
 * @param ev - event
 */
static void hub_connector_context_event(BSC_SOCKET_CTX *ctx, BSC_CTX_EVENT ev)
{
    BSC_HUB_CONNECTOR *c;

    DEBUG_PRINTF(
        "hub_connector_context_event() >>> ctx = %p, ev = %d\n", ctx, ev);

    if (ev == BSC_CTX_DEINITIALIZED) {
        bws_dispatch_lock();
        c = (BSC_HUB_CONNECTOR *)ctx->user_arg;
        if (c->state != BSC_HUB_CONNECTOR_STATE_IDLE) {
            c->state = BSC_HUB_CONNECTOR_STATE_IDLE;
            hub_connector_free(c);
            c->event_func(
                BSC_HUBC_EVENT_STOPPED, c, c->user_arg, NULL, 0, NULL);
        }
        bws_dispatch_unlock();
    }

    DEBUG_PRINTF("hub_connector_context_event() <<<\n");
}

/**
 * @brief Start a BACnet hub connector
 * @param ca_cert_chain - pointer to the CA certificate chain
 * @param ca_cert_chain_size - size of the CA certificate chain
 * @param cert_chain - pointer to the certificate chain
 * @param cert_chain_size - size of the certificate chain
 * @param key - pointer to the private key
 * @param key_size - size of the private key
 * @param local_uuid - pointer to the local UUID
 * @param local_vmac - pointer to the local VMAC address
 * @param max_local_bvlc_len - maximum BVLC message size
 * @param max_local_npdu_len - maximum NPDU message size
 * @param connect_timeout_s - connection timeout in seconds
 * @param heartbeat_timeout_s - heartbeat timeout in seconds
 * @param disconnect_timeout_s - disconnect timeout in seconds
 * @param primaryURL - primary hub URL
 * @param failoverURL - failover hub URL
 * @param reconnect_timeout_s - reconnect timeout in seconds
 * @param event_func - event function
 * @param user_arg - user argument
 * @param h - pointer to the hub connector handle
 * @return status
 */
BSC_SC_RET bsc_hub_connector_start(
    uint8_t *ca_cert_chain,
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

    DEBUG_PRINTF("bsc_hub_connector_start() >>>\n");

    if (!ca_cert_chain || !ca_cert_chain_size || !cert_chain ||
        !cert_chain_size || !key || !key_size || !local_uuid || !local_vmac ||
        !max_local_npdu_len || !max_local_bvlc_len || !connect_timeout_s ||
        !heartbeat_timeout_s || !disconnect_timeout_s || !primaryURL ||
        !reconnect_timeout_s || !event_func || !h) {
        DEBUG_PRINTF("bsc_hub_connector_start() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    if (strlen(primaryURL) > BSC_WSURL_MAX_LEN ||
        (failoverURL && (strlen(failoverURL) > BSC_WSURL_MAX_LEN))) {
        DEBUG_PRINTF("bsc_hub_connector_start() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    bws_dispatch_lock();
    c = hub_connector_alloc();
    if (!c) {
        bws_dispatch_unlock();
        DEBUG_PRINTF(
            "bsc_hub_connector_start() <<< ret = BSC_SC_NO_RESOURCES\n");
        return BSC_SC_NO_RESOURCES;
    }
    c->reconnect_timeout_s = reconnect_timeout_s;
    c->primary_url[0] = 0;
    c->failover_url[0] = 0;
    c->user_arg = user_arg;
    snprintf((char *)c->primary_url, sizeof(c->primary_url), "%s", primaryURL);
    if (failoverURL) {
        snprintf(
            (char *)c->failover_url, sizeof(c->failover_url), "%s",
            failoverURL);
    }
    c->event_func = event_func;
    bsc_init_ctx_cfg(
        BSC_SOCKET_CTX_INITIATOR, &c->cfg, BSC_WEBSOCKET_HUB_PROTOCOL, 0, NULL,
        ca_cert_chain, ca_cert_chain_size, cert_chain, cert_chain_size, key,
        key_size, local_uuid, local_vmac, max_local_bvlc_len,
        max_local_npdu_len, connect_timeout_s, heartbeat_timeout_s,
        disconnect_timeout_s);

    DEBUG_PRINTF(
        "bsc_hub_connector_start() uuid = %s, vmac = %s\n",
        bsc_uuid_to_string(&c->cfg.local_uuid),
        bsc_vmac_to_string(&c->cfg.local_vmac));

    ret = bsc_init_ctx(
        &c->ctx, &c->cfg, &bsc_hub_connector_ctx_funcs, c->sock,
        sizeof(c->sock) / sizeof(BSC_SOCKET), (void *)c);

    if (ret == BSC_SC_SUCCESS) {
        *h = (BSC_HUB_CONNECTOR_HANDLE)c;
        DEBUG_PRINTF(
            "bsc_hub_connector_start() hub = %p connecting to url %s\n", c,
            c->primary_url);
        ret = bsc_connect(
            &c->ctx, &c->sock[BSC_HUB_CONN_PRIMARY], (char *)c->primary_url);
        if (ret == BSC_SC_SUCCESS) {
            c->state = BSC_HUB_CONNECTOR_STATE_CONNECTING_PRIMARY;
        } else {
            if (c->failover_url[0] == 0) {
                c->state = BSC_HUB_CONNECTOR_STATE_IDLE;
                bsc_deinit_ctx(&c->ctx);
                hub_connector_free(c);
                *h = NULL;
            } else {
                c->state = BSC_HUB_CONNECTOR_STATE_CONNECTING_FAILOVER;
                DEBUG_PRINTF(
                    "bsc_hub_connector_start() hub = %p connecting to url %s\n",
                    c, c->primary_url);
                ret = bsc_connect(
                    &c->ctx, &c->sock[BSC_HUB_CONN_FAILOVER],
                    (char *)c->failover_url);
                if (ret != BSC_SC_SUCCESS) {
                    c->state = BSC_HUB_CONNECTOR_STATE_IDLE;
                    bsc_deinit_ctx(&c->ctx);
                    hub_connector_free(c);
                    *h = NULL;
                }
            }
        }
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_hub_connector_start() <<< ret = %d\n", ret);
    return ret;
}

/**
 * @brief Stop a BACnet hub connector
 * @param h - pointer to the hub connector
 */
void bsc_hub_connector_stop(BSC_HUB_CONNECTOR_HANDLE h)
{
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)h;
    DEBUG_PRINTF("bsc_hub_connector_stop() >>> h = %p\n", h);

    bws_dispatch_lock();

#if DEBUG_ENABLED == 1
    if (c) {
        DEBUG_PRINTF("bsc_hub_connector_stop() state = %d\n", c->state);
    }
#endif

    if (c && c->state != BSC_HUB_CONNECTOR_STATE_WAIT_FOR_CTX_DEINIT &&
        c->state != BSC_HUB_CONNECTOR_STATE_IDLE) {
        c->state = BSC_HUB_CONNECTOR_STATE_WAIT_FOR_CTX_DEINIT;
        bsc_deinit_ctx(&c->ctx);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_hub_connector_stop() <<<\n");
}

/**
 * @brief Send a BACnet hub connector PDU
 * @param h - pointer to the hub connector
 * @param pdu - pointer to the PDU
 * @param pdu_len - PDU length
 * @return status
 */
BSC_SC_RET
bsc_hub_connector_send(BSC_HUB_CONNECTOR_HANDLE h, uint8_t *pdu, size_t pdu_len)
{
    BSC_SC_RET ret;
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)h;

    DEBUG_PRINTF(
        "bsc_hub_connector_send() >>> h = %p,  pdu = %p, pdu_len = %d\n", h,
        pdu, pdu_len);

    bws_dispatch_lock();
    if (!c) {
        DEBUG_PRINTF("bsc_hub_connector_send() <<< ret = BSC_SC_BAD_PARAM\n");
        bws_dispatch_unlock();
        return BSC_SC_BAD_PARAM;
    }
    if (c->state == BSC_HUB_CONNECTOR_STATE_IDLE ||
        (c->state != BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY &&
         c->state != BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER)) {
        DEBUG_PRINTF(
            "bsc_hub_connector_send() pdu is dropped, state of "
            "hub_connector %p is %d\n",
            c, c->state);
        DEBUG_PRINTF(
            "bsc_hub_connector_send() <<< ret = BSC_SC_INVALID_OPERATION\n");
        bws_dispatch_unlock();
        return BSC_SC_INVALID_OPERATION;
    }
    if (c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY) {
        ret = bsc_send(&c->sock[BSC_HUB_CONN_PRIMARY], pdu, pdu_len);
    } else {
        ret = bsc_send(&c->sock[BSC_HUB_CONN_FAILOVER], pdu, pdu_len);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_hub_connector_send() <<< ret = %d\n", ret);
    return ret;
}

/**
 * @brief Check if the hub connector is stopped
 * @param h - pointer to the hub connector
 * @return status
 */
bool bsc_hub_connector_stopped(BSC_HUB_CONNECTOR_HANDLE h)
{
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)h;
    bool ret = false;

    DEBUG_PRINTF("bsc_hub_connector_stopped() >>> h = %p\n", h);
    bws_dispatch_lock();
    if (c && c->state == BSC_HUB_CONNECTOR_STATE_IDLE) {
        ret = true;
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_hub_connector_stopped() <<< ret = %d\n", ret);
    return ret;
}

/**
 * @brief Get the hub connector status
 * @param h - pointer to the hub connector
 * @param primary - primary or failover status
 * @return pointer to the status
 */
BACNET_SC_HUB_CONNECTION_STATUS *
bsc_hub_connector_status(BSC_HUB_CONNECTOR_HANDLE h, bool primary)
{
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)h;
    BACNET_SC_HUB_CONNECTION_STATUS *ret = NULL;
    bws_dispatch_lock();
    if (c) {
        if (primary) {
            ret = &c->primary_status;
        } else {
            ret = &c->failover_status;
        }
    }
    bws_dispatch_unlock();
    return ret;
}

/**
 * @brief Get the hub connector state
 * @param h - pointer to the hub connector
 * @return state
 */
BACNET_SC_HUB_CONNECTOR_STATE
bsc_hub_connector_state(BSC_HUB_CONNECTOR_HANDLE h)
{
    BSC_HUB_CONNECTOR *c = (BSC_HUB_CONNECTOR *)h;
    BACNET_SC_HUB_CONNECTOR_STATE ret =
        BACNET_SC_HUB_CONNECTOR_STATE_NO_HUB_CONNECTION;
    bws_dispatch_lock();
    if (c) {
        if (c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY) {
            ret = BACNET_SC_HUB_CONNECTOR_STATE_CONNECTED_TO_PRIMARY;
        } else if (c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER) {
            ret = BACNET_SC_HUB_CONNECTOR_STATE_CONNECTED_TO_FAILOVER;
        }
    }
    bws_dispatch_unlock();
    return ret;
}
