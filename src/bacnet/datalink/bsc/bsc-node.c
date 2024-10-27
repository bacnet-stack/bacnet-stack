/**
 * @file
 * @brief BACNet secure connect node API.
 * @author Kirill Neznamov
 * @date October 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bsc-node.h"
#include "bacnet/datalink/bsc/bsc-event.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-hub-function.h"
#include "bacnet/datalink/bsc/bsc-hub-connector.h"
#include "bacnet/datalink/bsc/bsc-node-switch.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/basic/object/sc_netport.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"

#define DEBUG_BSC_NODE 0

#if DEBUG_BSC_NODE == 1
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF debug_printf_disabled
#endif

#define ERROR_STR_OPTION_NOT_UNDERSTOOD \
    "'must understand' option not understood "

#define ERROR_STR_DIRECT_CONNECTIONS_NOT_SUPPORTED \
    "direct connections are not supported"

typedef enum {
    BSC_NODE_STATE_IDLE = 0,
    BSC_NODE_STATE_STARTING = 1,
    BSC_NODE_STATE_STARTED = 2,
    BSC_NODE_STATE_RESTARTING = 3,
    BSC_NODE_STATE_STOPPING = 4
} BSC_NODE_STATE;

struct BSC_Node {
    bool used;
    BSC_NODE_STATE state;
    BSC_NODE_CONF *conf;
    BSC_ADDRESS_RESOLUTION *resolution;
    BSC_HUB_CONNECTOR_HANDLE hub_connector;
    BSC_HUB_FUNCTION_HANDLE hub_function;
    BSC_NODE_SWITCH_HANDLE node_switch;
    BACNET_SC_FAILED_CONNECTION_REQUEST *failed;
    BACNET_SC_DIRECT_CONNECTION_STATUS *direct_status;
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *hub_status;
};

#if BSC_CONF_NODES_NUM < 1
#error "BSC_CONF_NODES_NUM must be >= 1"
#endif

static struct BSC_Node bsc_node[BSC_CONF_NODES_NUM] = { 0 };

static BACNET_SC_FAILED_CONNECTION_REQUEST
    bsc_failed_request[BSC_CONF_NODES_NUM]
                      [BSC_CONF_FAILED_CONNECTION_STATUS_MAX_NUM];
static bool bsc_failed_request_initialized[BSC_CONF_NODES_NUM] = { 0 };

static BACNET_SC_DIRECT_CONNECTION_STATUS
    bsc_direct_status[BSC_CONF_NODES_NUM]
                     [BSC_CONF_NODE_SWITCH_CONNECTION_STATUS_MAX_NUM];
static bool bsc_direct_status_initialized[BSC_CONF_NODES_NUM] = { 0 };

static BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS
    bsc_hub_status[BSC_CONF_NODES_NUM]
                  [BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM];
static bool bsc_hub_status_initialized[BSC_CONF_NODES_NUM] = { 0 };

static BSC_ADDRESS_RESOLUTION
    bsc_address_resolution[BSC_CONF_NODES_NUM]
                          [BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM];

static BSC_NODE_CONF bsc_conf[BSC_CONF_NODES_NUM];

static BSC_SC_RET bsc_node_start_state(BSC_NODE *node, BSC_NODE_STATE state);

static void bsc_node_init_direct_status(BACNET_SC_DIRECT_CONNECTION_STATUS *s)
{
    int j;

    for (j = 0; j < BSC_CONF_NODE_SWITCH_CONNECTION_STATUS_MAX_NUM; j++) {
        memset(&s[j], 0, sizeof(*s));
        memset(&s[j].Connect_Timestamp, 0xFF, sizeof(s[j].Connect_Timestamp));
        memset(
            &s[j].Disconnect_Timestamp, 0xFF,
            sizeof(s[j].Disconnect_Timestamp));
    }
}

static void
bsc_node_init_hub_status(BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *s)
{
    int j;

    for (j = 0; j < BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM; j++) {
        memset(&s[j], 0, sizeof(*s));
        memset(&s[j].Connect_Timestamp, 0xFF, sizeof(s[j].Connect_Timestamp));
        memset(
            &s[j].Disconnect_Timestamp, 0xFF,
            sizeof(s[j].Disconnect_Timestamp));
    }
}

static BSC_NODE *bsc_alloc_node(void)
{
    int i, j;
    DEBUG_PRINTF("bsc_alloc_node() >>> \n");

    for (i = 0; i < BSC_CONF_NODES_NUM; i++) {
        if (bsc_node[i].used == false) {
            memset(&bsc_node[i], 0, sizeof(bsc_node[i]));
            bsc_node[i].used = true;
            bsc_node[i].hub_status = (BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS
                                          *)&bsc_hub_status[i][0];
            bsc_node[i].direct_status =
                (BACNET_SC_DIRECT_CONNECTION_STATUS *)&bsc_direct_status[i][0];

            /* Start/stop cycles of a node must not make an influence to history
               That's why hub and direct status arrays are initialized
               only once */

            if (!bsc_hub_status_initialized[i]) {
                bsc_node_init_hub_status(bsc_node[i].hub_status);
            }

            if (!bsc_direct_status_initialized[i]) {
                bsc_node_init_direct_status(bsc_node[i].direct_status);
            }

            bsc_node[i].conf = &bsc_conf[i];
            bsc_node[i].resolution = &bsc_address_resolution[i][0];
            bsc_node[i].failed = &bsc_failed_request[i][0];
            memset(
                bsc_node[i].resolution, 0,
                sizeof(BSC_ADDRESS_RESOLUTION) *
                    BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM);

            /* Start/stop cycles of a node must not make an influence to history
             * about failed requests */
            /* That's why bsc_failed_request[] array is initialized only once */

            if (!bsc_failed_request_initialized[i]) {
                for (j = 0; j < BSC_CONF_FAILED_CONNECTION_STATUS_MAX_NUM;
                     j++) {
                    memset(
                        &bsc_failed_request[i][j], 0,
                        sizeof(bsc_failed_request[i][j]));
                    memset(
                        &bsc_failed_request[i][j].Timestamp, 0xff,
                        sizeof(bsc_failed_request[i][j].Timestamp));
                }
                bsc_failed_request_initialized[i] = true;
            }
            DEBUG_PRINTF(
                "bsc_alloc_node() <<< i = %d, node = %p, conf = %p\n", i,
                &bsc_node[i], bsc_node[i].conf);
            return &bsc_node[i];
        }
    }
    DEBUG_PRINTF("bsc_alloc_node() <<< ret = NULL\n");
    return NULL;
}

static bool node_switch_enabled(BSC_NODE_CONF *conf)
{
    if (conf->direct_connect_initiate_enable ||
        conf->direct_connect_accept_enable) {
        return true;
    }
    return false;
}

static BSC_ADDRESS_RESOLUTION *
node_get_address_resolution(BSC_NODE *node, BACNET_SC_VMAC_ADDRESS *vmac)
{
    int i;

    for (i = 0; i < BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM; i++) {
        if (node->resolution[i].used &&
            !memcmp(
                &vmac->address[0], &node->resolution[i].vmac.address[0],
                BVLC_SC_VMAC_SIZE)) {
            return &node->resolution[i];
        }
    }
    return NULL;
}

static void node_free_address_resolution(BSC_ADDRESS_RESOLUTION *r)
{
    r->used = false;
    r->urls_num = false;
}

static BSC_ADDRESS_RESOLUTION *
node_alloc_address_resolution(BSC_NODE *node, BACNET_SC_VMAC_ADDRESS *vmac)
{
    int i;
    unsigned long max = 0;
    int max_index = 0;

    for (i = 0; i < BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM; i++) {
        if (!node->resolution[i].used) {
            node->resolution[i].used = true;
            mstimer_set(
                &node->resolution[i].fresh_timer,
                node->conf->address_resolution_freshness_timeout_s * 1000);
            memcpy(
                &node->resolution[i].vmac.address[0], &vmac->address[0],
                BVLC_SC_VMAC_SIZE);
            return &node->resolution[i];
        }
    }

    /* find and remove oldest resolution */

    for (i = 0; i < BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM; i++) {
        if (mstimer_elapsed(&node->resolution[i].fresh_timer) > max) {
            max = mstimer_elapsed(&node->resolution[i].fresh_timer);
            max_index = i;
        }
    }

    node->resolution[max_index].urls_num = 0;
    return &node->resolution[max_index];
}

static void bsc_free_node(BSC_NODE *node)
{
    DEBUG_PRINTF(
        "bsc_free_node() >>> node = %p, state = %d\n", node, node->state);
    node->used = false;
    DEBUG_PRINTF("bsc_free_node() <<<\n");
}

static void bsc_node_process_stop_event(BSC_NODE *node)
{
    bool stopped = true;

    DEBUG_PRINTF(
        "bsc_node_process_stop_event() >>> node = %p, state = %d\n", node,
        node->state);

    if (node->conf->hub_function_enabled) {
        if (node->hub_function &&
            !bsc_hub_function_stopped(node->hub_function)) {
            DEBUG_PRINTF(
                "bsc_node_process_stop_event() hub_function %p is not "
                "stopped\n",
                node->hub_function);
            stopped = false;
        }
    }
    if (node->node_switch && node_switch_enabled(node->conf)) {
        if (!bsc_node_switch_stopped(node->node_switch)) {
            DEBUG_PRINTF(
                "bsc_node_process_stop_event() node_switch %p is not stopped\n",
                node->node_switch);
            stopped = false;
        }
    }
    if (node->hub_connector &&
        !bsc_hub_connector_stopped(node->hub_connector)) {
        DEBUG_PRINTF(
            "bsc_node_process_stop_event() hub_connector %p is not stopped\n",
            node->hub_connector);
        stopped = false;
    }

    DEBUG_PRINTF("bsc_node_process_stop_event() stopped = %d\n", stopped);

    if (node->state == BSC_NODE_STATE_STOPPING) {
        if (stopped) {
            node->state = BSC_NODE_STATE_IDLE;
            DEBUG_PRINTF("bsc_node_process_stop_event() emit stop event\n");
            node->conf->event_func(node, BSC_NODE_EVENT_STOPPED, NULL, NULL, 0);
        }
    } else if (node->state == BSC_NODE_STATE_RESTARTING) {
        if (stopped) {
            DEBUG_PRINTF("bsc_node_process_stop_event() emit restart event\n");
            (void)bsc_node_start_state(node, BSC_NODE_STATE_RESTARTING);
        }
    }
    DEBUG_PRINTF("bsc_node_process_stop_event() <<<\n");
}

static void bsc_node_process_start_event(BSC_NODE *node)
{
    bool started = true;

    DEBUG_PRINTF(
        "bsc_node_process_start_event() >>> node = %p, state = %d\n", node,
        node->state);
    if (node->hub_function && node->conf->hub_function_enabled) {
        if (!bsc_hub_function_started(node->hub_function)) {
            started = false;
        }
    }
    if (node->node_switch && node_switch_enabled(node->conf)) {
        if (!bsc_node_switch_started(node->node_switch)) {
            started = false;
        }
    }
    DEBUG_PRINTF("bsc_node_process_start_event() started = %d\n", started);
    if (started) {
        if (node->state == BSC_NODE_STATE_STARTING) {
            node->state = BSC_NODE_STATE_STARTED;
            node->conf->event_func(node, BSC_NODE_EVENT_STARTED, NULL, NULL, 0);
        } else if (node->state == BSC_NODE_STATE_RESTARTING) {
            node->state = BSC_NODE_STATE_STARTED;
            node->conf->event_func(
                node, BSC_NODE_EVENT_RESTARTED, NULL, NULL, 0);
        }
    }
    DEBUG_PRINTF("bsc_node_process_start_event() <<<\n");
}

static void bsc_node_restart(BSC_NODE *node)
{
    DEBUG_PRINTF(
        "bsc_node_restart() >>> node = %p hub_function %p "
        "hub_connector %p node_switch %p\n",
        node, node->hub_function, node->hub_connector, node->node_switch);
    node->state = BSC_NODE_STATE_RESTARTING;
    if (node->conf->primaryURL) {
        bsc_hub_connector_stop(node->hub_connector);
    }
    if (node->hub_function && node->conf->hub_function_enabled) {
        bsc_hub_function_stop(node->hub_function);
    }
    if (node_switch_enabled(node->conf)) {
        bsc_node_switch_stop(node->node_switch);
    }
    DEBUG_PRINTF("bsc_node_restart() <<<\n");
}

static void bsc_node_parse_urls(
    BSC_ADDRESS_RESOLUTION *r, BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    int i, j;
    int start;
    uint8_t *url = NULL;

    url = decoded_pdu->payload.address_resolution_ack.utf8_websocket_uri_string;
    r->urls_num = 0;
    for (i = 0, j = 0, start = 0;
         i < decoded_pdu->payload.address_resolution_ack
                 .utf8_websocket_uri_string_len;
         i++) {
        if (url[i] == 0x20) {
            if (i > BSC_CONF_NODE_MAX_URI_SIZE_IN_ADDRESS_RESOLUTION_ACK ||
                (i - start) == 0) {
                start = i + 1;
                continue;
            } else {
                memcpy(&r->utf8_urls[j][0], &url[start], i - start);
                r->utf8_urls[j][i] = 0;
                j++;
                start = i + 1;
            }
        }
    }
    if (i - start > 0 &&
        i <= BSC_CONF_NODE_MAX_URI_SIZE_IN_ADDRESS_RESOLUTION_ACK) {
        memcpy(&r->utf8_urls[j][0], &url[start], i - start);
        r->utf8_urls[j][i] = 0;
        j++;
    }
    r->urls_num = j;
}

static void bsc_node_process_received(
    BSC_NODE *node,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    int i;
    static uint8_t buf[BVLC_SC_NPDU_SIZE_CONF];
    size_t bufsize;
    BSC_SC_RET ret;
    uint16_t error_class;
    uint16_t error_code;
    BSC_ADDRESS_RESOLUTION *r;

    (void)ret;
    DEBUG_PRINTF(
        "bsc_node_process_received() >>> node = %p, pdu = %p, pdu_len "
        "= %d, decoded_pdu = %p\n",
        node, pdu, pdu_len, decoded_pdu);

    for (i = 0; i < decoded_pdu->hdr.dest_options_num; i++) {
        if (decoded_pdu->dest_options[i].must_understand) {
            DEBUG_PRINTF("bsc_node_process_received() pdu with "
                         "'must-understand' is dropped\n");
            if (bvlc_sc_need_send_bvlc_result(decoded_pdu)) {
                error_code = ERROR_CODE_HEADER_NOT_UNDERSTOOD;
                error_class = ERROR_CLASS_COMMUNICATION;
                bufsize = bvlc_sc_encode_result(
                    buf, sizeof(buf), decoded_pdu->hdr.message_id, NULL,
                    decoded_pdu->hdr.origin, decoded_pdu->hdr.bvlc_function, 1,
                    &decoded_pdu->dest_options[i].packed_header_marker,
                    &error_class, &error_code,
                    (uint8_t *)ERROR_STR_OPTION_NOT_UNDERSTOOD);
                if (bufsize) {
                    ret = bsc_node_send(node, buf, bufsize);
#if DEBUG_ENABLED == 1
                    if (ret != BSC_SC_SUCCESS) {
                        DEBUG_PRINTF(
                            "bsc_node_process_received() warning "
                            "bvlc-result pdu is not sent, error %d\n",
                            ret);
                    }
#endif
                }
                DEBUG_PRINTF("bsc_node_process_received() <<<\n");
                return;
            } else {
                DEBUG_PRINTF("bsc_node_process_received() <<<\n");
                return;
            }
        }
    }

    switch (decoded_pdu->hdr.bvlc_function) {
        case BVLC_SC_RESULT: {
            if (decoded_pdu->payload.result.bvlc_function ==
                BVLC_SC_ADDRESS_RESOLUTION) {
                DEBUG_PRINTF(
                    "received a NAK for address resolution from %s\n",
                    bsc_vmac_to_string(decoded_pdu->hdr.origin));
                r = node_get_address_resolution(node, decoded_pdu->hdr.origin);
                if (r) {
                    r->urls_num = 0;
                    mstimer_restart(&r->fresh_timer);
                } else {
                    r = node_alloc_address_resolution(
                        node, decoded_pdu->hdr.origin);
                    if (r) {
                        r->urls_num = 0;
                        mstimer_restart(&r->fresh_timer);
                    } else {
                        DEBUG_PRINTF(
                            "can't allocate address resolution for "
                            "node with address %s\n",
                            bsc_vmac_to_string(decoded_pdu->hdr.origin));
                    }
                }
            }
            DEBUG_PRINTF(
                "node %p get pdu with bvlc "
                "function %d error_class %d error_code %d from node %s\n",
                node, decoded_pdu->payload.result.bvlc_function,
                decoded_pdu->payload.result.error_class,
                decoded_pdu->payload.result.error_code,
                bsc_vmac_to_string(decoded_pdu->hdr.origin));
            node->conf->event_func(
                node, BSC_NODE_EVENT_RECEIVED_RESULT, NULL, pdu, pdu_len);
            break;
        }
        case BVLC_SC_ADVERTISIMENT: {
            node->conf->event_func(
                node, BSC_NODE_EVENT_RECEIVED_ADVERTISIMENT, NULL, pdu,
                pdu_len);
            break;
        }
        case BVLC_SC_ADVERTISIMENT_SOLICITATION: {
            bufsize = bvlc_sc_encode_advertisiment(
                buf, sizeof(buf), bsc_get_next_message_id(), NULL,
                decoded_pdu->hdr.origin,
                bsc_hub_connector_state(node->hub_connector),
                node_switch_enabled(node->conf)
                    ? BVLC_SC_DIRECT_CONNECTION_ACCEPT_SUPPORTED
                    : BVLC_SC_DIRECT_CONNECTION_ACCEPT_UNSUPPORTED,
                node->conf->max_local_bvlc_len, node->conf->max_local_npdu_len);
            if (bufsize) {
                ret = bsc_node_send(node, buf, bufsize);
#if DEBUG_ENABLED == 1
                if (ret != BSC_SC_SUCCESS) {
                    DEBUG_PRINTF(
                        "bsc_node_process_received() warning "
                        "advertisement pdu is not sent to node %s, error %d\n",
                        ret, bsc_vmac_to_string(decoded_pdu->hdr.origin));
                }
#endif
            }
            break;
        }
        case BVLC_SC_ADDRESS_RESOLUTION: {
            DEBUG_PRINTF(
                "bsc_node_process_received() got BVLC_SC_ADDRESS_RESOLUTION\n");
            if (node_switch_enabled(node->conf)) {
                bufsize = bvlc_sc_encode_address_resolution_ack(
                    buf, sizeof(buf), decoded_pdu->hdr.message_id, NULL,
                    decoded_pdu->hdr.origin,
                    (uint8_t *)node->conf->direct_connection_accept_uris,
                    node->conf->direct_connection_accept_uris_len);
                if (bufsize) {
                    ret = bsc_node_send(node, buf, bufsize);
#if DEBUG_ENABLED == 1
                    if (ret != BSC_SC_SUCCESS) {
                        DEBUG_PRINTF(
                            "bsc_node_process_received() warning "
                            "address resolution ack is not sent, error %d\n",
                            ret);
                    }
#endif
                }
            } else {
                DEBUG_PRINTF(
                    "bsc_node_process_received() node switch is "
                    "disabled, send error to node %s\n",
                    bsc_vmac_to_string(decoded_pdu->hdr.origin));
                error_code = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                error_class = ERROR_CLASS_COMMUNICATION;
                bufsize = bvlc_sc_encode_result(
                    buf, sizeof(buf), decoded_pdu->hdr.message_id, NULL,
                    decoded_pdu->hdr.origin, decoded_pdu->hdr.bvlc_function, 1,
                    NULL, &error_class, &error_code,
                    (uint8_t *)ERROR_STR_DIRECT_CONNECTIONS_NOT_SUPPORTED);
                if (bufsize) {
                    ret = bsc_node_send(node, buf, bufsize);
#if DEBUG_ENABLED == 1
                    if (ret != BSC_SC_SUCCESS) {
                        DEBUG_PRINTF(
                            "bsc_node_process_received() warning "
                            "bvlc-result pdu is not sent, error %d\n",
                            ret);
                    }
#endif
                }
            }
            break;
        }
        case BVLC_SC_ADDRESS_RESOLUTION_ACK: {
            DEBUG_PRINTF("bsc_node_process_received() got "
                         "BVLC_SC_ADDRESS_RESOLUTION_ACK\n");
            r = node_get_address_resolution(node, decoded_pdu->hdr.origin);
            if (!r) {
                r = node_alloc_address_resolution(
                    node, decoded_pdu->hdr.origin);
#if DEBUG_ENABLED == 1
                if (!r) {
                    DEBUG_PRINTF(
                        "can't allocate address resolution for node "
                        "with address %s\n",
                        bsc_vmac_to_string(decoded_pdu->hdr.origin));
                }
#endif
            }
            if (r) {
                bsc_node_parse_urls(r, decoded_pdu);
                mstimer_restart(&r->fresh_timer);
                bsc_node_switch_process_address_resolution(
                    node->node_switch, r);
            }
            break;
        }
        case BVLC_SC_ENCAPSULATED_NPDU: {
            node->conf->event_func(
                node, BSC_NODE_EVENT_RECEIVED_NPDU, NULL, pdu, pdu_len);
            break;
        }
        default:
            break;
    }
    DEBUG_PRINTF("bsc_node_process_received() <<<\n");
}

static void bsc_hub_connector_event(
    BSC_HUB_CONNECTOR_EVENT ev,
    BSC_HUB_CONNECTOR_HANDLE h,
    void *user_arg,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    BSC_NODE *node = (BSC_NODE *)user_arg;

    (void)h;
    bws_dispatch_lock();
    DEBUG_PRINTF(
        "bsc_hub_connector_event() >>> ev = %d, h = %p, node = %p\n", ev, h,
        user_arg);
    if (ev == BSC_HUBC_EVENT_STOPPED) {
        node->hub_connector = NULL;
        bsc_node_process_stop_event(node);
    } else if (ev == BSC_HUBC_EVENT_ERROR_DUPLICATED_VMAC) {
        if (node->state != BSC_NODE_STATE_STOPPING &&
            node->state != BSC_NODE_STATE_RESTARTING) {
            bsc_node_restart(node);
        }
    } else if (ev == BSC_HUBC_EVENT_RECEIVED) {
        bsc_node_process_received(node, pdu, pdu_len, decoded_pdu);
    }
    DEBUG_PRINTF("bsc_hub_connector_event() <<<\n");
    bws_dispatch_unlock();
}

static void bsc_hub_function_event(
    BSC_HUB_FUNCTION_EVENT ev, BSC_HUB_FUNCTION_HANDLE h, void *user_arg)
{
    BSC_NODE *node = (BSC_NODE *)user_arg;

    (void)h;
    bws_dispatch_lock();
    DEBUG_PRINTF(
        "bsc_hub_function_event() >>> ev = %d, h = %p, node = %p\n", ev, h,
        user_arg);
    if (ev == BSC_HUBF_EVENT_STARTED) {
        bsc_node_process_start_event(node);
    } else if (ev == BSC_HUBF_EVENT_STOPPED) {
        node->hub_function = NULL;
        bsc_node_process_stop_event(node);
    } else if (ev == BSC_HUBF_EVENT_ERROR_DUPLICATED_VMAC) {
        if (node->state != BSC_NODE_STATE_STOPPING &&
            node->state != BSC_NODE_STATE_RESTARTING &&
            node->state != BSC_NODE_STATE_IDLE) {
            bsc_node_restart(node);
        }
    }
    DEBUG_PRINTF("bsc_hub_function_event() <<<\n");
    bws_dispatch_unlock();
}

static void bsc_node_switch_event(
    BSC_NODE_SWITCH_EVENT ev,
    BSC_NODE_SWITCH_HANDLE h,
    void *user_arg,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *pdu,
    size_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu)
{
    BSC_NODE *node = (BSC_NODE *)user_arg;

    (void)h;
    bws_dispatch_lock();
    DEBUG_PRINTF(
        "bsc_node_switch_event() >>> ev = %d, h = %p, node = %p\n", ev, h,
        user_arg);
    if (ev == BSC_NODE_SWITCH_EVENT_STARTED) {
        bsc_node_process_start_event(node);
    } else if (ev == BSC_NODE_SWITCH_EVENT_STOPPED) {
        node->node_switch = NULL;
        bsc_node_process_stop_event(node);
    } else if (ev == BSC_NODE_SWITCH_EVENT_DUPLICATED_VMAC) {
        if (node->state != BSC_NODE_STATE_STOPPING &&
            node->state != BSC_NODE_STATE_RESTARTING &&
            node->state != BSC_NODE_STATE_IDLE) {
            bsc_node_restart(node);
        }
    } else if (ev == BSC_NODE_SWITCH_EVENT_RECEIVED) {
        bsc_node_process_received(node, pdu, pdu_len, decoded_pdu);
    } else if (ev == BSC_NODE_SWITCH_EVENT_CONNECTED) {
        node->conf->event_func(
            node, BSC_NODE_EVENT_DIRECT_CONNECTED, dest, NULL, 0);
    } else if (ev == BSC_NODE_SWITCH_EVENT_DISCONNECTED) {
        node->conf->event_func(
            node, BSC_NODE_EVENT_DIRECT_DISCONNECTED, dest, NULL, 0);
    }
    DEBUG_PRINTF("bsc_node_switch_event() <<<\n");
    bws_dispatch_unlock();
}

BSC_SC_RET bsc_node_init(BSC_NODE_CONF *conf, BSC_NODE **node)
{
    DEBUG_PRINTF("bsc_node_init() >>> conf = %p, node = %p\n", conf, node);

    if (!conf || !node) {
        DEBUG_PRINTF("bsc_node_init() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    if (!conf->ca_cert_chain || !conf->ca_cert_chain_size ||
        !conf->cert_chain || !conf->cert_chain_size || !conf->key ||
        !conf->key_size || !conf->local_uuid || conf->connect_timeout_s <= 0 ||
        conf->heartbeat_timeout_s <= 0 || conf->disconnect_timeout_s <= 0 ||
        conf->reconnnect_timeout_s <= 0 ||
        conf->address_resolution_timeout_s <= 0 ||
        conf->address_resolution_freshness_timeout_s <= 0 ||
        !conf->event_func) {
        DEBUG_PRINTF("bsc_node_init() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }
    bws_dispatch_lock();
    *node = bsc_alloc_node();

    if (!(*node)) {
        DEBUG_PRINTF("bsc_node_init() <<< ret =  BSC_SC_NO_RESOURCE\n");
        bws_dispatch_unlock();
        return BSC_SC_NO_RESOURCES;
    }

    memcpy((*node)->conf, conf, sizeof(*conf));
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_init() <<< ret = BSC_SC_SUCCESS\n");
    return BSC_SC_SUCCESS;
}

BSC_SC_RET bsc_node_deinit(BSC_NODE *node)
{
    DEBUG_PRINTF("bsc_node_deinit() >>> node = %p\n", node);
    bws_dispatch_lock();
    if (node->state != BSC_NODE_STATE_IDLE) {
        bws_dispatch_unlock();
        DEBUG_PRINTF("bsc_node_deinit() <<< ret = BSC_SC_INVALID_OPERATION\n");
        return BSC_SC_INVALID_OPERATION;
    }
    bsc_free_node(node);
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_deinit() <<< ret = BSC_SC_SUCCESS\n");
    return BSC_SC_SUCCESS;
}

static BSC_SC_RET bsc_node_start_state(BSC_NODE *node, BSC_NODE_STATE state)
{
    BSC_SC_RET ret = BSC_SC_BAD_PARAM;
    bws_dispatch_lock();
    DEBUG_PRINTF(
        "bsc_node_start_state() >>> node = %p state = %d\n", node, state);

    node->state = state;
    node->hub_connector = NULL;
    node->hub_function = NULL;
    node->node_switch = NULL;

    if (node->state != BSC_NODE_STATE_RESTARTING) {
        memset(
            node->resolution, 0,
            sizeof(BSC_ADDRESS_RESOLUTION) *
                BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM);
    } else {
        bsc_generate_random_vmac(&node->conf->local_vmac);
        DEBUG_PRINTF(
            "bsc_node_start_state() generated random vmac %s for node %p\n",
            bsc_vmac_to_string(&node->conf->local_vmac), node);
    }

    if (node->conf->primaryURL) {
        ret = bsc_hub_connector_start(
            node->conf->ca_cert_chain, node->conf->ca_cert_chain_size,
            node->conf->cert_chain, node->conf->cert_chain_size,
            node->conf->key, node->conf->key_size, node->conf->local_uuid,
            &node->conf->local_vmac, node->conf->max_local_bvlc_len,
            node->conf->max_local_npdu_len, node->conf->connect_timeout_s,
            node->conf->heartbeat_timeout_s, node->conf->disconnect_timeout_s,
            node->conf->primaryURL, node->conf->failoverURL,
            node->conf->reconnnect_timeout_s, bsc_hub_connector_event, node,
            &node->hub_connector);

        if (ret != BSC_SC_SUCCESS) {
            node->state = BSC_NODE_STATE_IDLE;
            bws_dispatch_unlock();
            DEBUG_PRINTF("bsc_node_start_state() <<< ret = %d\n", ret);
            return ret;
        }
    }

    if (node->conf->hub_function_enabled) {
        ret = bsc_hub_function_start(
            node->conf->ca_cert_chain, node->conf->ca_cert_chain_size,
            node->conf->cert_chain, node->conf->cert_chain_size,
            node->conf->key, node->conf->key_size, node->conf->hub_server_port,
            node->conf->hub_iface, node->conf->local_uuid,
            &node->conf->local_vmac, node->conf->max_local_bvlc_len,
            node->conf->max_local_npdu_len, node->conf->connect_timeout_s,
            node->conf->heartbeat_timeout_s, node->conf->disconnect_timeout_s,
            bsc_hub_function_event, node, &node->hub_function);
        if (ret != BSC_SC_SUCCESS) {
            node->state = BSC_NODE_STATE_IDLE;
            bsc_hub_connector_stop(node->hub_connector);
            bws_dispatch_unlock();
            DEBUG_PRINTF("bsc_node_start_state() <<< ret = %d\n", ret);
            return ret;
        }
    }

    if (node_switch_enabled(node->conf)) {
        ret = bsc_node_switch_start(
            node->conf->ca_cert_chain, node->conf->ca_cert_chain_size,
            node->conf->cert_chain, node->conf->cert_chain_size,
            node->conf->key, node->conf->key_size,
            node->conf->direct_server_port, node->conf->direct_iface,
            node->conf->local_uuid, &node->conf->local_vmac,
            node->conf->max_local_bvlc_len, node->conf->max_local_npdu_len,
            node->conf->connect_timeout_s, node->conf->heartbeat_timeout_s,
            node->conf->disconnect_timeout_s, node->conf->reconnnect_timeout_s,
            node->conf->address_resolution_timeout_s,
            node->conf->direct_connect_accept_enable,
            node->conf->direct_connect_initiate_enable, bsc_node_switch_event,
            node, &node->node_switch);
        if (ret != BSC_SC_SUCCESS) {
            node->state = BSC_NODE_STATE_IDLE;
            bsc_hub_connector_stop(node->hub_connector);
            bsc_hub_function_stop(node->hub_function);
            bws_dispatch_unlock();
            DEBUG_PRINTF("bsc_node_start_state() <<< ret = %d\n", ret);
            return ret;
        }
    }
    if (!node->conf->hub_function_enabled && !node_switch_enabled(node->conf)) {
        if (!node->conf->primaryURL) {
            node->state = BSC_NODE_STATE_IDLE;
        } else {
            node->state = BSC_NODE_STATE_STARTED;
            node->conf->event_func(node, BSC_NODE_EVENT_STARTED, NULL, NULL, 0);
        }
    }
    DEBUG_PRINTF(
        "bsc_node_start_state() hub_function %p hub_connector %p node_switch "
        "%p\n",
        node->hub_function, node->hub_connector, node->node_switch);
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_start_state() <<< ret = %d\n", ret);
    return ret;
}

BSC_SC_RET bsc_node_start(BSC_NODE *node)
{
    BSC_SC_RET ret;

    DEBUG_PRINTF("bsc_node_start() >>> node = %p\n", node);

    if (!node) {
        DEBUG_PRINTF("bsc_node_start() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    bws_dispatch_lock();

    if (node->state != BSC_NODE_STATE_IDLE) {
        bws_dispatch_unlock();
        DEBUG_PRINTF("bsc_node_start() <<< ret = BSC_SC_INVALID_OPERATION\n");
        return BSC_SC_INVALID_OPERATION;
    }
    ret = bsc_node_start_state(node, BSC_NODE_STATE_STARTING);
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_start() <<< ret = %d\n", ret);
    return ret;
}

void bsc_node_stop(BSC_NODE *node)
{
    DEBUG_PRINTF("bsc_node_stop() >>> node = %p\n", node);

    if (node) {
        bws_dispatch_lock();

        if (node->state != BSC_NODE_STATE_IDLE &&
            node->state != BSC_NODE_STATE_STOPPING) {
            node->state = BSC_NODE_STATE_STOPPING;
            bsc_hub_connector_stop(node->hub_connector);
            if (node->conf->hub_function_enabled) {
                bsc_hub_function_stop(node->hub_function);
            }
            if (node_switch_enabled(node->conf)) {
                bsc_node_switch_stop(node->node_switch);
            }
        }

        bws_dispatch_unlock();
    }

    DEBUG_PRINTF("bsc_node_stop() <<<\n");
}

BSC_SC_RET
bsc_node_hub_connector_send(void *p_node, uint8_t *pdu, size_t pdu_len)
{
    BSC_NODE *node = (BSC_NODE *)p_node;
    BSC_SC_RET ret;

    DEBUG_PRINTF(
        "bsc_node_hub_connector_send() >>> p_node = %p, pdu = %p, "
        "pdu_len = %d\n",
        p_node, pdu, pdu_len);

    if (!node) {
        DEBUG_PRINTF(
            "bsc_node_hub_connector_send() <<< ret =  BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    bws_dispatch_lock();

    if (node->state != BSC_NODE_STATE_STARTED) {
        DEBUG_PRINTF("bsc_node_hub_connector_send() <<< ret = "
                     "BSC_SC_INVALID_OPERATION\n");
        bws_dispatch_unlock();
        return BSC_SC_INVALID_OPERATION;
    }

    ret = bsc_hub_connector_send(node->hub_connector, pdu, pdu_len);
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_hub_connector_send() <<< ret = %d\n", ret);
    return ret;
}

BSC_SC_RET bsc_node_send(BSC_NODE *p_node, uint8_t *pdu, size_t pdu_len)
{
    BSC_NODE *node = (BSC_NODE *)p_node;
    BSC_SC_RET ret;

    DEBUG_PRINTF(
        "bsc_node_send() >>> p_node = %p(%s), pdu = %p, "
        "pdu_len = %d\n",
        p_node, p_node ? bsc_vmac_to_string(&p_node->conf->local_vmac) : NULL,
        pdu, pdu_len);

    if (!node) {
        DEBUG_PRINTF("bsc_node_send() <<< ret =  BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    bws_dispatch_lock();

    if (node->state != BSC_NODE_STATE_STARTED) {
        DEBUG_PRINTF("bsc_node_send() <<< ret = "
                     "BSC_SC_INVALID_OPERATION\n");
        bws_dispatch_unlock();
        return BSC_SC_INVALID_OPERATION;
    }

    if (node_switch_enabled(node->conf)) {
        ret = bsc_node_switch_send(node->node_switch, pdu, pdu_len);
    } else {
        ret = bsc_hub_connector_send(node->hub_connector, pdu, pdu_len);
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_send() <<< ret = %d\n", ret);
    return ret;
}

BSC_ADDRESS_RESOLUTION *
bsc_node_get_address_resolution(void *p_node, BACNET_SC_VMAC_ADDRESS *vmac)
{
    int i;
    BSC_NODE *node = (BSC_NODE *)p_node;
    bws_dispatch_lock();

    if (!node || node->state != BSC_NODE_STATE_STARTED || !vmac) {
        bws_dispatch_unlock();
        return NULL;
    }
    for (i = 0; i < BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM; i++) {
        if (node->resolution[i].used &&
            !memcmp(
                &vmac->address[0], &node->resolution[i].vmac.address[0],
                BVLC_SC_VMAC_SIZE)) {
            if (!mstimer_expired(&node->resolution[i].fresh_timer)) {
                bws_dispatch_unlock();
                return &node->resolution[i];
            } else {
                node_free_address_resolution(&node->resolution[i]);
                bws_dispatch_unlock();
                return NULL;
            }
        }
    }
    bws_dispatch_unlock();
    return NULL;
}

BSC_SC_RET
bsc_node_send_address_resolution(void *p_node, BACNET_SC_VMAC_ADDRESS *dest)
{
    BSC_NODE *node = (BSC_NODE *)p_node;
    uint8_t pdu[32];
    size_t pdu_len;
    BSC_SC_RET ret;
    DEBUG_PRINTF(
        "bsc_node_send_address_resolution() >>> node = %p, dest = %p\n", node,
        dest);
    pdu_len = bvlc_sc_encode_address_resolution(
        pdu, sizeof(pdu), bsc_get_next_message_id(), NULL, dest);
    ret = bsc_node_send(node, pdu, pdu_len);
    DEBUG_PRINTF("bsc_node_send_address_resolution() <<< ret = %d\n", ret);
    return ret;
}

BSC_SC_RET bsc_node_connect_direct(
    BSC_NODE *node, BACNET_SC_VMAC_ADDRESS *dest, char **urls, size_t urls_cnt)
{
    BSC_SC_RET ret = BSC_SC_INVALID_OPERATION;
    DEBUG_PRINTF(
        "bsc_node_connect_direct() >>> node = %p, dest = %p, urls = "
        "%p, urls_cnt = %d\n",
        node, dest, urls, urls_cnt);
    bws_dispatch_lock();
    if (node->state == BSC_NODE_STATE_STARTED &&
        node->conf->direct_connect_initiate_enable) {
        ret = bsc_node_switch_connect(node->node_switch, dest, urls, urls_cnt);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_connect_direct() <<< ret = %d\n", ret);
    return ret;
}

void bsc_node_disconnect_direct(BSC_NODE *node, BACNET_SC_VMAC_ADDRESS *dest)
{
    DEBUG_PRINTF(
        "bsc_node_disconnect_direct() >>> node = %p, dest = %p\n", node, dest);
    bws_dispatch_lock();
    if (node->state == BSC_NODE_STATE_STARTED &&
        node->conf->direct_connect_initiate_enable) {
        bsc_node_switch_disconnect(node->node_switch, dest);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_disconnect_direct() <<< \n");
}

bool bsc_node_direct_connection_established(
    BSC_NODE *node, BACNET_SC_VMAC_ADDRESS *dest, char **urls, size_t urls_cnt)
{
    bool ret = false;
    bws_dispatch_lock();
    if (node->state == BSC_NODE_STATE_STARTED &&
        (node->conf->direct_connect_initiate_enable ||
         node->conf->direct_connect_accept_enable)) {
        ret =
            bsc_node_switch_connected(node->node_switch, dest, urls, urls_cnt);
    }
    bws_dispatch_unlock();
    return ret;
}

BACNET_SC_HUB_CONNECTOR_STATE
bsc_node_hub_connector_state(BSC_NODE *node)
{
    BACNET_SC_HUB_CONNECTOR_STATE ret =
        BACNET_SC_HUB_CONNECTOR_STATE_NO_HUB_CONNECTION;
    bws_dispatch_lock();
    if (node->state == BSC_NODE_STATE_STARTED) {
        ret = bsc_hub_connector_state(node->hub_connector);
    }
    bws_dispatch_unlock();
    return ret;
}

BACNET_SC_HUB_CONNECTION_STATUS *
bsc_node_hub_connector_status(BSC_NODE *node, bool primary)
{
    BACNET_SC_HUB_CONNECTION_STATUS *ret = NULL;
    bws_dispatch_lock();
    if (node->state == BSC_NODE_STATE_STARTED) {
        ret = bsc_hub_connector_status(node->hub_connector, primary);
    }
    bws_dispatch_unlock();
    return ret;
}

BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *
bsc_node_hub_function_status(BSC_NODE *node, size_t *cnt)
{
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *ret = NULL;
    bws_dispatch_lock();
    if (node->state == BSC_NODE_STATE_STARTED &&
        node->conf->hub_function_enabled) {
        *cnt = BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM;
        ret = node->hub_status;
    }
    bws_dispatch_unlock();
    return ret;
}

BACNET_SC_DIRECT_CONNECTION_STATUS *
bsc_node_direct_connection_status(BSC_NODE *node, size_t *cnt)
{
    BACNET_SC_DIRECT_CONNECTION_STATUS *ret = NULL;
    bws_dispatch_lock();
    if (node->state == BSC_NODE_STATE_STARTED &&
        (node->conf->direct_connect_accept_enable ||
         node->conf->direct_connect_initiate_enable)) {
        *cnt = BSC_CONF_NODE_SWITCH_CONNECTION_STATUS_MAX_NUM;
        ret = node->direct_status;
    }
    bws_dispatch_unlock();
    return ret;
}

void bsc_node_maintenance_timer(uint16_t seconds)
{
    (void)seconds;

    bsc_socket_maintenance_timer(seconds);
    bsc_hub_connector_maintenance_timer(seconds);
    bsc_node_switch_maintenance_timer(seconds);
}

static void bsc_node_add_failed_request_info(
    BACNET_SC_FAILED_CONNECTION_REQUEST *r,
    BACNET_HOST_N_PORT_DATA *peer,
    BACNET_SC_VMAC_ADDRESS *vmac,
    BACNET_SC_UUID *uuid,
    BACNET_ERROR_CODE error,
    const char *error_desc)
{
    bsc_set_timestamp(&r->Timestamp);
    memcpy(&r->Peer_Address, peer, sizeof(*peer));
    memcpy(r->Peer_VMAC, &vmac->address[0], BVLC_SC_VMAC_SIZE);
    memcpy(&r->Peer_UUID.uuid.uuid128[0], &uuid->uuid[0], BVLC_SC_UUID_SIZE);
    r->Error = error;
    if (!error_desc) {
        r->Error_Details[0] = 0;
    } else {
        bsc_copy_str(r->Error_Details, error_desc, sizeof(r->Error_Details));
    }
}

void bsc_node_store_failed_request_info(
    BSC_NODE *node,
    BACNET_HOST_N_PORT_DATA *peer,
    BACNET_SC_VMAC_ADDRESS *vmac,
    BACNET_SC_UUID *uuid,
    BACNET_ERROR_CODE error,
    const char *error_desc)
{
    size_t i, j = 0;
    bool found = false;
    BACNET_DATE_TIME t;

    bws_dispatch_lock();

    for (i = 0; i < BSC_CONF_FAILED_CONNECTION_STATUS_MAX_NUM; i++) {
        if (node->failed[i].Peer_Address.host[0] == 0) {
            found = true;
            break;
        }
    }
    if (found) {
        bsc_node_add_failed_request_info(
            &node->failed[i], peer, vmac, uuid, error, error_desc);
    } else {
        bsc_set_timestamp(&t);
        for (i = 0; i < BSC_CONF_FAILED_CONNECTION_STATUS_MAX_NUM; i++) {
            if (datetime_compare(&node->failed[i].Timestamp, &t) < 0) {
                j = i;
                memcpy(&t, &node->failed[i].Timestamp, sizeof(t));
            }
        }
        bsc_node_add_failed_request_info(
            &node->failed[j], peer, vmac, uuid, error, error_desc);
    }

    bws_dispatch_unlock();
}

BACNET_SC_FAILED_CONNECTION_REQUEST *
bsc_node_failed_requests_status(BSC_NODE *node, size_t *cnt)
{
    BACNET_SC_FAILED_CONNECTION_REQUEST *ret = NULL;
    bws_dispatch_lock();
    if (node->state == BSC_NODE_STATE_STARTED &&
        (node->conf->direct_connect_accept_enable ||
         node->conf->hub_function_enabled)) {
        ret = node->failed;
        *cnt = BSC_CONF_FAILED_CONNECTION_STATUS_MAX_NUM;
    }
    bws_dispatch_unlock();
    return ret;
}

BACNET_SC_DIRECT_CONNECTION_STATUS *bsc_node_find_direct_status_for_vmac(
    BSC_NODE *node, BACNET_SC_VMAC_ADDRESS *vmac)
{
    int i;
    int index = -1;
    BACNET_DATE_TIME timestamp;

    for (i = 0; i < BSC_CONF_NODE_SWITCH_CONNECTION_STATUS_MAX_NUM; i++) {
        if (!datetime_is_valid(
                &node->direct_status[i].Connect_Timestamp.date,
                &node->direct_status[i].Connect_Timestamp.time)) {
            return &node->direct_status[i];
        }
        if (!memcmp(
                &node->direct_status[i].Peer_VMAC[0], &vmac->address[0],
                BVLC_SC_VMAC_SIZE)) {
            return &node->direct_status[i];
        }
    }

    /* ok, all entries are already filled, try to found oldest entry with
       non connected status */

    for (i = 0; i < BSC_CONF_NODE_SWITCH_CONNECTION_STATUS_MAX_NUM; i++) {
        if (node->direct_status[i].State !=
                BACNET_SC_CONNECTION_STATE_CONNECTED &&
            datetime_is_valid(
                &node->direct_status[i].Disconnect_Timestamp.date,
                &node->direct_status[i].Disconnect_Timestamp.time)) {
            if (index == -1 ||
                (datetime_compare(
                     &node->direct_status[i].Disconnect_Timestamp, &timestamp) <
                 0)) {
                index = i;
                memcpy(
                    &timestamp, &node->direct_status[i].Disconnect_Timestamp,
                    sizeof(timestamp));
            }
        }
    }

    if (index != -1) {
        return &node->direct_status[index];
    }

    /* ok, all entries are already filled and all are in connected state,
       so reuse oldest entry which is in connected state */

    memcpy(
        &timestamp, &node->direct_status[0].Connect_Timestamp,
        sizeof(timestamp));
    index = 0;

    for (i = 0; i < BSC_CONF_NODE_SWITCH_CONNECTION_STATUS_MAX_NUM; i++) {
        if (datetime_compare(
                &node->direct_status[i].Connect_Timestamp, &timestamp) < 0) {
            index = i;
            memcpy(
                &timestamp, &node->direct_status[i].Connect_Timestamp,
                sizeof(timestamp));
        }
    }

    return &node->direct_status[index];
}

BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *
bsc_node_find_hub_status_for_vmac(BSC_NODE *node, BACNET_SC_VMAC_ADDRESS *vmac)
{
    int i;
    int index = -1;
    BACNET_DATE_TIME timestamp;

    for (i = 0; i < BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM; i++) {
        if (!datetime_is_valid(
                &node->hub_status[i].Connect_Timestamp.date,
                &node->hub_status[i].Connect_Timestamp.time)) {
            return &node->hub_status[i];
        }
        if (!memcmp(
                &node->hub_status[i].Peer_VMAC[0], &vmac->address[0],
                BVLC_SC_VMAC_SIZE)) {
            return &node->hub_status[i];
        }
    }

    /* ok, all entries are already filled, try to found oldest entry with
       non connected status */

    for (i = 0; i < BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM; i++) {
        if (node->hub_status[i].State != BACNET_SC_CONNECTION_STATE_CONNECTED &&
            datetime_is_valid(
                &node->hub_status[i].Disconnect_Timestamp.date,
                &node->hub_status[i].Disconnect_Timestamp.time)) {
            if (index == -1 ||
                (datetime_compare(
                     &node->hub_status[i].Disconnect_Timestamp, &timestamp) <
                 0)) {
                index = i;
                memcpy(
                    &timestamp, &node->hub_status[i].Disconnect_Timestamp,
                    sizeof(timestamp));
            }
        }
    }

    if (index != -1) {
        return &node->hub_status[index];
    }

    /* ok, all entries are already filled and all are in connected state,
       so reuse oldest entry which is in connected state */

    memcpy(
        &timestamp, &node->hub_status[0].Connect_Timestamp, sizeof(timestamp));
    index = 0;

    for (i = 0; i < BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM; i++) {
        if (datetime_compare(
                &node->hub_status[i].Connect_Timestamp, &timestamp) < 0) {
            index = i;
            memcpy(
                &timestamp, &node->hub_status[i].Connect_Timestamp,
                sizeof(timestamp));
        }
    }

    return &node->hub_status[index];
}
