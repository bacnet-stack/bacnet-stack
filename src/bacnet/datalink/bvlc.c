/**
 * @file
 * @brief BACnet/IP virtual link control module encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 * @defgroup DLBIP BACnet/IP DataLink Network Layer
 * @ingroup DataLink
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/hostnport.h"
#include "bacnet/datalink/bvlc.h"

/**
 * @brief Encode the BVLC header
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param message_type - BVLL Messages
 * @param length - number of bytes for this message type
 *
 * @return number of bytes encoded
 */
int bvlc_encode_header(
    uint8_t *pdu, uint16_t pdu_size, uint8_t message_type, uint16_t length)
{
    int bytes_encoded = 0;

    if (pdu && (pdu_size >= 2)) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = message_type;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], length);
        bytes_encoded = 4;
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Result message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param message_type - BVLL Messages
 * @param message_length - number of bytes for this message type
 *
 * @return number of bytes decoded
 */
int bvlc_decode_header(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *message_type,
    uint16_t *message_length)
{
    int bytes_consumed = 0;

    if (pdu && (pdu_len >= 4)) {
        if (pdu[0] == BVLL_TYPE_BACNET_IP) {
            if (message_type) {
                *message_type = pdu[1];
            }
            if (message_length) {
                decode_unsigned16(&pdu[2], message_length);
            }
            bytes_consumed = 4;
        }
    }

    return bytes_consumed;
}

/**
 * @brief J.2.1 BVLC-Result: Encode
 *
 * This message provides a mechanism to acknowledge the result
 * of those BVLL service requests that require an acknowledgment,
 * whether successful (ACK) or unsuccessful (NAK).
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param result_code - BVLC result code
 *
 * @return number of bytes encoded
 *
 * BVLC Type:     1-octet   X'81'   BVLL for BACnet/IPv4
 * BVLC Function: 1-octet   X'00'   BVLC-Result
 * BVLC Length:   2-octets  X'0006' Length of the BVLL message
 * Result Code:   2-octets  X'0000' Successful completion
 *                          X'0010' Write-Broadcast-Distribution-Table NAK
 *                          X'0020' Read-Broadcast-Distribution-Table NAK
 *                          X'0030' Register-Foreign-Device NAK
 *                          X'0040' Read-Foreign-Device-Table NAK
 *                          X'0050' Delete-Foreign-Device-Table-Entry NAK
 *                          X'0060' Distribute-Broadcast-To-Network NAK
 */
int bvlc_encode_result(uint8_t *pdu, uint16_t pdu_size, uint16_t result_code)
{
    int bytes_encoded = 0;
    const uint16_t length = 6;

    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(pdu, pdu_size, BVLC_RESULT, length);
        if (bytes_encoded == 4) {
            encode_unsigned16(&pdu[4], result_code);
            bytes_encoded = (int)length;
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Result message, after header is decoded
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param result_code - BVLC result code
 *
 * @return number of bytes decoded
 */
int bvlc_decode_result(
    const uint8_t *pdu, uint16_t pdu_len, uint16_t *result_code)
{
    int bytes_consumed = 0;
    const uint16_t length = 2;

    if (pdu && (pdu_len >= length)) {
        if (result_code) {
            decode_unsigned16(&pdu[0], result_code);
        }
        bytes_consumed = (int)length;
    }

    return bytes_consumed;
}

/**
 * @brief Copy the BVLC Broadcast Distribution Mask
 * @param dst - BVLC Broadcast Distribution Mask that will be filled with src
 * @param src - BVLC Broadcast Distribution Mask that will be copied into dst
 * @return true if the mask was copied
 */
bool bvlc_broadcast_distribution_mask_copy(
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK *dst,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *src)
{
    bool status = false;
    unsigned int i = 0;

    if (src && dst) {
        for (i = 0; i < IP_ADDRESS_MAX; i++) {
            dst->address[i] = src->address[i];
        }
        status = true;
    }

    return status;
}

/**
 * @brief Compare the BVLC Broadcast Distribution Masks
 * @param dst - BVLC Broadcast Distribution Mask that will be compared to src
 * @param src - BVLC Broadcast Distribution Mask that will be compared to dst
 * @return true if the masks are different
 */
bool bvlc_broadcast_distribution_mask_different(
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *dst,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *src)
{
    bool status = false;
    unsigned int i = 0;

    if (src && dst) {
        for (i = 0; i < IP_ADDRESS_MAX; i++) {
            if (dst->address[i] != src->address[i]) {
                status = true;
            }
        }
    }

    return status;
}

/**
 * @brief Compare the Broadcast-Distribution-Table entry
 * @param dst - Broadcast-Distribution-Table entry that will be compared to src
 * @param src - Broadcast-Distribution-Table entry that will be compared to dst
 * @return true if the addresses are different
 */
bool bvlc_broadcast_distribution_table_entry_different(
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *dst,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *src)
{
    bool status = false;

    if (src && dst) {
        status = bvlc_address_different(&dst->dest_address, &src->dest_address);
        if (!status) {
            status = bvlc_broadcast_distribution_mask_different(
                &dst->broadcast_mask, &src->broadcast_mask);
        }
    }

    return status;
}

/**
 * @brief Copy the Broadcast-Distribution-Table entry
 * @param dst - Broadcast-Distribution-Table entry that will be filled with src
 * @param src - Broadcast-Distribution-Table entry that will be copied into dst
 * @return true if the address was copied
 */
bool bvlc_broadcast_distribution_table_entry_copy(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *dst,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *src)
{
    bool status = false;

    if (src && dst) {
        status = bvlc_address_copy(&dst->dest_address, &src->dest_address);
        if (status) {
            status = bvlc_broadcast_distribution_mask_copy(
                &dst->broadcast_mask, &src->broadcast_mask);
        }
    }

    return status;
}

/**
 * @brief Count the number of valid Write-Broadcast-Distribution-Table entries
 *
 * @param bdt_list - first element in array BDT entries
 * @return number of elements of BDT entries that are valid
 */
uint16_t bvlc_broadcast_distribution_table_valid_count(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)
{
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry = NULL;
    uint16_t bdt_entry_count = 0;

    /* count the number of entries */
    bdt_entry = bdt_list;
    while (bdt_entry) {
        if (bdt_entry->valid) {
            bdt_entry_count++;
        }
        bdt_entry = bdt_entry->next;
    }

    return bdt_entry_count;
}

/**
 * @brief Clear all Write-Broadcast-Distribution-Table entries
 * @param bdt_list - first element in array BDT entries
 */
void bvlc_broadcast_distribution_table_valid_clear(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)
{
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry;

    /* count the number of entries */
    bdt_entry = bdt_list;
    while (bdt_entry) {
        bdt_entry->valid = false;
        bdt_entry = bdt_entry->next;
    }
}

/**
 * @brief Count the total number of Write-Broadcast-Distribution-Table entries
 *
 * @param bdt_list - first element in array BDT entries
 * @return number of elements of BDT entries
 */
uint16_t bvlc_broadcast_distribution_table_count(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)
{
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry = NULL;
    uint16_t bdt_entry_count = 0;

    /* count the number of entries */
    bdt_entry = bdt_list;
    while (bdt_entry) {
        bdt_entry_count++;
        bdt_entry = bdt_entry->next;
    }

    return bdt_entry_count;
}

/**
 * @brief Convert Write-Broadcast-Distribution-Table entry array
 *  into linked list.
 *
 * @param bdt_list - first element in array BDT entries
 * @param bdt_array_size - number of array elements of BDT entries
 */
void bvlc_broadcast_distribution_table_link_array(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list,
    const size_t bdt_array_size)
{
    size_t i = 0;

    for (i = 0; i < bdt_array_size; i++) {
        if (i > 0) {
            bdt_list[i - 1].next = &bdt_list[i];
        }
        bdt_list[i].next = NULL;
    }
}

/**
 * @brief Append an entry to the Broadcast-Distribution-Table
 * @param bdt_list - first entry in list of BDT entries
 * @param bdt_new - new entry to append to list of BDT entries
 * @return true if the Broadcast-Distribution-Table entry was appended
 */
bool bvlc_broadcast_distribution_table_entry_append(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_new)
{
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry = NULL;
    bool status = false;

    bdt_entry = bdt_list;
    while (bdt_entry) {
        if (bdt_entry->valid) {
            if (!bvlc_broadcast_distribution_table_entry_different(
                    bdt_entry, bdt_new)) {
                status = true;
                break;
            }
        } else {
            /* first empty slot! Assume the remaining are empty. */
            status = true;
            /* Copy new entry to the empty slot */
            bvlc_broadcast_distribution_table_entry_copy(bdt_entry, bdt_new);
            bdt_entry->valid = true;
            break;
        }
        bdt_entry = bdt_entry->next;
    }

    return status;
}

/**
 * @brief Set an entry to the Broadcast-Distribution-Table
 * @param bdt_entry - first element in list of BDT entries
 * @param addr - B/IPv4 address to match, along with mask
 * @param mask - BVLC Broadcast Distribution Mask to match, along with addr
 * @return true if the Broadcast Distribution entry was set
 */
bool bvlc_broadcast_distribution_table_entry_set(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry,
    const BACNET_IP_ADDRESS *addr,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask)
{
    bool status = false;

    if (bdt_entry && addr && mask) {
        status = bvlc_address_copy(&bdt_entry->dest_address, addr);
        if (status) {
            status = bvlc_broadcast_distribution_mask_copy(
                &bdt_entry->broadcast_mask, mask);
        }
    }

    return status;
}

/**
 * @brief Set the Broadcast-Distribution-Table entry distribution mask
 * @param mask - broadcast distribution mask
 * @param broadcast_mask - 32-bit broadcast mask in host byte order
 * @return true if the broadcast distribution was set
 */
bool bvlc_broadcast_distribution_mask_from_host(
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask, uint32_t broadcast_mask)
{
    bool status = false;

    if (mask) {
        encode_unsigned32(mask->address, broadcast_mask);
        status = true;
    }

    return status;
}

/**
 * @brief Get the Broadcast-Distribution-Table entry distribution mask
 * @param broadcast_mask - 32-bit broadcast mask in host byte order
 * @param mask - broadcast distribution mask
 * @return true if the broadcast distribution was retrieved
 */
bool bvlc_broadcast_distribution_mask_to_host(
    uint32_t *broadcast_mask, const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask)
{
    bool status = false;

    if (broadcast_mask && mask) {
        decode_unsigned32(mask->address, broadcast_mask);
        status = true;
    }

    return status;
}

/**
 * @brief Set the Broadcast-Distribution-Table entry distribution mask
 * @param mask - broadcast distribution mask
 * @param addr0 - broadcast distribution mask octet
 * @param addr1 - broadcast distribution mask octet
 * @param addr2 - broadcast distribution mask octet
 * @param addr3 - broadcast distribution mask octet
 */
void bvlc_broadcast_distribution_mask_set(
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask,
    uint8_t addr0,
    uint8_t addr1,
    uint8_t addr2,
    uint8_t addr3)
{
    if (mask) {
        mask->address[0] = addr0;
        mask->address[1] = addr1;
        mask->address[2] = addr2;
        mask->address[3] = addr3;
    }
}

/**
 * @brief Get the Broadcast-Distribution-Table entry distribution mask
 * @param mask - broadcast distribution mask
 * @param addr0 - broadcast distribution mask octet
 * @param addr1 - broadcast distribution mask octet
 * @param addr2 - broadcast distribution mask octet
 * @param addr3 - broadcast distribution mask octet
 */
void bvlc_broadcast_distribution_mask_get(
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask,
    uint8_t *addr0,
    uint8_t *addr1,
    uint8_t *addr2,
    uint8_t *addr3)
{
    if (mask) {
        if (addr0) {
            *addr0 = mask->address[0];
        }
        if (addr1) {
            *addr1 = mask->address[1];
        }
        if (addr2) {
            *addr2 = mask->address[2];
        }
        if (addr3) {
            *addr3 = mask->address[3];
        }
    }
}

/**
 * @brief Set the B/IP address for a Forwarded-NPDU message
 *
 * The B/IP address to which the Forwarded-NPDU message is
 * sent is formed by inverting the broadcast distribution
 * mask in the BDT entry and logically ORing it with the
 * BBMD address of the same entry.
 *
 * @param addr - B/IPv4 address to match, along with mask
 * @param bdt_entry - The BDT entry containing an address and mask
 * @return true if the B/IPv4 address was set
 */
bool bvlc_broadcast_distribution_table_entry_forward_address(
    BACNET_IP_ADDRESS *addr,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)
{
    bool status = false;

    if (bdt_entry && addr) {
        status = bvlc_address_mask(
            addr, &bdt_entry->dest_address, &bdt_entry->broadcast_mask);
    }

    return status;
}

/**
 * @brief Encode the Broadcast-Distribution-Table for Network Port object
 *
 *    BACnetLIST of BACnetBDTEntry
 *
 *    BACnetBDTEntry ::= SEQUENCE {
 *       bbmd-address [0] BACnetHostNPort,
 *           BACnetHostNPort ::= SEQUENCE {
 *               host [0] BACnetHostAddress,
 *                   BACnetHostAddress ::= CHOICE {
 *                       ip-address [1] OCTET STRING, -- 4 octets for B/IP
 *                   }
 *               port [1] Unsigned16
 *           }
 *        broadcast-mask [1] OCTET STRING
 *    }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param bdt_head - one BACnetBDTEntry
 * @return length of the APDU buffer
 */
int bvlc_broadcast_distribution_table_entry_encode(
    uint8_t *apdu,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OCTET_STRING octet_string;

    if (bdt_entry) {
        /* bbmd-address [0] BACnetHostNPort - opening */
        len = encode_opening_tag(apdu, 0);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /*  host [0] BACnetHostAddress - opening */
        len = encode_opening_tag(apdu, 0);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* CHOICE - ip-address [1] OCTET STRING */
        octetstring_init(
            &octet_string, &bdt_entry->dest_address.address[0], IP_ADDRESS_MAX);
        len = encode_context_octet_string(apdu, 1, &octet_string);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /*  host [0] BACnetHostAddress - closing */
        len = encode_closing_tag(apdu, 0);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* port [1] Unsigned16 */
        len = encode_context_unsigned(apdu, 1, bdt_entry->dest_address.port);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* bbmd-address [0] BACnetHostNPort - closing */
        len = encode_closing_tag(apdu, 0);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* broadcast-mask [1] OCTET STRING */
        octetstring_init(
            &octet_string, &bdt_entry->broadcast_mask.address[0],
            IP_ADDRESS_MAX);
        len = encode_context_octet_string(apdu, 1, &octet_string);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the Broadcast-Distribution-Table for Network Port object
 *
 *    BACnetLIST of BACnetBDTEntry
 *
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer size
 * @param bdt_head - head of the BDT linked list
 * @return length of the APDU buffer
 */
int bvlc_broadcast_distribution_table_list_encode(
    uint8_t *apdu, const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)
{
    int len = 0;
    int apdu_len = 0;
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry;

    bdt_entry = bdt_head;
    while (bdt_entry) {
        if (bdt_entry->valid) {
            len =
                bvlc_broadcast_distribution_table_entry_encode(apdu, bdt_entry);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        } else {
            len = 0;
        }
        /* next entry */
        bdt_entry = bdt_entry->next;
    }

    return apdu_len;
}

/**
 * @brief Encode the Broadcast-Distribution-Table for Network Port object
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer size
 * @param bdt_head - head of the BDT linked list
 * @return length of the APDU buffer, or BACNET_STATUS_ERROR on error
 */
int bvlc_broadcast_distribution_table_encode(
    uint8_t *apdu,
    uint16_t apdu_size,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)
{
    int len = 0;

    len = bvlc_broadcast_distribution_table_list_encode(NULL, bdt_head);
    if (len <= apdu_size) {
        len = bvlc_broadcast_distribution_table_list_encode(apdu, bdt_head);
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief Decode the Broadcast-Distribution-Table for Network Port object
 * @param apdu - the APDU buffer
 * @param apdu_len - the APDU buffer length
 * @param bdt_head - head of a BDT linked list
 * @return length of the APDU buffer decoded, or ERROR, REJECT, or ABORT
 */
int bvlc_broadcast_distribution_table_decode(
    const uint8_t *apdu,
    uint16_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)
{
    int len = 0;
    BACNET_OCTET_STRING octet_string = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry = NULL;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* default reject code */
    if (error_code) {
        *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
    }
    /* check for value pointers */
    if ((apdu_len == 0) || (!apdu)) {
        return BACNET_STATUS_REJECT;
    }
    bdt_entry = bdt_head;
    while (bdt_entry) {
        /* bbmd-address [0] BACnetHostNPort - opening */
        if (!decode_is_opening_tag_number(&apdu[len++], 0)) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        /* host [0] BACnetHostAddress - opening */
        if (!decode_is_opening_tag_number(&apdu[len++], 0)) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        /* CHOICE - ip-address [1] OCTET STRING */
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 1) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        len += decode_octet_string(&apdu[len], len_value_type, &octet_string);
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        (void)octetstring_copy_value(
            &bdt_entry->dest_address.address[0], IP_ADDRESS_MAX, &octet_string);
        /*  host [0] BACnetHostAddress - closing */
        if (!decode_is_closing_tag_number(&apdu[len++], 0)) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        /* port [1] Unsigned16 */
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 1) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        len += decode_unsigned(&apdu[len], len_value_type, &unsigned_value);
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        if (unsigned_value <= UINT16_MAX) {
            bdt_entry->dest_address.port = unsigned_value;
        } else {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
            }
            return BACNET_STATUS_REJECT;
        }
        /* bbmd-address [0] BACnetHostNPort - closing */
        if (!decode_is_closing_tag_number(&apdu[len++], 0)) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        /* broadcast-mask [1] OCTET STRING */
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 1) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        len += decode_octet_string(&apdu[len], len_value_type, &octet_string);
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        (void)octetstring_copy_value(
            &bdt_entry->broadcast_mask.address[0], IP_ADDRESS_MAX,
            &octet_string);
        bdt_entry->valid = true;
        /* next entry */
        bdt_entry = bdt_entry->next;
    }

    return apdu_len;
}

/**
 * @brief J.2.2 Write-Broadcast-Distribution-Table: encode
 *
 * This message provides a mechanism for initializing or updating a
 * Broadcast Distribution Table (BDT) in a BACnet Broadcast Management
 * Device (BBMD).
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param bdt_list - list of BDT entries
 *
 * @return number of bytes encoded
 *
 * BVLC Type:           1-octet  X'81' BVLL for BACnet/IP
 * BVLC Function:       1-octet  X'01' Write-Broadcast-Distribution-Table
 * BVLC Length:         2-octets    L  Length L, in octets, of the BVLL message
 * List of BDT Entries: N*10-octets
 */
int bvlc_encode_write_broadcast_distribution_table(
    uint8_t *pdu,
    uint16_t pdu_size,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)
{
    int bytes_encoded = 0;
    int len = 0;
    uint16_t offset = 0;
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry = NULL;
    uint16_t bdt_entry_count = 0;
    uint16_t length = 0;

    /* count the number of entries */
    bdt_entry_count = bvlc_broadcast_distribution_table_valid_count(bdt_list);
    length = 4 + (bdt_entry_count * BACNET_IP_BDT_ENTRY_SIZE);
    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE, length);
        if (bytes_encoded == 4) {
            offset = 4;
            /* encode the entries */
            bdt_entry = bdt_list;
            while (bdt_entry) {
                if (bdt_entry->valid) {
                    len = bvlc_encode_broadcast_distribution_table_entry(
                        &pdu[offset], pdu_size - offset, bdt_entry);
                    offset += len;
                }
                bdt_entry = bdt_entry->next;
            }
            bytes_encoded = offset;
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the Write-Broadcast-Distribution-Table
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param bdt_list - BDT Entry list
 *
 * @return number of bytes decoded
 */
int bvlc_decode_write_broadcast_distribution_table(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)
{
    int bytes_consumed = 0;
    int len = 0;
    uint16_t offset = 0;
    uint16_t pdu_bytes = 0;
    uint16_t bdt_entry_count = 0;
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry = NULL;
    uint16_t list_len = 0;

    /* count the number of available entries */
    bdt_entry_count = bvlc_broadcast_distribution_table_count(bdt_list);
    list_len = bdt_entry_count * BACNET_IP_BDT_ENTRY_SIZE;
    /* will the entries fit */
    if (pdu && (pdu_len <= list_len)) {
        bdt_entry = bdt_list;
        while (bdt_entry) {
            pdu_bytes = pdu_len - offset;
            if (pdu_bytes >= BACNET_IP_BDT_ENTRY_SIZE) {
                len = bvlc_decode_broadcast_distribution_table_entry(
                    &pdu[offset], pdu_bytes, bdt_entry);
                if (len > 0) {
                    bdt_entry->valid = true;
                }
                offset += len;
            } else {
                bdt_entry->valid = false;
            }
            bdt_entry = bdt_entry->next;
        }
        bytes_consumed = (int)offset;
    }

    return bytes_consumed;
}

/**
 * @brief J.2.3 Read-Broadcast-Distribution-Table: encode
 *
 * The message provides a mechanism for retrieving the contents of a BBMD's
 * BDT.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param bdt_list - list of BDT entries
 *
 * @return number of bytes encoded
 *
 * BVLC Type:           1-octet  X'81'   BVLL for BACnet/IP
 * BVLC Function:       1-octet  X'02'   Read-Broadcast-Distribution-Table
 * BVLC Length:         2-octets X'0004' Length, in octets, of the BVLL message
 */
int bvlc_encode_read_broadcast_distribution_table(
    uint8_t *pdu, uint16_t pdu_size)
{
    int bytes_encoded = 0;
    uint16_t length = 1 + 1 + 2;

    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_READ_BROADCAST_DIST_TABLE, length);
    }

    return bytes_encoded;
}

/**
 * @brief J.2.4 Read-Broadcast-Distribution-Table-Ack: encode
 *
 * The message provides a mechanism for retrieving the contents of a BBMD's
 * BDT.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param bdt_list - list of BDT entries
 *
 * @return number of bytes encoded
 *
 * BVLC Type:           1-octet  X'81'   BVLL for BACnet/IP
 * BVLC Function:       1-octet  X'02'   Read-Broadcast-Distribution-Table
 * BVLC Length:         2-octets L       length, in octets, of the BVLL message
 * List of BDT Entries: N*10-octets
 */
int bvlc_encode_read_broadcast_distribution_table_ack(
    uint8_t *pdu,
    uint16_t pdu_size,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)
{
    int bytes_encoded = 0;
    int len = 0;
    uint16_t offset = 0;
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry = NULL;
    uint16_t bdt_entry_count = 0;
    uint16_t length = 0;

    /* count the number of entries */
    bdt_entry_count = bvlc_broadcast_distribution_table_valid_count(bdt_list);
    length = 4 + (bdt_entry_count * BACNET_IP_BDT_ENTRY_SIZE);
    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_READ_BROADCAST_DIST_TABLE_ACK, length);
        if (bytes_encoded == 4) {
            offset = 4;
            /* encode the entries */
            bdt_entry = bdt_list;
            while (bdt_entry) {
                if (bdt_entry->valid) {
                    len = bvlc_encode_broadcast_distribution_table_entry(
                        &pdu[offset], pdu_size - offset, bdt_entry);
                    offset += len;
                }
                bdt_entry = bdt_entry->next;
            }
            bytes_encoded = (int)length;
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the Read-Broadcast-Distribution-Table-Ack
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param bdt_list - list of BDT Entries to be overwritten
 *
 * @return number of bytes decoded
 */
int bvlc_decode_read_broadcast_distribution_table_ack(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)
{
    int bytes_consumed = 0;
    int len = 0;
    uint16_t offset = 0;
    uint16_t pdu_bytes = 0;
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry = NULL;

    if (pdu && (pdu_len >= BACNET_IP_BDT_ENTRY_SIZE)) {
        bdt_entry = bdt_list;
        while (bdt_entry) {
            pdu_bytes = pdu_len - offset;
            if (pdu_bytes >= BACNET_IP_BDT_ENTRY_SIZE) {
                len = bvlc_decode_broadcast_distribution_table_entry(
                    &pdu[offset], pdu_bytes, bdt_entry);
                if (len > 0) {
                    bdt_entry->valid = true;
                }
                offset += len;
            } else {
                bdt_entry->valid = false;
            }
            bdt_entry = bdt_entry->next;
        }
        bytes_consumed = (int)offset;
    }

    return bytes_consumed;
}

/**
 * @brief J.2.5 Forwarded-NPDU: Encode
 *
 * This BVLL message is used in broadcast messages from a BBMD
 * as well as in messages forwarded to registered foreign devices.
 * It contains the source address of the original node, or
 * if NAT is being used, the address with which the original node
 * is accessed, as well as the original BACnet NPDU
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param bip_address - Original-Source-B/IPv4-Address
 * @param npdu - BACnet NPDU from Originating Device buffer
 * @param npdu_len - size of the BACnet NPDU buffer
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'81'   BVLL for BACnet/IP
 * BVLC Function:               1-octet   X'04'   Forwarded-NPDU
 * BVLC Length:                 2-octets  L       Length of the BVLL message
 * B/IP Address of Originating Device:   6-octets
 * BACnet NPDU from Originating Device:  N-octets (N=L-10)
 */
int bvlc_encode_forwarded_npdu(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_IP_ADDRESS *bip_address,
    const uint8_t *npdu,
    uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 1 + 1 + 2 + BIP_ADDRESS_MAX;
    uint16_t i = 0;
    uint16_t offset = 0;

    length += npdu_len;
    if (pdu && (pdu_size >= length)) {
        bytes_encoded =
            bvlc_encode_header(pdu, pdu_size, BVLC_FORWARDED_NPDU, length);
        if (bytes_encoded == 4) {
            offset = 4;
            bvlc_encode_address(&pdu[offset], pdu_size - offset, bip_address);
            offset += BIP_ADDRESS_MAX;
            if (npdu && (length > 0)) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[offset + i] = npdu[i];
                }
            }
            bytes_encoded = (int)length;
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Forwarded-NPDU message, after decoded header
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param bip_address - Original-Source-B/IPv4-Address
 * @param npdu - BACnet NPDU buffer
 * @param npdu_size - size of the buffer for the decoded BACnet NPDU
 * @param npdu_len - decoded length of the BACnet NPDU buffer
 *
 * @return number of bytes decoded
 */
int bvlc_decode_forwarded_npdu(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_ADDRESS *bip_address,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len)
{
    int bytes_consumed = 0;
    uint16_t length = 0;
    uint16_t i = 0;
    uint16_t offset = 0;

    if (pdu && (pdu_len >= BIP_ADDRESS_MAX)) {
        if (bip_address) {
            bvlc_decode_address(&pdu[offset], pdu_len - offset, bip_address);
        }
        offset += BIP_ADDRESS_MAX;
        length = pdu_len - BIP_ADDRESS_MAX;
        if (npdu && (length <= npdu_size)) {
            for (i = 0; i < length; i++) {
                npdu[i] = pdu[offset + i];
            }
        }
        if (npdu_len) {
            *npdu_len = length;
        }
        bytes_consumed = (int)pdu_len;
    }

    return bytes_consumed;
}

/**
 * @brief J.2.6 Register-Foreign-Device: encode
 *
 * This message allows a foreign device, as defined in Clause J.5.1,
 * to register with a BBMD for the purpose of receiving broadcast messages.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param ttl_seconds - Time-to-Live T, in seconds
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'81'   BVLL for BACnet/IP
 * BVLC Function:               1-octet   X'05'   Register-Foreign-Device
 * BVLC Length:                 2-octets  X'0006' Length of the BVLL message
 * Time-to-Live:                2-octets  T       Time-to-Live T, in seconds
 */
int bvlc_encode_register_foreign_device(
    uint8_t *pdu, uint16_t pdu_size, uint16_t ttl_seconds)
{
    int bytes_encoded = 0;
    const uint16_t length = 6;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_REGISTER_FOREIGN_DEVICE, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned16(&pdu[offset], ttl_seconds);
            bytes_encoded = (int)length;
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Register-Foreign-Device message, after decoded header
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param ttl_seconds - Time-to-Live T, in seconds
 *
 * @return number of bytes decoded
 */
int bvlc_decode_register_foreign_device(
    const uint8_t *pdu, uint16_t pdu_len, uint16_t *ttl_seconds)
{
    int bytes_consumed = 0;
    const uint16_t length = 2;
    uint16_t offset = 0;

    if (pdu && (pdu_len >= length)) {
        if (ttl_seconds) {
            decode_unsigned16(&pdu[offset], ttl_seconds);
        }
        bytes_consumed = (int)length;
    }

    return bytes_consumed;
}

/**
 * @brief Encode the Foreign_Device-Table for Network Port object
 *
 *    BACnetLIST of BACnetFDTEntry
 *
 *    BACnetFDTEntry ::= SEQUENCE {
 *        bacnetip-address [0] OCTET STRING, -- 6-octet B/IP registrant address
 *        time-to-live [1] Unsigned16, -- time to live in seconds
 *        remaining-time-to-live [2] Unsigned16 -- remaining time in seconds
 *    }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param fdt_head - head of the BDT linked list
 * @return length of the APDU buffer
 */
int bvlc_foreign_device_table_entry_encode(
    uint8_t *apdu, const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OCTET_STRING octet_string = { 0 };

    if (fdt_entry && fdt_entry->valid) {
        /* bacnetip-address [0] OCTET STRING */
        len = bvlc_encode_address(
            octetstring_value(&octet_string),
            octetstring_capacity(&octet_string), &fdt_entry->dest_address);
        octetstring_truncate(&octet_string, len);
        len = encode_context_octet_string(apdu, 0, &octet_string);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* time-to-live [1] Unsigned16 */
        len = encode_context_unsigned(apdu, 1, fdt_entry->ttl_seconds);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* remaining-time-to-live [2] Unsigned16 */
        len =
            encode_context_unsigned(apdu, 2, fdt_entry->ttl_seconds_remaining);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the Foreign_Device-Table for Network Port object
 *
 *    BACnetLIST of BACnetFDTEntry
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param fdt_head - head of the BDT linked list
 * @return length of the APDU buffer
 */
int bvlc_foreign_device_table_list_encode(
    uint8_t *apdu, const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)
{
    int len = 0;
    int apdu_len = 0;
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry;

    fdt_entry = fdt_head;
    while (fdt_entry) {
        if (fdt_entry->valid) {
            len = bvlc_foreign_device_table_entry_encode(apdu, fdt_entry);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
        /* next entry */
        fdt_entry = fdt_entry->next;
    }

    return apdu_len;
}

/**
 * @brief Encode the Foreign_Device-Table for Network Port object
 *
 *    BACnetLIST of BACnetFDTEntry
 *
 *    BACnetFDTEntry ::= SEQUENCE {
 *        bacnetip-address [0] OCTET STRING, -- 6-octet B/IP registrant address
 *        time-to-live [1] Unsigned16, -- time to live in seconds
 *        remaining-time-to-live [2] Unsigned16 -- remaining time in seconds
 *    }
 *
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer size
 * @param fdt_head - head of the BDT linked list
 * @return length of the APDU buffer
 */
int bvlc_foreign_device_table_encode(
    uint8_t *apdu,
    uint16_t apdu_size,
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)
{
    int len = 0;

    len = bvlc_foreign_device_table_list_encode(NULL, fdt_head);
    if (len <= apdu_size) {
        len = bvlc_foreign_device_table_list_encode(apdu, fdt_head);
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief J.2.7 Read-Foreign-Device-Table: encode
 *
 * The message provides a mechanism for retrieving the contents of a BBMD's
 * Foreign-Device-Table.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param bdt_list - list of FDT entries
 *
 * @return number of bytes encoded
 *
 * BVLC Type:           1-octet  X'81'   BVLL for BACnet/IP
 * BVLC Function:       1-octet  X'06'   Read-Foreign-Device-Table
 * BVLC Length:         2-octets X'0004' Length, in octets, of the BVLL message
 */
int bvlc_encode_read_foreign_device_table(uint8_t *pdu, uint16_t pdu_size)
{
    int bytes_encoded = 0;
    uint16_t length = 1 + 1 + 2;

    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_READ_FOREIGN_DEVICE_TABLE, length);
    }

    return bytes_encoded;
}

/**
 * @brief Compare the Foreign Device Table entry
 * @param entry1 - Foreign Device Table entry that will be compared to entry2
 * @param entry2 - Foreign Device Table entry that will be compared to entry1
 * @return true if the entries are different
 */
bool bvlc_foreign_device_table_entry_different(
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *entry1,
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *entry2)
{
    if (entry1 && entry2) {
        if (bvlc_address_different(
                &entry1->dest_address, &entry2->dest_address)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Copy the Foreign Device Table entry
 * @param entry1 - Foreign Device Table entry that will be filled with entry2
 * @param entry2 - Foreign Device Table entry that will be copied into entry1
 * @return true if the Foreign Device Table entry was copied
 */
bool bvlc_foreign_device_table_entry_copy(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *entry1,
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *entry2)
{
    bool status = false;

    if (entry1 && entry2) {
        entry1->ttl_seconds = entry2->ttl_seconds;
        entry1->ttl_seconds_remaining = entry2->ttl_seconds_remaining;
        status =
            bvlc_address_copy(&entry1->dest_address, &entry2->dest_address);
    }

    return status;
}

/**
 * @brief Foreign-Device-Table timer maintenance
 * @param fdt_list - first element in list of FDT entries
 * @param seconds - number of elapsed seconds since the last call
 */
void bvlc_foreign_device_table_maintenance_timer(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list, uint16_t seconds)
{
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry = NULL;

    fdt_entry = fdt_list;
    while (fdt_entry) {
        if (fdt_entry->valid) {
            if (fdt_entry->ttl_seconds_remaining) {
                if (fdt_entry->ttl_seconds_remaining < seconds) {
                    fdt_entry->ttl_seconds_remaining = 0;
                } else {
                    fdt_entry->ttl_seconds_remaining -= seconds;
                }
                if (fdt_entry->ttl_seconds_remaining == 0) {
                    fdt_entry->valid = false;
                }
            }
        }
        fdt_entry = fdt_entry->next;
    }
}

/**
 * @brief Delete an entry in the Foreign-Device-Table
 * @param fdt_list - first element in list of FDT entries
 * @param addr - B/IPv4 address to be deleted
 * @return true if the Foreign Device entry was found and removed.
 */
bool bvlc_foreign_device_table_entry_delete(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list,
    const BACNET_IP_ADDRESS *addr)
{
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry = NULL;
    bool status = false;

    fdt_entry = fdt_list;
    while (fdt_entry) {
        if (fdt_entry->valid) {
            if (!bvlc_address_different(&fdt_entry->dest_address, addr)) {
                status = true;
                fdt_entry->valid = false;
                fdt_entry->ttl_seconds_remaining = 0;
                break;
            }
        }
        fdt_entry = fdt_entry->next;
    }

    return status;
}

/**
 * @brief Add an entry to the Foreign-Device-Table
 * @param fdt_list - first element in list of FDT entries
 * @param addr - B/IPv4 address to be added
 * @param ttl_seconds - Time-to-Live T, in seconds
 * @return true if the Foreign Device entry was added or already exists
 */
bool bvlc_foreign_device_table_entry_add(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list,
    const BACNET_IP_ADDRESS *addr,
    uint16_t ttl_seconds)
{
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry = NULL;
    bool status = false;

    fdt_entry = fdt_list;
    while (fdt_entry) {
        if (fdt_entry->valid) {
            /* am I here already?  If so, update my time to live... */
            if (!bvlc_address_different(&fdt_entry->dest_address, addr)) {
                status = true;
                fdt_entry->ttl_seconds = ttl_seconds;
                /* Upon receipt of a BVLL Register-Foreign-Device message,
                   a BBMD shall start a timer with a value equal to the
                   Time-to-Live parameter supplied plus a fixed grace
                   period of 30 seconds. */
                if (ttl_seconds < (UINT16_MAX - 30)) {
                    fdt_entry->ttl_seconds_remaining = ttl_seconds + 30;
                } else {
                    fdt_entry->ttl_seconds_remaining = UINT16_MAX;
                }
                break;
            }
        }
        fdt_entry = fdt_entry->next;
    }
    if (!status) {
        fdt_entry = fdt_list;
        while (fdt_entry) {
            if (!fdt_entry->valid) {
                /* add to the first empty entry */
                bvlc_address_copy(&fdt_entry->dest_address, addr);
                fdt_entry->ttl_seconds = ttl_seconds;
                if (ttl_seconds < (UINT16_MAX - 30)) {
                    fdt_entry->ttl_seconds_remaining = ttl_seconds + 30;
                } else {
                    fdt_entry->ttl_seconds_remaining = UINT16_MAX;
                }
                fdt_entry->valid = true;
                status = true;
                break;
            }
            fdt_entry = fdt_entry->next;
        }
    }

    return status;
}

/**
 * @brief Count the number of valid Foreign-Device-Table entries
 * @param fdt_list - first element in list of FDT entries
 * @return number of elements of FDT entries that are valid
 */
uint16_t bvlc_foreign_device_table_valid_count(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)
{
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry = NULL;
    uint16_t entry_count = 0;

    /* count the number of entries */
    fdt_entry = fdt_list;
    while (fdt_entry) {
        if (fdt_entry->valid) {
            entry_count++;
        }
        fdt_entry = fdt_entry->next;
    }

    return entry_count;
}

/**
 * @brief Count the total number of Foreign-Device-Table entries
 *
 * @param bdt_list - first element in array BDT entries
 * @return number of elements of BDT entries
 */
uint16_t
bvlc_foreign_device_table_count(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)
{
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry = NULL;
    uint16_t entry_count = 0;

    /* count the number of entries */
    fdt_entry = fdt_list;
    while (fdt_entry) {
        entry_count++;
        fdt_entry = fdt_entry->next;
    }

    return entry_count;
}

/**
 * @brief Convert Foreign-Device-Table entry array into linked list.
 *
 * @param fdt_list - first element in array FDT entries
 * @param array_size - number of array elements of FDT entries
 */
void bvlc_foreign_device_table_link_array(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list, const size_t array_size)
{
    size_t i = 0;

    for (i = 0; i < array_size; i++) {
        if (i > 0) {
            fdt_list[i - 1].next = &fdt_list[i];
        }
        fdt_list[i].next = NULL;
    }
}

/**
 * @brief J.2.8 Read-Foreign-Device-Table-Ack: encode
 *
 * This message returns the current contents of a BBMD's FDT to the requester.
 * An empty FDT shall be signified by a list of length zero.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param fdt_list - list of FDT entries
 *
 * @return number of bytes encoded
 *
 * BVLC Type:           1-octet  X'81'   BVLL for BACnet/IP
 * BVLC Function:       1-octet  X'07'   Read-Foreign-Device-Table-Ack
 * BVLC Length:         2-octets L       length, in octets, of the BVLL message
 * List of FDT Entries: N*10-octets
 *
 * N indicates the number of entries in the FDT whose contents are being
 * returned. Each returned entry consists of the 6-octet B/IP address of
 * the registrant; the 2-octet Time-to-Live value supplied at the time of
 * registration; and a 2-octet value representing the number of seconds
 * remaining before the BBMD will purge the registrant's FDT entry if no
 * re-registration occurs. The time remaining includes the 30-second grace
 * period as defined in Clause J.5.2.3.
 */
int bvlc_encode_read_foreign_device_table_ack(
    uint8_t *pdu,
    uint16_t pdu_size,
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)
{
    int bytes_encoded = 0;
    int len = 0;
    uint16_t offset = 0;
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry = NULL;
    uint16_t entry_count = 0;
    uint16_t length = 0;

    /* count the number of entries */
    entry_count = bvlc_foreign_device_table_valid_count(fdt_list);
    length = 4 + (entry_count * BACNET_IP_FDT_ENTRY_SIZE);
    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_READ_FOREIGN_DEVICE_TABLE_ACK, length);
        if (bytes_encoded == 4) {
            offset = 4;
            /* encode the entries */
            fdt_entry = fdt_list;
            while (fdt_entry) {
                if (fdt_entry->valid) {
                    len = bvlc_encode_foreign_device_table_entry(
                        &pdu[offset], pdu_size - offset, fdt_entry);
                    offset += len;
                }
                fdt_entry = fdt_entry->next;
            }
            bytes_encoded = (int)length;
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode Read-Foreign-Device-Table-Ack
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param fdt_list - list of FDT entries
 *
 * @return number of bytes decoded
 */
int bvlc_decode_read_foreign_device_table_ack(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)
{
    int bytes_consumed = 0;
    int len = 0;
    uint16_t offset = 0;
    uint16_t pdu_bytes = 0;
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry = NULL;

    if (pdu && (pdu_len >= BACNET_IP_FDT_ENTRY_SIZE)) {
        fdt_entry = fdt_list;
        while (fdt_entry) {
            pdu_bytes = pdu_len - offset;
            if (pdu_bytes >= BACNET_IP_BDT_ENTRY_SIZE) {
                len = bvlc_decode_foreign_device_table_entry(
                    &pdu[offset], pdu_bytes, fdt_entry);
                if (len > 0) {
                    fdt_entry->valid = true;
                }
                offset += len;
            } else {
                fdt_entry->valid = false;
            }
            fdt_entry = fdt_entry->next;
        }
        bytes_consumed = (int)offset;
    }

    return bytes_consumed;
}

/**
 * @brief J.2.9 Delete-Foreign-Device-Table-Entry: encode
 *
 * This message is used to delete an entry from the Foreign-Device-Table.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param ip_address - FDT Entry IP address
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'81'   BVLL for BACnet/IP
 * BVLC Function:               1-octet   X'08'   Delete-Foreign-Device
 * BVLC Length:                 2-octets  X'000A' Length of the BVLL message
 * FDT Entry:                   6-octets  The FDT entry is the B/IP address
 *                                        of the table entry to be deleted.
 */
int bvlc_encode_delete_foreign_device(
    uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_ADDRESS *ip_address)
{
    int bytes_encoded = 0;
    const uint16_t length = 0x000A;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY, length);
        if (bytes_encoded == 4) {
            offset = 4;
            if (ip_address) {
                bytes_encoded += bvlc_encode_address(
                    &pdu[offset], pdu_size - offset, ip_address);
            }
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Delete-Foreign-Device message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param ip_address - FDT Entry IP address
 *
 * @return number of bytes decoded
 */
int bvlc_decode_delete_foreign_device(
    const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_ADDRESS *ip_address)
{
    int bytes_consumed = 0;
    const uint16_t length = BIP_ADDRESS_MAX;

    if (pdu && (pdu_len >= length)) {
        if (ip_address) {
            bvlc_decode_address(&pdu[0], pdu_len, ip_address);
        }
        bytes_consumed = (int)length;
    }

    return bytes_consumed;
}

/**
 * @brief J.2.10 Distribute-Broadcast-To-Network: encode
 *
 * This message provides a mechanism whereby a foreign device may cause
 * a BBMD to broadcast a message on all IP subnets in the BBMD's BDT.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param npdu - BACnet NPDU from Originating Device buffer
 * @param npdu_len - size of the BACnet NPDU buffer
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'81'   BVLL for BACnet/IP
 * BVLC Function:               1-octet   X'09'   Original-Unicast-NPDU
 * BVLC Length:                 2-octets  L       Length of the BVLL message
 * BACnet NPDU from Originating Device:  Variable length
 */
int bvlc_encode_distribute_broadcast_to_network(
    uint8_t *pdu, uint16_t pdu_size, const uint8_t *npdu, uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 1 + 1 + 2;
    uint16_t i = 0;

    length += npdu_len;
    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK, length);
        if (bytes_encoded == 4) {
            if (npdu && (npdu_len > 0)) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[4 + i] = npdu[i];
                }
            }
            bytes_encoded = (int)length;
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Original-Broadcast-NPDU message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param npdu - buffer to copy the decoded BACnet NDPU
 * @param npdu_size - size of the buffer for the decoded BACnet NPDU
 * @param npdu_len - decoded length of the BACnet NPDU
 *
 * @return number of bytes decoded
 */
int bvlc_decode_distribute_broadcast_to_network(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len)
{
    int bytes_consumed = 0;
    uint16_t i = 0;

    if (pdu && npdu && (pdu_len > 0) && (pdu_len <= npdu_size)) {
        for (i = 0; i < pdu_len; i++) {
            npdu[i] = pdu[i];
        }
    }
    if (npdu_len) {
        *npdu_len = pdu_len;
    }
    bytes_consumed = (int)pdu_len;

    return bytes_consumed;
}

/**
 * @brief J.2.11 Original-Unicast-NPDU: Encode
 *
 * This message is used to send directed NPDUs to another
 * B/IP device or router.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param npdu - BACnet NPDU buffer
 * @param npdu_len - size of the BACnet NPDU buffer
 *
 * @return number of bytes encoded
 *
 * BVLC Type:     1-octet   X'81'   BVLL for BACnet/IPv4
 * BVLC Function: 1-octet   X'0A'   Original-Unicast-NPDU
 * BVLC Length:   2-octets  L       Length L, in octets, of the BVLL message
 * BACnet NPDU:   Variable length
 */
int bvlc_encode_original_unicast(
    uint8_t *pdu, uint16_t pdu_size, const uint8_t *npdu, uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 4;
    uint16_t i = 0;

    length += npdu_len;
    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_ORIGINAL_UNICAST_NPDU, length);
        if (bytes_encoded == 4) {
            if (npdu && (npdu_len > 0)) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[4 + i] = npdu[i];
                }
                bytes_encoded = (int)length;
            }
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Original-Unicast-NPDU message, after decoding header
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param npdu - BACnet NPDU buffer
 * @param npdu_size - size of the buffer for the decoded BACnet NPDU
 * @param npdu_len - decoded length of the BACnet NPDU buffer
 *
 * @return number of bytes decoded
 */
int bvlc_decode_original_unicast(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len)
{
    int bytes_consumed = 0;
    uint16_t i = 0;

    if (pdu && npdu && (pdu_len > 0) && (pdu_len <= npdu_size)) {
        for (i = 0; i < pdu_len; i++) {
            npdu[i] = pdu[i];
        }
    }
    if (npdu_len) {
        *npdu_len = pdu_len;
    }
    bytes_consumed = (int)pdu_len;

    return bytes_consumed;
}

/**
 * @brief J.2.12 Original-Broadcast-NPDU: Encode
 *
 * This message is used by B/IP devices and routers which are
 * not foreign devices to broadcast NPDUs on a B/IP network
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param npdu - BACnet NPDU buffer
 * @param npdu_len - size of the BACnet NPDU buffer
 *
 * @return number of bytes encoded
 *
 * BVLC Type:     1-octet   X'81'   BVLL for BACnet/IPv4
 * BVLC Function: 1-octet   X'0B'   Original-Broadcast-NPDU
 * BVLC Length:   2-octets  L       Length of the BVLL message
 * BACnet NPDU:   Variable length
 */
int bvlc_encode_original_broadcast(
    uint8_t *pdu, uint16_t pdu_size, const uint8_t *npdu, uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 4;
    uint16_t i = 0;

    length += npdu_len;
    if (pdu && (pdu_size >= length)) {
        bytes_encoded = bvlc_encode_header(
            pdu, pdu_size, BVLC_ORIGINAL_BROADCAST_NPDU, length);
        if (bytes_encoded == 4) {
            if (npdu && (npdu_len > 0)) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[4 + i] = npdu[i];
                }
                bytes_encoded = (int)length;
            }
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Original-Broadcast-NPDU message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param npdu - buffer to copy the decoded BACnet NDPU
 * @param npdu_size - size of the buffer for the decoded BACnet NPDU
 * @param npdu_len - decoded length of the BACnet NPDU
 *
 * @return number of bytes decoded
 */
int bvlc_decode_original_broadcast(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len)
{
    int bytes_consumed = 0;
    uint16_t i = 0;

    if (pdu && npdu && (pdu_len > 0) && (pdu_len <= npdu_size)) {
        for (i = 0; i < pdu_len; i++) {
            npdu[i] = pdu[i];
        }
    }
    if (npdu_len) {
        *npdu_len = pdu_len;
    }
    bytes_consumed = (int)pdu_len;

    return bytes_consumed;
}

/**
 * @brief J.2.13 Secure-BVLL: encode
 *
 * This message is used to secure BVLL messages that do not contain NPDUs.
 * Its use is described in Clause 24.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param sbuf - Security Wrapper buffer
 * @param sbuf_len - size of the Security Wrapper buffer
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'81'   BVLL for BACnet/IP
 * BVLC Function:               1-octet   X'0C'   Secure-BVLL
 * BVLC Length:                 2-octets  L       Length of the BVLL message
 * Security Wrapper:            Variable length
 */
int bvlc_encode_secure_bvll(
    uint8_t *pdu, uint16_t pdu_size, const uint8_t *sbuf, uint16_t sbuf_len)
{
    int bytes_encoded = 0;
    uint16_t length = 1 + 1 + 2;
    uint16_t i = 0;

    length += sbuf_len;
    if (pdu && (pdu_size >= length)) {
        bytes_encoded =
            bvlc_encode_header(pdu, pdu_size, BVLC_SECURE_BVLL, length);
        if (bytes_encoded == 4) {
            if (sbuf && sbuf_len) {
                for (i = 0; i < sbuf_len; i++) {
                    pdu[4 + i] = sbuf[i];
                }
            }
            bytes_encoded = (int)length;
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Secure-BVLL message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param sbuf - Security Wrapper buffer
 * @param sbuf_size - size of the Security Wrapper buffer
 * @param sbuf_len - number of bytes decoded into the Security Wrapper buffer
 *
 * @return number of bytes decoded
 */
int bvlc_decode_secure_bvll(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *sbuf,
    uint16_t sbuf_size,
    uint16_t *sbuf_len)
{
    int bytes_consumed = 0;
    uint16_t i = 0;

    if (pdu && sbuf && (pdu_len > 0) && (pdu_len <= sbuf_size)) {
        for (i = 0; i < pdu_len; i++) {
            sbuf[i] = pdu[i];
        }
    }
    if (sbuf_len) {
        *sbuf_len = pdu_len;
    }
    bytes_consumed = (int)pdu_len;

    return bytes_consumed;
}

/**
 * @brief Encode the BVLC Address
 *
 * Data link layer addressing between B/IPv4 nodes consists of a 32-bit
 * IPv4 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv4 address.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param bip_address - B/IPv4 address
 *
 * @return number of bytes encoded
 */
int bvlc_encode_address(
    uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_ADDRESS *bip_address)
{
    int bytes_encoded = 0;
    uint16_t length = BIP_ADDRESS_MAX;
    unsigned i = 0;

    if (pdu && (pdu_size >= length) && bip_address) {
        for (i = 0; i < IP_ADDRESS_MAX; i++) {
            pdu[i] = bip_address->address[i];
        }
        encode_unsigned16(&pdu[IP_ADDRESS_MAX], bip_address->port);
        bytes_encoded = (int)length;
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Address
 *
 * Data link layer addressing between B/IPv4 nodes consists of a 32-bit
 * IPv4 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv4 address.
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param bip_address - B/IPv4 address
 *
 * @return number of bytes decoded
 */
int bvlc_decode_address(
    const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_ADDRESS *bip_address)
{
    int bytes_consumed = 0;
    uint16_t length = BIP_ADDRESS_MAX;
    unsigned i = 0;

    if (pdu && (pdu_len >= length) && bip_address) {
        for (i = 0; i < IP_ADDRESS_MAX; i++) {
            bip_address->address[i] = pdu[i];
        }
        decode_unsigned16(&pdu[IP_ADDRESS_MAX], &bip_address->port);
        bytes_consumed = (int)length;
    }

    return bytes_consumed;
}

/**
 * @brief Copy the BVLC Address
 *
 * Data link layer addressing between B/IPv4 nodes consists of a 32-bit
 * IPv4 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv4 address.
 *
 * @param dst - B/IPv4 address that will be filled with src
 * @param src - B/IPv4 address that will be copied into dst
 *
 * @return true if the address was copied
 */
bool bvlc_address_copy(BACNET_IP_ADDRESS *dst, const BACNET_IP_ADDRESS *src)
{
    bool status = false;
    unsigned int i = 0;

    if (src && dst) {
        for (i = 0; i < IP_ADDRESS_MAX; i++) {
            dst->address[i] = src->address[i];
        }
        dst->port = src->port;
        status = true;
    }

    return status;
}

/**
 * @brief Compare the BVLC Address
 *
 * Data link layer addressing between B/IPv4 nodes consists of a 32-bit
 * IPv4 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv4 address.
 *
 * @param dst - B/IPv4 address that will be compared to src
 * @param src - B/IPv4 address that will be compared to dst
 *
 * @return true if the addresses are different
 */
bool bvlc_address_different(
    const BACNET_IP_ADDRESS *dst, const BACNET_IP_ADDRESS *src)
{
    bool status = false;
    unsigned int i = 0;

    if (src && dst) {
        for (i = 0; i < IP_ADDRESS_MAX; i++) {
            if (dst->address[i] != src->address[i]) {
                status = true;
            }
        }
        if (dst->port != src->port) {
            status = true;
        }
    }

    return status;
}

/**
 * @brief Apply the Broadcast Distribution Mask to an address
 * @param dst - B/IPv4 address that will be masked
 * @param src - B/IPv4 address that will be ORed with the mask
 * @param mask - B/IPv4 broadcast distribution mask
 * @return true if the addresses are different
 */
bool bvlc_address_mask(
    BACNET_IP_ADDRESS *dst,
    const BACNET_IP_ADDRESS *src,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask)
{
    bool status = false;
    unsigned int i = 0;

    if (src && dst && mask) {
        for (i = 0; i < IP_ADDRESS_MAX; i++) {
            dst->address[i] = src->address[i] | ~mask->address[i];
        }
        dst->port = src->port;
    }

    return status;
}

/**
 * @brief Set a BVLC Address from 4 octets
 *
 * Data link layer addressing between B/IPv4 nodes consists of a 32-bit
 * IPv4 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv4 address.
 *
 * @param addr - B/IPv4 address that be set
 * @param addr0 - B/IPv4 address octet
 * @param addr1 - B/IPv4 address octet
 * @param addr2 - B/IPv4 address octet
 * @param addr3 - B/IPv4 address octet
 *
 * @return true if the address is set
 */
bool bvlc_address_set(
    BACNET_IP_ADDRESS *addr,
    uint8_t addr0,
    uint8_t addr1,
    uint8_t addr2,
    uint8_t addr3)
{
    bool status = false;

    if (addr) {
        addr->address[0] = addr0;
        addr->address[1] = addr1;
        addr->address[2] = addr2;
        addr->address[3] = addr3;
        status = true;
    }

    return status;
}

/**
 * @brief Get a BVLC Address into 4 octets
 *
 * Data link layer addressing between B/IPv4 nodes consists of a 128-bit
 * IPv4 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv4 address.
 *
 * @param addr - B/IPv4 address that be set
 * @param addr0 - B/IPv4 address octet
 * @param addr1 - B/IPv4 address octet
 * @param addr2 - B/IPv4 address octet
 * @param addr3 - B/IPv4 address octet
 *
 * @return true if the address is set
 */
bool bvlc_address_get(
    const BACNET_IP_ADDRESS *addr,
    uint8_t *addr0,
    uint8_t *addr1,
    uint8_t *addr2,
    uint8_t *addr3)
{
    bool status = false;

    if (addr) {
        if (addr0) {
            *addr0 = addr->address[0];
        }
        if (addr1) {
            *addr1 = addr->address[1];
        }
        if (addr2) {
            *addr2 = addr->address[2];
        }
        if (addr3) {
            *addr3 = addr->address[3];
        }
        status = true;
    }

    return status;
}

/**
 * @brief Convert IPv4 Address from ASCII
 *
 * IPv4 addresses are represented as four octets, separated by dots/periods,
 * of four decimal digits.
 *
 * Adapted from uiplib.c uIP TCP/IP stack and the Contiki operating system.
 * Thank you, Adam Dunkel, and the Swedish Institute of Computer Science.
 *
 * @param addr - B/IPv4 address that is set
 * @param addrstr - B/IPv4 address in ASCII dotted decimal format
 *
 * @return true if a valid address was set
 */
bool bvlc_address_from_ascii(BACNET_IP_ADDRESS *addr, const char *addrstr)
{
    uint16_t tmp = 0;
    char c = 0;
    unsigned char i = 0, j = 0;

    if (!addr) {
        return false;
    }
    if (!addrstr) {
        return false;
    }

    for (i = 0; i < 4; ++i) {
        j = 0;
        do {
            c = *addrstr;
            ++j;
            if (j > 4) {
                return false;
            }
            if ((c == '.') || (c == 0) || (c == ' ')) {
                addr->address[i] = (uint8_t)tmp;
                tmp = 0;
            } else if ((c >= '0') && (c <= '9')) {
                tmp = (tmp * 10) + (c - '0');
                if (tmp > UINT8_MAX) {
                    return false;
                }
            } else {
                return false;
            }
            ++addrstr;
        } while ((c != '.') && (c != 0) && (c != ' '));
    }

    return true;
}

/**
 * @brief Convert IPv4 Address and UDP port number from ASCII
 *
 * @param addr - B/IPv4 address and port that is set
 * @param addrstr - B/IPv4 address in ASCII dotted decimal format
 * @param portstr - B/IPv4 port in 16-bit ASCII hex compressed format
 *
 * @return true if a valid address was set
 */
bool bvlc_address_port_from_ascii(
    BACNET_IP_ADDRESS *addr, const char *addrstr, const char *portstr)
{
    bool status = false;
    unsigned long port = 0;

    if (bvlc_address_from_ascii(addr, addrstr)) {
        port = strtoul(portstr, NULL, 0);
        if (port <= UINT16_MAX) {
            addr->port = port;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Convert IPv4 Address from network byte order 32-bit value
 *
 * @param dst - B/IPv4 address that is set
 * @param addr - B/IPv4 address in network byte order 32-bit value
 */
void bvlc_address_from_network(BACNET_IP_ADDRESS *dst, uint32_t addr)
{
    if (dst) {
        /* copy most significant octet first, network byte order, big endian */
        encode_unsigned32(&dst->address[0], addr);
    }
}

/**
 * @brief Convert IPv4 Address to local BACnet address
 *
 * Data link layer addressing between B/IPv4 nodes consists of a 32-bit
 * IPv4 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first).
 * This address shall be referred to as a B/IPv4 address.
 *
 * @param dst - BACnet MAC address that is set
 * @param addr - B/IPv4 address in network byte order 32-bit value
 * @return true if a valid address was set
 */
bool bvlc_ip_address_to_bacnet_local(
    BACNET_ADDRESS *addr, const BACNET_IP_ADDRESS *ipaddr)
{
    bool status = false;

    if (addr && ipaddr) {
        /* most significant octet first, network byte order, big endian */
        addr->mac[0] = ipaddr->address[0];
        addr->mac[1] = ipaddr->address[1];
        addr->mac[2] = ipaddr->address[2];
        addr->mac[3] = ipaddr->address[3];
        encode_unsigned16(&addr->mac[4], ipaddr->port);
        addr->mac[6] = 0;
        addr->mac_len = 6;
        /* local only, no routing */
        addr->net = 0;
        /* no SLEN/DLEN */
        addr->len = 0;
        /* no SADR/DADR */
        addr->adr[0] = 0;
        addr->adr[1] = 0;
        addr->adr[2] = 0;
        addr->adr[3] = 0;
        addr->adr[4] = 0;
        addr->adr[5] = 0;
        addr->adr[6] = 0;
        status = true;
    }

    return status;
}

/**
 * @brief Convert IPv4 Address from local BACnet address
 * @param addr - BACnet MAC address that is converted into B/IPv4 address/port
 * @param ipaddr - B/IPv4 address that is set
 * @return true if a valid address was set
 */
bool bvlc_ip_address_from_bacnet_local(
    BACNET_IP_ADDRESS *ipaddr, const BACNET_ADDRESS *addr)
{
    bool status = false;

    if (addr && ipaddr) {
        if (addr->mac_len == 6) {
            /* most significant octet first, network byte order, big endian */
            ipaddr->address[0] = addr->mac[0];
            ipaddr->address[1] = addr->mac[1];
            ipaddr->address[2] = addr->mac[2];
            ipaddr->address[3] = addr->mac[3];
            decode_unsigned16(&addr->mac[4], &ipaddr->port);
            status = true;
        }
    }

    return status;
}

/**
 * @brief Convert IPv4 Address to remote BACnet address
 * @param dst - BACnet MAC address that is set
 * @param dnet - network number of BACnet address
 * @param addr - B/IPv4 address in network byte order 32-bit value
 * @return true if a valid address was set
 */
bool bvlc_ip_address_to_bacnet_remote(
    BACNET_ADDRESS *addr, uint16_t dnet, const BACNET_IP_ADDRESS *ipaddr)
{
    bool status = false;

    if (addr && ipaddr) {
        /* don't modify local MAC or MAC len */
        /* add DNET/SNET */
        addr->net = dnet;
        /* no SADR/DADR */
        /* most significant octet first, network byte order, big endian */
        addr->adr[0] = ipaddr->address[0];
        addr->adr[1] = ipaddr->address[1];
        addr->adr[2] = ipaddr->address[2];
        addr->adr[3] = ipaddr->address[3];
        encode_unsigned16(&addr->adr[4], ipaddr->port);
        addr->adr[6] = 0;
        /* set SLEN/DLEN for BACnet/IPv4 */
        addr->len = 6;
        status = true;
    }

    return status;
}

/**
 * @brief Convert IPv4 Address from remote BACnet address
 * @param addr - BACnet MAC address that is converted into B/IPv4 address/port
 * @param dnet - network number of BACnet address
 * @param ipaddr - B/IPv4 address that is set
 * @return true if a valid address was set
 */
bool bvlc_ip_address_from_bacnet_remote(
    BACNET_IP_ADDRESS *ipaddr, uint16_t *dnet, const BACNET_ADDRESS *addr)
{
    bool status = false;

    if (addr && ipaddr) {
        if (addr->len == 6) {
            /* most significant octet first, network byte order, big endian */
            ipaddr->address[0] = addr->adr[0];
            ipaddr->address[1] = addr->adr[1];
            ipaddr->address[2] = addr->adr[2];
            ipaddr->address[3] = addr->adr[3];
            decode_unsigned16(&addr->adr[4], &ipaddr->port);
            if (dnet) {
                *dnet = addr->net;
            }
            status = true;
        }
    }

    return status;
}

/**
 * @brief Encode the BVLC Broadcast Distribution Mask
 *
 * The Broadcast Distribution Mask is a 4-octet field that
 * indicates how broadcast messages are to be distributed on
 * the IP subnet served by the BBMD.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param bd_mask - Broadcast Distribution Mask
 *
 * @return number of bytes encoded
 */
int bvlc_encode_broadcast_distribution_mask(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *bd_mask)
{
    int bytes_encoded = 0;
    unsigned i = 0;

    if (pdu && (pdu_size >= BACNET_IP_BDT_MASK_SIZE) && bd_mask) {
        for (i = 0; i < IP_ADDRESS_MAX; i++) {
            pdu[i] = bd_mask->address[i];
        }
        bytes_encoded = BACNET_IP_BDT_MASK_SIZE;
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Broadcast Distribution Mask
 *
 * The Broadcast Distribution Mask is a 4-octet field that
 * indicates how broadcast messages are to be distributed on
 * the IP subnet served by the BBMD.
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param bd_mask - Broadcast Distribution Mask
 *
 * @return number of bytes decoded
 */
int bvlc_decode_broadcast_distribution_mask(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK *bd_mask)
{
    int bytes_consumed = 0;
    unsigned i = 0;

    if (pdu && (pdu_len >= BACNET_IP_BDT_MASK_SIZE)) {
        if (bd_mask) {
            for (i = 0; i < IP_ADDRESS_MAX; i++) {
                bd_mask->address[i] = pdu[i];
            }
        }
        bytes_consumed = (int)BACNET_IP_BDT_MASK_SIZE;
    }

    return bytes_consumed;
}

/**
 * @brief Encode the BVLC Broadcast Distribution Table Entry
 *
 * Each BDT entry consists of the 6-octet B/IP address of a
 * BBMD followed by a 4-octet field called the broadcast distribution mask
 * that indicates how broadcast messages are to be distributed on the
 * IP subnet served by the BBMD
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param bdt_entry - BDT Entry
 *
 * @return number of bytes encoded
 */
int bvlc_encode_broadcast_distribution_table_entry(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)
{
    int bytes_encoded = 0;
    int len = 0;
    int offset = 0;

    if (pdu && (pdu_size >= BACNET_IP_BDT_ENTRY_SIZE)) {
        if (bdt_entry) {
            len = bvlc_encode_address(
                &pdu[offset], pdu_size - offset, &bdt_entry->dest_address);
            if (len > 0) {
                offset += len;
                len = bvlc_encode_broadcast_distribution_mask(
                    &pdu[offset], pdu_size - offset,
                    &bdt_entry->broadcast_mask);
            }
            if (len > 0) {
                offset += len;
                bytes_encoded = offset;
            }
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Broadcast Distribution Table Entry
 *
 * Each BDT entry consists of the 6-octet B/IP address of a
 * BBMD followed by a 4-octet field called the broadcast distribution mask
 * that indicates how broadcast messages are to be distributed on the
 * IP subnet served by the BBMD
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param bdt_entry - BDT Entry
 *
 * @return number of bytes decoded
 */
int bvlc_decode_broadcast_distribution_table_entry(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)
{
    int bytes_consumed = 0;
    int len = 0;
    int offset = 0;

    if (pdu && (pdu_len >= BACNET_IP_BDT_ENTRY_SIZE)) {
        if (bdt_entry) {
            len = bvlc_decode_address(
                &pdu[offset], pdu_len - offset, &bdt_entry->dest_address);
            if (len > 0) {
                offset += len;
                len = bvlc_decode_broadcast_distribution_mask(
                    &pdu[offset], pdu_len - offset, &bdt_entry->broadcast_mask);
            }
            if (len > 0) {
                offset += len;
                bytes_consumed = offset;
            }
        }
    }

    return bytes_consumed;
}

/**
 * @brief Encode the BVLC Foreign Device Table Entry
 *
 * Each BDT entry consists of the 6-octet B/IP address of the registrant;
 * the 2-octet Time-to-Live value supplied at the time of registration;
 * and a 2-octet value representing the number of seconds remaining.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param fdt_entry - Foreign Device Table (FDT) Entry
 *
 * @return number of bytes encoded
 */
int bvlc_encode_foreign_device_table_entry(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry)
{
    int bytes_encoded = 0;
    int len = 0;
    int offset = 0;

    if (pdu && (pdu_size >= BACNET_IP_FDT_ENTRY_SIZE)) {
        if (fdt_entry) {
            len = bvlc_encode_address(
                &pdu[offset], pdu_size - offset, &fdt_entry->dest_address);
            if (len > 0) {
                offset += len;
                len = encode_unsigned16(&pdu[offset], fdt_entry->ttl_seconds);
            }
            if (len > 0) {
                offset += len;
                len = encode_unsigned16(
                    &pdu[offset], fdt_entry->ttl_seconds_remaining);
            }
            if (len > 0) {
                offset += len;
                bytes_encoded = offset;
            }
        }
    }

    return bytes_encoded;
}

/**
 * @brief Decode the BVLC Foreign Device Table Entry
 *
 * Each BDT entry consists of the 6-octet B/IP address of the registrant;
 * the 2-octet Time-to-Live value supplied at the time of registration;
 * and a 2-octet value representing the number of seconds remaining.
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param fdt_entry - Foreign Device Table (FDT) Entry
 *
 * @return number of bytes decoded
 */
int bvlc_decode_foreign_device_table_entry(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry)
{
    int bytes_consumed = 0;
    int len = 0;
    int offset = 0;

    if (pdu && (pdu_len >= BACNET_IP_FDT_ENTRY_SIZE)) {
        if (fdt_entry) {
            len = bvlc_decode_address(
                &pdu[offset], pdu_len - offset, &fdt_entry->dest_address);
            if (len > 0) {
                offset += len;
                len = decode_unsigned16(&pdu[offset], &fdt_entry->ttl_seconds);
            }
            if (len > 0) {
                offset += len;
                len = decode_unsigned16(
                    &pdu[offset], &fdt_entry->ttl_seconds_remaining);
            }
            if (len > 0) {
                offset += len;
                bytes_consumed = offset;
            }
        }
    }

    return bytes_consumed;
}

/**
 * @brief Get a text name for each BVLC result code
 * @param result_code - BVLC result code
 * @return ASCII text name for each BVLC result code or empty string
 */
const char *bvlc_result_code_name(uint16_t result_code)
{
    const char *name = "";

    switch (result_code) {
        case BVLC_RESULT_SUCCESSFUL_COMPLETION:
            name = "Successful Completion";
            break;
        case BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK:
            name = "Write-Broadcast-Distribution-Table NAK";
            break;
        case BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK:
            name = "Read-Broadcast-Distribution-Table NAK";
            break;
        case BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK:
            name = "Register-Foreign-Device NAK";
            break;
        case BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK:
            name = "Read-Foreign-Device-Table NAK";
            break;
        case BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK:
            name = "Delete-Foreign-Device-Table-Entry NAK";
            break;
        case BVLC_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK:
            name = "Distribute-Broadcast-To-Network NAK";
            break;
        default:
            break;
    }

    return name;
}

/**
 * @brief Encode a BBMD Address for Network Port object
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer size
 * @param ip_address - IP address and port number
 * @return length of the APDU buffer
 */
int bvlc_foreign_device_bbmd_host_address_encode(
    uint8_t *apdu, uint16_t apdu_size, const BACNET_IP_ADDRESS *ip_address)
{
    BACNET_HOST_N_PORT address = { 0 };
    int apdu_len = 0;

    address.host_ip_address = true;
    address.host_name = false;
    octetstring_init(
        &address.host.ip_address, &ip_address->address[0], IP_ADDRESS_MAX);
    address.port = ip_address->port;
    apdu_len = host_n_port_encode(NULL, &address);
    if (apdu_len <= apdu_size) {
        apdu_len = host_n_port_encode(apdu, &address);
    }

    return apdu_len;
}

/**
 * @brief Decode the Broadcast-Distribution-Table for Network Port object
 * @param apdu - the APDU buffer
 * @param apdu_len - the APDU buffer length
 * @param ip_address - IP address and port number
 * @return length of the APDU buffer decoded, or ERROR, REJECT, or ABORT
 */
int bvlc_foreign_device_bbmd_host_address_decode(
    const uint8_t *apdu,
    uint16_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_IP_ADDRESS *ip_address)
{
    BACNET_HOST_N_PORT address = { 0 };
    int len = 0;

    len = host_n_port_decode(apdu, apdu_len, error_code, &address);
    if (len > 0) {
        if (address.host_ip_address) {
            ip_address->port = address.port;
            (void)octetstring_copy_value(
                &ip_address->address[0], IP_ADDRESS_MAX,
                &address.host.ip_address);
        } else {
            len = BACNET_STATUS_REJECT;
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
            }
        }
    }

    return len;
}
