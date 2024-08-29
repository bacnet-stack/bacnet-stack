/**
 * @file
 * @brief BBMD (BACnet Broadcast Management Device) for BACnet/IPv4
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2020
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdio.h> /* for standard i/o, like printing */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <string.h> /* for memcpy */
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/bbmd/h_bbmd.h"

/* Define BBMD_ENABLED to get the functions that a
 * BBMD needs to handle its services.
 * Separately, define BBMD_CLIENT_ENABLED to get the
 * functions that allow a client to manage a BBMD.
 */
#ifndef BBMD_ENABLED
#define BBMD_ENABLED 1
#endif

#if BBMD_ENABLED
#ifndef BBMD_CLIENT_ENABLED
#define BBMD_CLIENT_ENABLED 1
#endif
#endif

#ifndef PRINT_ENABLED
#define PRINT_ENABLED 0
#endif

/** enable debugging */
static bool BVLC_Debug = false;
/** result from a client request */
static uint16_t BVLC_Result_Code = BVLC_RESULT_INVALID;
/** incoming function */
static uint8_t BVLC_Function_Code = BVLC_INVALID;
/** Global IP address for NAT handling */
static BACNET_IP_ADDRESS BVLC_Global_Address;
/** Flag to indicate if NAT handling is enabled/disabled */
static bool BVLC_NAT_Handling = false;
/** if we are a foreign device, store the remote BBMD address/port here */
static BACNET_IP_ADDRESS Remote_BBMD;
/** if we are a foreign device, store the Time-To-Live Seconds here */
static uint16_t Remote_BBMD_TTL_Seconds;
#if BBMD_ENABLED || BBMD_CLIENT_ENABLED
/* local buffer & length for sending */
static uint8_t BVLC_Buffer[BIP_MPDU_MAX];
static uint16_t BVLC_Buffer_Len;
#endif
#if BBMD_ENABLED/* Broadcast Distribution Table */
#ifndef MAX_BBMD_ENTRIES
#define MAX_BBMD_ENTRIES 128
#endif
static BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY
    BBMD_Table[MAX_BBMD_ENTRIES];
/* Foreign Device Table */
#ifndef MAX_FD_ENTRIES
#define MAX_FD_ENTRIES 128
#endif
static BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY FD_Table[MAX_FD_ENTRIES];
#endif

/**
 * @brief Enabled debug printing of BACnet/IPv4 BBMD
 */
void bvlc_debug_enable(void)
{
    BVLC_Debug = true;
}

/**
 * @brief Disable debug printing of BACnet/IPv4 BBMD
 */
void bvlc_debug_disable(void)
{
    BVLC_Debug = false;
}

/**
 * @brief Print the IPv4 address with debug info for this module
 * @param str - debug info string
 * @param addr - IPv4 address
 */
static void debug_print_bip(const char *str, const BACNET_IP_ADDRESS *addr)
{
#if PRINT_ENABLED
    if (BVLC_Debug) {
        printf("BVLC: %s %u.%u.%u.%u:%u\n", str, (unsigned)addr->address[0],
            (unsigned)addr->address[1], (unsigned)addr->address[2],
            (unsigned)addr->address[3], (unsigned)addr->port);
    }
#else
    (void)str;
    (void)addr;
#endif
}

/**
 * @brief Print an unsigned value with debug info for this module
 * @param str - debug info string
 * @param value - unsigned int value
 */
static void debug_print_unsigned(const char *str, const unsigned int value)
{
#if PRINT_ENABLED
    if (BVLC_Debug) {
        printf("BVLC: %s %u\n", str, value);
    }
#else
    (void)str;
    (void)value;
#endif
}

/**
 * @brief Print an unsigned value with debug info for this module
 * @param str - debug info string
 * @param value - unsigned int value
 */
static void debug_print_npdu(
    const char *str, const unsigned int offset, const unsigned int length)
{
#if PRINT_ENABLED
    if (BVLC_Debug) {
        printf("BVLC: %s NPDU=MTU[%u] len=%u\n", str, offset, length);
    }
#else
    (void)str;
    (void)offset;
    (void)length;
#endif
}

/**
 * @brief Print debug info for this module
 * @param str - debug info string
 */
static void debug_print_string(const char *str)
{
#if PRINT_ENABLED
    if (BVLC_Debug) {
        printf("BVLC: %s\n", str);
    }
#else
    (void)str;
#endif
}

#if BBMD_ENABLED
/* Define BBMD_BACKUP_FILE if the contents of the BDT
 * (broadcast distribution table) are to be stored in
 * a backup file, so the contents are not lost across
 * power failures, shutdowns, etc...
 * (this is required behaviour as defined in BACnet standard).
 *
 * BBMD_BACKUP_FILE should be set to the file name
 * in which to store the BDT.
 */
#if defined(BBMD_BACKUP_FILE) && (BBMD_BACKUP_FILE == 1)
#undef BBMD_BACKUP_FILE
#define BBMD_BACKUP_FILE BACnet_BDT_table
#endif
#if defined(BBMD_BACKUP_FILE)
#define tostr(a) str(a)
#define str(a) #a

void bvlc_bdt_backup_local(void)
{
    static FILE *bdt_file_ptr = NULL;

    /* only try opening the file if not already opened previously */
    if (!bdt_file_ptr) {
        bdt_file_ptr = fopen(tostr(BBMD_BACKUP_FILE), "wb");
    }

    /* if error opening file for writing -> silently abort */
    if (!bdt_file_ptr) {
        return;
    }

    fseek(bdt_file_ptr, 0, SEEK_SET);
    fwrite(BBMD_Table, sizeof(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY),
        MAX_BBMD_ENTRIES, bdt_file_ptr);
    fflush(bdt_file_ptr);
}

void bvlc_bdt_restore_local(void)
{
    static FILE *bdt_file_ptr = NULL;

    /* only try opening the file if not already opened previously */
    if (!bdt_file_ptr) {
        bdt_file_ptr = fopen(tostr(BBMD_BACKUP_FILE), "rb");
    }

    /* if error opening file for reading -> silently abort */
    if (!bdt_file_ptr) {
        return;
    }

    fseek(bdt_file_ptr, 0, SEEK_SET);
    {
        BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY
        BBMD_Table_tmp[MAX_BBMD_ENTRIES];
        size_t entries = 0;

        entries = fread(BBMD_Table_tmp,
            sizeof(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY),
            MAX_BBMD_ENTRIES, bdt_file_ptr);
        if (entries == MAX_BBMD_ENTRIES) {
            /* success reading the BDT table. */
            memcpy(BBMD_Table, BBMD_Table_tmp,
                sizeof(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY) *
                    MAX_BBMD_ENTRIES);
        }
    }
}
#else
void bvlc_bdt_backup_local(void)
{
}
void bvlc_bdt_restore_local(void)
{
}
#endif
#endif

/** A timer function that is called about once a second.
 *
 * @param seconds - number of elapsed seconds since the last call
 */
void bvlc_maintenance_timer(uint16_t seconds)
{
#if BBMD_ENABLED
    bvlc_foreign_device_table_maintenance_timer(&FD_Table[0], seconds);
#else
    (void)seconds;
#endif
}

/**
 * Compares the IP source address to my IP address
 *
 * @param addr - IP source address
 *
 * @return true if the IP from sin match me
 */
static bool bbmd_address_match_self(const BACNET_IP_ADDRESS *addr)
{
    BACNET_IP_ADDRESS my_addr = { 0 };
    bool status = false;

    if (bip_get_addr(&my_addr)) {
        if (!bvlc_address_different(&my_addr, addr)) {
            status = true;
        }
    }

    return status;
}

#if BBMD_ENABLED
/** Determines if a BDT member has a unicast mask
 *
 * @param addr - BDT member that is sought
 *
 * @return True if BDT member is found and has a unicast mask
 */
static bool bbmd_bdt_member_mask_is_unicast(const BACNET_IP_ADDRESS *addr)
{
    bool unicast = false;
    BACNET_IP_ADDRESS my_addr = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK unicast_mask = { 0 };
    BACNET_IP_ADDRESS *dest_address = NULL;
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK *broadcast_mask = NULL;
    unsigned i = 0; /* loop counter */

    bip_get_addr(&my_addr);
    bvlc_broadcast_distribution_mask_from_host(&unicast_mask, 0xFFFFFFFFL);
    for (i = 0; i < MAX_BBMD_ENTRIES; i++) {
        if (BBMD_Table[i].valid) {
            dest_address = &BBMD_Table[i].dest_address;
            broadcast_mask = &BBMD_Table[i].broadcast_mask;
            if (bvlc_address_different(&my_addr, dest_address) &&
                !bvlc_address_different(addr, dest_address) &&
                !bvlc_broadcast_distribution_mask_different(
                    broadcast_mask, &unicast_mask)) {
                unicast = true;
                break;
            }
        }
    }

    return unicast;
}

/** Send a BVLL Forwarded-NPDU message on its local IP subnet using
 * the local B/IP broadcast address as the destination address.
 *
 * @param bip_src - source IP address and UDP port
 * @param npdu - returns the NPDU
 * @param max_npdu - amount of space available in the NPDU
 * @param npdu_length - reported length of the NPDU
 * @return number of bytes encoded in the Forwarded NPDU
 */
static uint16_t bbmd_forward_npdu(
    const BACNET_IP_ADDRESS *bip_src,
    const uint8_t *npdu,
    uint16_t npdu_length)
{
    BACNET_IP_ADDRESS broadcast_address = { 0 };
    uint8_t mtu[BIP_MPDU_MAX] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = (uint16_t)bvlc_encode_forwarded_npdu(
        &mtu[0], (uint16_t)sizeof(mtu), bip_src, npdu, npdu_length);
    if (mtu_len > 0) {
        bip_get_broadcast_addr(&broadcast_address);
        bip_send_mpdu(&broadcast_address, mtu, mtu_len);
        debug_printf("BVLC: Sent Forwarded-NPDU as local broadcast.\n");
    }

    return mtu_len;
}

/** Sends all Broadcast Devices a Forwarded NPDU
 *
 * @param bip_src - source IP address and UDP port
 * @param npdu - returns the NPDU
 * @param max_npdu - amount of space available in the NPDU
 * @param npdu_length - reported length of the NPDU
 * @param original - was the message an original (not forwarded)
 * @return number of bytes encoded in the Forwarded NPDU
 */
static uint16_t bbmd_bdt_forward_npdu(const BACNET_IP_ADDRESS *bip_src,
    const uint8_t *npdu,
    uint16_t npdu_length,
    bool original)
{
    uint8_t mtu[BIP_MPDU_MAX] = { 0 };
    uint16_t mtu_len = 0;
    unsigned i = 0; /* loop counter */
    BACNET_IP_ADDRESS bip_dest = { 0 };
    BACNET_IP_ADDRESS my_addr = { 0 };

    bip_get_addr(&my_addr);
    /* If we are forwarding an original broadcast message and the NAT
     * handling is enabled, change the source address to NAT routers
     * global IP address so the recipient can reply (local IP address
     * is not accessible from internet side.
     *
     * If we are forwarding a message from peer BBMD or foreign device
     * or the NAT handling is disabled, leave the source address as is.
     */
    if (BVLC_NAT_Handling && original) {
        mtu_len = (uint16_t)bvlc_encode_forwarded_npdu(&mtu[0],
            (uint16_t)sizeof(mtu), &BVLC_Global_Address, npdu, npdu_length);
    } else {
        mtu_len = (uint16_t)bvlc_encode_forwarded_npdu(
            &mtu[0], (uint16_t)sizeof(mtu), bip_src, npdu, npdu_length);
    }
    /* loop through the BDT and send one to each entry */
    for (i = 0; i < MAX_BBMD_ENTRIES; i++) {
        if (BBMD_Table[i].valid) {
            bvlc_broadcast_distribution_table_entry_forward_address(
                &bip_dest, &BBMD_Table[i]);
            if (!bvlc_address_different(&bip_dest, &my_addr)) {
                /* don't forward to our selves */
                continue;
            }
            if (!bvlc_address_different(&bip_dest, bip_src)) {
                /* don't forward back to origin */
                continue;
            }
            if (BVLC_NAT_Handling) {
                if (bvlc_address_different(&bip_dest, &BVLC_Global_Address)) {
                    /* NAT router port forwards BACnet packets from global IP.
                       Packets sent to that global IP by us would end up back,
                       creating a loop. */
                    continue;
                }
            }
            bip_send_mpdu(&bip_dest, mtu, mtu_len);
            debug_print_bip("BDT Send Forwarded-NPDU", &bip_dest);
        }
    }

    return mtu_len;
}

/** Sends all Foreign Devices a Forwarded NPDU
 *
 * @param bip_src - source IP address and UDP port
 * @param npdu - returns the NPDU
 * @param max_npdu - amount of space available in the NPDU
 * @param npdu_length - reported length of the NPDU
 * @param original - was the message an original (not forwarded)
 * @return number of bytes encoded in the Forwarded NPDU
 */
static uint16_t bbmd_fdt_forward_npdu(const BACNET_IP_ADDRESS *bip_src,
    const uint8_t *npdu,
    uint16_t npdu_length,
    bool original)
{
    uint8_t mtu[BIP_MPDU_MAX] = { 0 };
    uint16_t mtu_len = 0;
    unsigned i = 0; /* loop counter */
    BACNET_IP_ADDRESS bip_dest = { 0 };
    BACNET_IP_ADDRESS my_addr = { 0 };

    bip_get_addr(&my_addr);
    /* If we are forwarding an original broadcast message and the NAT
     * handling is enabled, change the source address to NAT routers
     * global IP address so the recipient can reply (local IP address
     * is not accessible from internet side.
     *
     * If we are forwarding a message from peer BBMD or foreign device
     * or the NAT handling is disabled, leave the source address as is.
     */
    if (BVLC_NAT_Handling && original) {
        mtu_len = (uint16_t)bvlc_encode_forwarded_npdu(&mtu[0],
            (uint16_t)sizeof(mtu), &BVLC_Global_Address, npdu, npdu_length);
    } else {
        mtu_len = (uint16_t)bvlc_encode_forwarded_npdu(
            &mtu[0], (uint16_t)sizeof(mtu), bip_src, npdu, npdu_length);
    }

    /* loop through the FDT and send one to each entry */
    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        if (FD_Table[i].valid && FD_Table[i].ttl_seconds_remaining) {
            bvlc_address_copy(&bip_dest, &FD_Table[i].dest_address);
            if (!bvlc_address_different(&bip_dest, &my_addr)) {
                /* don't forward to our selves */
                continue;
            }
            if (!bvlc_address_different(&bip_dest, bip_src)) {
                /* don't forward back to origin */
                continue;
            }
            if (BVLC_NAT_Handling) {
                if (bvlc_address_different(&bip_dest, &BVLC_Global_Address)) {
                    /* NAT router port forwards BACnet packets from global IP.
                       Packets sent to that global IP by us would end up back,
                       creating a loop. */
                    continue;
                }
            }
            bip_send_mpdu(&bip_dest, mtu, mtu_len);
            debug_print_bip("FDT Send Forwarded-NPDU", &bip_dest);
        }
    }

    return mtu_len;
}

/** Prints the Read-BDT-Ack NPDU
 *
 * @param addr - source IP address and UDP port
 * @param npdu - returns the NPDU
 * @param max_npdu - amount of space available in the NPDU
 * @param npdu_length - reported length of the NPDU
 * @param original - was the message an original (not forwarded)
 */
static void bbmd_read_bdt_ack_handler(
    const BACNET_IP_ADDRESS *addr, const uint8_t *npdu, uint16_t npdu_length)
{
#if PRINT_ENABLED
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY bdt_entry = { 0 };
    uint16_t offset = 0;
    unsigned count = 1;
    int len = 0;

    printf("BBMD: %u.%u.%u.%u:%u\n", (unsigned)addr->address[0],
        (unsigned)addr->address[1], (unsigned)addr->address[2],
        (unsigned)addr->address[3], (unsigned)addr->port);
    while (npdu_length >= BACNET_IP_BDT_ENTRY_SIZE) {
        len = bvlc_decode_broadcast_distribution_table_entry(
            &npdu[offset], npdu_length, &bdt_entry);
        if (len > 0) {
            printf("BDT-%03u: %u.%u.%u.%u:%u %u.%u.%u.%u\n", count,
                (unsigned)bdt_entry.dest_address.address[0],
                (unsigned)bdt_entry.dest_address.address[1],
                (unsigned)bdt_entry.dest_address.address[2],
                (unsigned)bdt_entry.dest_address.address[3],
                (unsigned)bdt_entry.dest_address.port,
                (unsigned)bdt_entry.broadcast_mask.address[0],
                (unsigned)bdt_entry.broadcast_mask.address[1],
                (unsigned)bdt_entry.broadcast_mask.address[2],
                (unsigned)bdt_entry.broadcast_mask.address[3]);
            offset += len;
            npdu_length -= len;
            count++;
        } else {
            break;
        }
    }
#else
    (void)addr;
    (void)npdu;
    (void)npdu_length;
#endif
}

/** Prints the Read-FDT-Ack NPDU
 *
 * @param addr - source IP address and UDP port
 * @param npdu - returns the NPDU
 * @param max_npdu - amount of space available in the NPDU
 * @param npdu_length - reported length of the NPDU
 * @param original - was the message an original (not forwarded)
 */
static void bbmd_read_fdt_ack_handler(
    const BACNET_IP_ADDRESS *addr, const uint8_t *npdu, uint16_t npdu_length)
{
#if PRINT_ENABLED
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY fdt_entry = { 0 };
    uint16_t offset = 0;
    unsigned count = 1;
    int len = 0;

    printf("BBMD: %u.%u.%u.%u:%u\n", (unsigned)addr->address[0],
        (unsigned)addr->address[1], (unsigned)addr->address[2],
        (unsigned)addr->address[3], (unsigned)addr->port);
    while (npdu_length >= BACNET_IP_FDT_ENTRY_SIZE) {
        len = bvlc_decode_foreign_device_table_entry(
            &npdu[offset], npdu_length, &fdt_entry);
        if (len > 0) {
            printf("FDT-%03u: %u.%u.%u.%u:%u %us %us\n", count,
                (unsigned)fdt_entry.dest_address.address[0],
                (unsigned)fdt_entry.dest_address.address[1],
                (unsigned)fdt_entry.dest_address.address[2],
                (unsigned)fdt_entry.dest_address.address[3],
                (unsigned)fdt_entry.dest_address.port,
                (unsigned)fdt_entry.ttl_seconds,
                (unsigned)fdt_entry.ttl_seconds_remaining);
            offset += len;
            npdu_length -= len;
            count++;
        } else {
            break;
        }
    }
#else
    (void)addr;
    (void)npdu;
    (void)npdu_length;
#endif
}
#endif

/**
 * The common send function for BACnet/IP application layer
 *
 * @param dest - Points to a #BACNET_ADDRESS structure containing the
 *  destination address.
 * @param npdu_data - Points to a BACNET_NPDU_DATA structure containing the
 *  destination network layer control flags and data.
 * @param mtu - the bytes of data to send
 * @param mtu_len - the number of bytes of data to send
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
int bvlc_send_pdu(const BACNET_ADDRESS *dest,
    const BACNET_NPDU_DATA *npdu_data,
    const uint8_t *pdu,
    unsigned pdu_len)
{
    BACNET_IP_ADDRESS bvlc_dest = { 0 };
    uint8_t mtu[BIP_MPDU_MAX] = { 0 };
    uint16_t mtu_len = 0;
#if BBMD_ENABLED
    BACNET_IP_ADDRESS bip_src = { 0 };
#endif

    /* this datalink doesn't need to know the npdu data */
    (void)npdu_data;
    /* handle various broadcasts: */
    if ((dest->net == BACNET_BROADCAST_NETWORK) || (dest->mac_len == 0)) {
        /* mac_len = 0 is a broadcast address */
        /* net = 0 indicates local, net = 65535 indicates global */
        if (Remote_BBMD.port) {
            /* we are a foreign device */
            bvlc_address_copy(&bvlc_dest, &Remote_BBMD);
            mtu_len = bvlc_encode_distribute_broadcast_to_network(
                mtu, sizeof(mtu), pdu, pdu_len);
            debug_print_bip("Send Distribute-Broadcast-to-Network", &bvlc_dest);
        } else {
            bip_get_broadcast_addr(&bvlc_dest);
            mtu_len =
                bvlc_encode_original_broadcast(mtu, sizeof(mtu), pdu, pdu_len);
            debug_print_bip("Send Original-Broadcast-NPDU", &bvlc_dest);
#if BBMD_ENABLED
            if (mtu_len > 0) {
                bip_get_addr(&bip_src);
                (void)bbmd_fdt_forward_npdu(&bip_src, pdu, pdu_len, true);
                (void)bbmd_bdt_forward_npdu(&bip_src, pdu, pdu_len, true);
            }
#endif
        }
    } else if ((dest->net > 0) && (dest->len == 0)) {
        /* net > 0 and net < 65535 are network specific broadcast if len = 0 */
        if (dest->mac_len == 6) {
            /* network specific broadcast to address */
            bvlc_ip_address_from_bacnet_local(&bvlc_dest, dest);
        } else {
            bip_get_broadcast_addr(&bvlc_dest);
        }
        mtu_len =
            bvlc_encode_original_broadcast(mtu, sizeof(mtu), pdu, pdu_len);
        debug_print_bip("Send Original-Broadcast-NPDU", &bvlc_dest);
    } else if (dest->mac_len == 6) {
        /* valid unicast */
        bvlc_ip_address_from_bacnet_local(&bvlc_dest, dest);
        mtu_len = bvlc_encode_original_unicast(mtu, sizeof(mtu), pdu, pdu_len);
        debug_print_bip("Send Original-Unicast-NPDU", &bvlc_dest);
    } else {
        debug_print_string("Send failure. Invalid Address.");
        return -1;
    }

    return bip_send_mpdu(&bvlc_dest, mtu, mtu_len);
}

/**
 * The Result Code send function for BACnet/IPv4 application layer
 *
 * @param dest_addr - Points to a #BACNET_IP_ADDRESS structure containing the
 *  destination IPv4 address.
 * @param result_code - BVLC result code
 *
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
static int bvlc_send_result(
    const BACNET_IP_ADDRESS *dest_addr, uint16_t result_code)
{
    uint8_t mtu[BIP_MPDU_MAX] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = bvlc_encode_result(&mtu[0], sizeof(mtu), result_code);

    return bip_send_mpdu(dest_addr, mtu, mtu_len);
}

/**
 * Use this handler when you are not a BBMD.
 * Sets the BVLC_Function_Code in case it is needed later.
 *
 * @param addr - BACnet/IPv4 source address any NAK or reply back to.
 * @param src - BACnet source address
 * @param mtu - The received MTU buffer.
 * @param mtu_len - How many bytes in MTU buffer.
 *
 * @return number of bytes offset into the NPDU for APDU, or 0 if handled
 */
int bvlc_bbmd_disabled_handler(BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *mtu,
    uint16_t mtu_len)
{
    uint16_t result_code = BVLC_RESULT_SUCCESSFUL_COMPLETION;
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
    BACNET_IP_ADDRESS fwd_address = { 0 };

    header_len =
        bvlc_decode_header(mtu, mtu_len, &message_type, &message_length);
    if (header_len == 4) {
        BVLC_Function_Code = message_type;
        pdu = &mtu[header_len];
        pdu_len = mtu_len - header_len;
        switch (BVLC_Function_Code) {
            case BVLC_RESULT:
                function_len = bvlc_decode_result(pdu, pdu_len, &result_code);
                if (function_len) {
                    BVLC_Result_Code = result_code;
                    debug_print_unsigned(
                        "Received Result Code =", BVLC_Result_Code);
                }
                break;
            case BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE:
                result_code =
                    BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK;
                send_result = true;
                break;
            case BVLC_READ_BROADCAST_DIST_TABLE:
                result_code = BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK;
                send_result = true;
                break;
            case BVLC_READ_BROADCAST_DIST_TABLE_ACK:
                result_code = BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK;
                send_result = true;
                break;
            case BVLC_FORWARDED_NPDU:
                debug_print_bip("Received Forwarded-NPDU", addr);
                function_len = bvlc_decode_forwarded_npdu(
                    pdu, pdu_len, &fwd_address, NULL, 0, &npdu_len);
                if (function_len) {
                    if (bbmd_address_match_self(&fwd_address)) {
                        /* ignore forwards from my IPv4 address */
                        debug_print_string("Dropped Forwarded-NPDU from me!");
                        break;
                    }
                    bvlc_ip_address_to_bacnet_local(src, &fwd_address);
                    offset = header_len + function_len - npdu_len;
                    debug_print_npdu("Forwarded-NPDU", offset, npdu_len);
                } else {
                    debug_print_string("Dropped Forwarded-NPDU: Malformed!");
                }
                break;
            case BVLC_REGISTER_FOREIGN_DEVICE:
                result_code = BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK;
                send_result = true;
                break;
            case BVLC_READ_FOREIGN_DEVICE_TABLE:
                result_code = BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK;
                send_result = true;
                break;
            case BVLC_READ_FOREIGN_DEVICE_TABLE_ACK:
                result_code = BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK;
                send_result = true;
                break;
            case BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY:
                result_code = BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK;
                send_result = true;
                break;
            case BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK:
                result_code = BVLC_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK;
                send_result = true;
                break;
            case BVLC_ORIGINAL_UNICAST_NPDU:
                /* This message is used to send directed NPDUs to
                   another B/IPv4 node or router. */
                debug_print_bip("Received Original-Unicast-NPDU", addr);
                if (bbmd_address_match_self(addr)) {
                    /* ignore messages from my IPv4 address */
                    debug_print_string(
                        "Dropped Original-Unicast-NPDU from me!");
                    break;
                }
                function_len = bvlc_decode_original_unicast(
                    pdu, pdu_len, NULL, 0, &npdu_len);
                if (function_len) {
                    bvlc_ip_address_to_bacnet_local(src, addr);
                    offset = header_len + function_len - npdu_len;
                    debug_print_npdu("Original-Unicast-NPDU", offset, npdu_len);
                } else {
                    debug_print_string(
                        "Dropped Original-Unicast-NPDU: Malformed!");
                }
                break;
            case BVLC_ORIGINAL_BROADCAST_NPDU:
                debug_print_bip("Received Original-Broadcast-NPDU", addr);
                if (bbmd_address_match_self(addr)) {
                    /* ignore messages from my IPv4 address */
                    debug_print_string(
                        "Dropped Original-Broadcast-NPDU from me!");
                    break;
                }
                function_len = bvlc_decode_original_broadcast(
                    pdu, pdu_len, NULL, 0, &npdu_len);
                if (function_len) {
                    bvlc_ip_address_to_bacnet_local(src, addr);
                    offset = header_len + function_len - npdu_len;
                    /* BTL test: verifies that the IUT will quietly discard any
                       Confirmed-Request-PDU, whose destination address is a
                       multicast or broadcast address, received from the
                       network layer. */
                    npdu = &mtu[offset];
                    if (npdu_confirmed_service(npdu, npdu_len)) {
                        offset = 0;
                        debug_print_string("Dropped Original-Broadcast-NPDU: "
                                           "Confirmed Service!");
                    } else {
                        debug_print_npdu(
                            "Original-Broadcast-NPDU", offset, npdu_len);
                    }
                } else {
                    debug_print_string(
                        "Dropped Original-Broadcast-NPDU: Malformed!");
                }
                break;
            case BVLC_SECURE_BVLL:
                break;
            default:
                break;
        }
        if (send_result) {
            bvlc_send_result(addr, result_code);
            debug_print_unsigned("Sent result code =", result_code);
        }
    }

    return offset;
}

#if BBMD_ENABLED
/**
 * Use this handler when you are a BBMD.
 * Sets the BVLC_Function_Code in case it is needed later.
 *
 * @param addr [in] BACnet/IPv4 source address any NAK or reply back to.
 * @param src [out] BACnet style the source address interpreted VMAC
 * @param mtu [in] The received MTU buffer.
 * @param mtu_len [in] How many bytes in MTU buffer.
 *
 * @return number of bytes offset into the NPDU for APDU, or 0 if handled
 */
int bvlc_bbmd_enabled_handler(BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *mtu,
    uint16_t mtu_len)
{
    uint16_t result_code = BVLC_RESULT_SUCCESSFUL_COMPLETION;
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
    uint16_t ttl_seconds = 0;
    BACNET_IP_ADDRESS fwd_address = { 0 };
    BACNET_IP_ADDRESS broadcast_address = { 0 };

    header_len =
        bvlc_decode_header(mtu, mtu_len, &message_type, &message_length);
    if (header_len != 4) {
        return 0;
    }
    BVLC_Function_Code = message_type;
    pdu = &mtu[header_len];
    pdu_len = mtu_len - header_len;
    switch (BVLC_Function_Code) {
        case BVLC_RESULT:
            function_len = bvlc_decode_result(pdu, pdu_len, &result_code);
            if (function_len) {
                BVLC_Result_Code = result_code;
                debug_print_unsigned(
                    "Received Result Code =", BVLC_Result_Code);
            }
            break;
        case BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE:
            debug_print_bip("Received Write-BDT", addr);
            function_len = bvlc_decode_write_broadcast_distribution_table(
                pdu, pdu_len, &BBMD_Table[0]);
            if (function_len > 0) {
                /* BDT changed! Save backup to file */
                bvlc_bdt_backup_local();
                result_code = BVLC_RESULT_SUCCESSFUL_COMPLETION;
                send_result = true;
            } else {
                result_code =
                    BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK;
                send_result = true;
            }
            /* not an NPDU */
            offset = 0;
            break;
        case BVLC_READ_BROADCAST_DIST_TABLE:
            debug_print_bip("Received Read-BDT", addr);
            BVLC_Buffer_Len = bvlc_encode_read_broadcast_distribution_table_ack(
                BVLC_Buffer, sizeof(BVLC_Buffer), &BBMD_Table[0]);
            if (BVLC_Buffer_Len > 0) {
                bip_send_mpdu(addr, BVLC_Buffer, BVLC_Buffer_Len);
            } else {
                result_code = BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK;
                send_result = true;
            }
            /* not an NPDU */
            offset = 0;
            break;
        case BVLC_READ_BROADCAST_DIST_TABLE_ACK:
            debug_print_bip("Received Read-BDT-Ack", addr);
            bbmd_read_bdt_ack_handler(addr, pdu, pdu_len);
            /* not an NPDU */
            offset = 0;
            break;
        case BVLC_FORWARDED_NPDU:
            debug_print_bip("Received Forwarded-NPDU", addr);
            /* Upon receipt of a BVLL Forwarded-NPDU message, a BBMD shall
               process it according to whether it was received from a peer
               BBMD as the result of a directed broadcast or a unicast
               transmission. A BBMD may ascertain the method by which
               Forwarded-NPDU messages will arrive by inspecting the
               broadcast distribution mask field in its own BDT entry
               since all BDTs are required to be identical.
               If the message arrived via directed broadcast,
               it was also received by the other devices on the
               BBMD's subnet. In this case the BBMD merely retransmits
               the message directly to each foreign device currently
               in the BBMD's FDT. If the message arrived via a unicast
               transmission it has not yet been received by the other
               devices on the BBMD's subnet. In this case, the message is
               sent to the devices on the BBMD's subnet using the
               B/IP broadcast address as well as to each
               foreign device currently in the BBMD's FDT.
               A BBMD on a subnet with no other BACnet devices may omit
               the broadcast using the B/IP broadcast address.
               The method by which a BBMD determines whether or not
               other BACnet devices are present is a local matter. */
            function_len = bvlc_decode_forwarded_npdu(
                pdu, pdu_len, &fwd_address, NULL, 0, &npdu_len);
            if (function_len) {
                if (bbmd_address_match_self(&fwd_address)) {
                    /* ignore forwards from my IPv4 address */
                    debug_print_string("Dropped Forwarded-NPDU from me!");
                    break;
                }
                if (bbmd_bdt_member_mask_is_unicast(addr)) {
                    /*  Upon receipt of a BVLL Forwarded-NPDU message
                        from a BBMD which is in the receiving BBMD's BDT,
                        a BBMD shall construct a BVLL Forwarded-NPDU and
                        transmit it via broadcast to B/IPv4 devices in the
                        local broadcast domain. */
                    bip_get_broadcast_addr(&broadcast_address);
                    bip_send_mpdu(&broadcast_address, mtu, mtu_len);
                }
                /*  In addition, the constructed BVLL Forwarded-NPDU
                    message shall be unicast to each foreign device in
                    the BBMD's FDT. */
                offset = header_len + function_len - npdu_len;
                npdu = &mtu[offset];
                (void)bbmd_fdt_forward_npdu(&fwd_address, npdu, npdu_len, false);
                /* prepare the message for me! */
                bvlc_ip_address_to_bacnet_local(src, &fwd_address);
                debug_print_npdu("Forwarded-NPDU", offset, npdu_len);
            }
            break;
        case BVLC_REGISTER_FOREIGN_DEVICE:
            debug_print_bip("Received-Register-Foreign-Device", addr);
            /* Upon receipt of a BVLL Register-Foreign-Device message, a BBMD
               shall start a timer with a value equal to the Time-to-Live
               parameter supplied plus a fixed grace period of 30 seconds. If,
               within the period during which the timer is active, another BVLL
               Register-Foreign-Device message from the same device is received,
               the timer shall be reset and restarted. If the time expires
               without the receipt of another BVLL Register-Foreign-Device
               message from the same foreign device, the FDT entry for this
               device shall be cleared. */
            function_len =
                bvlc_decode_register_foreign_device(pdu, pdu_len, &ttl_seconds);
            if (function_len) {
                if (bvlc_foreign_device_table_entry_add(
                        &FD_Table[0], addr, ttl_seconds)) {
                    result_code = BVLC_RESULT_SUCCESSFUL_COMPLETION;
                    send_result = true;
                } else {
                    result_code = BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK;
                    send_result = true;
                }
            }
            /* not an NPDU */
            offset = 0;
            break;
        case BVLC_READ_FOREIGN_DEVICE_TABLE:
            debug_print_bip("Received Read-FDT", addr);
            /* Upon receipt of a BVLL Read-Foreign-Device-Table message, a
               BBMD shall load the contents of its FDT into a BVLL Read-
               Foreign-Device-Table-Ack message and send it to the originating
               device. If the BBMD is unable to perform the read of its FDT,
               it shall return a BVLC-Result message to the originating device
               with a result code of X'0040' indicating that the read attempt
               has failed. */
            BVLC_Buffer_Len = bvlc_encode_read_foreign_device_table_ack(
                BVLC_Buffer, sizeof(BVLC_Buffer), &FD_Table[0]);
            if (BVLC_Buffer_Len > 0) {
                bip_send_mpdu(addr, BVLC_Buffer, BVLC_Buffer_Len);
            } else {
                result_code = BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK;
                send_result = true;
            }
            /* not an NPDU */
            offset = 0;
            break;
        case BVLC_READ_FOREIGN_DEVICE_TABLE_ACK:
            debug_print_bip("Received Read-FDT-Ack", addr);
            bbmd_read_fdt_ack_handler(addr, pdu, pdu_len);
            /* not an NPDU */
            offset = 0;
            break;
        case BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY:
            debug_print_bip("Received Delete-FDT-Entry", addr);
            /* Upon receipt of a BVLL Delete-Foreign-Device-Table-Entry
               message, a BBMD shall search its foreign device table for an
               entry corresponding to the B/IP address supplied in the message.
               If an entry is found, it shall be deleted and the BBMD shall
               return a BVLC-Result message to the originating device with a
               result code of X'0000'. Otherwise, the BBMD shall return a
               BVLCResult message to the originating device with a result code
               of X'0050' indicating that the deletion attempt has failed. */
            function_len =
                bvlc_decode_delete_foreign_device(pdu, pdu_len, &fwd_address);
            if (function_len > 0) {
                if (bvlc_foreign_device_table_entry_delete(
                        &FD_Table[0], &fwd_address)) {
                    result_code = BVLC_RESULT_SUCCESSFUL_COMPLETION;
                    send_result = true;
                } else {
                    result_code =
                        BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK;
                    send_result = true;
                }
            } else {
                result_code = BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK;
                send_result = true;
            }
            /* not an NPDU */
            offset = 0;
            break;
        case BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK:
            debug_print_bip("Received Distribute-Broadcast-To-Network", addr);
            /* Upon receipt of a BVLL Distribute-Broadcast-To-Network message
               from a foreign device, the receiving BBMD shall transmit a
               BVLL Forwarded-NPDU message on its local IP subnet using the
               local B/IP broadcast address as the destination address. In
               addition, a Forwarded-NPDU message shall be sent to each entry
               in its BDT as described in the case of the receipt of a
               BVLL Original-Broadcast-NPDU as well as directly to each foreign
               device currently in the BBMD's FDT except the originating
               node. If the BBMD is unable to perform the forwarding function,
               it shall return a BVLC-Result message to the foreign device
               with a result code of X'0060' indicating that the forwarding
               attempt was unsuccessful */
            npdu_len = bbmd_forward_npdu(addr, pdu, pdu_len);
            if (npdu_len > 0) {
                (void)bbmd_fdt_forward_npdu(addr, pdu, pdu_len, false);
                (void)bbmd_bdt_forward_npdu(addr, pdu, pdu_len, false);
            } else {
                result_code = BVLC_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK;
                send_result = true;
            }
            /* not an NPDU */
            offset = 0;
            break;
        case BVLC_ORIGINAL_UNICAST_NPDU:
            /* This message is used to send directed NPDUs to
               another B/IPv4 node or router. */
            debug_print_bip("Received Original-Unicast-NPDU", addr);
            if (bbmd_address_match_self(addr)) {
                /* ignore messages from my IPv4 address */
                debug_print_string("Dropped Original-Unicast-NPDU from me!");
                break;
            }
            function_len =
                bvlc_decode_original_unicast(pdu, pdu_len, NULL, 0, &npdu_len);
            if (function_len) {
                /* prepare the message for me! */
                bvlc_ip_address_to_bacnet_local(src, addr);
                offset = header_len + function_len - npdu_len;
                debug_print_npdu("Original-Unicast-NPDU", offset, npdu_len);
            } else {
                debug_print_string(
                    "Dropped Original-Broadcast-NPDU: Malformed!");
            }
            break;
        case BVLC_ORIGINAL_BROADCAST_NPDU:
            debug_print_bip("Received Original-Broadcast-NPDU", addr);
            if (bbmd_address_match_self(addr)) {
                /* ignore messages from my IPv4 address */
                debug_print_string("Dropped Original-Broadcast-NPDU from me!");
                break;
            }
            function_len = bvlc_decode_original_broadcast(
                pdu, pdu_len, NULL, 0, &npdu_len);
            if (function_len) {
                /* prepare the message for me! */
                bvlc_ip_address_to_bacnet_local(src, addr);
                offset = header_len + function_len - npdu_len;
                /* Upon receipt of a BVLL Original-Broadcast-NPDU message,
                   a BBMD shall construct a BVLL Forwarded-NPDU message and
                   send it to each IP subnet in its BDT with the exception
                   of its own. The B/IP address to which the Forwarded-NPDU
                   message is sent is formed by inverting the broadcast
                   distribution mask in the BDT entry and logically ORing it
                   with the BBMD address of the same entry. This process
                   produces either the directed broadcast address of the remote
                   subnet or the unicast address of the BBMD on that subnet
                   depending on the contents of the broadcast distribution
                   mask. See J.4.3.2.. In addition, the received BACnet NPDU
                   shall be sent directly to each foreign device currently in
                   the BBMD's FDT also using the BVLL Forwarded-NPDU message. */
                npdu = &mtu[offset];
                /* BTL test: verifies that the IUT will quietly discard any
                   Confirmed-Request-PDU, whose destination address is a
                   multicast or broadcast address, received from the
                   network layer. */
                if (npdu_confirmed_service(npdu, npdu_len)) {
                    offset = 0;
                    debug_print_string("Dropped Original-Broadcast-NPDU: "
                                       "Confirmed Service!");
                } else {
                    (void)bbmd_fdt_forward_npdu(addr, npdu, npdu_len, true);
                    (void)bbmd_bdt_forward_npdu(addr, npdu, npdu_len, true);
                    debug_print_npdu(
                        "Original-Broadcast-NPDU", offset, npdu_len);
                }
            } else {
                debug_print_string(
                    "Dropped Original-Broadcast-NPDU: Malformed!");
            }
            break;
        case BVLC_SECURE_BVLL:
            debug_print_bip("Received Secure-BVLL", addr);
            break;
        default:
            debug_print_unsigned("Unknown BVLC =", BVLC_Function_Code);
            break;
    }
    if (send_result) {
        bvlc_send_result(addr, result_code);
        debug_print_unsigned("Sent result code =", result_code);
    }

    return offset;
}
#endif

/**
 * Use this handler for BACnet/IPv4 BVLC
 *
 * @param addr [in] IPv4 address to send any NAK back to.
 * @param src [out] returns the source address
 * @param npdu [in] The received buffer.
 * @param npdu_len [in] How many bytes in npdu[].
 *
 * @return number of bytes offset into the NPDU for APDU, or 0 if handled
 */
int bvlc_handler(BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *npdu,
    uint16_t npdu_len)
{
#if BBMD_ENABLED
    debug_print_bip("Received BVLC (BBMD Enabled)", addr);
    return bvlc_bbmd_enabled_handler(addr, src, npdu, npdu_len);
#else
    debug_print_bip("Received BVLC (BBMD Disabled)", addr);
    return bvlc_bbmd_disabled_handler(addr, src, npdu, npdu_len);
#endif
}

int bvlc_broadcast_handler(BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *npdu,
    uint16_t npdu_len)
{
    int offset = 0;
    uint8_t message_type = 0;
    uint16_t message_length = 0;
    int header_len = 0;

    debug_print_bip("Received Broadcast", addr);
    header_len =
        bvlc_decode_header(npdu, npdu_len, &message_type, &message_length);
    if (header_len == 4) {
        switch (message_type) {
            case BVLC_ORIGINAL_UNICAST_NPDU:
                /* drop unicast when sent as a broadcast */
                debug_print_bip("Dropped BVLC (Original Unicast)", addr);
                break;
            default:
                offset = bvlc_handler(addr, src, npdu, npdu_len);
                break;
        }
    }

    return offset;
}

#if BBMD_CLIENT_ENABLED
/** Register as a foreign device with the indicated BBMD.
 * @param bbmd_addr - IPv4 address of BBMD with which to register
 * @param ttl_seconds - Lease time to use when registering.
 * @return Positive number (of bytes sent) on success,
 *         0 if no registration request is sent, or
 *         -1 if registration fails.
 */
int bvlc_register_with_bbmd(
    const BACNET_IP_ADDRESS *bbmd_addr, uint16_t ttl_seconds)
{
    /* Store the BBMD address and port so that we won't broadcast locally. */
    /* We are a foreign device! */
    bvlc_address_copy(&Remote_BBMD, bbmd_addr);
    Remote_BBMD_TTL_Seconds = ttl_seconds;
    BVLC_Buffer_Len = bvlc_encode_register_foreign_device(
        &BVLC_Buffer[0], sizeof(BVLC_Buffer), ttl_seconds);

    return bip_send_mpdu(bbmd_addr, &BVLC_Buffer[0], BVLC_Buffer_Len);
}

/** Get the remote BBMD address that was used to Register as a foreign device
 * @param bbmd_addr - IPv4 address of BBMD with which to register
 * @return Positive number (of bytes sent) on success,
 *         0 if no registration request is sent, or
 *         -1 if registration fails.
 */
void bvlc_remote_bbmd_address(BACNET_IP_ADDRESS *bbmd_addr)
{
    bvlc_address_copy(bbmd_addr, &Remote_BBMD);
}

/**
 * @brief Get the remote BBMD time-to-live seconds used to
 *  Register Foreign Device
 * @return Lease time in seconds to use when registering.
 */
uint16_t bvlc_remote_bbmd_lifetime(void)
{
    return Remote_BBMD_TTL_Seconds;
}
#endif

#if BBMD_CLIENT_ENABLED
/**
 * Read the BDT from the indicated BBMD
 * @param bbmd_addr - IPv4 address of BBMD with which to read
 * @return Positive number (of bytes sent) on success
 */
int bvlc_bbmd_read_bdt(const BACNET_IP_ADDRESS *bbmd_addr)
{
    BVLC_Buffer_Len = bvlc_encode_read_broadcast_distribution_table(
        &BVLC_Buffer[0], sizeof(BVLC_Buffer));

    return bip_send_mpdu(bbmd_addr, &BVLC_Buffer[0], BVLC_Buffer_Len);
}

/**
 * Write the BDT to the indicated BBMD
 * @param bbmd_addr - IPv4 address of BBMD with which to read
 * @return Positive number (of bytes sent) on success
 */
int bvlc_bbmd_write_bdt(const BACNET_IP_ADDRESS *bbmd_addr,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)
{
    BVLC_Buffer_Len = bvlc_encode_write_broadcast_distribution_table(
        &BVLC_Buffer[0], sizeof(BVLC_Buffer), bdt_list);

    return bip_send_mpdu(bbmd_addr, &BVLC_Buffer[0], BVLC_Buffer_Len);
}

/**
 * Read the FDT from the indicated BBMD
 * @param bbmd_addr - IPv4 address of BBMD with which to read
 * @return Positive number (of bytes sent) on success
 */
int bvlc_bbmd_read_fdt(const BACNET_IP_ADDRESS *bbmd_addr)
{
    BVLC_Buffer_Len = bvlc_encode_read_foreign_device_table(
        &BVLC_Buffer[0], sizeof(BVLC_Buffer));

    return bip_send_mpdu(bbmd_addr, &BVLC_Buffer[0], BVLC_Buffer_Len);
}
#endif

/**
 * Returns the last BVLL Result we received, either as the result of a BBMD
 * request we sent, or (if not a BBMD or Client), from trying to register
 * as a foreign device.
 *
 * @return BVLC_RESULT_SUCCESSFUL_COMPLETION on success,
 * BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK if registration failed,
 * or one of the other codes (if we are a BBMD).
 */
uint16_t bvlc_get_last_result(void)
{
    return BVLC_Result_Code;
}

/**
 * Sets the last BVLL Result we received
 * @param result code
 */
void bvlc_set_last_result(uint16_t result_code)
{
    BVLC_Result_Code = result_code;
}

/**
 * Returns the current BVLL Function Code we are processing.
 * We have to store this higher layer code for when the lower layers
 * need to know what it is, especially to differentiate between
 * BVLC_ORIGINAL_UNICAST_NPDU and BVLC_ORIGINAL_BROADCAST_NPDU.
 *
 * @return A BVLC_ code, such as BVLC_ORIGINAL_UNICAST_NPDU.
 */
uint8_t bvlc_get_function_code(void)
{
    return BVLC_Function_Code;
}

/**
 * Sets the current BVLL Function Code
 * @param A BVLC_ code, such as BVLC_ORIGINAL_UNICAST_NPDU.
 */
void bvlc_set_function_code(uint8_t function_code)
{
    BVLC_Function_Code = function_code;
}

#if BBMD_ENABLED
/**
 * @brief Get handle to foreign device table (FDT).
 * @return pointer to first entry of foreign device table
 */
BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *bvlc_fdt_list(void)
{
    return &FD_Table[0];
}

/**
 * @brief Get handle to broadcast distribution table (BDT).
 * @return pointer to first entry of broadcast distribution table
 */
BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bvlc_bdt_list(void)
{
    return &BBMD_Table[0];
}

/**
 * @brief Invalidate all entries in the broadcast distribution table (BDT).
 */
void bvlc_bdt_list_clear(void)
{
    bvlc_broadcast_distribution_table_valid_clear(&BBMD_Table[0]);
    /* BDT changed! Save backup to file */
    bvlc_bdt_backup_local();
}
#endif

/**
 * @brief Enable NAT handling and set the global IP address
 *
 * If the communication between BBMDs goes through a NAT enabled internet
 * router, special considerations are needed as stated in Annex J.7.8.
 *
 * In short, the local IP address of the BBMD is different than the global
 * address which is visible to the other BBMDs or foreign devices. This is
 * why the source address in forwarded messages needs to be changed to the
 * global IP address.
 *
 * For other considerations/limitations see Annex J.7.8.
 *
 * @param [in] - Global IP address visible to peer BBMDs and foreign devices
 */
void bvlc_set_global_address_for_nat(const BACNET_IP_ADDRESS *addr)
{
    bvlc_address_copy(&BVLC_Global_Address, addr);
    BVLC_NAT_Handling = true;
    debug_print_bip("NAT Address enabled", addr);
}

/**
 * @brief Disable NAT handling.
 */
void bvlc_disable_nat(void)
{
    BVLC_NAT_Handling = false;
    debug_print_string("NAT Address disabled");
}

void bvlc_init(void)
{
#if BBMD_ENABLED
    debug_print_string("Initializing (BBMD Enabled).");
    bvlc_broadcast_distribution_table_link_array(
        &BBMD_Table[0], MAX_BBMD_ENTRIES);
    bvlc_foreign_device_table_link_array(&FD_Table[0], MAX_FD_ENTRIES);
#else
    debug_print_string("Initializing (BBMD Disabled).");
#endif
}
