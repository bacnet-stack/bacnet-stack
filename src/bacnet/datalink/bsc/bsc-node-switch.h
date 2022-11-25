/**
 * @file
 * @brief BACNet secure connect node switch function API.
 * @author Kirill Neznamov
 * @date October 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BACNET__NODE__SWITCH__INCLUDED__
#define __BACNET__NODE__SWITCH__INCLUDED__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bsc-node.h"

typedef void* BSC_NODE_SWITCH_HANDLE;

typedef enum {
    BSC_NODE_SWITCH_EVENT_STARTED = 1,
    BSC_NODE_SWITCH_EVENT_STOPPED = 2,
    BSC_NODE_SWITCH_EVENT_RECEIVED = 3,
    BSC_NODE_SWITCH_EVENT_DUPLICATED_VMAC = 4,

    // BSC_NODE_SWITCH_EVENT_CONNECTED event is emitted
    // only if bsc_node_switch_connect() function has
    // successfully connected to peer specified by vmac.
    // For a connection initiated from a remote peer that
    // event won't be emitted.

    BSC_NODE_SWITCH_EVENT_CONNECTED = 5

    // The BSC_NODE_SWITCH_EVENT_DISCONNECTED event is emitted
    // only if bsc_node_switch_disconnect() was called,
    // e.g. in a case of disconnect was initiated by local peer.
    // For disconnect process initiated from a remote peer that
    // event won't be emitted.
    // Also such kind of event won't be generated if user called
    // bsc_node_switch_stop(). In such case all connections are
    // closed automatically.
    BSC_NODE_SWITCH_EVENT_DISCONNECTED = 6

} BSC_NODE_SWITCH_EVENT;

// dest parameter is actual only for
// BSC_NODE_SWITCH_EVENT_CONNECTED and BSC_NODE_SWITCH_EVENT_DISCONNECTED
// events. It is the parameter which was specified in
// bsc_node_switch_connect() or bsc_node_switch_disconnect() calls.

typedef void (*BSC_NODE_SWITCH_EVENT_FUNC)(BSC_NODE_SWITCH_EVENT ev,
                                           BSC_NODE_SWITCH_HANDLE h,
                                           void* user_arg,
                                           BACNET_SC_VMAC_ADDRESS *dest,
                                           uint8_t *pdu,
                                           uint16_t pdu_len,
                                           BVLC_SC_DECODED_MESSAGE *decoded_pdu);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_switch_start(
   uint8_t *ca_cert_chain,
   size_t ca_cert_chain_size,
   uint8_t *cert_chain,
   size_t cert_chain_size,
   uint8_t *key,
   size_t key_size,
   int port,
   char* iface,
   BACNET_SC_UUID *local_uuid,
   BACNET_SC_VMAC_ADDRESS *local_vmac,
   uint16_t max_local_bvlc_len,
   uint16_t max_local_npdu_len,
   unsigned int connect_timeout_s,
   unsigned int heartbeat_timeout_s,
   unsigned int disconnect_timeout_s,
   unsigned int reconnnect_timeout_s,
   unsigned int address_resolution_timeout_s,
   BSC_NODE_SWITCH_EVENT_FUNC event_func,
   void* user_arg,
   BSC_NODE_SWITCH_HANDLE* h);

BACNET_STACK_EXPORT
void bsc_node_switch_stop(BSC_NODE_SWITCH_HANDLE h);

BACNET_STACK_EXPORT
bool bsc_node_switch_stopped(BSC_NODE_SWITCH_HANDLE h);

BACNET_STACK_EXPORT
bool bsc_node_switch_started(BSC_NODE_SWITCH_HANDLE h);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_switch_connect(
    BSC_NODE_SWITCH_HANDLE h,
    BACNET_SC_VMAC_ADDRESS *dest,
    char** urls, size_t urls_cnt);

BACNET_STACK_EXPORT
void bsc_node_switch_disconnect(BSC_NODE_SWITCH_HANDLE h, 
                                BACNET_SC_VMAC_ADDRESS *dest);

BACNET_STACK_EXPORT 
void bsc_node_switch_process_address_resolution(BSC_NODE_SWITCH_HANDLE h, 
                                                BSC_ADDRESS_RESOLUTION* r);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_switch_send(
     BSC_NODE_SWITCH_HANDLE h,
     uint8_t *pdu,
     unsigned pdu_len);

#endif