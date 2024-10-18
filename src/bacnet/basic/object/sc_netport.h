/**
 * @file
 * @author Mikhail Antropov
 * @date 2022
 * @brief Helper Network port objects to implement secure connect attributes
 * @copyright SPDX-License-Identifier: MIT
 */

#ifndef SC_NETPORT_H
#define SC_NETPORT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"
#include "bacnet/datetime.h"
#include "bacnet/hostnport.h"
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

#define BACNET_SC_HUB_URI_MAX 2
#ifndef BACNET_SC_DIRECT_ACCEPT_URI_MAX
#define BACNET_SC_DIRECT_ACCEPT_URI_MAX 2
#endif

#define BACNET_ISSUER_CERT_FILE_MAX 2
#define BACNET_ERROR_STRING_LENGTH BSC_CONF_WEBSOCKET_ERR_DESC_STR_MAX_LEN
#define BACNET_URI_LENGTH BSC_CONF_WSURL_MAX_LEN
#define BACNET_PEER_VMAC_LENGTH BVLC_SC_VMAC_SIZE
#define BACNET_BINDING_STRING_LENGTH 80

#define BACNET_SC_BINDING_SEPARATOR ':'

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
    /* optionals */
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

typedef struct BACnetRouterEntry_T {
    uint16_t Network_Number;
    uint8_t Mac_Address[6];
    enum { Available = 0, Busy = 1, Disconnected = 2 } Status;
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
/* getter / setter */
/* */

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER
Network_Port_Max_BVLC_Length_Accepted(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Max_BVLC_Length_Accepted_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_Max_BVLC_Length_Accepted_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER
Network_Port_Max_NPDU_Length_Accepted(uint32_t object_instance);
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
BACNET_UNSIGNED_INTEGER
Network_Port_SC_Minimum_Reconnect_Time(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Minimum_Reconnect_Time_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Minimum_Reconnect_Time_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER
Network_Port_SC_Maximum_Reconnect_Time(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Maximum_Reconnect_Time_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Maximum_Reconnect_Time_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER
Network_Port_SC_Connect_Wait_Timeout(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Connect_Wait_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Connect_Wait_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER
Network_Port_SC_Disconnect_Wait_Timeout(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Disconnect_Wait_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Disconnect_Wait_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER
Network_Port_SC_Heartbeat_Timeout(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Heartbeat_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);
bool Network_Port_SC_Heartbeat_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
BACNET_SC_HUB_CONNECTOR_STATE
Network_Port_SC_Hub_Connector_State(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Hub_Connector_State_Set(
    uint32_t object_instance, BACNET_SC_HUB_CONNECTOR_STATE value);

BACNET_STACK_EXPORT
uint32_t Network_Port_Operational_Certificate_File(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Operational_Certificate_File_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint32_t
Network_Port_Issuer_Certificate_File(uint32_t object_instance, uint8_t index);
BACNET_STACK_EXPORT
bool Network_Port_Issuer_Certificate_File_Set(
    uint32_t object_instance, uint8_t index, uint32_t value);

BACNET_STACK_EXPORT
uint32_t
Network_Port_Certificate_Signing_Request_File(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Certificate_Signing_Request_File_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
BACNET_ROUTER_ENTRY *Network_Port_Routing_Table_Find(
    uint32_t object_instance, uint16_t Network_Number);
BACNET_STACK_EXPORT
BACNET_ROUTER_ENTRY *
Network_Port_Routing_Table_Get(uint32_t object_instance, uint8_t index);
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
    uint32_t object_instance, uint16_t Network_Number);
BACNET_STACK_EXPORT
bool Network_Port_Routing_Table_Delete_All(uint32_t object_instance);
BACNET_STACK_EXPORT
uint8_t Network_Port_Routing_Table_Count(uint32_t object_instance);

BACNET_STACK_EXPORT
BACNET_SC_HUB_CONNECTION_STATUS *
Network_Port_SC_Primary_Hub_Connection_Status(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Primary_Hub_Connection_Status_Set(
    uint32_t object_instance,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_DATE_TIME *connect_ts,
    BACNET_DATE_TIME *disconnect_ts,
    BACNET_ERROR_CODE error,
    const char *error_details);

BACNET_STACK_EXPORT
BACNET_SC_HUB_CONNECTION_STATUS *
Network_Port_SC_Failover_Hub_Connection_Status(uint32_t object_instance);
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
bool Network_Port_SC_Hub_Function_Accept_URI_Dirty_Set(
    uint32_t object_instance, uint8_t index, const char *str);
const char *
Network_Port_SC_Hub_Function_Accept_URIs_char(uint32_t object_instance);
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
BACNET_SC_FAILED_CONNECTION_REQUEST *
Network_Port_SC_Failed_Connection_Requests_Get(
    uint32_t object_instance, uint8_t index);
BACNET_STACK_EXPORT
bool Network_Port_SC_Failed_Connection_Requests_Add(
    uint32_t object_instance,
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
uint32_t Network_Port_Certificate_Key_File(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Certificate_Key_File_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
const BACNET_UUID *Network_Port_SC_Local_UUID(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_SC_Local_UUID_Set(
    uint32_t object_instance, BACNET_UUID *value);

/* */
/* Encode / decode */
/* */

BACNET_STACK_EXPORT
int bacapp_encode_SCHubConnection(
    uint8_t *apdu, BACNET_SC_HUB_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_SCHubConnection(
    uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_HUB_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_encode_context_SCHubConnection(
    uint8_t *apdu, uint8_t tag_number, BACNET_SC_HUB_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_context_SCHubConnection(
    uint8_t *apdu, uint8_t tag_number, BACNET_SC_HUB_CONNECTION_STATUS *value);

BACNET_STACK_EXPORT
int bacapp_encode_SCHubFunctionConnection(
    uint8_t *apdu, BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_SCHubFunctionConnection(
    uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_encode_context_SCHubFunctionConnection(
    uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_context_SCHubFunctionConnection(
    uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value);

BACNET_STACK_EXPORT
int bacapp_encode_SCFailedConnectionRequest(
    uint8_t *apdu, BACNET_SC_FAILED_CONNECTION_REQUEST *value);
BACNET_STACK_EXPORT
int bacapp_decode_SCFailedConnectionRequest(
    uint8_t *apdu,
    uint16_t max_apdu_len,
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

BACNET_STACK_EXPORT
int bacapp_encode_RouterEntry(uint8_t *apdu, BACNET_ROUTER_ENTRY *value);
BACNET_STACK_EXPORT
int bacapp_decode_RouterEntry(uint8_t *apdu, BACNET_ROUTER_ENTRY *value);
BACNET_STACK_EXPORT
int bacapp_encode_context_RouterEntry(
    uint8_t *apdu, uint8_t tag_number, BACNET_ROUTER_ENTRY *value);
BACNET_STACK_EXPORT
int bacapp_decode_context_RouterEntry(
    uint8_t *apdu, uint8_t tag_number, BACNET_ROUTER_ENTRY *value);

BACNET_STACK_EXPORT
int bacapp_encode_SCDirectConnection(
    uint8_t *apdu, BACNET_SC_DIRECT_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_SCDirectConnection(
    uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_encode_context_SCDirectConnection(
    uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value);
BACNET_STACK_EXPORT
int bacapp_decode_context_SCDirectConnection(
    uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value);

void Network_Port_SC_Pending_Params_Apply(uint32_t object_instance);
void Network_Port_SC_Pending_Params_Discard(uint32_t object_instance);

int Network_Port_SC_snprintf_value(
    char *str,
    size_t str_len,
    const BACNET_OBJECT_PROPERTY_VALUE *object_value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
