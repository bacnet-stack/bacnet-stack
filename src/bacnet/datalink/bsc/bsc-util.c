/**
 * @file
 * @brief module for common function for BACnet/SC implementation
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date Jule 2022
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdlib.h>
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/object/sc_netport.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bsc-util.h"

#define PRINTF debug_printf_stdout
#define PRINTF_ERR debug_printf_stderr

/**
 * @brief Map websocket return code to BACnet/SC return code
 * @param ret - websocket return code
 * @return BACnet/SC return code
 */
BSC_SC_RET bsc_map_websocket_retcode(BSC_WEBSOCKET_RET ret)
{
    switch (ret) {
        case BSC_WEBSOCKET_SUCCESS:
            return BSC_SC_SUCCESS;
        case BSC_WEBSOCKET_NO_RESOURCES:
            return BSC_SC_NO_RESOURCES;
        case BSC_WEBSOCKET_BAD_PARAM:
            return BSC_SC_BAD_PARAM;
        case BSC_WEBSOCKET_INVALID_OPERATION:
        default:
            return BSC_SC_INVALID_OPERATION;
    }
}

/**
 * @brief Copy BACnet Secure Connect VMAC address
 * @param dst - destination VMAC address
 * @param src - source VMAC address
 */
void bsc_copy_vmac(BACNET_SC_VMAC_ADDRESS *dst, BACNET_SC_VMAC_ADDRESS *src)
{
    memcpy(dst->address, src->address, sizeof(src->address));
}

/**
 * @brief Copy the BACnet Secure Connect UUID
 * @param dst - destination UUID
 * @param src - source UUID
 */
void bsc_copy_uuid(BACNET_SC_UUID *dst, BACNET_SC_UUID *src)
{
    memcpy(dst->uuid, src->uuid, sizeof(src->uuid));
}

/**
 * @brief Convert BACnet Secure Connect VMAC address to string
 * @param vmac - VMAC address
 * @return string representation of VMAC address
 */
char *bsc_vmac_to_string(BACNET_SC_VMAC_ADDRESS *vmac)
{
    static char buf[128];
    snprintf(
        buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x", vmac->address[0],
        vmac->address[1], vmac->address[2], vmac->address[3], vmac->address[4],
        vmac->address[5]);
    return buf;
}

/**
 * @brief Convert BACnet Secure Connect UUID to string
 * @param uuid - UUID
 * @return string representation of UUID
 */
char *bsc_uuid_to_string(BACNET_SC_UUID *uuid)
{
    static char buf[128];
    snprintf(
        buf, sizeof(buf),
        "%02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x",
        uuid->uuid[0], uuid->uuid[1], uuid->uuid[2], uuid->uuid[3],
        uuid->uuid[4], uuid->uuid[5], uuid->uuid[6], uuid->uuid[7],
        uuid->uuid[8], uuid->uuid[9], uuid->uuid[10], uuid->uuid[11],
        uuid->uuid[12], uuid->uuid[13], uuid->uuid[14], uuid->uuid[15]);
    return buf;
}

/*
 * bsc_node_load_cert_bacfile loads one credentional file from bacfile object
 * Note: the function adds null-terminated byte to loaded file
 * (certificate, certificate key).
 * The MbedTLS PEM parser requires data to be null-terminated.
 */
#ifdef CONFIG_MBEDTLS
#define ZERO_BYTE 1
#else
#define ZERO_BYTE 0
#endif

/**
 * @brief Load certificate from BACnet file object
 * @param file_instance - file instance
 * @param pbuf - pointer to the buffer
 * @param psize - pointer to the size
 * @return true if successful, else false
 */
static bool bsc_node_load_cert_bacfile(
    uint32_t file_instance, uint8_t **pbuf, size_t *psize)
{
    uint32_t file_length;
    uint8_t *buf;

    *psize = bacfile_file_size(file_instance) + ZERO_BYTE;
    if (*psize == 0) {
        return false;
    }

    buf = calloc(1, *psize);
    if (buf == NULL) {
        return false;
    }

    file_length =
        bacfile_read(file_instance, buf, (uint32_t)(*psize - ZERO_BYTE));
#ifdef CONFIG_MBEDTLS
    buf[*psize - 1] = 0;
#endif
    if (file_length == 0) {
        PRINTF_ERR("Can't read %s file\n", bacfile_pathname(file_instance));
        free(buf);
        return false;
    }
    *pbuf = buf;
    return true;
}

/**
 * @brief Fill BACnet/SC node configuration from network port object
 * @param bsc_conf - pointer to the BACnet/SC node configuration
 * @param event_func - event function
 * @return true if successful, else false
 */
bool bsc_node_conf_fill_from_netport(
    BSC_NODE_CONF *bsc_conf, BSC_NODE_EVENT_FUNC event_func)
{
    uint32_t instance;
    uint32_t file_instance;

    instance = Network_Port_Index_To_Instance(0);
    bsc_conf->ca_cert_chain = NULL;
    bsc_conf->cert_chain = NULL;
    bsc_conf->key = NULL;

    file_instance = Network_Port_Issuer_Certificate_File(instance, 0);
    if (!bsc_node_load_cert_bacfile(
            file_instance, &bsc_conf->ca_cert_chain,
            &bsc_conf->ca_cert_chain_size)) {
        bsc_node_conf_cleanup(bsc_conf);
        return false;
    }

    file_instance = Network_Port_Operational_Certificate_File(instance);
    if (!bsc_node_load_cert_bacfile(
            file_instance, &bsc_conf->cert_chain, &bsc_conf->cert_chain_size)) {
        bsc_node_conf_cleanup(bsc_conf);
        return false;
    }

    file_instance = Network_Port_Certificate_Key_File(instance);
    bsc_conf->key_size = bacfile_file_size(file_instance) + 1;
    if (!bsc_node_load_cert_bacfile(
            file_instance, &bsc_conf->key, &bsc_conf->key_size)) {
        bsc_node_conf_cleanup(bsc_conf);
        return false;
    }
    bsc_conf->local_uuid =
        (BACNET_SC_UUID *)Network_Port_SC_Local_UUID(instance);
    Network_Port_MAC_Address_Value(
        instance, bsc_conf->local_vmac.address,
        sizeof(bsc_conf->local_vmac.address));
    bsc_conf->max_local_bvlc_len =
        (uint16_t)Network_Port_Max_BVLC_Length_Accepted(instance);
    bsc_conf->max_local_npdu_len =
        (uint16_t)Network_Port_Max_NPDU_Length_Accepted(instance);
    bsc_conf->connect_timeout_s =
        (uint16_t)Network_Port_SC_Connect_Wait_Timeout(instance);
    bsc_conf->heartbeat_timeout_s =
        (uint16_t)Network_Port_SC_Heartbeat_Timeout(instance);
    bsc_conf->disconnect_timeout_s =
        (uint16_t)Network_Port_SC_Disconnect_Wait_Timeout(instance);
    bsc_conf->reconnnect_timeout_s =
        (uint16_t)Network_Port_SC_Maximum_Reconnect_Time(instance);
    bsc_conf->address_resolution_timeout_s = bsc_conf->connect_timeout_s;
    bsc_conf->address_resolution_freshness_timeout_s =
        bsc_conf->connect_timeout_s;
    bsc_conf->primaryURL =
        (char *)Network_Port_SC_Primary_Hub_URI_char(instance);
    bsc_conf->failoverURL =
        (char *)Network_Port_SC_Failover_Hub_URI_char(instance);
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    bsc_conf->direct_connect_initiate_enable =
        Network_Port_SC_Direct_Connect_Initiate_Enable(instance);
    bsc_conf->direct_connect_accept_enable =
        Network_Port_SC_Direct_Connect_Accept_Enable(instance);
    Network_Port_SC_Direct_Connect_Binding_get(
        instance, &bsc_conf->direct_server_port, &bsc_conf->direct_iface);
#endif
#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    Network_Port_SC_Hub_Function_Binding_get(
        instance, &bsc_conf->hub_server_port, &bsc_conf->hub_iface);
    bsc_conf->hub_function_enabled =
        Network_Port_SC_Hub_Function_Enable(instance);
#endif

#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    bsc_conf->direct_connection_accept_uris =
        Network_Port_SC_Direct_Connect_Accept_URIs_char(instance);
    bsc_conf->direct_connection_accept_uris_len =
        strlen(bsc_conf->direct_connection_accept_uris);
#endif
    bsc_conf->event_func = event_func;
    return true;
}

/**
 * @brief Cleanup BACnet/SC node configuration
 * @param bsc_conf - pointer to the BACnet/SC node configuration
 */
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

/**
 * @brief Copy string
 * @param dst - destination string
 * @param src - source string
 * @param dst_len - destination string length
 */
void bsc_copy_str(char *dst, const char *src, size_t dst_len)
{
    size_t len;
    if (dst_len > 0) {
        len = strlen(src) >= dst_len ? dst_len - 1 : strlen(src);
        memcpy(dst, src, len);
        dst[len] = 0;
    }
}

/**
 * @brief Set timestamp
 * @param timestamp - pointer to the timestamp
 */
void bsc_set_timestamp(BACNET_DATE_TIME *timestamp)
{
    int16_t utc_offset_minutes;
    bool dst_active;
    datetime_local(
        &timestamp->date, &timestamp->time, &utc_offset_minutes, &dst_active);
}

/**
 * @brief Check if BACnet/SC certificate files exist
 * @return true if all files exist, else false
 */
bool bsc_cert_files_check(uint32_t netport_instance)
{
    uint32_t file_instance;

    file_instance = Network_Port_Issuer_Certificate_File(netport_instance, 0);
    if (bacfile_file_size(file_instance) == 0) {
        PRINTF_ERR(
            "Issuer Certificate file %u size=0. Path=%s\n", file_instance,
            bacfile_pathname(file_instance));
        return false;
    }

    file_instance = Network_Port_Operational_Certificate_File(netport_instance);
    if (bacfile_file_size(file_instance) == 0) {
        PRINTF_ERR(
            "Operational Certificate file %u size=0. Path=%s\n", file_instance,
            bacfile_pathname(file_instance));
        PRINTF_ERR("Certificate file not exist\n");
        return false;
    }

    file_instance = Network_Port_Certificate_Key_File(netport_instance);
    if (bacfile_file_size(file_instance) == 0) {
        PRINTF_ERR(
            "Certificate Key file %u size=0. Path=%s\n", file_instance,
            bacfile_pathname(file_instance));
        return false;
    }

    return true;
}

/**
 * @brief Return a return code string for human readable log messages
 * @param ret BACnet/SC return codes
 * @return return code string for human readable log messages
 */
const char *bsc_return_code_to_string(BSC_SC_RET ret)
{
    switch (ret) {
        case BSC_SC_SUCCESS:
            return "SUCCESS";
        case BSC_SC_NO_RESOURCES:
            return "NO_RESOURCES";
        case BSC_SC_BAD_PARAM:
            return "BAD_PARAM";
        case BSC_SC_INVALID_OPERATION:
            return "INVALID_OPERATION";
        default:
            break;
    }

    return "UNKNOWN";
}

/**
 * @brief Return a string for human readable log messages
 * @param ev BACnet/SC return codes
 * @return return code string for human readable log messages
 */
const char *bsc_socket_event_to_string(BSC_SOCKET_EVENT ev)
{
    switch (ev) {
        case BSC_SOCKET_EVENT_CONNECTED:
            return "CONNECTED";
        case BSC_SOCKET_EVENT_DISCONNECTED:
            return "DISCONNECTED";
        case BSC_SOCKET_EVENT_RECEIVED:
            return "RECEIVED";
        default:
            break;
    }

    return "UNKNOWN";
}

/**
 * @brief Return a string for human readable log messages
 * @param ev BACnet/SC return codes
 * @return return code string for human readable log messages
 */
const char *bsc_socket_state_to_string(BSC_SOCKET_STATE state)
{
    switch (state) {
        case BSC_SOCK_STATE_IDLE:
            return "IDLE";
        case BSC_SOCK_STATE_AWAITING_WEBSOCKET:
            return "AWAITING_WEBSOCKET";
        case BSC_SOCK_STATE_AWAITING_REQUEST:
            return "AWAITING_REQUEST";
        case BSC_SOCK_STATE_AWAITING_ACCEPT:
            return "AWAITING_ACCEPT";
        case BSC_SOCK_STATE_CONNECTED:
            return "CONNECTED";
        case BSC_SOCK_STATE_DISCONNECTING:
            return "DISCONNECTING";
        case BSC_SOCK_STATE_ERROR:
            return "ERROR";
        case BSC_SOCK_STATE_ERROR_FLUSH_TX:
            return "ERROR_FLUSH_TX";
        default:
            break;
    }

    return "UNKNOWN";
}

/**
 * @brief Return a string for human readable log messages
 * @param ev BACnet/SC return codes
 * @return return code string for human readable log messages
 */
const char *bsc_websocket_return_to_string(BSC_WEBSOCKET_RET ret)
{
    switch (ret) {
        case BSC_WEBSOCKET_SUCCESS:
            return "SUCCESS";
        case BSC_WEBSOCKET_NO_RESOURCES:
            return "NO_RESOURCES";
        case BSC_WEBSOCKET_BAD_PARAM:
            return "BAD_PARAM";
        case BSC_WEBSOCKET_INVALID_OPERATION:
            return "INVALID_OPERATION";
        default:
            break;
    }

    return "UNKNOWN";
}

/**
 * @brief Return a string for human readable log messages
 * @param ev BACnet/SC return codes
 * @return return code string for human readable log messages
 */
const char *bsc_websocket_event_to_string(BSC_WEBSOCKET_EVENT event)
{
    switch (event) {
        case BSC_WEBSOCKET_CONNECTED:
            return "CONNECTED";
        case BSC_WEBSOCKET_DISCONNECTED:
            return "DISCONNECTED";
        case BSC_WEBSOCKET_RECEIVED:
            return "RECEIVED";
        case BSC_WEBSOCKET_SENDABLE:
            return "SENDABLE";
        case BSC_WEBSOCKET_SERVER_STARTED:
            return "SERVER_STARTED";
        case BSC_WEBSOCKET_SERVER_STOPPED:
            return "SERVER_STOPPED";
        default:
            break;
    }

    return "UNKNOWN";
}

/**
 * @brief Return a string for human readable log messages
 * @param ev BACnet/SC return codes
 * @return return code string for human readable log messages
 */
const char *bsc_bvlc_message_type_to_string(BVLC_SC_MESSAGE_TYPE message)
{
    switch (message) {
        case BVLC_SC_RESULT:
            return "RESULT";
        case BVLC_SC_ENCAPSULATED_NPDU:
            return "ENCAPSULATED_NPDU";
        case BVLC_SC_ADDRESS_RESOLUTION:
            return "ADDRESS_RESOLUTION";
        case BVLC_SC_ADDRESS_RESOLUTION_ACK:
            return "ADDRESS_RESOLUTION_ACK";
        case BVLC_SC_ADVERTISIMENT:
            return "ADVERTISIMENT";
        case BVLC_SC_ADVERTISIMENT_SOLICITATION:
            return "ADVERTISIMENT_SOLICITATION";
        case BVLC_SC_CONNECT_REQUEST:
            return "CONNECT_REQUEST";
        case BVLC_SC_CONNECT_ACCEPT:
            return "CONNECT_ACCEPT";
        case BVLC_SC_DISCONNECT_REQUEST:
            return "DISCONNECT_REQUEST";
        case BVLC_SC_DISCONNECT_ACK:
            return "DISCONNECT_ACK";
        case BVLC_SC_HEARTBEAT_REQUEST:
            return "HEARTBEAT_REQUEST";
        case BVLC_SC_HEARTBEAT_ACK:
            return "HEARTBEAT_ACK";
        case BVLC_SC_PROPRIETARY_MESSAGE:
            return "PROPRIETARY_MESSAGE";
        default:
            break;
    }

    return "UNKNOWN";
}

/**
 * @brief Return a string for human readable log messages
 * @param ev BACnet/SC return codes
 * @return return code string for human readable log messages
 */
const char *bsc_context_state_to_string(BSC_CTX_STATE state)
{
    switch (state) {
        case BSC_CTX_STATE_IDLE:
            return "IDLE";
        case BSC_CTX_STATE_INITIALIZING:
            return "INITIALIZING";
        case BSC_CTX_STATE_INITIALIZED:
            return "INITIALIZED";
        case BSC_CTX_STATE_DEINITIALIZING:
            return "DEINITIALIZING";
        default:
            break;
    }

    return "UNKNOWN";
}
