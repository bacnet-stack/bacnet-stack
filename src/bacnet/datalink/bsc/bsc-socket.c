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
#include "bacnet/datalink/bsc/bsc-mutex.h"
#include "bacnet/datalink/bsc/bsc-runloop.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/debug.h"

static const char *s_error_no_origin =
    "'Originating Virtual Address' field must be present";
static const char *s_error_dest_presented =
    "'Destination Virtual Address' field must be absent";
static const char *s_error_origin_presented =
    "'Originating Virtual Address' field must be absent";
static const char *s_error_no_dest =
    "'Destination Virtual Address' field must be present";

static void bsc_runloop(void *ctx);

void bsc_init_ctx_cfg(BSC_SOCKET_CTX_TYPE type,
    BSC_CONTEXT_CFG *cfg,
    BSC_WEBSOCKET_PROTOCOL proto,
    uint16_t port,
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
    debug_printf("bsc_init_ctx_cfg() >>> cfg = %p\n");
    if (cfg) {
        cfg->proto = proto;
        cfg->port = port;
        cfg->type = type;
        cfg->ca_cert_chain = ca_cert_chain;
        cfg->ca_cert_chain_size = ca_cert_chain_size;
        cfg->cert_chain = cert_chain;
        cfg->cert_chain_size = cert_chain_size;
        cfg->priv_key = key, cfg->priv_key_size = key_size;
        memcpy(&cfg->local_uuid, local_uuid, sizeof(*local_uuid));
        memcpy(&cfg->local_vmac, local_vmac, sizeof(*local_vmac));
        cfg->max_bvlc_len = max_local_bvlc_len;
        cfg->max_ndpu_len = max_local_ndpu_len;
        cfg->connect_timeout_s = connect_timeout_s;
        cfg->heartbeat_timeout_s = heartbeat_timeout_s;
        cfg->disconnect_timeout_s = disconnect_timeout_s;
    }
    debug_printf("bsc_init_ctx_cfg() <<<\n");
}

static BSC_SOCKET *bsc_find_conn_by_websocket(
    BSC_SOCKET_CTX *ctx, BSC_WEBSOCKET_HANDLE h)
{
    int i;
    for (i = 0; i < ctx->sock_num; i++) {
        if (ctx->sock[i].state != BSC_SOCK_STATE_IDLE && ctx->sock[i].wh == h) {
            return &ctx->sock[i];
        }
    }
    return NULL;
}

static BSC_SOCKET *bsc_find_free_socket(BSC_SOCKET_CTX *ctx)
{
    int i;
    for (i = 0; i < ctx->sock_num; i++) {
        if (ctx->sock[i].state == BSC_SOCK_STATE_IDLE) {
            return &ctx->sock[i];
        }
    }
    return NULL;
}

static void bsc_srv_process_error(BSC_SOCKET *c, BSC_SC_RET reason)
{
    debug_printf("bsc_srv_process_error) >>> c = %p, reason = %d\n", c, reason);
    c->state = BSC_SOCK_STATE_ERROR;
    c->disconnect_reason = reason;
    bws_srv_disconnect(c->ctx->cfg->proto, c->wh);
    debug_printf("bsc_srv_process_error) <<<\n");
}

static void bsc_cli_process_error(BSC_SOCKET *c, BSC_SC_RET reason)
{
    debug_printf("bsc_cli_process_error) >>> c = %p, reason = %d\n", c, reason);
    c->state = BSC_SOCK_STATE_ERROR;
    c->disconnect_reason = reason;
    bws_cli_disconnect(c->wh);
    debug_printf("bsc_cli_process_error) <<<\n");
}

static bool bsc_send_error_extended(BSC_SOCKET *c,
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
    unsigned int len;

    debug_printf(
        "bsc_send_error_extended() >>> bvlc_function = %d\n", bvlc_function);
    if (error_header_marker) {
        debug_printf("                              error_header_marker = %d\n",
            *error_header_marker);
    }
    if (error_class) {
        debug_printf(
            "                              error_class = %d\n", error_class);
    }
    if (error_code) {
        debug_printf(
            "                              error_code = %d\n", error_code);
    }
    if (utf8_details_string) {
        debug_printf("                              utf8_details_string = %s\n",
            utf8_details_string);
    }

    c->message_id++;
    len = bvlc_sc_encode_result(&c->tx_buf[c->tx_buf_size + sizeof(len)],
        (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
            ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
            : 0,
        c->message_id, origin, dest, bvlc_function, 1, error_header_marker,
        &eclass, &ecode, utf8_details_string);
    if (len) {
        memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
        c->tx_buf_size += sizeof(len) + len;
        if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
            bws_cli_send(c->wh);
        } else {
            bws_srv_send(c->ctx->cfg->proto, c->wh);
        }
        debug_printf("bsc_send_error_extended() <<< ret = true\n");
        return true;
    }
    debug_printf("bsc_send_error_extended() <<< ret = false\n");
    return false;
}

static bool bsc_send_protocol_error_extended(BSC_SOCKET *c,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *error_header_marker,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code,
    uint8_t *utf8_details_string)
{
    unsigned int len;

    if (bsc_is_unicast_message(&c->dm)) {
        return bsc_send_error_extended(c, origin, dest, BVLC_SC_RESULT,
            error_header_marker, error_class, error_code, utf8_details_string);
    }
    return false;
}

static bool bsc_send_protocol_error(BSC_SOCKET *c,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code,
    uint8_t *utf8_details_string)
{
    return bsc_send_protocol_error_extended(
        c, origin, dest, NULL, error_class, error_code, utf8_details_string);
}

static void bsc_process_socket_disconnecting(
    BSC_SOCKET *c, uint8_t *buf, uint16_t buflen)
{
    debug_printf("bsc_process_socket_disconnecting() >>> c = %p\n", c);

    if (c->dm.hdr.bvlc_function == BVLC_SC_DISCONNECT_ACK) {
        if (c->dm.hdr.message_id != c->expected_disconnect_message_id) {
            debug_printf(
                "bsc_process_socket_disconnecting() got disconect ack with "
                "unexpected message id %d for socket %p\n",
                c->dm.hdr.message_id, c);
        } else {
            debug_printf(
                "bsc_process_socket_disconnecting() got disconect ack for "
                "socket %p\n",
                c);
        }
        if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
            bws_cli_disconnect(c->wh);
        } else {
            bws_srv_disconnect(c->ctx->cfg->proto, c->wh);
        }
    } else if (c->dm.hdr.bvlc_function == BVLC_SC_RESULT) {
        if (c->dm.payload.result.bvlc_function == BVLC_SC_DISCONNECT_REQUEST &&
            c->dm.payload.result.result != 0) {
            debug_printf(
                "bsc_process_socket_disconnecting() got BVLC_SC_RESULT "
                "NAK on BVLC_SC_DISCONNECT_REQUEST\n");
            if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                bws_cli_disconnect(c->wh);
            } else {
                bws_srv_disconnect(c->ctx->cfg->proto, c->wh);
            }
        }
    } else {
        c->ctx->funcs->socket_event(
            c, BSC_SOCKET_EVENT_RECEIVED, BSC_SC_SUCCESS, buf, buflen, &c->dm);
    }

    debug_printf("bsc_process_socket_disconnecting() <<<\n");
}

static void bsc_process_socket_connected_state(
    BSC_SOCKET *c, uint8_t *buf, uint16_t buflen)
{
    uint16_t message_id;
    uint16_t len;

    debug_printf("bsc_process_socket_connected_state() >>> c = %p, buf = %p, "
                 "buflen = %d\n",
        c, buf, buflen);

    if (c->dm.hdr.bvlc_function == BVLC_SC_HEARTBEAT_ACK) {
        if (c->dm.hdr.message_id != c->expected_heartbeat_message_id) {
            debug_printf(
                "bsc_process_socket_connected_state() got heartbeat ack with "
                "unexpected message id %d for socket %p\n",
                c->dm.hdr.message_id, c);
        } else {
            debug_printf(
                "bsc_process_socket_connected_state() got heartbeat ack for "
                "socket %p\n",
                c);
        }
    } else if (c->dm.hdr.bvlc_function == BVLC_SC_HEARTBEAT_REQUEST) {
        debug_printf("bsc_process_socket_connected_state() got heartbeat "
                     "request with message_id %d\n",
            c->dm.hdr.message_id);
        message_id = c->dm.hdr.message_id;
        len = bvlc_sc_encode_heartbeat_ack(
            &c->tx_buf[c->tx_buf_size + sizeof(len)],
            (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
                ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
                : 0,
            message_id);
        if (len) {
            memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
            c->tx_buf_size += len + sizeof(len);
            if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
                bws_srv_send(c->ctx->cfg->proto, c->wh);
            } else {
                bws_cli_send(c->wh);
            }
        } else {
            debug_printf("bsc_process_socket_connected_state() no resources to "
                         "answer on hearbeat request "
                         "socket %p\n",
                c);
        }
    } else if (c->dm.hdr.bvlc_function == BVLC_SC_DISCONNECT_REQUEST) {
        debug_printf("bsc_process_socket_connected_state() got disconnect "
                     "request with message_id %d\n",
            c->dm.hdr.message_id);
        c->message_id++;
        len = bvlc_sc_encode_disconnect_ack(
            &c->tx_buf[c->tx_buf_size + sizeof(len)],
            (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
                ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
                : 0,
            c->message_id);
        if (len) {
            memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
            c->tx_buf_size += len + sizeof(len);
            c->disconnect_reason = BSC_SC_PEER_DISCONNECTED;
            c->state = BSC_SOCK_STATE_ERROR_FLUSH_TX;
            if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
                bws_srv_send(c->ctx->cfg->proto, c->wh);
            } else {
                bws_cli_send(c->wh);
            }
        } else {
            debug_printf(
                "bsc_process_socket_connected_state() no resources to answer "
                "on disconnect request, just disconnecting without ack\n");
            c->state = BSC_SOCK_STATE_DISCONNECTING;
            if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
                bws_srv_disconnect(c->ctx->cfg->proto, c->wh);
            } else {
                bws_cli_disconnect(c->wh);
            }
        }
    } else if (c->dm.hdr.bvlc_function == BVLC_SC_DISCONNECT_ACK) {
        // This is unexpected! We assume that the remote peer is confused and
        // thought we sent a Disconnect-Request so we'll close the socket
        // and hope that remote peer clears itself up.
        debug_printf("bsc_process_socket_connected_state() got unexpected "
                     "disconnect ack with message_id %d\n",
            c->dm.hdr.message_id);
        c->state = BSC_SOCK_STATE_DISCONNECTING;
        if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
            bws_srv_disconnect(c->ctx->cfg->proto, c->wh);
        } else {
            bws_cli_disconnect(c->wh);
        }
    } else if (c->dm.hdr.bvlc_function == BVLC_SC_RESULT) {
        if (!c->dm.hdr.dest && !c->dm.hdr.origin) {
            debug_printf("bsc_process_socket_connected_state() got unexpected "
                         "bvlc result, message is dropped\n");
        } else {
            debug_printf(
                "bsc_process_socket_connected_state() emit received event "
                "buf = %p, size = %d\n",
                buf, buflen);
            c->ctx->funcs->socket_event(c, BSC_SOCKET_EVENT_RECEIVED,
                BSC_SC_SUCCESS, buf, buflen, &c->dm);
        }
    } else if (c->dm.hdr.bvlc_function == BVLC_SC_ENCAPSULATED_NPDU ||
        c->dm.hdr.bvlc_function == BVLC_SC_ADDRESS_RESOLUTION ||
        c->dm.hdr.bvlc_function == BVLC_SC_ADDRESS_RESOLUTION_ACK ||
        c->dm.hdr.bvlc_function == BVLC_SC_ADVERTISIMENT ||
        c->dm.hdr.bvlc_function == BVLC_SC_ADVERTISIMENT_SOLICITATION ||
        c->dm.hdr.bvlc_function == BVLC_SC_PROPRIETARY_MESSAGE) {
        debug_printf("bsc_process_socket_connected_state() emit received event "
                     "buf = %p, size = %d\n",
            buf, buflen);
        c->ctx->funcs->socket_event(
            c, BSC_SOCKET_EVENT_RECEIVED, BSC_SC_SUCCESS, buf, buflen, &c->dm);
    }

    debug_printf("bsc_process_socket_connected_state() <<<\n");
}

static void bsc_process_socket_state(BSC_SOCKET *c)
{
    bool expired;
    uint16_t len;
    uint8_t *p;
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    const char *err_desc = NULL;

    debug_printf(
        "bsc_process_socket_state() >>> ctx = %p, c = %p, state = %d\n", c->ctx,
        c, c->state);
    debug_printf(
        "bsc_process_socket_state() %d bytes are in rx buf\n", c->rx_buf_size);

    p = c->rx_buf;
    while (c->rx_buf_size) {
        memcpy(&len, p, sizeof(len));
        p += sizeof(len);
        c->rx_buf_size -= len + sizeof(len);

        if (!bvlc_sc_decode_message(p, len, &c->dm, &code, &class, &err_desc)) {
            // if code == ERROR_CODE_OTHER that means that received bvlc message
            // has length less than 4 octets.
            // According EA-001-4 'Clarifying BVLC-Result in BACnet/SC '
            // If a BVLC message is received that has fewer than four octets, a
            // BVLC-Result NAK shall not be returned.
            // The message shall be discarded and not be processed.
            if (code != ERROR_CODE_OTHER) {
                bsc_send_protocol_error(c, c->dm.hdr.origin, c->dm.hdr.dest,
                    class, code, (uint8_t *)err_desc);
            } else {
                debug_printf(
                    "bsc_process_socket_state() decoding failed, message "
                    "is silently dropped cause it's length < 4 bytes\n");
            }
        } else {
            if (c->dm.hdr.bvlc_function == BVLC_SC_ENCAPSULATED_NPDU ||
                c->dm.hdr.bvlc_function == BVLC_SC_ADVERTISIMENT ||
                c->dm.hdr.bvlc_function == BVLC_SC_ADDRESS_RESOLUTION_ACK ||
                c->dm.hdr.bvlc_function == BVLC_SC_ADDRESS_RESOLUTION ||
                c->dm.hdr.bvlc_function == BVLC_SC_ADVERTISIMENT_SOLICITATION ||
                c->dm.hdr.bvlc_function == BVLC_SC_RESULT) {
                if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR &&
                    c->ctx->cfg->proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
                    // this is a case when socket is a hub connector receiving
                    // from hub
                    if (c->dm.hdr.origin == NULL &&
                        c->dm.hdr.bvlc_function != BVLC_SC_RESULT) {
                        class = ERROR_CLASS_COMMUNICATION;
                        code = ERROR_CODE_HEADER_ENCODING_ERROR;

                        bsc_send_protocol_error(c, NULL, NULL, class, code,
                            (uint8_t *)s_error_no_origin);
                        continue;
                    } else if (c->dm.hdr.dest != NULL &&
                        !bsc_is_vmac_broadcast(c->dm.hdr.dest)) {
                        class = ERROR_CLASS_COMMUNICATION;
                        code = ERROR_CODE_HEADER_ENCODING_ERROR;

                        bsc_send_protocol_error(c, NULL, NULL, class, code,
                            (uint8_t *)s_error_dest_presented);
                        continue;
                    }
                } else if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR &&
                    c->ctx->cfg->proto == BSC_WEBSOCKET_HUB_PROTOCOL) {
                    //  this is a case when socket is hub  function receiving
                    //  from node
                    if (c->dm.hdr.dest == NULL) {
                        class = ERROR_CLASS_COMMUNICATION;
                        code = ERROR_CODE_HEADER_ENCODING_ERROR;

                        bsc_send_protocol_error(c, NULL, NULL, class, code,
                            (uint8_t *)s_error_no_dest);
                        continue;
                    } else if (c->dm.hdr.origin != NULL) {
                        class = ERROR_CLASS_COMMUNICATION;
                        code = ERROR_CODE_HEADER_ENCODING_ERROR;

                        bsc_send_protocol_error(c, NULL, NULL, class, code,
                            (uint8_t *)s_error_origin_presented);
                        continue;
                    }
                }
            }

            // every valid message restarts hearbeat timeout
            if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
                mstimer_set(
                    &c->heartbeat, 2 * c->ctx->cfg->heartbeat_timeout_s * 1000);
            } else {
                mstimer_set(
                    &c->heartbeat, c->ctx->cfg->heartbeat_timeout_s * 1000);
            }
            if (c->state == BSC_SOCK_STATE_CONNECTED) {
                bsc_process_socket_connected_state(c, p, len);
            } else if (c->state == BSC_SOCK_STATE_DISCONNECTING) {
                bsc_process_socket_disconnecting(c, p, len);
            }
        }
    }

    expired = mstimer_expired(&c->t);
    debug_printf("bsc_process_socket_state() expired = %d\n", expired);

    if (c->state == BSC_SOCK_STATE_AWAITING_ACCEPT && expired) {
        c->state = BSC_SOCK_STATE_ERROR;
        c->disconnect_reason = BSC_SC_TIMEDOUT;
        bws_cli_disconnect(c->wh);
    } else if (c->state == BSC_SOCK_STATE_AWAITING_REQUEST && expired) {
        c->state = BSC_SOCK_STATE_ERROR;
        c->disconnect_reason = BSC_SC_TIMEDOUT;
        bws_srv_disconnect(c->ctx->cfg->proto, c->wh);
    } else if (c->state == BSC_SOCK_STATE_DISCONNECTING && expired) {
        c->state = BSC_SOCK_STATE_ERROR;
        c->disconnect_reason = BSC_SC_TIMEDOUT;
        if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
            bws_cli_disconnect(c->wh);
        } else {
            bws_srv_disconnect(c->ctx->cfg->proto, c->wh);
        }
    } else if (c->state == BSC_SOCK_STATE_CONNECTED) {
        expired = mstimer_expired(&c->heartbeat);
        debug_printf(
            "bsc_process_socket_state() hearbeat timeout expired = %d\n",
            expired);
        if (expired) {
            debug_printf("bsc_process_socket_state() heartbeat timeout elapsed "
                         "for socket %p\n",
                c);
            if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                debug_printf(
                    "bsc_process_socket_state() going to send heartbeat "
                    "request on connection %p\n",
                    c);
                c->message_id++;
                c->expected_heartbeat_message_id = c->message_id;
                debug_printf(
                    "bsc_process_socket_state() heartbeat message_id %04x\n",
                    c->expected_heartbeat_message_id);

                len = bvlc_sc_encode_heartbeat_request(
                    &c->tx_buf[c->tx_buf_size + sizeof(len)],
                    (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
                        ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
                        : 0,
                    c->message_id);

                if (len) {
                    memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
                    c->tx_buf_size += sizeof(len) + len;
                    bws_cli_send(c->wh);
                    mstimer_set(
                        &c->heartbeat, c->ctx->cfg->heartbeat_timeout_s * 1000);
                } else {
                    debug_printf("bsc_process_socket_state() sending of "
                                 "heartbeat request failed on connection %p\n",
                        c);
                }
            } else if (c->ctx->cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
                debug_printf("bsc_process_socket_state() zombie socket %p is "
                             "disconnecting...\n",
                    c);
                bws_srv_disconnect(c->ctx->cfg->proto, c->wh);
            }
        }
    }
    debug_printf("bsc_process_socket_state() <<<\n");
}

static void bsc_runloop(void *p)
{
    BSC_SOCKET_CTX *ctx = (BSC_SOCKET_CTX *)p;
    // debug_printf("bsc_runloop() >>> ctx = %p, state = %d\n", ctx,
    // ctx->state);
    bsc_global_mutex_lock();
    if (ctx->state == BSC_CTX_STATE_INITIALIZED) {
        for (int i = 0; i < ctx->sock_num; i++) {
            if (ctx->sock[i].state != BSC_SOCK_STATE_IDLE) {
                bsc_process_socket_state(&ctx->sock[i]);
            }
        }
    }
    bsc_global_mutex_unlock();
    // debug_printf("bsc_runloop() <<<\n");
}

static void bsc_process_srv_awaiting_request(
    BSC_SOCKET *c, uint8_t *buf, size_t bufsize)
{
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    BSC_SOCKET *existing = NULL;
    uint16_t message_id;
    uint16_t len;
    uint16_t ucode;
    uint16_t uclass;
    const char *err_desc = NULL;

    debug_printf("bsc_process_srv_awaiting_request() >>> c = %p, buf = %p, "
                 "bufsize = %d\n",
        c, buf, bufsize);

    if (!bvlc_sc_decode_message(
            buf, bufsize, &c->dm, &code, &class, &err_desc)) {
        debug_printf(
            "bsc_process_srv_awaiting_request() decoding of received message "
            "failed, error code = %d, class = %d\n",
            code, class);
    } else if (c->dm.hdr.bvlc_function != BVLC_SC_CONNECT_REQUEST) {
        debug_printf("bsc_process_srv_awaiting_request() unexpected message "
                     "with bvlc function "
                     "%d is discarded in awaiting request state\n",
            c->dm.hdr.bvlc_function);
    } else {
        existing = c->ctx->funcs->find_connection_for_uuid(
            c->dm.payload.connect_request.uuid);

        if (existing) {
            // Regarding AB.6.2.3 BACnet/SC Connection Accepting Peer
            // State Machine: On receipt of a Connect-Request message
            // from the initiating peer whose 'Device UUID' is equal to the
            // initiating peer device UUID of an existing connection,
            // then return a Connect-Accept message, disconnect and
            // close the existing connection to the connection peer node
            // with matching Device UUID, and enter the CONNECTED state.
            debug_printf(
                "bsc_process_srv_awaiting_request() accepting connection from "
                "known uuid %s\n and vmac %s\n",
                bsc_uuid_to_string(c->dm.payload.connect_request.uuid),
                bsc_vmac_to_string(c->dm.payload.connect_request.vmac));

            memcpy(
                &c->vmac, c->dm.payload.connect_request.vmac, sizeof(c->vmac));
            memcpy(
                &c->uuid, c->dm.payload.connect_request.uuid, sizeof(c->uuid));
            c->max_npdu_len = c->dm.payload.connect_request.max_npdu_len;
            c->max_bvlc_len = c->dm.payload.connect_request.max_bvlc_len;
            message_id = c->dm.hdr.message_id;

            len = bvlc_sc_encode_connect_accept(
                &c->tx_buf[c->tx_buf_size + sizeof(len)],
                (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
                    ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
                    : 0,
                message_id, &c->ctx->cfg->local_vmac, &c->ctx->cfg->local_uuid,
                c->ctx->cfg->max_bvlc_len, c->ctx->cfg->max_ndpu_len);

            if (!len) {
                bsc_srv_process_error(c, BSC_SC_NO_RESOURCES);
                debug_printf("bsc_process_srv_awaiting_request() <<<\n");
                return;
            } else {
                memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
                c->tx_buf_size += len + sizeof(len);
            }

            bws_srv_send(c->ctx->cfg->proto, c->wh);

            existing->message_id++;

            len = bvlc_sc_encode_disconnect_request(
                &existing->tx_buf[existing->tx_buf_size + sizeof(len)],
                (sizeof(existing->tx_buf) - existing->tx_buf_size >=
                    sizeof(len))
                    ? (sizeof(existing->tx_buf) - existing->tx_buf_size -
                          sizeof(len))
                    : 0,
                existing->message_id);

            if (len) {
                memcpy(&existing->tx_buf[existing->tx_buf_size], &len,
                    sizeof(len));
                existing->tx_buf_size += len + sizeof(len);
                bws_srv_send(existing->ctx->cfg->proto, existing->wh);
            } else {
                debug_printf(
                    "bsc_process_srv_awaiting_request() sending of disconnect "
                    "request failed, err = BSC_SC_NO_RESOURCES\n");
            }
            debug_printf("bsc_process_srv_awaiting_request() set socket %p to "
                         "connected state\n",
                c);

            mstimer_set(
                &c->heartbeat, 2 * c->ctx->cfg->heartbeat_timeout_s * 1000);
            c->state = BSC_SOCK_STATE_CONNECTED;
            c->ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_CONNECTED, BSC_SC_SUCCESS, NULL, 0, NULL);
            debug_printf("bsc_process_srv_awaiting_request() <<<\n");
            return;
        }

        existing = c->ctx->funcs->find_connection_for_vmac(
            c->dm.payload.connect_request.vmac);

        if (existing) {
            debug_printf(
                "bsc_process_srv_awaiting_request() rejected connection for "
                "duplicated vmac %s from uuid %s,"
                " vmac is used by uuid %s\n",
                bsc_vmac_to_string(c->dm.payload.connect_request.vmac),
                bsc_uuid_to_string(c->dm.payload.connect_request.uuid),
                bsc_uuid_to_string(&existing->uuid));

            uclass = ERROR_CLASS_COMMUNICATION;
            ucode = ERROR_CODE_NODE_DUPLICATE_VMAC;
            message_id = c->dm.hdr.message_id;

            len =
                bvlc_sc_encode_result(&c->tx_buf[c->tx_buf_size + sizeof(len)],
                    (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
                        ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
                        : 0,
                    message_id, NULL, NULL, BVLC_SC_CONNECT_REQUEST, 1, NULL,
                    &ucode, &uclass, NULL);

            if (len) {
                memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
                c->tx_buf_size += len + sizeof(len);
                c->state = BSC_SOCK_STATE_ERROR_FLUSH_TX;
                c->disconnect_reason = BSC_SC_DUPLICATED_VMAC;
                bws_srv_send(c->ctx->cfg->proto, c->wh);
            } else {
                debug_printf(
                    "bsc_process_srv_awaiting_request() sending of nack result "
                    "message failed, err = BSC_SC_NO_RESOURCES\n");
                bsc_srv_process_error(c, BSC_SC_DUPLICATED_VMAC);
            }
            debug_printf("bsc_process_srv_awaiting_request() <<<\n");
            return;
        }

        if (memcmp(&c->vmac, &c->ctx->cfg->local_vmac,
                sizeof(c->ctx->cfg->local_vmac)) == 0 &&
            memcmp(&c->uuid, &c->ctx->cfg->local_uuid,
                sizeof(c->ctx->cfg->local_uuid)) != 0) {
            debug_printf("bsc_process_srv_awaiting_request() rejected "
                         "connection of a duplicate "
                         "of this port's vmac %s from uuid %s\n",
                bsc_vmac_to_string(&c->vmac), bsc_uuid_to_string(&c->uuid));
            uclass = ERROR_CLASS_COMMUNICATION;
            ucode = ERROR_CODE_NODE_DUPLICATE_VMAC;
            message_id = c->dm.hdr.message_id;

            len =
                bvlc_sc_encode_result(&c->tx_buf[c->tx_buf_size + sizeof(len)],
                    (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
                        ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
                        : 0,
                    message_id, NULL, NULL, BVLC_SC_CONNECT_REQUEST, 1, NULL,
                    &ucode, &uclass, NULL);

            if (len) {
                memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
                c->tx_buf_size += len + sizeof(len);
                c->state = BSC_SOCK_STATE_ERROR_FLUSH_TX;
                c->disconnect_reason = BSC_SC_DUPLICATED_VMAC;
                bws_srv_send(c->ctx->cfg->proto, c->wh);
            } else {
                debug_printf(
                    "bsc_process_srv_awaiting_request() sending of nack result "
                    "message failed, err =  BSC_SC_NO_RESOURCES\n");
                bsc_srv_process_error(c, BSC_SC_DUPLICATED_VMAC);
            }
            debug_printf("bsc_process_srv_awaiting_request() <<<\n");
            return;
        }
        memcpy(&c->vmac, &c->dm.payload.connect_request.vmac, sizeof(c->vmac));
        memcpy(&c->uuid, &c->dm.payload.connect_request.uuid, sizeof(c->uuid));
        debug_printf(
            "bsc_process_srv_awaiting_request() accepted connection from new "
            "uuid %s with vmac %d\n",
            bsc_vmac_to_string(&c->vmac), bsc_uuid_to_string(&c->uuid));

        message_id = c->dm.hdr.message_id;

        len = bvlc_sc_encode_connect_accept(
            &c->tx_buf[c->tx_buf_size + sizeof(len)],
            (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
                ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
                : 0,
            message_id, &c->ctx->cfg->local_vmac, &c->ctx->cfg->local_uuid,
            c->ctx->cfg->max_bvlc_len, c->ctx->cfg->max_ndpu_len);

        if (len) {
            memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
            c->tx_buf_size += len + sizeof(len);
            debug_printf("bsc_process_srv_awaiting_request() set socket %p to "
                         "connected state\n",
                c);
            mstimer_set(
                &c->heartbeat, 2 * c->ctx->cfg->heartbeat_timeout_s * 1000);
            c->state = BSC_SOCK_STATE_CONNECTED;
            c->ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_CONNECTED, BSC_SC_SUCCESS, NULL, 0, NULL);
            bws_srv_send(c->ctx->cfg->proto, c->wh);
        } else {
            debug_printf("bsc_process_srv_awaiting_request() sending of "
                         "connect accept failed, err = BSC_SC_NO_RESOURCES\n");
            bsc_srv_process_error(c, BSC_SC_NO_RESOURCES);
        }
    }

    debug_printf("bsc_process_srv_awaiting_request() <<<\n");
}

static void bsc_dispatch_srv_func(BSC_WEBSOCKET_PROTOCOL proto,
    BSC_WEBSOCKET_HANDLE h,
    BSC_WEBSOCKET_EVENT ev,
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
    int i;

    debug_printf("bsc_dispatch_srv_func() >>> proto = %d, h = %d, ev = %d, buf "
                 "= %p, bufsize = %d, ctx = %p\n",
        proto, h, ev, buf, bufsize, ctx);

    bsc_global_mutex_lock();

    if (ev == BSC_WEBSOCKET_SERVER_STOPPED) {
        for (i = 0; i < ctx->sock_num; i++) {
            ctx->sock[i].state = BSC_SOCK_STATE_IDLE;
        }
        debug_printf("bsc_dispatch_srv_func() ctx %p is deinitialized\n", ctx);
        bsc_runloop_unreg(ctx);
        ctx->state = BSC_CTX_STATE_IDLE;
        ctx->funcs->context_event(ctx, BSC_CTX_DEINITIALIZED);
        debug_printf("bsc_dispatch_srv_func() <<<\n");
        bsc_global_mutex_unlock();
        return;
    } else if (ev == BSC_WEBSOCKET_SERVER_STARTED) {
        ctx->state = BSC_CTX_STATE_INITIALIZED;
        debug_printf("bsc_dispatch_srv_func() ctx %p is initialized\n", ctx);
        ctx->funcs->context_event(ctx, BSC_CTX_INITIALIZED);
        debug_printf("bsc_dispatch_srv_func() <<<\n");
        bsc_global_mutex_unlock();
        return;
    }

    if (ev != BSC_WEBSOCKET_CONNECTED) {
        c = bsc_find_conn_by_websocket(ctx, h);
        if (!c) {
            debug_printf("bsc_dispatch_srv_func() can not find socket "
                         "descriptor for websocket %d\n",
                h);
            debug_printf("bsc_dispatch_srv_func() <<<\n");
            bsc_global_mutex_unlock();
            return;
        }
        debug_printf(
            "bsc_dispatch_srv_func() socket %p, state = %d\n", c, c->state);
    }

    if (ev == BSC_WEBSOCKET_DISCONNECTED) {
        c->state = BSC_SOCK_STATE_IDLE;
        c->wh = BSC_SOCKET_EVENT_DISCONNECTED;
        if (c->state == BSC_SOCK_STATE_ERROR) {
            ctx->funcs->socket_event(c, BSC_SOCKET_EVENT_DISCONNECTED,
                c->disconnect_reason, NULL, 0, NULL);
        } else {
            ctx->funcs->socket_event(c, BSC_SOCKET_EVENT_DISCONNECTED,
                BSC_SC_PEER_DISCONNECTED, NULL, 0, NULL);
        }
    } else if (ev == BSC_WEBSOCKET_CONNECTED) {
        c = bsc_find_free_socket(ctx);
        if (!c) {
            debug_printf("bsc_dispatch_srv_func() no free socket, connection "
                         "is dropped\n");
            bws_srv_disconnect(proto, h);
        } else {
            c->wh = h;
            c->ctx = ctx;
            c->tx_buf_size = 0;
            c->rx_buf_size = 0;
            c->state = BSC_SOCK_STATE_AWAITING_REQUEST;
            mstimer_set(&c->t, c->ctx->cfg->connect_timeout_s * 1000);
        }
    } else if (ev == BSC_WEBSOCKET_RECEIVED) {
        debug_printf("bsc_dispatch_srv_func() processing "
                     "BSC_WEBSOCKET_RECEIVED event\n");
        debug_printf(
            "bsc_dispatch_srv_func() socket %p, state = %d\n", c, c->state);
        if (c->state == BSC_SOCK_STATE_AWAITING_REQUEST) {
            bsc_process_srv_awaiting_request(c, buf, bufsize);
        } else if (c->state == BSC_SOCK_STATE_DISCONNECTING ||
            c->state == BSC_SOCK_STATE_CONNECTED) {
            debug_printf("bsc_dispatch_srv_func() c->rx_buf_size = %d\n",
                c->rx_buf_size);
            if (bufsize > BVLC_SC_NPDU_MAX_SIZE) {
                debug_printf("bsc_dispatch_srv_func() message is dropped, size "
                             "> BVLC_SC_NPDU_MAX_SIZE, socket %p, state %d, "
                             "data_size %d\n",
                    c, c->state, bufsize);
            } else if (sizeof(c->rx_buf) - c->rx_buf_size - sizeof(len) >=
                bufsize) {
                len = (uint16_t)bufsize;
                memcpy(&c->rx_buf[c->rx_buf_size], &len, sizeof(len));
                c->rx_buf_size += sizeof(len);
                memcpy(&c->rx_buf[c->rx_buf_size], buf, bufsize);
                c->rx_buf_size += bufsize;
                bsc_runloop_schedule();
            } else {
                debug_printf("bsc_dispatch_srv_func() no space in rx_buf, "
                             "message is dropped,  socket %p, state %d, "
                             "data_size %d, rx_buf_size = %d\n",
                    c, c->state, bufsize, c->rx_buf_size);
            }
        } else {
            debug_printf("bsc_dispatch_srv_func() data was dropped for socket "
                         "%p, state %d, data_size %d\n",
                c, c->state, bufsize);
        }
    } else if (ev == BSC_WEBSOCKET_SENDABLE) {
        p = c->tx_buf;

        while (c->tx_buf_size > 0) {
            memcpy(&len, p, sizeof(len));
            wret = bws_srv_dispatch_send(proto, c->wh, &p[sizeof(len)], len);
            if (wret != BSC_WEBSOCKET_SUCCESS) {
                debug_printf("bsc_dispatch_srv_func() send data failed, start "
                             "disconnect operation on socket %p\n",
                    c);
                bsc_srv_process_error(c, bsc_map_websocket_retcode(wret));
                failed = true;
                break;
            } else {
                c->tx_buf_size -= len + sizeof(len);
                p += len + sizeof(len);
            }
        }

        if (!failed) {
            if (c->state == BSC_SOCK_STATE_ERROR_FLUSH_TX) {
                bsc_srv_process_error(c, c->disconnect_reason);
            }
        }
    }

    bsc_global_mutex_unlock();
    debug_printf("bsc_dispatch_srv_func() <<<\n");
}

static void bsc_process_cli_awaiting_accept(
    BSC_SOCKET *c, uint8_t *buf, size_t bufsize)
{
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    const char *err_desc = NULL;

    debug_printf("bsc_process_cli_awaiting_accept() >>> c = %p\n", c);

    if (!bvlc_sc_decode_message(
            buf, bufsize, &c->dm, &code, &class, &err_desc)) {
        debug_printf("bsc_process_cli_awaiting_accept() <<< decoding failed "
                     "code = %d, class = %d\n",
            code, class);
        return;
    }

    if (c->dm.hdr.bvlc_function == BVLC_SC_CONNECT_ACCEPT) {
        if (c->dm.hdr.message_id != c->expected_connect_accept_message_id) {
            debug_printf(
                "bsc_process_cli_awaiting_accept() got bvlc result packet "
                "with unexpected message id %04x\n",
                c->dm.hdr.message_id);
        } else {
            debug_printf("bsc_process_cli_awaiting_accept() set state of "
                         "socket %p to BSC_SOCKET_EVENT_CONNECTED\n",
                c);
            memcpy(
                &c->vmac, c->dm.payload.connect_accept.vmac, sizeof(c->vmac));
            memcpy(
                &c->uuid, c->dm.payload.connect_accept.uuid, sizeof(c->uuid));
            c->max_bvlc_len = c->dm.payload.connect_accept.max_bvlc_len;
            c->max_npdu_len = c->dm.payload.connect_accept.max_npdu_len;
            mstimer_set(&c->heartbeat, c->ctx->cfg->heartbeat_timeout_s * 1000);
            c->state = BSC_SOCK_STATE_CONNECTED;
            c->ctx->funcs->socket_event(
                c, BSC_SOCKET_EVENT_CONNECTED, BSC_SC_SUCCESS, NULL, 0, NULL);
        }
    } else if (c->dm.hdr.bvlc_function == BVLC_SC_RESULT) {
        if (c->dm.payload.result.bvlc_function != BVLC_SC_CONNECT_REQUEST) {
            debug_printf("bsc_process_cli_awaiting_accept() got unexpected "
                         "bvlc function %d in "
                         "BVLC-Result message in awaiting accept state\n",
                c->dm.payload.result.bvlc_function);
        } else if (c->dm.hdr.message_id !=
            c->expected_connect_accept_message_id) {
            debug_printf(
                "bsc_process_cli_awaiting_accept() got bvlc result packet "
                "with unexpected message id %04x\n",
                c->dm.hdr.message_id);
        } else if (c->dm.payload.result.error_code ==
            ERROR_CODE_NODE_DUPLICATE_VMAC) {
            // According AB.6.2.2 BACnet/SC Connection Initiating
            // Peer State Machine: on receipt of a BVLC-Result NAK
            // message with an 'Error Code' of NODE_DUPLICATE_VMAC,
            // the initiating peer's node shall choose a new
            // Random-48 VMAC, close the WebSocket connection, and
            // enter the IDLE state.
            // Signal upper layer about that error
            bsc_cli_process_error(c, BSC_SC_DUPLICATED_VMAC);
        } else {
            debug_printf("bsc_process_cli_awaiting_accept() got unexpected "
                         "BVLC_RESULT error code "
                         "%d in BVLC-Result message in awaiting accept state\n",
                c->dm.payload.result.error_code);
        }
    } else if (c->dm.hdr.bvlc_function == BVLC_SC_DISCONNECT_REQUEST) {
        // AB.6.2.2 BACnet/SC Connection Initiating Peer State
        // Machine does not say anything about situation when
        // disconnect request is received from remote peer after
        // connect request. Handle this situation as an error, log
        // it and close connection.
        debug_printf("bsc_process_cli_awaiting_accept() got unexpected "
                     "disconnect request\n");
        bsc_cli_process_error(c, BSC_SC_PEER_DISCONNECTED);
    } else if (c->dm.hdr.bvlc_function == BVLC_SC_DISCONNECT_ACK) {
        // AB.6.2.2 BACnet/SC Connection Initiating Peer State
        // Machine does not say anything about situation when
        // disconnect ack is received from remote peer after connect
        // request. Handle this situation as an error, log it and
        // close connection.
        debug_printf(
            "bsc_process_cli_awaiting_accept() got unexpected disconnect ack "
            "request\n");
        bsc_cli_process_error(c, BSC_SC_PEER_DISCONNECTED);
    } else {
        debug_printf("bsc_process_cli_awaiting_accept() unexpected message "
                     "with bvlc function "
                     "%d is discarded in awaiting accept state\n",
            c->dm.hdr.bvlc_function);
    }
    debug_printf("bsc_process_cli_awaiting_accept() <<<\n");
}

static void bsc_dispatch_cli_func(BSC_WEBSOCKET_HANDLE h,
    BSC_WEBSOCKET_EVENT ev,
    uint8_t *buf,
    size_t bufsize,
    void *dispatch_func_user_param)
{
    BSC_SOCKET_CTX *ctx = (BSC_SOCKET_CTX *)dispatch_func_user_param;
    BSC_SOCKET *c;
    uint16_t len;
    BSC_WEBSOCKET_RET wret;
    uint8_t *p;
    int i;
    bool all_socket_disconnected = true;

    debug_printf("bsc_dispatch_cli_func() >>> h = %d, ev = %d, buf = %p, "
                 "bufsize = %d, ctx = %p\n",
        h, ev, buf, bufsize, ctx);
    bsc_global_mutex_lock();

    c = bsc_find_conn_by_websocket(ctx, h);

    if (!c) {
        debug_printf("bsc_dispatch_cli_func() <<< warning, can not find "
                     "connection object for websocket %d\n",
            h);
    }

    if (ev == BSC_WEBSOCKET_DISCONNECTED) {
        if (ctx->state == BSC_CTX_STATE_DEINITIALIZING) {
            c->state = BSC_SOCK_STATE_IDLE;
            for (i = 0; i < ctx->sock_num; i++) {
                if (ctx->sock[i].state != BSC_SOCK_STATE_IDLE) {
                    all_socket_disconnected = false;
                    break;
                }
            }
            if (all_socket_disconnected) {
                ctx->state = BSC_CTX_STATE_IDLE;
                ctx->funcs->context_event(ctx, BSC_CTX_DEINITIALIZED);
            }
        } else if (c->state == BSC_SOCK_STATE_ERROR) {
            c->state = BSC_SOCK_STATE_IDLE;
            c->wh = BSC_WEBSOCKET_INVALID_HANDLE;
            ctx->funcs->socket_event(c, BSC_SOCKET_EVENT_DISCONNECTED,
                c->disconnect_reason, NULL, 0, NULL);
        } else {
            c->state = BSC_SOCK_STATE_IDLE;
            c->wh = BSC_WEBSOCKET_INVALID_HANDLE;
            ctx->funcs->socket_event(c, BSC_SOCKET_EVENT_DISCONNECTED,
                BSC_SC_PEER_DISCONNECTED, NULL, 0, NULL);
        }
    } else if (ev == BSC_WEBSOCKET_CONNECTED) {
        if (c->state == BSC_SOCK_STATE_AWAITING_WEBSOCKET) {
            debug_printf("bsc_dispatch_cli_func() conn %p, websocket %d, state "
                         "changed to BSC_SOCK_STATE_AWAITING_ACCEPT\n",
                c, h);
            c->state = BSC_SOCK_STATE_AWAITING_ACCEPT;
            mstimer_set(&c->t, c->ctx->cfg->connect_timeout_s * 1000);
            c->message_id = (uint16_t)(rand() % USHRT_MAX);
            c->expected_connect_accept_message_id = c->message_id;
            debug_printf("bsc_dispatch_cli_func() expected connect accept "
                         "message id = %04x\n",
                c->expected_connect_accept_message_id);

            debug_printf(
                "bsc_dispatch_cli_func() going to send connect request "
                "with uuid %s\n and vmac %s\n",
                bsc_uuid_to_string(&ctx->cfg->local_uuid),
                bsc_vmac_to_string(&ctx->cfg->local_vmac));

            len = bvlc_sc_encode_connect_request(
                &c->tx_buf[c->tx_buf_size + sizeof(len)],
                (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
                    ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
                    : 0,
                c->message_id, &ctx->cfg->local_vmac, &ctx->cfg->local_uuid,
                ctx->cfg->max_bvlc_len, ctx->cfg->max_ndpu_len);

            if (!len) {
                bsc_cli_process_error(c, BSC_SC_NO_RESOURCES);
            } else {
                memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
                c->tx_buf_size += len + sizeof(len);
            }
            bws_cli_send(c->wh);
        }
    } else if (ev == BSC_WEBSOCKET_SENDABLE) {
        p = c->tx_buf;

        while (c->tx_buf_size > 0) {
            memcpy(&len, p, sizeof(len));
            wret = bws_cli_dispatch_send(c->wh, &p[sizeof(len)], len);
            if (wret != BSC_WEBSOCKET_SUCCESS) {
                debug_printf("bsc_dispatch_cli_func() send data failed, start "
                             "disconnect operation on socket %p\n",
                    c);
                bsc_cli_process_error(c, bsc_map_websocket_retcode(wret));
                break;
            } else {
                c->tx_buf_size -= len + sizeof(len);
                p += len + sizeof(len);
            }
        }
    } else if (ev == BSC_WEBSOCKET_RECEIVED) {
        if (c->state == BSC_SOCK_STATE_AWAITING_ACCEPT) {
            bsc_process_cli_awaiting_accept(c, buf, bufsize);
        } else if (c->state == BSC_SOCK_STATE_DISCONNECTING ||
            c->state == BSC_SOCK_STATE_CONNECTED) {
            if (bufsize > BVLC_SC_NPDU_MAX_SIZE) {
                debug_printf("bsc_dispatch_cli_func() message is dropped, size "
                             "> BVLC_SC_NPDU_MAX_SIZE, socket %p, state %d, "
                             "data_size %d\n",
                    c, c->state, bufsize);
            } else if (sizeof(c->rx_buf) - c->rx_buf_size - sizeof(len) >=
                bufsize) {
                len = (uint16_t)bufsize;
                memcpy(&c->rx_buf[c->rx_buf_size], &len, sizeof(len));
                c->rx_buf_size += sizeof(len);
                memcpy(&c->rx_buf[c->rx_buf_size], buf, bufsize);
                c->rx_buf_size += bufsize;
                bsc_runloop_schedule();
            } else {
                debug_printf("bsc_dispatch_cli_func() no space in rx_buf, "
                             "message is dropped,  socket %p, state %d, "
                             "data_size %d, rx_buf_size = %d\n",
                    c, c->state, bufsize, c->rx_buf_size);
            }
        } else {
            debug_printf("bsc_dispatch_cli_func() data was dropped for socket "
                         "%p, state %d, data_size %d\n",
                c, c->state, bufsize);
        }
    }
    bsc_global_mutex_unlock();
    debug_printf("bsc_dispatch_cli_func() <<<\n");
}

BSC_SC_RET bsc_init_сtx(BSC_SOCKET_CTX *ctx,
    BSC_CONTEXT_CFG *cfg,
    BSC_SOCKET_CTX_FUNCS *funcs,
    BSC_SOCKET *sockets,
    size_t sockets_num)
{
    BSC_WEBSOCKET_RET ret;
    BSC_SC_RET sc_ret = BSC_SC_SUCCESS;
    int i;

    debug_printf(
        "bsc_init_сtx() >>> ctx = %p, cfg = %p, funcs = %p\n", ctx, cfg, funcs);

    if (!ctx || !cfg || !funcs || !funcs->find_connection_for_vmac ||
        !funcs->find_connection_for_uuid || !funcs->socket_event ||
        !funcs->context_event || !sockets || !sockets_num) {
        debug_printf("bsc_init_ctx() <<< ret = BSC_SC_BAD_PARAM\n");
        return BSC_SC_BAD_PARAM;
    }

    bsc_global_mutex_lock();

    if (ctx->state != BSC_CTX_STATE_IDLE) {
        bsc_global_mutex_unlock();
        debug_printf("bsc_init_ctx() <<< ret = BSC_SC_INVALID_OPERATION\n");
        return BSC_SC_INVALID_OPERATION;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->cfg = cfg;
    ctx->funcs = funcs;
    ctx->sock = sockets;
    ctx->sock_num = sockets_num;
    for (i = 0; i < sockets_num; i++) {
        ctx->sock[i].state = BSC_SOCK_STATE_IDLE;
        ctx->sock[i].wh = BSC_WEBSOCKET_INVALID_HANDLE;
    }
    if (cfg->type == BSC_SOCKET_CTX_ACCEPTOR) {
        ret = bws_srv_start(cfg->proto, cfg->port, cfg->ca_cert_chain,
            cfg->ca_cert_chain_size, cfg->cert_chain, cfg->cert_chain_size,
            cfg->priv_key, cfg->priv_key_size, cfg->connect_timeout_s,
            bsc_dispatch_srv_func, ctx);

        sc_ret = bsc_map_websocket_retcode(ret);

        if (sc_ret == BSC_SC_SUCCESS) {
            ctx->state = BSC_CTX_STATE_INITIALIZING;
            bsc_runloop_reg((void *)ctx, bsc_runloop);
        }
    } else {
        ctx->state = BSC_CTX_STATE_INITIALIZED;
        bsc_runloop_reg((void *)ctx, bsc_runloop);
        ctx->funcs->context_event(ctx, BSC_CTX_INITIALIZED);
    }

    bsc_global_mutex_unlock();
    debug_printf("bsc_init_ctx() <<< ret = %d \n", sc_ret);
    return sc_ret;
}

BACNET_STACK_EXPORT
void bsc_deinit_ctx(BSC_SOCKET_CTX *ctx)
{
    int i;
    bool active_socket = false;
    debug_printf("bsc_deinit_ctx() >>> ctx = %p\n", ctx);

    bsc_global_mutex_lock();
    if (!ctx || ctx->state == BSC_CTX_STATE_IDLE ||
        ctx->state == BSC_CTX_STATE_DEINITIALIZING) {
        debug_printf("bsc_deinit_ctx() no action required\n");
        bsc_global_mutex_unlock();
        debug_printf("bsc_deinit_ctx() <<<\n");
    }

    if (ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
        ctx->state = BSC_CTX_STATE_DEINITIALIZING;
        for (i = 0; i < ctx->sock_num; i++) {
            if (ctx->sock[i].state != BSC_SOCK_STATE_IDLE) {
                active_socket = true;
                bws_cli_disconnect(ctx->sock[i].wh);
            }
        }
        if (!active_socket) {
            ctx->state = BSC_CTX_STATE_IDLE;
            ctx->funcs->context_event(ctx, BSC_CTX_DEINITIALIZED);
        }
    } else {
        ctx->state = BSC_CTX_STATE_DEINITIALIZING;
        bws_srv_stop(ctx->cfg->proto);
    }

    bsc_global_mutex_unlock();
    debug_printf("bsc_deinit_ctx() <<<\n");
}

BACNET_STACK_EXPORT
BSC_SC_RET bsc_connect(BSC_SOCKET_CTX *ctx, BSC_SOCKET *c, char *url)
{
    BSC_SC_RET ret = BSC_SC_INVALID_OPERATION;
    BSC_WEBSOCKET_RET wret;

    debug_printf("bsc_connect() >>> ctx = %p, c = %p, url = %s\n", ctx, c, url);

    if (!ctx || !c || !url) {
        ret = BSC_SC_BAD_PARAM;
    } else {
        bsc_global_mutex_lock();

        if (ctx->state == BSC_CTX_STATE_INITIALIZED &&
            ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
            c->ctx = ctx;
            c->state = BSC_SOCK_STATE_AWAITING_WEBSOCKET;
            c->tx_buf_size = 0;
            c->rx_buf_size = 0;

            wret =
                bws_cli_connect(ctx->cfg->proto, url, ctx->cfg->ca_cert_chain,
                    ctx->cfg->ca_cert_chain_size, ctx->cfg->cert_chain,
                    ctx->cfg->cert_chain_size, ctx->cfg->priv_key,
                    ctx->cfg->priv_key_size, ctx->cfg->connect_timeout_s,
                    bsc_dispatch_cli_func, ctx, &c->wh);

            ret = bsc_map_websocket_retcode(wret);
            if (wret != BSC_WEBSOCKET_SUCCESS) {
                c->state = BSC_SOCK_STATE_IDLE;
            }
        }

        bsc_global_mutex_unlock();
    }

    debug_printf("bsc_connect() <<< ret = %d\n", ret);
    return ret;
}

BACNET_STACK_EXPORT
void bsc_disconnect(BSC_SOCKET *c)
{
    uint16_t len;

    debug_printf("bsc_disconnect() >>> c = %p\n", c);
    bsc_global_mutex_lock();

    if (c->ctx->state == BSC_CTX_STATE_INITIALIZED &&
        c->state == BSC_SOCK_STATE_CONNECTED) {
        c->message_id++;
        c->expected_disconnect_message_id = c->message_id;
        c->state = BSC_SOCK_STATE_DISCONNECTING;
        mstimer_set(&c->t, c->ctx->cfg->disconnect_timeout_s * 1000);
        len = bvlc_sc_encode_disconnect_request(
            &c->tx_buf[c->tx_buf_size + sizeof(len)],
            (sizeof(c->tx_buf) - c->tx_buf_size >= sizeof(len))
                ? (sizeof(c->tx_buf) - c->tx_buf_size - sizeof(len))
                : 0,
            c->message_id);
        if (!len) {
            debug_printf("bsc_disconnect() disconnect request not sent, err = "
                         "BSC_SC_NO_RESOURCES\n");
            if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                bsc_cli_process_error(c, BSC_SC_NO_RESOURCES);
            } else {
                bsc_srv_process_error(c, BSC_SC_NO_RESOURCES);
            }
        } else {
            memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
            c->tx_buf_size += len + sizeof(len);
        }
    }
    bsc_global_mutex_unlock();
    debug_printf("bsc_connect() <<<\n");
}

BACNET_STACK_EXPORT
BSC_SC_RET bsc_send2(BSC_SOCKET *c,
    uint8_t *part1,
    uint16_t part1_len,
    uint8_t *part2,
    uint16_t part2_len)
{
    BSC_SC_RET ret = BSC_SC_SUCCESS;
    size_t req_len;
    uint16_t len;

    debug_printf("bsc_send2() >>> c = %p, part1 = %p, part1_len = %d, part2 = "
                 "%p, part2_len = %d\n",
        c, part1, part1_len, part2, part2_len);

    if (!c || !part1 || !part1_len || (part2 && !part2_len) ||
        (!part2 && part2_len)) {
        ret = BSC_SC_BAD_PARAM;
    } else {
        bsc_global_mutex_lock();
        if (c->ctx->state != BSC_CTX_STATE_INITIALIZED ||
            c->state != BSC_SOCK_STATE_CONNECTED) {
            ret = BSC_SC_INVALID_OPERATION;
        } else {
            req_len = part2 ? part1_len + part2_len : part1_len;
            if (req_len > USHRT_MAX) {
                ret = BSC_SC_BAD_PARAM;
            } else {
                if (sizeof(c->tx_buf) - c->tx_buf_size <
                    req_len + sizeof(uint16_t)) {
                    ret = BSC_SC_NO_RESOURCES;
                } else {
                    len = (uint16_t)req_len;
                    memcpy(&c->tx_buf[c->tx_buf_size], &len, sizeof(len));
                    c->tx_buf_size += sizeof(len);
                    memcpy(&c->tx_buf[c->tx_buf_size], part1, part1_len);
                    c->tx_buf_size += part1_len;
                    if (part2) {
                        memcpy(&c->tx_buf[c->tx_buf_size], part2, part2_len);
                        c->tx_buf_size += part2_len;
                    }
                    if (c->ctx->cfg->type == BSC_SOCKET_CTX_INITIATOR) {
                        bws_cli_send(c->wh);
                    } else {
                        bws_srv_send(c->ctx->cfg->proto, c->wh);
                    }
                }
            }
        }
        bsc_global_mutex_unlock();
    }

    debug_printf("bsc_send2() <<< ret = %d\n", ret);
    return ret;
}

BACNET_STACK_EXPORT
BSC_SC_RET bsc_send(BSC_SOCKET *c, uint8_t *pdu, uint16_t pdu_len)
{
    BSC_SC_RET ret;
    debug_printf(
        "bsc_send() >>> c = %p, pdu = %p, pdu_len = %d\n", c, pdu, pdu_len);
    ret = bsc_send2(c, pdu, pdu_len, NULL, 0);
    debug_printf("bsc_send() <<< ret = %d\n", ret);
    return ret;
}

BACNET_STACK_EXPORT
void bsc_get_remote_bvlc(BSC_SOCKET *c, uint16_t *p_val)
{
    *p_val = 0;
    bsc_global_mutex_lock();
    if (c->state == BSC_SOCK_STATE_CONNECTED ||
        c->state == BSC_SOCK_STATE_DISCONNECTING) {
        *p_val = c->max_bvlc_len;
    }
    bsc_global_mutex_unlock();
}

BACNET_STACK_EXPORT
void bsc_get_remote_npdu(BSC_SOCKET *c, uint16_t *p_val)
{
    *p_val = 0;
    bsc_global_mutex_lock();
    if (c->state == BSC_SOCK_STATE_CONNECTED ||
        c->state == BSC_SOCK_STATE_DISCONNECTING) {
        *p_val = c->max_npdu_len;
    }
    bsc_global_mutex_unlock();
}