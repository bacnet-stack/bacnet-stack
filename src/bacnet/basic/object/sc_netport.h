/**
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file
 * @author Mikhail Antropov
 * @date 2022
 * @brief Helper Network port objects to implement secure connect attributes
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
#ifndef BACNET_SC_NETPORT_H
#define BACNET_SC_NETPORT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"
#include "bacnet/secure_connect.h"
#include "bacnet/datalink/bsc/bsc-conf.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"

/* BACnet file instance numbers */
#ifndef BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE
#define BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE \
    BSC_CONF_ISSUER_CERTIFICATE_FILE_1_INSTANCE
#endif
#ifndef BSC_ISSUER_CERTIFICATE_FILE_2_INSTANCE
#define BSC_ISSUER_CERTIFICATE_FILE_2_INSTANCE \
    BSC_CONF_ISSUER_CERTIFICATE_FILE_2_INSTANCE
#endif
#ifndef BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE
#define BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE \
    BSC_CONF_OPERATIONAL_CERTIFICATE_FILE_INSTANCE
#endif
#ifndef BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE
#define BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE \
    BSC_CONF_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BACNET_SC_HUB_URI_MAX 2
#ifndef BACNET_SC_DIRECT_ACCEPT_URI_MAX
#define BACNET_SC_DIRECT_ACCEPT_URI_MAX 2
#endif

#define BACNET_ISSUER_CERT_FILE_MAX 2
#define BACNET_BINDING_STRING_LENGTH 80

#define BACNET_SC_BINDING_SEPARATOR ':'

/* */
/* getter / setter */
/* */

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER Network_Port_Max_BVLC_Length_Accepted(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Max_BVLC_Length_Accepted_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_Max_BVLC_Length_Accepted_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER Network_Port_Max_NPDU_Length_Accepted(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Max_NPDU_Length_Accepted_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_Max_NPDU_Length_Accepted_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
bool Network_Port_SC_Primary_Hub_URI(
    uint32_t object_instance, BACNET_CHARACTER_STRING *str);
const char *Network_Port_SC_Primary_Hub_URI_char(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Primary_Hub_URI_Set(
    uint32_t object_instance, const char *str);
bool Network_Port_SC_Primary_Hub_URI_Dirty_Set(
    uint32_t object_instance, const char *str);

BACNET_STACK_EXPORT
bool Network_Port_SC_Failover_Hub_URI(
    uint32_t object_instance, BACNET_CHARACTER_STRING *str);
const char *Network_Port_SC_Failover_Hub_URI_char(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Failover_Hub_URI_Set(
    uint32_t object_instance, const char *str);
bool Network_Port_SC_Failover_Hub_URI_Dirty_Set(
    uint32_t object_instance, const char *str);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER Network_Port_SC_Minimum_Reconnect_Time(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Minimum_Reconnect_Time_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Minimum_Reconnect_Time_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER Network_Port_SC_Maximum_Reconnect_Time(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Maximum_Reconnect_Time_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Maximum_Reconnect_Time_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER Network_Port_SC_Connect_Wait_Timeout(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Connect_Wait_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Connect_Wait_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER Network_Port_SC_Disconnect_Wait_Timeout(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Disconnect_Wait_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Disconnect_Wait_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER Network_Port_SC_Heartbeat_Timeout(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Heartbeat_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Heartbeat_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_SC_HUB_CONNECTOR_STATE Network_Port_SC_Hub_Connector_State(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Connector_State_Set(
    uint32_t object_instance, BACNET_SC_HUB_CONNECTOR_STATE value);

BACNET_STACK_EXPORT
uint32_t Network_Port_Operational_Certificate_File(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Operational_Certificate_File_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint32_t Network_Port_Issuer_Certificate_File(
    uint32_t object_instance, uint8_t index);
BACNET_STACK_EXPORT
bool Network_Port_Issuer_Certificate_File_Set(
    uint32_t object_instance, uint8_t index, uint32_t value);
BACNET_STACK_EXPORT
int Network_Port_Issuer_Certificate_File_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu);

BACNET_STACK_EXPORT
uint32_t Network_Port_Certificate_Signing_Request_File(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Certificate_Signing_Request_File_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
BACNET_ROUTER_ENTRY *Network_Port_Routing_Table_Find(
    uint32_t object_instance, uint16_t Network_Number);
BACNET_STACK_EXPORT
BACNET_ROUTER_ENTRY *Network_Port_Routing_Table_Get(
    uint32_t object_instance, uint8_t index);
BACNET_STACK_EXPORT
bool Network_Port_Routing_Table_Add(uint32_t object_instance,
    uint16_t network_number,
    uint8_t *mac,
    uint8_t mac_len,
    uint8_t status,
    uint8_t performance_index);
BACNET_STACK_EXPORT
bool Network_Port_Routing_Table_Delete(
    uint32_t object_instance, uint16_t Network_Number);
BACNET_STACK_EXPORT
bool Network_Port_Routing_Table_Delete_All(uint32_t object_instance);
BACNET_STACK_EXPORT
uint8_t Network_Port_Routing_Table_Count(uint32_t object_instance);
BACNET_STACK_EXPORT
int Network_Port_Routing_Table_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu);

BACNET_STACK_EXPORT
BACNET_SC_HUB_CONNECTION_STATUS *Network_Port_SC_Primary_Hub_Connection_Status(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Primary_Hub_Connection_Status_Set(uint32_t object_instance,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_DATE_TIME *connect_ts,
    BACNET_DATE_TIME *disconnect_ts,
    BACNET_ERROR_CODE error,
    const char *error_details);

BACNET_STACK_EXPORT
BACNET_SC_HUB_CONNECTION_STATUS *Network_Port_SC_Failover_Hub_Connection_Status(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Failover_Hub_Connection_Status_Set(
    uint32_t object_instance,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_DATE_TIME *connect_ts,
    BACNET_DATE_TIME *disconnect_ts,
    BACNET_ERROR_CODE error,
    const char *error_details);

BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Function_Enable(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Function_Enable_Set(
    uint32_t object_instance, bool value);
bool Network_Port_SC_Hub_Function_Enable_Dirty_Set(
    uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Function_Accept_URI(
    uint32_t object_instance, uint8_t index, BACNET_CHARACTER_STRING *str);
BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Function_Accept_URI_Set(
    uint32_t object_instance, uint8_t index, const char *str);
BACNET_STACK_EXPORT
int Network_Port_SC_Hub_Function_Accept_URI_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu);

bool Network_Port_SC_Hub_Function_Accept_URI_Dirty_Set(
    uint32_t object_instance, uint8_t index, const char *str);
const char *Network_Port_SC_Hub_Function_Accept_URIs_char(
    uint32_t object_instance);
bool Network_Port_SC_Hub_Function_Accept_URIs_Set(
    uint32_t object_instance, const char *str);

/*
    Internal representation of the SC_Hub_Function_Binding value is a
    struct {
        uint16 port;
        char interface_name[BACNET_BINDING_STRING_LENGTH - sizeof(uint16)];
    }
    External representation is string e.q "interface_name:port_number".

    The Network_Port_SC_Hub_Function_Binding() and
    the Network_Port_SC_Hub_Function_Binding_Set() do transform from/to.

    The Network_Port_SC_Hub_Function_Binding_Dirty sets value like
    external representation.
*/
BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Function_Binding(
    uint32_t object_instance, BACNET_CHARACTER_STRING *str);
void Network_Port_SC_Hub_Function_Binding_get(
    uint32_t object_instance, uint16_t *port, char **ifname);
BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Function_Binding_Set(
    uint32_t object_instance, const char *str);
bool Network_Port_SC_Hub_Function_Binding_Dirty_Set(
    uint32_t object_instance, const char *str);

BACNET_STACK_EXPORT
BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *
Network_Port_SC_Hub_Function_Connection_Status_Get(
    uint32_t object_instance, uint8_t index);
BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Function_Connection_Status_Add(
    uint32_t object_instance,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_DATE_TIME *connect_ts,
    BACNET_DATE_TIME *disconnect_ts,
    BACNET_HOST_N_PORT_DATA *peer_address,
    uint8_t *peer_VMAC,
    uint8_t *peer_UUID,
    BACNET_ERROR_CODE error,
    const char *error_details);
BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Function_Connection_Status_Delete_All(
    uint32_t object_instance);
BACNET_STACK_EXPORT
int Network_Port_SC_Hub_Function_Connection_Status_Count(
    uint32_t object_instance);
BACNET_STACK_EXPORT
int Network_Port_SC_Hub_Function_Connection_Status_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu);

BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Initiate_Enable(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Initiate_Enable_Set(
    uint32_t object_instance, bool value);
bool Network_Port_SC_Direct_Connect_Initiate_Enable_Dirty_Set(
    uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Accept_Enable(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Accept_Enable_Set(
    uint32_t object_instance, bool value);
bool Network_Port_SC_Direct_Connect_Accept_Enable_Dirty_Set(
    uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Accept_URI(
    uint32_t object_instance, uint8_t index, BACNET_CHARACTER_STRING *str);
BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Accept_URI_Set(
    uint32_t object_instance, uint8_t index, const char *str);
BACNET_STACK_EXPORT
int Network_Port_SC_Direct_Connect_Accept_URI_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu);

bool Network_Port_SC_Direct_Connect_Accept_URI_Dirty_Set(
    uint32_t object_instance, uint8_t index, const char *str);
char *Network_Port_SC_Direct_Connect_Accept_URIs_char(uint32_t object_instance);
bool Network_Port_SC_Direct_Connect_Accept_URIs_Set(
    uint32_t object_instance, const char *str);

/*
    Internal representation of the SC_Direct_Connect_Binding value is a
    struct {
        uint16 port;
        char interface_name[BACNET_BINDING_STRING_LENGTH - sizeof(uint16)];
    }
    External representation is string e.q "interface_name:port_number".

    The Network_Port_SC_Direct_Connect_Binding() and
    the Network_Port_SC_Direct_Connect_Binding_Set() do transform from/to.

    The Network_Port_SC_Direct_Connect_Binding_Dirty sets value like
    external representation.
*/
BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Binding(
    uint32_t object_instance, BACNET_CHARACTER_STRING *str);
void Network_Port_SC_Direct_Connect_Binding_get(
    uint32_t object_instance, uint16_t *port, char **ifname);
BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Binding_Set(
    uint32_t object_instance, const char *str);
bool Network_Port_SC_Direct_Connect_Binding_Dirty_Set(
    uint32_t object_instance, const char *str);

BACNET_STACK_EXPORT
BACNET_SC_DIRECT_CONNECTION_STATUS *
Network_Port_SC_Direct_Connect_Connection_Status_Get(
    uint32_t object_instance, uint8_t index);
BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Connection_Status_Add(
    uint32_t object_instance,
    const char *uri,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_DATE_TIME *connect_ts,
    BACNET_DATE_TIME *disconnect_ts,
    BACNET_HOST_N_PORT_DATA *peer_address,
    uint8_t *peer_VMAC,
    uint8_t *peer_UUID,
    BACNET_ERROR_CODE error,
    const char *error_details);
BACNET_STACK_EXPORT
bool Network_Port_SC_Direct_Connect_Connection_Status_Delete_All(
    uint32_t object_instance);
BACNET_STACK_EXPORT
int Network_Port_SC_Direct_Connect_Connection_Status_Count(
    uint32_t object_instance);
BACNET_STACK_EXPORT
int Network_Port_SC_Direct_Connect_Connection_Status_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu);

BACNET_STACK_EXPORT
BACNET_SC_FAILED_CONNECTION_REQUEST *
Network_Port_SC_Failed_Connection_Requests_Get(
    uint32_t object_instance, uint8_t index);
BACNET_STACK_EXPORT
bool Network_Port_SC_Failed_Connection_Requests_Add(uint32_t object_instance,
    BACNET_DATE_TIME *ts,
    BACNET_HOST_N_PORT_DATA *peer_address,
    uint8_t *peer_VMAC,
    uint8_t *peer_UUID,
    BACNET_ERROR_CODE error,
    const char *error_details);
BACNET_STACK_EXPORT
bool Network_Port_SC_Failed_Connection_Requests_Delete_All(
    uint32_t object_instance);
BACNET_STACK_EXPORT
int Network_Port_SC_Failed_Connection_Requests_Count(uint32_t object_instance);
BACNET_STACK_EXPORT
int Network_Port_SC_Failed_Connection_Requests_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu);

BACNET_STACK_EXPORT
uint32_t Network_Port_Certificate_Key_File(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Certificate_Key_File_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
const BACNET_UUID *Network_Port_SC_Local_UUID(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Local_UUID_Set(
    uint32_t object_instance, BACNET_UUID *value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
