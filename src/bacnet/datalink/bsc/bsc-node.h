/**
 * @file
 * @brief BACNet secure connect node API.
 *        In general, user should not use that API directly,
 *        BACNet/SC datalink API should be used.
 * @author Kirill Neznamov
 * @date October 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#ifndef __BACNET__NODE__INCLUDED__
#define __BACNET__NODE__INCLUDED__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bacnet/datalink/bsc/bsc-conf.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/object/sc_netport.h"

typedef struct BSC_Node BSC_NODE;

typedef enum {
    BSC_NODE_EVENT_STARTED = 1,
    BSC_NODE_EVENT_STOPPED = 2,
    BSC_NODE_EVENT_RESTARTED = 3,
    BSC_NODE_EVENT_RECEIVED_NPDU = 4,
    BSC_NODE_EVENT_RECEIVED_RESULT = 5,
    BSC_NODE_EVENT_RECEIVED_ADVERTISIMENT = 6,
    BSC_NODE_EVENT_DIRECT_CONNECTED = 7,
    BSC_NODE_EVENT_DIRECT_DISCONNECTED = 8
} BSC_NODE_EVENT;

typedef void (*BSC_NODE_EVENT_FUNC)(BSC_NODE* node, BSC_NODE_EVENT ev,
                                    BACNET_SC_VMAC_ADDRESS *dest,
                                    uint8_t *pdu, uint16_t pdu_len);
typedef struct {
    bool used;
    BACNET_SC_VMAC_ADDRESS vmac;
    uint8_t utf8_urls[BSC_CONF_NODE_MAX_URIS_NUM_IN_ADDRESS_RESOLUTION_ACK][BSC_CONF_NODE_MAX_URI_SIZE_IN_ADDRESS_RESOLUTION_ACK+1];
    size_t urls_num;
    struct mstimer fresh_timer;
} BSC_ADDRESS_RESOLUTION;

typedef struct {
   uint8_t *ca_cert_chain;
   size_t ca_cert_chain_size;
   uint8_t *cert_chain;
   size_t cert_chain_size;
   uint8_t *key;
   size_t key_size;
   BACNET_SC_UUID *local_uuid;
   BACNET_SC_VMAC_ADDRESS *local_vmac;
   uint16_t max_local_bvlc_len;
   uint16_t max_local_npdu_len;
   unsigned int connect_timeout_s;
   unsigned int heartbeat_timeout_s;
   unsigned int disconnect_timeout_s;
   unsigned int reconnnect_timeout_s;
   unsigned int address_resolution_timeout_s;
   unsigned int address_resolution_freshness_timeout_s;
   char* primaryURL;
   char* failoverURL;
   uint16_t hub_server_port;
   uint16_t direct_server_port;
   char* iface;
   bool direct_connect_accept_enable;
   bool direct_connect_initiate_enable;
   bool hub_function_enabled;
   char *direct_connection_accept_uris;     /* URIs joined ' 'space */
   unsigned int direct_connection_accept_uris_len;
   BSC_NODE_EVENT_FUNC event_func;
} BSC_NODE_CONF;

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_init(
   BSC_NODE_CONF* conf,
   BSC_NODE** node);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_deinit(BSC_NODE *node);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_start(BSC_NODE* node);

BACNET_STACK_EXPORT
void bsc_node_stop(BSC_NODE *node);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_send(BSC_NODE* node,
     uint8_t *pdu,
     unsigned pdu_len);

BACNET_STACK_EXPORT
BSC_ADDRESS_RESOLUTION* bsc_node_get_address_resolution(
      void* node,
      BACNET_SC_VMAC_ADDRESS* vmac);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_send_address_resolution(
      void* node,
      BACNET_SC_VMAC_ADDRESS* dest);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_hub_connector_send(
     void *user_arg,
     uint8_t *pdu,
     unsigned pdu_len);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_node_connect_direct(
    BSC_NODE *node,
    BACNET_SC_VMAC_ADDRESS *dest,
    char** urls, size_t urls_cnt);

BACNET_STACK_EXPORT
void bsc_node_disconnect_direct(
    BSC_NODE *node, BACNET_SC_VMAC_ADDRESS *dest);

BACNET_STACK_EXPORT
BVLC_SC_HUB_CONNECTION_STATUS
bsc_node_hub_connector_status(BSC_NODE *node);

BACNET_STACK_EXPORT
bool bsc_node_direct_connection_established(
    BSC_NODE *node,
    BACNET_SC_VMAC_ADDRESS *dest,
    char** urls, size_t urls_cnt);

BACNET_STACK_EXPORT
void bsc_node_maintenance_timer(uint16_t seconds);

#endif
