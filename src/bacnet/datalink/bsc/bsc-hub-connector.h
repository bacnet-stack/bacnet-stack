/**
 * @file
 * @brief BACNet secure connect hub connector API.
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
#include <pthread.h>
#include "bacnet/datalink/bsc/bsc-connection.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"

struct BSC_Hub_Connector;
typedef struct BSC_Hub_Connector BSC_HUB_CONNECTOR;

BACNET_STACK_EXPORT
bool bsc_hub_connector_start(BSC_CONNECTION_CTX *ctx,
                             BSC_HUB_CONNECTOR *c,
                             char* primaryURL,
                             char* failoverURL,
                             unsigned int reconnnect_timeout_s);

BACNET_STACK_EXPORT
void bsc_hub_connector_stop(BSC_HUB_CONNECTOR *c);

BACNET_STACK_EXPORT
bool bsc_hub_connector_send(BSC_HUB_CONNECTOR *c,
     uint8_t *pdu,
     unsigned pdu_len);

BACNET_STACK_EXPORT
int bsc_hub_connector_recv(BSC_HUB_CONNECTOR *c,
               uint8_t *pdu,
               uint16_t max_pdu,
               unsigned int timeout);

#endif