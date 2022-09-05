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
#include "bacnet/datalink/bsc/bsc-connection.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-connection-private.h"
#include "bacnet/datalink/bsc/bsc-mutex.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/debug.h"

static void bsc_remove_connection(BSC_CONNECTION *c)
{
    BSC_CONNECTION_CTX *ctx = c->ctx;
    debug_printf("bsc_remove_connection() >>> c = %p, ctx = %p, head = %p, tail = %p\n",
        c, ctx, ctx->head, ctx->tail);

    c->state = BSC_CONN_STATE_IDLE;

    if (ctx->head == ctx->tail) {
        ctx->head = NULL;
        ctx->tail = NULL;
    } else if (!c->last) {
        ctx->head = c->next;
        ctx->head->last = NULL;
    } else if (!c->next) {
        ctx->tail = c->last;
        ctx->tail->next = NULL;
    } else {
        c->next->last = c->last;
        c->last->next = c->next;
    }

    debug_printf("bsc_remove_connection() <<<\n");
}

static void bsc_add_connection(BSC_CONNECTION *c)
{
    BSC_CONNECTION_CTX *ctx = c->ctx;
    debug_printf("bsc_add_connection() >>> c = %p, ctx = %p, head = %p, tail = %p\n",
        c, ctx, ctx->head, ctx->tail);

    if (ctx->head == NULL && ctx->tail == NULL) {
        ctx->head = c;
        ctx->tail = c;
        c->next = NULL;
        c->last = NULL;
    } else if (ctx->head == ctx->tail) {
        c->last = ctx->head;
        c->next = NULL;
        ctx->tail = c;
        ctx->head->next = c;
    } else {
        c->last = ctx->tail;
        c->next = NULL;
        ctx->tail->next = c;
        ctx->tail = c;
    }
    debug_printf("bsc_add_connection() <<<\n");
}

void bsc_init_ctx_cfg(BSC_CTX_TYPE type,
                      BSC_CONTEXT_CFG* cfg,
                      BACNET_WEBSOCKET_PROTOCOL proto,
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
    if(cfg) {
       cfg->port = port;
       cfg->type = type;
       cfg->ca_cert_chain = ca_cert_chain;
       cfg->ca_cert_chain_size = ca_cert_chain_size;
       cfg->cert_chain = cert_chain;
       cfg->cert_chain_size = cert_chain_size;
       cfg->priv_key = key,
       cfg->priv_key_size = key_size;
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

BACNET_SC_RET bsc_init_сtx(BSC_CONNECTION_CTX *ctx,
                           BSC_CONTEXT_CFG* cfg,
                           BSC_CONNECTION_CTX_FUNCS* funcs)
{
    BACNET_WEBSOCKET_RET ret;
    BACNET_SC_RET sc_ret;

    bsc_global_mutex_lock();
    debug_printf("bsc_init_сtx() >>> ctx = %p, cfg = %p, funcs = %p\n", ctx, cfg, funcs);
    memset(ctx, 0, sizeof(*ctx));
    ctx->cfg = cfg;
    ctx->funcs = funcs;

    if(cfg->type == BSC_CTX_ACCEPTOR) {
         ret = bws_srv_get()->bws_start(cfg->proto,
                                        cfg->port,
                                        cfg->ca_cert_chain,
                                        cfg->ca_cert_chain_size,
                                        cfg->cert_chain,
                                        cfg->cert_chain_size,
                                        cfg->priv_key,
                                        cfg->priv_key_size);
    }

    sc_ret = bsc_map_websocket_retcode(ret);
    debug_printf("bsc_init() <<< ret = %d \n", sc_ret);
    bsc_global_mutex_unlock();
    return sc_ret;
}

void bsc_deinit_ctx(BSC_CONNECTION_CTX *ctx)
{
    debug_printf("bsc_deinit_ctx() >>> ctx = %p\n", ctx);
    bsc_global_mutex_lock();
    bsc_global_mutex_unlock();
    debug_printf("bsc_cleanup() <<<\n");
}

#if 0
void bsc_cleanup(void) 
{
    debug_printf("bsc_cleanup() >>>\n");
    bsc_global_mutex_lock();

    // the call below will unblock all
    // blocked calls of the websocket server.

    bws_srv_get()->bws_stop();

    while (bsc_conn_list_head) {
        if(bsc_conn_list_head->peer_type == BSC_PEER_INITIATOR &&
           bsc_conn_list_head->state != BSC_CONN_STATE_IDLE) {
            bws_cli_get->bws_disconnect(bsc_conn_list_head->wh);
        }
        bsc_remove_connection(bsc_conn_list_head);
    }

    bsc_global_mutex_unlock();
    debug_printf("bsc_cleanup() <<<\n");
}
#endif

void bsc_close(BSC_CONNECTION *c)
{
  // TODO!
}

static bool bsc_check_connection_heartbeat(
    BSC_CONNECTION *c, uint16_t seconds_elapsed)
{
    bool ret = true;

    debug_printf("bsc_check_connection_heartbeat() >>> c = %p, state = %d\n", c,
        c->state);
    if (c->state == BSC_CONN_STATE_CONNECTED) {
        c->heartbeat_seconds_elapsed += seconds_elapsed;
        if (c->ctx->cfg->type == BSC_CTX_INITIATOR) {
            if (c->heartbeat_seconds_elapsed >= c->ctx->cfg->heartbeat_timeout_s) {
                return false;
            }
        } else if (c->ctx->cfg->type == BSC_CTX_ACCEPTOR) {
            if (c->heartbeat_seconds_elapsed >= 2 * c->ctx->cfg->heartbeat_timeout_s) {
                return false;
            }
        }
    }

    debug_printf("bsc_check_connection_heartbeat() <<< ret = %d\n", ret);
    return ret;
}

static BACNET_WEBSOCKET_RET bsc_disconnect_websocket(BSC_CONNECTION *c)
{
    BACNET_WEBSOCKET_RET ret;
    if (c->ctx->cfg->type == BSC_CTX_INITIATOR) {
        ret = bws_cli_get()->bws_disconnect(c->wh);
    } else {
        ret = bws_srv_get()->bws_disconnect(c->ctx->cfg->proto, c->wh);
    }
    return ret;
}

static BACNET_WEBSOCKET_RET bsc_websocket_send(
    BSC_CONNECTION *c, uint8_t *buf, unsigned int len)
{
    BACNET_WEBSOCKET_RET ret;
    if (c->ctx->cfg->type == BSC_CTX_INITIATOR) {
        ret = bws_cli_get()->bws_send(c->wh, buf, len);
    } else {
        ret = bws_srv_get()->bws_send(c->ctx->cfg->proto, c->wh, buf, len);
    }
    return ret;
}

static BACNET_WEBSOCKET_RET bsc_websocket_recv(BSC_CONNECTION *c,
    uint8_t *buf,
    size_t bufsize,
    size_t *bytes_received,
    int timeout)
{
    BACNET_WEBSOCKET_RET final_ret = BACNET_WEBSOCKET_TIMEDOUT;
    BACNET_WEBSOCKET_RET ret;
    unsigned long time_stamp = mstimer_now() / 1000;

    debug_printf(
        "bsc_websocket_recv() >>> c = %p, timeout_s = %d\n", c, timeout);

    while(bsc_seconds_left(time_stamp, timeout)) {
       if (c->ctx->cfg->type == BSC_CTX_INITIATOR) {
           ret = bws_cli_get()->bws_recv(
                   c->wh, buf, bufsize, bytes_received, timeout);
       } else {
           ret = bws_srv_get()->bws_recv( c->ctx->cfg->proto,
                   c->wh, buf, bufsize, bytes_received, timeout);
       }

       // AB.7.5.3 BACnet/SC BVLC Message Exchange:
       //   If the length of a BVLC message received through a WebSocket connection
       //   exceeds the maximum BVLC length supported by the receiving node,
       //   the BVLC message shall be discarded and not be processed.

       if(ret != BACNET_WEBSOCKET_SUCCESS) {
         final_ret = ret;
         break;
       }
       else {
         if(*bytes_received > c->ctx->cfg->max_bvlc_len) {
           debug_printf("bsc_websocket_recv() received message of size %d is discarded,"
                        " bsc_max_bvlc_len = %d\n", *bytes_received, c->max_bvlc_len );
           *bytes_received = 0;
           continue;
         }
         final_ret = BACNET_WEBSOCKET_SUCCESS;
         break;
       }
    }

    debug_printf(
        "bsc_websocket_recv() <<< ret = %d\n", final_ret);

    return final_ret;
}

BACNET_SC_RET bsc_accept(BSC_CONNECTION_CTX *ctx,
                         BSC_CONNECTION *c,
                         unsigned int timeout_s)
{
    BACNET_WEBSOCKET_RET ret;
    BACNET_SC_RET sc_ret = BACNET_SC_SUCCESS;
    BVLC_SC_DECODED_MESSAGE dm;
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    uint16_t ucode;
    uint16_t uclass;
    uint8_t buf[sizeof(dm)];
    unsigned int len;
    size_t r;
    BSC_CONNECTION *existing = NULL;
    uint16_t message_id;

    debug_printf(
        "bsc_accept() >>> c = %p, timeout_s = %d\n", c, timeout_s);

    if(ctx->cfg->type != BSC_CTX_ACCEPTOR) {
      debug_printf("bsc_conn_accept() <<< ret = %d\n", BACNET_SC_BAD_PARAM);
      return BACNET_SC_BAD_PARAM;
    }

    // There is no need to syncronize access to connection "c" because
    // other API calls can't be used in other threads until bsc_accept()
    // returns with valid connection object. But, access to connection list
    // must be syncronized otherwise.

    memset(c, 0, sizeof(*c));
    c->ctx = ctx;
    ret = bws_srv_get()->bws_accept(ctx->cfg->proto, &c->wh, timeout_s * 1000);
    debug_printf("bsc_accept() accepted connection, ret = %d\n", ret);

    if (ret != BACNET_WEBSOCKET_SUCCESS) {
        sc_ret = bsc_map_websocket_retcode(ret);
        debug_printf("bsc_conn_accept() <<< ret = %d\n", sc_ret);
        return sc_ret;
    }

    c->state = BSC_CONN_STATE_AWAITING_REQUEST;
    c->time_stamp = mstimer_now();

    while (1) {
        int left_time = (int) bsc_seconds_left(c->time_stamp, c->ctx->cfg->connect_timeout_s);

        if (!left_time) {
            // connection timeout is elapsed, but connection was not
            // established
            debug_printf(
                "bsc_accept() connnection timeout of %d ms elapsed\n",
                c->ctx->cfg->connect_timeout_s * 1000);
            bws_srv_get()->bws_disconnect(ctx->cfg->proto, c->wh);
            sc_ret =  BACNET_SC_TIMEDOUT;
            break;
        }

        ret =
            bws_srv_get()->bws_recv(ctx->cfg->proto, c->wh, buf, sizeof(buf), &r, left_time);

        if (ret != BACNET_WEBSOCKET_SUCCESS) {
            debug_printf(
                "bsc_accept() got error %d on bws_recv()\n", ret);
            bws_srv_get()->bws_disconnect(ctx->cfg->proto, c->wh);
            sc_ret = bsc_map_websocket_retcode(ret);
            break;
        }

        if (!bvlc_sc_decode_message(buf, r, &dm, &code, &class)) {
            debug_printf("bsc_accept() decoding of received message "
                         "failed, error code = %d, class = %d\n",
                code, class);
        } else if (dm.hdr.bvlc_function != BVLC_SC_CONNECT_REQUEST) {
            debug_printf(
                "bsc_accept() unexpected message with bvlc function "
                "%d is discarded in awaiting request state\n",
                dm.hdr.bvlc_function);
        } else {
            existing = ctx->funcs->find_connection_for_uuid(
                dm.payload.connect_request.uuid);
            if (existing) {
                // Regarding AB.6.2.3 BACnet/SC Connection Accepting Peer
                // State Machine, caseConnect-Request received, known Device
                // UUID: On receipt of a Connect-Request message from the
                // initiating peer whose 'Device UUID' is equal to the
                // initiating peer device UUID of an existing connection,
                // then return a Connect-Accept message, disconnect and
                // close the existing connection to the connection peer node
                // with matching Device UUID, and enter the CONNECTED state.
                debug_printf("bsc_accept() accepting connection from "
                             "known uuid %s\n and vmac %s\n",
                    bsc_uuid_to_string(dm.payload.connect_request.uuid),
                    bsc_vmac_to_string(dm.payload.connect_request.vmac));

                memcpy(&c->vmac, dm.payload.connect_request.vmac,
                    sizeof(c->vmac));
                memcpy(&c->uuid, dm.payload.connect_request.uuid,
                    sizeof(c->uuid));
                c->max_npdu_len = dm.payload.connect_request.max_npdu_len;
                c->max_bvlc_len = dm.payload.connect_request.max_bvlc_len;
                message_id = dm.hdr.message_id;

                len = bvlc_sc_encode_connect_accept(buf, sizeof(buf),
                    dm.hdr.message_id, &c->ctx->cfg->local_vmac, &c->ctx->cfg->local_uuid,
                    c->ctx->cfg->max_bvlc_len, c->ctx->cfg->max_ndpu_len);

                ret = bws_srv_get()->bws_send(ctx->cfg->proto, c->wh, buf, len);

                if (ret != BACNET_WEBSOCKET_SUCCESS) {
                    debug_printf("bsc_accept() sending of connect "
                                 "accept failed, err = %d\n",
                        ret);
                    bws_srv_get()->bws_disconnect(ctx->cfg->proto, c->wh);
                    sc_ret = bsc_map_websocket_retcode(ret);
                    break;
                }

                existing->message_id++;

                len = bvlc_sc_encode_disconnect_request(
                    buf, sizeof(buf), existing->message_id);
                ret = bws_srv_get()->bws_send(ctx->cfg->proto, existing->wh, buf, len);

                if (ret != BACNET_WEBSOCKET_SUCCESS) {
                    debug_printf("bsc_accept() sending of disconnect "
                                 "request failed, err = %d\n",
                        ret);
                }
                c->heartbeat_seconds_elapsed = 0;
                c->state = BSC_CONN_STATE_CONNECTED;
                bsc_global_mutex_lock();
                bsc_add_connection(c);
                bsc_global_mutex_unlock();
                break;
            }

            existing = ctx->funcs->find_connection_for_vmac(
                dm.payload.connect_request.vmac);

            if (existing) {
                debug_printf("bsc_accept() rejected connection for "
                             "duplicated vmac %s from uuid %s,"
                             " vmac is used by uuid %s\n",
                    bsc_vmac_to_string(dm.payload.connect_request.vmac),
                    bsc_uuid_to_string(dm.payload.connect_request.uuid),
                    bsc_uuid_to_string(&existing->uuid));

                uclass = ERROR_CLASS_COMMUNICATION;
                ucode = ERROR_CODE_NODE_DUPLICATE_VMAC;
                len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                    NULL, NULL, BVLC_SC_CONNECT_REQUEST, 1, NULL, &ucode,
                    &uclass, NULL);
                ret = bws_srv_get()->bws_send(ctx->cfg->proto, c->wh, buf, len);
                if (ret != BACNET_WEBSOCKET_SUCCESS) {
                    debug_printf("bsc_accept() sending of nack result "
                                 "message failed, err = %d\n",
                        ret);
                }
                bws_srv_get()->bws_disconnect(ctx->cfg->proto, c->wh);
                sc_ret = BACNET_SC_DUPLICATED_VMAC;
                break;
            }

            if (memcmp(&c->vmac, &c->ctx->cfg->local_vmac, sizeof(c->ctx->cfg->local_vmac)) ==
                    0 &&
                memcmp(&c->uuid, &c->ctx->cfg->local_uuid, sizeof(c->ctx->cfg->local_uuid)) !=
                    0) {
                debug_printf(
                    "bsc_accept() rejected connection of a duplicate "
                    "of this port's vmac %s from uuid %s\n",
                    bsc_vmac_to_string(&c->vmac),
                    bsc_uuid_to_string(&c->uuid));
                uclass = ERROR_CLASS_COMMUNICATION;
                ucode = ERROR_CODE_NODE_DUPLICATE_VMAC;
                len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                    NULL, NULL, BVLC_SC_CONNECT_REQUEST, 1, NULL, &ucode,
                    &uclass, NULL);
                ret = bws_srv_get()->bws_send(ctx->cfg->proto, c->wh, buf, len);
                if (ret != BACNET_WEBSOCKET_SUCCESS) {
                    debug_printf("bsc_accept() sending of nack result "
                                 "message failed, err = %d\n",
                        ret);
                }
                bws_srv_get()->bws_disconnect(ctx->cfg->proto, c->wh);
                sc_ret = BACNET_SC_DUPLICATED_VMAC;
                break;
            }

            debug_printf("bsc_accept() accepted connection from new "
                         "uuid %s with vmac %d\n",
                bsc_vmac_to_string(&c->vmac), bsc_uuid_to_string(&c->uuid));

            len = bvlc_sc_encode_connect_accept(buf, sizeof(buf),
                message_id, &c->ctx->cfg->local_vmac, &c->ctx->cfg->local_uuid,
                c->ctx->cfg->max_bvlc_len, c->ctx->cfg->max_ndpu_len);

            ret = bws_srv_get()->bws_send(ctx->cfg->proto, c->wh, buf, len);
            if (ret != BACNET_WEBSOCKET_SUCCESS) {
                debug_printf("bsc_accept() sending of connect accept "
                             "failed, err = %d\n",
                    ret);
                bws_srv_get()->bws_disconnect(ctx->cfg->proto, c->wh);
                sc_ret = bsc_map_websocket_retcode(ret);
                break;
            }

            c->heartbeat_seconds_elapsed = 0;
            c->state = BSC_CONN_STATE_CONNECTED;
            bsc_global_mutex_lock();
            bsc_add_connection(c);
            bsc_global_mutex_unlock();
            break;
        }
    }

    debug_printf("bsc_conn_accept() <<< ret = %d\n", sc_ret);
    return sc_ret;
}

static BACNET_SC_RET bsc_connect_prepare(BSC_CONNECTION_CTX *ctx,
    BSC_CONNECTION *c,
    char *url,
    uint8_t *buf,
    int len)
{
    BACNET_WEBSOCKET_RET ret;
    BACNET_SC_RET sc_ret;

    debug_printf(
        "bsc_connect_prepare() >>> ctx = %p, c = %p, url = %s, buf = %p, len = %d\n",
        ctx, c, url, buf, len);

    memset(c, 0, sizeof(*c));
    c->ctx = ctx;
    bsc_add_connection(c);

    c->state = BSC_CONN_STATE_AWAITING_WEBSOCKET;

    ret = bws_cli_get()->bws_connect(ctx->cfg->proto, url, ctx->cfg->ca_cert_chain,
        ctx->cfg->ca_cert_chain_size, ctx->cfg->cert_chain, ctx->cfg->cert_chain_size,
        ctx->cfg->priv_key, ctx->cfg->priv_key_size, &c->wh);

    if (ret != BACNET_WEBSOCKET_SUCCESS) {
        bsc_remove_connection(c);
        sc_ret = bsc_map_websocket_retcode(ret);
        debug_printf("bsc_connect_prepare() <<< ret = %d\n", sc_ret);
        return sc_ret;
    }

    c->state = BSC_CONN_STATE_AWAITING_ACCEPT;
    c->time_stamp = mstimer_now();
    c->message_id = (uint16_t)(rand() % USHRT_MAX);
    c->expected_connect_accept_message_id = c->message_id;
    debug_printf(
        "bsc_connect_prepare() expected connect accept message id = %04x\n",
        c->expected_connect_accept_message_id);

    len = bvlc_sc_encode_connect_request(buf, sizeof(buf), c->message_id,
        &ctx->cfg->local_vmac, &ctx->cfg->local_uuid, ctx->cfg->max_bvlc_len, ctx->cfg->max_ndpu_len);

    ret = bws_cli_get()->bws_send(c->wh, buf, len);

    if (ret != BACNET_WEBSOCKET_SUCCESS) {
        bws_cli_get()->bws_disconnect(c->wh);
        bsc_remove_connection(c);
        sc_ret = bsc_map_websocket_retcode(ret);
        debug_printf("bsc_connect_prepare() <<< ret = %d\n", sc_ret);
        return sc_ret;
    }

    debug_printf("bsc_connect_prepare() <<< ret = %d \n", BACNET_SC_SUCCESS);
    return BACNET_SC_SUCCESS;
}

BACNET_SC_RET bsc_connect(
    BSC_CONNECTION_CTX *ctx,
    BSC_CONNECTION *c,
    char *url)
{
    BACNET_WEBSOCKET_RET ret;
    BACNET_SC_RET sc_ret;
    BVLC_SC_DECODED_MESSAGE dm;
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    uint8_t buf[sizeof(dm)];
    unsigned int len;
    size_t r;

    debug_printf(
        "bsc_connect() >>> ctx = %p, c = %p, url = %s\n", ctx, c, url);

    sc_ret = bsc_connect_prepare(ctx, c, url, buf, sizeof(buf));
    if(sc_ret != BACNET_SC_SUCCESS) {
        debug_printf("bsc_connect() <<< ret = %d\n", sc_ret);
        return sc_ret;
    }

    while (1) {
        ret = bws_cli_get()->bws_recv(c->wh, buf, sizeof(buf), &r, 1000);
        if (ret != BACNET_WEBSOCKET_SUCCESS &&
            ret != BACNET_WEBSOCKET_TIMEDOUT) {
            bws_cli_get()->bws_disconnect(c->wh);
            bsc_remove_connection(c);
            break;
        } else if (ret == BACNET_WEBSOCKET_TIMEDOUT) {
            unsigned long t = mstimer_now();
            unsigned long delta = t > c->time_stamp
                ? (t - c->time_stamp) / 1000
                : (c->time_stamp - t) / 1000;

            if (delta >= ctx->cfg->connect_timeout_s) {
                bws_cli_get()->bws_disconnect(c->wh);
                bsc_remove_connection(c);
                break;
            }
        } else if (ret == BACNET_WEBSOCKET_SUCCESS) {
            if (bvlc_sc_decode_message(buf, r, &dm, &code, &class)) {
                if (dm.hdr.bvlc_function == BVLC_SC_CONNECT_ACCEPT) {
                    if (dm.hdr.message_id !=
                        c->expected_connect_accept_message_id) {
                        debug_printf("bsc_connect() got bvlc result packet "
                                     "with unexpected message id %04x\n",
                            dm.hdr.message_id);
                        continue;
                    }
                    memcpy(&c->vmac, dm.payload.connect_accept.vmac,
                        sizeof(c->vmac));
                    memcpy(&c->uuid, dm.payload.connect_accept.uuid,
                        sizeof(c->uuid));
                    c->max_bvlc_len = dm.payload.connect_accept.max_bvlc_len;
                    c->max_npdu_len = dm.payload.connect_accept.max_npdu_len;
                    c->heartbeat_seconds_elapsed = 0;
                    c->state = BSC_CONN_STATE_CONNECTED;
                    debug_printf("bsc_connect() <<< ret = true\n");
                    return true;
                } else if (dm.hdr.bvlc_function == BVLC_SC_RESULT) {
                    if (dm.payload.result.bvlc_function !=
                        BVLC_SC_CONNECT_REQUEST) {
                        debug_printf(
                            "bsc_connect() got unexpected bvlc function %d in "
                            "BVLC-Result message in awaiting accept state\n",
                            dm.payload.result.bvlc_function);
                        continue;
                    }
                    if (dm.hdr.message_id !=
                        c->expected_connect_accept_message_id) {
                        debug_printf("bsc_connect() got bvlc result packet "
                                     "with unexpected message id %04x\n",
                            dm.hdr.message_id);
                        break;
                    }
                    // check for transition "BVLC-Result NAK, VMAC collision"
                    if (dm.payload.result.error_code ==
                        ERROR_CODE_NODE_DUPLICATE_VMAC) {
                        // According AB.6.2.2 BACnet/SC Connection Initiating
                        // Peer State Machine: on receipt of a BVLC-Result NAK
                        // message with an 'Error Code' of NODE_DUPLICATE_VMAC,
                        // the initiating peer's node shall choose a new
                        // Random-48 VMAC, close the WebSocket connection, and
                        // enter the IDLE state.
                        sc_ret = bsc_connect_prepare(
                                ctx, c, url,  buf, sizeof(buf));
                        if (sc_ret != BACNET_SC_SUCCESS) {
                            debug_printf("bsc_connect() <<< ret = %d\n", sc_ret);
                            return sc_ret;
                        }
                        continue;
                    }
                    debug_printf(
                        "bsc_connect() got unexpected BVLC_RESULT error code "
                        "%d in BVLC-Result message in awaiting accept state\n",
                        dm.payload.result.error_code);
                    break;
                } else if (dm.hdr.bvlc_function == BVLC_SC_DISCONNECT_REQUEST) {
                    // AB.6.2.2 BACnet/SC Connection Initiating Peer State
                    // Machine does not say anything about situation when
                    // disconnect request is received from remote peer after
                    // connect request. Handle this situation as an error, log
                    // it and close connection.
                    debug_printf(
                        "bsc_connect() got unexpected disconnect request\n");
                    bws_cli_get()->bws_disconnect(c->wh);
                    bsc_remove_connection(c);
                    break;
                } else if (dm.hdr.bvlc_function == BVLC_SC_DISCONNECT_ACK) {
                    // AB.6.2.2 BACnet/SC Connection Initiating Peer State
                    // Machine does not say anything about situation when
                    // disconnect ack is received from remote peer after connect
                    // request. Handle this situation as an error, log it and
                    // close connection.
                    debug_printf("bsc_connect() got unexpected disconnect ack "
                                 "request\n");
                    bws_cli_get()->bws_disconnect(c->wh);
                    bsc_remove_connection(c);
                    break;
                } else {
                    debug_printf(
                        "bsc_connect() unexpected message with bvlc function "
                        "%d is discarded in awaiting accept state\n",
                        dm.hdr.bvlc_function);
                    continue;
                }
            }
        }
    }

    debug_printf("bsc_connect() <<< ret = false\n");
    return false;
}

/**
 * @brief bsc_process_incoming() function processes service BACNet/SC packets.
 *
 * @param c - pointer BACNet/SC connection descriptor .
 * @param message - decoded BACNet/SC packet.
 *
 * @return true if message was processed by this function and must be discarded,
 *         otherwise false is returned
 */

static bool bsc_process_incoming(
    BSC_CONNECTION *c, BVLC_SC_DECODED_MESSAGE *message)
{
    bool ret = false;
    BACNET_WEBSOCKET_RET res;

    (void)res;
    debug_printf(
        "bsc_process_incoming() >>> c = %p, state = %d\n", c, c->state);

    c->heartbeat_seconds_elapsed = 0;

    if (c->state == BSC_CONN_STATE_CONNECTED) {
        if (message->hdr.bvlc_function == BVLC_SC_HEARTBEAT_ACK) {
            if (message->hdr.message_id != c->expected_heartbeat_message_id) {
                debug_printf("bsc_process_incoming() got heartbeat ack with "
                             "unexpected message id %d for connection %p\n",
                    message->hdr.message_id, c);
            } else {
                debug_printf("bsc_process_incoming() got heartbeat ack for "
                             "connection %p\n",
                    c);
            }
            ret = true;
        } else if (c->state == BSC_CONN_STATE_DISCONNECTING) {
            if (message->hdr.bvlc_function == BVLC_SC_DISCONNECT_ACK) {
                if (message->hdr.message_id !=
                    c->expected_disconnect_message_id) {
                    debug_printf(
                        "bsc_process_incoming() got disconect ack with "
                        "unexpected message id %d for connection %p\n",
                        message->hdr.message_id, c);
                } else {
                    debug_printf("bsc_process_incoming() got disconect ack for "
                                 "connection %p\n",
                        c);
                }
                res = bsc_disconnect_websocket(c);
                debug_printf("bsc_process_incoming() websocket disconnected, "
                             "status = %d\n",
                    res);
                bsc_remove_connection(c);
                ret = true;
            } else if (message->hdr.bvlc_function == BVLC_SC_RESULT) {
                if (message->payload.result.bvlc_function ==
                        BVLC_SC_DISCONNECT_REQUEST &&
                    message->payload.result.result != 0) {
                    debug_printf("bsc_process_incoming() got BVLC_SC_RESULT "
                                 "NAK on BVLC_SC_DISCONNECT_REQUEST\n");
                    res = bsc_disconnect_websocket(c);
                    debug_printf("bsc_process_incoming() websocket "
                                 "disconnected, status = %d\n",
                        res);
                    bsc_remove_connection(c);
                    ret = true;
                }
            }
        }
    }
    debug_printf("bsc_process_incoming() <<< ret = %d\n", ret);
    return ret;
}

void bsc_disconnect(BSC_CONNECTION *c)
{
    BACNET_WEBSOCKET_RET ret;
    BVLC_SC_DECODED_MESSAGE dm;
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    uint8_t buf[sizeof(dm)];
    unsigned int len;
    size_t r;

    debug_printf("bsc_disconnect() >>> c = %p, state = %d\n", c, c->state);

    // Do not allow a user to send disconnect request on non-connected BACNet/SC
    // link
    if (c->state == BSC_CONN_STATE_CONNECTED ||
        c->state == BSC_CONN_STATE_DISCONNECTING) {
        c->message_id++;
        c->expected_disconnect_message_id = c->message_id;
        c->state = BSC_CONN_STATE_DISCONNECTING;
        c->time_stamp = mstimer_now();
        len =
            bvlc_sc_encode_disconnect_request(buf, sizeof(buf), c->message_id);
        ret = bsc_websocket_send(c, buf, len);
        debug_printf(
            "bsc_disconnect() disconnect request is sent, status = %d\n", ret);

        while (1) {
            int left_time = (int)bsc_seconds_left(c->time_stamp, c->ctx->cfg->disconnect_timeout_s);

            if (!left_time) {
                debug_printf("bsc_disconnect() connnection disconnect timeout "
                             "of %d ms elapsed\n",
                    c->ctx->cfg->disconnect_timeout_s * 1000);
                ret = bsc_disconnect_websocket(c);
                debug_printf("bsc_disconnect() websocket disconnected by "
                             "timeout, status = %d\n",
                    ret);
                bsc_remove_connection(c);
                break;
            }

            ret = bsc_websocket_recv(c, buf, sizeof(buf), &r, left_time);

            if (ret != BACNET_WEBSOCKET_SUCCESS) {
                debug_printf("bsc_disconnect() websocket recv data failed, "
                             "error = %d \n",
                    ret);
                ret = bsc_disconnect_websocket(c);
                debug_printf(
                    "bsc_disconnect() websocket disconnected, status = %d\n",
                    ret);
                bsc_remove_connection(c);
                break;
            }

            if (!bvlc_sc_decode_message(buf, r, &dm, &code, &class)) {
                debug_printf("bsc_disconnect() decoding of received message "
                             "failed, error code = %d, class = %d\n",
                    code, class);
            } else {
                bsc_process_incoming(c, &dm);
                if (c->state == BSC_CONN_STATE_IDLE) {
                    debug_printf(
                        "bsc_disconnect() successfull websocket disconnect\n");
                    break;
                }
            }
        }
    }

    debug_printf("bsc_disconnect() <<<\n");
}

int bsc_send(BSC_CONNECTION *c, uint8_t *pdu, uint16_t pdu_len)
{
    int ret = -1;
    BACNET_WEBSOCKET_RET wr;

    debug_printf(
        "bsc_send() >>> c = %p, pdu = %p, pdu_len = %d\n", c, pdu, pdu_len);

    if (c->state == BSC_CONN_STATE_CONNECTED) {
        if (c->ctx->cfg->type == BSC_CTX_ACCEPTOR) {
            c->heartbeat_seconds_elapsed = 0;
        }
        wr = bsc_websocket_send(c, pdu, (size_t) pdu_len);
        if (wr == BACNET_WEBSOCKET_SUCCESS) {
            debug_printf("bsc_send() pdu with size %d is sent\n", pdu_len);
            ret = (int) pdu_len;
        } else {
            debug_printf(
                "bsc_send() sendig of pdu with size %d is failed, error =%d\n",
                pdu_len, ret);

            if(wr != BACNET_WEBSOCKET_CLOSED) {
               // non fatal error occured
               ret = 0;
            }
        }
    }

    debug_printf("bsc_send() <<< ret = %d\n", ret);
    return ret;
}

int bsc_recv(
    BSC_CONNECTION *c, uint8_t *pdu, uint16_t max_pdu, unsigned int timeout)
{
    BACNET_WEBSOCKET_RET ret;
    BVLC_SC_DECODED_MESSAGE dm;
    BACNET_ERROR_CODE code;
    BACNET_ERROR_CLASS class;
    size_t r;
    int retval = 0;

    debug_printf(
        "bsc_recv() >>> c = %p,  pdu = %p, pdu_len = %d, timeout = %d\n", c,
        pdu, max_pdu, timeout);

    if (c->state == BSC_CONN_STATE_CONNECTED) {
        ret = bsc_websocket_recv(c, pdu, max_pdu, &r, timeout);

        if (ret != BACNET_WEBSOCKET_SUCCESS) {
            debug_printf("bsc_recv() recv data failed, error = %d \n", ret);
            if(ret == BACNET_WEBSOCKET_CLOSED) {
              retval = -1;
            }
        } else {
            if (!bvlc_sc_decode_message(pdu, r, &dm, &code, &class)) {
                debug_printf("bsc_recv() decoding of received message "
                             "failed, error code = %d, class = %d\n",
                    code, class);
            } else {
                if (bsc_process_incoming(c, &dm)) {
                    debug_printf("bsc_recv() discarded service pdu of "
                                 "bvlc_function %d and size %d",
                        dm.hdr.bvlc_function, r);
                }
                else {
                  retval = (int) r;
                }
            }
        }
    }

    debug_printf("bsc_recv() <<< ret = %d\n", retval);
    return retval;
}

void bsc_maintainence_timer(BSC_CONNECTION_CTX *ctx,
                            uint16_t seconds_elapsed)
{
    BSC_CONNECTION *e = ctx->head;
    BSC_CONNECTION *c;
    uint8_t buf[sizeof(BVLC_SC_DECODED_HDR)];
    unsigned int len;

    debug_printf(
        "bsc_maintainence_timer() >>> seconds_elapsed = %d\n", seconds_elapsed);

    while (e) {
        if (bsc_check_connection_heartbeat(e, seconds_elapsed)) {
            e = e->next;
        } else {
            debug_printf("bsc_maintainence_timer() heartbeat timeout elapsed "
                         "for connection %p\n",
                e);
            if (e->ctx->cfg->type == BSC_CTX_INITIATOR) {
                debug_printf("bsc_maintainence_timer() going to send heartbeat "
                             "request on connection %p\n",
                    e);
                e->message_id++;
                e->expected_heartbeat_message_id = e->message_id;
                debug_printf(
                    "bsc_maintainence_timer() heartbeat message_id %04x\n",
                    e->expected_heartbeat_message_id);
                len = bvlc_sc_encode_heartbeat_request(
                    buf, sizeof(buf), e->message_id);
                bws_cli_get()->bws_send(e->wh, buf, len);
                e->heartbeat_seconds_elapsed = 0;
                e = e->next;
            } else if (e->ctx->cfg->type == BSC_CTX_ACCEPTOR) {
                debug_printf("bsc_maintainence_timer() zombie connection %p is "
                             "removed\n",
                    e);
                bws_srv_get()->bws_disconnect(e->ctx->cfg->proto, e->wh);
                c = e;
                e = e->next;
                bsc_remove_connection(c);
            }
        }
    }

    debug_printf("bsc_maintainence_timer() <<<\n");
}

void bsc_get_remote_bvlc(BSC_CONNECTION *c, uint16_t *p_val)
{
  *p_val = 0;
  if(c->state == BSC_CONN_STATE_CONNECTED ||
     c->state == BSC_CONN_STATE_DISCONNECTING ) {
     *p_val = c->max_bvlc_len;
  }
}

void bsc_get_remote_npdu(BSC_CONNECTION *c, uint16_t *p_val)
{
  *p_val = 0;
  if(c->state == BSC_CONN_STATE_CONNECTED ||
     c->state == BSC_CONN_STATE_DISCONNECTING ) {
     *p_val = c->max_npdu_len;
  }
}