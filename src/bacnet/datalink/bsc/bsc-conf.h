/**
 * @file
 * @brief Configuration file of BACNet/SC datalink.
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date August 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DATALINK_BSC_CONF_H
#define BACNET_DATALINK_BSC_CONF_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bvlc-sc.h"

#if !defined(BACDL_BSC)
#define BSC_CONF_TX_PRE 0
#else
#ifndef bsd
#define bsd 1
#endif
#ifndef linux
#define linux 2
#endif
#ifndef win32
#define win32 3
#endif
#if BACNET_PORT == bsd || BACNET_PORT == linux || BACNET_PORT == win32
#include <libwebsockets.h>
#define BSC_CONF_TX_PRE LWS_PRE
#else
#define BSC_CONF_TX_PRE 0
#endif
#endif

#ifndef BSC_CONF_HUB_CONNECTORS_NUM
#define BSC_CONF_HUB_CONNECTORS_NUM 1
#endif

#ifndef BSC_CONF_HUB_FUNCTIONS_NUM
#define BSC_CONF_HUB_FUNCTIONS_NUM 1
#endif

#ifndef BSC_CONF_NODE_SWITCHES_NUM
#define BSC_CONF_NODE_SWITCHES_NUM 1
#endif

#ifndef BSC_CONF_NODES_NUM
#define BSC_CONF_NODES_NUM 1
#endif

#ifndef BVLC_SC_NPDU_SIZE_CONF
/* 16 bytes is sum of all sizes of all static (non variable)
   fields of header of BVLC message */
#define BVLC_SC_NPDU_SIZE_CONF ((MAX_PDU) + 16)
#endif

#ifndef BSC_CONF_WEBSOCKET_RX_BUFFER_LEN
#define BSC_CONF_WEBSOCKET_RX_BUFFER_LEN BVLC_SC_NPDU_SIZE_CONF
#endif

/* THIS should not be changed, most of BACNet/SC devices must have */
/* hub connector, it uses 2 connections */
#ifndef BSC_CONF_HUB_CONNECTOR_CONNECTIONS_NUM
#define BSC_CONF_HUB_CONNECTOR_CONNECTIONS_NUM (BSC_CONF_HUB_CONNECTORS_NUM * 2)
#endif

#ifndef BSC_CONF_HUB_FUNCTION_CONNECTIONS_NUM
#define BSC_CONF_HUB_FUNCTION_CONNECTIONS_NUM (BSC_CONF_HUB_FUNCTIONS_NUM * 10)
#endif

#ifndef BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM
#define BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM 10
#endif

/* Total amount of client(initiator) webosocket connections */
#ifndef BSC_CONF_CLIENT_CONNECTIONS_NUM
#define BSC_CONF_CLIENT_CONNECTIONS_NUM       \
    (BSC_CONF_HUB_CONNECTOR_CONNECTIONS_NUM + \
     BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM * BSC_CONF_NODE_SWITCHES_NUM)
#endif

#ifndef BSC_CONF_SERVER_HUB_CONNECTIONS_MAX_NUM
#define BSC_CONF_SERVER_HUB_CONNECTIONS_MAX_NUM \
    (BSC_CONF_HUB_FUNCTION_CONNECTIONS_NUM)
#endif

#ifndef BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM
#define BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM \
    (BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM * BSC_CONF_NODE_SWITCHES_NUM)
#endif

#define BSC_CONF_SOCKET_TX_BUFFERED_PACKET_NUM 2
#define BSC_CONF_DATALINK_BUFFERED_PACKET_NUM 10

#define BSC_CONF_SOCK_RX_BUFFER_SIZE BVLC_SC_NPDU_SIZE_CONF

/* 2 bytes is a prefix containing BVLC message length.
   BSC_CONF_TX_PRE - some reserved bytes before actual payload.
   Some libs like libwebsocket requires some bytes to be reserved
   before actual payload for sending, so BSC_CONF_TX_PRE is used for
   that purpose (it allows to avoid copying of payload and
   buffer reallocation)
*/

#define BSC_CONF_SOCK_TX_BUFFER_SIZE                  \
    ((BVLC_SC_NPDU_SIZE_CONF + 2 + BSC_CONF_TX_PRE) * \
     BSC_CONF_SOCKET_TX_BUFFERED_PACKET_NUM)

/* datalink RX buffer size is always rounded to next power of two */
/* so if final buffer size is 1628 it will be rounded to 2048 */

#define BSC_CONF_DATALINK_RX_BUFFER_SIZE \
    (BVLC_SC_NPDU_SIZE_CONF * BSC_CONF_DATALINK_BUFFERED_PACKET_NUM)

#ifndef BSC_CONF_WSURL_MAX_LEN
#define BSC_CONF_WSURL_MAX_LEN 128
#endif

#ifndef BSC_CONF_WEBSOCKET_ERR_DESC_STR_MAX_LEN
#define BSC_CONF_WEBSOCKET_ERR_DESC_STR_MAX_LEN 128
#endif

#ifndef BSC_CONF_NODE_MAX_URI_SIZE_IN_ADDRESS_RESOLUTION_ACK
#define BSC_CONF_NODE_MAX_URI_SIZE_IN_ADDRESS_RESOLUTION_ACK \
    BSC_CONF_WSURL_MAX_LEN
#endif

#ifndef BSC_CONF_NODE_MAX_URIS_NUM_IN_ADDRESS_RESOLUTION_ACK
#define BSC_CONF_NODE_MAX_URIS_NUM_IN_ADDRESS_RESOLUTION_ACK    \
    (BSC_CONF_SOCK_RX_BUFFER_SIZE /                             \
         BSC_CONF_NODE_MAX_URI_SIZE_IN_ADDRESS_RESOLUTION_ACK - \
     1)
#endif

#ifndef BSC_CONF_OPERATIONAL_CERTIFICATE_FILE_INSTANCE
#define BSC_CONF_OPERATIONAL_CERTIFICATE_FILE_INSTANCE 5
#endif

#ifndef BSC_CONF_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE
#define BSC_CONF_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE 6
#endif

#ifndef BSC_CONF_ISSUER_CERTIFICATE_FILE_1_INSTANCE
#define BSC_CONF_ISSUER_CERTIFICATE_FILE_1_INSTANCE 7
#endif

#ifndef BSC_CONF_ISSUER_CERTIFICATE_FILE_2_INSTANCE
#define BSC_CONF_ISSUER_CERTIFICATE_FILE_2_INSTANCE 8
#endif

#ifndef BSC_CONF_ISSUER_CERTIFICATE_FILE_3_INSTANCE
#define BSC_CONF_ISSUER_CERTIFICATE_FILE_3_INSTANCE 9
#endif

#ifndef BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM
#define BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM \
    BSC_CONF_HUB_FUNCTION_CONNECTIONS_NUM
#endif

#ifndef BSC_CONF_NODE_SWITCH_CONNECTION_STATUS_MAX_NUM
#define BSC_CONF_NODE_SWITCH_CONNECTION_STATUS_MAX_NUM \
    BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM
#endif

#ifndef BSC_CONF_FAILED_CONNECTION_STATUS_MAX_NUM
#define BSC_CONF_FAILED_CONNECTION_STATUS_MAX_NUM 4
#endif

#endif
