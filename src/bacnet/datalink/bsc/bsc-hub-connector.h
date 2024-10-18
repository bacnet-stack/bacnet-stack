/**
 * @file
 * @brief BACNet secure connect hub connector API.
 *        In general, user should not use that API directly,
 *        BACNet/SC datalink API should be used.
 * @author Kirill Neznamov
 * @date July 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BACNET__HUB__CONNECTOR__INCLUDED__
#define __BACNET__HUB__CONNECTOR__INCLUDED__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacnet/datalink/bsc/bsc-retcodes.h"

typedef void *BSC_HUB_CONNECTOR_HANDLE;

typedef enum {
    BSC_HUBC_EVENT_CONNECTED_PRIMARY = 1,
    BSC_HUBC_EVENT_CONNECTED_FAILOVER = 2,
    BSC_HUBC_EVENT_ERROR_DUPLICATED_VMAC = 3,
    BSC_HUBC_EVENT_RECEIVED = 4,
    BSC_HUBC_EVENT_STOPPED = 5
} BSC_HUB_CONNECTOR_EVENT;

typedef void (*BSC_HUB_CONNECTOR_EVENT_FUNC)(
    BSC_HUB_CONNECTOR_EVENT ev,
    BSC_HUB_CONNECTOR_HANDLE h,
    void *user_arg,
    uint8_t *pdu,
    uint16_t pdu_len,
    BVLC_SC_DECODED_MESSAGE *decoded_pdu);

BACNET_STACK_EXPORT
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
    unsigned int reconnnect_timeout_s,
    BSC_HUB_CONNECTOR_EVENT_FUNC event_func,
    void *user_arg,
    BSC_HUB_CONNECTOR_HANDLE *h);

BACNET_STACK_EXPORT
void bsc_hub_connector_stop(BSC_HUB_CONNECTOR_HANDLE h);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_hub_connector_send(
    BSC_HUB_CONNECTOR_HANDLE h, uint8_t *pdu, unsigned pdu_len);

BACNET_STACK_EXPORT
bool bsc_hub_connector_stopped(BSC_HUB_CONNECTOR_HANDLE h);

BACNET_STACK_EXPORT
BACNET_SC_HUB_CONNECTOR_STATE
bsc_hub_connector_state(BSC_HUB_CONNECTOR_HANDLE h);

BACNET_STACK_EXPORT
BACNET_SC_HUB_CONNECTION_STATUS *
bsc_hub_connector_status(BSC_HUB_CONNECTOR_HANDLE h, bool primary);

BACNET_STACK_EXPORT
void bsc_hub_connector_maintenance_timer(uint16_t seconds);

#endif
