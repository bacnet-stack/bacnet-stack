/**
 * @file
 * @brief BACnet secure connect node switch function API.
 *        In general, user should not use that API directly,
 *        BACnet/SC datalink API should be used.
 * @author Kirill Neznamov <kirill\.neznamov@dsr-corporation\.com>
 * @date October 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DATALINK_BSC_NODE_SWITCH_H
#define BACNET_DATALINK_BSC_NODE_SWITCH_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bsc-node.h"

typedef void *BSC_NODE_SWITCH_HANDLE;

typedef enum {
    BSC_NODE_SWITCH_EVENT_STARTED = 1,
    BSC_NODE_SWITCH_EVENT_STOPPED = 2,
    BSC_NODE_SWITCH_EVENT_RECEIVED = 3,
    BSC_NODE_SWITCH_EVENT_DUPLICATED_VMAC = 4,

    /* BSC_NODE_SWITCH_EVENT_CONNECTED event is emitted every */
    /* time remote peer connects only if bsc_node_switch_connect() */
    /* was called after start for corresponded mac or url/urls. */
    /* Events indication are stopped only */
    /* if node switch was stopped or bsc_node_switch_disconnect() */
    /* was called. For a connection initiated from a remote peer that */
    /* event won't be ever emitted. */

    BSC_NODE_SWITCH_EVENT_CONNECTED = 5,

    /* The BSC_NODE_SWITCH_EVENT_DISCONNECTED event is emitted */
    /* every time remote peer disconnects only if */
    /* bsc_node_switch_connect() was called after start. */
    /* If user called bsc_node_switch_disconnect() or stopped */
    /* node switch, after last event indication next event indications */
    /* are stopped for corresponded peer with corresponded vmac. */
    BSC_NODE_SWITCH_EVENT_DISCONNECTED = 6

} BSC_NODE_SWITCH_EVENT;

/* dest parameter is actual only for */
/* BSC_NODE_SWITCH_EVENT_CONNECTED and BSC_NODE_SWITCH_EVENT_DISCONNECTED */
/* events. It is the parameter which was specified in */
/* bsc_node_switch_connect() or bsc_node_switch_disconnect() calls. */

typedef void (*BSC_NODE_SWITCH_EVENT_FUNC)(
    BSC_NODE_SWITCH_EVENT ev,
    BSC_NODE_SWITCH_HANDLE h,
    void *user_arg,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *pdu,
    size_t pdu_len,
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
    char *iface,
    BACNET_SC_UUID *local_uuid,
    BACNET_SC_VMAC_ADDRESS *local_vmac,
    uint16_t max_local_bvlc_len,
    uint16_t max_local_npdu_len,
    uint16_t connect_timeout_s,
    uint16_t heartbeat_timeout_s,
    uint16_t disconnect_timeout_s,
    uint16_t reconnnect_timeout_s,
    uint16_t address_resolution_timeout_s,
    bool direct_connect_accept_enable,
    bool direct_connect_initiate_enable,
    BSC_NODE_SWITCH_EVENT_FUNC event_func,
    void *user_arg,
    BSC_NODE_SWITCH_HANDLE *h);

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
    char **urls,
    size_t urls_cnt);

BACNET_STACK_EXPORT
bool bsc_node_switch_connected(
    BSC_NODE_SWITCH_HANDLE h,
    BACNET_SC_VMAC_ADDRESS *dest,
    char **urls,
    size_t urls_cnt);

BACNET_STACK_EXPORT
void bsc_node_switch_disconnect(
    BSC_NODE_SWITCH_HANDLE h, BACNET_SC_VMAC_ADDRESS *dest);

BACNET_STACK_EXPORT
void bsc_node_switch_process_address_resolution(
    BSC_NODE_SWITCH_HANDLE h, BSC_ADDRESS_RESOLUTION *r);

BACNET_STACK_EXPORT
BSC_SC_RET
bsc_node_switch_send(BSC_NODE_SWITCH_HANDLE h, uint8_t *pdu, size_t pdu_len);

BACNET_STACK_EXPORT
void bsc_node_switch_maintenance_timer(uint16_t seconds);

#endif
