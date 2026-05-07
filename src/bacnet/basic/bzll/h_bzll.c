/**
 * @file
 * @brief BACnet Zigbee Link Layer
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h> /* for standard i/o, like printing */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <stdlib.h> /* for rand */
#include <string.h> /* for memcpy */
#include "bacnet/bacdcode.h"
#include "bacnet/datalink/bzll.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/bzll/bzllvmac.h"
/* me! */
#include "bacnet/basic/bzll/h_bzll.h"

/* buffer for incoming BACnet messages */
struct bzll_packet {
    BACNET_ADDRESS src;
    uint16_t length;
    uint8_t buffer[BZLL_MPDU_MAX];
};

/* Time to request a read if there is no communication */
#ifndef BZLL_NODE_TIME_TO_LIVE_S
#define BZLL_NODE_TIME_TO_LIVE_S (600)
#endif

/* Time of interval to scan all devices */
#ifndef BZLL_SCAN_NODES_INTERVAL_S
#define BZLL_SCAN_NODES_INTERVAL_S (3600)
#endif

/* Size of the protocol address size */
#define BZLL_ADDRESS_SIZE (3)

/* count must be a power of 2 for ringbuf library */
#ifndef BZLL_RECEIVE_PACKET_COUNT
#define BZLL_RECEIVE_PACKET_COUNT 2
#endif

static uint16_t bzll_broadcast_group_id = 0;
static struct bzll_packet Receive_Buffer[BZLL_RECEIVE_PACKET_COUNT];
static RING_BUFFER Receive_Queue;
static uint32_t bzll_time_scan_nodes_remaining = BZLL_SCAN_NODES_INTERVAL_S;

/**
 * @brief Enable debugging if print is enabled
 */
void bzll_debug_enable(void)
{
    BZLL_VMAC_Debug_Enable();
}

/**
 * @brief Determine if debugging is enabled
 */
bool bzll_debug_enabled(void)
{
    return BZLL_VMAC_Debug_Enabled();
}

/* Callback function for sending MPDU */
static bzll_send_mpdu_callback BZLL_Send_MPDU_Callback;

/**
 * @brief Set the callback function for sending MPDU
 *
 * @param cb - callback function to be called when an MPDU needs to be sent
 */
void bzll_send_mpdu_callback_set(bzll_send_mpdu_callback cb)
{
    BZLL_Send_MPDU_Callback = cb;
}

/**
 * @brief Send a packet to the destination address via the callback function
 *
 * @param dest - Points to a BZLL VMAC structure containing the
 *  EUI64 address and endpoint.
 * @param mtu - Points to a buffer containing the MPDU data to send.
 * @param mtu_len - number of bytes of MPDU data to send
 *
 * @return number of bytes sent on success, negative value on failure
 */
static int bzll_send_mpdu(
    const struct bzll_vmac_data *dest, const uint8_t *mtu, int mtu_len)
{
    if (BZLL_Send_MPDU_Callback) {
        return BZLL_Send_MPDU_Callback(dest, mtu, mtu_len);
    }
    return -1;
}

/* Callback function for getting my address */
static bzll_get_address_callback BZLL_Get_My_Address_Callback;

/**
 * @brief Set the callback function for getting my address
 *
 * @param cb - callback function to be called when my address is needed
 */
void bzll_get_my_address_callback_set(bzll_get_address_callback cb)
{
    BZLL_Get_My_Address_Callback = cb;
}

/**
 * @brief Get my address via the callback function
 *
 * @param addr - Points to a BZLL VMAC structure to be filled with my address
 *
 * @return true if the address was obtained successfully
 */
static bool bzll_get_addr(struct bzll_vmac_data *addr)
{
    if (BZLL_Get_My_Address_Callback) {
        return BZLL_Get_My_Address_Callback(addr) == 0;
    }
    return false;
}

/* Callback function to advertise the new address */
static bzll_advertise_address_callback BZLL_Advertise_Address_Callback;

/**
 * @brief Set the callback function to advertise the new address
 *
 * @param cb [in] - Callback function to be called when a new address has to be
 * advertised.
 */
void bzll_advertise_address_callback_set(bzll_advertise_address_callback cb)
{
    BZLL_Advertise_Address_Callback = cb;
}

/**
 * @brief Advertise the new address.
 * @param addr [in] - New address to be advertise
 * @return 0 if success, negative values for error
 */
static int bzll_advertise_address(const BACNET_ADDRESS *addr)
{
    if (BZLL_Advertise_Address_Callback) {
        if (addr->mac_len == BZLL_ADDRESS_SIZE) {
            return BZLL_Advertise_Address_Callback(
                addr->mac, BZLL_ADDRESS_SIZE);
        }
    }
    return -1;
}

/**
 * @brief Pointer to the callback stored.
 */
static bzll_request_read_property_address_callback
    BZLL_Request_Read_Property_Address_Callback;

/**
 * @brief Set callback to request read property address
 *
 * @param cb [in] Pointer to the callback.
 */
void bzll_request_read_property_address_callback_set(
    bzll_request_read_property_address_callback cb)
{
    BZLL_Request_Read_Property_Address_Callback = cb;
}

/**
 * @brief Call the callback to request the read of the address property
 *
 * @param dest [in] - Destination address. It can be a broadcast address.
 *
 * @return return 0 for success. Negative values for error.
 */
static int bzll_request_read_property_address(const struct bzll_vmac_data *dest)
{
    if (BZLL_Request_Read_Property_Address_Callback) {
        return BZLL_Request_Read_Property_Address_Callback(dest);
    }
    return -1;
}

/**
 * @brief Update the status and ttl of the node.
 *
 * @param vmac_data [in] - VMAC address of the node.
 */
static void bzll_update_node_status(struct bzll_vmac_data *vmac_data)
{
    struct bzll_node_status_table_entry *status_entry;
    bool status = BZLL_VMAC_Node_Status_Entry(vmac_data, &status_entry);
    if (status && status_entry) {
        status_entry->valid = true;
        status_entry->ttl_seconds_remaining = BZLL_NODE_TIME_TO_LIVE_S;
    }
}

/**
 * @brief Compares the Zigbee source address to my VMAC
 *
 * @param addr [in] - Zigbee source address
 *
 * @return true if the Zigbee source address matches my VMAC
 */
static bool bzll_address_match_self(const struct bzll_vmac_data *addr)
{
    struct bzll_vmac_data my_addr = { 0 };
    bool status = false;

    if (bzll_get_addr(&my_addr)) {
        if (BZLL_VMAC_Same(&my_addr, addr)) {
            status = true;
        }
    }

    return status;
}

/**
 * @brief Finds the BZLL address for the #BACNET_ADDRESS via VMAC
 *
 * @param addr [out]- BZLL address
 * @param vmac_src [out] - VMAC address (Device ID)
 * @param baddr [in] - Points to a #BACNET_ADDRESS structure containing the
 *  VMAC address.
 *
 * @return true if the address was in the VMAC table
 */
static bool bzll_address_from_bacnet_address(
    struct bzll_vmac_data *addr,
    uint32_t *vmac_device_id,
    const BACNET_ADDRESS *baddr)
{
    bool status = false;
    uint32_t device_id = 0;

    if (addr && baddr) {
        status = BZLL_VMAC_Device_ID_From_Address(baddr, &device_id);
        if (status) {
            status = BZLL_VMAC_Entry_By_Device_ID(device_id, addr);
            if (status) {
                debug_printf(
                    "BZLL: Found VMAC %lu.\n", (unsigned long)device_id);
                if (vmac_device_id) {
                    *vmac_device_id = device_id;
                }
            }
        }
    }

    return status;
}

/**
 * @brief Fill the bzll_vmac_data with the reserved value to indicate broadcast.
 * The Zigbee layer shall send to the GroupId of the BACnet cluster.
 *
 * @param vmac_data [out] - Address
 */
static void bzll_fill_vmac_broadcast_addr(struct bzll_vmac_data *vmac_data)
{
    uint8_t i = 0;
    for (i = 0; i < BZLL_VMAC_EUI64; i++) {
        vmac_data->mac[i] = 0xFF;
    }
    vmac_data->endpoint = 0xFF;
}

/**
 * @brief A timer function that is called about once a second.
 *
 * @param seconds [in] - number of elapsed seconds since the last call
 */
void bzll_maintenance_timer(uint16_t seconds)
{
    struct bzll_node_status_table_entry *status_entry;
    struct bzll_vmac_data dest;
    uint32_t device_id = 0;
    bool status = false;
    int count = 0;
    int i = 0;

    /* Verifies each of the nodes. */
    count = BZLL_VMAC_Count();
    for (i = 0; i < count; i++) {
        status = BZLL_VMAC_Node_Status_By_Index_Entry(i, &status_entry);
        if (status && status_entry) {
            if (status_entry->ttl_seconds_remaining > seconds) {
                status_entry->ttl_seconds_remaining -= seconds;
                continue;
            } else {
                if (status_entry->valid) {
                    /* The flag has to be valid when the response arrives */
                    status_entry->valid = false;
                    status_entry->ttl_seconds_remaining =
                        BZLL_NODE_TIME_TO_LIVE_S;
                    BZLL_VMAC_Entry_By_Index(i, NULL, &dest);
                    if (bzll_request_read_property_address(&dest) != 0) {
                        debug_printf("Read request Failed");
                    }
                    continue;
                } /* else remove */
            } /* else remove */
        } /* else remove */
        /* Remove from the broadcast table */
        BZLL_VMAC_Entry_By_Index(i, &device_id, NULL);
        BZLL_VMAC_Delete(device_id);
    }

    /* Scan all nodes */
    if (bzll_time_scan_nodes_remaining <= seconds) {
        bzll_time_scan_nodes_remaining = BZLL_SCAN_NODES_INTERVAL_S;
        count = BZLL_VMAC_Count();
        for (i = 0; i < count; i++) {
            status = BZLL_VMAC_Node_Status_By_Index_Entry(i, &status_entry);
            if (status && status_entry) {
                /* The flag has to be valid when the response arrives */
                status_entry->valid = false;
                status_entry->ttl_seconds_remaining = BZLL_NODE_TIME_TO_LIVE_S;
            }
        }
        bzll_fill_vmac_broadcast_addr(&dest);
        if (bzll_request_read_property_address(&dest) != 0) {
            debug_printf("Read request Failed");
        }
    } else {
        bzll_time_scan_nodes_remaining -= seconds;
    }
}

/**
 * @brief The common send function for BACnet Zigbee application layer
 *
 * @param dest [in] - Points to a #BACNET_ADDRESS structure containing the
 *  destination address.
 * @param npdu_data [in] - Points to a BACNET_NPDU_DATA structure containing the
 *  destination network layer control flags and data.
 * @param pdu [in] - the bytes of data to send
 * @param pdu_len [in] - the number of bytes of data to send
 *
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned to indicate the error.
 */
int bzll_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    struct bzll_vmac_data bzll_dest = { 0 };
    uint32_t vmac_dst = 0;

    /* this datalink doesn't need to know the npdu data */
    (void)npdu_data;
    /* handle various broadcasts: */
    if ((dest->net == BACNET_BROADCAST_NETWORK) || (dest->mac_len == 0)) {
        /* mac_len = 0 is a broadcast address */
        /* net = 0 indicates local, net = 65535 indicates global */
        bzll_fill_vmac_broadcast_addr(&bzll_dest);
        debug_printf("BZLL: Sent Original-Broadcast-NPDU.\n");
    } else if ((dest->net > 0) && (dest->len == 0)) {
        /* net > 0 and net < 65535 are network specific broadcast if len = 0 */
        if (dest->mac_len == BZLL_ADDRESS_SIZE) {
            /* network specific broadcast to address */
            if (!bzll_address_from_bacnet_address(
                    &bzll_dest, &vmac_dst, dest)) {
                return -1;
            }
            debug_printf("BZLL: Sent Network-Specific-Broadcast-NPDU.\n");
        } else {
            bzll_fill_vmac_broadcast_addr(&bzll_dest);
            debug_printf("BZLL: Sent Original-Broadcast-NPDU.\n");
        }
    } else if (dest->mac_len == BZLL_ADDRESS_SIZE) {
        /* valid unicast */
        if (!bzll_address_from_bacnet_address(&bzll_dest, &vmac_dst, dest)) {
            return -1;
        }
        debug_printf("BZLL: Sending to VMAC %lu.\n", (unsigned long)vmac_dst);
        debug_printf("BZLL: Sent Original-Unicast-NPDU.\n");
    } else {
        debug_printf("BZLL: Send failure. Invalid Address.\n");
        return -1;
    }

    return bzll_send_mpdu(&bzll_dest, pdu, pdu_len);
}

/**
 * @brief BACnet/Zigbee Datalink Receive handler.
 *
 * @param src [out] - returns the source address
 * @param npdu [out] - returns the NPDU buffer
 * @param max_npdu [in] -maximum size of the NPDU buffer
 * @param timeout [in] - number of milliseconds to wait for a packet
 *
 * @return Number of bytes received, or 0 if none or timeout.
 */
uint16_t bzll_receive(
    BACNET_ADDRESS *src, uint8_t *npdu, uint16_t max_npdu, unsigned timeout)
{
    struct bzll_packet pkt;
    uint16_t npdu_len = 0;

    (void)timeout;
    if (npdu && src) {
        if (Ringbuf_Pop(&Receive_Queue, (uint8_t *)&pkt)) {
            if (pkt.length <= max_npdu) {
                memcpy(npdu, pkt.buffer, pkt.length);
                bacnet_address_copy(src, &pkt.src);
                npdu_len = pkt.length;
            }
        }
    }

    return npdu_len;
}

/**
 * @brief Handler for received packets from the datalink layer
 *
 * @param src [in] address to send any ACK or NAK back to.
 * @param npdu [in] The received BACnet NPDU buffer.
 * @param npdu_len [in] The number of bytes in the NPDU buffer.
 */
static void
bzll_receive_handler(BACNET_ADDRESS *src, uint8_t *npdu, uint16_t npdu_len)
{
    struct bzll_packet *pkt = NULL;

    if (npdu && src) {
        pkt = (struct bzll_packet *)Ringbuf_Data_Peek(&Receive_Queue);
        if (pkt && (npdu_len <= sizeof(pkt->buffer))) {
            memcpy(pkt->buffer, npdu, npdu_len);
            bacnet_address_copy(&pkt->src, src);
            pkt->length = npdu_len;
            Ringbuf_Data_Put(&Receive_Queue, (volatile uint8_t *)pkt);
        } else {
            debug_printf("Received package is bigger than the buffer.");
        }
    }
}

/**
 * @brief Use this handler for BACnet/Zigbee received packets.
 *
 * @param src [in] address to send any ACK or NAK back to.
 * @param npdu [in] The received BACnet NPDU buffer.
 * @param npdu_len [in] How many bytes in the NPDU buffer.
 */
void bzll_receive_pdu(
    struct bzll_vmac_data *src, uint8_t *npdu, uint16_t npdu_len)
{
    uint32_t device_id = BACNET_NO_DEV_ID;
    BACNET_ADDRESS baddr = { 0 };
    bool status = false;

    if (bzll_address_match_self(src)) {
        /* ignore messages from my BACnet/Zigbee address */
    } else if (BZLL_VMAC_Entry_To_Device_ID(src, &device_id)) {
        /* this is a BACnet/Zigbee packet with a known device ID */
        debug_printf(
            "BZLL: Received packet from device %lu.\n",
            (unsigned long)device_id);

        /* update the status of the node */
        bzll_update_node_status(src);

        BZLL_VMAC_Device_ID_To_Address(&baddr, device_id);
        bzll_receive_handler(&baddr, npdu, npdu_len);
    } else {
        /* this is an unknown BACnet/Zigbee VMAC address */
        status = BZLL_VMAC_Add(&device_id, src);
        if (status) {
            /* update the status of the node */
            bzll_update_node_status(src);
        }

        BZLL_VMAC_Device_ID_To_Address(&baddr, device_id);
        bzll_receive_handler(&baddr, npdu, npdu_len);
    }
}

/**
 * @brief Cleanup any memory usage
 */
void bzll_cleanup(void)
{
    BZLL_VMAC_Cleanup();
    while (!Ringbuf_Empty(&Receive_Queue)) {
        Ringbuf_Pop(&Receive_Queue, NULL);
    }
}

/**
 * @brief Initialize any tables or other memory
 * @param interface_name
 *
 * @return true if initialized
 */
bool bzll_init(char *interface_name)
{
    (void)interface_name;
    Ringbuf_Initialize(
        &Receive_Queue, (uint8_t *)&Receive_Buffer, sizeof(Receive_Buffer),
        sizeof(struct bzll_packet), BZLL_RECEIVE_PACKET_COUNT);
    BZLL_VMAC_Init();
    bzll_time_scan_nodes_remaining = BZLL_SCAN_NODES_INTERVAL_S;

    return true;
}

/**
 * @brief Initialize the a data link broadcast address
 *
 * @param dest - address to be filled with broadcast designator
 */
void bzll_get_broadcast_address(BACNET_ADDRESS *dest)
{
    int i = 0;

    if (dest) {
        dest->mac_len = BZLL_ADDRESS_SIZE;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->mac[i] = 0xFF;
        }
        dest->net = BACNET_BROADCAST_NETWORK;
        /* always zero when DNET is broadcast */
        dest->len = 0;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }
    return;
}

/**
 * @brief Get the BACnet address for my interface
 *
 * @param my_address [out] - address to be filled with my interface address
 */
void bzll_get_my_address(BACNET_ADDRESS *my_address)
{
    uint32_t instance;

    instance = Device_Object_Instance_Number();
    bacnet_vmac_address_set(my_address, instance);

    return;
}

/**
 * @brief Get the maximum incoming transfer size read attribute
 *
 * @param transfer_size [out] - Get the maximum outgoing transfer size read
 * attribute.
 */
void bzll_get_maximum_incoming_transfer_size(uint32_t *transfer_size)
{
    *transfer_size = BZLL_MPDU_MAX;
}

/**
 * @brief Get the maximum outgoing transfer size read attribute.
 *
 * @param transfer_size [out] - Return the transfer size.
 */
void bzll_get_maximum_outgoing_transfer_size(uint32_t *transfer_size)
{
    *transfer_size = BZLL_MPDU_MAX;
}

/**
 * @brief Handler to the match protocol address command
 *
 * @param protocol_addr [in] - Instance Protocol address array
 * @param address_size [in] - Instance Protocol address array size
 *
 * @return true if the address is the same of my address
 */
bool bzll_match_protocol_address(
    const uint8_t *protocol_addr, const uint8_t address_size)
{
    BACNET_ADDRESS my_addr;
    BACNET_ADDRESS addr;
    uint32_t instance;

    if (address_size == BZLL_ADDRESS_SIZE) {
        decode_unsigned24(protocol_addr, &instance);
        bacnet_vmac_address_set(&addr, instance);

        bzll_get_my_address(&my_addr);

        return bacnet_address_same(&my_addr, &addr);
    }
    return false;
}

/**
 * @brief Response to the Read Property request.
 *
 * @param vmac_data [in] - The MAC address of the node
 * @param protocol_addr [in] Instance Address of the node
 * @param address_size [in] - Instance Address size of the node
 *
 * @return true if the VMAC table was updated successfully.
 */
bool bzll_update_node_protocol_address(
    struct bzll_vmac_data *vmac_data,
    uint8_t *protocol_addr,
    uint16_t address_size)
{
    uint32_t instance;
    bool status = false;

    if (address_size == BZLL_ADDRESS_SIZE) {
        decode_unsigned24(protocol_addr, &instance);
        status = BZLL_VMAC_Add(&instance, vmac_data);
        if (status) {
            bzll_update_node_status(vmac_data);
        }
        return status;
    }
    return false;
}

/**
 * @brief Handler to update the BACnet Object number after a write property
 * request.
 *
 * @param protocol_addr [in] - New address array
 * @param address_size [in] - New address array size
 *
 * @return true if the address was changed successfully
 */
bool bzll_update_object_protocol_address(
    uint8_t *protocol_addr, uint16_t address_size)
{
    uint32_t instance;
    if (address_size == BZLL_ADDRESS_SIZE) {
        decode_unsigned24(protocol_addr, &instance);
        return Device_Set_Object_Instance_Number(instance);
    }
    return false;
}

/**
 * @brief Call when the BACNET address changes
 *
 * @param my_address [in] - New address to be broadcast.
 */
void bzll_set_my_address(const BACNET_ADDRESS *my_address)
{
    bzll_advertise_address(my_address);
}

/**
 * @brief Retrieves device protocol address
 *
 * @param protocol_address [out] - Protocol address
 * @param protocol_address_size  [out] - Address Size
 */
void bzll_get_my_protocol_address(
    uint8_t *protocol_address, uint8_t *protocol_address_size)
{
    BACNET_ADDRESS addr;
    bzll_get_my_address(&addr);
    *protocol_address_size = addr.mac_len;
    memcpy(protocol_address, addr.mac, addr.mac_len);
}

/**
 * @brief Set broadcast group Id
 * @param group_id [in] - Group Id to be stored
 */
void bzll_set_broadcast_group_id(uint16_t group_id)
{
    bzll_broadcast_group_id = group_id;
}

/**
 * @brief Get broadcast group Id
 *
 * @return Stored group Id
 */
uint16_t bzll_get_broadcast_group_id(void)
{
    return bzll_broadcast_group_id;
}

bool bzll_valid(void)
{
    return true;
}
