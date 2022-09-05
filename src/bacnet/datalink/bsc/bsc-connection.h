/**
 * @file
 * @brief BACNet secure connect API.
 * @author Kirill Neznamov
 * @date July 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BACNET__SECURE__CONNECTION__INCLUDED__
#define __BACNET__SECURE__CONNECTION__INCLUDED__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"

#define BSC_DEFAULT_PORT 443

struct BSC_Connection;
typedef struct BSC_Connection BSC_CONNECTION;

struct BSC_ConnectionContext;
typedef struct BSC_ConnectionContext BSC_CONNECTION_CTX;

struct BSC_ConnectionContextFuncs;
typedef struct BSC_ConnectionContextFuncs BSC_CONNECTION_CTX_FUNCS;

struct BSC_ContextCFG;
typedef struct BSC_ContextCFG BSC_CONTEXT_CFG;

typedef enum {
    BSC_CTX_INITIATOR = 1,
    BSC_CTX_ACCEPTOR = 2
} BSC_CTX_TYPE;

// max_local_bvlc_len - The maximum BVLC message size int bytes that can be
//                      received and processed by BACNET/SC datalink.
// max_local_ndpu_len - The maximum NPDU message size in bytes hat can be
//                      handled by BACNET/SC datalink.

BACNET_STACK_EXPORT
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
                        unsigned int disconnect_timeout_s);

BACNET_STACK_EXPORT
BACNET_SC_RET bsc_init_сtx(BSC_CONNECTION_CTX *ctx,
                           BSC_CONTEXT_CFG* cfg,
                           BSC_CONNECTION_CTX_FUNCS* funcs);

BACNET_STACK_EXPORT
void bsc_deinit_ctx(BSC_CONNECTION_CTX *ctx);

BACNET_STACK_EXPORT
BACNET_SC_RET bsc_accept(BSC_CONNECTION_CTX *ctx,
                         BSC_CONNECTION *c,
                         unsigned int timeout_s);

BACNET_STACK_EXPORT
BACNET_SC_RET bsc_connect(
    BSC_CONNECTION_CTX *ctx,
    BSC_CONNECTION *c,
    char *url);

BACNET_STACK_EXPORT
void bsc_disconnect(BSC_CONNECTION *c);

// ret > 0 if data was sent successfully 
// ret == 0 if data was not sent because of non-fatal error
// ret < 0 if connection was closed

BACNET_STACK_EXPORT
int bsc_send(BSC_CONNECTION *c, uint8_t *pdu, uint16_t pdu_len);

// ret > 0 if data was sent successfully 
// ret == 0 if data was not sent because of non-fatal error
// ret < 0 if connection was closed

BACNET_STACK_EXPORT
int bsc_recv(
    BSC_CONNECTION *c, uint8_t *pdu, uint16_t max_pdu, unsigned int timeout);

BACNET_STACK_EXPORT
void bsc_maintainence_timer(BSC_CONNECTION_CTX *ctx,
                            uint16_t seconds_elapsed);

BACNET_STACK_EXPORT
void bsc_get_remote_bvlc(BSC_CONNECTION *c, uint16_t *p_val);

BACNET_STACK_EXPORT
void bsc_get_remote_npdu(BSC_CONNECTION *c, uint16_t *p_val);

#endif