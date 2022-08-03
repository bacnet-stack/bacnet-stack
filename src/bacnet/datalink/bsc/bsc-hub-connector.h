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

BACNET_STACK_EXPORT
bool bsc_hub_connector_set_configuration(void);

BACNET_STACK_EXPORT
bool bsc_hub_connector_start(void);

BACNET_STACK_EXPORT
bool bsc_hub_connector_stop(void);

 BACNET_STACK_EXPORT
 int bsc_hub_connector_send(BACNET_SC_VMAC_ADDRESS *dest,
     uint8_t *pdu,
     unsigned pdu_len);

 BACNET_STACK_EXPORT
 uint16_t bsc_hub_connector_recv(BACNET_SC_VMAC_ADDRESS *src,
                   uint8_t *pdu,
                   uint16_t max_pdu,
                   unsigned int timeout);

#endif