/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2016 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#include <stdio.h> /* for standard i/o, like printing */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <string.h> /* for memcpy */
#include "bacnet/bacdcode.h"
#include "bacnet/datalink/bip6.h"
#include "bacnet/datalink/bvlc6.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/bbmd6/vmac.h"

/** result from a client request */
static uint16_t BVLC6_Result_Code = BVLC6_RESULT_SUCCESSFUL_COMPLETION;
/** incoming function */
static uint8_t BVLC6_Function_Code = BVLC6_RESULT;

/** if we are a foreign device, store the remote BBMD address/port here */
static BACNET_IP6_ADDRESS Remote_BBMD;
#if defined(BACDL_BIP6) && BBMD6_ENABLED
/* local buffer & length for sending */
static uint8_t BVLC6_Buffer[MAX_MPDU];
static uint16_t BVLC6_Buffer_Len;
/* Broadcast Distribution Table */
#ifndef MAX_BBMD6_ENTRIES
#define MAX_BBMD6_ENTRIES 128
#endif
static BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY
    BBMD_Table[MAX_BBMD6_ENTRIES];
/* Foreign Device Table */
#ifndef MAX_FD6_ENTRIES
#define MAX_FD6_ENTRIES 128
#endif
static BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY FD_Table[MAX_FD6_ENTRIES];
#endif

/** A timer function that is called about once a second.
 *
 * @param seconds - number of elapsed seconds since the last call
 */
void bvlc6_maintenance_timer(uint16_t seconds)
{
#if defined(BACDL_BIP6) && BBMD6_ENABLED
    unsigned i = 0;

    for (i = 0; i < MAX_FD6_ENTRIES; i++) {
        if (FD_Table[i].valid) {
            if (FD_Table[i].ttl_seconds_remaining) {
                if (FD_Table[i].ttl_seconds_remaining < seconds) {
                    FD_Table[i].ttl_seconds_remaining = 0;
                } else {
                    FD_Table[i].ttl_seconds_remaining -= seconds;
                }
                if (FD_Table[i].ttl_seconds_remaining == 0) {
                    FD_Table[i].valid = false;
                }
            }
        }
    }
#endif
}

/**
 * Sets the IPv6 source address from a VMAC address structure
 *
 * @param addr - IPv6 source address
 * @param vmac - VMAC address that will be use for holding the IPv6 address
 *
 * @return true if the address was set
 */
static bool bbmd6_address_from_vmac(
    BACNET_IP6_ADDRESS *addr, struct vmac_data *vmac)
{
    bool status = false;
    unsigned int i = 0;

    if (vmac && addr && (vmac->mac_len == 18)) {
        for (i = 0; i < IP6_ADDRESS_MAX; i++) {
            addr->address[i] = vmac->mac[i];
        }
        decode_unsigned16(&vmac->mac[16], &addr->port);
        status = true;
    }

    return status;
}

/**
 * Sets the IPv6 source address to a VMAC address structure
 *
 * @param vmac - VMAC address that will be use for holding the IPv6 address
 * @param addr - IPv6 source address
 *
 * @return true if the address was set
 */
static bool bbmd6_address_to_vmac(
    struct vmac_data *vmac, BACNET_IP6_ADDRESS *addr)
{
    bool status = false;
    unsigned int i = 0;

    if (vmac && addr) {
        for (i = 0; i < IP6_ADDRESS_MAX; i++) {
            vmac->mac[i] = addr->address[i];
        }
        encode_unsigned16(&vmac->mac[16], addr->port);
        vmac->mac_len = 18;
        status = true;
    }

    return status;
}

/**
 * Adds an IPv6 source address and Device ID key to a VMAC address cache
 *
 * @param device_id - device ID used as the key-pair
 * @param addr - IPv6 source address
 *
 * @return true if the VMAC address was added
 */
static bool bbmd6_add_vmac(uint32_t device_id, BACNET_IP6_ADDRESS *addr)
{
    bool status = false;
    struct vmac_data *vmac;
    struct vmac_data new_vmac;

    if (addr) {
        vmac = VMAC_Find_By_Key(device_id);
        if (vmac) {
            /* already exists - replace? */
        } else if (bbmd6_address_to_vmac(&new_vmac, addr)) {
            /* new entry - add it! */
            status = VMAC_Add(device_id, &new_vmac);
            debug_printf("BVLC6: Adding VMAC %lu.\n", (unsigned long)device_id);
        }
    }

    return status;
}

/**
 * Compares the IPv6 source address to my VMAC
 *
 * @param addr - IPv6 source address
 *
 * @return true if the IPv6 from sin match me
 */
static bool bbmd6_address_match_self(BACNET_IP6_ADDRESS *addr)
{
    BACNET_IP6_ADDRESS my_addr = { { 0 } };
    bool status = false;

    if (bip6_get_addr(&my_addr)) {
        if (!bvlc6_address_different(&my_addr, addr)) {
            status = true;
        }
    }

    return status;
}

/**
 * Finds the BACNET_IP6_ADDRESS for the #BACNET_ADDRESS via VMAC
 *
 * @param addr - IPv6 address
 * @param vmac_src - VMAC address (Device ID)
 * @param baddr - Points to a #BACNET_ADDRESS structure containing the
 *  VMAC address.
 *
 * @return true if the address was in the VMAC table
 */
static bool bbmd6_address_from_bacnet_address(
    BACNET_IP6_ADDRESS *addr, uint32_t *vmac_src, BACNET_ADDRESS *baddr)
{
    struct vmac_data *vmac;
    bool status = false;
    uint32_t device_id = 0;

    if (addr && baddr) {
        status = bvlc6_vmac_address_get(baddr, &device_id);
        if (status) {
            vmac = VMAC_Find_By_Key(device_id);
            if (vmac) {
                debug_printf("BVLC6: Found VMAC %lu (len=%u).\n",
                    (unsigned long)device_id, (unsigned)vmac->mac_len);
                status = bbmd6_address_from_vmac(addr, vmac);
                if (vmac_src) {
                    *vmac_src = device_id;
                }
            }
        }
    }

    return status;
}

/**
 * The common send function for BACnet/IPv6 application layer
 *
 * @param dest - Points to a #BACNET_ADDRESS structure containing the
 *  destination address.
 * @param npdu_data - Points to a BACNET_NPDU_DATA structure containing the
 *  destination network layer control flags and data.
 * @param pdu - the bytes of data to send
 * @param pdu_len - the number of bytes of data to send
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
int bvlc6_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    BACNET_IP6_ADDRESS bvlc_dest = { { 0 } };
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;
    uint32_t vmac_src = 0;
    uint32_t vmac_dst = 0;

    /* this datalink doesn't need to know the npdu data */
    (void)npdu_data;
    /* handle various broadcasts: */
    if ((dest->net == BACNET_BROADCAST_NETWORK) || (dest->mac_len == 0)) {
        /* mac_len = 0 is a broadcast address */
        /* net = 0 indicates local, net = 65535 indicates global */
        if (Remote_BBMD.port) {
            /* we are a foreign device */
            bvlc6_address_copy(&bvlc_dest, &Remote_BBMD);
            vmac_src = Device_Object_Instance_Number();
            mtu_len = bvlc6_encode_distribute_broadcast_to_network(
                mtu, sizeof(mtu), vmac_src, pdu, pdu_len);
            debug_printf("BVLC6: Sent Distribute-Broadcast-to-Network.\n");
        } else {
            bip6_get_broadcast_addr(&bvlc_dest);
            vmac_src = Device_Object_Instance_Number();
            mtu_len = bvlc6_encode_original_broadcast(
                mtu, sizeof(mtu), vmac_src, pdu, pdu_len);
            debug_printf("BVLC6: Sent Original-Broadcast-NPDU.\n");
        }
    } else if ((dest->net > 0) && (dest->len == 0)) {
        /* net > 0 and net < 65535 are network specific broadcast if len = 0 */
        if (dest->mac_len == 3) {
            /* network specific broadcast to address */
            bbmd6_address_from_bacnet_address(&bvlc_dest, &vmac_dst, dest);
        } else {
            bip6_get_broadcast_addr(&bvlc_dest);
        }
        vmac_src = Device_Object_Instance_Number();
        mtu_len = bvlc6_encode_original_broadcast(
            mtu, sizeof(mtu), vmac_src, pdu, pdu_len);
        debug_printf("BVLC6: Sent Original-Broadcast-NPDU.\n");
    } else if (dest->mac_len == 3) {
        /* valid unicast */
        bbmd6_address_from_bacnet_address(&bvlc_dest, &vmac_dst, dest);
        debug_printf("BVLC6: Sending to VMAC %lu.\n", (unsigned long)vmac_dst);
        vmac_src = Device_Object_Instance_Number();
        mtu_len = bvlc6_encode_original_unicast(
            mtu, sizeof(mtu), vmac_src, vmac_dst, pdu, pdu_len);
        debug_printf("BVLC6: Sent Original-Unicast-NPDU.\n");
    } else {
        debug_printf("BVLC6: Send failure. Invalid Address.\n");
        return -1;
    }

    return bip6_send_mpdu(&bvlc_dest, mtu, mtu_len);
}

#if defined(BACDL_BIP6) && BBMD6_ENABLED
/**
 * The send function for Broacast Distribution Table
 *
 * @param addr - Points to a #BACNET_IP6_ADDRESS structure containing the
 *  source IPv6 address.
 * @param vmac_src - Source-Virtual-Address
 * @param npdu - the bytes of NPDU+APDU data to send
 * @param npdu_len - the number of bytes of NPDU+APDU data to send
 */
static void bbmd6_send_pdu_bdt(uint8_t *mtu, unsigned int mtu_len)
{
    BACNET_IP6_ADDRESS my_addr = { { 0 } };
    unsigned i = 0; /* loop counter */

    if (mtu) {
        bip6_get_addr(&my_addr);
        for (i = 0; i < MAX_BBMD_ENTRIES; i++) {
            if (BBMD_Table[i].valid) {
                if (bvlc6_address_different(
                        &my_addr, &BBMD_Table[i].bip6_address)) {
                    bip6_send_mpdu(&BBMD_Table[i].bip6_address, mtu, mtu_len);
                }
            }
        }
    }
}

/**
 * The send function for Broacast Distribution Table
 *
 * @param addr - Points to a #BACNET_IP6_ADDRESS structure containing the
 *  source IPv6 address.
 * @param vmac_src - Source-Virtual-Address
 * @param npdu - the bytes of NPDU+APDU data to send
 * @param npdu_len - the number of bytes of NPDU+APDU data to send
 */
static void bbmd6_send_pdu_fdt(uint8_t *mtu, unsigned int mtu_len)
{
    BACNET_IP6_ADDRESS my_addr = { { 0 } };
    unsigned i = 0; /* loop counter */

    if (mtu) {
        bip6_get_addr(&my_addr);
        for (i = 0; i < MAX_FD_ENTRIES; i++) {
            if (FD_Table[i].valid) {
                if (bvlc6_address_different(
                        &my_addr, &FD_Table[i].bip6_address)) {
                    bip6_send_mpdu(&FD_Table[i].bip6_address, mtu, mtu_len);
                }
            }
        }
    }
}

/**
 * The Forward NPDU send function for Broacast Distribution Table
 *
 * @param addr - Points to a #BACNET_IP6_ADDRESS structure containing the
 *  source IPv6 address.
 * @param vmac_src - Source-Virtual-Address
 * @param npdu - the bytes of NPDU+APDU data to send
 * @param npdu_len - the number of bytes of NPDU+APDU data to send
 */
static void bbmd6_send_forward_npdu(BACNET_IP6_ADDRESS *address,
    uint32_t vmac_src,
    uint8_t *npdu,
    unsigned int npdu_len)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;
    unsigned i = 0; /* loop counter */

    for (i = 0; i < MAX_BBMD_ENTRIES; i++) {
        if (BBMD_Table[i].valid) {
            if (bbmd6_address_match_self(&BBMD_Table[i].bip6_address)) {
                /* don't forward to our selves */
            } else {
                bip6_send_mpdu(&BBMD_Table[i].bip6_address, mtu, mtu_len);
            }
        }
    }
    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        if (FD_Table[i].valid) {
            if (bbmd6_address_match_self(&FD_Table[i].bip6_address)) {
                /* don't forward to our selves */
            } else {
                bip6_send_mpdu(&FD_Table[i].bip6_address, mtu, mtu_len);
            }
        }
    }
}

#endif

/**
 * The Result Code send function for BACnet/IPv6 application layer
 *
 * @param dest_addr - Points to a #BACNET_IP6_ADDRESS structure containing the
 *  destination IPv6 address.
 * @param vmac_src - Source-Virtual-Address
 * @param result_code - BVLC result code
 *
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
static int bvlc6_send_result(
    BACNET_IP6_ADDRESS *dest_addr, uint32_t vmac_src, uint16_t result_code)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = bvlc6_encode_result(&mtu[0], sizeof(mtu), vmac_src, result_code);

    return bip6_send_mpdu(dest_addr, mtu, mtu_len);
}

/**
 * The Address Resolution Ack send function for BACnet/IPv6 application layer
 *
 * @param dest_addr - Points to a #BACNET_IP6_ADDRESS structure containing the
 *  destination IPv6 address.
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_dst - Destination-Virtual-Address
 *
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
static int bvlc6_send_address_resolution_ack(
    BACNET_IP6_ADDRESS *dest_addr, uint32_t vmac_src, uint32_t vmac_dst)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = bvlc6_encode_address_resolution_ack(
        &mtu[0], sizeof(mtu), vmac_src, vmac_dst);

    return bip6_send_mpdu(dest_addr, mtu, mtu_len);
}

/**
 * The Virtual Address Resolution Ack send function for BACnet/IPv6
 * application layer
 *
 * @param dest_addr - Points to a #BACNET_IP6_ADDRESS structure containing the
 *  destination IPv6 address.
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_dst - Destination-Virtual-Address
 *
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
static int bvlc6_send_virtual_address_resolution_ack(
    BACNET_IP6_ADDRESS *dest_addr, uint32_t vmac_src, uint32_t vmac_dst)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = bvlc6_encode_virtual_address_resolution_ack(
        &mtu[0], sizeof(mtu), vmac_src, vmac_dst);

    return bip6_send_mpdu(dest_addr, mtu, mtu_len);
}

/**
 * Handler for Virtual-Address-Resolution
 *
 * @param addr - BACnet/IPv6 source address any NAK or reply back to.
 * @param pdu - The received NPDU+APDU buffer.
 * @param pdu_len - How many bytes in NPDU+APDU buffer.
 */
static void bbmd6_virtual_address_resolution_handler(
    BACNET_IP6_ADDRESS *addr, uint8_t *pdu, uint16_t pdu_len)
{
    int function_len = 0;
    uint32_t vmac_src = 0;
    uint32_t vmac_me = 0;

    if (addr && pdu) {
        debug_printf("BIP6: Received Virtual-Address-Resolution.\n");
        if (bbmd6_address_match_self(addr)) {
            /* ignore messages from my IPv6 address */
        } else {
            function_len = bvlc6_decode_virtual_address_resolution(
                pdu, pdu_len, &vmac_src);
            if (function_len) {
                bbmd6_add_vmac(vmac_src, addr);
                /* The Address-Resolution-ACK message is unicast
                   to the B/IPv6 node that originally initiated
                   the Address-Resolution message. */
                vmac_me = Device_Object_Instance_Number();
                bvlc6_send_virtual_address_resolution_ack(
                    addr, vmac_me, vmac_src);
            }
        }
    }
}

/**
 * Handler for Virtual-Address-Resolution-ACK
 *
 * @param addr - BACnet/IPv6 source address any NAK or reply back to.
 * @param pdu - The received NPDU+APDU buffer.
 * @param pdu_len - How many bytes in NPDU+APDU buffer.
 */
static void bbmd6_virtual_address_resolution_ack_handler(
    BACNET_IP6_ADDRESS *addr, uint8_t *pdu, uint16_t pdu_len)
{
    int function_len = 0;
    uint32_t vmac_src = 0;
    uint32_t vmac_dst = 0;

    if (addr && pdu) {
        debug_printf("BIP6: Received Virtual-Address-Resolution-ACK.\n");
        if (bbmd6_address_match_self(addr)) {
            /* ignore messages from my IPv6 address */
        } else {
            function_len = bvlc6_decode_virtual_address_resolution_ack(
                pdu, pdu_len, &vmac_src, &vmac_dst);
            if (function_len) {
                bbmd6_add_vmac(vmac_src, addr);
            }
        }
    }
}

/**
 * Handler for Address-Resolution
 *
 * @param addr - BACnet/IPv6 source address any NAK or reply back to.
 * @param pdu - The received NPDU+APDU buffer.
 * @param pdu_len - How many bytes in NPDU+APDU buffer.
 */
static void bbmd6_address_resolution_handler(
    BACNET_IP6_ADDRESS *addr, uint8_t *pdu, uint16_t pdu_len)
{
    int function_len = 0;
    uint32_t vmac_src = 0;
    uint32_t vmac_target = 0;
    uint32_t vmac_me = 0;

    if (addr && pdu) {
        debug_printf("BIP6: Received Address-Resolution.\n");
        if (bbmd6_address_match_self(addr)) {
            /* ignore messages from my IPv6 address */
        } else {
            function_len = bvlc6_decode_address_resolution(
                pdu, pdu_len, &vmac_src, &vmac_target);
            if (function_len) {
                bbmd6_add_vmac(vmac_src, addr);
                vmac_me = Device_Object_Instance_Number();
                if (vmac_target == vmac_me) {
                    /* The Address-Resolution-ACK message is unicast
                       to the B/IPv6 node that originally initiated
                       the Address-Resolution message. */
                    bvlc6_send_address_resolution_ack(addr, vmac_me, vmac_src);
                }
            }
        }
    }
}

/**
 * Handler for Address-Resolution-ACK
 *
 * @param addr - BACnet/IPv6 source address any NAK or reply back to.
 * @param pdu - The received NPDU+APDU buffer.
 * @param pdu_len - How many bytes in NPDU+APDU buffer.
 */
static void bbmd6_address_resolution_ack_handler(
    BACNET_IP6_ADDRESS *addr, uint8_t *pdu, uint16_t pdu_len)
{
    int function_len = 0;
    uint32_t vmac_src = 0;
    uint32_t vmac_dst = 0;

    if (addr && pdu) {
        debug_printf("BIP6: Received Address-Resolution-ACK.\n");
        if (bbmd6_address_match_self(addr)) {
            /* ignore messages from my IPv6 address */
        } else {
            function_len = bvlc6_decode_address_resolution_ack(
                pdu, pdu_len, &vmac_src, &vmac_dst);
            if (function_len) {
                bbmd6_add_vmac(vmac_src, addr);
            }
        }
    }
}

/**
 * Use this handler when you are not a BBMD.
 * Sets the BVLC6_Function_Code in case it is needed later.
 *
 * @param addr - BACnet/IPv6 source address any NAK or reply back to.
 * @param src - BACnet style the source address interpreted VMAC
 * @param mtu - The received MTU buffer.
 * @param mtu_len - How many bytes in MTU buffer.
 *
 * @return number of bytes offset into the NPDU for APDU, or 0 if handled
 */
int bvlc6_bbmd_disabled_handler(BACNET_IP6_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *mtu,
    uint16_t mtu_len)
{
    uint16_t result_code = BVLC6_RESULT_SUCCESSFUL_COMPLETION;
    uint32_t vmac_src = 0;
    uint32_t vmac_dst = 0;
    uint8_t message_type = 0;
    uint16_t message_length = 0;
    int header_len = 0;
    int function_len = 0;
    uint8_t *pdu = NULL;
    uint16_t pdu_len = 0;
    uint16_t npdu_len = 0;
    bool send_result = false;
    uint16_t offset = 0;
    BACNET_IP6_ADDRESS fwd_address = { { 0 } };

    header_len =
        bvlc6_decode_header(mtu, mtu_len, &message_type, &message_length);
    if (header_len == 4) {
        BVLC6_Function_Code = message_type;
        pdu = &mtu[header_len];
        pdu_len = mtu_len - header_len;
        switch (BVLC6_Function_Code) {
            case BVLC6_RESULT:
                function_len =
                    bvlc6_decode_result(pdu, pdu_len, &vmac_src, &result_code);
                if (function_len) {
                    BVLC6_Result_Code = result_code;
                    /* The Virtual MAC address table shall be updated
                       using the respective parameter values of the
                       incoming messages. */
                    bbmd6_add_vmac(vmac_src, addr);
                    bvlc6_vmac_address_set(src, vmac_src);
                    debug_printf(
                        "BIP6: Received Result Code=%d\n", BVLC6_Result_Code);
                }
                break;
            case BVLC6_REGISTER_FOREIGN_DEVICE:
                result_code = BVLC6_RESULT_REGISTER_FOREIGN_DEVICE_NAK;
                send_result = true;
                break;
            case BVLC6_DELETE_FOREIGN_DEVICE:
                result_code = BVLC6_RESULT_DELETE_FOREIGN_DEVICE_NAK;
                send_result = true;
                break;
            case BVLC6_DISTRIBUTE_BROADCAST_TO_NETWORK:
                result_code = BVLC6_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK;
                send_result = true;
                break;
            case BVLC6_ORIGINAL_UNICAST_NPDU:
                /* This message is used to send directed NPDUs to
                   another B/IPv6 node or router. */
                debug_printf("BIP6: Received Original-Unicast-NPDU.\n");
                if (bbmd6_address_match_self(addr)) {
                    /* ignore messages from my IPv6 address */
                    debug_printf("BIP6: Original-Unicast-NPDU is me!.\n");
                } else {
                    function_len = bvlc6_decode_original_unicast(
                        pdu, pdu_len, &vmac_src, &vmac_dst, NULL, 0, &npdu_len);
                    if (function_len) {
                        if (vmac_dst == Device_Object_Instance_Number()) {
                            /* The Virtual MAC address table shall be updated
                               using the respective parameter values of the
                               incoming messages. */
                            bbmd6_add_vmac(vmac_src, addr);
                            bvlc6_vmac_address_set(src, vmac_src);
                            offset = header_len + (function_len - npdu_len);
                        } else {
                            debug_printf("BIP6: Original-Unicast-NPDU: "
                                         "VMAC is not me!\n");
                        }
                    } else {
                        debug_printf(
                            "BIP6: Original-Unicast-NPDU: Unable to decode!\n");
                    }
                }
                break;
            case BVLC6_ORIGINAL_BROADCAST_NPDU:
                debug_printf("BIP6: Received Original-Broadcast-NPDU.\n");
                if (bbmd6_address_match_self(addr)) {
                    /* ignore messages from my IPv6 address */
                    debug_printf("BIP6: Original-Broadcast-NPDU is me!\n");
                } else {
                    function_len = bvlc6_decode_original_broadcast(
                        pdu, pdu_len, &vmac_src, NULL, 0, &npdu_len);
                    if (function_len) {
                        /* The Virtual MAC address table shall be updated
                           using the respective parameter values of the
                           incoming messages. */
                        bbmd6_add_vmac(vmac_src, addr);
                        bvlc6_vmac_address_set(src, vmac_src);
                        offset = header_len + (function_len - npdu_len);
                    } else {
                        debug_printf("BIP6: Original-Broadcast-NPDU: Unable to "
                                     "decode!\n");
                    }
                }
                break;
            case BVLC6_FORWARDED_NPDU:
                debug_printf("BIP6: Received Forwarded-NPDU.\n");
                if (bbmd6_address_match_self(addr)) {
                    /* ignore messages from my IPv6 address */
                    debug_printf("BIP6: Forwarded-NPDU is me!\n");
                } else {
                    function_len = bvlc6_decode_forwarded_npdu(pdu, pdu_len,
                        &vmac_src, &fwd_address, NULL, 0, &npdu_len);
                    if (function_len) {
                        /* The Virtual MAC address table shall be updated
                           using the respective parameter values of the
                           incoming messages. */
                        bbmd6_add_vmac(vmac_src, &fwd_address);
                        bvlc6_vmac_address_set(src, vmac_src);
                        offset = header_len + (function_len - npdu_len);
                    } else {
                        debug_printf(
                            "BIP6: Forwarded-NPDU: Unable to decode!\n");
                    }
                }
                break;
            case BVLC6_FORWARDED_ADDRESS_RESOLUTION:
                result_code = BVLC6_RESULT_ADDRESS_RESOLUTION_NAK;
                send_result = true;
                break;
            case BVLC6_ADDRESS_RESOLUTION:
                bbmd6_address_resolution_handler(addr, pdu, pdu_len);
                break;
            case BVLC6_ADDRESS_RESOLUTION_ACK:
                bbmd6_address_resolution_ack_handler(addr, pdu, pdu_len);
                break;
            case BVLC6_VIRTUAL_ADDRESS_RESOLUTION:
                bbmd6_virtual_address_resolution_handler(addr, pdu, pdu_len);
                break;
            case BVLC6_VIRTUAL_ADDRESS_RESOLUTION_ACK:
                bbmd6_virtual_address_resolution_ack_handler(
                    addr, pdu, pdu_len);
                break;
            case BVLC6_SECURE_BVLL:
                break;
            default:
                break;
        }
        if (send_result) {
            vmac_src = Device_Object_Instance_Number();
            bvlc6_send_result(addr, vmac_src, result_code);
            debug_printf("BIP6: sent result code=%d\n", result_code);
        }
    }

    return offset;
}

#if defined(BACDL_BIP6) && BBMD6_ENABLED
#warning FIXME: Needs ported to IPv6
/**
 * Use this handler when you are a BBMD.
 * Sets the BVLC6_Function_Code in case it is needed later.
 *
 * @param addr - BACnet/IPv6 source address any NAK or reply back to.
 * @param src - BACnet style the source address interpreted VMAC
 * @param mtu - The received MTU buffer.
 * @param mtu_len - How many bytes in MTU buffer.
 *
 * @return number of bytes offset into the NPDU for APDU, or 0 if handled
 */
int bvlc6_bbmd_enabled_handler(BACNET_IP6_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *mtu,
    uint16_t mtu_len)
{
    uint16_t result_code = BVLC6_RESULT_SUCCESSFUL_COMPLETION;
    uint32_t vmac_me = 0;
    uint32_t vmac_src = 0;
    uint32_t vmac_dst = 0;
    uint32_t vmac_target = 0;
    uint8_t message_type = 0;
    uint16_t message_length = 0;
    int header_len = 0;
    int function_len = 0;
    uint8_t *pdu = NULL;
    uint16_t pdu_len = 0;
    uint8_t *npdu = NULL;
    uint16_t npdu_len = 0;
    bool send_result = false;
    uint16_t offset = 0;
    BACNET_IP6_ADDRESS fwd_address = { { 0 } };

    header_len =
        bvlc6_decode_header(mtu, mtu_len, &message_type, &message_length);
    if (header_len == 4) {
        BVLC6_Function_Code = message_type;
        pdu = &mtu[header_len];
        pdu_len = mtu_len - header_len;
        switch (BVLC6_Function_Code) {
            case BVLC6_RESULT:
                function_len =
                    bvlc6_decode_result(pdu, pdu_len, &vmac_src, &result_code);
                if (function_len) {
                    BVLC6_Result_Code = result_code;
                    /* The Virtual MAC address table shall be updated
                       using the respective parameter values of the
                       incoming messages. */
                    bbmd6_add_vmac(vmac_src, addr);
                    bvlc6_vmac_address_set(src, vmac_src);
                    debug_printf(
                        "BIP6: Received Result Code=%d\n", BVLC6_Result_Code);
                }
                break;
            case BVLC6_REGISTER_FOREIGN_DEVICE:
                result_code = BVLC6_RESULT_REGISTER_FOREIGN_DEVICE_NAK;
                send_result = true;
                break;
            case BVLC6_DELETE_FOREIGN_DEVICE:
                result_code = BVLC6_RESULT_DELETE_FOREIGN_DEVICE_NAK;
                send_result = true;
                break;
            case BVLC6_DISTRIBUTE_BROADCAST_TO_NETWORK:
                result_code = BVLC6_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK;
                send_result = true;
                break;
            case BVLC6_ORIGINAL_UNICAST_NPDU:
                /* This message is used to send directed NPDUs to
                   another B/IPv6 node or router. */
                debug_printf("BIP6: Received Original-Unicast-NPDU.\n");
                if (bbmd6_address_match_self(addr)) {
                    debug_printf("BIP6: Dropped Original-Unicast-NPDU.\n");
                    /* ignore messages from my IPv6 address */
                    offset = 0;
                } else {
                    function_len = bvlc6_decode_original_unicast(
                        pdu, pdu_len, &vmac_src, &vmac_dst, NULL, 0, &npdu_len);
                    if (function_len) {
                        if (vmac_dst == Device_Object_Instance_Number()) {
                            /* The Virtual MAC address table shall be updated
                               using the respective parameter values of the
                               incoming messages. */
                            bbmd6_add_vmac(vmac_src, addr);
                            bvlc6_vmac_address_set(src, vmac_src);
                            offset = header_len + (function_len - npdu_len);
                        }
                    }
                }
                break;
            case BVLC6_ORIGINAL_BROADCAST_NPDU:
                debug_printf("BIP6: Received Original-Broadcast-NPDU.\n");
                function_len = bvlc6_decode_original_broadcast(
                    pdu, pdu_len, &vmac_src, NULL, 0, &npdu_len);
                if (function_len) {
                    offset = header_len + (function_len - npdu_len);
                    npdu = &mtu[offset];
                    /*  Upon receipt of a BVLL Original-Broadcast-NPDU
                        message from the local multicast domain, a BBMD
                        shall construct a BVLL Forwarded-NPDU message and
                        unicast it to each entry in its BDT. In addition,
                        the constructed BVLL Forwarded-NPDU message shall
                        be unicast to each foreign device currently in
                        the BBMD's FDT */
                    BVLC6_Buffer_Len = bvlc6_encode_forwarded_npdu(
                        &BVLC6_Buffer[0], sizeof(BVLC6_Buffer), vmac_src, addr,
                        npdu, npdu_len);
                    bbmd6_send_pdu_bdt(&BVLC6_Buffer[0], BVLC6_Buffer_Len);
                    bbmd6_send_pdu_fdt(&BVLC6_Buffer[0], BVLC6_Buffer_Len);
                    if (!bbmd6_address_match_self(addr)) {
                        /* The Virtual MAC address table shall be updated
                           using the respective parameter values of the
                           incoming messages. */
                        bbmd6_add_vmac(vmac_src, addr);
                        bvlc6_vmac_address_set(src, vmac_src);
                    }
                }
                break;
            case BVLC6_FORWARDED_NPDU:
                debug_printf("BIP6: Received Forwarded-NPDU.\n");
                function_len = bvlc6_decode_forwarded_npdu(
                    pdu, pdu_len, &vmac_src, &fwd_address, NULL, 0, &npdu_len);
                if (function_len) {
                    offset = header_len + (function_len - npdu_len);
                    npdu = &mtu[offset];
                    /*  Upon receipt of a BVLL Forwarded-NPDU message
                        from a BBMD which is in the receiving BBMD's BDT,
                        a BBMD shall construct a BVLL Forwarded-NPDU and
                        transmit it via multicast to B/IPv6 devices in the
                        local multicast domain. */
                    BVLC6_Buffer_Len = bvlc6_encode_forwarded_npdu(
                        &BVLC6_Buffer[0], sizeof(BVLC6_Buffer), vmac_src, addr,
                        npdu, npdu_len);
                    bip6_get_broadcast_addr(&bvlc_dest);
                    bip6_send_mpdu(
                        &bvlc_dest, &BVLC6_Buffer[0], BVLC6_Buffer_Len);
                    /*  In addition, the constructed BVLL Forwarded-NPDU
                        message shall be unicast to each foreign device in
                        the BBMD's FDT. If the BBMD is unable to transmit
                        the Forwarded-NPDU, or the message was not received
                        from a BBMD which is in the receiving BBMD's BDT,
                        no BVLC-Result shall be returned and the message
                        shall be discarded. */
                    bbmd6_send_pdu_fdt(&BVLC6_Buffer[0], BVLC6_Buffer_Len);
                    if (!bbmd6_address_match_self(addr)) {
                        /* The Virtual MAC address table shall be updated
                           using the respective parameter values of the
                           incoming messages. */
                        bbmd6_add_vmac(vmac_src, &fwd_address);
                        bvlc6_vmac_address_set(src, vmac_src);
                    }
                }
                break;
            case BVLC6_FORWARDED_ADDRESS_RESOLUTION:
                result_code = BVLC6_RESULT_ADDRESS_RESOLUTION_NAK;
                send_result = true;
                break;
            case BVLC6_ADDRESS_RESOLUTION:
                bbmd6_address_resolution_handler(addr, pdu, pdu_len);
                break;
            case BVLC6_ADDRESS_RESOLUTION_ACK:
                bbmd6_address_resolution_ack_handler(addr, pdu, pdu_len);
                break;
            case BVLC6_VIRTUAL_ADDRESS_RESOLUTION:
                bbmd6_virtual_address_resolution_handler(addr, pdu, pdu_len);
                break;
            case BVLC6_VIRTUAL_ADDRESS_RESOLUTION_ACK:
                bbmd6_virtual_address_resolution_ack_handler(
                    addr, pdu, pdu_len);
                break;
            case BVLC6_SECURE_BVLL:
                break;
            default:
                break;
        }
        if (send_result) {
            vmac_src = Device_Object_Instance_Number();
            bvlc6_send_result(addr, vmac_src, result_code);
            debug_printf("BIP6: sent result code=%d\n", result_code);
        }
    }

    return offset;
}
#endif

/**
 * Use this handler for BACnet/IPv6 BVLC
 *
 * @param addr [in] IPv6 address to send any NAK back to.
 * @param src [out] returns the source address
 * @param npdu [in] The received buffer.
 * @param npdu_len [in] How many bytes in npdu[].
 *
 * @return number of bytes offset into the NPDU for APDU, or 0 if handled
 */
int bvlc6_handler(BACNET_IP6_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *npdu,
    uint16_t npdu_len)
{
#if defined(BACDL_BIP6) && BBMD6_ENABLED
    return bvlc6_bbmd_enabled_handler(addr, src, npdu, npdu_len);
#else
    return bvlc6_bbmd_disabled_handler(addr, src, npdu, npdu_len);
#endif
}

/** Register as a foreign device with the indicated BBMD.
 * @param bbmd_address - IPv4 address (long) of BBMD to register with,
 *                       in network byte order.
 * @param bbmd_port - Network port of BBMD, in network byte order
 * @param time_to_live_seconds - Lease time to use when registering.
 * @return Positive number (of bytes sent) on success,
 *         0 if no registration request is sent, or
 *         -1 if registration fails.
 */
int bvlc6_register_with_bbmd(BACNET_IP6_ADDRESS *bbmd_addr,
    uint32_t vmac_src,
    uint16_t time_to_live_seconds)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = bvlc6_encode_register_foreign_device(
        &mtu[0], sizeof(mtu), vmac_src, time_to_live_seconds);

    return bip6_send_mpdu(bbmd_addr, &mtu[0], mtu_len);
}

/** Returns the last BVLL Result we received, either as the result of a BBMD
 * request we sent, or (if not a BBMD or Client), from trying to register
 * as a foreign device.
 *
 * @return BVLC6_RESULT_SUCCESSFUL_COMPLETION on success,
 * BVLC6_RESULT_REGISTER_FOREIGN_DEVICE_NAK if registration failed,
 * or one of the other codes (if we are a BBMD).
 */
uint16_t bvlc6_get_last_result(void)
{
    return BVLC6_Result_Code;
}

/** Returns the current BVLL Function Code we are processing.
 * We have to store this higher layer code for when the lower layers
 * need to know what it is, especially to differentiate between
 * BVLC6_ORIGINAL_UNICAST_NPDU and BVLC6_ORIGINAL_BROADCAST_NPDU.
 *
 * @return A BVLC6_ code, such as BVLC6_ORIGINAL_UNICAST_NPDU.
 */
uint8_t bvlc6_get_function_code(void)
{
    return BVLC6_Function_Code;
}

/**
 * Cleanup any memory usage
 */
void bvlc6_cleanup(void)
{
    VMAC_Cleanup();
}

/**
 * Initialize any tables or other memory
 */
void bvlc6_init(void)
{
    VMAC_Init();
    BVLC6_Result_Code = BVLC6_RESULT_SUCCESSFUL_COMPLETION;
    BVLC6_Function_Code = BVLC6_RESULT;
    bvlc6_address_set(&Remote_BBMD, 0, 0, 0, 0, 0, 0, 0,
        BIP6_MULTICAST_GROUP_ID);
#if defined(BACDL_BIP6) && BBMD6_ENABLED
    memset(&BBMD_Table, 0, sizeof(BBMD_Table));
    memset(&FD_Table, 0, sizeof(FD_Table));
#endif
}
