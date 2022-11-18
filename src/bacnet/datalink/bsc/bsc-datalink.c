/**
 * @file
 * @brief BACNet secure connect hub function API.
 * @author Kirill Neznamov
 * @date October 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#include "bacnet/basic/sys/debug.h"
#include <bacnet/basic/sys/fifo.h>
#include "bacnet/datalink/bsc/bsc-conf.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-mutex.h"
#include "bacnet/datalink/bsc/bsc-event.h"
#include "bacnet/datalink/bsc/bsc-runloop.h"
#include "bacnet/datalink/bsc/bsc-hub-function.h"
#include "bacnet/datalink/bsc/bsc-node.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/object/sc_netport.h"
#include "bacnet/basic/object/bacfile.h"

#define BSC_NEXT_POWER_OF_TWO1(v) \
    ((((unsigned int)v) - 1) | ((((unsigned int)v) - 1) >> 1))
#define BSC_NEXT_POWER_OF_TWO2(v) \
    (BSC_NEXT_POWER_OF_TWO1(v) | BSC_NEXT_POWER_OF_TWO1(v) >> 2)
#define BSC_NEXT_POWER_OF_TWO3(v) \
    (BSC_NEXT_POWER_OF_TWO2(v) | BSC_NEXT_POWER_OF_TWO2(v) >> 4)
#define BSC_NEXT_POWER_OF_TWO4(v) \
    (BSC_NEXT_POWER_OF_TWO3(v) | BSC_NEXT_POWER_OF_TWO3(v) >> 8)
#define BSC_NEXT_POWER_OF_TWO(v) \
    ((BSC_NEXT_POWER_OF_TWO4(v) | BSC_NEXT_POWER_OF_TWO4(v) >> 16)) + 1

static FIFO_BUFFER bsc_fifo = { 0 };
static uint8_t bsc_fifo_buf[BSC_NEXT_POWER_OF_TWO(BVLC_SC_NPDU_SIZE_CONF * 10)];
static BSC_NODE *bsc_node = NULL;
static BSC_NODE_CONF bsc_conf;
static BSC_EVENT *bsc_event = NULL;
static bool bsc_datalink_initialized = false;

static void bsc_node_event(
    BSC_NODE *node, BSC_NODE_EVENT ev, BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *pdu, uint16_t pdu_len)
{
    bsc_global_mutex_lock();

    if (ev == BSC_NODE_EVENT_STARTED || ev == BSC_NODE_EVENT_STOPPED) {
        bsc_event_signal(bsc_event);
    } else if (ev == BSC_NODE_EVENT_RECEIVED_NPDU) {
        if (bsc_datalink_initialized) {
            if (FIFO_Available(&bsc_fifo, pdu_len + sizeof(pdu_len))) {
                FIFO_Add(&bsc_fifo, (uint8_t *)&pdu_len, sizeof(pdu_len));
                FIFO_Add(&bsc_fifo, pdu, pdu_len);
            } else {
                debug_printf("pdu of size %d\n is dropped\n", pdu_len);
            }
        }
    }
    bsc_global_mutex_unlock();
}

void bsc_init_conf(uint32_t instance)
{
    uint32_t file_instance;
    static BACNET_SC_VMAC_ADDRESS vmac;
    BACNET_OCTET_STRING mac_address;
    BACNET_CHARACTER_STRING str;
    uint32_t offset;
    uint32_t i;

    bsc_conf.object_instance = instance;

    file_instance = Network_Port_Issuer_Certificate_File(instance, 0);
    bsc_conf.ca_cert_chain_size = bacfile_file_size(file_instance);
    bsc_conf.ca_cert_chain = bacfile_instance_memory_context(file_instance,
        NULL, 0);

    file_instance = Network_Port_Operational_Certificate_File(instance);
    bsc_conf.cert_chain_size = bacfile_file_size(file_instance);
    bsc_conf.cert_chain = bacfile_instance_memory_context(file_instance,
        NULL, 0);

    file_instance = Network_Port_Certificate_Key_File(instance);
    bsc_conf.key_size = bacfile_file_size(file_instance);
    bsc_conf.key = bacfile_instance_memory_context(file_instance, NULL, 0);

    bsc_conf.local_uuid = (BACNET_SC_UUID*)Network_Port_SC_Local_UUID(instance);

    memset(&bsc_conf.local_vmac, 0, sizeof(bsc_conf.local_vmac));
    if (Network_Port_MAC_Address(instance, &mac_address)) {
        memcpy(bsc_conf.local_vmac.address, mac_address.value, mac_address.length);
    }

    bsc_conf.max_local_bvlc_len =
        Network_Port_Max_BVLC_Length_Accepted(instance);
    bsc_conf.max_local_npdu_len =
        Network_Port_Max_NPDU_Length_Accepted(instance);
    bsc_conf.connect_timeout_s = Network_Port_SC_Connect_Wait_Timeout(instance);
    bsc_conf.heartbeat_timeout_s = Network_Port_SC_Heartbeat_Timeout(instance);
    bsc_conf.disconnect_timeout_s =
        Network_Port_SC_Disconnect_Wait_Timeout(instance);
    bsc_conf.reconnnect_timeout_s =
        Network_Port_SC_Maximum_Reconnect_Time(instance);
    bsc_conf.address_resolution_timeout_s = bsc_conf.connect_timeout_s;
    bsc_conf.address_resolution_freshness_timeout_s =
        bsc_conf.connect_timeout_s;
    bsc_conf.primaryURL = (char*)Network_Port_SC_Primary_Hub_URI_char(instance);
    bsc_conf.failoverURL =
        (char*)Network_Port_SC_Failover_Hub_URI_char(instance);
#if BSC_CONF_HUB_CONNECTORS_NUM==1
    bsc_conf.direct_server_port = Network_Port_SC_Direct_Server_Port(instance);
    bsc_conf.initiate_enabled =
        Network_Port_SC_Direct_Connect_Initiate_Enable(instance);
    bsc_conf.accept_enabled =
        Network_Port_SC_Direct_Connect_Accept_Enable(instance);
    bsc_conf.iface =
        (char*)Network_Port_SC_Direct_Connect_Binding_char(instance);
#endif
#if BSC_CONF_HUB_FUNCTIONS_NUM==1
    bsc_conf.hub_server_port = Network_Port_SC_Hub_Server_Port(instance);
    bsc_conf.hub_function_enabled =
        Network_Port_SC_Hub_Function_Enable(instance);
    bsc_conf.iface = (char*)Network_Port_SC_Hub_Function_Binding_char(instance);
#endif

#if BSC_CONF_HUB_CONNECTORS_NUM==1
    offset = 0;
    for (i = 0; i < BACNET_SC_DIRECT_ACCEPT_URI_MAX; i++) {
        if (Network_Port_SC_Direct_Connect_Accept_URI(instance, i, &str)){
            if (offset != 0)
                bsc_conf.direct_connection_accept_uris[offset++] = ' ';
            memcpy(&bsc_conf.direct_connection_accept_uris[offset], str.value,
                str.length);
            offset += str.length;
        }
    }
    bsc_conf.direct_connection_accept_uris[offset] = 0;
    bsc_conf.direct_connection_accept_uris_len = offset;
#endif
    bsc_conf.event_func = bsc_node_event;
}

BACNET_STACK_EXPORT
bool bsc_init(char *ifname)
{
    BSC_SC_RET r;
    bool ret = false;
    debug_printf("bsc_init() >>>\n");
    debug_printf("bsc_init() BACNET/SC datalink configured to use input fifo "
                 "of size %d\n",
        sizeof(bsc_fifo_buf));

    bsc_global_mutex_lock();
    if (!bsc_node && !bsc_datalink_initialized) {
        bsc_event = bsc_event_init();
        if (bsc_event) {
            FIFO_Init(&bsc_fifo, bsc_fifo_buf, sizeof(bsc_fifo_buf));
            // TODO: implement integration with BACNET/SC properties

            r = bsc_node_init(&bsc_conf, &bsc_node);
            if (r != BSC_SC_SUCCESS) {
                bsc_event_deinit(bsc_event);
                bsc_event = NULL;
            } else {
                r = bsc_node_start(bsc_node);
                if (r == BSC_SC_SUCCESS) {
                    ret = true;
                    bsc_global_mutex_unlock();
                    bsc_event_wait(bsc_event);
                    bsc_global_mutex_lock();
                    bsc_datalink_initialized = true;
                } else {
                    bsc_node_deinit(bsc_node);
                    bsc_event_deinit(bsc_event);
                    bsc_event = NULL;
                    bsc_node = NULL;
                }
            }
        }
    }
    bsc_global_mutex_unlock();
    debug_printf("bsc_init() <<< ret = %d\n", ret);
    return ret;
}

BACNET_STACK_EXPORT
void bsc_cleanup(void)
{
    debug_printf("bsc_init() >>>\n");
    bsc_global_mutex_lock();
    if (bsc_node) {
        if (!bsc_datalink_initialized) {
            bsc_global_mutex_unlock();
            bsc_event_wait(bsc_event);
            bsc_global_mutex_lock();
        }
        bsc_node_stop(bsc_node);
        bsc_global_mutex_unlock();
        bsc_event_wait(bsc_event);
        bsc_global_mutex_lock();
        bsc_datalink_initialized = false;
        bsc_node_deinit(bsc_node);
        bsc_event_deinit(bsc_event);
        bsc_event = NULL;
        bsc_node = NULL;
    }
    bsc_global_mutex_unlock();
    debug_printf("bsc_init() <<<\n");
}

BACNET_STACK_EXPORT
int bsc_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    (void)npdu_data;
    BSC_SC_RET ret;
    BACNET_SC_VMAC_ADDRESS dest_vmac = { 0xFF };
    int len = -1;
    static uint8_t buf[BVLC_SC_NPDU_SIZE_CONF];

    bsc_global_mutex_lock();

    if (dest->net == BACNET_BROADCAST_NETWORK || dest->mac_len == 0) {
        // broadcast message
        memset(&dest_vmac.address[0], 0xFF, BVLC_SC_VMAC_SIZE);
    } else if (dest->mac_len == 6) {
        // unicast
        memcpy(&dest_vmac.address[0], &dest->mac[0], BVLC_SC_VMAC_SIZE);
    } else {
        bsc_global_mutex_unlock();
        debug_printf(
            "bsc_send_pdu() <<< ret = -1, incorrect dest mac address\n");
        return len;
    }

    len = bvlc_sc_encode_encapsulated_npdu(buf, sizeof(buf),
        bsc_get_next_message_id(), NULL, &dest_vmac, pdu, pdu_len);

    ret = bsc_node_send(bsc_node, buf, len);

    if (ret != BSC_SC_SUCCESS) {
        len = -1;
    }

    bsc_global_mutex_unlock();
    debug_printf("bsc_send_pdu() <<< ret = %d\n", len);
    return len;
}

BACNET_STACK_EXPORT
uint16_t bsc_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    uint16_t pdu_len = 0;
    BVLC_SC_DECODED_MESSAGE dm;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    const char *err_desc = NULL;

    // TODO: implement timeout

    debug_printf("bsc_receive() >>>\n");
    bsc_global_mutex_lock();
    if (FIFO_Available(&bsc_fifo, sizeof(uint16_t) + 1)) {
        FIFO_Pull(&bsc_fifo, (uint8_t *)&pdu_len, sizeof(pdu_len));
        if (max_pdu < pdu_len) {
            debug_printf("bsc_receive() pdu of size %d is dropped\n", pdu_len);
            pdu_len = 0;
        } else {
            FIFO_Pull(&bsc_fifo, pdu, pdu_len);
            if (!bvlc_sc_decode_message(
                    pdu, pdu_len, &dm, &error, &class, &err_desc)) {
                debug_printf("bsc_receive() pdu of size %d is dropped because "
                             "of err = %d, class %d, desc = %s\n",
                    pdu_len, error, class, err_desc);
                pdu_len = 0;
            } else {
                src->mac_len = BVLC_SC_VMAC_SIZE;
                if (dm.hdr.origin) {
                    memcpy(src->mac, dm.hdr.origin, BVLC_SC_VMAC_SIZE);
                } else {
                    debug_printf("bsc_receive() pdu of size %d is dropped "
                                 "because origin addr is absent\n",
                        pdu_len);
                    pdu_len = 0;
                }
            }
        }
    }
    bsc_global_mutex_unlock();
    debug_printf("bsc_receive() <<< ret = %d\n", pdu_len);
    return pdu_len;
}

#if CONFIG_ZTEST==1
const BSC_NODE_CONF *bsc_conf_get(void)
{
    return &bsc_conf;
}
#endif
