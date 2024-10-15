/**
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file
 * @author Mikhail Antropov
 * @date 2022
 * @brief Helper Network port objects for implement secure connect attributes
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
#include <bacnet/basic/object/device.h>
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

static int string_splite(
    const char *str, char separator, int *indexes, int length);
static bool string_subsstr(
    char *str, int length, int index, const char *substr);

static void host_n_port_to_data(
    BACNET_HOST_N_PORT *peer, BACNET_HOST_N_PORT_DATA *peer_data)
{
    peer_data->type = (peer->host_ip_address ? BACNET_HOST_N_PORT_IP : 0) +
        (peer->host_name ? BACNET_HOST_N_PORT_HOST : 0);

    if (peer->host_ip_address) {
        octetstring_copy_value(
            (uint8_t *)peer_data->host, 6, &peer->host.ip_address);
    } else if (peer->host_name) {
        characterstring_ansi_copy(
            peer_data->host, sizeof(peer_data->host), &peer->host.name);
    } else {
        peer_data->host[0] = 0;
    }

    peer_data->port = peer->port;
}

static void host_n_port_from_data(
    BACNET_HOST_N_PORT_DATA *peer_data, BACNET_HOST_N_PORT *peer)
{
    peer->host_ip_address = peer_data->type & BACNET_HOST_N_PORT_IP;
    peer->host_name = peer_data->type & BACNET_HOST_N_PORT_HOST;

    if (peer->host_ip_address) {
        octetstring_init(&peer->host.ip_address, (uint8_t *)peer_data->host, 6);
    } else if (peer->host_name) {
        characterstring_init_ansi(&peer->host.name, peer_data->host);
    }

    peer->port = peer_data->port;
}

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

BACNET_UNSIGNED_INTEGER Network_Port_Max_BVLC_Length_Accepted(
    uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER max_length = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        max_length = params->Max_BVLC_Length_Accepted;

    return max_length;
}

bool Network_Port_Max_BVLC_Length_Accepted_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->Max_BVLC_Length_Accepted = value;

    return true;
}

bool Network_Port_Max_BVLC_Length_Accepted_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->Max_BVLC_Length_Accepted_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER Network_Port_Max_NPDU_Length_Accepted(
    uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER max_length = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        max_length = params->Max_NPDU_Length_Accepted;

    return max_length;
}

bool Network_Port_Max_NPDU_Length_Accepted_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->Max_NPDU_Length_Accepted = value;

    return true;
}

bool Network_Port_Max_NPDU_Length_Accepted_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

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
    if (!params)
        return false;

    if (str)
        snprintf(params->SC_Primary_Hub_URI, sizeof(params->SC_Primary_Hub_URI),
            "%s", str);
    else
        params->SC_Primary_Hub_URI[0] = 0;

    return true;
}

bool Network_Port_SC_Primary_Hub_URI_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if (str)
        snprintf(params->SC_Primary_Hub_URI_dirty,
            sizeof(params->SC_Primary_Hub_URI_dirty), "%s", str);
    else
        params->SC_Primary_Hub_URI_dirty[0] = 0;

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
    if (!params)
        return false;

    if (str)
        snprintf(params->SC_Failover_Hub_URI,
            sizeof(params->SC_Failover_Hub_URI), "%s", str);
    else
        params->SC_Failover_Hub_URI[0] = 0;

    return true;
}

bool Network_Port_SC_Failover_Hub_URI_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if (str)
        snprintf(params->SC_Failover_Hub_URI_dirty,
            sizeof(params->SC_Failover_Hub_URI_dirty), "%s", str);
    else
        params->SC_Failover_Hub_URI_dirty[0] = 0;

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

BACNET_UNSIGNED_INTEGER Network_Port_SC_Minimum_Reconnect_Time(
    uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        timeout = params->SC_Minimum_Reconnect_Time;

    return timeout;
}

bool Network_Port_SC_Minimum_Reconnect_Time_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if ((value < SC_MIN_RECONNECT_MIN) || (value > SC_MIN_RECONNECT_MAX))
        return false;

    params->SC_Minimum_Reconnect_Time = value;

    return true;
}

bool Network_Port_SC_Minimum_Reconnect_Time_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if ((value < SC_MIN_RECONNECT_MIN) || (value > SC_MIN_RECONNECT_MAX))
        return false;

    params->SC_Minimum_Reconnect_Time_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER Network_Port_SC_Maximum_Reconnect_Time(
    uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        timeout = params->SC_Maximum_Reconnect_Time;

    return timeout;
}

bool Network_Port_SC_Maximum_Reconnect_Time_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if ((value < SC_MAX_RECONNECT_MIN) || (value > SC_MAX_RECONNECT_MAX))
        return false;

    params->SC_Maximum_Reconnect_Time = value;

    return true;
}

bool Network_Port_SC_Maximum_Reconnect_Time_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if ((value < SC_MAX_RECONNECT_MIN) || (value > SC_MAX_RECONNECT_MAX))
        return false;

    params->SC_Maximum_Reconnect_Time_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER Network_Port_SC_Connect_Wait_Timeout(
    uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        timeout = params->SC_Connect_Wait_Timeout;

    return timeout;
}

bool Network_Port_SC_Connect_Wait_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if ((value < SC_WAIT_CONNECT_MIN) || (value > SC_WAIT_CONNECT_MAX))
        return false;

    params->SC_Connect_Wait_Timeout = value;

    return true;
}

bool Network_Port_SC_Connect_Wait_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if ((value < SC_WAIT_CONNECT_MIN) || (value > SC_WAIT_CONNECT_MAX))
        return false;

    params->SC_Connect_Wait_Timeout_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER Network_Port_SC_Disconnect_Wait_Timeout(
    uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        timeout = params->SC_Disconnect_Wait_Timeout;

    return timeout;
}

bool Network_Port_SC_Disconnect_Wait_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Disconnect_Wait_Timeout = value;

    return true;
}

bool Network_Port_SC_Disconnect_Wait_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Disconnect_Wait_Timeout_dirty = value;

    return true;
}

BACNET_UNSIGNED_INTEGER Network_Port_SC_Heartbeat_Timeout(
    uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER timeout = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        timeout = params->SC_Heartbeat_Timeout;

    return timeout;
}

bool Network_Port_SC_Heartbeat_Timeout_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Heartbeat_Timeout = value;

    return true;
}

bool Network_Port_SC_Heartbeat_Timeout_Dirty_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Heartbeat_Timeout_dirty = value;

    return true;
}

BACNET_SC_HUB_CONNECTOR_STATE Network_Port_SC_Hub_Connector_State(
    uint32_t object_instance)
{
    BACNET_SC_HUB_CONNECTOR_STATE state = BACNET_NO_HUB_CONNECTION;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        state = params->SC_Hub_Connector_State;

    return state;
}

bool Network_Port_SC_Hub_Connector_State_Set(
    uint32_t object_instance, BACNET_SC_HUB_CONNECTOR_STATE value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Hub_Connector_State = value;

    return true;
}

uint32_t Network_Port_Operational_Certificate_File(uint32_t object_instance)
{
    uint32_t id = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        id = params->Operational_Certificate_File;

    return id;
}

bool Network_Port_Operational_Certificate_File_Set(
    uint32_t object_instance, uint32_t value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->Operational_Certificate_File = value;

    return true;
}

uint32_t Network_Port_Issuer_Certificate_File(
    uint32_t object_instance, uint8_t index)
{
    uint32_t id = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params && (index < BACNET_ISSUER_CERT_FILE_MAX))
        id = params->Issuer_Certificate_Files[index];

    return id;
}

bool Network_Port_Issuer_Certificate_File_Set(
    uint32_t object_instance, uint8_t index, uint32_t value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params || (index >= BACNET_ISSUER_CERT_FILE_MAX))
        return false;

    params->Issuer_Certificate_Files[index] = value;

    return true;
}

uint32_t Network_Port_Certificate_Signing_Request_File(uint32_t object_instance)
{
    uint32_t id = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        id = params->Certificate_Signing_Request_File;

    return id;
}

bool Network_Port_Certificate_Signing_Request_File_Set(
    uint32_t object_instance, uint32_t value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

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
    if (!params)
        return entry;

    entry = Keylist_Data(params->Routing_Table, network_number);

    return entry;
}

BACNET_ROUTER_ENTRY *Network_Port_Routing_Table_Get(
    uint32_t object_instance, uint8_t index)
{
    BACNET_ROUTER_ENTRY *entry = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return entry;

    entry = Keylist_Data_Index(params->Routing_Table, index);

    return entry;
}

bool Network_Port_Routing_Table_Add(uint32_t object_instance,
    uint16_t network_number,
    uint8_t *mac,
    uint8_t mac_len,
    uint8_t status,
    uint8_t performance_index)
{
    bool st = false;
    uint8_t network_type;
    BACNET_ROUTER_ENTRY *entry;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    network_type = Network_Port_Type(object_instance);
    entry = calloc(1, sizeof(BACNET_ROUTER_ENTRY));
    if (!entry)
        return false;

    entry->Network_Number = network_number;
    MAC_Address_Set(network_type, entry->Mac_Address, mac, mac_len);
    entry->Status = status;
    entry->Performance_Index = performance_index;
    st = Keylist_Data_Add(params->Routing_Table, network_number, entry);

    return st;
}

bool Network_Port_Routing_Table_Delete(
    uint32_t object_instance, uint16_t network_number)
{
    BACNET_ROUTER_ENTRY *entry;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    entry = Keylist_Data_Delete(params->Routing_Table, network_number);
    free(entry);

    return true;
}

bool Network_Port_Routing_Table_Delete_All(uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    Keylist_Delete(params->Routing_Table);
    params->Routing_Table = Keylist_Create();

    return true;
}

uint8_t Network_Port_Routing_Table_Count(uint32_t object_instance)
{
    uint8_t count = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return 0;

    count = Keylist_Count(params->Routing_Table);

    return count;
}

#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0

BACNET_SC_HUB_CONNECTION_STATUS *Network_Port_SC_Primary_Hub_Connection_Status(
    uint32_t object_instance)
{
    BACNET_SC_HUB_CONNECTION_STATUS *connection = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        connection = &params->SC_Primary_Hub_Connection_Status;

    return connection;
}

bool Network_Port_SC_Primary_Hub_Connection_Status_Set(uint32_t object_instance,
    BACNET_SC_CONNECTION_STATE state,
    BACNET_DATE_TIME *connect_ts,
    BACNET_DATE_TIME *disconnect_ts,
    BACNET_ERROR_CODE error,
    const char *error_details)
{
    BACNET_SC_HUB_CONNECTION_STATUS *st;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    st = &params->SC_Primary_Hub_Connection_Status;
    st->State = state;
    st->Connect_Timestamp = *connect_ts;
    st->Disconnect_Timestamp = *disconnect_ts;
    st->Error = error;
    if (error_details)
        bsc_copy_str(
            st->Error_Details, error_details, sizeof(st->Error_Details));
    else
        st->Error_Details[0] = 0;

    return true;
}

BACNET_SC_HUB_CONNECTION_STATUS *Network_Port_SC_Failover_Hub_Connection_Status(
    uint32_t object_instance)
{
    BACNET_SC_HUB_CONNECTION_STATUS *connection = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        connection = &params->SC_Failover_Hub_Connection_Status;

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
    if (!params)
        return false;

    st = &params->SC_Failover_Hub_Connection_Status;
    st->State = state;
    st->Connect_Timestamp = *connect_ts;
    st->Disconnect_Timestamp = *disconnect_ts;
    st->Error = error;
    if (error_details)
        bsc_copy_str(
            st->Error_Details, error_details, sizeof(st->Error_Details));
    else
        st->Error_Details[0] = 0;

    return true;
}

bool Network_Port_SC_Hub_Function_Enable(uint32_t object_instance)
{
    bool enable = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        enable = params->SC_Hub_Function_Enable;

    return enable;
}

bool Network_Port_SC_Hub_Function_Enable_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Hub_Function_Enable = value;

    return true;
}

bool Network_Port_SC_Hub_Function_Enable_Dirty_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

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
            status = characterstring_init_ansi_safe(str,
                params->SC_Hub_Function_Accept_URIs + idx[index],
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
    if (!params || (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX))
        return false;

    string_subsstr(params->SC_Hub_Function_Accept_URIs,
        sizeof(params->SC_Hub_Function_Accept_URIs), index, str);

    return true;
}

bool Network_Port_SC_Hub_Function_Accept_URI_Dirty_Set(
    uint32_t object_instance, uint8_t index, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params || (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX))
        return false;

    string_subsstr(params->SC_Hub_Function_Accept_URIs_dirty,
        sizeof(params->SC_Hub_Function_Accept_URIs_dirty), index, str);

    return true;
}

const char *Network_Port_SC_Hub_Function_Accept_URIs_char(
    uint32_t object_instance)
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
        if (str)
            snprintf(params->SC_Hub_Function_Accept_URIs,
                sizeof(params->SC_Hub_Function_Accept_URIs), "%s", str);
        else
            params->SC_Hub_Function_Accept_URIs[0] = 0;
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
        snprintf(tmp, sizeof(tmp), "%s:%d",
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

    if (!params)
        return false;

    if (str && str[0]) {
        bsc_copy_str(tmp, str, sizeof(tmp));
        sc_binding_parse(tmp, &port, &ifname);
        *(uint16_t *)params->SC_Hub_Function_Binding = port;
        if (ifname) {
            bsc_copy_str(params->SC_Hub_Function_Binding + sizeof(uint16_t),
                ifname,
                sizeof(params->SC_Hub_Function_Binding) - sizeof(uint16_t));
        } else {
            *(params->SC_Hub_Function_Binding + sizeof(uint16_t)) = 0;
        }
    } else {
        memset(params->SC_Hub_Function_Binding, 0,
            sizeof(params->SC_Hub_Function_Binding));
    }

    return true;
}

bool Network_Port_SC_Hub_Function_Binding_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if (str)
        snprintf(params->SC_Hub_Function_Binding_dirty,
            sizeof(params->SC_Hub_Function_Binding_dirty), "%s", str);
    else
        params->SC_Hub_Function_Binding_dirty[0] = 0;

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *
Network_Port_SC_Hub_Function_Connection_Status_Get(
    uint32_t object_instance, uint8_t index)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (!params)
        return NULL;

    if (index >= params->SC_Hub_Function_Connection_Status_Count)
        return NULL;

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
    if (!params)
        return false;

    if (params->SC_Hub_Function_Connection_Status_Count + 1 >
        ARRAY_SIZE(params->SC_Hub_Function_Connection_Status))
        return false;

    st = &params->SC_Hub_Function_Connection_Status
              [params->SC_Hub_Function_Connection_Status_Count];
    params->SC_Hub_Function_Connection_Status_Count += 1;

    st->State = state;
    st->Connect_Timestamp = *connect_ts;
    st->Disconnect_Timestamp = *disconnect_ts;
    st->Peer_Address = *peer_address;
    memcpy(st->Peer_VMAC, peer_VMAC, sizeof(st->Peer_VMAC));
    memcpy(st->Peer_UUID.uuid.uuid128, peer_UUID,
        sizeof(st->Peer_UUID.uuid.uuid128));
    st->Error = error;
    if (error_details)
        bsc_copy_str(
            st->Error_Details, error_details, sizeof(st->Error_Details));
    else
        st->Error_Details[0] = 0;

    return true;
}

bool Network_Port_SC_Hub_Function_Connection_Status_Delete_All(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Hub_Function_Connection_Status_Count = 0;

    return true;
}

int Network_Port_SC_Hub_Function_Connection_Status_Count(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return 0;

    return params->SC_Hub_Function_Connection_Status_Count;
}

#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */

#if BSC_CONF_HUB_CONNECTORS_NUM != 0

bool Network_Port_SC_Direct_Connect_Initiate_Enable(uint32_t object_instance)
{
    bool enable = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        enable = params->SC_Direct_Connect_Initiate_Enable;

    return enable;
}

bool Network_Port_SC_Direct_Connect_Initiate_Enable_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Direct_Connect_Initiate_Enable = value;

    return true;
}
bool Network_Port_SC_Direct_Connect_Initiate_Enable_Dirty_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Direct_Connect_Initiate_Enable_dirty = value;

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

bool Network_Port_SC_Direct_Connect_Accept_Enable(uint32_t object_instance)
{
    bool enable = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params)
        enable = params->SC_Direct_Connect_Accept_Enable;

    return enable;
}

bool Network_Port_SC_Direct_Connect_Accept_Enable_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Direct_Connect_Accept_Enable = value;

    return true;
}

bool Network_Port_SC_Direct_Connect_Accept_Enable_Dirty_Set(
    uint32_t object_instance, bool value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Direct_Connect_Accept_Enable_dirty = value;

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

/* return number of urls, */
/* indexes are fill first position of urls plus position of end string */
static int string_splite(
    const char *str, char separator, int *indexes, int length)
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
            status = characterstring_init_ansi_safe(str,
                params->SC_Direct_Connect_Accept_URIs + idx[index],
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
    if (!params || (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX))
        return false;

    string_subsstr(params->SC_Direct_Connect_Accept_URIs,
        sizeof(params->SC_Direct_Connect_Accept_URIs), index, str);

    return true;
}

bool Network_Port_SC_Direct_Connect_Accept_URI_Dirty_Set(
    uint32_t object_instance, uint8_t index, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params || (index >= BACNET_SC_DIRECT_ACCEPT_URI_MAX))
        return false;

    string_subsstr(params->SC_Direct_Connect_Accept_URIs_dirty,
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
        if (str)
            snprintf(params->SC_Direct_Connect_Accept_URIs,
                sizeof(params->SC_Direct_Connect_Accept_URIs), "%s", str);
        else
            params->SC_Direct_Connect_Accept_URIs[0] = 0;
    }

    return status;
}

bool Network_Port_SC_Direct_Connect_Accept_URIs_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    bool status = false;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);

    if (params) {
        if (str)
            snprintf(params->SC_Direct_Connect_Accept_URIs_dirty,
                sizeof(params->SC_Direct_Connect_Accept_URIs_dirty), "%s", str);
        else
            params->SC_Direct_Connect_Accept_URIs_dirty[0] = 0;
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
        snprintf(tmp, sizeof(tmp), "%s:%d",
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

    if (!params)
        return false;

    if (str && str[0]) {
        bsc_copy_str(tmp, str, sizeof(tmp));
        sc_binding_parse(tmp, &port, &ifname);
        *(uint16_t *)params->SC_Direct_Connect_Binding = port;
        if (ifname) {
            bsc_copy_str(params->SC_Direct_Connect_Binding + sizeof(uint16_t),
                ifname,
                sizeof(params->SC_Direct_Connect_Binding) - sizeof(uint16_t));
        } else {
            *(params->SC_Direct_Connect_Binding + sizeof(uint16_t)) = 0;
        }
    } else {
        memset(params->SC_Direct_Connect_Binding, 0,
            sizeof(params->SC_Direct_Connect_Binding));
    }

    return true;
}

bool Network_Port_SC_Direct_Connect_Binding_Dirty_Set(
    uint32_t object_instance, const char *str)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if (str)
        snprintf(params->SC_Direct_Connect_Binding_dirty,
            sizeof(params->SC_Direct_Connect_Binding_dirty), "%s", str);
    else
        params->SC_Direct_Connect_Binding_dirty[0] = 0;

    Network_Port_Changes_Pending_Set(object_instance, true);

    return true;
}

BACNET_SC_DIRECT_CONNECTION_STATUS *
Network_Port_SC_Direct_Connect_Connection_Status_Get(
    uint32_t object_instance, uint8_t index)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return NULL;

    if (index >= params->SC_Direct_Connect_Connection_Status_Count)
        return NULL;

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
    if (!params)
        return false;

    if (params->SC_Direct_Connect_Connection_Status_Count + 1 >
        ARRAY_SIZE(params->SC_Direct_Connect_Connection_Status))
        return false;

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
    memcpy(st->Peer_UUID.uuid.uuid128, peer_UUID,
        sizeof(st->Peer_UUID.uuid.uuid128));
    st->Error = error;
    if (error_details)
        bsc_copy_str(
            st->Error_Details, error_details, sizeof(st->Error_Details));
    else
        st->Error_Details[0] = 0;

    return true;
}

bool Network_Port_SC_Direct_Connect_Connection_Status_Delete_All(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Direct_Connect_Connection_Status_Count = 0;

    return true;
}

int Network_Port_SC_Direct_Connect_Connection_Status_Count(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return 0;

    return params->SC_Direct_Connect_Connection_Status_Count;
}

#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

BACNET_SC_FAILED_CONNECTION_REQUEST *
Network_Port_SC_Failed_Connection_Requests_Get(
    uint32_t object_instance, uint8_t index)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return NULL;

    if (index >= params->SC_Failed_Connection_Requests_Count)
        return NULL;

    return &params->SC_Failed_Connection_Requests[index];
}

bool Network_Port_SC_Failed_Connection_Requests_Add(uint32_t object_instance,
    BACNET_DATE_TIME *ts,
    BACNET_HOST_N_PORT_DATA *peer_address,
    uint8_t *peer_VMAC,
    uint8_t *peer_UUID,
    BACNET_ERROR_CODE error,
    const char *error_details)
{
    BACNET_SC_FAILED_CONNECTION_REQUEST *entry;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    if (params->SC_Failed_Connection_Requests_Count + 1 >
        ARRAY_SIZE(params->SC_Failed_Connection_Requests))
        return false;

    entry = &params->SC_Failed_Connection_Requests
                 [params->SC_Failed_Connection_Requests_Count];
    params->SC_Failed_Connection_Requests_Count += 1;

    entry->Timestamp = *ts;
    // host_n_port_to_data(peer_address, &entry->Peer_Address);
    memcpy(&entry->Peer_Address, peer_address, sizeof(entry->Peer_Address));
    memcpy(entry->Peer_VMAC, peer_VMAC, sizeof(entry->Peer_VMAC));
    memcpy(entry->Peer_UUID.uuid.uuid128, peer_UUID,
        sizeof(entry->Peer_UUID.uuid.uuid128));
    entry->Error = error;
    if (error_details)
        bsc_copy_str(
            entry->Error_Details, error_details, sizeof(entry->Error_Details));
    else
        entry->Error_Details[0] = 0;

    return true;
}

bool Network_Port_SC_Failed_Connection_Requests_Delete_All(
    uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->SC_Failed_Connection_Requests_Count = 0;

    return true;
}

int Network_Port_SC_Failed_Connection_Requests_Count(uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return 0;

    return params->SC_Failed_Connection_Requests_Count;
}

uint32_t Network_Port_Certificate_Key_File(uint32_t object_instance)
{
    uint32_t id = 0;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return 0;

    id = params->Certificate_Key_File;

    return id;
}

bool Network_Port_Certificate_Key_File_Set(
    uint32_t object_instance, uint32_t value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    params->Certificate_Key_File = value;

    return true;
}

const BACNET_UUID *Network_Port_SC_Local_UUID(uint32_t object_instance)
{
    const BACNET_UUID *uuid = NULL;
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return NULL;

    uuid = &params->Local_UUID;

    return uuid;
}

bool Network_Port_SC_Local_UUID_Set(
    uint32_t object_instance, BACNET_UUID *value)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return false;

    memcpy(&params->Local_UUID, value, sizeof(*value));

    return true;
}

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0

/**
 * @brief Encode a SCHubConnection complex data type
 *
 *  BACnetSCHubConnection ::= SEQUENCE {
 *      connection-state [0] BACnetSCConnectionState,
 *      connect-timestamp [1] BACnetDateTime,
 *      disconnect-timestamp [2] BACnetDateTime,
 *      error [3] Error OPTIONAL,
 *      error-details [4] CharacterString OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_SCHubConnection(
    uint8_t *apdu, BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    BACNET_CHARACTER_STRING str;

    if (apdu && value) {
        apdu_len += encode_context_enumerated(&apdu[0], 0, value->State);
        apdu_len += bacapp_encode_context_datetime(
            &apdu[apdu_len], 1, &value->Connect_Timestamp);
        apdu_len += bacapp_encode_context_datetime(
            &apdu[apdu_len], 2, &value->Disconnect_Timestamp);

        if (value->State == BACNET_DISCONNECTED_WITH_ERRORS ||
            value->State == BACNET_FAILED_TO_CONNECT) {
            apdu_len +=
                encode_context_enumerated(&apdu[apdu_len], 3, value->Error);

            if (characterstring_init_ansi(&str, value->Error_Details)) {
                apdu_len +=
                    encode_context_character_string(&apdu[apdu_len], 4, &str);
            }
        }
    }
    return apdu_len;
}

int bacapp_encode_context_SCHubConnection(
    uint8_t *apdu, uint8_t tag_number, BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value && apdu) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
        len = bacapp_encode_SCHubConnection(&apdu[apdu_len], value);
        apdu_len += len;
        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_context_SCHubConnection(
    uint8_t *apdu, uint8_t tag_number, BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    int len = 0;
    const uint16_t apdu_len_max = MAX_APDU;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len =
            bacapp_decode_SCHubConnection(&apdu[len], apdu_len_max, value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

/**
 * @brief Encode a SCHubFunctionConnection complex data type
 *
 *  BACnetSCHubFunctionConnection ::= SEQUENCE {
 *      connection-state [0] BACnetSCConnectionState,
 *      connect-timestamp [1] BACnetDateTime,
 *      disconnect-timestamp [2] BACnetDateTime,
 *      peer-address [3] BACnetHostNPort,
 *      peer-vmac [4] OCTET STRING (SIZE(6)),
 *      peer-uuid [5] OCTET STRING (SIZE(16)),
 *      error [6] Error OPTIONAL,
 *      error-details [7] CharacterString OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_SCHubFunctionConnection(
    uint8_t *apdu, BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if (apdu && value) {
        apdu_len += encode_context_enumerated(&apdu[0], 0, value->State);
        apdu_len += bacapp_encode_context_datetime(
            &apdu[apdu_len], 1, &value->Connect_Timestamp);
        apdu_len += bacapp_encode_context_datetime(
            &apdu[apdu_len], 2, &value->Disconnect_Timestamp);
        host_n_port_from_data(&value->Peer_Address, &hp);
        apdu_len += host_n_port_context_encode(&apdu[apdu_len], 3, &hp);

        if (octetstring_init(
                &octet, value->Peer_VMAC, sizeof(value->Peer_VMAC))) {
            apdu_len += encode_context_octet_string(&apdu[apdu_len], 4, &octet);
        } else {
            return -1;
        }

        if (octetstring_init(&octet, value->Peer_UUID.uuid.uuid128,
                sizeof(value->Peer_UUID.uuid.uuid128))) {
            apdu_len += encode_context_octet_string(&apdu[apdu_len], 5, &octet);
        } else {
            return -1;
        }
        if (value->State == BACNET_DISCONNECTED_WITH_ERRORS ||
            value->State == BACNET_FAILED_TO_CONNECT) {
            apdu_len +=
                encode_context_enumerated(&apdu[apdu_len], 6, value->Error);

            if (characterstring_init_ansi(&str, value->Error_Details)) {
                apdu_len +=
                    encode_context_character_string(&apdu[apdu_len], 7, &str);
            }
        }
    }
    return apdu_len;
}

int bacapp_encode_context_SCHubFunctionConnection(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value && apdu) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
        len = bacapp_encode_SCHubFunctionConnection(&apdu[apdu_len], value);
        apdu_len += len;
        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_context_SCHubFunctionConnection(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    int len = 0;
    const uint16_t apdu_len_max = MAX_APDU;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len = bacapp_decode_SCHubFunctionConnection(
            &apdu[len], apdu_len_max, value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */

int bacapp_decode_SCHubFunctionConnection(uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    int len;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 0, &value->State)) ==
        -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 1, &value->Connect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 2, &value->Disconnect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = host_n_port_context_decode(apdu, apdu_len, &value->Error, 3, &hp)) == -1) {
        return -1;
    }
    host_n_port_to_data(&hp, &value->Peer_Address);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 4, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_VMAC, sizeof(value->Peer_VMAC), &octet);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 5, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_UUID.uuid.uuid128,
        sizeof(value->Peer_UUID.uuid.uuid128), &octet);
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 6, &value->Error)) >
        0) {
        apdu_len += len;
    } else {
        value->Error = ERROR_CODE_OTHER;
    }

    if ((len = decode_context_character_string(&apdu[apdu_len], 7, &str)) > 0) {
        snprintf(value->Error_Details, sizeof(value->Error_Details), "%s",
            characterstring_value(&str));
        apdu_len += len;
    } else {
        value->Error_Details[0] = 0;
    }

    return (apdu_len <= max_apdu_len) ? apdu_len : -1;
}

int bacapp_decode_SCHubConnection(uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    int len;
    BACNET_CHARACTER_STRING str;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 0, &value->State)) ==
        -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 1, &value->Connect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 2, &value->Disconnect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 3, &value->Error)) >
        0) {
        apdu_len += len;
    } else {
        value->Error = ERROR_CODE_OTHER;
    }

    if ((len = decode_context_character_string(&apdu[apdu_len], 4, &str)) > 0) {
        snprintf(value->Error_Details, sizeof(value->Error_Details), "%s",
            characterstring_value(&str));
        apdu_len += len;
    } else {
        value->Error_Details[0] = 0;
    }

    return (apdu_len <= max_apdu_len) ? apdu_len : -1;
}
/**
 * @brief Encode a SCFailedConnectionRequest complex data type
 *
 *  BACnetSCFailedConnectionRequest ::= SEQUENCE {
 *      timestamp [0] BACnetDateTime,
 *      peer-address [1] BACnetHostNPort,
 *      peer-vmac [2] OCTET STRING (SIZE(6)) OPTIONAL,
 *      peer-uuid [3] OCTET STRING (SIZE(16)) OPTIONAL,
 *      error [4] Error,
 *      error-details [5] CharacterString OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_SCFailedConnectionRequest(
    uint8_t *apdu, BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    int apdu_len = 0;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if (apdu && value) {
        apdu_len +=
            bacapp_encode_context_datetime(&apdu[0], 0, &value->Timestamp);
        host_n_port_from_data(&value->Peer_Address, &hp);
        apdu_len += host_n_port_context_encode(&apdu[apdu_len], 1, &hp);

        if (octetstring_init(
                &octet, value->Peer_VMAC, sizeof(value->Peer_VMAC))) {
            apdu_len += encode_context_octet_string(&apdu[apdu_len], 2, &octet);
        } else {
            return -1;
        }

        if (octetstring_init(&octet, value->Peer_UUID.uuid.uuid128,
                sizeof(value->Peer_UUID.uuid.uuid128))) {
            apdu_len += encode_context_octet_string(&apdu[apdu_len], 3, &octet);
        } else {
            return -1;
        }

        if (value->Error != ERROR_CODE_OTHER) {
            apdu_len +=
                encode_context_enumerated(&apdu[apdu_len], 4, value->Error);

            if (characterstring_init_ansi(&str, value->Error_Details)) {
                apdu_len +=
                    encode_context_character_string(&apdu[apdu_len], 5, &str);
            }
        }
    }
    return apdu_len;
}

int bacapp_decode_SCFailedConnectionRequest(uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    int apdu_len = 0;
    int len;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if ((len = bacapp_decode_context_datetime(
             &apdu[0], 0, &value->Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = host_n_port_context_decode(apdu, apdu_len, &value->Error, 1, &hp)) == -1) {
        return -1;
    }
    host_n_port_to_data(&hp, &value->Peer_Address);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 2, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_VMAC, sizeof(value->Peer_VMAC), &octet);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 3, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_UUID.uuid.uuid128,
        sizeof(value->Peer_UUID.uuid.uuid128), &octet);
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 4, &value->Error)) >
        0) {
        apdu_len += len;
    } else {
        value->Error = ERROR_CODE_OTHER;
    }

    if ((len = decode_context_character_string(&apdu[apdu_len], 5, &str)) > 0) {
        snprintf(value->Error_Details, sizeof(value->Error_Details), "%s",
            characterstring_value(&str));
        apdu_len += len;
    } else {
        value->Error_Details[0] = 0;
    }

    return (apdu_len <= max_apdu_len) ? apdu_len : -1;
}

int bacapp_encode_context_SCFailedConnectionRequest(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value && apdu) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
        len = bacapp_encode_SCFailedConnectionRequest(&apdu[apdu_len], value);
        apdu_len += len;
        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_context_SCFailedConnectionRequest(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    int len = 0;
    const uint16_t apdu_len_max = MAX_APDU;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len = bacapp_decode_SCFailedConnectionRequest(
            &apdu[len], apdu_len_max, value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE

/**
 * @brief Encode a SCFailedConnectionRequest complex data type
 *
 *  BACnetRouterEntry ::= SEQUENCE {
 *      network-number [0] Unsigned16,
 *      mac-address [1] OCTET STRING,
 *      status [2] ENUMERATED {
 *          available (0),
 *          busy (1),
 *          disconnected (2)
 *      },
 *      performance-index [3] Unsigned8 OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_RouterEntry(uint8_t *apdu, BACNET_ROUTER_ENTRY *value)
{
    int apdu_len = 0;
    BACNET_OCTET_STRING octet;
    BACNET_UNSIGNED_INTEGER v;

    if (apdu && value) {
        v = value->Network_Number;
        apdu_len += encode_context_unsigned(&apdu[0], 0, v);

        if (octetstring_init(
                &octet, value->Mac_Address, sizeof(value->Mac_Address))) {
            apdu_len += encode_context_octet_string(&apdu[apdu_len], 1, &octet);
        } else {
            return -1;
        }

        apdu_len +=
            encode_context_enumerated(&apdu[apdu_len], 2, value->Status);

        if (value->Performance_Index != 0) {
            v = value->Performance_Index;
            apdu_len += encode_context_unsigned(&apdu[apdu_len], 3, v);
        }
    }
    return apdu_len;
}

int bacapp_decode_RouterEntry(uint8_t *apdu, BACNET_ROUTER_ENTRY *value)
{
    int apdu_len = 0;
    int len;
    BACNET_OCTET_STRING octet;
    BACNET_UNSIGNED_INTEGER v;

    if ((len = decode_context_unsigned(&apdu[0], 0, &v)) == -1) {
        return -1;
    }
    value->Network_Number = (uint16_t)v;
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 1, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(
        value->Mac_Address, sizeof(value->Mac_Address), &octet);
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 2, &value->Status)) ==
        -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = decode_context_unsigned(&apdu[apdu_len], 3, &v)) > 0) {
        value->Performance_Index = (uint8_t)v;
        apdu_len += len;
    } else {
        value->Performance_Index = 0;
    }

    return apdu_len;
}

int bacapp_encode_context_RouterEntry(
    uint8_t *apdu, uint8_t tag_number, BACNET_ROUTER_ENTRY *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value && apdu) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
        len = bacapp_encode_RouterEntry(&apdu[apdu_len], value);
        apdu_len += len;
        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_context_RouterEntry(
    uint8_t *apdu, uint8_t tag_number, BACNET_ROUTER_ENTRY *value)
{
    int len = 0;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len = bacapp_decode_RouterEntry(&apdu[len], value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */

#if BSC_CONF_HUB_CONNECTORS_NUM != 0

/**
 * @brief Encode a SCDirectConnection complex data type
 *
 * * BACnetSCDirectConnection ::= SEQUENCE {
 *      uri [0] CharacterString,
 *      connection-state [1] BACnetSCConnectionState,
 *      connect-timestamp [2] BACnetDateTime,
 *      disconnect-timestamp [3] BACnetDateTime,
 *      peer-address [4] BACnetHostNPort OPTIONAL,
 *      peer-vmac [5] OCTET STRING (SIZE(6)) OPTIONAL,
 *      peer-uuid [6] OCTET STRING (SIZE(16)) OPTIONAL,
 *      error [7] Error OPTIONAL,
 *      error-details [8] CharacterString OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_SCDirectConnection(
    uint8_t *apdu, BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if (apdu && value) {
        if (characterstring_init_ansi(&str, value->URI)) {
            apdu_len += encode_context_character_string(&apdu[0], 0, &str);
        } else {
            return -1;
        }

        apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, value->State);
        apdu_len += bacapp_encode_context_datetime(
            &apdu[apdu_len], 2, &value->Connect_Timestamp);
        apdu_len += bacapp_encode_context_datetime(
            &apdu[apdu_len], 3, &value->Disconnect_Timestamp);
        host_n_port_from_data(&value->Peer_Address, &hp);
        apdu_len += host_n_port_context_encode(&apdu[apdu_len], 4, &hp);

        if (octetstring_init(
                &octet, value->Peer_VMAC, sizeof(value->Peer_VMAC))) {
            apdu_len += encode_context_octet_string(&apdu[apdu_len], 5, &octet);
        } else {
            return -1;
        }

        if (octetstring_init(&octet, value->Peer_UUID.uuid.uuid128,
                sizeof(value->Peer_UUID.uuid.uuid128))) {
            apdu_len += encode_context_octet_string(&apdu[apdu_len], 6, &octet);
        } else {
            return -1;
        }

        if (value->State == BACNET_DISCONNECTED_WITH_ERRORS ||
            value->State == BACNET_FAILED_TO_CONNECT) {
            apdu_len +=
                encode_context_enumerated(&apdu[apdu_len], 7, value->Error);

            if (characterstring_init_ansi(&str, value->Error_Details)) {
                apdu_len +=
                    encode_context_character_string(&apdu[apdu_len], 8, &str);
            }
        }
    }
    return apdu_len;
}

int bacapp_encode_context_SCDirectConnection(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value && apdu) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
        len = bacapp_encode_SCDirectConnection(&apdu[apdu_len], value);
        apdu_len += len;
        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_context_SCDirectConnection(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    int len = 0;
    const uint16_t apdu_len_max = MAX_APDU;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len =
            bacapp_decode_SCDirectConnection(&apdu[len], apdu_len_max, value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

int bacapp_decode_SCDirectConnection(uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    int len;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if ((len = decode_context_character_string(&apdu[apdu_len], 0, &str)) ==
        -1) {
        return -1;
    }
    snprintf(value->URI, sizeof(value->URI), "%s", characterstring_value(&str));
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 1, &value->State)) ==
        -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 2, &value->Connect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 3, &value->Disconnect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = host_n_port_context_decode(apdu, apdu_len, &value->Error, 4, &hp)) == -1) {
        return -1;
    }
    host_n_port_to_data(&hp, &value->Peer_Address);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 5, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_VMAC, sizeof(value->Peer_VMAC), &octet);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 6, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_UUID.uuid.uuid128,
        sizeof(value->Peer_UUID.uuid.uuid128), &octet);
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 7, &value->Error)) >
        0) {
        apdu_len += len;
    } else {
        value->Error = ERROR_CODE_OTHER;
    }

    if ((len = decode_context_character_string(&apdu[apdu_len], 8, &str)) > 0) {
        snprintf(value->Error_Details, sizeof(value->Error_Details), "%s",
            characterstring_value(&str));
        apdu_len += len;
    } else {
        value->Error_Details[0] = 0;
    }

    return (apdu_len <= max_apdu_len) ? apdu_len : -1;
}

void Network_Port_SC_Pending_Params_Apply(uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    if (!params)
        return;

    params->Max_BVLC_Length_Accepted = params->Max_BVLC_Length_Accepted_dirty;
    params->Max_NPDU_Length_Accepted = params->Max_NPDU_Length_Accepted_dirty;
    params->SC_Minimum_Reconnect_Time = params->SC_Minimum_Reconnect_Time_dirty;
    params->SC_Maximum_Reconnect_Time = params->SC_Maximum_Reconnect_Time_dirty;
    params->SC_Connect_Wait_Timeout = params->SC_Connect_Wait_Timeout_dirty;
    params->SC_Disconnect_Wait_Timeout =
        params->SC_Disconnect_Wait_Timeout_dirty;
    params->SC_Heartbeat_Timeout = params->SC_Heartbeat_Timeout_dirty;

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    memcpy(params->SC_Primary_Hub_URI, params->SC_Primary_Hub_URI_dirty,
        sizeof(params->SC_Primary_Hub_URI));
    memcpy(params->SC_Failover_Hub_URI, params->SC_Failover_Hub_URI_dirty,
        sizeof(params->SC_Failover_Hub_URI));

    params->SC_Hub_Function_Enable = params->SC_Hub_Function_Enable_dirty;

    memcpy(params->SC_Hub_Function_Accept_URIs,
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

    memcpy(params->SC_Direct_Connect_Accept_URIs,
        params->SC_Direct_Connect_Accept_URIs_dirty,
        sizeof(params->SC_Direct_Connect_Accept_URIs));

    Network_Port_SC_Direct_Connect_Binding_Set(
        object_instance, params->SC_Direct_Connect_Binding_dirty);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */
}

void Network_Port_SC_Pending_Params_Discard(uint32_t object_instance)
{
    BACNET_SC_PARAMS *params = Network_Port_SC_Params(object_instance);
    uint16_t port;
    char *ifname;

    if (!params)
        return;

    (void)port;
    (void)ifname;

    params->Max_BVLC_Length_Accepted_dirty = params->Max_BVLC_Length_Accepted;
    params->Max_NPDU_Length_Accepted_dirty = params->Max_NPDU_Length_Accepted;
    params->SC_Minimum_Reconnect_Time_dirty = params->SC_Minimum_Reconnect_Time;
    params->SC_Maximum_Reconnect_Time_dirty = params->SC_Maximum_Reconnect_Time;
    params->SC_Connect_Wait_Timeout_dirty = params->SC_Connect_Wait_Timeout;
    params->SC_Disconnect_Wait_Timeout_dirty =
        params->SC_Disconnect_Wait_Timeout;
    params->SC_Heartbeat_Timeout_dirty = params->SC_Heartbeat_Timeout;

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    memcpy(params->SC_Primary_Hub_URI_dirty, params->SC_Primary_Hub_URI,
        sizeof(params->SC_Primary_Hub_URI_dirty));
    memcpy(params->SC_Failover_Hub_URI_dirty, params->SC_Failover_Hub_URI,
        sizeof(params->SC_Failover_Hub_URI_dirty));

    params->SC_Hub_Function_Enable_dirty = params->SC_Hub_Function_Enable;

    memcpy(params->SC_Hub_Function_Accept_URIs_dirty,
        params->SC_Hub_Function_Accept_URIs,
        sizeof(params->SC_Hub_Function_Accept_URIs));

    Network_Port_SC_Hub_Function_Binding_get(object_instance, &port, &ifname);
    snprintf(params->SC_Hub_Function_Binding_dirty,
        sizeof(params->SC_Hub_Function_Binding_dirty), "%s:%d", ifname, port);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */

#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    params->SC_Direct_Connect_Initiate_Enable_dirty =
        params->SC_Direct_Connect_Initiate_Enable;
    params->SC_Direct_Connect_Accept_Enable_dirty =
        params->SC_Direct_Connect_Accept_Enable;

    memcpy(params->SC_Direct_Connect_Accept_URIs_dirty,
        params->SC_Direct_Connect_Accept_URIs,
        sizeof(params->SC_Direct_Connect_Accept_URIs));

    Network_Port_SC_Direct_Connect_Binding_get(object_instance, &port, &ifname);
    snprintf(params->SC_Direct_Connect_Binding_dirty,
        sizeof(params->SC_Direct_Connect_Binding_dirty), "%s:%d", ifname, port);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */
}

static int bacapp_snprintf_timestamp(
    char *str, size_t str_len, BACNET_DATE_TIME *ts)
{
    return snprintf(str, str_len, "%04u-%02u-%02uT%02u:%02u:%02u.%03u, ",
        ts->date.year, ts->date.month, ts->date.day, ts->time.hour,
        ts->time.min, ts->time.sec, ts->time.hundredths);
}

static int bacapp_snprintf_host_n_port(
    char *str, size_t str_len, BACNET_HOST_N_PORT_DATA *host_port)
{
    return snprintf(str, str_len, "%u.%u.%u.%u:%u, ",
        (uint8_t)host_port->host[0], (uint8_t)host_port->host[1],
        (uint8_t)host_port->host[2], (uint8_t)host_port->host[3],
        host_port->port);
}

static int bacapp_snprintf_vmac(char *str, size_t str_len, uint8_t *vmac)
{
    return snprintf(str, str_len, "%u.%u.%u.%u.%u.%u, ", vmac[0], vmac[1],
        vmac[2], vmac[3], vmac[4], vmac[5]);
}

static int bacapp_snprintf_uuid(char *str, size_t str_len, BACNET_UUID *uuid)
{
    struct guid *guid = &uuid->uuid.guid;
    uint8_t *p = guid->clock_seq_and_node;
    return snprintf(str, str_len,
        "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x, ",
        guid->time_low, guid->time_mid, guid->time_hi_and_version, p[0], p[1],
        p[2], p[3], p[4], p[5], p[6], p[7]);
}

#define SNPRINTF_AND_MOVE(...)                  \
    slen = snprintf(str, str_len, __VA_ARGS__); \
    if (str) {                                  \
        str += slen;                            \
        str_len -= slen;                        \
    }                                           \
    ret_val += slen;

#define BACAPP_SNPRINTF_AND_MOVE(function, param) \
    slen = (function)(str, str_len, (param));     \
    if (str) {                                    \
        str += slen;                              \
        str_len -= slen;                          \
    }                                             \
    ret_val += slen;

int Network_Port_SC_snprintf_value(
    char *str, size_t str_len, const BACNET_OBJECT_PROPERTY_VALUE *object_value)
{
    BACNET_APPLICATION_DATA_VALUE *value = object_value->value;
    BACNET_PROPERTY_ID property = object_value->object_property;
    // BACNET_OBJECT_TYPE object_type = object_value->object_type;
    BACNET_ARRAY_INDEX array_index = object_value->array_index;
    BACNET_APPLICATION_DATA_VALUE result;

    int ret_val = 0;
    int len = 0;
    int slen;

    switch (property) {
        case PROP_SC_FAILED_CONNECTION_REQUESTS:
            if (array_index == 0) {
                len = bacapp_decode_generic_property(
                    value->type.Custom_Value.data, value->type.Custom_Value.len,
                    &result, property);
                ret_val =
                    snprintf(str, str_len, "%ld", (long)result.type.Signed_Int);
            } else {
                BACNET_SC_FAILED_CONNECTION_REQUEST req;
                while (len < value->type.Custom_Value.len) {
                    int rc = bacapp_decode_SCFailedConnectionRequest(
                        value->type.Custom_Value.data + len,
                        value->type.Custom_Value.len - len, &req);
                    if (rc > 0) {
                        SNPRINTF_AND_MOVE("{");
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_timestamp, &req.Timestamp);
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_host_n_port, &req.Peer_Address);
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_vmac, req.Peer_VMAC);
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_uuid, &req.Peer_UUID);
                        SNPRINTF_AND_MOVE("%d", req.Error);
                        if (req.Error_Details[0]) {
                            SNPRINTF_AND_MOVE(", \"%s\"", req.Error_Details);
                        }
                        SNPRINTF_AND_MOVE("}");
                        len += rc;
                    } else {
                        break;
                    }
                }
            }
            break;

        case PROP_SC_HUB_FUNCTION_CONNECTION_STATUS:
            if (array_index == 0) {
                len = bacapp_decode_generic_property(
                    value->type.Custom_Value.data, value->type.Custom_Value.len,
                    &result, property);
                ret_val =
                    snprintf(str, str_len, "%ld", (long)result.type.Signed_Int);
            } else {
                BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS status;
                while (len < value->type.Custom_Value.len) {
                    int rc = bacapp_decode_SCHubFunctionConnection(
                        value->type.Custom_Value.data + len,
                        value->type.Custom_Value.len - len, &status);
                    if (rc > 0) {
                        SNPRINTF_AND_MOVE("{%d, ", status.State);
                        BACAPP_SNPRINTF_AND_MOVE(bacapp_snprintf_timestamp,
                            &status.Connect_Timestamp);
                        BACAPP_SNPRINTF_AND_MOVE(bacapp_snprintf_timestamp,
                            &status.Disconnect_Timestamp);
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_host_n_port, &status.Peer_Address);
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_vmac, status.Peer_VMAC);
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_uuid, &status.Peer_UUID);
                        SNPRINTF_AND_MOVE("%d", status.Error);
                        if (status.Error_Details[0]) {
                            SNPRINTF_AND_MOVE(", \"%s\"", status.Error_Details);
                        }
                        SNPRINTF_AND_MOVE("}");
                        len += rc;
                    } else {
                        break;
                    }
                }
            }
            break;

        case PROP_SC_DIRECT_CONNECT_CONNECTION_STATUS:
            if (array_index == 0) {
                len = bacapp_decode_generic_property(
                    value->type.Custom_Value.data, value->type.Custom_Value.len,
                    &result, property);
                ret_val =
                    snprintf(str, str_len, "%ld", (long)result.type.Signed_Int);
            } else {
                BACNET_SC_DIRECT_CONNECTION_STATUS status;
                while (len < value->type.Custom_Value.len) {
                    int rc = bacapp_decode_SCDirectConnection(
                        value->type.Custom_Value.data + len,
                        value->type.Custom_Value.len - len, &status);
                    if (rc > 0) {
                        SNPRINTF_AND_MOVE("{%s, %d, ",
                            status.URI[0] ? status.URI : "NULL", status.State);
                        BACAPP_SNPRINTF_AND_MOVE(bacapp_snprintf_timestamp,
                            &status.Connect_Timestamp);
                        BACAPP_SNPRINTF_AND_MOVE(bacapp_snprintf_timestamp,
                            &status.Disconnect_Timestamp);
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_host_n_port, &status.Peer_Address);
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_vmac, status.Peer_VMAC);
                        BACAPP_SNPRINTF_AND_MOVE(
                            bacapp_snprintf_uuid, &status.Peer_UUID);
                        SNPRINTF_AND_MOVE("%d", status.Error);
                        if (status.Error_Details[0]) {
                            SNPRINTF_AND_MOVE(", \"%s\"", status.Error_Details);
                        }
                        SNPRINTF_AND_MOVE("}");
                        len += rc;
                    } else {
                        break;
                    }
                }
            }
            break;

        case PROP_SC_PRIMARY_HUB_CONNECTION_STATUS:
        case PROP_SC_FAILOVER_HUB_CONNECTION_STATUS: {
            BACNET_SC_HUB_CONNECTION_STATUS status;
            int rc =
                bacapp_decode_SCHubConnection(value->type.Custom_Value.data,
                    value->type.Custom_Value.len, &status);
            if (rc > 0) {
                SNPRINTF_AND_MOVE("{%d, ", status.State);
                BACAPP_SNPRINTF_AND_MOVE(
                    bacapp_snprintf_timestamp, &status.Connect_Timestamp);
                BACAPP_SNPRINTF_AND_MOVE(
                    bacapp_snprintf_timestamp, &status.Disconnect_Timestamp);
                SNPRINTF_AND_MOVE("%d", status.Error);
                if (status.Error_Details[0]) {
                    SNPRINTF_AND_MOVE(", \"%s\"", status.Error_Details);
                }
                SNPRINTF_AND_MOVE("}");
                len += rc;
            }
        } break;

        default:
            ret_val = snprintf(str, str_len, "UnknownType(tag=%d)", value->tag);
            break;
    }
    return ret_val;
}
