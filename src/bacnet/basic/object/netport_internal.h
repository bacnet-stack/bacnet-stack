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

#ifndef NETPORT_INTERNAL_H
#define NETPORT_INTERNAL_H

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
#include "bacnet/basic/object/sc_netport.h"
/*#include "bacnet/basic/object/netport.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Support
   - SC_Hub_Function_Connection_Status
   - SC_Direct_Connect_Connection_Status
   - SC_FailedConnectionRequests
*/
/*#define BACNET_SC_STATUS_SUPPORT */

    typedef struct BACnetSCAttributes_T {
        BACNET_UNSIGNED_INTEGER Max_BVLC_Length_Accepted;
        BACNET_UNSIGNED_INTEGER Max_BVLC_Length_Accepted_dirty;
        BACNET_UNSIGNED_INTEGER Max_NPDU_Length_Accepted;
        BACNET_UNSIGNED_INTEGER Max_NPDU_Length_Accepted_dirty;
        char SC_Primary_Hub_URI[BACNET_URI_LENGTH];
        char SC_Primary_Hub_URI_dirty[BACNET_URI_LENGTH];
        char SC_Failover_Hub_URI[BACNET_URI_LENGTH];
        char SC_Failover_Hub_URI_dirty[BACNET_URI_LENGTH];
        BACNET_UNSIGNED_INTEGER SC_Minimum_Reconnect_Time;
        BACNET_UNSIGNED_INTEGER SC_Minimum_Reconnect_Time_dirty;
        BACNET_UNSIGNED_INTEGER SC_Maximum_Reconnect_Time;
        BACNET_UNSIGNED_INTEGER SC_Maximum_Reconnect_Time_dirty;
        BACNET_UNSIGNED_INTEGER SC_Connect_Wait_Timeout;
        BACNET_UNSIGNED_INTEGER SC_Connect_Wait_Timeout_dirty;
        BACNET_UNSIGNED_INTEGER SC_Disconnect_Wait_Timeout;
        BACNET_UNSIGNED_INTEGER SC_Disconnect_Wait_Timeout_dirty;
        BACNET_UNSIGNED_INTEGER SC_Heartbeat_Timeout;
        BACNET_UNSIGNED_INTEGER SC_Heartbeat_Timeout_dirty;
        BACNET_SC_HUB_CONNECTOR_STATE SC_Hub_Connector_State;
        uint32_t Operational_Certificate_File;
        uint32_t Issuer_Certificate_Files[BACNET_ISSUER_CERT_FILE_MAX];
        uint32_t Certificate_Signing_Request_File;
        /* Optional params */
#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE
        OS_Keylist Routing_Table;
#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */
#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
        BACNET_SC_HUB_CONNECTION SC_Primary_Hub_Connection_Status;
        BACNET_SC_HUB_CONNECTION SC_Failover_Hub_Connection_Status;
        bool SC_Hub_Function_Enable;
        bool SC_Hub_Function_Enable_dirty;
        char SC_Hub_Function_Accept_URIs[
            BACNET_SC_HUB_URI_MAX * (BACNET_URI_LENGTH + 1)];
        char SC_Hub_Function_Accept_URIs_dirty[
            BACNET_SC_HUB_URI_MAX * (BACNET_URI_LENGTH + 1)];
        char SC_Hub_Function_Binding[BACNET_BINDING_STRING_LENGTH];
        char SC_Hub_Function_Binding_dirty[BACNET_BINDING_STRING_LENGTH];
#ifdef BACNET_SC_STATUS_SUPPORT
        BACNET_SC_HUB_FUNCTION_CONNECTION SC_Hub_Function_Connection_Status;
#endif
        uint16_t Hub_Server_Port;
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
        bool SC_Direct_Connect_Initiate_Enable;
        bool SC_Direct_Connect_Initiate_Enable_dirty;
        bool SC_Direct_Connect_Accept_Enable;
        bool SC_Direct_Connect_Accept_Enable_dirty;
        char SC_Direct_Connect_Accept_URIs[
            BACNET_SC_DIRECT_ACCEPT_URI_MAX * (BACNET_URI_LENGTH + 1)];
        char SC_Direct_Connect_Accept_URIs_dirty[
            BACNET_SC_DIRECT_ACCEPT_URI_MAX * (BACNET_URI_LENGTH + 1)];
        char SC_Direct_Connect_Binding[BACNET_BINDING_STRING_LENGTH];
        char SC_Direct_Connect_Binding_dirty[BACNET_BINDING_STRING_LENGTH];
#ifdef BACNET_SC_STATUS_SUPPORT
        BACNET_SC_DIRECT_CONNECTION SC_Direct_Connect_Connection_Status;
#endif
        uint16_t Direct_Server_Port;
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */
#ifdef BACNET_SC_STATUS_SUPPORT
        OS_Keylist SC_Failed_Connection_Requests;
#endif
        uint32_t Certificate_Key_File;
        BACNET_UUID Local_UUID;
    } BACNET_SC_PARAMS;


    BACNET_SC_PARAMS *Network_Port_SC_Params(
        uint32_t object_instance);
    void Network_Port_SC_Pending_Params_Apply(
        uint32_t object_instance);
    void Network_Port_SC_Pending_Params_Discard(
        uint32_t object_instance);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NETPORT_INTERNAL_H */
