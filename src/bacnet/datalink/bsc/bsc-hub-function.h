/**
 * @file
 * @brief BACNet secure connect hub function API.
 * @author Kirill Neznamov
 * @date July 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BACNET__HUB__FUNCTION__INCLUDED__
#define __BACNET__HUB__FUNCTION__INCLUDED__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacnet/datalink/bsc/bsc-retcodes.h"

typedef enum {
    BSC_HUBF_EVENT_STARTED = 1,
    BSC_HUBF_EVENT_STOPPED = 2
} BSC_HUB_FUNCTION_EVENT;

typedef void (*HUB_FUNCTION_EVENT)(BSC_HUB_FUNCTION_EVENT ev, BSC_SC_RET err);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_hub_function_start(
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
   unsigned int disconnect_timeout_s,
   char* primaryURL,
   char* failoverURL,
   unsigned int reconnnect_timeout_s,
   HUB_CONNECTOR_EVENT event_func);

BACNET_STACK_EXPORT
void bsc_hub_function_stop(void);

#endif