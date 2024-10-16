
/**
 * @file
 * @brief BACNet secure connect API.
 * @author Kirill Neznamov
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/debug.h"

#define DEBUG_BSC_SOCKET 0

#if DEBUG_BSC_SOCKET == 1
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF debug_printf_disabled
#endif

static const char *s_error_no_origin =
    "'Originating Virtual Address' field must be present";
static const char *s_error_dest_presented =
    "'Destination Virtual Address' field must be absent";
static const char *s_error_origin_presented =
    "'Originating Virtual Address' field must be absent";
static const char *s_error_no_dest =
    "'Destination Virtual Address' field must be present";

static BSC_SOCKET_CTX *bsc_socket_ctx[BSC_SOCKET_CTX_NUM] = { 0 };
static BVLC_SC_DECODED_MESSAGE bsc_dm = { 0 };

#define TX_BUF_PTR(c) \
    &c->tx_buf[c->tx_buf_size + sizeof(uint16_t) + BSC_CONF_TX_PRE]

#define TX_BUF_UPDATE(c, len)                                   \
    memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(uint16_t)); \
    c->tx_buf_size += sizeof(uint16_t) + BSC_CONF_TX_PRE + len

#define TX_BUF_BYTES_AVAIL(c)                                       \
    (((sizeof(c->tx_buf) - c->tx_buf_size) >=                       \
      (sizeof(uint16_t) + BSC_CONF_TX_PRE))                         \
         ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(uint16_t) - \
            BSC_CONF_TX_PRE)                                        \
         : 0)

static bool bsc_ctx_add(BSC_SOCKET_CTX *ctx)
{
    int i;
    for (i = 0; i < BSC_SOCKET_CTX_NUM; i++) {
        if (bsc_socket_ctx[i] == NULL) {
            bsc_socket_ctx[i] = ctx;
            return true;
        }
    }
    return false;
}

static void bsc_ctx_remove(BSC_SOCKET_CTX *ctx)
{
    int i;
    for (i = 0; i < BSC_SOCKET_CTX_NUM; i++) {
        if (bsc_socket_ctx[i] == ctx) {
            bsc_socket_ctx[i] = NULL;
            break;
        }
    }
}

static void bsc_reset_socket(BSC_SOCKET *c)
{
    memset(&c->vmac, 0, sizeof(c->vmac));
    memset(&c->uuid, 0, sizeof(c->uuid));
    c->tx_buf_size = 0;
}

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
    unsigned int disconnect_timeout_s)
{
    DEBUG_PRINTF("bsc_init_ctx_cfg() >>> cfg = %p\n");
    if (cfg) {
        cfg->proto = proto;
        cfg->port = port;
        cfg->type = type;
        cfg->iface = iface;
        cfg->ca_cert_chain = ca_cert_chain;
        cfg->ca_cert_chain_size = ca_cert_chain_size;
        cfg->cert_chain = cert_chain;
        cfg->cert_chain_size = cert_chain_size;
        cfg->priv_key = key;
        cfg->priv_key_size = key_size;
        bsc_copy_uuid(&cfg->local_uuid, local_uuid);
        bsc_copy_vmac(&cfg->local_vmac, local_vmac);
        cfg->max_bvlc_len = max_local_bvlc_len;
        cfg->max_ndpu_len = max_local_ndpu_len;
        cfg->connect_timeout_s = connect_timeout_s;
        cfg->heartbeat_timeout_s = heartbeat_timeout_s;
        cfg->disconnect_timeout_s = disconnect_timeout_s;
    }
    DEBUG_PRINTF("bsc_init_ctx_cfg() <<<\n");
}

static BSC_SOCKET *
bsc_find_conn_by_websocket(BSC_SOCKET_CTX *ctx, BSC_WEBSOCKET_HANDLE h)
{
    size_t i;
    for (i = 0; i < ctx->sock_num; i++) {
        if (ctx->sock[i].state != BSC_SOCK_STATE_IDLE && ctx->sock[i].wh == h) {
            return &ctx->sock[i];
        }
    }
    return NULL;
}

static BSC_SOCKET *bsc_find_free_socket(BSC_SOCKET_CTX *ctx)
{
    size_t i;
    for (i = 0; i < ctx->sock_num; i++) {
        if (ctx->sock[i].state == BSC_SOCK_STATE_IDLE) {
            bsc_reset_socket(&ctx->sock[i]);
            return &ctx->sock[i];
        }
    }
    return NULL;
}

static void bsc_srv_process_error(BSC_SOCKET *c, BACNET_ERROR_CODE reason)
{
    DEBUG_PRINTF(
        "bsc_srv_process_error() >>> c = %p, reason = %d\n", c, reason);
    c->state = BSC_SOCK_STATE_ERROR;
    c->reason = reason;
    bws_srv_disconnect(c->ctx->sh, c->wh);
    DEBUG_PRINTF("bsc_srv_process_error() <<<\n");
}

static void bsc_cli_process_error(BSC_SOCKET *c, BACNET_ERROR_CODE reason)
{
    DEBUG_PRINTF("bsc_cli_process_error) >>> c = %p, reason = %d\n", c, reason);
    c->state = BSC_SOCK_STATE_ERROR;
    c->reason = reason;
    bws_cli_disconnect(c->wh);
    DEBUG_PRINTF("bsc_cli_process_error) <<<\n");
}

static bool bsc_prepare_error_extended(
    BSC_SOCKET *c,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t bvlc_function,
    uint8_t *error_header_marker,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code,
    uint8_t *utf8_details_string)
{
    uint16_t eclass = (uint16_t)error_class;
    uint16_t ecode = (uint16_t)error_code;
    uint16_t len;
    uint16_t message_id;

    DEBUG_PRINTF(
        "bsc_prepare_error_extended() >>> bvlc_function = %d\n", bvlc_function);

#if DEBUG_BSC_SOCKET == 1
    if (error_header_marker) {
        DEBUG_PRINTF(
            "                              error_header_marker = %d\n",
            *error_header_marker);
    }
    if (error_class) {
        DEBUG_PRINTF(
            "                              error_class = %d\n", error_class);
    }
    if (error_code) {
        DEBUG_PRINTF(
            "                              error_code = %d\n", error_code);
    }
    if (utf8_details_string) {
        DEBUG_PRINTF(
            "                              utf8_details_string = %s\n",
            utf8_details_string);
    }
    if (origin) {
        DEBUG_PRINTF(
            "                              origin = %s\n",
            bsc_vmac_to_string(origin));
    }
    if (dest) {
        DEBUG_PRINTF(
            "                              dest = %s\n",
            bsc_vmac_to_string(dest));
    }
#endif

    message_id = bsc_get_next_message_id();
    DEBUG_PRINTF("                              message_id = %d\n", message_id);

    len = bvlc_sc_encode_result(
        TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c), message_id, origin, dest,
        bvlc_function, 1, error_header_marker, &eclass, &ecode,
        utf8_details_string);
    if (len) {
        TX_BUF_UPDATE(c, len);
        DEBUG_PRINTF(
            "bsc_prepare_error_extended() <<< ret = true, pdu_len = %d\n", len);
        return true;
    }
    DEBUG_PRINTF("bsc_prepare_error_extended() <<< ret = false\n");
    return false;
}

static bool bsc_prepare_protocol_error_extended(
    BSC_SOCKET *c,
    BVLC_SC_DECODED_MESSAGE *dm,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *error_header_marker,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code,
    uint8_t *utf8_details_string)
{
    if (bvlc_sc_need_send_bvlc_result(dm)) {
        return bsc_prepare_error_extended(
            c, origin, dest, BVLC_SC_RESULT, error_header_marker, error_class,
            error_code, utf8_details_string);
    }
    return false;
}

static bool bsc_prepare_protocol_error(
    BSC_SOCKET *c,
    BVLC_SC_DECODED_MESSAGE *dm,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code,
    uint8_t *utf8_details_string)
{
    return bsc_prepare_protocol_error_extended(
        c, dm, origin, dest, NULL, error_class, error_code,
        utf8_details_string);
}

static void bsc_clear_vmac_and_uuid(BSC_SOCKET *c)
{
    memset(&c->vmac, 0, sizeof(c->vmac));
    memset(&c->uuid, 0, sizeof(c->uuid));
}

static void bsc_set_socket_idle(BSC_SOCKET *c)
{
    c->state = BSC_SOCK_STATE_IDLE;
    c->wh = BSC_WEBSOCKET_INVALID_HANDLE;
}

static void bsc_process_socket_disconnecting(
    BSC_SOCKET *c,
    BVLC_SC_DECODED_MESSAGE *dm,
    uint8_t *buf,
    uint16_t buflen,
    bool *need_disconnect)
{
    DEBUG_PRINTF("bsc_process_socket_disconnecting() >>> c = %p\n", c);

    if (dm->hdr.bvlc_function == BVLC_SC_DISCONNECT_ACK) {
#if DEBUG_BSC_SOCKET == 1
        if (dm->hdr.message_id != c->expected_disconnect_message_id) {
            DEBUG_PRINTF(
                "bsc_process_socket_disconnecting() got disconnect ack with "
                "unexpected message id %d for socket %p\n",
                dm->hdr.message_id, c);
        } else {
            DEBUG_PRINTF(
                "bsc_process_socket_disconnecting() got disconnect ack for "
                "socket %p\n",
                c);
        }
#endif
        *need_disconnect = true;
    } else if (dm->hdr.bvlc_function == BVLC_SC_RESULT) {
        if (dm->payload.result.bvlc_function == BVLC_SC_DISCONNECT_REQUEST &&
            dm->payload.result.result != 0) {
            DEBUG_PRINTF(
                "bsc_process_socket_disconnecting() got BVLC_SC_RESULT "
                "NAK on BVLC_SC_DISCONNECT_REQUEST\n");
            *need_disconnect = true;
        }
    } else {
        c->ctx->funcs->socket_event(
            c, BSC_SOCKET_EVENT_RECEIVED, 0, NULL, buf, buflen, dm);
    }

    DEBUG_PRINTF("bsc_process_socket_disconnecting() <<<\n");
}

static void bsc_process_socket_connected_state(
    BSC_SOCKET *c,
    BVLC_SC_DECODED_MESSAGE *dm,
    uint8_t *buf,
    uint16_t buflen,
    bool *need_disconnect,
    bool *need_send)
{
    uint16_t message_id;
    uint16_t len;

    DEBUG_PRINTF(
        "bsc_process_socket_connected_state() >>> c = %p, dm = %p,  buf = %p, "
        "buflen = %d\n",
        c, dm, buf, buflen);

    if (dm->hdr.bvlc_function == BVLC_SC_HEARTBEAT_ACK) {
#if DEBUG_ENABLED == 1
        if (dm->hdr.message_id != c->expected_heartbeat_message_id) {
            DEBUG_PRINTF(
                "bsc_process_socket_connected_state() got heartbeat ack with "
                "unexpected message id %d for socket %p\n",
                dm->hdr.message_id, c);
        } else {
            DEBUG_PRINTF(
                "bsc_process_socket_connected_state() got heartbeat ack for "
                "socket %p\n",
                c);
        }
#endif
    } else if (dm->hdr.bvlc_function == BVLC_SC_HEARTBEAT_REQUEST) {
        DEBUG_PRINTF(
            "bsc_process_socket_connected_state() got heartbeat "
            "request with message_id %d\n",
            dm->hdr.message_id);
        message_id = dm->hdr.message_id;
        len = bvlc_sc_encode_heartbeat_ack(
            TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c), message_id);
        if (len) {
            TX_BUF_UPDATE(c, len);
            *need_send = true;
        }
#if DEBUG_BSC_SOCKET == 1
        else {
            DEBUG_PRINTF(
                "bsc_process_socket_connected_state() no resources to "
                "answer on hearbeat request "
                "socket %p\n",
                c);
        }
#endif
    } else if (dm->hdr.bvlc_function == BVLC_SC_DISCONNECT_REQUEST) {
        DEBUG_PRINTF(
            "bsc_process_socket_connected_state() got disconnect "
            "request with message_id %d\n",
            dm->hdr.message_id);
        message_id = dm->hdr.message_id;
        len = bvlc_sc_encode_disconnect_ack(
            TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c), message_id);
        if (len) {
            TX_BUF_UPDATE(c, len);
            c->reason = ERROR_CODE_SUCCESS;
            c->state = BSC_SOCK_STATE_ERROR_FLUSH_TX;
            *need_send = true;
        } else {
            DEBUG_PRINTF(
                "bsc_process_socket_connected_state() no resources to answer "
                "on disconnect request, just disconnecting without ack\n");
            c->state = BSC_SOCK_STATE_DISCONNECTING;
            *need_disconnect = true;
        }
    } else if (dm->hdr.bvlc_function == BVLC_SC_DISCONNECT_ACK) {
        /* This is unexpected! We assume that the remote peer is confused and */
        /* thought we sent a Disconnect-Request so we'll close the socket */
        /* and hope that remote peer clears itself up. */
        DEBUG_PRINTF(
            "bsc_process_socket_connected_state() got unexpected "
            "disconnect ack with message_id %d\n",
            dm->hdr.message_id);
        c->state = BSC_SOCK_STATE_DISCONNECTING;
        *need_disconnect = true;
    } else if (dm->hdr.bvlc_function == BVLC_SC_RESULT) {
        DEBUG_PRINTF(
            "bsc_process_socket_connected_state() emit received event "
            "buf = %p, size = %d\n",
            buf, buflen);
        c->ctx->funcs->socket_event(
            c, BSC_SOCKET_EVENT_RECEIVED, 0, NULL, buf, buflen, dm);
    } else if (
        dm->hdr.bvlc_function == BVLC_SC_ENCAPSULATED_NPDU ||
        dm->hdr.bvlc_function == BVLC_SC_ADDRESS_RESOLUTION ||
        dm->hdr.bvlc_function == BVLC_SC_ADDRESS_RESOLUTION_ACK ||
        dm->hdr.bvlc_function == BVLC_SC_ADVERTISIMENT ||
        dm->hdr.bvlc_function == BVLC_SC_ADVERTISIMENT_SOLICITATION ||
        dm->hdr.bvlc_function == BVLC_SC_PROPRIETARY_MESSAGE) {
        DEBUG_PRINTF(
            "bsc_process_socket_connected_state() emit received event "
            "buf = %p, size = %d\n",
            buf, buflen);

        c->ctx->funcs->socket_event(
            c, BSC_SOCKET_EVENT_RECEIVED, 0, NULL, buf, buflen, dm);
    }

    DEBUG_PRINTF("bsc_process_socket_connected_state() <<<\n");
}

static void bsc_process_socket_state(
    BSC_SOCKET *c,
    BVLC_SC_DECODED_MESSAGE *dm,
    uint8_t *rx_buf,
    uint16_t rx_buf_size,
    bool *need_disconnect,
    bool *need_send)
{
    bool expired;
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    const char *err_desc = NULL;
    bool valid = true;
    unsigned int len;

    DEBUG_PRINTF(
        "bsc_process_socket_state() >>> ctx = %p, c = %p, state = %d, "
        "rx_buf = %p, rx_buf_size = %d\n",
        c->ctx, c, c->state, rx_buf, rx_buf_size);

    if (rx_buf) {
        if (!bvlc_sc_decode_message(
                rx_buf, rx_buf_size, dm, &code, &class, &err_desc)) {
            /* if code == ERROR_CODE_OTHER that means that received bvlc message
             */
            /* has length less than 4 octets. */
            /* According EA-001-4 'Clarifying BVLC-Result in BACnet/SC ' */
            /* If a BVLC message is received that has fewer than four octets, a
             */
            /* BVLC-Result NAK shall not be returned. */
            /* The message shall be discarded and not be processed. */
            if (code != ERROR_CODE_OTHER) {
                *need_send = bsc_prepare_protocol_error(
                    c, dm, dm->hdr.origin, dm->hdr.dest, class, code,
                    (uint8_t *)err_desc);
            }
#if DEBUG_BSC_SOCKET == 1
            else {
                DEBUG_PRINTF(
                    "bsc_process_socket_state() decoding failed, message "
                    "is silently dropped cause it's length < 4 bytes\n");
            }
#endif
        } else {
            DEBUG_PRINTF(
                "c->dm.hdr.bvlc_function == %d, message_id = %d\n",
                dm->hdr.bvlc_function, dm->hdr.message_id);
            if (dm->hdr.bvlc_function == BVLC_SC_ENCAPSULATED_NPDU ||
                dm->hdr.bvlc_function == BVLC_SC_ADVERTISIMENT ||
                dm->hdr.bvlc_function == BVLC_SC_ADDRESS_RESOLUTION_ACK ||
                dm->hdr.bvlc_function == BVLC_SC_ADDRESS_RESOLUTION ||
                dm->hdr.bvlc_function == BVLC_SC_ADVERTISIMENT_SOLICITATION ||
                dm->hdr.bvlc_function == BVLC_SC_RESULT) {
                if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR &&
                    c->ctx->cfg->proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
                    /* this is a case when socket is a hub connector receiving
                     */
                    /* from hub */
                    if (dm->hdr.origin == NULL &&
                        dm->hdr.bvlc_function != BVLC_SC_RESULT) {
                        class = ERROR_CLASS_COMMUNICATION;
                        code = ERROR_CODE_HEADER_ENCODING_ERROR;

                        *need_send = bsc_prepare_protocol_error(
                            c, dm, NULL, &c->vmac, class, code,
                            (uint8_t *)s_error_no_origin);
                        valid = false;
                    } else if (
                        dm->hdr.dest != NULL &&
                        !bvlc_sc_is_vmac_broadcast(dm->hdr.dest)) {
                        class = ERROR_CLASS_COMMUNICATION;
                        code = ERROR_CODE_HEADER_ENCODING_ERROR;
                        *need_send = bsc_prepare_protocol_error(
                            c, dm, NULL, &c->vmac, class, code,
                            (uint8_t *)s_error_dest_presented);
                        valid = false;
                    }
                } else if (
                    c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR &&
                    c->ctx->cfg->proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
                    /*  this is a case when socket is hub  function receiving */
                    /*  from node */
                    if (dm->hdr.dest == NULL) {
                        class = ERROR_CLASS_COMMUNICATION;
                        code = ERROR_CODE_HEADER_ENCODING_ERROR;

                        *need_send = bsc_prepare_protocol_error(
                            c, dm, NULL, NULL, class, code,
                            (uint8_t *)s_error_no_dest);
                        valid = false;
                    } else if (dm->hdr.origin != NULL) {
                        class = ERROR_CLASS_COMMUNICATION;
                        code = ERROR_CODE_HEADER_ENCODING_ERROR;
                        *need_send = bsc_prepare_protocol_error(
                            c, dm, NULL, NULL, class, code,
                            (uint8_t *)s_error_origin_presented);
                        valid = false;
                    }
                }
            }

            /* every valid message restarts hearbeat timeout */
            /* and only valid messages are processed */
            if (valid) {
                if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
                    mstimer_set(
                        &c->heartbeat,
                        2 * c->ctx->cfg->heartbeat_timeout_s * 1000);
                } else {
                    mstimer_set(
                        &c->heartbeat, c->ctx->cfg->heartbeat_timeout_s * 1000);
                }
                if (c->state == BSC_SOCK_STATE_CONNECTED) {
                    bsc_process_socket_connected_state(
                        c, dm, rx_buf, rx_buf_size, need_disconnect, need_send);
                } else if (c->state == BSC_SOCK_STATE_DISCONNECTING) {
                    bsc_process_socket_disconnecting(
                        c, dm, rx_buf, rx_buf_size, need_disconnect);
                }
            }
        }
    }

    expired = mstimer_expired(&c->t);
    DEBUG_PRINTF("bsc_process_socket_state() expired = %d\n", expired);

    if (c->state == BSC_SOCK_STATE_AWAITING_ACCEPT && expired) {
        c->state = BSC_SOCK_STATE_ERROR;
        c->reason = ERROR_CODE_TIMEOUT;
        *need_disconnect = true;
    } else if (c->state == BSC_SOCK_STATE_AWAITING_REQUEST && expired) {
        c->state = BSC_SOCK_STATE_ERROR;
        c->reason = ERROR_CODE_TIMEOUT;
        *need_disconnect = true;
    } else if (c->state == BSC_SOCK_STATE_DISCONNECTING && expired) {
        c->state = BSC_SOCK_STATE_ERROR;
        c->reason = ERROR_CODE_TIMEOUT;
        *need_disconnect = true;
    } else if (c->state == BSC_SOCK_STATE_CONNECTED) {
        expired = mstimer_expired(&c->heartbeat);
        DEBUG_PRINTF(
            "bsc_process_socket_state() hearbeat timeout expired = %d\n",
            expired);
        if (expired) {
            DEBUG_PRINTF(
                "bsc_process_socket_state() heartbeat timeout elapsed "
                "for socket %p\n",
                c);
            if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                DEBUG_PRINTF(
                    "bsc_process_socket_state() going to send heartbeat "
                    "request on connection %p\n",
                    c);
                c->expected_heartbeat_message_id = bsc_get_next_message_id();
                DEBUG_PRINTF(
                    "bsc_process_socket_state() heartbeat message_id %04x\n",
                    c->expected_heartbeat_message_id);

                len = bvlc_sc_encode_heartbeat_request(
                    TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c),
                    c->expected_heartbeat_message_id);

                if (len) {
                    TX_BUF_UPDATE(c, len);
                    mstimer_set(
                        &c->heartbeat, c->ctx->cfg->heartbeat_timeout_s * 1000);
                    *need_send = true;
                } else {
                    DEBUG_PRINTF(
                        "bsc_process_socket_state() sending of "
                        "heartbeat request failed on connection %p\n",
                        c);
                }
            } else if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
                DEBUG_PRINTF(
                    "bsc_process_socket_state() zombie socket %p is "
                    "disconnecting...\n",
                    c);
                c->state = BSC_SOCK_STATE_ERROR;
                c->reason = ERROR_CODE_TIMEOUT;
                *need_disconnect = true;
            }
        }
    }
    DEBUG_PRINTF("bsc_process_socket_state() <<<\n");
}

static void bsc_runloop_socket(
    BSC_SOCKET *s,
    BVLC_SC_DECODED_MESSAGE *dm,
    uint8_t *rx_buf,
    uint16_t rx_buf_size)
{
    bool need_disconnect = false;
    bool need_send = false;

    if (s->state != BSC_SOCK_STATE_IDLE) {
        bsc_process_socket_state(
            s, dm, rx_buf, rx_buf_size, &need_disconnect, &need_send);
        if (need_disconnect) {
            if (s->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                bws_cli_disconnect(s->wh);
            } else {
                bws_srv_disconnect(s->ctx->sh, s->wh);
            }
        }
        if (need_send) {
            if (s->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                bws_cli_send(s->wh);
            } else {
                bws_srv_send(s->ctx->sh, s->wh);
            }
        }
    }
}

void bsc_socket_maintenance_timer(uint16_t seconds)
{
    int i, j;
    (void)seconds;

    bws_dispatch_lock();

    for (i = 0; i < BSC_SOCKET_CTX_NUM; i++) {
        if (bsc_socket_ctx[i] != NULL) {
            if (bsc_socket_ctx[i]->state == BSC_CTX_STATE_INITIALIZED) {
                for (j = 0; j < bsc_socket_ctx[i]->sock_num; j++) {
                    bsc_runloop_socket(
                        &bsc_socket_ctx[i]->sock[j], NULL, NULL, 0);
                }
            }
        }
    }

    bws_dispatch_unlock();
}

static void bsc_process_srv_awaiting_request(
    BSC_SOCKET *c, BVLC_SC_DECODED_MESSAGE *dm, uint8_t *buf, size_t bufsize)
{
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    BSC_SOCKET *existing = NULL;
    uint16_t message_id;
    uint16_t len;
    uint16_t ucode;
    uint16_t uclass;
    const char *err_desc = NULL;

    DEBUG_PRINTF(
        "bsc_process_srv_awaiting_request() >>> c = %p, dm = %p, buf = %p, "
        "bufsize = %d\n",
        c, dm, buf, bufsize);

    if (!bvlc_sc_decode_message(buf, bufsize, dm, &code, &class, &err_desc)) {
        DEBUG_PRINTF(
            "bsc_process_srv_awaiting_request() decoding of received message "
            "failed, error code = %d, class = %d\n",
            code, class);
        if (c->ctx->funcs->failed_request) {
            c->ctx->funcs->failed_request(
                c->ctx, c, NULL, NULL, code, err_desc);
        }
    } else if (dm->hdr.bvlc_function == BVLC_SC_CONNECT_REQUEST) {
        existing = c->ctx->funcs->find_connection_for_uuid(
            dm->payload.connect_request.uuid, c->ctx->user_arg);

        if (existing) {
            /* Regarding AB.6.2.3 BACnet/SC Connection Accepting Peer */
            /* State Machine: On receipt of a Connect-Request message */
            /* from the initiating peer whose 'Device UUID' is equal to the */
            /* initiating peer device UUID of an existing connection, */
            /* then return a Connect-Accept message, disconnect and */
            /* close the existing connection to the connection peer node */
            /* with matching Device UUID, and enter the CONNECTED state. */
            DEBUG_PRINTF(
                "bsc_process_srv_awaiting_request() accepting connection from "
                "known uuid %s\n and vmac %s\n",
                bsc_uuid_to_string(dm->payload.connect_request.uuid),
                bsc_vmac_to_string(dm->payload.connect_request.vmac));
            DEBUG_PRINTF(
                "bsc_process_srv_awaiting_request() existing = %p, "
                "existing->state = %d, c = %p\n",
                existing, existing->state, c);
            bsc_copy_vmac(&c->vmac, dm->payload.connect_request.vmac);
            bsc_copy_uuid(&c->uuid, dm->payload.connect_request.uuid);
            c->max_npdu_len = dm->payload.connect_request.max_npdu_len;
            c->max_bvlc_len = dm->payload.connect_request.max_bvlc_len;
            message_id = dm->hdr.message_id;

            len = bvlc_sc_encode_connect_accept(
                TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c), message_id,
                &c->ctx->cfg->local_vmac, &c->ctx->cfg->local_uuid,
                c->ctx->cfg->max_bvlc_len, c->ctx->cfg->max_ndpu_len);

            if (!len) {
                if (c->ctx->funcs->failed_request) {
                    c->ctx->funcs->failed_request(
                        c->ctx, c, dm->payload.connect_request.vmac,
                        dm->payload.connect_request.uuid,
                        ERROR_CODE_ABORT_OUT_OF_RESOURCES, NULL);
                }
                bsc_srv_process_error(c, ERROR_CODE_ABORT_OUT_OF_RESOURCES);
                DEBUG_PRINTF("bsc_process_srv_awaiting_request() <<<\n");
                return;
            } else {
                TX_BUF_UPDATE(c, len);
                DEBUG_PRINTF(
                    "bsc_process_srv_awaiting_request() request to "
                    "send connect accept to socket %d(%p)\n",
                    c->wh, c);
                bws_srv_send(c->ctx->sh, c->wh);
            }

            existing->expected_disconnect_message_id =
                bsc_get_next_message_id();

            len = bvlc_sc_encode_disconnect_request(
                TX_BUF_PTR(existing), TX_BUF_BYTES_AVAIL(existing),
                existing->expected_disconnect_message_id);

            if (len) {
                TX_BUF_UPDATE(existing, len);
                DEBUG_PRINTF(
                    "bsc_process_srv_awaiting_request() request to "
                    "send disconnect request to socket %d(%p)\n",
                    existing->wh, existing);
                bws_srv_send(existing->ctx->sh, existing->wh);
            } else {
                DEBUG_PRINTF(
                    "bsc_process_srv_awaiting_request() sending of disconnect "
                    "request failed, err = BSC_SC_NO_RESOURCES\n");
            }
            DEBUG_PRINTF(
                "bsc_process_srv_awaiting_request() set socket %p to "
                "connected state\n",
                c);

            mstimer_set(
                &c->heartbeat, 2 * c->ctx->cfg->heartbeat_timeout_s * 1000);
            c->state = BSC_SOCK_STATE_CONNECTED;
            c->ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_CONNECTED, 0, NULL, NULL, 0, NULL);
            DEBUG_PRINTF("bsc_process_srv_awaiting_request() <<<\n");
            return;
        }

        existing = c->ctx->funcs->find_connection_for_vmac(
            dm->payload.connect_request.vmac, c->ctx->user_arg);

        if (existing) {
            DEBUG_PRINTF(
                "bsc_process_srv_awaiting_request() rejected connection for "
                "duplicated vmac %s from uuid %s,"
                " vmac is used by uuid %s\n",
                bsc_vmac_to_string(dm->payload.connect_request.vmac),
                bsc_uuid_to_string(dm->payload.connect_request.uuid),
                bsc_uuid_to_string(&existing->uuid));

            uclass = ERROR_CLASS_COMMUNICATION;
            ucode = ERROR_CODE_NODE_DUPLICATE_VMAC;
            message_id = dm->hdr.message_id;
            if (c->ctx->funcs->failed_request) {
                c->ctx->funcs->failed_request(
                    c->ctx, c, dm->payload.connect_request.vmac,
                    dm->payload.connect_request.uuid,
                    ERROR_CODE_NODE_DUPLICATE_VMAC, NULL);
            }
            len = bvlc_sc_encode_result(
                TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c), message_id, NULL, NULL,
                BVLC_SC_CONNECT_REQUEST, 1, NULL, &uclass, &ucode, NULL);

            if (len) {
                TX_BUF_UPDATE(c, len);
                c->state = BSC_SOCK_STATE_ERROR_FLUSH_TX;
                c->reason = ERROR_CODE_NODE_DUPLICATE_VMAC;
                bws_srv_send(c->ctx->sh, c->wh);
            } else {
                DEBUG_PRINTF(
                    "bsc_process_srv_awaiting_request() sending of nack result "
                    "message failed, err = BSC_SC_NO_RESOURCES\n");
                bsc_srv_process_error(c, ERROR_CODE_NODE_DUPLICATE_VMAC);
            }
            DEBUG_PRINTF("bsc_process_srv_awaiting_request() <<<\n");
            return;
        }

        bsc_copy_vmac(&c->vmac, dm->payload.connect_request.vmac);
        bsc_copy_uuid(&c->uuid, dm->payload.connect_request.uuid);

        DEBUG_PRINTF(
            "bsc_process_srv_awaiting_request() local vmac = %s, "
            "local uuid = %s\n",
            bsc_vmac_to_string(&c->ctx->cfg->local_vmac),
            bsc_uuid_to_string(&c->ctx->cfg->local_uuid));

        DEBUG_PRINTF(
            "bsc_process_srv_awaiting_request() remote vmac = %s, "
            "remote uuid = %s\n",
            bsc_vmac_to_string(&c->vmac), bsc_uuid_to_string(&c->uuid));

        if (memcmp(
                &c->vmac.address, &c->ctx->cfg->local_vmac.address,
                sizeof(c->ctx->cfg->local_vmac.address)) == 0 &&
            memcmp(
                &c->uuid.uuid, &c->ctx->cfg->local_uuid.uuid,
                sizeof(c->ctx->cfg->local_uuid.uuid)) != 0) {
            DEBUG_PRINTF(
                "bsc_process_srv_awaiting_request() rejected "
                "connection of a duplicate "
                "of this port's vmac %s from uuid %s\n",
                bsc_vmac_to_string(&c->vmac), bsc_uuid_to_string(&c->uuid));
            uclass = ERROR_CLASS_COMMUNICATION;
            ucode = ERROR_CODE_NODE_DUPLICATE_VMAC;
            message_id = dm->hdr.message_id;
            if (c->ctx->funcs->failed_request) {
                c->ctx->funcs->failed_request(
                    c->ctx, c, &c->vmac, &c->uuid,
                    ERROR_CODE_NODE_DUPLICATE_VMAC, NULL);
            }
            len = bvlc_sc_encode_result(
                TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c), message_id, NULL, NULL,
                BVLC_SC_CONNECT_REQUEST, 1, NULL, &uclass, &ucode, NULL);

            if (len) {
                TX_BUF_UPDATE(c, len);
                c->state = BSC_SOCK_STATE_ERROR_FLUSH_TX;
                c->reason = ERROR_CODE_NODE_DUPLICATE_VMAC;
                bws_srv_send(c->ctx->sh, c->wh);
            } else {
                DEBUG_PRINTF(
                    "bsc_process_srv_awaiting_request() sending of nack result "
                    "message failed, err =  BSC_SC_NO_RESOURCES\n");
                bsc_srv_process_error(c, ERROR_CODE_NODE_DUPLICATE_VMAC);
            }
            DEBUG_PRINTF("bsc_process_srv_awaiting_request() <<<\n");
            return;
        }
        DEBUG_PRINTF(
            "bsc_process_srv_awaiting_request() accepted connection from new "
            "uuid %s with vmac %s\n",
            bsc_uuid_to_string(&c->uuid), bsc_vmac_to_string(&c->vmac));

        message_id = dm->hdr.message_id;

        len = bvlc_sc_encode_connect_accept(
            TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c), message_id,
            &c->ctx->cfg->local_vmac, &c->ctx->cfg->local_uuid,
            c->ctx->cfg->max_bvlc_len, c->ctx->cfg->max_ndpu_len);

        if (len) {
            TX_BUF_UPDATE(c, len);
            DEBUG_PRINTF(
                "bsc_process_srv_awaiting_request() set socket %p to "
                "connected state\n",
                c);
            mstimer_set(
                &c->heartbeat, 2 * c->ctx->cfg->heartbeat_timeout_s * 1000);
            c->state = BSC_SOCK_STATE_CONNECTED;
            c->ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_CONNECTED, 0, NULL, NULL, 0, NULL);
            bws_srv_send(c->ctx->sh, c->wh);
        } else {
            DEBUG_PRINTF("bsc_process_srv_awaiting_request() sending of "
                         "connect accept failed, err = BSC_SC_NO_RESOURCES\n");
            if (c->ctx->funcs->failed_request) {
                c->ctx->funcs->failed_request(
                    c->ctx, c, &c->vmac, &c->uuid,
                    ERROR_CODE_ABORT_OUT_OF_RESOURCES, NULL);
            }
            bsc_srv_process_error(c, ERROR_CODE_ABORT_OUT_OF_RESOURCES);
        }
    }
#if DEBUG_BSC_SOCKET == 1
    else {
        DEBUG_PRINTF(
            "bsc_process_srv_awaiting_request() unexpected message "
            "with bvlc function "
            "%d is discarded in awaiting request state\n",
            dm->hdr.bvlc_function);
    }
#endif
    DEBUG_PRINTF("bsc_process_srv_awaiting_request() <<<\n");
}

static void bsc_dispatch_srv_func(
    BSC_WEBSOCKET_SRV_HANDLE sh,
    BSC_WEBSOCKET_HANDLE h,
    BSC_WEBSOCKET_EVENT ev,
    BACNET_ERROR_CODE ws_reason,
    char *ws_reason_desc,
    uint8_t *buf,
    size_t bufsize,
    void *dispatch_func_user_param)
{
    BSC_SOCKET_CTX *ctx = (BSC_SOCKET_CTX *)dispatch_func_user_param;
    BSC_SOCKET *c = NULL;
    BSC_WEBSOCKET_RET wret;
    uint8_t *p;
    bool failed = false;
    uint16_t len;
    size_t i;

    (void)sh;
    bws_dispatch_lock();
    DEBUG_PRINTF(
        "bsc_dispatch_srv_func() >>> sh = %p, h = %d, ev = %d, "
        "reason = %d, desc = %p, buf "
        "= %p, bufsize = %d, ctx = %p\n",
        sh, h, ev, ws_reason, ws_reason_desc, buf, bufsize, ctx);

    if (ev == BSC_WEBSOCKET_SERVER_STOPPED) {
        for (i = 0; i < ctx->sock_num; i++) {
            ctx->sock[i].state = BSC_SOCK_STATE_IDLE;
        }
        DEBUG_PRINTF("bsc_dispatch_srv_func() ctx %p is deinitialized\n", ctx);
        bsc_ctx_remove(ctx);
        ctx->state = BSC_CTX_STATE_IDLE;
        ctx->funcs->context_event(ctx, BSC_CTX_DEINITIALIZED);
        bsc_socket_maintenance_timer(0);
        DEBUG_PRINTF("bsc_dispatch_srv_func() <<<\n");
        bws_dispatch_unlock();
        return;
    } else if (ev == BSC_WEBSOCKET_SERVER_STARTED) {
        ctx->state = BSC_CTX_STATE_INITIALIZED;
        DEBUG_PRINTF("bsc_dispatch_srv_func() ctx %p is initialized\n", ctx);
        ctx->funcs->context_event(ctx, BSC_CTX_INITIALIZED);
        bsc_socket_maintenance_timer(0);
        DEBUG_PRINTF("bsc_dispatch_srv_func() <<<\n");
        bws_dispatch_unlock();
        return;
    }

    if (ev != BSC_WEBSOCKET_CONNECTED) {
        c = bsc_find_conn_by_websocket(ctx, h);
        if (!c) {
            DEBUG_PRINTF(
                "bsc_dispatch_srv_func() can not find socket "
                "descriptor for websocket %d\n",
                h);
            DEBUG_PRINTF("bsc_dispatch_srv_func() <<<\n");
            bws_dispatch_unlock();
            return;
        }
        DEBUG_PRINTF(
            "bsc_dispatch_srv_func() socket %p, state = %d\n", c, c->state);
    }

    if (ev == BSC_WEBSOCKET_DISCONNECTED) {
        if (c->state == BSC_SOCK_STATE_ERROR) {
            bsc_set_socket_idle(c);
            ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_DISCONNECTED, c->reason, NULL, NULL, 0,
                NULL);
            bsc_clear_vmac_and_uuid(c);
        } else {
            bsc_set_socket_idle(c);
            ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_DISCONNECTED, ws_reason, ws_reason_desc,
                NULL, 0, NULL);
            bsc_clear_vmac_and_uuid(c);
        }
    } else if (ev == BSC_WEBSOCKET_CONNECTED) {
        c = bsc_find_free_socket(ctx);
        if (!c) {
            DEBUG_PRINTF("bsc_dispatch_srv_func() no free socket, connection "
                         "is dropped\n");
            bws_srv_disconnect(ctx->sh, h);
        } else {
            bsc_reset_socket(c);
            c->wh = h;
            c->ctx = ctx;
            c->state = BSC_SOCK_STATE_AWAITING_REQUEST;
            mstimer_set(&c->t, c->ctx->cfg->connect_timeout_s * 1000);
        }
    } else if (ev == BSC_WEBSOCKET_RECEIVED) {
        DEBUG_PRINTF("bsc_dispatch_srv_func() processing "
                     "BSC_WEBSOCKET_RECEIVED event\n");
        DEBUG_PRINTF(
            "bsc_dispatch_srv_func() socket %p, state = %d\n", c, c->state);
        if (c->state == BSC_SOCK_STATE_AWAITING_REQUEST) {
            bsc_process_srv_awaiting_request(c, &bsc_dm, buf, bufsize);
        } else if (
            c->state == BSC_SOCK_STATE_DISCONNECTING ||
            c->state == BSC_SOCK_STATE_CONNECTED) {
            bsc_runloop_socket(c, &bsc_dm, buf, bufsize);
        }
#if DEBUG_BSC_SOCKET == 1
        else {
            DEBUG_PRINTF(
                "bsc_dispatch_srv_func() data was dropped for socket "
                "%p, state %d, data_size %d\n",
                c, c->state, bufsize);
        }
#endif
    } else if (ev == BSC_WEBSOCKET_SENDABLE) {
        p = c->tx_buf;

        while (c->tx_buf_size > 0) {
            memcpy(&len, p, sizeof(len));
            wret = bws_srv_dispatch_send(
                c->ctx->sh, c->wh, &p[sizeof(len) + BSC_CONF_TX_PRE], len);
            if (wret != BSC_WEBSOCKET_SUCCESS) {
                DEBUG_PRINTF(
                    "bsc_dispatch_srv_func() send data failed, start "
                    "disconnect operation on socket %p\n",
                    c);
                bsc_srv_process_error(
                    c,
                    c->state != BSC_SOCK_STATE_ERROR_FLUSH_TX
                        ? ERROR_CODE_ABORT_OUT_OF_RESOURCES
                        : c->reason);
                failed = true;
                break;
            } else {
                c->tx_buf_size -= len + sizeof(len) + BSC_CONF_TX_PRE;
                p += len + sizeof(len) + BSC_CONF_TX_PRE;
            }
        }

        if (!failed) {
            if (c->state == BSC_SOCK_STATE_ERROR_FLUSH_TX) {
                bsc_srv_process_error(c, c->reason);
            }
        }
    }

    bsc_socket_maintenance_timer(0);
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_dispatch_srv_func() <<<\n");
}

static void bsc_process_cli_awaiting_accept(
    BSC_SOCKET *c, BVLC_SC_DECODED_MESSAGE *dm, uint8_t *buf, size_t bufsize)
{
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    const char *err_desc = NULL;

    DEBUG_PRINTF(
        "bsc_process_cli_awaiting_accept() >>> c = %p, dm = %p, buf = "
        "%p, bufsize = %d\n",
        c, dm, buf, bufsize);

    if (!bvlc_sc_decode_message(buf, bufsize, dm, &code, &class, &err_desc)) {
        DEBUG_PRINTF(
            "bsc_process_cli_awaiting_accept() <<< decoding failed "
            "code = %d, class = %d\n",
            code, class);
        return;
    }

    if (dm->hdr.bvlc_function == BVLC_SC_CONNECT_ACCEPT) {
        if (dm->hdr.message_id != c->expected_connect_accept_message_id) {
            DEBUG_PRINTF(
                "bsc_process_cli_awaiting_accept() got bvlc result packet "
                "with unexpected message id %04x\n",
                dm->hdr.message_id);
        } else {
            DEBUG_PRINTF(
                "bsc_process_cli_awaiting_accept() set state of "
                "socket %p to BSC_SOCKET_EVENT_CONNECTED\n",
                c);
            bsc_copy_vmac(&c->vmac, dm->payload.connect_accept.vmac);
            bsc_copy_uuid(&c->uuid, dm->payload.connect_accept.uuid);
            c->max_bvlc_len = dm->payload.connect_accept.max_bvlc_len;
            c->max_npdu_len = dm->payload.connect_accept.max_npdu_len;
            mstimer_set(&c->heartbeat, c->ctx->cfg->heartbeat_timeout_s * 1000);
            c->state = BSC_SOCK_STATE_CONNECTED;
            c->ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_CONNECTED, 0, NULL, NULL, 0, NULL);
        }
    } else if (dm->hdr.bvlc_function == BVLC_SC_RESULT) {
        if (dm->payload.result.bvlc_function != BVLC_SC_CONNECT_REQUEST) {
            DEBUG_PRINTF(
                "bsc_process_cli_awaiting_accept() got unexpected "
                "bvlc function %d in "
                "BVLC-Result message in awaiting accept state\n",
                dm->payload.result.bvlc_function);
        } else if (
            dm->hdr.message_id != c->expected_connect_accept_message_id) {
            DEBUG_PRINTF(
                "bsc_process_cli_awaiting_accept() got bvlc result packet "
                "with unexpected message id %04x\n",
                dm->hdr.message_id);
        } else if (
            dm->payload.result.error_code == ERROR_CODE_NODE_DUPLICATE_VMAC) {
            /* According AB.6.2.2 BACnet/SC Connection Initiating */
            /* Peer State Machine: on receipt of a BVLC-Result NAK */
            /* message with an 'Error Code' of NODE_DUPLICATE_VMAC, */
            /* the initiating peer's node shall choose a new */
            /* Random-48 VMAC, close the WebSocket connection, and */
            /* enter the IDLE state. */
            /* Signal upper layer about that error */
            bsc_cli_process_error(c, ERROR_CODE_NODE_DUPLICATE_VMAC);
        }
#if DEBUG_BSC_SOCKET == 1
        else {
            DEBUG_PRINTF(
                "bsc_process_cli_awaiting_accept() got unexpected "
                "BVLC_RESULT error code "
                "%d in BVLC-Result message in awaiting accept state\n",
                dm->payload.result.error_code);
        }
#endif
    } else if (dm->hdr.bvlc_function == BVLC_SC_DISCONNECT_REQUEST) {
        /* AB.6.2.2 BACnet/SC Connection Initiating Peer State */
        /* Machine does not say anything about situation when */
        /* disconnect request is received from remote peer after */
        /* connect request. Handle this situation as an error, log */
        /* it and close connection. */
        DEBUG_PRINTF("bsc_process_cli_awaiting_accept() got unexpected "
                     "disconnect request\n");
        bsc_cli_process_error(c, ERROR_CODE_OTHER);
    } else if (dm->hdr.bvlc_function == BVLC_SC_DISCONNECT_ACK) {
        /* AB.6.2.2 BACnet/SC Connection Initiating Peer State */
        /* Machine does not say anything about situation when */
        /* disconnect ack is received from remote peer after connect */
        /* request. Handle this situation as an error, log it and */
        /* close connection. */
        DEBUG_PRINTF(
            "bsc_process_cli_awaiting_accept() got unexpected disconnect ack "
            "request\n");
        bsc_cli_process_error(c, ERROR_CODE_OTHER);
    }
#if DEBUG_BSC_SOCKET == 1
    else {
        DEBUG_PRINTF(
            "bsc_process_cli_awaiting_accept() unexpected message "
            "with bvlc function "
            "%d is discarded in awaiting accept state\n",
            dm->hdr.bvlc_function);
    }
#endif
    DEBUG_PRINTF("bsc_process_cli_awaiting_accept() <<<\n");
}

static void bsc_dispatch_cli_func(
    BSC_WEBSOCKET_HANDLE h,
    BSC_WEBSOCKET_EVENT ev,
    BACNET_ERROR_CODE ws_reason,
    char *ws_reason_desc,
    uint8_t *buf,
    size_t bufsize,
    void *dispatch_func_user_param)
{
    BSC_SOCKET_CTX *ctx = (BSC_SOCKET_CTX *)dispatch_func_user_param;
    BSC_SOCKET *c;
    uint16_t len;
    BSC_WEBSOCKET_RET wret;
    uint8_t *p;
    size_t i;
    bool all_socket_disconnected = true;
    bool failed = false;

    DEBUG_PRINTF(
        "bsc_dispatch_cli_func() >>> h = %d, ev = %d, reason = %d, "
        "reason_desc = %p, buf = %p, "
        "bufsize = %d, ctx = %p\n",
        h, ev, ws_reason, ws_reason_desc, buf, bufsize, ctx);

    bws_dispatch_lock();

    c = bsc_find_conn_by_websocket(ctx, h);

    if (!c) {
        DEBUG_PRINTF(
            "bsc_dispatch_cli_func() <<< warning, can not find "
            "connection object for websocket %d\n",
            h);
        bws_dispatch_unlock();
        return;
    }

    DEBUG_PRINTF("bsc_dispatch_cli_func() ev = %d, state = %d\n", ev, c->state);

    if (ev == BSC_WEBSOCKET_DISCONNECTED) {
        DEBUG_PRINTF("bsc_dispatch_cli_func() ctx->state = %d\n", ctx->state);
        if (ctx->state == BSC_CTX_STATE_DEINITIALIZING) {
            bsc_set_socket_idle(c);
            bsc_clear_vmac_and_uuid(c);
            for (i = 0; i < ctx->sock_num; i++) {
                if (ctx->sock[i].state != BSC_SOCK_STATE_IDLE) {
                    all_socket_disconnected = false;
                    break;
                }
            }
            if (all_socket_disconnected) {
                ctx->state = BSC_CTX_STATE_IDLE;
                bsc_ctx_remove(ctx);
                ctx->funcs->context_event(ctx, BSC_CTX_DEINITIALIZED);
            }
        } else if (c->state == BSC_SOCK_STATE_ERROR) {
            bsc_set_socket_idle(c);
            ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_DISCONNECTED, c->reason, NULL, NULL, 0,
                NULL);
            bsc_clear_vmac_and_uuid(c);
        } else {
            bsc_set_socket_idle(c);
            ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_DISCONNECTED, ws_reason, ws_reason_desc,
                NULL, 0, NULL);
            bsc_clear_vmac_and_uuid(c);
        }
    } else if (ev == BSC_WEBSOCKET_CONNECTED) {
        if (c->state == BSC_SOCK_STATE_AWAITING_WEBSOCKET) {
            DEBUG_PRINTF(
                "bsc_dispatch_cli_func() conn %p, websocket %d, state "
                "changed to BSC_SOCK_STATE_AWAITING_ACCEPT\n",
                c, h);
            c->state = BSC_SOCK_STATE_AWAITING_ACCEPT;
            mstimer_set(&c->t, c->ctx->cfg->connect_timeout_s * 1000);
            c->expected_connect_accept_message_id = bsc_get_next_message_id();
            DEBUG_PRINTF(
                "bsc_dispatch_cli_func() expected connect accept "
                "message id = %04x\n",
                c->expected_connect_accept_message_id);

            DEBUG_PRINTF(
                "bsc_dispatch_cli_func() going to send connect request "
                "with uuid %s and vmac %s\n",
                bsc_uuid_to_string(&ctx->cfg->local_uuid),
                bsc_vmac_to_string(&ctx->cfg->local_vmac));

            len = bvlc_sc_encode_connect_request(
                TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c),
                c->expected_connect_accept_message_id, &ctx->cfg->local_vmac,
                &ctx->cfg->local_uuid, ctx->cfg->max_bvlc_len,
                ctx->cfg->max_ndpu_len);

            if (!len) {
                bsc_cli_process_error(c, ERROR_CODE_ABORT_OUT_OF_RESOURCES);
            } else {
                TX_BUF_UPDATE(c, len);
            }
            bws_cli_send(c->wh);
        }
    } else if (ev == BSC_WEBSOCKET_SENDABLE) {
        p = c->tx_buf;

        while (c->tx_buf_size > 0) {
            memcpy(&len, p, sizeof(len));
            DEBUG_PRINTF(
                "bsc_dispatch_cli_func() sending pdu of %d bytes\n", len);
            wret = bws_cli_dispatch_send(
                c->wh, &p[sizeof(len) + BSC_CONF_TX_PRE], len);
            if (wret != BSC_WEBSOCKET_SUCCESS) {
                DEBUG_PRINTF(
                    "bsc_dispatch_cli_func() pdu send failed, err = %d, start "
                    "disconnect operation on socket %p\n",
                    wret, c);
                bsc_cli_process_error(
                    c,
                    c->state != BSC_SOCK_STATE_ERROR_FLUSH_TX
                        ? ERROR_CODE_ABORT_OUT_OF_RESOURCES
                        : c->reason);
                failed = true;
                break;
            } else {
                c->tx_buf_size -= len + sizeof(len) + BSC_CONF_TX_PRE;
                p += len + sizeof(len) + BSC_CONF_TX_PRE;
            }
        }
        if (!failed) {
            if (c->state == BSC_SOCK_STATE_ERROR_FLUSH_TX) {
                bsc_cli_process_error(c, c->reason);
            }
        }
    } else if (ev == BSC_WEBSOCKET_RECEIVED) {
        if (c->state == BSC_SOCK_STATE_AWAITING_ACCEPT) {
            bsc_process_cli_awaiting_accept(c, &bsc_dm, buf, bufsize);
        } else if (
            c->state == BSC_SOCK_STATE_DISCONNECTING ||
            c->state == BSC_SOCK_STATE_CONNECTED) {
            bsc_runloop_socket(c, &bsc_dm, buf, bufsize);
        }
#if DEBUG_BSC_SOCKET == 1
        else {
            DEBUG_PRINTF(
                "bsc_dispatch_cli_func() data was dropped for socket "
                "%p, state %d, data_size %d\n",
                c, c->state, bufsize);
        }
#endif
    }

    bsc_socket_maintenance_timer(0);
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_dispatch_cli_func() <<<\n");
}

BSC_SC_RET bsc_init_ctx(
    BSC_SOCKET_CTX *ctx,
    BSC_CONTEXT_CFG *cfg,
    BSC_SOCKET_CTX_FUNCS *funcs,
    BSC_SOCKET *sockets,
    size_t sockets_num,
    void *user_arg)
{
    BSC_WEBSOCKET_RET ret;
    BSC_SC_RET sc_ret = BSC_SC_SUCCESS;
    size_t i;

    DEBUG_PRINTF(
        "bsc_init_сtx() >>> ctx = %p, cfg = %p, funcs = %p, user_arg = %p\n",
        ctx, cfg, funcs, user_arg);

    if (!ctx || !cfg || !funcs || !funcs->socket_event ||
        !funcs->context_event || !sockets || !sockets_num) {
        DEBUG_PRINTF("bsc_init_ctx() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    if (cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
        if (!funcs->find_connection_for_vmac ||
            !funcs->find_connection_for_uuid) {
            DEBUG_PRINTF("bsc_init_ctx() <<< ret = BSC_SC_BAD_PARAM\n");
            return BSC_SC_BAD_PARAM;
        }
    }

    bws_dispatch_lock();
    if (ctx->state != BSC_CTX_STATE_IDLE) {
        bws_dispatch_unlock();
        DEBUG_PRINTF("bsc_init_ctx() <<< ret = BSC_SC_INVALID_OPERATION\n");
        return BSC_SC_INVALID_OPERATION;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->user_arg = user_arg;
    ctx->cfg = cfg;
    ctx->funcs = funcs;
    ctx->sock = sockets;
    ctx->sock_num = sockets_num;

    for (i = 0; i < sockets_num; i++) {
        bsc_set_socket_idle(&ctx->sock[i]);
    }

    ctx->state = BSC_CTX_STATE_INITIALIZING;
    if (!bsc_ctx_add(ctx)) {
        sc_ret = BSC_SC_NO_RESOURCES;
    } else {
        if (cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
            ret = bws_srv_start(
                cfg->proto, cfg->port, cfg->iface, cfg->ca_cert_chain,
                cfg->ca_cert_chain_size, cfg->cert_chain, cfg->cert_chain_size,
                cfg->priv_key, cfg->priv_key_size, cfg->connect_timeout_s,
                bsc_dispatch_srv_func, ctx, &ctx->sh);

            sc_ret = bsc_map_websocket_retcode(ret);

            if (sc_ret != BSC_SC_SUCCESS) {
                bsc_ctx_remove(ctx);
            }
        } else {
            ctx->state = BSC_CTX_STATE_INITIALIZED;
            ctx->funcs->context_event(ctx, BSC_CTX_INITIALIZED);
        }
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_init_ctx() <<< ret = %d \n", sc_ret);
    return sc_ret;
}

void bsc_deinit_ctx(BSC_SOCKET_CTX *ctx)
{
    size_t i;
    bool active_socket = false;
    DEBUG_PRINTF("bsc_deinit_ctx() >>> ctx = %p\n", ctx);

    bws_dispatch_lock();

    if (!ctx || ctx->state == BSC_CTX_STATE_IDLE ||
        ctx->state == BSC_CTX_STATE_DEINITIALIZING) {
        DEBUG_PRINTF("bsc_deinit_ctx() no action required\n");
        bws_dispatch_unlock();
        DEBUG_PRINTF("bsc_deinit_ctx() <<<\n");
        return;
    }

    if (ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
        ctx->state = BSC_CTX_STATE_DEINITIALIZING;
        for (i = 0; i < ctx->sock_num; i++) {
            if (ctx->sock[i].state != BSC_SOCK_STATE_IDLE) {
                active_socket = true;
                DEBUG_PRINTF(
                    "bsc_deinit_ctx() disconnect socket %d(%p) with wh = %d\n",
                    i, &ctx->sock[i], ctx->sock[i].wh);
                bws_cli_disconnect(ctx->sock[i].wh);
            }
        }
        if (!active_socket) {
            DEBUG_PRINTF(
                "bsc_deinit_ctx() no active sockets, ctx de-initialized\n");
            ctx->state = BSC_CTX_STATE_IDLE;
            bsc_ctx_remove(ctx);
            ctx->funcs->context_event(ctx, BSC_CTX_DEINITIALIZED);
        }
    } else {
        ctx->state = BSC_CTX_STATE_DEINITIALIZING;
        (void)bws_srv_stop(ctx->sh);
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_deinit_ctx() <<<\n");
}

BSC_SC_RET bsc_connect(BSC_SOCKET_CTX *ctx, BSC_SOCKET *c, char *url)
{
    BSC_SC_RET ret = BSC_SC_INVALID_OPERATION;
    BSC_WEBSOCKET_RET wret;

    DEBUG_PRINTF("bsc_connect() >>> ctx = %p, c = %p, url = %s\n", ctx, c, url);

    if (!ctx || !c || !url) {
        ret = BSC_SC_BAD_PARAM;
    } else {
        bws_dispatch_lock();

        if (ctx->state == BSC_CTX_STATE_INITIALIZED &&
            ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
            c->ctx = ctx;
            c->state = BSC_SOCK_STATE_AWAITING_WEBSOCKET;
            c->tx_buf_size = 0;

            wret = bws_cli_connect(
                ctx->cfg->proto, url, ctx->cfg->ca_cert_chain,
                ctx->cfg->ca_cert_chain_size, ctx->cfg->cert_chain,
                ctx->cfg->cert_chain_size, ctx->cfg->priv_key,
                ctx->cfg->priv_key_size, ctx->cfg->connect_timeout_s,
                bsc_dispatch_cli_func, ctx, &c->wh);

            ret = bsc_map_websocket_retcode(wret);
            if (wret != BSC_WEBSOCKET_SUCCESS) {
                bsc_set_socket_idle(c);
                bsc_clear_vmac_and_uuid(c);
            }
        }

        bws_dispatch_unlock();
    }

    DEBUG_PRINTF("bsc_connect() <<< ret = %d\n", ret);
    return ret;
}

void bsc_disconnect(BSC_SOCKET *c)
{
    uint16_t len;

    DEBUG_PRINTF("bsc_disconnect() >>> c = %p\n", c);

    bws_dispatch_lock();

    if (c->ctx->state == BSC_CTX_STATE_INITIALIZED) {
        if (c->state == BSC_SOCK_STATE_CONNECTED) {
            c->expected_disconnect_message_id = bsc_get_next_message_id();
            c->state = BSC_SOCK_STATE_DISCONNECTING;
            mstimer_set(&c->t, c->ctx->cfg->disconnect_timeout_s * 1000);
            len = bvlc_sc_encode_disconnect_request(
                TX_BUF_PTR(c), TX_BUF_BYTES_AVAIL(c),
                c->expected_disconnect_message_id);
            if (!len) {
                DEBUG_PRINTF(
                    "bsc_disconnect() disconnect request not sent, err = "
                    "BSC_SC_NO_RESOURCES\n");
                if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                    bsc_cli_process_error(c, ERROR_CODE_ABORT_OUT_OF_RESOURCES);
                } else {
                    bsc_srv_process_error(c, ERROR_CODE_ABORT_OUT_OF_RESOURCES);
                }
            } else {
                TX_BUF_UPDATE(c, len);
                if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                    bws_cli_send(c->wh);
                } else {
                    bws_srv_send(c->ctx->sh, c->wh);
                }
            }
        } else if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
            if (c->state != BSC_SOCK_STATE_IDLE) {
                bws_cli_disconnect(c->wh);
            }
        } else if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
            if (c->state != BSC_SOCK_STATE_IDLE) {
                bws_srv_disconnect(c->ctx->sh, c->wh);
            }
        }
    }

    bws_dispatch_unlock();

    DEBUG_PRINTF("bsc_disconnect() <<<\n");
}

BSC_SC_RET bsc_send(BSC_SOCKET *c, uint8_t *pdu, uint16_t pdu_len)
{
    BSC_SC_RET ret = BSC_SC_SUCCESS;

    DEBUG_PRINTF(
        "bsc_send() >>> c = %p, pdu = %p, pdu_len = %d\n", c, pdu, pdu_len);

    if (!c || !pdu || !pdu_len) {
        ret = BSC_SC_BAD_PARAM;
    } else {
        bws_dispatch_lock();

        if (c->ctx->state != BSC_CTX_STATE_INITIALIZED ||
            c->state != BSC_SOCK_STATE_CONNECTED) {
            ret = BSC_SC_INVALID_OPERATION;
        } else {
            if (TX_BUF_BYTES_AVAIL(c) < pdu_len) {
                ret = BSC_SC_NO_RESOURCES;
            } else {
                memcpy(TX_BUF_PTR(c), pdu, pdu_len);
                TX_BUF_UPDATE(c, pdu_len);
                if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                    bws_cli_send(c->wh);
                } else {
                    bws_srv_send(c->ctx->sh, c->wh);
                }
            }
        }

        bws_dispatch_unlock();
    }

    DEBUG_PRINTF("bsc_send() <<< ret = %d\n", ret);
    return ret;
}

uint16_t bsc_get_next_message_id(void)
{
    static uint16_t message_id;
    static bool initialized = false;
    uint16_t ret;

    bws_dispatch_lock();
    if (!initialized) {
        message_id = (uint16_t)(rand() % USHRT_MAX);
        initialized = true;
    } else {
        message_id++;
    }
    ret = message_id;
    DEBUG_PRINTF("next message id = %u(%04x)\n", ret, ret);
    bws_dispatch_unlock();
    return ret;
}

bool bsc_socket_get_peer_addr(BSC_SOCKET *c, BACNET_HOST_N_PORT_DATA *data)
{
    bool ret = false;

    if (!c || !data) {
        return false;
    }

    bws_dispatch_lock();
    if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
        data->type = BACNET_HOST_N_PORT_IP;
        ret = bws_srv_get_peer_ip_addr(
            c->ctx->sh, c->wh, (uint8_t *)data->host, sizeof(data->host),
            &data->port);
    }
    bws_dispatch_unlock();
    return ret;
}

BACNET_STACK_EXPORT
uint8_t *bsc_socket_get_global_buf(void)
{
    static uint8_t buf[BSC_PRE + BVLC_SC_NPDU_SIZE_CONF];
    return &buf[BSC_PRE];
}

BACNET_STACK_EXPORT
size_t bsc_socket_get_global_buf_size(void)
{
    return BVLC_SC_NPDU_SIZE_CONF;
}
