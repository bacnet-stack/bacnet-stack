/**
 * @file
 * @brief BACnet secure connect hub function API.
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date October 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#include "bacnet/basic/sys/debug.h"
#include <bacnet/basic/sys/fifo.h>
#include "bacnet/datalink/bsc/bsc-conf.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-datalink.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-event.h"
#include "bacnet/datalink/bsc/bsc-hub-function.h"
#include "bacnet/datalink/bsc/bsc-node.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/object/sc_netport.h"
#include "bacnet/basic/object/bacfile.h"

#define PRINTF debug_printf_stderr
#undef DEBUG_PRINTF
#if DEBUG_BSC_DATALINK
#define DEBUG_PRINTF debug_printf
#else
#define DEBUG_PRINTF debug_printf_disabled
#endif

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

typedef enum {
    BSC_DATALINK_STATE_IDLE = 0,
    BSC_DATALINK_STATE_STARTING = 1,
    BSC_DATALINK_STATE_STARTED = 2,
    BSC_DATALINK_STATE_STOPPING = 3
} BSC_DATALINK_STATE;

static FIFO_BUFFER bsc_fifo = { 0 };
static uint8_t
    bsc_fifo_buf[BSC_NEXT_POWER_OF_TWO(BSC_CONF_DATALINK_RX_BUFFER_SIZE)];
static BSC_NODE *bsc_node = NULL;
static BSC_NODE_CONF bsc_conf;
static BSC_DATALINK_STATE bsc_datalink_state = BSC_DATALINK_STATE_IDLE;
static BSC_EVENT *bsc_event = NULL;
static BSC_EVENT *bsc_data_event = NULL;

/**
 * @brief bsc_node_event() is a callback function which is called by
 *  BACnet/SC datalink when some event occurs.
 *  The function is used to notify the upper layer about the events.
 * @param node - pointer to the BACnet/SC node
 * @param ev - event type
 * @param dest - pointer to the destination address
 * @param pdu - pointer to the received PDU
 * @param pdu_len - length of the received PDU
 */
static void bsc_node_event(
    BSC_NODE *node,
    BSC_NODE_EVENT ev,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *pdu,
    size_t pdu_len)
{
    uint16_t pdu16_len;
    DEBUG_PRINTF("bsc_node_event() >>> ev = %d\n", ev);
    bws_dispatch_lock();
    (void)node;
    (void)dest;
    if (ev == BSC_NODE_EVENT_STARTED || ev == BSC_NODE_EVENT_STOPPED) {
        if (bsc_datalink_state != BSC_DATALINK_STATE_IDLE) {
            bsc_event_signal(bsc_event);
        }
    } else if (ev == BSC_NODE_EVENT_RECEIVED_NPDU) {
        if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
            if (pdu_len <= USHRT_MAX &&
                FIFO_Available(
                    &bsc_fifo, (unsigned)(pdu_len + sizeof(pdu16_len)))) {
                pdu16_len = (uint16_t)pdu_len;
                FIFO_Add(&bsc_fifo, (uint8_t *)&pdu16_len, sizeof(pdu16_len));
                FIFO_Add(&bsc_fifo, pdu, pdu16_len);
                bsc_event_signal(bsc_data_event);
            }
#if DEBUG_ENABLED
            else {
                PRINTF("pdu of size %d\n is dropped\n", pdu_len);
            }
#endif
        }
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_event() <<<\n");
}

/**
 * @brief bsc_deinit_resources() is a function which is used to
 *  deinitialize all resources allocated by BACnet/SC datalink.
 */
static void bsc_deinit_resources(void)
{
    if (bsc_event) {
        bsc_event_deinit(bsc_event);
        bsc_event = NULL;
    }
    if (bsc_data_event) {
        bsc_event_deinit(bsc_data_event);
        bsc_data_event = NULL;
    }
}

/**
 * @brief Initialize the BACnet/SC datalink layer
 * @param ifname - name of the network interface
 * @return true if the initialization was successful, otherwise false
 */
bool bsc_init(char *ifname)
{
    BSC_SC_RET r;
    bool ret = false;
    DEBUG_PRINTF("bsc_init() >>>\n");

    (void)ifname;

    bws_dispatch_lock();

    if (bsc_datalink_state != BSC_DATALINK_STATE_IDLE) {
        bws_dispatch_unlock();
        PRINTF("bsc_init() <<< ret = %d\n", ret);
        return ret;
    }

    bsc_event = bsc_event_init();
    bsc_data_event = bsc_event_init();

    if (!bsc_event || !bsc_data_event) {
        bsc_deinit_resources();
        bws_dispatch_unlock();
        PRINTF("bsc_init() <<< ret = %d\n", false);
        return false;
    }

    DEBUG_PRINTF(
        "bsc_init() BACNET/SC datalink configured to use input fifo "
        "of size %d\n",
        sizeof(bsc_fifo_buf));
    FIFO_Init(&bsc_fifo, bsc_fifo_buf, sizeof(bsc_fifo_buf));

    ret = bsc_node_conf_fill_from_netport(&bsc_conf, &bsc_node_event);

    if (!ret) {
        bsc_deinit_resources();
        bws_dispatch_unlock();
        PRINTF("bsc_init() <<< configuration of BACNET/SC datalink "
               "failed, ret = false\n");
        return ret;
    }

    bsc_datalink_state = BSC_DATALINK_STATE_STARTING;
    r = bsc_node_init(&bsc_conf, &bsc_node);
    if (r == BSC_SC_SUCCESS) {
        r = bsc_node_start(bsc_node);
        if (r == BSC_SC_SUCCESS) {
            bws_dispatch_unlock();
            bsc_event_wait(bsc_event);
            bws_dispatch_lock();
            bsc_datalink_state = BSC_DATALINK_STATE_STARTED;
            bws_dispatch_unlock();
            DEBUG_PRINTF("bsc_init() <<< ret = %d\n", true);
            return true;
        }
    }
    bsc_deinit_resources();
    bsc_node_conf_cleanup(&bsc_conf);
    bsc_datalink_state = BSC_DATALINK_STATE_IDLE;
    bws_dispatch_unlock();
    PRINTF("bsc_init() <<< ret = %d\n", false);
    return false;
}

/**
 * @brief Blocking thread-safe bsc_cleanup() function
 *  de-initializes BACNet/SC datalink.
 */
void bsc_cleanup(void)
{
    DEBUG_PRINTF("bsc_cleanup() >>>\n");
    bws_dispatch_lock();
    if (bsc_datalink_state != BSC_DATALINK_STATE_IDLE &&
        bsc_datalink_state != BSC_DATALINK_STATE_STOPPING) {
        if (bsc_datalink_state == BSC_DATALINK_STATE_STARTING) {
            bws_dispatch_unlock();
            bsc_event_wait(bsc_event);
            bws_dispatch_lock();
        }
        if (bsc_datalink_state != BSC_DATALINK_STATE_STOPPING) {
            bsc_datalink_state = BSC_DATALINK_STATE_STOPPING;
            bsc_event_signal(bsc_data_event);
            bsc_node_stop(bsc_node);
            bws_dispatch_unlock();
            bsc_event_wait(bsc_event);
            bsc_event_wait(bsc_data_event);
            bws_dispatch_lock();
            bsc_deinit_resources();
            (void)bsc_node_deinit(bsc_node);
            bsc_node_conf_cleanup(&bsc_conf);
            bsc_node = NULL;
            bsc_datalink_state = BSC_DATALINK_STATE_IDLE;
        }
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_cleanup() <<<\n");
}

/**
 * @brief Send a BACnet/SC PDU to a remote node
 * @param dest - destination address
 * @param npdu_data - network layer data
 * @param pdu - PDU to send
 * @param pdu_len - length of the PDU
 * @return number of bytes sent on success, negative number on failure
 */
int bsc_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    BSC_SC_RET ret;
    BACNET_SC_VMAC_ADDRESS dest_vmac;
    int len = -1;
    static uint8_t buf[BVLC_SC_NPDU_SIZE_CONF];

    /* this datalink doesn't need to know the npdu data */
    (void)npdu_data;

    bws_dispatch_lock();

    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        if (dest->net == BACNET_BROADCAST_NETWORK || dest->mac_len == 0) {
            /* broadcast message */
            memset(&dest_vmac.address[0], 0xFF, BVLC_SC_VMAC_SIZE);
        } else if (dest->mac_len == BVLC_SC_VMAC_SIZE) {
            /* unicast */
            memcpy(&dest_vmac.address[0], &dest->mac[0], BVLC_SC_VMAC_SIZE);
        } else {
            bws_dispatch_unlock();
            PRINTF("bsc_send_pdu() <<< ret = -1, incorrect dest mac address\n");
            return len;
        }

        len = (int)bvlc_sc_encode_encapsulated_npdu(
            buf, sizeof(buf), bsc_get_next_message_id(), NULL, &dest_vmac, pdu,
            pdu_len);

        ret = bsc_node_send(bsc_node, buf, len);
        len = pdu_len;

        if (ret != BSC_SC_SUCCESS) {
            len = -1;
        }
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_send_pdu() <<< ret = %d\n", len);
    return len;
}

/**
 * @brief Remove a BACnet/SC packet from the FIFO buffer
 * @param packet_size - size of the packet to remove
 */
static void bsc_remove_packet(size_t packet_size)
{
    size_t i;
    for (i = 0; i < packet_size; i++) {
        (void)FIFO_Get(&bsc_fifo);
    }
}

/**
 * @brief Blocking thread-safe bsc_receive() function
 * receives NPDUs transferred over BACNet/SC
 * from a node specified by it's virtual MAC address as
 * defined in Clause AB.1.5.2.
 * @param src - source VMAC address
 * @param pdu - a buffer to hold the PDU portion of the received packet,
 * after the BVLC portion has been stripped off.
 * @param max_pdu - size of the pdu[] buffer
 * @param timeout_ms - the number of milliseconds to wait for a packet
 * @return the number of octets (remaining) in the PDU, or zero if no packet
 */
uint16_t bsc_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout_ms)
{
    uint16_t pdu_len = 0;
    uint16_t npdu16_len = 0;
    BVLC_SC_DECODED_MESSAGE dm;
    uint16_t error_code;
    uint16_t error_class;
    const char *err_desc = NULL;
    static uint8_t buf[BVLC_SC_NPDU_SIZE_CONF];

    bws_dispatch_lock();

    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        if (FIFO_Count(&bsc_fifo) <= sizeof(npdu16_len)) {
            bws_dispatch_unlock();
            bsc_event_timedwait(bsc_data_event, timeout_ms);
            bws_dispatch_lock();
        }

        if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED &&
            FIFO_Count(&bsc_fifo) > sizeof(npdu16_len)) {
            DEBUG_PRINTF("bsc_receive() processing data...\n");
            FIFO_Pull(&bsc_fifo, (uint8_t *)&npdu16_len, sizeof(npdu16_len));

            if (sizeof(buf) < npdu16_len) {
                PRINTF("bsc_receive() pdu of size %d is dropped\n", npdu16_len);
                bsc_remove_packet(npdu16_len);
            } else {
                FIFO_Pull(&bsc_fifo, buf, npdu16_len);
                if (!bvlc_sc_decode_message(
                        buf, npdu16_len, &dm, &error_code, &error_class,
                        &err_desc)) {
                    PRINTF(
                        "bsc_receive() pdu of size %d is dropped because "
                        "of err = %d, class %d, desc = %s\n",
                        npdu16_len, error_code, error_class, err_desc);
                    bsc_remove_packet(npdu16_len);
                } else {
                    if (dm.hdr.origin &&
                        max_pdu >= dm.payload.encapsulated_npdu.npdu_len) {
                        src->mac_len = BVLC_SC_VMAC_SIZE;
                        memcpy(
                            &src->mac[0], &dm.hdr.origin->address[0],
                            BVLC_SC_VMAC_SIZE);
                        memcpy(
                            pdu, dm.payload.encapsulated_npdu.npdu,
                            dm.payload.encapsulated_npdu.npdu_len);
                        pdu_len =
                            (uint16_t)dm.payload.encapsulated_npdu.npdu_len;
                    }
#if DEBUG_ENABLED
                    else {
                        PRINTF(
                            "bsc_receive() pdu of size %d is dropped "
                            "because origin addr is absent or output "
                            "buf of size %d is to small\n",
                            npdu16_len, max_pdu);
                    }
#endif
                }
            }
            DEBUG_PRINTF("bsc_receive() pdu_len = %d\n", pdu_len);
        }
    }
    bws_dispatch_unlock();

    return pdu_len;
}

/**
 * @brief Function can be used to retrieve broadcast
 * VMAC address for BACNet/SC node.
 * @param dest - value of broadcast VMAC address
 */
void bsc_get_broadcast_address(BACNET_ADDRESS *dest)
{
    if (dest) {
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->mac_len = BVLC_SC_VMAC_SIZE;
        memset(&dest->mac[0], 0xFF, BVLC_SC_VMAC_SIZE);
        /* no SADR */
        dest->len = 0;
        memset(dest->adr, 0, sizeof(dest->adr));
    }
}

/**
 * @brief Function can be used to retrieve local
 * VMAC address of initialized BACNet/SC datalink.
 * @param my_address - value of local VMAC address
 */
void bsc_get_my_address(BACNET_ADDRESS *my_address)
{
    if (my_address) {
        memset(my_address, 0, sizeof(*my_address));
    }

    bws_dispatch_lock();
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        my_address->mac_len = BVLC_SC_VMAC_SIZE;
        memcpy(
            &my_address->mac[0], &bsc_conf.local_vmac.address[0],
            BVLC_SC_VMAC_SIZE);
    }
    bws_dispatch_unlock();
}

/**
 * @brief Determine if the BACnet/SC direct connection is established
 * with a remote BACnet/SC node.
 * @param dest - BACnet/SC VMAC address of the remote node to check direct
 * connection status
 * @param urls - array representing the possible URIs of a remote node for
 * acceptance of direct connections. Can contain 1 element
 * @param urls_cnt - number of elements in the urls array
 * @return true if the connection is established, otherwise false
 */
bool bsc_direct_connection_established(
    BACNET_SC_VMAC_ADDRESS *dest, char **urls, size_t urls_cnt)
{
    bool ret = false;
    bws_dispatch_lock();
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        ret = bsc_node_direct_connection_established(
            bsc_node, dest, urls, urls_cnt);
    }
    bws_dispatch_unlock();
    return ret;
}

/**
 * @brief Start the process of establishing a direct BACnet/SC connection
 * to a node identified by either urls or dest parameter. The user should
 * note that if the dest parameter is used, the local node tries to resolve
 * it (e.g. to get URIs related to dest VMAC from all existing BACnet/SC
 * nodes in the network). As a result, the process of establishing a BACnet/SC
 * connection by dest may take an unpredictable amount of time depending on
 * the current network configuration.
 * @param dest - BACnet/SC VMAC address of the remote node to check direct
 * connection status
 * @param urls - array representing the possible URIs of a remote node for
 * acceptance of direct connections. Can contain 1 element
 * @param urls_cnt - number of elements in the urls array
 * @return BSC_SC_SUCCESS if the process of establishing a BACnet/SC
 * connection was started successfully, otherwise an error code
 */
BSC_SC_RET
bsc_connect_direct(BACNET_SC_VMAC_ADDRESS *dest, char **urls, size_t urls_cnt)
{
    BSC_SC_RET ret = BSC_SC_INVALID_OPERATION;
    DEBUG_PRINTF(
        "bsc_connect_direct() >>> dest = %p, urls = %p, urls_cnt = %d\n", dest,
        urls, urls_cnt);
    bws_dispatch_lock();
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        ret = bsc_node_connect_direct(bsc_node, dest, urls, urls_cnt);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_connect_direct() ret = %d\n", ret);
    return ret;
}

/**
 * @brief Disconnect a direct BACnet/SC connection with a remote node
 * identified by its VMAC address.
 * @param dest - BACnet/SC VMAC address of the remote node to disconnect
 * the direct connection with
 */
void bsc_disconnect_direct(BACNET_SC_VMAC_ADDRESS *dest)
{
    bws_dispatch_lock();
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        bsc_node_disconnect_direct(bsc_node, dest);
    }
    bws_dispatch_unlock();
}

/**
 * @brief Process the hub connector state
 */
static void bsc_update_hub_connector_state(void)
{
    BACNET_SC_HUB_CONNECTOR_STATE state;
    uint32_t instance;

    instance = Network_Port_Index_To_Instance(0);
    state = bsc_node_hub_connector_state(bsc_node);
    Network_Port_SC_Hub_Connector_State_Set(instance, state);
}

/**
 * @brief Process the hub connector status
 */
static void bsc_update_hub_connector_status(void)
{
    BACNET_SC_HUB_CONNECTION_STATUS *status;
    uint32_t instance;

    instance = Network_Port_Index_To_Instance(0);
    status = bsc_node_hub_connector_status(bsc_node, true);
    if (status) {
        Network_Port_SC_Primary_Hub_Connection_Status_Set(
            instance, status->State, &status->Connect_Timestamp,
            &status->Disconnect_Timestamp, status->Error,
            status->Error_Details[0] ? status->Error_Details : NULL);
    }
    status = bsc_node_hub_connector_status(bsc_node, false);
    if (status) {
        Network_Port_SC_Failover_Hub_Connection_Status_Set(
            instance, status->State, &status->Connect_Timestamp,
            &status->Disconnect_Timestamp, status->Error,
            status->Error_Details[0] ? status->Error_Details : NULL);
    }
}

/**
 * @brief Process the hub function status
 */
static void bsc_update_hub_function_status(void)
{
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *s;
    size_t cnt;
    int i;
    uint32_t instance = Network_Port_Index_To_Instance(0);
    BACNET_SC_VMAC_ADDRESS uninitialized = { 0 };

    s = bsc_node_hub_function_status(bsc_node, &cnt);
    if (s) {
        Network_Port_SC_Hub_Function_Connection_Status_Delete_All(instance);
        for (i = 0; i < cnt; i++) {
            if (memcmp(
                    &uninitialized.address[0], &s[i].Peer_VMAC[0],
                    BVLC_SC_VMAC_SIZE) != 0) {
                Network_Port_SC_Hub_Function_Connection_Status_Add(
                    instance, s[i].State, &s[i].Connect_Timestamp,
                    &s[i].Disconnect_Timestamp, &s[i].Peer_Address,
                    s[i].Peer_VMAC, s[i].Peer_UUID.uuid.uuid128, s[i].Error,
                    s[i].Error_Details[0] == 0 ? NULL : s[i].Error_Details);
            }
        }
    }
}

/**
 * @brief Add direct connection status to the network port
 * @param s - direct connection status
 * @param cnt - number of direct connection statuses
 */
static void bsc_add_direct_status_to_netport(
    BACNET_SC_DIRECT_CONNECTION_STATUS *s, size_t cnt)
{
    int i;
    uint32_t instance = Network_Port_Index_To_Instance(0);
    BACNET_SC_VMAC_ADDRESS uninitialized = { 0 };

    for (i = 0; i < cnt; i++) {
        if (memcmp(
                &uninitialized.address[0], &s[i].Peer_VMAC[0],
                BVLC_SC_VMAC_SIZE) != 0) {
            Network_Port_SC_Direct_Connect_Connection_Status_Add(
                instance, s[i].URI[0] == 0 ? NULL : s[i].URI, s[i].State,
                &s[i].Connect_Timestamp, &s[i].Disconnect_Timestamp,
                &s[i].Peer_Address, s[i].Peer_VMAC, s[i].Peer_UUID.uuid.uuid128,
                s[i].Error,
                s[i].Error_Details[0] == 0 ? NULL : s[i].Error_Details);
        }
    }
}

/**
 * @brief Process the direct connection status
 */
static void bsc_update_direct_connection_status(void)
{
    BACNET_SC_DIRECT_CONNECTION_STATUS *s = NULL;
    size_t cnt = 0;
    uint32_t instance = Network_Port_Index_To_Instance(0);

    s = bsc_node_direct_connection_status(bsc_node, &cnt);
    if (s) {
        Network_Port_SC_Direct_Connect_Connection_Status_Delete_All(instance);
        if (s && cnt) {
            bsc_add_direct_status_to_netport(s, cnt);
        }
    }
}

/**
 * @brief Process the failed requests
 */
static void bsc_update_failed_requests(void)
{
    BACNET_SC_FAILED_CONNECTION_REQUEST *r;
    size_t cnt;
    int i;

    uint32_t instance = Network_Port_Index_To_Instance(0);
    r = bsc_node_failed_requests_status(bsc_node, &cnt);
    if (r) {
        Network_Port_SC_Failed_Connection_Requests_Delete_All(instance);
        for (i = 0; i < cnt; i++) {
            if (r[i].Peer_Address.host[0] != 0) {
#if DEBUG_ENABLED
                DEBUG_PRINTF(
                    "failed request record %d, host %s, vmac %s, uuid "
                    "%s, error %d, details = %s\n",
                    i, r[i].Peer_Address.host,
                    bsc_vmac_to_string(
                        (BACNET_SC_VMAC_ADDRESS *)r[i].Peer_VMAC),
                    bsc_uuid_to_string(
                        (BACNET_SC_UUID *)r[i].Peer_UUID.uuid.uuid128),
                    r[i].Error, r[i].Error_Details);
#endif
                Network_Port_SC_Failed_Connection_Requests_Add(
                    instance, &r[i].Timestamp, &r[i].Peer_Address,
                    r[i].Peer_VMAC, r[i].Peer_UUID.uuid.uuid128, r[i].Error,
                    r[i].Error_Details[0] != 0 ? r[i].Error_Details : NULL);
            }
        }
    }
}

/**
 * @brief Update the network port properties
 */
static void bsc_update_netport_properties(void)
{
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        bsc_update_hub_connector_state();
        bsc_update_hub_connector_status();
        bsc_update_hub_function_status();
        bsc_update_direct_connection_status();
        bsc_update_failed_requests();
    }
}

/**
 * @brief Manage the BACnet/SC datalink timer
 * @param seconds - number of elapsed seconds
 */
void bsc_maintenance_timer(uint16_t seconds)
{
    bws_dispatch_lock();
    bsc_node_maintenance_timer(seconds);
    bsc_update_netport_properties();
    bws_dispatch_unlock();
}
