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
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/bzll/bzllvmac.h"
#include "bacnet/basic/bzll/h_bzll.h"
#include "bacnet/basic/sys/ringbuf.h"

/* buffer for incoming BACnet messages */
struct bzll_packet {
    BACNET_ADDRESS src;
    uint16_t length;
    uint8_t buffer[BZLL_MPDU_MAX];
};
/* count must be a power of 2 for ringbuf library */
#ifndef BZLL_RECEIVE_PACKET_COUNT
#define BZLL_RECEIVE_PACKET_COUNT 2
#endif
static struct bzll_packet Receive_Buffer[BZLL_RECEIVE_PACKET_COUNT];
static RING_BUFFER Receive_Queue;

/**
 * @brief Enable debugging if print is enabled
 */
void bzll_debug_enable(void)
{
    BZLL_VMAC_Debug_Enable();
}

/* Callback function for sending MPDU */
static bzll_send_mpdu_callback BZLL_Send_MPDU_Callback;

/**
 * @brief Set the callback function for sending MPDU
 * @param cb - callback function to be called when an MPDU needs to be sent
 */
void bzll_send_mpdu_callback_set(bzll_send_mpdu_callback cb)
{
    BZLL_Send_MPDU_Callback = cb;
}

/**
 * @brief Send a packet to the destination address via the callback function
 * @param dest - Points to a BZLL VMAC structure containing the
 *  EUI64 address and endpoint.
 * @param mtu - Points to a buffer containing the MPDU data to send.
 * @param mtu_len - number of bytes of MPDU data to send
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
 * @param cb - callback function to be called when my address is needed
 */
void bzll_get_my_address_callback_set(bzll_get_address_callback cb)
{
    BZLL_Get_My_Address_Callback = cb;
}

/**
 * @brief Get my address via the callback function
 * @param addr - Points to a BZLL VMAC structure to be filled with my address
 * @return true if the address was obtained successfully
 */
static bool bzll_get_addr(struct bzll_vmac_data *addr)
{
    if (BZLL_Get_My_Address_Callback) {
        return BZLL_Get_My_Address_Callback(addr) == 0;
    }
    return false;
}

/* Callback function for getting broadcast address */
static bzll_get_address_callback BZLL_Get_Broadcast_Address_Callback;

/**
 * @brief Set the callback function for getting broadcast address
 * @param cb - callback function to be called when broadcast address is needed
 */
void bzll_get_broadcast_address_callback_set(bzll_get_address_callback cb)
{
    BZLL_Get_Broadcast_Address_Callback = cb;
}

/**
 * @brief Get the broadcast address via the callback function
 * @param addr - Points to a BZLL VMAC structure to be filled with my address
 * @return true if the address was obtained successfully
 */
static bool bzll_get_broadcast_addr(struct bzll_vmac_data *addr)
{
    if (BZLL_Get_Broadcast_Address_Callback) {
        return BZLL_Get_Broadcast_Address_Callback(addr) == 0;
    }
    return false;
}

/**
 * @brief Generate a random VMAC address
 *
 * H.7.2 Using Device Instance as a VMAC Address
 *
 * The random portion of a random instance VMAC address
 * is a number in the range 0 to 4194303.
 * The resulting random instance VMAC address
 * is in the range 4194304 to 8388607 (X'100000' to X'7FFFFF ').
 * The generation of a random instance VMAC shall yield
 * any number in the entire range with equal probability.
 * A random instance VMAC is formatted as follows:
 *
 * Bit Number:
 *   7   6   5   4   3   2   1   0
 * |---|---|---|---|---|---|---|---|
 * | 0 | 1 |   High 6 Bits         |
 * |---|---|---|---|---|---|---|---|
 * |           Middle Octet        |
 * |---|---|---|---|---|---|---|---|
 * |           Low Octet           |
 * |---|---|---|---|---|---|---|---|
 *
 * @param src [out] returns the source address
 * @param npdu [out] The buffer to receive the NPDU.
 * @param npdu_len [in] The maximum number of bytes that can be received in
 * npdu[].
 */
uint32_t bzll_random_instance_vmac(void)
{
    uint32_t random_instance;

    /* The random portion of a random instance VMAC address
       is a number in the range 0 to 4194303. */
    random_instance = (uint32_t)(rand() % 4194304UL) + 4194304UL;

    return random_instance;
}

/**
 * @brief A timer function that is called about once a second.
 * @param seconds - number of elapsed seconds since the last call
 */
void bzll_maintenance_timer(uint16_t seconds)
{
    /* To be able to detect new nodes on the network,
       the BZLL on a router shall periodically issue
       a Read Attribute command requesting the Protocol
       Address attribute from all network nodes.

       The period at which a router requests all Protocol
       Address attributes is a local matter. */
    (void)seconds;
}

/**
 * Adds an Zigbee source address and Device ID key to a VMAC address cache
 * @param device_id - device ID used as the key-pair
 * @param addr - Zigbee source address
 */
static void bzll_add_vmac(uint32_t device_id, const struct bzll_vmac_data *addr)
{
    bool found = false;
    uint32_t list_device_id = 0;
    struct bzll_vmac_data *vmac = NULL;
    struct bzll_vmac_data new_vmac = { 0 };
    unsigned i = 0;

    if (BZLL_VMAC_Entry_To_Device_ID(addr, &list_device_id)) {
        if (list_device_id == device_id) {
            /* valid VMAC entry exists. */
            found = true;
        } else {
            /* VMAC exists, but device ID changed */
            BZLL_VMAC_Delete(list_device_id);
            debug_printf(
                "BZLL: VMAC existed for %u [", (unsigned int)list_device_id);
            for (i = 0; i < sizeof(new_vmac.mac); i++) {
                debug_printf("%02X", new_vmac.mac[i]);
            }
            debug_printf("]\n");
            debug_printf(
                "BZLL: Removed VMAC for %lu.\n", (unsigned long)list_device_id);
        }
    }
    if (!found) {
        if (BZLL_VMAC_Entry_By_Device_ID(device_id, vmac)) {
            /* device ID already exists. Update MAC. */
            BZLL_VMAC_Copy(vmac, &new_vmac);
            debug_printf("BZLL: VMAC for %u [", (unsigned int)device_id);
            for (i = 0; i < sizeof(new_vmac.mac); i++) {
                debug_printf("%02X", new_vmac.mac[i]);
            }
            debug_printf("]\n");
            debug_printf(
                "BZLL: Updated VMAC for %lu.\n", (unsigned long)device_id);
        } else {
            /* new entry - add it! */
            BZLL_VMAC_Add(device_id, &new_vmac);
            debug_printf("BZLL: VMAC for %u [", (unsigned int)device_id);
            for (i = 0; i < sizeof(new_vmac.mac); i++) {
                debug_printf("%02X", new_vmac.mac[i]);
            }
            debug_printf("]\n");
            debug_printf(
                "BZLL: Added VMAC for %lu.\n", (unsigned long)device_id);
        }
    }
}

/**
 * Compares the Zigbee source address to my VMAC
 *
 * @param addr - Zigbee source address
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
 * Finds the BZLL address for the #BACNET_ADDRESS via VMAC
 *
 * @param addr - BZLL address
 * @param vmac_src - VMAC address (Device ID)
 * @param baddr - Points to a #BACNET_ADDRESS structure containing the
 *  VMAC address.
 *
 * @return true if the address was in the VMAC table
 */
static bool bzll_address_from_bacnet_address(
    struct bzll_vmac_data *addr,
    uint32_t *vmac_device_id,
    const BACNET_ADDRESS *baddr)
{
    struct bzll_vmac_data *vmac = NULL;
    bool status = false;
    uint32_t device_id = 0;

    if (addr && baddr) {
        status = BZLL_VMAC_Device_ID_From_Address(baddr, &device_id);
        if (status) {
            if (BZLL_VMAC_Entry_By_Device_ID(device_id, vmac)) {
                BZLL_VMAC_Copy(addr, vmac);
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

int bzll_encode_original_broadcast(
    uint8_t *pdu,
    uint16_t pdu_size,
    uint32_t vmac,
    const uint8_t *npdu,
    uint16_t npdu_len)
{
    /* FIXME: */
    (void)vmac;
    (void)pdu;
    (void)pdu_size;
    (void)npdu;
    (void)npdu_len;
    return 0;
}

int bzll_encode_original_unicast(
    uint8_t *pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_dst,
    const uint8_t *npdu,
    uint16_t npdu_len)
{
    /* FIXME: */
    (void)vmac_src;
    (void)vmac_dst;
    (void)pdu;
    (void)pdu_size;
    (void)npdu;
    (void)npdu_len;
    return 0;
}

/**
 * @brief The common send function for BACnet Zigbee application layer
 * @param dest - Points to a #BACNET_ADDRESS structure containing the
 *  destination address.
 * @param npdu_data - Points to a BACNET_NPDU_DATA structure containing the
 *  destination network layer control flags and data.
 * @param pdu - the bytes of data to send
 * @param pdu_len - the number of bytes of data to send
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
    uint8_t mtu[BZLL_MPDU_MAX] = { 0 };
    uint16_t mtu_len = 0;
    uint32_t vmac_src = 0;
    uint32_t vmac_dst = 0;

    /* this datalink doesn't need to know the npdu data */
    (void)npdu_data;
    /* handle various broadcasts: */
    if ((dest->net == BACNET_BROADCAST_NETWORK) || (dest->mac_len == 0)) {
        /* mac_len = 0 is a broadcast address */
        /* net = 0 indicates local, net = 65535 indicates global */
        vmac_src = Device_Object_Instance_Number();
        mtu_len = bzll_encode_original_broadcast(
            mtu, sizeof(mtu), vmac_src, pdu, pdu_len);
        debug_printf("BZLL: Sent Original-Broadcast-NPDU.\n");
    } else if ((dest->net > 0) && (dest->len == 0)) {
        /* net > 0 and net < 65535 are network specific broadcast if len = 0 */
        if (dest->mac_len == 3) {
            /* network specific broadcast to address */
            bzll_address_from_bacnet_address(&bzll_dest, &vmac_dst, dest);
        } else {
            bzll_get_broadcast_addr(&bzll_dest);
        }
        vmac_src = Device_Object_Instance_Number();
        mtu_len = bzll_encode_original_broadcast(
            mtu, sizeof(mtu), vmac_src, pdu, pdu_len);
        debug_printf("BZLL: Sent Original-Broadcast-NPDU.\n");
    } else if (dest->mac_len == 3) {
        /* valid unicast */
        bzll_address_from_bacnet_address(&bzll_dest, &vmac_dst, dest);
        debug_printf("BZLL: Sending to VMAC %lu.\n", (unsigned long)vmac_dst);
        vmac_src = Device_Object_Instance_Number();
        mtu_len = bzll_encode_original_unicast(
            mtu, sizeof(mtu), vmac_src, vmac_dst, pdu, pdu_len);
        debug_printf("BZLL: Sent Original-Unicast-NPDU.\n");
    } else {
        debug_printf("BZLL: Send failure. Invalid Address.\n");
        return -1;
    }

    return bzll_send_mpdu(&bzll_dest, mtu, mtu_len);
}

/**
 * BACnet/IP Datalink Receive handler.
 *
 * @param src - returns the source address
 * @param npdu - returns the NPDU buffer
 * @param max_npdu -maximum size of the NPDU buffer
 * @param timeout - number of milliseconds to wait for a packet
 *
 * @return Number of bytes received, or 0 if none or timeout.
 */
uint16_t bzll_receive(
    BACNET_ADDRESS *src, uint8_t *npdu, uint16_t max_npdu, unsigned timeout)
{
    struct bzll_packet *pkt;
    uint16_t npdu_len = 0;

    (void)timeout;
    if (npdu && src) {
        if (Ringbuf_Pop(&Receive_Queue, (uint8_t *)&pkt)) {
            if (max_npdu <= sizeof(pkt->buffer)) {
                memcpy(npdu, pkt->buffer, pkt->length);
                bacnet_address_copy(src, &pkt->src);
                npdu_len = pkt->length;
            }
        }
    }

    return npdu_len;
}

/**
 * @brief Handler for received packets from the datalink layer
 * @param src [in] address to send any ACK or NAK back to.
 * @param npdu [in] The received BACnet NPDU buffer.
 * @param npdu_len [in] The number of bytes in the NPDU buffer.
 */
static void
bzll_receive_handler(BACNET_ADDRESS *src, uint8_t *npdu, uint16_t max_npdu)
{
    struct bzll_packet *pkt = NULL;

    if (npdu && src) {
        pkt = (struct bzll_packet *)Ringbuf_Data_Peek(&Receive_Queue);
        if (pkt && (max_npdu < sizeof(pkt->buffer))) {
            memcpy(pkt->buffer, npdu, max_npdu);
            bacnet_address_copy(&pkt->src, src);
            pkt->length = max_npdu;
            Ringbuf_Data_Put(&Receive_Queue, (volatile uint8_t *)pkt);
        }
    }
}

/**
 * Use this handler for BACnet/Zigbee received packets.
 *
 * @param src [in] address to send any ACK or NAK back to.
 * @param npdu [in] The received BACnet NPDU buffer.
 * @param npdu_len [in] How many bytes in the NPDU buffer.
 */
void bzll_receive_pdu(
    struct bzll_vmac_data *src, uint8_t *npdu, uint16_t npdu_len)
{
    uint32_t device_id = 0;
    BACNET_ADDRESS baddr = { 0 };

    if (bzll_address_match_self(src)) {
        /* ignore messages from my BACnet/Zigbee address */
    } else if (BZLL_VMAC_Entry_To_Device_ID(src, &device_id)) {
        /* this is a BACnet/Zigbee packet with a known device ID */
        debug_printf(
            "BZLL: Received packet from device %lu.\n",
            (unsigned long)device_id);
        BZLL_VMAC_Device_ID_To_Address(&baddr, device_id);
        bzll_receive_handler(&baddr, npdu, npdu_len);
    } else {
        /* this is an unknown BACnet/Zigbee VMAC address */
        do {
            /* generate a new random instance VMAC address */
            device_id = bzll_random_instance_vmac();
            /* check if the generated device ID is already in use */
        } while (BZLL_VMAC_Entry_By_Device_ID(device_id, NULL));
        bzll_add_vmac(device_id, src);
        BZLL_VMAC_Device_ID_To_Address(&baddr, device_id);
        bzll_receive_handler(&baddr, npdu, npdu_len);
    }
}

/**
 * Cleanup any memory usage
 */
void bzll_cleanup(void)
{
    BZLL_VMAC_Cleanup();
}

/**
 * Initialize any tables or other memory
 */
bool bzll_init(char *interface_name)
{
    (void)interface_name;
    Ringbuf_Init(
        &Receive_Queue, (uint8_t *)&Receive_Buffer, sizeof(struct bzll_packet),
        BZLL_RECEIVE_PACKET_COUNT);
    BZLL_VMAC_Init();

    return true;
}
