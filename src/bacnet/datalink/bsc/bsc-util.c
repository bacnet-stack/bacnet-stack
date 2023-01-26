/**
 * @file
 * @brief module for common function for BACnet/SC implementation
 * @author Kirill Neznamov
 * @date Jule 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/object/sc_netport.h"
#include "bacnet/basic/object/bacfile.h"
#include <stdlib.h>

BSC_SC_RET bsc_map_websocket_retcode(BSC_WEBSOCKET_RET ret)
{
    switch(ret) {
        case BSC_WEBSOCKET_SUCCESS:
            return BSC_SC_SUCCESS;
        case BSC_WEBSOCKET_NO_RESOURCES:
            return BSC_SC_NO_RESOURCES;
        case BSC_WEBSOCKET_BAD_PARAM:
            return BSC_SC_BAD_PARAM;
        case BSC_WEBSOCKET_INVALID_OPERATION:
            return BSC_SC_INVALID_OPERATION;
        default:
            return BSC_SC_INVALID_OPERATION;
    }
}

void bsc_copy_vmac(BACNET_SC_VMAC_ADDRESS *dst, BACNET_SC_VMAC_ADDRESS *src)
{
   memcpy(dst->address, src->address, sizeof(src->address));
}

void bsc_copy_uuid(BACNET_SC_UUID *dst, BACNET_SC_UUID *src)
{
   memcpy(dst->uuid, src->uuid, sizeof(src->uuid));
}

char *bsc_vmac_to_string(BACNET_SC_VMAC_ADDRESS *vmac)
{
    static char buf[128];
    sprintf(buf, "%02x%02x%02x%02x%02x%02x", vmac->address[0], vmac->address[1],
        vmac->address[2], vmac->address[3], vmac->address[4], vmac->address[5]);
    return buf;
}

char *bsc_uuid_to_string(BACNET_SC_UUID *uuid)
{
    static char buf[128];
    sprintf(buf,
        "%02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x",
        uuid->uuid[0], uuid->uuid[1], uuid->uuid[2], uuid->uuid[3],
        uuid->uuid[4], uuid->uuid[5], uuid->uuid[6], uuid->uuid[7],
        uuid->uuid[8], uuid->uuid[9], uuid->uuid[10], uuid->uuid[11],
        uuid->uuid[12], uuid->uuid[13], uuid->uuid[14], uuid->uuid[15]);
    return buf;
}

void bsc_generate_random_vmac(BACNET_SC_VMAC_ADDRESS *p)
{
    int i;

    for (i = 0; i < BVLC_SC_VMAC_SIZE; i++) {
        p->address[i] = rand() % 255;
        if (i == 0) {
            /* According H.7.3 EUI-48 and Random-48 VMAC Address: */
            /* The Random-48 VMAC is a 6-octet VMAC address in which the least */
            /* significant 4 bits (Bit 3 to Bit 0) in the first octet shall be */
            /* B'0010' (X'2'), and all other 44 bits are randomly selected to be */
            /* 0 or 1. */
            p->address[i] = (p->address[i] & 0xF0 ) | 0x02;
        }
    }
}

void bsc_generate_random_uuid(BACNET_SC_UUID *p)
{
    int i;

    for (i = 0; i < BVLC_SC_UUID_SIZE; i++) {
        p->uuid[i] = rand() % 255;
    }
}

bool bsc_node_conf_fill_from_netport(BSC_NODE_CONF *bsc_conf,
    BSC_NODE_EVENT_FUNC event_func)
{
    uint32_t instance;
    uint32_t file_instance;
    uint32_t file_length;

    instance = Network_Port_Index_To_Instance(0);

    file_instance = Network_Port_Issuer_Certificate_File(instance, 0);
    bsc_conf->ca_cert_chain_size = bacfile_file_size(file_instance);
    bsc_conf->ca_cert_chain = calloc(1, bsc_conf->ca_cert_chain_size);
    file_length = bacfile_read(file_instance,
        bsc_conf->ca_cert_chain, bsc_conf->ca_cert_chain_size);
    if (file_length == 0) {
        bsc_conf->ca_cert_chain_size = 0;
        return false;
    }

    file_instance = Network_Port_Operational_Certificate_File(instance);
    bsc_conf->cert_chain_size = bacfile_file_size(file_instance);
    bsc_conf->cert_chain = calloc(1, bsc_conf->cert_chain_size);
    file_length = bacfile_read(file_instance,
        bsc_conf->cert_chain, bsc_conf->cert_chain_size);
    if (file_length == 0) {
        free(bsc_conf->ca_cert_chain);
        bsc_conf->ca_cert_chain = NULL;
        bsc_conf->ca_cert_chain_size = 0;
        bsc_conf->cert_chain_size = 0;
        return false;
    }

    file_instance = Network_Port_Certificate_Key_File(instance);
    bsc_conf->key_size = bacfile_file_size(file_instance);
    bsc_conf->key = calloc(1, bsc_conf->key_size);
    file_length = bacfile_read(file_instance,
        bsc_conf->key, bsc_conf->key_size);
    if (file_length == 0) {
        free(bsc_conf->ca_cert_chain);
        free(bsc_conf->cert_chain);
        bsc_conf->ca_cert_chain = NULL;
        bsc_conf->cert_chain = NULL;
        bsc_conf->ca_cert_chain_size = 0;
        bsc_conf->cert_chain_size = 0;
        bsc_conf->key_size = 0;
        return false;
    }

    bsc_conf->local_uuid =
        (BACNET_SC_UUID*)Network_Port_SC_Local_UUID(instance);

    bsc_conf->local_vmac =
        (BACNET_SC_VMAC_ADDRESS *)Network_Port_MAC_Address_pointer(instance);
    bsc_conf->max_local_bvlc_len =
        Network_Port_Max_BVLC_Length_Accepted(instance);
    bsc_conf->max_local_npdu_len =
        Network_Port_Max_NPDU_Length_Accepted(instance);
    bsc_conf->connect_timeout_s =
        Network_Port_SC_Connect_Wait_Timeout(instance);
    bsc_conf->heartbeat_timeout_s = Network_Port_SC_Heartbeat_Timeout(instance);
    bsc_conf->disconnect_timeout_s =
        Network_Port_SC_Disconnect_Wait_Timeout(instance);
    bsc_conf->reconnnect_timeout_s =
        Network_Port_SC_Maximum_Reconnect_Time(instance);
    bsc_conf->address_resolution_timeout_s = bsc_conf->connect_timeout_s;
    bsc_conf->address_resolution_freshness_timeout_s =
        bsc_conf->connect_timeout_s;
    bsc_conf->primaryURL =
        (char*)Network_Port_SC_Primary_Hub_URI_char(instance);
    bsc_conf->failoverURL =
        (char*)Network_Port_SC_Failover_Hub_URI_char(instance);
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    bsc_conf->direct_connect_initiate_enable =
        Network_Port_SC_Direct_Connect_Initiate_Enable(instance);
    bsc_conf->direct_connect_accept_enable =
        Network_Port_SC_Direct_Connect_Accept_Enable(instance);
    Network_Port_SC_Direct_Connect_Binding_get(instance,
        &bsc_conf->direct_server_port, &bsc_conf->direct_iface);
#endif
#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
    Network_Port_SC_Hub_Function_Binding_get(instance,
        &bsc_conf->hub_server_port, &bsc_conf->hub_iface);
    bsc_conf->hub_function_enabled =
        Network_Port_SC_Hub_Function_Enable(instance);
#endif

#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    bsc_conf->direct_connection_accept_uris =
        Network_Port_SC_Direct_Connect_Accept_URIs_char(instance);
    bsc_conf->direct_connection_accept_uris_len =
        strlen(bsc_conf->direct_connection_accept_uris);
#endif
    bsc_conf->event_func = event_func;
    return true;
}

void bsc_node_conf_cleanup(BSC_NODE_CONF *bsc_conf)
{
    bsc_conf->ca_cert_chain_size = 0;
    if (bsc_conf->ca_cert_chain) {
        free(bsc_conf->ca_cert_chain);
    }
    bsc_conf->cert_chain_size = 0;
    if (bsc_conf->cert_chain) {
        free(bsc_conf->cert_chain);
    }
    bsc_conf->key_size = 0;
    if (bsc_conf->key) {
        free(bsc_conf->key);
    }
}

