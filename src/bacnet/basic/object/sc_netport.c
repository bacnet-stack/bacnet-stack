/**
 * @file
 * @brief Helper Network port objects for implement secure connect attributes
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @date June 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <bacnet/config.h>
#include <bacnet/basic/binding/address.h>
#include <bacnet/bacdef.h>
#include <bacnet/bacapp.h>
#include <bacnet/bacint.h>
#include <bacnet/bacdcode.h>
#include <bacnet/npdu.h>
#include <bacnet/apdu.h>
#include <bacnet/datalink/datalink.h>
#include <bacnet/timestamp.h>
/* me */
#include <bacnet/basic/object/bacfile.h>
#include <bacnet/basic/object/netport.h>
#include <bacnet/basic/object/sc_netport.h>
#include <bacnet/basic/object/netport_internal.h>
#include <bacnet/datalink/bsc/bsc-util.h>

#define SC_MIN_RECONNECT_MIN 2
#define SC_MIN_RECONNECT_MAX 300

#define SC_MAX_RECONNECT_MIN 2
#define SC_MAX_RECONNECT_MAX 600

#define SC_WAIT_CONNECT_MIN 5
#define SC_WAIT_CONNECT_MAX 300

static int
string_splite(const char *str, char separator, int *indexes, int length);
static bool
string_subsstr(char *str, int length, int index, const char *substr);

static void sc_binding_parse(char *str, uint16_t *port, char **ifname)
{
    char *p = NULL;

    *port = 0;
    *ifname = NULL;

    if (str != NULL) {
        for (p = str; (*p != 0) && (*p != BACNET_SC_BINDING_SEPARATOR); ++p)
            ;

        if (p != str) {
            if (*p == BACNET_SC_BINDING_SEPARATOR) {
                *p = 0;
                *port = (uint16_t)strtol(p + 1, NULL, 0);
                *ifname = (char *)str;
            } else {
                *port = (uint16_t)strtol(str, NULL, 0);
                *ifname = NULL;
            }
        }
    }
}

BACNET_UNSIGNED_INTEGER
Network_Port_Max_BVLC_Length_Accepted(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER max_length = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        max_length = params->Max_BVLC_Length_Accepted;
    }

    return max_length;
}

bool Network_Port_Max_BVLC_Length_Accepted_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->Max_BVLC_Length_Accepted = value;

    return true;
}

bool Network_Port_Max_BVLC_Length_Accepted_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->Max_BVLC_Length_Accepted_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER
Network_Port_Max_NPDU_Length_Accepted(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER max_length = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        max_length = params->Max_NPDU_Length_Accepted;
    }

    return max_length;
}

bool Network_Port_Max_NPDU_Length_Accepted_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->Max_NPDU_Length_Accepted = value;

    return true;
}

bool Network_Port_Max_NPDU_Length_Accepted_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->Max_NPDU_Length_Accepted_dirty = value;

    return true;
}

bool Network_Port_SC_Primary_Hub_URI(
    uint32_t object_instance, BACNET_CHARACTER_STRING *str)
{
    bool status = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        status = characterstring_init_ansi(str, params->SC_Primary_Hub_URI);
    }

    return status;
}

const char *Network_Port_SC_Primary_Hub_URI_char(uint32_t object_instance)
{
    char *p = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params && params->SC_Primary_Hub_URI[0]) {
        p = params->SC_Primary_Hub_URI;
    }

    return p;
}

bool Network_Port_SC_Primary_Hub_URI_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if (str) {
        snprintf(
            params->SC_Primary_Hub_URI, sizeof(params->SC_Primary_Hub_URI),
            "%s", str);
    } else {
        params->SC_Primary_Hub_URI[0] = 0;
    }

    return true;
}

bool Network_Port_SC_Primary_Hub_URI_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if (str) {
        snprintf(
            params->SC_Primary_Hub_URI_dirty,
            sizeof(params->SC_Primary_Hub_URI_dirty), "%s", str);
    } else {
        params->SC_Primary_Hub_URI_dirty[0] = 0;
    }

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

bool Network_Port_SC_Failover_Hub_URI(
    uint32_t object_instance, BACNET_CHARACTER_STRING *str)
{
    bool status = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        status = characterstring_init_ansi(str, params->SC_Failover_Hub_URI);
    }

    return status;
}

const char *Network_Port_SC_Failover_Hub_URI_char(uint32_t object_instance)
{
    char *p = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params && params->SC_Failover_Hub_URI[0]) {
        p = params->SC_Failover_Hub_URI;
    }

    return p;
}

bool Network_Port_SC_Failover_Hub_URI_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if (str) {
        snprintf(
            params->SC_Failover_Hub_URI, sizeof(params->SC_Failover_Hub_URI),
            "%s", str);
    } else {
        params->SC_Failover_Hub_URI[0] = 0;
    }

    return true;
}

bool Network_Port_SC_Failover_Hub_URI_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if (str) {
        snprintf(
            params->SC_Failover_Hub_URI_dirty,
            sizeof(params->SC_Failover_Hub_URI_dirty), "%s", str);
    } else {
        params->SC_Failover_Hub_URI_dirty[0] = 0;
    }

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

BACNET_UNSIGNED_INTEGER
Network_Port_SC_Minimum_Reconnect_Time(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        timeout = params->SC_Minimum_Reconnect_Time;
    }

    return timeout;
}

bool Network_Port_SC_Minimum_Reconnect_Time_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if ((value < SC_MIN_RECONNECT_MIN) || (value > SC_MIN_RECONNECT_MAX)) {
        return false;
    }

    params->SC_Minimum_Reconnect_Time = value;

    return true;
}

bool Network_Port_SC_Minimum_Reconnect_Time_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if ((value < SC_MIN_RECONNECT_MIN) || (value > SC_MIN_RECONNECT_MAX)) {
        return false;
    }

    params->SC_Minimum_Reconnect_Time_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER
Network_Port_SC_Maximum_Reconnect_Time(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        timeout = params->SC_Maximum_Reconnect_Time;
    }

    return timeout;
}

bool Network_Port_SC_Maximum_Reconnect_Time_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if ((value < SC_MAX_RECONNECT_MIN) || (value > SC_MAX_RECONNECT_MAX)) {
        return false;
    }

    params->SC_Maximum_Reconnect_Time = value;

    return true;
}

bool Network_Port_SC_Maximum_Reconnect_Time_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if ((value < SC_MAX_RECONNECT_MIN) || (value > SC_MAX_RECONNECT_MAX)) {
        return false;
    }

    params->SC_Maximum_Reconnect_Time_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER
Network_Port_SC_Connect_Wait_Timeout(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        timeout = params->SC_Connect_Wait_Timeout;
    }

    return timeout;
}

bool Network_Port_SC_Connect_Wait_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if ((value < SC_WAIT_CONNECT_MIN) || (value > SC_WAIT_CONNECT_MAX)) {
        return false;
    }

    params->SC_Connect_Wait_Timeout = value;

    return true;
}

bool Network_Port_SC_Connect_Wait_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if ((value < SC_WAIT_CONNECT_MIN) || (value > SC_WAIT_CONNECT_MAX)) {
        return false;
    }

    params->SC_Connect_Wait_Timeout_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER
Network_Port_SC_Disconnect_Wait_Timeout(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        timeout = params->SC_Disconnect_Wait_Timeout;
    }

    return timeout;
}

bool Network_Port_SC_Disconnect_Wait_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Disconnect_Wait_Timeout = value;

    return true;
}

bool Network_Port_SC_Disconnect_Wait_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Disconnect_Wait_Timeout_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER
Network_Port_SC_Heartbeat_Timeout(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        timeout = params->SC_Heartbeat_Timeout;
    }

    return timeout;
}

bool Network_Port_SC_Heartbeat_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Heartbeat_Timeout = value;

    return true;
}

bool Network_Port_SC_Heartbeat_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Heartbeat_Timeout_dirty = value;

    return true;
}

BACNET_SC_HUB_CONNECTOR_STATE
Network_Port_SC_Hub_Connector_State(uint32_t object_instance)
{
    BACNET_SC_HUB_CONNECTOR_STATE state =
        BACNET_SC_HUB_CONNECTOR_STATE_NO_HUB_CONNECTION;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        state = params->SC_Hub_Connector_State;
    }

    return state;
}

bool Network_Port_SC_Hub_Connector_State_Set(
    uint32_t object_instance, BACNET_SC_HUB_CONNECTOR_STATE value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Hub_Connector_State = value;

    return true;
}

uint32_t Network_Port_Operational_Certificate_File(uint32_t object_instance)
{
    uint32_t id = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        id = params->Operational_Certificate_File;
    }

    return id;
}

bool Network_Port_Operational_Certificate_File_Set(
    uint32_t object_instance, uint32_t value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->Operational_Certificate_File = value;

    return true;
}

uint32_t
Network_Port_Issuer_Certificate_File(uint32_t object_instance, uint8_t index)
{
    uint32_t id = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params && (index < BACNET_ISSUER_CERT_FILE_MAX)) {
        id = params->Issuer_Certificate_Files[index];
    }

    return id;
}

bool Network_Port_Issuer_Certificate_File_Set(
    uint32_t object_instance, uint8_t index, uint32_t value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params || (index >= BACNET_ISSUER_CERT_FILE_MAX)) {
        return false;
    }

    params->Issuer_Certificate_Files[index] = value;

    return true;
}

/**
 * @brief Encode one BACnetARRAY property element; a function template
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to n for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for invalid array index
 */
int Network_Port_Issuer_Certificate_File_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0;
    uint32_t file_instance = 0;

    if (index >= BACNET_ISSUER_CERT_FILE_MAX) {
        apdu_len = BACNET_STATUS_ERROR;
    } else {
        file_instance =
            Network_Port_Issuer_Certificate_File(object_instance, index);
        apdu_len = encode_application_unsigned(apdu, file_instance);
    }

    return apdu_len;
}

uint32_t Network_Port_Certificate_Signing_Request_File(uint32_t object_instance)
{
    uint32_t id = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        id = params->Certificate_Signing_Request_File;
    }

    return id;
}

bool Network_Port_Certificate_Signing_Request_File_Set(
    uint32_t object_instance, uint32_t value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->Certificate_Signing_Request_File = value;

    return true;
}

#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE

#if 0
/* This function will needed if the Network_Port_Routing_Table_Find()
    or Network_Port_Routing_Table_Get() will return individual fields,
     not the entire struct. */
static bool MAC_Address_Get(uint8_t network_type, uint8_t *mac,
    BACNET_OCTET_STRING *mac_address)
{
    bool status = false;
    uint8_t ip_mac[4 + 2] = { 0 };
    size_t mac_len = 0;

    switch (network_type) {
        case PORT_TYPE_ETHERNET:
            mac_len = 6;
            break;
        case PORT_TYPE_MSTP:
            mac_len = 1;
            break;
        case PORT_TYPE_BIP:
            memcpy(&ip_mac[0], mac, 4);
            /* convert port from host-byte-order to network-byte-order */
            encode_unsigned16(&ip_mac[4], *(uint16_t*)(mac + 4));
            mac = &ip_mac[0];
            mac_len = sizeof(ip_mac);
            break;
        case PORT_TYPE_BIP6:
            mac_len = 3;
            break;
        default:
            break;
    }
    if (mac) {
        status = octetstring_init(mac_address, mac, mac_len);
    }
    return status;
}
#endif

static bool MAC_Address_Set(
    uint8_t network_type, uint8_t *mac_dest, uint8_t *mac_src, uint8_t mac_len)
{
    bool status = false;
    size_t mac_size = 0;
    uint8_t ip_mac[4 + 2] = { 0 };

    switch (network_type) {
        case PORT_TYPE_ETHERNET:
            mac_size = 6;
            break;
        case PORT_TYPE_MSTP:
            mac_size = 1;
            break;
        case PORT_TYPE_BIP:
            memcpy(&ip_mac[0], mac_src, 4);
            /* convert port from host-byte-order to network-byte-order */
            decode_unsigned16(&mac_src[4], (uint16_t *)&ip_mac[4]);
            mac_src = &ip_mac[0];
            mac_len = sizeof(ip_mac);
            break;
        case PORT_TYPE_BIP6:
            mac_size = 3;
            break;
        default:
            break;
    }
    if (mac_src && mac_dest && (mac_len == mac_size)) {
        memcpy(mac_dest, mac_src, mac_size);
        status = true;
    }
    return status;
}

BACNET_ROUTER_ENTRY *Network_Port_Routing_Table_Find(
    uint32_t object_instance, uint16_t network_number)
{
    BACNET_ROUTER_ENTRY *entry = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return entry;
    }

    entry = Keylist_Data(params->Routing_Table, network_number);

    return entry;
}

BACNET_ROUTER_ENTRY *
Network_Port_Routing_Table_Get(uint32_t object_instance, uint8_t index)
{
    BACNET_ROUTER_ENTRY *entry = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return entry;
    }

    entry = Keylist_Data_Index(params->Routing_Table, index);

    return entry;
}

bool Network_Port_Routing_Table_Add(
    uint32_t object_instance,
    uint16_t network_number,
    uint8_t *mac,
    uint8_t mac_len,
    uint8_t status,
    uint8_t performance_index)
{
    int index;
    uint8_t network_type;
    BACNET_ROUTER_ENTRY *entry;
    BACNET_SC_PARAMS *params;

    params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }
    network_type = Network_Port_Type(object_instance);
    entry = calloc(1, sizeof(BACNET_ROUTER_ENTRY));
    if (!entry) {
        return false;
    }
    entry->Network_Number = network_number;
    MAC_Address_Set(network_type, entry->Mac_Address, mac, mac_len);
    entry->Status = status;
    entry->Performance_Index = performance_index;
    index = Keylist_Data_Add(params->Routing_Table, network_number, entry);
    if (index < 0) {
        free(entry);
        return false;
    }

    return true;
}

bool Network_Port_Routing_Table_Delete(
    uint32_t object_instance, uint16_t network_number)
{
    BACNET_ROUTER_ENTRY *entry;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    entry = Keylist_Data_Delete(params->Routing_Table, network_number);
    free(entry);

    return true;
}

bool Network_Port_Routing_Table_Delete_All(uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    Keylist_Delete(params->Routing_Table);
    params->Routing_Table = Keylist_Create();

    return true;
}

uint8_t Network_Port_Routing_Table_Count(uint32_t object_instance)
{
    uint8_t count = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return 0;
    }

    count = Keylist_Count(params->Routing_Table);

    return count;
}

/**
 * @brief Handle a request to encode all the the routing table entries
 * @param apdu [out] Buffer in which the APDU contents are built.
 * @param max_apdu [in] Max length of the APDU buffer.
 *
 * @return How many bytes were encoded in the buffer, or
 *   BACNET_STATUS_ABORT if the response would not fit within the buffer.
 */
int Network_Port_Routing_Table_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu)
{
    BACNET_ROUTER_ENTRY *entry = NULL;
    int apdu_len = 0;
    unsigned index = 0;
    unsigned size = 0;

    size = Network_Port_Routing_Table_Count(object_instance);
    for (index = 0; index < size; index++) {
        entry = Network_Port_Routing_Table_Get(object_instance, index);
        apdu_len += bacapp_encode_RouterEntry(NULL, entry);
    }
    if (apdu_len > max_apdu) {
        return BACNET_STATUS_ABORT;
    }
    apdu_len = 0;
    for (index = 0; index < size; index++) {
        entry = Network_Port_Routing_Table_Get(object_instance, index);
        apdu_len += bacapp_encode_RouterEntry(&apdu[apdu_len], entry);
    }

    return apdu_len;
}

#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0

BACNET_SC_HUB_CONNECTION_STATUS *
Network_Port_SC_Primary_Hub_Connection_Status(uint32_t object_instance)
{
    BACNET_SC_HUB_CONNECTION_STATUS *connection = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        connection = &params->SC_Primary_Hub_Connection_Status;
    }

    return connection;
}

bool Network_Port_SC_Primary_Hub_Connection_Status_Set(
    uint32_t object_instance,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_DATE_TIME *connect_ts,
    BACNET_DATE_TIME *disconnect_ts,
    BACNET_ERROR_CODE error,
    const char *error_details)
{
    BACNET_SC_HUB_CONNECTION_STATUS *st;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    st = &params->SC_Primary_Hub_Connection_Status;
    st->State = state;
    st->Connect_Timestamp = *connect_ts;
    st->Disconnect_Timestamp = *disconnect_ts;
    st->Error = error;
    if (error_details) {
        bsc_copy_str(
            st->Error_Details, error_details, sizeof(st->Error_Details));
    } else {
        st->Error_Details[0] = 0;
    }

    return true;
}

BACNET_SC_HUB_CONNECTION_STATUS *
Network_Port_SC_Failover_Hub_Connection_Status(uint32_t object_instance)
{
    BACNET_SC_HUB_CONNECTION_STATUS *connection = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        connection = &params->SC_Failover_Hub_Connection_Status;
    }

    return connection;
}

bool Network_Port_SC_Failover_Hub_Connection_Status_Set(
    uint32_t object_instance,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_DATE_TIME *connect_ts,
    BACNET_DATE_TIME *disconnect_ts,
    BACNET_ERROR_CODE error,
    const char *error_details)
{
    BACNET_SC_HUB_CONNECTION_STATUS *st;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    st = &params->SC_Failover_Hub_Connection_Status;
    st->State = state;
    st->Connect_Timestamp = *connect_ts;
    st->Disconnect_Timestamp = *disconnect_ts;
    st->Error = error;
    if (error_details) {
        bsc_copy_str(
            st->Error_Details, error_details, sizeof(st->Error_Details));
    } else {
        st->Error_Details[0] = 0;
    }

    return true;
}

bool Network_Port_SC_Hub_Function_Enable(uint32_t object_instance)
{
    bool enable = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        enable = params->SC_Hub_Function_Enable;
    }

    return enable;
}

bool Network_Port_SC_Hub_Function_Enable_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Hub_Function_Enable = value;

    return true;
}

bool Network_Port_SC_Hub_Function_Enable_Dirty_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Hub_Function_Enable_dirty = value;

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

bool Network_Port_SC_Hub_Function_Accept_URI(
    uint32_t object_instance, uint8_t index, BACNET_CHARACTER_STRING *str)
{
    bool status = false;
    int idx[BACNET_SC_DIRECT_ACCEPT_URI_MAX + 1] = { 0 };
    int len;

    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params && (index < BACNET_SC_DIRECT_ACCEPT_URI_MAX)) {
        len = string_splite(
            params->SC_Hub_Function_Accept_URIs, ' ', idx, sizeof(idx));
        if (index < len) {
            status = characterstring_init_ansi_safe(
                str, params->SC_Hub_Function_Accept_URIs + idx[index],
                idx[index + 1] - idx[index] - 1);
        } else {
            status = characterstring_init_ansi(str, "");
        }
    }

    return status;
}

bool Network_Port_SC_Hub_Function_Accept_URI_Set(
    uint32_t object_instance, uint8_t index, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params || (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX)) {
        return false;
    }

    string_subsstr(
        params->SC_Hub_Function_Accept_URIs,
        sizeof(params->SC_Hub_Function_Accept_URIs), index, str);

    return true;
}

/**
 * @brief Encode a BACnetARRAY property element; a function template
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to n for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for invalid array index
 */
int Network_Port_SC_Hub_Function_Accept_URI_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0;
    BACNET_CHARACTER_STRING uri = { 0 };

    if (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX) {
        apdu_len = BACNET_STATUS_ERROR;
    } else {
        if (Network_Port_SC_Hub_Function_Accept_URI(
                object_instance, index, &uri)) {
            apdu_len = encode_application_character_string(apdu, &uri);
        }
    }

    return apdu_len;
}

bool Network_Port_SC_Hub_Function_Accept_URI_Dirty_Set(
    uint32_t object_instance, uint8_t index, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params || (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX)) {
        return false;
    }

    string_subsstr(
        params->SC_Hub_Function_Accept_URIs_dirty,
        sizeof(params->SC_Hub_Function_Accept_URIs_dirty), index, str);

    return true;
}

const char *
Network_Port_SC_Hub_Function_Accept_URIs_char(uint32_t object_instance)
{
    char *p = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        p = params->SC_Hub_Function_Accept_URIs;
    }

    return p;
}

bool Network_Port_SC_Hub_Function_Accept_URIs_Set(
    uint32_t object_instance, const char *str)
{
    bool status = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        if (str) {
            snprintf(
                params->SC_Hub_Function_Accept_URIs,
                sizeof(params->SC_Hub_Function_Accept_URIs), "%s", str);
        } else {
            params->SC_Hub_Function_Accept_URIs[0] = 0;
        }
    }

    return status;
}

bool Network_Port_SC_Hub_Function_Binding(
    uint32_t object_instance, BACNET_CHARACTER_STRING *str)
{
    bool status = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    char tmp[sizeof(params->SC_Hub_Function_Binding)];

    if (params) {
        snprintf(
            tmp, sizeof(tmp), "%s:%d",
            params->SC_Hub_Function_Binding + sizeof(uint16_t),
            *(uint16_t *)params->SC_Hub_Function_Binding);

        status = characterstring_init_ansi(str, tmp);
    }

    return status;
}

void Network_Port_SC_Hub_Function_Binding_get(
    uint32_t object_instance, uint16_t *port, char **ifname)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    *port = 0;
    *ifname = NULL;

    if (params && params->SC_Hub_Function_Binding[0]) {
        *port = *(uint16_t *)params->SC_Hub_Function_Binding;
        *ifname = params->SC_Hub_Function_Binding + sizeof(uint16_t);
        if (**ifname == 0) {
            *ifname = NULL;
        }
    }
}

bool Network_Port_SC_Hub_Function_Binding_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    uint16_t port;
    char *ifname;
    char tmp[BACNET_BINDING_STRING_LENGTH];

    if (!params) {
        return false;
    }

    if (str && str[0]) {
        bsc_copy_str(tmp, str, sizeof(tmp));
        sc_binding_parse(tmp, &port, &ifname);
        *(uint16_t *)params->SC_Hub_Function_Binding = port;
        if (ifname) {
            bsc_copy_str(
                params->SC_Hub_Function_Binding + sizeof(uint16_t), ifname,
                sizeof(params->SC_Hub_Function_Binding) - sizeof(uint16_t));
        } else {
            *(params->SC_Hub_Function_Binding + sizeof(uint16_t)) = 0;
        }
    } else {
        memset(
            params->SC_Hub_Function_Binding, 0,
            sizeof(params->SC_Hub_Function_Binding));
    }

    return true;
}

bool Network_Port_SC_Hub_Function_Binding_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if (str) {
        snprintf(
            params->SC_Hub_Function_Binding_dirty,
            sizeof(params->SC_Hub_Function_Binding_dirty), "%s", str);
    } else {
        params->SC_Hub_Function_Binding_dirty[0] = 0;
    }

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *
Network_Port_SC_Hub_Function_Connection_Status_Get(
    uint32_t object_instance, uint8_t index)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (!params) {
        return NULL;
    }

    if (index >= params->SC_Hub_Function_Connection_Status_Count) {
        return NULL;
    }

    return &params->SC_Hub_Function_Connection_Status[index];
}

bool Network_Port_SC_Hub_Function_Connection_Status_Add(
    uint32_t object_instance,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_DATE_TIME *connect_ts,
    BACNET_DATE_TIME *disconnect_ts,
    BACNET_HOST_N_PORT_DATA *peer_address,
    uint8_t *peer_VMAC,
    uint8_t *peer_UUID,
    BACNET_ERROR_CODE error,
    const char *error_details)
{
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *st;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if (params->SC_Hub_Function_Connection_Status_Count + 1 >
        ARRAY_SIZE(params->SC_Hub_Function_Connection_Status)) {
        return false;
    }

    st = &params->SC_Hub_Function_Connection_Status
              [params->SC_Hub_Function_Connection_Status_Count];
    params->SC_Hub_Function_Connection_Status_Count += 1;

    st->State = state;
    st->Connect_Timestamp = *connect_ts;
    st->Disconnect_Timestamp = *disconnect_ts;
    st->Peer_Address = *peer_address;
    memcpy(st->Peer_VMAC, peer_VMAC, sizeof(st->Peer_VMAC));
    memcpy(
        st->Peer_UUID.uuid.uuid128, peer_UUID,
        sizeof(st->Peer_UUID.uuid.uuid128));
    st->Error = error;
    if (error_details) {
        bsc_copy_str(
            st->Error_Details, error_details, sizeof(st->Error_Details));
    } else {
        st->Error_Details[0] = 0;
    }

    return true;
}

bool Network_Port_SC_Hub_Function_Connection_Status_Delete_All(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Hub_Function_Connection_Status_Count = 0;

    return true;
}

int Network_Port_SC_Hub_Function_Connection_Status_Count(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return 0;
    }

    return params->SC_Hub_Function_Connection_Status_Count;
}

/**
 * @brief Handle a request to encode all the the entries
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [out] Buffer in which the APDU contents are built.
 * @param max_apdu [in] Max length of the APDU buffer.
 *
 * @return How many bytes were encoded in the buffer, or
 *   BACNET_STATUS_ABORT if the response would not fit within the buffer.
 */
int Network_Port_SC_Hub_Function_Connection_Status_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu)
{
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *entry = NULL;
    int apdu_len = 0;
    unsigned index = 0;
    unsigned size = 0;

    size =
        Network_Port_SC_Hub_Function_Connection_Status_Count(object_instance);
    for (index = 0; index < size; index++) {
        entry = Network_Port_SC_Hub_Function_Connection_Status_Get(
            object_instance, index);
        apdu_len += bacapp_encode_SCHubFunctionConnection(NULL, entry);
    }
    if (apdu_len > max_apdu) {
        return BACNET_STATUS_ABORT;
    }
    apdu_len = 0;
    for (index = 0; index < size; index++) {
        entry = Network_Port_SC_Hub_Function_Connection_Status_Get(
            object_instance, index);
        apdu_len +=
            bacapp_encode_SCHubFunctionConnection(&apdu[apdu_len], entry);
    }

    return apdu_len;
}

#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */

#if BSC_CONF_HUB_CONNECTORS_NUM != 0

bool Network_Port_SC_Direct_Connect_Initiate_Enable(uint32_t object_instance)
{
    bool enable = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        enable = params->SC_Direct_Connect_Initiate_Enable;
    }

    return enable;
}

bool Network_Port_SC_Direct_Connect_Initiate_Enable_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Direct_Connect_Initiate_Enable = value;

    return true;
}
bool Network_Port_SC_Direct_Connect_Initiate_Enable_Dirty_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Direct_Connect_Initiate_Enable_dirty = value;

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

bool Network_Port_SC_Direct_Connect_Accept_Enable(uint32_t object_instance)
{
    bool enable = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        enable = params->SC_Direct_Connect_Accept_Enable;
    }

    return enable;
}

bool Network_Port_SC_Direct_Connect_Accept_Enable_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Direct_Connect_Accept_Enable = value;

    return true;
}

bool Network_Port_SC_Direct_Connect_Accept_Enable_Dirty_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Direct_Connect_Accept_Enable_dirty = value;

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

/* return number of urls, */
/* indexes are fill first position of urls plus position of end string */
static int
string_splite(const char *str, char separator, int *indexes, int length)
{
    int i;
    int k = 0;
    indexes[0] = 0;
    for (i = 0; str[i] != 0 && k < length; i++) {
        if (str[i] == separator) {
            k++;
            indexes[k] = i + 1;
        }
    }

    if (k < length) {
        k++;
        indexes[k] = i + 1;
    }

    return k;
}

static bool string_subsstr(char *str, int length, int index, const char *substr)
{
    int idx[BACNET_SC_DIRECT_ACCEPT_URI_MAX + 1] = { 0 };
    int len;
    char *head;
    char *tail_old;
    char *tail_new;

    len = string_splite(str, ' ', idx, sizeof(idx));
    if (index < len) {
        head = str + idx[index];
        tail_old = str + idx[index + 1] - 1;
        tail_new = head + strlen(substr);
        if (tail_new + strlen(tail_old) + 1 - str >= length) {
            return false;
        }

        memmove(tail_new, tail_old, strlen(tail_old) + 1);
        memcpy(head, substr, strlen(substr));
    } else {
        head = str + strlen(str);
        if (head != str) {
            *head = ' ';
            head++;
        }
        if (head + strlen(substr) + 1 - str >= length) {
            return false;
        }
        memcpy(head, substr, strlen(substr) + 1);
    }

    return true;
}

bool Network_Port_SC_Direct_Connect_Accept_URI(
    uint32_t object_instance, uint8_t index, BACNET_CHARACTER_STRING *str)
{
    bool status = false;
    int idx[BACNET_SC_DIRECT_ACCEPT_URI_MAX + 1] = { 0 };
    int len;

    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params && (index < BACNET_SC_DIRECT_ACCEPT_URI_MAX)) {
        len = string_splite(
            params->SC_Direct_Connect_Accept_URIs, ' ', idx, sizeof(idx));
        if (index < len) {
            status = characterstring_init_ansi_safe(
                str, params->SC_Direct_Connect_Accept_URIs + idx[index],
                idx[index + 1] - idx[index] - 1);
        } else {
            status = characterstring_init_ansi(str, "");
        }
    }

    return status;
}

bool Network_Port_SC_Direct_Connect_Accept_URI_Set(
    uint32_t object_instance, uint8_t index, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params || (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX)) {
        return false;
    }

    string_subsstr(
        params->SC_Direct_Connect_Accept_URIs,
        sizeof(params->SC_Direct_Connect_Accept_URIs), index, str);

    return true;
}

/**
 * @brief Encode a BACnetARRAY property element; a function template
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to n for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
int Network_Port_SC_Direct_Connect_Accept_URI_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0;
    BACNET_CHARACTER_STRING uri = { 0 };

    if (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX) {
        apdu_len = BACNET_STATUS_ERROR;
    } else {
        if (Network_Port_SC_Direct_Connect_Accept_URI(
                object_instance, index, &uri)) {
            apdu_len = encode_application_character_string(apdu, &uri);
        }
    }

    return apdu_len;
}

bool Network_Port_SC_Direct_Connect_Accept_URI_Dirty_Set(
    uint32_t object_instance, uint8_t index, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params || (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX)) {
        return false;
    }

    string_subsstr(
        params->SC_Direct_Connect_Accept_URIs_dirty,
        sizeof(params->SC_Direct_Connect_Accept_URIs_dirty), index, str);

    return true;
}

char *Network_Port_SC_Direct_Connect_Accept_URIs_char(uint32_t object_instance)
{
    char *p = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        p = params->SC_Direct_Connect_Accept_URIs;
    }

    return p;
}

bool Network_Port_SC_Direct_Connect_Accept_URIs_Set(
    uint32_t object_instance, const char *str)
{
    bool status = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        if (str) {
            snprintf(
                params->SC_Direct_Connect_Accept_URIs,
                sizeof(params->SC_Direct_Connect_Accept_URIs), "%s", str);
        } else {
            params->SC_Direct_Connect_Accept_URIs[0] = 0;
        }
    }

    return status;
}

bool Network_Port_SC_Direct_Connect_Accept_URIs_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    bool status = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        if (str) {
            snprintf(
                params->SC_Direct_Connect_Accept_URIs_dirty,
                sizeof(params->SC_Direct_Connect_Accept_URIs_dirty), "%s", str);
        } else {
            params->SC_Direct_Connect_Accept_URIs_dirty[0] = 0;
        }
    }

    return status;
}

bool Network_Port_SC_Direct_Connect_Binding(
    uint32_t object_instance, BACNET_CHARACTER_STRING *str)
{
    bool status = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    char tmp[sizeof(params->SC_Direct_Connect_Binding)];

    if (params) {
        snprintf(
            tmp, sizeof(tmp), "%s:%d",
            params->SC_Direct_Connect_Binding + sizeof(uint16_t),
            *(uint16_t *)params->SC_Direct_Connect_Binding);

        status = characterstring_init_ansi(str, tmp);
    }

    return status;
}

void Network_Port_SC_Direct_Connect_Binding_get(
    uint32_t object_instance, uint16_t *port, char **ifname)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    *port = 0;
    *ifname = NULL;
    if (params && params->SC_Direct_Connect_Binding[0]) {
        *port = *(uint16_t *)params->SC_Direct_Connect_Binding;
        *ifname = params->SC_Direct_Connect_Binding + sizeof(uint16_t);
        if (**ifname == 0) {
            *ifname = NULL;
        }
    }
}

bool Network_Port_SC_Direct_Connect_Binding_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    uint16_t port;
    char *ifname;
    char tmp[BACNET_BINDING_STRING_LENGTH];

    if (!params) {
        return false;
    }

    if (str && str[0]) {
        bsc_copy_str(tmp, str, sizeof(tmp));
        sc_binding_parse(tmp, &port, &ifname);
        *(uint16_t *)params->SC_Direct_Connect_Binding = port;
        if (ifname) {
            bsc_copy_str(
                params->SC_Direct_Connect_Binding + sizeof(uint16_t), ifname,
                sizeof(params->SC_Direct_Connect_Binding) - sizeof(uint16_t));
        } else {
            *(params->SC_Direct_Connect_Binding + sizeof(uint16_t)) = 0;
        }
    } else {
        memset(
            params->SC_Direct_Connect_Binding, 0,
            sizeof(params->SC_Direct_Connect_Binding));
    }

    return true;
}

bool Network_Port_SC_Direct_Connect_Binding_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if (str) {
        snprintf(
            params->SC_Direct_Connect_Binding_dirty,
            sizeof(params->SC_Direct_Connect_Binding_dirty), "%s", str);
    } else {
        params->SC_Direct_Connect_Binding_dirty[0] = 0;
    }

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

BACNET_SC_DIRECT_CONNECTION_STATUS *
Network_Port_SC_Direct_Connect_Connection_Status_Get(
    uint32_t object_instance, uint8_t index)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return NULL;
    }

    if (index >= params->SC_Direct_Connect_Connection_Status_Count) {
        return NULL;
    }

    return &params->SC_Direct_Connect_Connection_Status[index];
}

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
    const char *error_details)
{
    BACNET_SC_DIRECT_CONNECTION_STATUS *st;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if (params->SC_Direct_Connect_Connection_Status_Count + 1 >
        ARRAY_SIZE(params->SC_Direct_Connect_Connection_Status)) {
        return false;
    }

    st = &params->SC_Direct_Connect_Connection_Status
              [params->SC_Direct_Connect_Connection_Status_Count];
    params->SC_Direct_Connect_Connection_Status_Count += 1;

    if (uri) {
        bsc_copy_str(st->URI, uri, sizeof(st->URI));
    } else {
        st->URI[0] = 0;
    }
    st->State = state;
    st->Connect_Timestamp = *connect_ts;
    st->Disconnect_Timestamp = *disconnect_ts;

    memcpy(&st->Peer_Address, peer_address, sizeof(st->Peer_Address));
    memcpy(st->Peer_VMAC, peer_VMAC, sizeof(st->Peer_VMAC));
    memcpy(
        st->Peer_UUID.uuid.uuid128, peer_UUID,
        sizeof(st->Peer_UUID.uuid.uuid128));
    st->Error = error;
    if (error_details) {
        bsc_copy_str(
            st->Error_Details, error_details, sizeof(st->Error_Details));
    } else {
        st->Error_Details[0] = 0;
    }

    return true;
}

bool Network_Port_SC_Direct_Connect_Connection_Status_Delete_All(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Direct_Connect_Connection_Status_Count = 0;

    return true;
}

int Network_Port_SC_Direct_Connect_Connection_Status_Count(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return 0;
    }

    return params->SC_Direct_Connect_Connection_Status_Count;
}

/**
 * @brief Handle a request to encode all the the entries
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [out] Buffer in which the APDU contents are built.
 * @param max_apdu [in] Max length of the APDU buffer.
 *
 * @return How many bytes were encoded in the buffer, or
 *   BACNET_STATUS_ABORT if the response would not fit within the buffer.
 */
int Network_Port_SC_Direct_Connect_Connection_Status_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu)
{
    BACNET_SC_DIRECT_CONNECTION_STATUS *entry = NULL;
    int apdu_len = 0;
    unsigned index = 0;
    unsigned size = 0;

    size =
        Network_Port_SC_Direct_Connect_Connection_Status_Count(object_instance);
    for (index = 0; index < size; index++) {
        entry = Network_Port_SC_Direct_Connect_Connection_Status_Get(
            object_instance, index);
        apdu_len += bacapp_encode_SCDirectConnection(NULL, entry);
    }
    if (apdu_len > max_apdu) {
        return BACNET_STATUS_ABORT;
    }
    apdu_len = 0;
    for (index = 0; index < size; index++) {
        entry = Network_Port_SC_Direct_Connect_Connection_Status_Get(
            object_instance, index);
        apdu_len += bacapp_encode_SCDirectConnection(&apdu[apdu_len], entry);
    }

    return apdu_len;
}
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

BACNET_SC_FAILED_CONNECTION_REQUEST *
Network_Port_SC_Failed_Connection_Requests_Get(
    uint32_t object_instance, uint8_t index)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return NULL;
    }

    if (index >= params->SC_Failed_Connection_Requests_Count) {
        return NULL;
    }

    return &params->SC_Failed_Connection_Requests[index];
}

bool Network_Port_SC_Failed_Connection_Requests_Add(
    uint32_t object_instance,
    BACNET_DATE_TIME *ts,
    BACNET_HOST_N_PORT_DATA *peer_address,
    uint8_t *peer_VMAC,
    uint8_t *peer_UUID,
    BACNET_ERROR_CODE error,
    const char *error_details)
{
    BACNET_SC_FAILED_CONNECTION_REQUEST *entry;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    if (params->SC_Failed_Connection_Requests_Count + 1 >
        ARRAY_SIZE(params->SC_Failed_Connection_Requests)) {
        return false;
    }

    entry = &params->SC_Failed_Connection_Requests
                 [params->SC_Failed_Connection_Requests_Count];
    params->SC_Failed_Connection_Requests_Count += 1;

    entry->Timestamp = *ts;
    memcpy(&entry->Peer_Address, peer_address, sizeof(entry->Peer_Address));
    memcpy(entry->Peer_VMAC, peer_VMAC, sizeof(entry->Peer_VMAC));
    memcpy(
        entry->Peer_UUID.uuid.uuid128, peer_UUID,
        sizeof(entry->Peer_UUID.uuid.uuid128));
    entry->Error = error;
    if (error_details) {
        bsc_copy_str(
            entry->Error_Details, error_details, sizeof(entry->Error_Details));
    } else {
        entry->Error_Details[0] = 0;
    }

    return true;
}

bool Network_Port_SC_Failed_Connection_Requests_Delete_All(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->SC_Failed_Connection_Requests_Count = 0;

    return true;
}

int Network_Port_SC_Failed_Connection_Requests_Count(uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return 0;
    }

    return params->SC_Failed_Connection_Requests_Count;
}

/**
 * @brief Handle a request to encode all the the entries
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [out] Buffer in which the APDU contents are built.
 * @param max_apdu [in] Max length of the APDU buffer.
 * @return How many bytes were encoded in the buffer, or
 *   BACNET_STATUS_ABORT if the response would not fit within the buffer.
 */
int Network_Port_SC_Failed_Connection_Requests_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu)
{
    BACNET_SC_FAILED_CONNECTION_REQUEST *entry = NULL;
    int apdu_len = 0;
    unsigned index = 0;
    unsigned size = 0;

    size = Network_Port_SC_Failed_Connection_Requests_Count(object_instance);
    for (index = 0; index < size; index++) {
        entry = Network_Port_SC_Failed_Connection_Requests_Get(
            object_instance, index);
        apdu_len += bacapp_encode_SCFailedConnectionRequest(NULL, entry);
    }
    if (apdu_len > max_apdu) {
        return BACNET_STATUS_ABORT;
    }
    apdu_len = 0;
    for (index = 0; index < size; index++) {
        entry = Network_Port_SC_Failed_Connection_Requests_Get(
            object_instance, index);
        apdu_len +=
            bacapp_encode_SCFailedConnectionRequest(&apdu[apdu_len], entry);
    }

    return apdu_len;
}

uint32_t Network_Port_Certificate_Key_File(uint32_t object_instance)
{
    uint32_t id = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return 0;
    }

    id = params->Certificate_Key_File;

    return id;
}

bool Network_Port_Certificate_Key_File_Set(
    uint32_t object_instance, uint32_t value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    params->Certificate_Key_File = value;

    return true;
}

const BACNET_UUID *Network_Port_SC_Local_UUID(uint32_t object_instance)
{
    const BACNET_UUID *uuid = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return NULL;
    }

    uuid = &params->Local_UUID;

    return uuid;
}

bool Network_Port_SC_Local_UUID_Set(
    uint32_t object_instance, BACNET_UUID *value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return false;
    }

    memcpy(&params->Local_UUID, value, sizeof(*value));

    return true;
}

void Network_Port_SC_Pending_Params_Apply(uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params) {
        return;
    }

    params->Max_BVLC_Length_Accepted = params->Max_BVLC_Length_Accepted_dirty;
    params->Max_NPDU_Length_Accepted = params->Max_NPDU_Length_Accepted_dirty;
    params->SC_Minimum_Reconnect_Time = params->SC_Minimum_Reconnect_Time_dirty;
    params->SC_Maximum_Reconnect_Time = params->SC_Maximum_Reconnect_Time_dirty;
    params->SC_Connect_Wait_Timeout = params->SC_Connect_Wait_Timeout_dirty;
    params->SC_Disconnect_Wait_Timeout =
        params->SC_Disconnect_Wait_Timeout_dirty;
    params->SC_Heartbeat_Timeout = params->SC_Heartbeat_Timeout_dirty;

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    memcpy(
        params->SC_Primary_Hub_URI, params->SC_Primary_Hub_URI_dirty,
        sizeof(params->SC_Primary_Hub_URI));
    memcpy(
        params->SC_Failover_Hub_URI, params->SC_Failover_Hub_URI_dirty,
        sizeof(params->SC_Failover_Hub_URI));

    params->SC_Hub_Function_Enable = params->SC_Hub_Function_Enable_dirty;

    memcpy(
        params->SC_Hub_Function_Accept_URIs,
        params->SC_Hub_Function_Accept_URIs_dirty,
        sizeof(params->SC_Hub_Function_Accept_URIs));

    Network_Port_SC_Hub_Function_Binding_Set(
        object_instance, params->SC_Hub_Function_Binding_dirty);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */

#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    params->SC_Direct_Connect_Initiate_Enable =
        params->SC_Direct_Connect_Initiate_Enable_dirty;
    params->SC_Direct_Connect_Accept_Enable =
        params->SC_Direct_Connect_Accept_Enable_dirty;

    memcpy(
        params->SC_Direct_Connect_Accept_URIs,
        params->SC_Direct_Connect_Accept_URIs_dirty,
        sizeof(params->SC_Direct_Connect_Accept_URIs));

    Network_Port_SC_Direct_Connect_Binding_Set(
        object_instance, params->SC_Direct_Connect_Binding_dirty);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */
}

void Network_Port_SC_Pending_Params_Discard(uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    uint16_t port = 0;
    char *ifname = NULL;

    if (!params) {
        return;
    }
    params->Max_BVLC_Length_Accepted_dirty = params->Max_BVLC_Length_Accepted;
    params->Max_NPDU_Length_Accepted_dirty = params->Max_NPDU_Length_Accepted;
    params->SC_Minimum_Reconnect_Time_dirty = params->SC_Minimum_Reconnect_Time;
    params->SC_Maximum_Reconnect_Time_dirty = params->SC_Maximum_Reconnect_Time;
    params->SC_Connect_Wait_Timeout_dirty = params->SC_Connect_Wait_Timeout;
    params->SC_Disconnect_Wait_Timeout_dirty =
        params->SC_Disconnect_Wait_Timeout;
    params->SC_Heartbeat_Timeout_dirty = params->SC_Heartbeat_Timeout;
#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    memcpy(
        params->SC_Primary_Hub_URI_dirty, params->SC_Primary_Hub_URI,
        sizeof(params->SC_Primary_Hub_URI_dirty));
    memcpy(
        params->SC_Failover_Hub_URI_dirty, params->SC_Failover_Hub_URI,
        sizeof(params->SC_Failover_Hub_URI_dirty));

    params->SC_Hub_Function_Enable_dirty = params->SC_Hub_Function_Enable;

    memcpy(
        params->SC_Hub_Function_Accept_URIs_dirty,
        params->SC_Hub_Function_Accept_URIs,
        sizeof(params->SC_Hub_Function_Accept_URIs));

    Network_Port_SC_Hub_Function_Binding_get(object_instance, &port, &ifname);
    snprintf(
        params->SC_Hub_Function_Binding_dirty,
        sizeof(params->SC_Hub_Function_Binding_dirty), "%s:%d", ifname, port);
#else
    (void)port;
    (void)ifname;
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */

#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    params->SC_Direct_Connect_Initiate_Enable_dirty =
        params->SC_Direct_Connect_Initiate_Enable;
    params->SC_Direct_Connect_Accept_Enable_dirty =
        params->SC_Direct_Connect_Accept_Enable;

    memcpy(
        params->SC_Direct_Connect_Accept_URIs_dirty,
        params->SC_Direct_Connect_Accept_URIs,
        sizeof(params->SC_Direct_Connect_Accept_URIs));

    Network_Port_SC_Direct_Connect_Binding_get(object_instance, &port, &ifname);
    snprintf(
        params->SC_Direct_Connect_Binding_dirty,
        sizeof(params->SC_Direct_Connect_Binding_dirty), "%s:%d", ifname, port);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */
}
