/**
 * @file
 * @brief Configuration file of BACNet/SC datalink.
 * @author Kirill Neznamov
 * @date August 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BSC__CONF__INCLUDED__
#define __BSC__CONF__INCLUDED__

// THIS should not be changed, most of BACNet/SC devices must have
// hub connector, it uses 2 connections
#define BSC_CONF_HUB_CONNECTOR_CONNECTIONS_NUM 2

// Total amount of client(initiator) webosocket connections
#define BSC_CONF_CLIENT_CONNECTIONS_NUM (BSC_CONF_HUB_CONNECTOR_CONNECTIONS_NUM)

#define BSC_CONF_RX_BUFFER_SIZE 4096
#define BSC_CONF_TX_BUFFER_SIZE 4096

#endif