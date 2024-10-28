/**
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file
 * @author Mikhail Antropov
 * @date 2023
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef BACNET_SC_STATUS_H
#define BACNET_SC_STATUS_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/datetime.h"
#include "bacnet/datalink/bsc/bsc-conf.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Support
   - SC_Hub_Function_Connection_Status
   - SC_Direct_Connect_Connection_Status
   - SC_FailedConnectionRequests
*/

typedef struct BACnetUUID_T_ {
    union {
        struct guid {
            uint32_t time_low;
            uint16_t time_mid;
            uint16_t time_hi_and_version;
            uint8_t clock_seq_and_node[8];
        } guid;
        uint8_t uuid128[16];
    } uuid;
} BACNET_UUID;

#define BACNET_ERROR_STRING_LENGTH BSC_CONF_WEBSOCKET_ERR_DESC_STR_MAX_LEN
#define BACNET_URI_LENGTH BSC_CONF_WSURL_MAX_LEN
#define BACNET_PEER_VMAC_LENGTH BVLC_SC_VMAC_SIZE

enum BACNET_HOST_N_PORT_TYPE {
    BACNET_HOST_N_PORT_IP = 1,
    BACNET_HOST_N_PORT_HOST = 2
};

typedef struct BACnetHostNPort_data_T {
    uint8_t type;
    char host[BACNET_URI_LENGTH];
    uint16_t port;
} BACNET_HOST_N_PORT_DATA;

typedef struct BACnetSCHubConnectionStatus_T {
    BACNET_SC_CONNECTION_STATE State; /*index = 0 */
    BACNET_DATE_TIME Connect_Timestamp; /*index = 1 */
    BACNET_DATE_TIME Disconnect_Timestamp; /*index = 2 */
    /* optionals - use ERROR_CODE_DEFAULT for default value */
    BACNET_ERROR_CODE Error; /* index = 3 */
    char Error_Details[BACNET_ERROR_STRING_LENGTH]; /* index = 4 */
} BACNET_SC_HUB_CONNECTION_STATUS;

typedef struct BACnetSCHubFunctionConnectionStatus_T {
    BACNET_SC_CONNECTION_STATE State;
    BACNET_DATE_TIME Connect_Timestamp;
    BACNET_DATE_TIME Disconnect_Timestamp;
    BACNET_HOST_N_PORT_DATA Peer_Address;
    uint8_t Peer_VMAC[BACNET_PEER_VMAC_LENGTH];
    BACNET_UUID Peer_UUID;
    BACNET_ERROR_CODE Error;
    char Error_Details[BACNET_ERROR_STRING_LENGTH];
} BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS;

typedef struct BACnetSCFailedConnectionRequest_T {
    BACNET_DATE_TIME Timestamp;
    BACNET_HOST_N_PORT_DATA Peer_Address;
    uint8_t Peer_VMAC[BACNET_PEER_VMAC_LENGTH];
    BACNET_UUID Peer_UUID;
    BACNET_ERROR_CODE Error;
    char Error_Details[BACNET_ERROR_STRING_LENGTH];
} BACNET_SC_FAILED_CONNECTION_REQUEST;

typedef enum BACnetRouterStatus {
    BACNET_ROUTER_STATUS_AVAILABLE = 0,
    BACNET_ROUTER_STATUS_BUSY = 1,
    BACNET_ROUTER_STATUS_DISCONNECTED = 2,
    BACNET_ROUTER_STATUS_MAX = 2
} BACNET_ROUTER_STATUS;

typedef struct BACnetRouterEntry_T {
    uint16_t Network_Number;
    uint8_t Mac_Address[6];
    BACNET_ROUTER_STATUS Status;
    uint8_t Performance_Index;
} BACNET_ROUTER_ENTRY;

typedef struct BACnetSCDirectConnectionStatus_T {
    char URI[BACNET_URI_LENGTH];
    BACNET_SC_CONNECTION_STATE State;
    BACNET_DATE_TIME Connect_Timestamp;
    BACNET_DATE_TIME Disconnect_Timestamp;
    BACNET_HOST_N_PORT_DATA Peer_Address;
    uint8_t Peer_VMAC[BACNET_PEER_VMAC_LENGTH];
    BACNET_UUID Peer_UUID;
    BACNET_ERROR_CODE Error;
    char Error_Details[BACNET_ERROR_STRING_LENGTH];
} BACNET_SC_DIRECT_CONNECTION_STATUS;

/* */
/* Encode / decode */
/* */

BACNET_STACK_EXPORT
int bacapp_encode_SCHubConnection(
    uint8_t *apdu, const BACNET_SC_HUB_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_SCHubConnection(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_SC_HUB_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_context_SCHubConnection(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_number,
    BACNET_SC_HUB_CONNECTION_STATUS *value);

BACNET_STACK_EXPORT
int bacapp_encode_SCHubFunctionConnection(
    uint8_t *apdu, const BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_SCHubFunctionConnection(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_context_SCHubFunctionConnection(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_number,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value);

BACNET_STACK_EXPORT
int bacapp_encode_SCFailedConnectionRequest(
    uint8_t *apdu, const BACNET_SC_FAILED_CONNECTION_REQUEST *value);
BACNET_STACK_EXPORT
int bacapp_decode_SCFailedConnectionRequest(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_SC_FAILED_CONNECTION_REQUEST *value);
BACNET_STACK_EXPORT
int bacapp_decode_context_SCFailedConnectionRequest(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_number,
    BACNET_SC_FAILED_CONNECTION_REQUEST *value);

BACNET_STACK_EXPORT
int bacapp_encode_RouterEntry(uint8_t *apdu, const BACNET_ROUTER_ENTRY *value);
BACNET_STACK_EXPORT
int bacapp_decode_RouterEntry(
    const uint8_t *apdu, size_t apdu_size, BACNET_ROUTER_ENTRY *value);
BACNET_STACK_EXPORT
int bacapp_encode_context_RouterEntry(
    uint8_t *apdu, uint8_t tag_number, const BACNET_ROUTER_ENTRY *value);
BACNET_STACK_EXPORT
int bacapp_decode_context_RouterEntry(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_number,
    BACNET_ROUTER_ENTRY *value);

BACNET_STACK_EXPORT
int bacapp_encode_SCDirectConnection(
    uint8_t *apdu, const BACNET_SC_DIRECT_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_SCDirectConnection(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_context_SCDirectConnection(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_number,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value);

BACNET_STACK_EXPORT
int bacapp_snprintf_SCFailedConnectionRequest(
    char *str, size_t str_len, const BACNET_SC_FAILED_CONNECTION_REQUEST *req);
BACNET_STACK_EXPORT
int bacapp_snprintf_SCHubFunctionConnection(
    char *str,
    size_t str_len,
    const BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *st);
BACNET_STACK_EXPORT
int bacapp_snprintf_SCDirectConnection(
    char *str, size_t str_len, const BACNET_SC_DIRECT_CONNECTION_STATUS *st);
BACNET_STACK_EXPORT
int bacapp_snprintf_SCHubConnection(
    char *str, size_t str_len, const BACNET_SC_HUB_CONNECTION_STATUS *st);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
