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

#ifndef SC_NETPORT_H
#define SC_NETPORT_H

#include <stdbool.h>
#include <stdint.h>
#include <bacnet/basic/sys/keylist.h>
#include <bacnet/bacnet_stack_exports.h>
#include <bacnet/bacdef.h>
#include <bacnet/bacenum.h>
#include <bacnet/apdu.h>
#include <bacnet/datetime.h>
#include <bacnet/hostnport.h>
#include <bacnet/datalink/bsc/bsc-conf.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Support 
   - SC_Hub_Function_Connection_Status
   - SC_Direct_Connect_Connection_Status
   - SC_FailedConnectionRequests
*/
//#define BACNET_SC_STATUS_SUPPORT

#ifdef BACNET_SECURE_CONNECT

    typedef struct BACnetUUID_T_{
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

    #define BACNET_SC_HUB_URI_MAX   2
    #define BACNET_SC_DIRECT_ACCEPT_URI_MAX 2

    #define BACNET_ISSUER_CERT_FILE_MAX 2
    #define BACNET_ERROR_STRING_LENGHT  64
    #define BACNET_URI_LENGHT  64
    #define BACNET_PEER_VMAC_LENGHT  6
    #define BACNET_BINDING_STRING_LENGHT  80
  
    typedef struct BACnetHostNPort_data_T {
        uint8_t type;
        char host[BACNET_URI_LENGHT];
        uint16_t port;
    } BACNET_HOST_N_PORT_DATA;

#if BSC_CONF_HUB_FUNCTIONS_NUM==1
    typedef struct BACnetSCHubConnection_T {
        BACNET_SC_CONNECTION_STATE Connection_State; //index = 0
        BACNET_DATE_TIME Connect_Timestamp; //index = 1
        BACNET_DATE_TIME Disconnect_Timestamp; //index = 2
        // optionals
        BACNET_ERROR_CODE Error; // index = 3
        char Error_Details[BACNET_ERROR_STRING_LENGHT]; // index = 4
    } BACNET_SC_HUB_CONNECTION;

    typedef struct BACnetSCHubFunctionConnection_T {
        BACNET_SC_CONNECTION_STATE State;
        BACNET_DATE_TIME Connect_Timestamp;
        BACNET_DATE_TIME Disconnect_Timestamp;
        BACNET_HOST_N_PORT_DATA Peer_Address;
        uint8_t Peer_VMAC[BACNET_PEER_VMAC_LENGHT];
        BACNET_UUID Peer_UUID;
        BACNET_ERROR_CODE Error;
        char Error_Details[BACNET_ERROR_STRING_LENGHT];
    } BACNET_SC_HUB_FUNCTION_CONNECTION;
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM==1 */

    typedef struct BACnetSCFailedConnectionRequest_T {
        BACNET_DATE_TIME Timestamp;
        BACNET_HOST_N_PORT_DATA Peer_Address;
        uint8_t Peer_VMAC[BACNET_PEER_VMAC_LENGHT];
        BACNET_UUID Peer_UUID;
        BACNET_ERROR_CODE Error;
        char Error_Details[BACNET_ERROR_STRING_LENGHT];
    } BACNET_SC_FAILED_CONNECTION_REQUEST;

#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE
    typedef struct BACnetRouterEntry_T {
        uint16_t Network_Number;
        uint8_t Mac_Address[6];
        enum {
            Available = 0,
            Busy = 1,
            Disconnected = 2
        } Status;
        uint8_t Performance_Index;
    } BACNET_ROUTER_ENTRY;
#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */

#if BSC_CONF_HUB_CONNECTORS_NUM==1
    typedef struct BACnetSCDirectConnection_T {
        char URI[BACNET_URI_LENGHT];
        BACNET_SC_CONNECTION_STATE Connection_State;
        BACNET_DATE_TIME Connect_Timestamp;
        BACNET_DATE_TIME Disconnect_Timestamp;
        BACNET_HOST_N_PORT_DATA Peer_Address;
        uint8_t Peer_VMAC[BACNET_PEER_VMAC_LENGHT];
        BACNET_UUID Peer_UUID;
        BACNET_ERROR_CODE Error;
        char Error_Details[BACNET_ERROR_STRING_LENGHT];
    } BACNET_SC_DIRECT_CONNECTION;
#endif /* BSC_CONF_HUB_CONNECTORS_NUM==1 */

    typedef struct BACnetSCAttributes_T {
        BACNET_UNSIGNED_INTEGER Max_BVLC_Length_Accepted;
        BACNET_UNSIGNED_INTEGER Max_NPDU_Length_Accepted;
        char SC_Primary_Hub_URI[BACNET_URI_LENGHT];
        char SC_Primary_Hub_URI_dirty[BACNET_URI_LENGHT];
        char SC_Failover_Hub_URI[BACNET_URI_LENGHT];
        char SC_Failover_Hub_URI_dirty[BACNET_URI_LENGHT];
        BACNET_UNSIGNED_INTEGER SC_Minimum_Reconnect_Time;
        BACNET_UNSIGNED_INTEGER SC_Maximum_Reconnect_Time;
        BACNET_UNSIGNED_INTEGER SC_Connect_Wait_Timeout;
        BACNET_UNSIGNED_INTEGER SC_Disconnect_Wait_Timeout;
        BACNET_UNSIGNED_INTEGER SC_Heartbeat_Timeout;
        BACNET_SC_HUB_CONNECTOR_STATE SC_Hub_Connector_State;
        uint32_t Operational_Certificate_File;
        uint32_t Issuer_Certificate_Files[BACNET_ISSUER_CERT_FILE_MAX];
        uint32_t Certificate_Signing_Request_File;
        /* Optional params */
#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE
        OS_Keylist Routing_Table;
#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */
#if BSC_CONF_HUB_FUNCTIONS_NUM==1
        BACNET_SC_HUB_CONNECTION SC_Primary_Hub_Connection_Status;
        BACNET_SC_HUB_CONNECTION SC_Failover_Hub_Connection_Status;
        bool SC_Hub_Function_Enable;
        bool SC_Hub_Function_Enable_dirty;
        char SC_Hub_Function_Accept_URIs[BACNET_SC_HUB_URI_MAX]
                                            [BACNET_URI_LENGHT];
        char SC_Hub_Function_Binding[BACNET_BINDING_STRING_LENGHT];
        char SC_Hub_Function_Binding_dirty[BACNET_BINDING_STRING_LENGHT];
#ifdef BACNET_SC_STATUS_SUPPORT
        BACNET_SC_HUB_FUNCTION_CONNECTION SC_Hub_Function_Connection_Status;
#endif
        uint16_t Hub_Server_Port;
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM==1 */
#if BSC_CONF_HUB_CONNECTORS_NUM==1
        bool SC_Direct_Connect_Initiate_Enable;
        bool SC_Direct_Connect_Initiate_Enable_dirty;
        bool SC_Direct_Connect_Accept_Enable;
        bool SC_Direct_Connect_Accept_Enable_dirty;
        char SC_Direct_Connect_Accept_URIs[BACNET_SC_DIRECT_ACCEPT_URI_MAX]
                                            [BACNET_URI_LENGHT];
        char SC_Direct_Connect_Binding[BACNET_BINDING_STRING_LENGHT];
        char SC_Direct_Connect_Binding_dirty[BACNET_BINDING_STRING_LENGHT];
#ifdef BACNET_SC_STATUS_SUPPORT
        BACNET_SC_DIRECT_CONNECTION SC_Direct_Connect_Connection_Status;
#endif
        uint16_t Direct_Server_Port;
#endif /* BSC_CONF_HUB_CONNECTORS_NUM==1 */
#ifdef BACNET_SC_STATUS_SUPPORT
        OS_Keylist SC_Failed_Connection_Requests;
#endif
        uint32_t Certificate_Key_File;
        BACNET_UUID Local_UUID;
    } BACNET_SC_PARAMS;

    //
    // getter / setter
    //

    BACNET_STACK_EXPORT
    BACNET_UNSIGNED_INTEGER Network_Port_Max_BVLC_Length_Accepted(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_Max_BVLC_Length_Accepted_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_STACK_EXPORT
    BACNET_UNSIGNED_INTEGER Network_Port_Max_NPDU_Length_Accepted(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_Max_NPDU_Length_Accepted_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_STACK_EXPORT
    bool Network_Port_SC_Primary_Hub_URI(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING *str);
    BACNET_STACK_EXPORT
    const char *Network_Port_SC_Primary_Hub_URI_char(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Primary_Hub_URI_Set(
        uint32_t object_instance,
        char *str);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Primary_Hub_URI_Dirty_Set(
        uint32_t object_instance,
        char *str);

    BACNET_STACK_EXPORT
    bool Network_Port_SC_Failover_Hub_URI(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING *str);
    BACNET_STACK_EXPORT
    const char *Network_Port_SC_Failover_Hub_URI_char(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Failover_Hub_URI_Set(
        uint32_t object_instance,
        char *str);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Failover_Hub_URI_Dirty_Set(
        uint32_t object_instance,
        char *str);

    BACNET_STACK_EXPORT
    BACNET_UNSIGNED_INTEGER Network_Port_SC_Minimum_Reconnect_Time(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Minimum_Reconnect_Time_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_STACK_EXPORT
    BACNET_UNSIGNED_INTEGER Network_Port_SC_Maximum_Reconnect_Time(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Maximum_Reconnect_Time_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_STACK_EXPORT
    BACNET_UNSIGNED_INTEGER Network_Port_SC_Connect_Wait_Timeout(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Connect_Wait_Timeout_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_STACK_EXPORT
    BACNET_UNSIGNED_INTEGER Network_Port_SC_Disconnect_Wait_Timeout(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Disconnect_Wait_Timeout_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_STACK_EXPORT
    BACNET_UNSIGNED_INTEGER Network_Port_SC_Heartbeat_Timeout(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Heartbeat_Timeout_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_STACK_EXPORT
    BACNET_SC_HUB_CONNECTOR_STATE Network_Port_SC_Hub_Connector_State(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Connector_State_Set(
        uint32_t object_instance,
        BACNET_SC_HUB_CONNECTOR_STATE value);

    BACNET_STACK_EXPORT
    uint32_t Network_Port_Operational_Certificate_File(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_Operational_Certificate_File_Set(
        uint32_t object_instance,
        uint32_t value);

    BACNET_STACK_EXPORT
    uint32_t Network_Port_Issuer_Certificate_File(
        uint32_t object_instance,
        uint8_t index);
    BACNET_STACK_EXPORT
    bool Network_Port_Issuer_Certificate_File_Set(
        uint32_t object_instance,
        uint8_t index,
        uint32_t value);

    BACNET_STACK_EXPORT
    uint32_t Network_Port_Certificate_Signing_Request_File(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_Certificate_Signing_Request_File_Set(
        uint32_t object_instance,
        uint32_t value);

#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE
    BACNET_STACK_EXPORT
    BACNET_ROUTER_ENTRY *Network_Port_Routing_Table_Find(
        uint32_t object_instance,
        uint16_t Network_Number);
    BACNET_STACK_EXPORT
    BACNET_ROUTER_ENTRY *Network_Port_Routing_Table_Get(
        uint32_t object_instance,
        uint8_t index);
    BACNET_STACK_EXPORT
    bool Network_Port_Routing_Table_Add(
        uint32_t object_instance,
        uint16_t network_number,
        uint8_t *mac,
        uint8_t mac_len,
        uint8_t status,
        uint8_t performance_index);
    BACNET_STACK_EXPORT
    bool Network_Port_Routing_Table_Delete(
        uint32_t object_instance,
        uint16_t Network_Number);
    BACNET_STACK_EXPORT
    bool Network_Port_Routing_Table_Delete_All(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    uint8_t Network_Port_Routing_Table_Count(
        uint32_t object_instance);
#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */

#if BSC_CONF_HUB_FUNCTIONS_NUM==1
    BACNET_STACK_EXPORT
    BACNET_SC_HUB_CONNECTION *Network_Port_SC_Primary_Hub_Connection_Status(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Primary_Hub_Connection_Status_Set(
        uint32_t object_instance,
        BACNET_SC_CONNECTION_STATE state,
        BACNET_DATE_TIME *connect_ts,
        BACNET_DATE_TIME *disconnect_ts,
        BACNET_ERROR_CODE error,
        char *error_details);

    BACNET_STACK_EXPORT
    BACNET_SC_HUB_CONNECTION *Network_Port_SC_Failover_Hub_Connection_Status(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Failover_Hub_Connection_Status_Set(
        uint32_t object_instance,
        BACNET_SC_CONNECTION_STATE state,
        BACNET_DATE_TIME *connect_ts,
        BACNET_DATE_TIME *disconnect_ts,
        BACNET_ERROR_CODE error,
        char *error_details);
 
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Function_Enable(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Function_Enable_Set(
        uint32_t object_instance,
        bool value);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Function_Enable_Dirty_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Function_Accept_URI(
        uint32_t object_instance,
        uint8_t index,
        BACNET_CHARACTER_STRING *str);
    BACNET_STACK_EXPORT
    const char *Network_Port_SC_Hub_Function_Accept_URI_char(
        uint32_t object_instance,
        uint8_t index);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Function_Accept_URI_Set(
        uint32_t object_instance,
        uint8_t index,
        char *str);

    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Function_Binding(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING *str);
    BACNET_STACK_EXPORT
    const char *Network_Port_SC_Hub_Function_Binding_char(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Function_Binding_Set(
        uint32_t object_instance,
        char *str);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Function_Binding_Dirty_Set(
        uint32_t object_instance,
        char *str);

#ifdef BACNET_SC_STATUS_SUPPORT

    BACNET_STACK_EXPORT
    BACNET_SC_HUB_FUNCTION_CONNECTION *
    Network_Port_SC_Hub_Function_Connection_Status(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Function_Connection_Status_Set(
        uint32_t object_instance,
        BACNET_SC_CONNECTION_STATE state,
        BACNET_DATE_TIME *connect_ts,
        BACNET_DATE_TIME *disconnect_ts,
        BACNET_HOST_N_PORT *peer_address,
        uint8_t *peer_VMAC,
        uint8_t *peer_UUID,
        BACNET_ERROR_CODE error,
        char *error_details);
#endif

    BACNET_STACK_EXPORT
    uint16_t Network_Port_SC_Hub_Server_Port(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Hub_Server_Port_Set(
        uint32_t object_instance,
        uint16_t value);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM==1 */

#if BSC_CONF_HUB_CONNECTORS_NUM==1
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Initiate_Enable(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Initiate_Enable_Set(
        uint32_t object_instance,
        bool value);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Initiate_Enable_Dirty_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Accept_Enable(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Accept_Enable_Set(
        uint32_t object_instance,
        bool value);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Accept_Enable_Dirty_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Accept_URI(
        uint32_t object_instance,
        uint8_t index,
        BACNET_CHARACTER_STRING *str);
    BACNET_STACK_EXPORT
    const char *Network_Port_SC_Direct_Connect_Accept_URI_char(
        uint32_t object_instance,
        uint8_t index);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Accept_URI_Set(
        uint32_t object_instance,
        uint8_t index,
        char *str);

    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Binding(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING *str);
    BACNET_STACK_EXPORT
    const char *Network_Port_SC_Direct_Connect_Binding_char(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Binding_Set(
        uint32_t object_instance,
        char *str);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Binding_Dirty_Set(
        uint32_t object_instance,
        char *str);

#ifdef BACNET_SC_STATUS_SUPPORT

    BACNET_STACK_EXPORT
    BACNET_SC_DIRECT_CONNECTION *
    Network_Port_SC_Direct_Connect_Connection_Status(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Connect_Connection_Status_Set(
        uint32_t object_instance,
        char *uri,
        BACNET_SC_CONNECTION_STATE state,
        BACNET_DATE_TIME *connect_ts,
        BACNET_DATE_TIME *disconnect_ts,
        BACNET_HOST_N_PORT *peer_address,
        uint8_t *peer_VMAC,
        uint8_t *peer_UUID,
        BACNET_ERROR_CODE error,
        char *error_details);
#endif

    BACNET_STACK_EXPORT
    uint16_t Network_Port_SC_Direct_Server_Port(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Direct_Server_Port_Set(
        uint32_t object_instance,
        uint16_t value);

#endif /* BSC_CONF_HUB_CONNECTORS_NUM==1 */

#ifdef BACNET_SC_STATUS_SUPPORT

    BACNET_STACK_EXPORT
    BACNET_SC_FAILED_CONNECTION_REQUEST *
    Network_Port_SC_Failed_Connection_Requests_Get(
        uint32_t object_instance,
        uint8_t index);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Failed_Connection_Requests_Add(
        uint32_t object_instance,
        BACNET_DATE_TIME *ts,
        BACNET_HOST_N_PORT *peer_address,
        uint8_t *peer_VMAC,
        uint8_t *peer_UUID,
        BACNET_ERROR_CODE error,
        char *error_details);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Failed_Connection_Requests_Delete_By_Index(
        uint32_t object_instance,
        uint8_t index);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Failed_Connection_Requests_Delete_All(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Failed_Connection_Requests_Count(
        uint32_t object_instance);

#endif

    BACNET_STACK_EXPORT
    uint32_t Network_Port_Certificate_Key_File(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_Certificate_Key_File_Set(
        uint32_t object_instance,
        uint32_t value);

    BACNET_STACK_EXPORT
    const BACNET_UUID *Network_Port_SC_Local_UUID(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Network_Port_SC_Local_UUID_Set(
        uint32_t object_instance,
        BACNET_UUID *value);

    //
    // Encode / decode
    //

#if BSC_CONF_HUB_FUNCTIONS_NUM==1
    BACNET_STACK_EXPORT
    int bacapp_encode_SCHubConnection(
        uint8_t *apdu,
        BACNET_SC_HUB_CONNECTION *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_SCHubConnection(
        uint8_t *apdu,
        BACNET_SC_HUB_CONNECTION *value);
    BACNET_STACK_EXPORT
    int bacapp_encode_context_SCHubConnection(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_SC_HUB_CONNECTION *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_context_SCHubConnection(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_SC_HUB_CONNECTION *value);

    BACNET_STACK_EXPORT
    int bacapp_encode_SCHubFunctionConnection(
        uint8_t *apdu,
        BACNET_SC_HUB_FUNCTION_CONNECTION *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_SCHubFunctionConnection(
        uint8_t *apdu,
        BACNET_SC_HUB_FUNCTION_CONNECTION *value);
    BACNET_STACK_EXPORT
    int bacapp_encode_context_SCHubFunctionConnection(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_SC_HUB_FUNCTION_CONNECTION *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_context_SCHubFunctionConnection(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_SC_HUB_FUNCTION_CONNECTION *value);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM==1 */

    BACNET_STACK_EXPORT
    int bacapp_encode_SCFailedConnectionRequest(
        uint8_t *apdu,
        BACNET_SC_FAILED_CONNECTION_REQUEST *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_SCFailedConnectionRequest(
        uint8_t *apdu,
        BACNET_SC_FAILED_CONNECTION_REQUEST *value);
    BACNET_STACK_EXPORT
    int bacapp_encode_context_SCFailedConnectionRequest(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_SC_FAILED_CONNECTION_REQUEST *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_context_SCFailedConnectionRequest(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_SC_FAILED_CONNECTION_REQUEST *value);

#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE
    BACNET_STACK_EXPORT
    int bacapp_encode_RouterEntry(
        uint8_t *apdu,
        BACNET_ROUTER_ENTRY *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_RouterEntry(
        uint8_t *apdu,
        BACNET_ROUTER_ENTRY *value);
    BACNET_STACK_EXPORT
    int bacapp_encode_context_RouterEntry(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_ROUTER_ENTRY *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_context_RouterEntry(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_ROUTER_ENTRY *value);
#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */

#if BSC_CONF_HUB_CONNECTORS_NUM==1
    BACNET_STACK_EXPORT
    int bacapp_encode_SCDirectConnection(
        uint8_t *apdu,
        BACNET_SC_DIRECT_CONNECTION *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_SCDirectConnection(
        uint8_t *apdu,
        BACNET_SC_DIRECT_CONNECTION *value);
    BACNET_STACK_EXPORT
    int bacapp_encode_context_SCDirectConnection(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_SC_DIRECT_CONNECTION *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_context_SCDirectConnection(
        uint8_t *apdu,
        uint8_t tag_number,
        BACNET_SC_DIRECT_CONNECTION *value);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM==1 */

    void Network_Port_SC_Pending_Params_Apply(
        uint32_t object_instance);
    void Network_Port_SC_Pending_Params_Discard(
        uint32_t object_instance);

#endif /* BACNET_SECURE_CONNECT */


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
