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

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"
#include "bacnet/datalink/bsc/websocket.h"

#ifndef __BACNET__SECURE__INCLUDED__
#define __BACNET__SECURE__INCLUDED__

#define BSC_DEFAULT_PORT 443

struct BSC_Connection;
typedef struct BSC_Connection BSC_CONNECTION;

// max_bvlc_len - The maximum BVLC message size int bytes that can be received
//                and processed by BACNET/SC datalink.
// max_ndpu_len - The maximum NPDU message size in bytes hat can be handled
//                by BACNET/SC datalink.

BACNET_STACK_EXPORT
bool bsc_set_configuration(int port,
                           uint8_t* ca_cert_chain,
                           size_t ca_cert_chain_size,
                           uint8_t* cert_chain,
                           size_t cert_chain_size,
                           uint8_t *key, size_t key_size,
                           BACNET_SC_UUID* local_uuid,
                           BACNET_SC_VMAC_ADDRESS* local_vmac,
                           uint16_t max_bvlc_len,
                           uint16_t max_ndpu_len,
                           unsigned int connect_timeout_s,
                           unsigned int heartbeat_timeout_s,
                           unsigned int disconnect_timeout_s);

BACNET_STACK_EXPORT
bool bsc_accept(BSC_CONNECTION* c, unsigned int timeout_s);

BACNET_STACK_EXPORT
bool bsc_connect(BSC_CONNECTION* c, char *url, BACNET_WEBSOCKET_CONNECTION_TYPE type);

BACNET_STACK_EXPORT
void bsc_disconnect(BSC_CONNECTION* c);

 BACNET_STACK_EXPORT
 int bsc_send(BSC_CONNECTION* c,
              BACNET_SC_VMAC_ADDRESS *dest,
              uint8_t *pdu,
              unsigned pdu_len);

 BACNET_STACK_EXPORT
 uint16_t bsc_recv(BSC_CONNECTION* c,
                   BACNET_SC_VMAC_ADDRESS *src,
                   uint8_t *pdu,
                   uint16_t max_pdu,
                   unsigned int timeout);

BACNET_STACK_EXPORT
void bsc_maintainence_timer(uint16_t seconds_elapsed);

#endif