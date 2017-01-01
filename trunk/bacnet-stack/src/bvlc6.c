/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2015 Steve Karg

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

#include <stdint.h>     /* for standard integer types uint8_t etc. */
#include <stdbool.h>    /* for the standard bool type. */
#include <time.h>
#include "bacenum.h"
#include "bacdcode.h"
#include "bacint.h"
#include "bvlc6.h"
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 0
#endif
#include "debug.h"

/** Encode the BVLC header
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param message_type - BVLL Messages
 * @param length - number of bytes for this message type
 *
 * @return number of bytes encoded
 */
int bvlc6_encode_header(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint8_t message_type,
    uint16_t length)
{
    int bytes_encoded = 0;

    if (pdu && (pdu_size >= 2)) {
        pdu[0] = BVLL_TYPE_BACNET_IP6;
        pdu[1] = message_type;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], length);
        bytes_encoded = 4;
    }

    return bytes_encoded;
}

/** Decode the BVLC Result message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param message_type - BVLL Messages
 * @param length - number of bytes for this message type
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_header(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint8_t * message_type,
    uint16_t * length)
{
    int bytes_consumed = 0;

    if (pdu && (pdu_len >= 4)) {
        if (pdu[0] == BVLL_TYPE_BACNET_IP6) {
            if (message_type) {
                *message_type = pdu[1];
            }
            if (length) {
                decode_unsigned16(&pdu[2], length);
            }
            bytes_consumed = 4;
        }
    }

    return bytes_consumed;
}


/** Encode the BVLC Result message
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
 * BVLC Type:             1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:         1-octet   X'00'   BVLC-Result
 * BVLC Length:           2-octets  X'0009' Length of the BVLL message
 * Source-Virtual-Address 3-octets
 * Result Code:           2-octets  X'0000' Successful completion
 *                                  X'0030' Address-Resolution NAK
 *                                  X'0060' Virtual-Address-Resolution NAK
 *                                  X'0090' Register-Foreign-Device NAK
 *                                  X'00A0' Delete-Foreign-Device-Table-Entry NAK
 *                                  X'00C0' Distribute-Broadcast-To-Network NAK
 */
int bvlc6_encode_result(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac,
    uint16_t result_code)
{
    int bytes_encoded = 0;
    const uint16_t length = 9;

    if (pdu && (pdu_size >= 9) && (vmac <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_RESULT, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac);
            encode_unsigned16(&pdu[7], result_code);
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Result message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac - Virtual MAC address
 * @param result_code - BVLC result code
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_result(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac,
    uint16_t * result_code)
{
    int bytes_consumed = 0;

    if (pdu && (pdu_len >= 5)) {
        if (vmac) {
            decode_unsigned24(&pdu[0], vmac);
        }
        if (result_code) {
            decode_unsigned16(&pdu[3], result_code);
        }
        bytes_consumed = 5;
    }

    return bytes_consumed;
}

/** Encode the BVLC Original-Unicast-NPDU message
 *
 * This message is used to send directed NPDUs to another B/IPv6 node
 * or router.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_dst - Destination-Virtual-Address
 * @param npdu - BACnet NPDU buffer
 * @param npdu_len - size of the BACnet NPDU buffer
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'01'   Original-Unicast-NPDU
 * BVLC Length:                 2-octets  L       Length of the BVLL message
 * Source-Virtual-Address:      3-octets
 * Destination-Virtual-Address: 3-octets
 * BACnet NPDU:                 Variable length
 */
int bvlc6_encode_original_unicast(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_dst,
    uint8_t * npdu,
    uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 10;
    uint16_t i = 0;


    length += npdu_len;
    if (pdu &&
        (pdu_size >= length) &&
        (vmac_src <= 0xFFFFFF) &&
        (vmac_dst <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_ORIGINAL_UNICAST_NPDU, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac_src);
            encode_unsigned24(&pdu[7], vmac_dst);
            if (npdu && length) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[10+i] = npdu[i];
                }
            }
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Original-Unicast-NPDU message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_dst - Destination-Virtual-Address
 * @param npdu - BACnet NPDU buffer
 * @param npdu_size - size of the buffer for the decoded BACnet NPDU
 * @param npdu_len - decoded length of the BACnet NPDU buffer
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_original_unicast(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac_src,
    uint32_t * vmac_dst,
    uint8_t * npdu,
    uint16_t npdu_size,
    uint16_t * npdu_len)
{
    int bytes_consumed = 0;
    uint16_t length = 0;
    uint16_t i = 0;

    if (pdu && (pdu_len >= 6)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[0], vmac_src);
        }
        if (vmac_dst) {
            decode_unsigned24(&pdu[3], vmac_dst);
        }
        length = pdu_len - 6;
        if (npdu && length && (length <= npdu_size)) {
            for (i = 0; i < length; i++) {
                npdu[i] = pdu[6+i];
            }
        }
        if (npdu_len) {
            *npdu_len = length;
        }
        bytes_consumed = pdu_len;
    }

    return bytes_consumed;
}

/** Encode the BVLC Original-Broadcast-NPDU message
 *
 * This message is used by B/IPv6 nodes which are not
 * foreign devices to broadcast NPDUs on a B/IPv6 network.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac - Source-Virtual-Address
 * @param npdu - BACnet NPDU buffer
 * @param npdu_len - size of the BACnet NPDU buffer
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'02'   Original-Broadcast-NPDU
 * BVLC Length:                 2-octets  L       Length of the BVLL message
 * Source-Virtual-Address:      3-octets
 * BACnet NPDU:                 Variable length
 */
int bvlc6_encode_original_broadcast(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac,
    uint8_t * npdu,
    uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 7;
    uint16_t i = 0;

    length += npdu_len;
    if (pdu &&
        (pdu_size >= length) &&
        (vmac <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_ORIGINAL_BROADCAST_NPDU, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac);
            if (npdu && length) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[7+i] = npdu[i];
                }
            }
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Original-Broadcast-NPDU message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac - decoded Source-Virtual-Address
 * @param npdu - buffer to copy the decoded BACnet NDPU
 * @param npdu_size - size of the buffer for the decoded BACnet NPDU
 * @param npdu_len - decoded length of the BACnet NPDU
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_original_broadcast(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac,
    uint8_t * npdu,
    uint16_t npdu_size,
    uint16_t * npdu_len)
{
    int bytes_consumed = 0;
    uint16_t length = 0;
    uint16_t i = 0;

    if (pdu && (pdu_len >= 3)) {
        if (vmac) {
            decode_unsigned24(&pdu[0], vmac);
        }
        length = pdu_len - 3;
        if (npdu && length && (length <= npdu_size)) {
            for (i = 0; i < length; i++) {
                npdu[i] = pdu[3+i];
            }
        }
        if (npdu_len) {
            *npdu_len = length;
        }
        bytes_consumed = pdu_len;
    }

    return bytes_consumed;
}

/** Encode the BVLC Address-Resolution message
 *
 * This message is unicast by B/IPv6 BBMDs to determine
 * the B/IPv6 address of a known virtual address belonging to
 * a different multicast domain.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_target - Target-Virtual-Address
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'03'   Address-Resolution
 * BVLC Length:                 2-octets  X'000A' Length of the BVLL message
 * Source-Virtual-Address:      3-octets
 * Target-Virtual-Address:      3-octets
 */
int bvlc6_encode_address_resolution(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_target)
{
    int bytes_encoded = 0;
    uint16_t length = 10;


    if (pdu && (pdu_size >= length) &&
        (vmac_src <= 0xFFFFFF) &&
        (vmac_target <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_ADDRESS_RESOLUTION, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac_src);
            encode_unsigned24(&pdu[7], vmac_target);
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Address-Resolution message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_target - Target-Virtual-Address
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_address_resolution(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac_src,
    uint32_t * vmac_target)
{
    int bytes_consumed = 0;

    if (pdu && (pdu_len >= 6)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[0], vmac_src);
        }
        if (vmac_target) {
            decode_unsigned24(&pdu[3], vmac_target);
        }
        bytes_consumed = 6;
    }

    return bytes_consumed;
}

/** Encode the BVLC Address
 *
 * Data link layer addressing between B/IPv6 nodes consists of a 128-bit
 * IPv6 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv6 address.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param bip6_address - B/IPv6 address
 *
 * @return number of bytes encoded
 */
int bvlc6_encode_address(
    uint8_t * pdu,
    uint16_t pdu_size,
    BACNET_IP6_ADDRESS * bip6_address)
{
    int bytes_encoded = 0;
    uint16_t length = BIP6_ADDRESS_MAX;
    unsigned i = 0;

    if (pdu && (pdu_size >= length) && bip6_address) {
        for (i = 0; i < IP6_ADDRESS_MAX; i++) {
            pdu[i] = bip6_address->address[i];
        }
        encode_unsigned16(&pdu[IP6_ADDRESS_MAX], bip6_address->port);
        bytes_encoded = length;
    }

    return bytes_encoded;
}

/** Decode the BVLC Address-Resolution message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param bip6_address - B/IPv6 address
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_address(
    uint8_t * pdu,
    uint16_t pdu_len,
    BACNET_IP6_ADDRESS * bip6_address)
{
    int bytes_consumed = 0;
    uint16_t length = BIP6_ADDRESS_MAX;
    unsigned i = 0;

    if (pdu && (pdu_len >= length) && bip6_address) {
        for (i = 0; i < IP6_ADDRESS_MAX; i++) {
            bip6_address->address[i] = pdu[i];
        }
        decode_unsigned16(&pdu[IP6_ADDRESS_MAX], &bip6_address->port);
        bytes_consumed = length;
    }

    return bytes_consumed;
}

/** Copy the BVLC Address
 *
 * Data link layer addressing between B/IPv6 nodes consists of a 128-bit
 * IPv6 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv6 address.
 *
 * @param dst - B/IPv6 address that will be filled with src
 * @param src - B/IPv6 address that will be copied into dst
 *
 * @return true if the address was copied
 */
bool bvlc6_address_copy(
    BACNET_IP6_ADDRESS * dst,
    BACNET_IP6_ADDRESS * src)
{
    bool status = false;
    unsigned int i = 0;

    if (src && dst) {
        for (i = 0; i < IP6_ADDRESS_MAX; i++) {
            dst->address[i] = src->address[i];
        }
        dst->port = src->port;
        status = true;
    }

    return status;
}

/** Compare the BVLC Address
 *
 * Data link layer addressing between B/IPv6 nodes consists of a 128-bit
 * IPv6 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv6 address.
 *
 * @param dst - B/IPv6 address that will be compared to src
 * @param src - B/IPv6 address that will be compared to dst
 *
 * @return true if the addresses are different
 */
bool bvlc6_address_different(
    BACNET_IP6_ADDRESS * dst,
    BACNET_IP6_ADDRESS * src)
{
    bool status = false;
    unsigned int i = 0;

    if (src && dst) {
        for (i = 0; i < IP6_ADDRESS_MAX; i++) {
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

/** Set a BVLC Address from 16-bit group chunks
 *
 * Data link layer addressing between B/IPv6 nodes consists of a 128-bit
 * IPv6 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv6 address.
 *
 * @param addr - B/IPv6 address that be set
 * @param addr0 - B/IPv6 address 16-bit
 * @param addr1 - B/IPv6 address bytes
 * @param addr2 - B/IPv6 address bytes
 * @param addr3 - B/IPv6 address bytes
 * @param addr4 - B/IPv6 address bytes
 * @param addr5 - B/IPv6 address bytes
 * @param addr6 - B/IPv6 address bytes
 * @param addr7 - B/IPv6 address bytes
 *
 * @return true if the address is set
 */
bool bvlc6_address_set(
    BACNET_IP6_ADDRESS * addr,
    uint16_t addr0,
    uint16_t addr1,
    uint16_t addr2,
    uint16_t addr3,
    uint16_t addr4,
    uint16_t addr5,
    uint16_t addr6,
    uint16_t addr7)
{
    bool status = false;

    if (addr) {
        encode_unsigned16(&addr->address[0], addr0);
        encode_unsigned16(&addr->address[2], addr1);
        encode_unsigned16(&addr->address[4], addr2);
        encode_unsigned16(&addr->address[6], addr3);
        encode_unsigned16(&addr->address[8], addr4);
        encode_unsigned16(&addr->address[10], addr5);
        encode_unsigned16(&addr->address[12], addr6);
        encode_unsigned16(&addr->address[14], addr7);
        status = true;
    }

    return status;
}

/** Get a BVLC Address into 16-bit group chunks
 *
 * Data link layer addressing between B/IPv6 nodes consists of a 128-bit
 * IPv6 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first). This
 * address shall be referred to as a B/IPv6 address.
 *
 * @param addr - B/IPv6 address that be set
 * @param addr0 - B/IPv6 address 16-bit
 * @param addr1 - B/IPv6 address bytes
 * @param addr2 - B/IPv6 address bytes
 * @param addr3 - B/IPv6 address bytes
 * @param addr4 - B/IPv6 address bytes
 * @param addr5 - B/IPv6 address bytes
 * @param addr6 - B/IPv6 address bytes
 * @param addr7 - B/IPv6 address bytes
 *
 * @return true if the address is set
 */
bool bvlc6_address_get(
    BACNET_IP6_ADDRESS * addr,
    uint16_t *addr0,
    uint16_t *addr1,
    uint16_t *addr2,
    uint16_t *addr3,
    uint16_t *addr4,
    uint16_t *addr5,
    uint16_t *addr6,
    uint16_t *addr7)
{
    bool status = false;

    if (addr) {
        if (addr0) {
            decode_unsigned16(&addr->address[0], addr0);
        }
        if (addr1) {
            decode_unsigned16(&addr->address[2], addr1);
        }
        if (addr2) {
            decode_unsigned16(&addr->address[4], addr2);
        }
        if (addr3) {
            decode_unsigned16(&addr->address[6], addr3);
        }
        if (addr4) {
            decode_unsigned16(&addr->address[8], addr4);
        }
        if (addr5) {
            decode_unsigned16(&addr->address[10], addr5);
        }
        if (addr6) {
            decode_unsigned16(&addr->address[12], addr6);
        }
        if (addr7) {
            decode_unsigned16(&addr->address[14], addr7);
        }
        status = true;
    }

    return status;
}

/** Set a BACnet VMAC Address from a Device ID
 *
 * @param addr - BACnet address that be set
 * @param device_id - 22-bit device ID
 *
 * @return true if the address is set
 */
bool bvlc6_vmac_address_set(
    BACNET_ADDRESS * addr,
    uint32_t device_id)
{
    bool status = false;

    if (addr) {
        encode_unsigned24(&addr->mac[0], device_id);
        addr->mac_len = 3;
        addr->net = 0;
        addr->len = 0;
        status = true;
    }

    return status;
}

/** Get a BACnet VMAC Address from a Device ID
 *
 * @param addr - BACnet address that be set
 * @param device_id - 22-bit device ID
 *
 * @return true if the address is set
 */
bool bvlc6_vmac_address_get(
    BACNET_ADDRESS * addr,
    uint32_t *device_id)
{
    bool status = false;

    if (addr && device_id) {
        if (addr->mac_len == 3) {
            decode_unsigned24(&addr->mac[0], device_id);
            status = true;
        }
    }

    return status;
}

/** Encode the BVLC Forwarded-Address-Resolution message
 *
 * This message is unicast by B/IPv6 BBMDs to determine
 * the B/IPv6 address of a known virtual address belonging to
 * a different multicast domain.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_target - Target-Virtual-Address
 * @param bip6_address - Original-Source-B/IPv6-Address
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:            1-octet   X'04'   Forwarded-Address-Resolution
 * BVLC Length:              2-octets  X'001C' Length of this message
 * Original-Source-Virtual-Address: 3-octets
 * Target-Virtual-Address:          3-octets
 * Original-Source-B/IPv6-Address  18-octets
 */
int bvlc6_encode_forwarded_address_resolution(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_target,
    BACNET_IP6_ADDRESS * bip6_address)
{
    int bytes_encoded = 0;
    uint16_t length = 0x001C;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length) &&
        (vmac_src <= 0xFFFFFF) &&
        (vmac_target <= 0xFFFFFF) &&
        bip6_address) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_FORWARDED_ADDRESS_RESOLUTION, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            encode_unsigned24(&pdu[offset], vmac_target);
            offset += 3;
            bvlc6_encode_address(&pdu[offset], pdu_size-offset, bip6_address);
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Forwarded-Address-Resolution message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_target - Target-Virtual-Address
 * @param bip6_address - Original-Source-B/IPv6-Address
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_forwarded_address_resolution(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac_src,
    uint32_t * vmac_target,
    BACNET_IP6_ADDRESS * bip6_address)
{
    int bytes_consumed = 0;
    const uint16_t length = 3 + 3 + BIP6_ADDRESS_MAX;
    uint16_t offset = 0;

    if (pdu && (pdu_len >= length)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[offset], vmac_src);
        }
        offset += 3;
        if (vmac_target) {
            decode_unsigned24(&pdu[offset], vmac_target);
        }
        offset += 3;
        if (bip6_address) {
            bvlc6_decode_address(&pdu[offset], pdu_len-offset, bip6_address);
        }
        bytes_consumed = length;
    }

    return bytes_consumed;
}

/** Encode generic BVLC Address-Ack message
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_dst - Destination-Virtual-Address
 *
 * @return number of bytes encoded
 */
static int bvlc6_encode_address_ack(
    uint8_t message_type,
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_dst)
{
    int bytes_encoded = 0;
    const uint16_t length = 10;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length) &&
        (vmac_src <= 0xFFFFFF) &&
        (vmac_dst <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            message_type, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            encode_unsigned24(&pdu[offset], vmac_dst);
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Encode the BVLC Address-Resolution-Ack message
 *
 * This message is the reply to either the Address-Resolution or
 * the Forwarded-Address-Resolution messages.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_dst - Destination-Virtual-Address
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'05'   Address-Resolution-Ack
 * BVLC Length:                 2-octets  X'000A' Length of the BVLL message
 * Source-Virtual-Address:      3-octets
 * Destination-Virtual-Address: 3-octets
 */
int bvlc6_encode_address_resolution_ack(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_dst)
{
    return bvlc6_encode_address_ack(
        BVLC6_ADDRESS_RESOLUTION_ACK,
        pdu, pdu_size,vmac_src, vmac_dst);
}

/** Decode the BVLC Address-Resolution-Ack message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_dst - Destination-Virtual-Address
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_address_resolution_ack(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac_src,
    uint32_t * vmac_dst)
{
    int bytes_consumed = 0;
    const uint16_t length = 6;
    uint16_t offset = 0;

    if (pdu && (pdu_len >= length)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[offset], vmac_src);
        }
        offset += 3;
        if (vmac_dst) {
            decode_unsigned24(&pdu[offset], vmac_dst);
        }
        bytes_consumed = length;
    }

    return bytes_consumed;
}

/** Encode the BVLC Virtual-Address-Resolution message
 *
 * This message is unicast by B/IPv6 nodes to determine the
 * virtual address of a device with a known B/IPv6 address.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Source-Virtual-Address
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'06'   Virtual-Address-Resolution
 * BVLC Length:                 2-octets  X'0007' Length of the BVLL message
 * Source-Virtual-Address:      3-octets
 */
int bvlc6_encode_virtual_address_resolution(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src)
{
    int bytes_encoded = 0;
    const uint16_t length = 7;

    if (pdu && (pdu_size >= length) &&
        (vmac_src <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_VIRTUAL_ADDRESS_RESOLUTION, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac_src);
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Virtual-Address-Resolution message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac_src - Source-Virtual-Address
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_virtual_address_resolution(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac_src)
{
    int bytes_consumed = 0;

    if (pdu && (pdu_len >= 3)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[0], vmac_src);
        }
        bytes_consumed = 3;
    }

    return bytes_consumed;
}

/** Encode the BVLC Virtual-Address-Resolution-Ack message
 *
 * This message is the reply to the Virtual-Address-Resolution message
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_dst - Destination-Virtual-Address
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'07'   Address-Resolution-Ack
 * BVLC Length:                 2-octets  X'000A' Length of the BVLL message
 * Source-Virtual-Address:      3-octets
 * Destination-Virtual-Address: 3-octets
 */
int bvlc6_encode_virtual_address_resolution_ack(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_dst)
{
    return bvlc6_encode_address_ack(
        BVLC6_VIRTUAL_ADDRESS_RESOLUTION_ACK,
        pdu, pdu_size,vmac_src, vmac_dst);
}

/** Decode the BVLC Virtual-Address-Resolution-Ack message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac_src - Source-Virtual-Address
 * @param vmac_dst - Destination-Virtual-Address
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_virtual_address_resolution_ack(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac_src,
    uint32_t * vmac_dst)
{
    return bvlc6_decode_address_resolution_ack(pdu, pdu_len,
        vmac_src, vmac_dst);
}

/** Encode the BVLC Forwarded-NPDU message
 *
 * This BVLL message is used in multicast messages from a BBMD
 * as well as in messages forwarded to registered foreign
 * devices. It contains the source address of the original
 * node as well as the original BACnet NPDU.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Original-Source-Virtual-Address
 * @param bip6_address - Original-Source-B/IPv6-Address
 * @param npdu - BACnet NPDU from Originating Device buffer
 * @param npdu_len - size of the BACnet NPDU buffer
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'08'   Forwarded-NPDU
 * BVLC Length:                 2-octets  L       Length of the BVLL message
 * Original-Source-Virtual-Address:      3-octets
 * Original-Source-B-IPv6-Address:      18-octets
 * BACnet NPDU from Originating Device:  N-octets (N=L-25)
 */
int bvlc6_encode_forwarded_npdu(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    BACNET_IP6_ADDRESS * bip6_address,
    uint8_t * npdu,
    uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 1+1+2+3+BIP6_ADDRESS_MAX;
    uint16_t i = 0;
    uint16_t offset = 0;

    length += npdu_len;
    if (pdu &&
        (pdu_size >= length) &&
        (vmac_src <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_FORWARDED_NPDU, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            bvlc6_encode_address(&pdu[offset], pdu_size-offset,
                bip6_address);
            offset += BIP6_ADDRESS_MAX;
            if (npdu && length) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[offset+i] = npdu[i];
                }
            }
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Forwarded-NPDU message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac_src - Original-Source-Virtual-Address
 * @param bip6_address - Original-Source-B/IPv6-Address
 * @param npdu - BACnet NPDU buffer
 * @param npdu_size - size of the buffer for the decoded BACnet NPDU
 * @param npdu_len - decoded length of the BACnet NPDU buffer
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_forwarded_npdu(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac_src,
    BACNET_IP6_ADDRESS * bip6_address,
    uint8_t * npdu,
    uint16_t npdu_size,
    uint16_t * npdu_len)
{
    int bytes_consumed = 0;
    uint16_t length = 0;
    uint16_t i = 0;
    const uint16_t address_len = 3+BIP6_ADDRESS_MAX;
    uint16_t offset = 0;

    if (pdu && (pdu_len >= address_len)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[offset], vmac_src);
        }
        offset += 3;
        if (bip6_address) {
            bvlc6_decode_address(&pdu[offset], pdu_len-offset,
                bip6_address);
        }
        offset += BIP6_ADDRESS_MAX;
        length = pdu_len - offset;
        if (npdu && length && (length <= npdu_size)) {
            for (i = 0; i < length; i++) {
                npdu[i] = pdu[offset+i];
            }
        }
        if (npdu_len) {
            *npdu_len = length;
        }
        bytes_consumed = pdu_len;
    }

    return bytes_consumed;
}

/** Encode the BVLC Register-Foreign-Device message
 *
 * This message allows a foreign device, as defined in X.4.5.1,
 * to register with a BBMD for the purpose of receiving
 * broadcast messages.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Source-Virtual-Address
 * @param ttl_seconds - Time-to-Live T, in seconds
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'09'   Register-Foreign-Device
 * BVLC Length:                 2-octets  X'0009' Length of the BVLL message
 * Source-Virtual-Address:      3-octets
 * Time-to-Live:                2-octets  T       Time-to-Live T, in seconds
 */
int bvlc6_encode_register_foreign_device(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint16_t ttl_seconds)
{
    int bytes_encoded = 0;
    const uint16_t length = 9;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length) &&
        (vmac_src <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_REGISTER_FOREIGN_DEVICE, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            encode_unsigned16(&pdu[offset], ttl_seconds);
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Register-Foreign-Device message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac_src - Source-Virtual-Address
 * @param ttl_seconds - Time-to-Live T, in seconds
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_register_foreign_device(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac_src,
    uint16_t * ttl_seconds)
{
    int bytes_consumed = 0;
    const uint16_t length = 5;
    uint16_t offset = 0;

    if (pdu && (pdu_len >= length)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[offset], vmac_src);
        }
        offset += 3;
        if (ttl_seconds) {
            decode_unsigned16(&pdu[offset], ttl_seconds);
        }
        bytes_consumed = length;
    }

    return bytes_consumed;
}

/** Encode the BVLC Delete-Foreign-Device message
 *
 * This message allows a foreign device, as defined in X.4.5.1,
 * to register with a BBMD for the purpose of receiving
 * broadcast messages.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac_src - Source-Virtual-Address
 * @param fdt_entry - FDT Entry
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'0A'   Delete-Foreign-Device
 * BVLC Length:                 2-octets  X'0019' Length of the BVLL message
 * Source-Virtual-Address:      3-octets
 * FDT Entry:                  18-octets
 */
int bvlc6_encode_delete_foreign_device(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY * fdt_entry)
{
    int bytes_encoded = 0;
    const uint16_t length = 0x0019;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length) &&
        (vmac_src <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_DELETE_FOREIGN_DEVICE, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            if (fdt_entry) {
                bvlc6_encode_address(&pdu[offset], pdu_size-offset,
                    &fdt_entry->bip6_address);
                offset += BIP6_ADDRESS_MAX;
                encode_unsigned16(&pdu[offset],
                    fdt_entry->ttl_seconds);
                offset += 2;
                encode_unsigned16(&pdu[offset],
                    fdt_entry->ttl_seconds_remaining);
                bytes_encoded = length;
            }
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Delete-Foreign-Device message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac_src - Source-Virtual-Address
 * @param fdt_entry - FDT Entry
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_delete_foreign_device(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac_src,
    BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY * fdt_entry)
{
    int bytes_consumed = 0;

    const uint16_t length = BIP6_ADDRESS_MAX+3+2+2-4;
    uint16_t offset = 0;

    if (pdu && (pdu_len >= length)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[offset], vmac_src);
            bytes_consumed = 3;
        }
        offset += 3;
        if (fdt_entry) {
            bvlc6_decode_address(&pdu[offset], pdu_len-offset,
                &fdt_entry->bip6_address);
            offset += BIP6_ADDRESS_MAX;
            decode_unsigned16(&pdu[offset],
                &fdt_entry->ttl_seconds);
            offset += 2;
            decode_unsigned16(&pdu[offset],
                &fdt_entry->ttl_seconds_remaining);
            bytes_consumed = length;
        }
        bytes_consumed = length;
    }

    return bytes_consumed;
}

/** Encode the BVLC Secure-BVLL message
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
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'0B'   Secure-BVLL
 * BVLC Length:                 2-octets  L       Length of the BVLL message
 * Security Wrapper:            Variable length
 */
int bvlc6_encode_secure_bvll(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint8_t * sbuf,
    uint16_t sbuf_len)
{
    int bytes_encoded = 0;
    uint16_t length = 4;
    uint16_t i = 0;

    length += sbuf_len;
    if (pdu &&
        (pdu_size >= length)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_SECURE_BVLL, length);
        if (bytes_encoded == 4) {
            if (sbuf && sbuf_len) {
                for (i = 0; i < sbuf_len; i++) {
                    pdu[4+i] = sbuf[i];
                }
            }
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Secure-BVLL message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param sbuf - Security Wrapper buffer
 * @param sbuf_size - size of the Security Wrapper buffer
 * @param sbuf_len - number of bytes decoded into the Security Wrapper buffer
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_secure_bvll(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint8_t * sbuf,
    uint16_t sbuf_size,
    uint16_t * sbuf_len)
{
    int bytes_consumed = 0;
    uint16_t i = 0;

    if (pdu && sbuf) {
        if (sbuf_len) {
            *sbuf_len = pdu_len;
        }
        if (pdu_len) {
            for (i = 0; i < pdu_len; i++) {
                sbuf[i] = pdu[i];
            }
        }
        bytes_consumed = pdu_len;
    }

    return bytes_consumed;
}

/** Encode the BVLC Distribute-Broadcast-To-Network message
 *
 * This message provides a mechanism whereby a foreign device
 * shall cause a BBMD to distribute a Forwarded-NPDU
 * BVLC to the local multicast domain, to all BBMD�s configured
 * in the BBMD�s BDT, and to all foreign devices in the
 * BBMD�s FDT.
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param vmac - Original-Source-Virtual-Address
 * @param npdu - BACnet NPDU from Originating Device buffer
 * @param npdu_len - size of the BACnet NPDU buffer
 *
 * @return number of bytes encoded
 *
 * BVLC Type:                   1-octet   X'82'   BVLL for BACnet/IPv6
 * BVLC Function:               1-octet   X'0C'   Original-Unicast-NPDU
 * BVLC Length:                 2-octets  L       Length of the BVLL message
 * Original-Source-Virtual-Address:      3-octets
 * BACnet NPDU from Originating Device:  Variable length
 */
int bvlc6_encode_distribute_broadcast_to_network(
    uint8_t * pdu,
    uint16_t pdu_size,
    uint32_t vmac,
    uint8_t * npdu,
    uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 7;
    uint16_t i = 0;


    length += npdu_len;
    if (pdu &&
        (pdu_size >= length) &&
        (vmac <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(pdu, pdu_size,
            BVLC6_DISTRIBUTE_BROADCAST_TO_NETWORK, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac);
            if (npdu && length) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[7+i] = npdu[i];
                }
            }
            bytes_encoded = length;
        }
    }

    return bytes_encoded;
}

/** Decode the BVLC Original-Broadcast-NPDU message
 *
 * @param pdu - buffer from which to decode the message
 * @param pdu_len - length of the buffer that needs decoding
 * @param vmac - decoded Original-Source-Virtual-Address
 * @param npdu - buffer to copy the decoded BACnet NDPU
 * @param npdu_size - size of the buffer for the decoded BACnet NPDU
 * @param npdu_len - decoded length of the BACnet NPDU
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_distribute_broadcast_to_network(
    uint8_t * pdu,
    uint16_t pdu_len,
    uint32_t * vmac,
    uint8_t * npdu,
    uint16_t npdu_size,
    uint16_t * npdu_len)
{
    int bytes_consumed = 0;
    uint16_t length = 0;
    uint16_t i = 0;

    if (pdu && (pdu_len >= 3)) {
        if (vmac) {
            decode_unsigned24(&pdu[0], vmac);
        }
        length = pdu_len - 3;
        if (npdu && length && (length <= npdu_size)) {
            for (i = 0; i < length; i++) {
                npdu[i] = pdu[3+i];
            }
        }
        if (npdu_len) {
            *npdu_len = length;
        }
        bytes_consumed = pdu_len;
    }

    return bytes_consumed;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

static void test_BVLC6_Address(
    Test * pTest,
    BACNET_IP6_ADDRESS * bip6_address_1,
    BACNET_IP6_ADDRESS * bip6_address_2)
{
    unsigned i = 0;

    if (bip6_address_1 && bip6_address_2) {
        ct_test(pTest, bip6_address_1->port == bip6_address_2->port);
        for (i = 0; i < IP6_ADDRESS_MAX; i++) {
            ct_test(pTest, bip6_address_1->address[i] ==
                bip6_address_2->address[i]);
        }
    }

    return;
}

static int test_BVLC6_Header(
    Test * pTest,
    uint8_t * pdu,
    uint16_t pdu_len,
    uint8_t * message_type,
    uint16_t * length)

{
    int bytes_consumed = 0;
    int len = 0;

    if (pdu && message_type && length) {
        len = bvlc6_decode_header(pdu, pdu_len, message_type,
            length);
        ct_test(pTest, len == 4);
        bytes_consumed = len;
    }

    return bytes_consumed;
}

static void test_BVLC6_Result_Code(
    Test * pTest,
    uint32_t vmac,
    uint16_t result_code)
{
    uint8_t pdu[50] = { 0 };
    uint32_t test_vmac = 0;
    uint16_t test_result_code = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;

    len = bvlc6_encode_result(pdu, sizeof(pdu), vmac, result_code);
    ct_test(pTest, len == 9);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_RESULT);
    ct_test(pTest, length == 9);
    test_len += bvlc6_decode_result(&pdu[4], length-4,
        &test_vmac, &test_result_code);
    ct_test(pTest, len == test_len);
    ct_test(pTest, vmac == test_vmac);
    ct_test(pTest, result_code == test_result_code);
    len = bvlc6_encode_result(pdu, sizeof(pdu), 0xffffff+1, result_code);
    ct_test(pTest, len == 0);
}

static void test_BVLC6_Result(
    Test * pTest)
{
    uint32_t vmac = 0;
    uint16_t result_code[6] = {
        BVLC6_RESULT_SUCCESSFUL_COMPLETION,
        BVLC6_RESULT_ADDRESS_RESOLUTION_NAK,
        BVLC6_RESULT_VIRTUAL_ADDRESS_RESOLUTION_NAK,
        BVLC6_RESULT_REGISTER_FOREIGN_DEVICE_NAK,
        BVLC6_RESULT_DELETE_FOREIGN_DEVICE_NAK,
        BVLC6_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK
    };
    unsigned int i = 0;

    vmac = 4194303;
    for (i = 0; i < 6; i++) {
        test_BVLC6_Result_Code(pTest, vmac, result_code[i]);
    }
}

static void test_BVLC6_Original_Unicast_NPDU_Message(
    Test * pTest,
    uint8_t * npdu,
    uint16_t npdu_len,
    uint32_t vmac_src,
    uint32_t vmac_dst)
{
    uint8_t test_npdu[50] = { 0 };
    uint8_t pdu[60] = { 0 };
    uint32_t test_vmac_src = 0;
    uint32_t test_vmac_dst = 0;
    uint16_t test_npdu_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc6_encode_original_unicast(pdu, sizeof(pdu),
        vmac_src, vmac_dst,
        npdu, npdu_len);
    msg_len = 10 + npdu_len;
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_ORIGINAL_UNICAST_NPDU);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_original_unicast(&pdu[4], length-4,
        &test_vmac_src, &test_vmac_dst,
        test_npdu, sizeof(test_npdu), &test_npdu_len);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac_src == test_vmac_src);
    ct_test(pTest, vmac_dst == test_vmac_dst);
    ct_test(pTest, npdu_len == test_npdu_len);
    for (i = 0; i < npdu_len; i++) {
        ct_test(pTest, npdu[i] == test_npdu[i]);
    }
}

static void test_BVLC6_Original_Unicast_NPDU(
    Test * pTest)
{
    uint8_t npdu[50] = { 0 };
    uint32_t vmac_src = 0;
    uint32_t vmac_dst = 0;
    uint16_t npdu_len = 0;
    uint16_t i = 0;

    test_BVLC6_Original_Unicast_NPDU_Message(pTest,
        npdu, npdu_len, vmac_src, vmac_dst);
    /* now with some NPDU data */
    for (i = 0; i < sizeof(npdu); i++) {
        npdu[i] = i;
    }
    npdu_len = sizeof(npdu);
    vmac_src = 4194303;
    vmac_dst = 4194302;
    test_BVLC6_Original_Unicast_NPDU_Message(pTest,
        npdu, npdu_len, vmac_src, vmac_dst);
}

static void test_BVLC6_Original_Broadcast_NPDU_Message(
    Test * pTest,
    uint8_t * npdu,
    uint16_t npdu_len,
    uint32_t vmac)
{
    uint8_t test_npdu[50] = { 0 };
    uint8_t pdu[60] = { 0 };
    uint32_t test_vmac = 0;
    uint16_t test_npdu_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc6_encode_original_broadcast(pdu, sizeof(pdu),
        vmac, npdu, npdu_len);
    msg_len = 7 + npdu_len;
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_ORIGINAL_BROADCAST_NPDU);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_original_broadcast(&pdu[4], length-4,
        &test_vmac, test_npdu, sizeof(test_npdu), &test_npdu_len);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac == test_vmac);
    ct_test(pTest, npdu_len == test_npdu_len);
    for (i = 0; i < npdu_len; i++) {
        ct_test(pTest, npdu[i] == test_npdu[i]);
    }
}

static void test_BVLC6_Original_Broadcast_NPDU(
    Test * pTest)
{
    uint8_t npdu[50] = { 0 };
    uint32_t vmac = 0;
    uint16_t npdu_len = 0;
    uint16_t i = 0;

    test_BVLC6_Original_Broadcast_NPDU_Message(pTest,
        npdu, npdu_len, vmac);
    /* now with some NPDU data */
    for (i = 0; i < sizeof(npdu); i++) {
        npdu[i] = i;
    }
    npdu_len = sizeof(npdu);
    vmac = 4194303;
    test_BVLC6_Original_Broadcast_NPDU_Message(pTest,
        npdu, npdu_len, vmac);
}

static void test_BVLC6_Address_Resolution_Message(
    Test * pTest,
    uint32_t vmac_src,
    uint32_t vmac_target)
{
    uint8_t pdu[60] = { 0 };
    uint32_t test_vmac_src = 0;
    uint32_t test_vmac_target = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;
    const int msg_len = 10;

    len = bvlc6_encode_address_resolution(pdu, sizeof(pdu),
        vmac_src, vmac_target);
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_ADDRESS_RESOLUTION);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_address_resolution(&pdu[4], length-4,
        &test_vmac_src, &test_vmac_target);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac_src == test_vmac_src);
    ct_test(pTest, vmac_target == test_vmac_target);
}

static void test_BVLC6_Address_Resolution(
    Test * pTest)
{
    uint32_t vmac_src = 0;
    uint32_t vmac_target = 0;

    test_BVLC6_Address_Resolution_Message(pTest,
        vmac_src, vmac_target);
    vmac_src = 4194303;
    vmac_target = 4194302;
    test_BVLC6_Address_Resolution_Message(pTest,
        vmac_src, vmac_target);
}

static void test_BVLC6_Forwarded_Address_Resolution_Message(
    Test * pTest,
    uint32_t vmac_src,
    uint32_t vmac_dst,
    BACNET_IP6_ADDRESS * bip6_address)
{
    BACNET_IP6_ADDRESS test_bip6_address = {{0}};
    uint8_t pdu[60] = { 0 };
    uint32_t test_vmac_src = 0;
    uint32_t test_vmac_dst = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;
    const int msg_len = 4 + 3 + 3 + BIP6_ADDRESS_MAX;

    len = bvlc6_encode_forwarded_address_resolution(
        pdu, sizeof(pdu),
        vmac_src, vmac_dst,
        bip6_address);
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_FORWARDED_ADDRESS_RESOLUTION);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_forwarded_address_resolution(
        &pdu[4], length-4,
        &test_vmac_src, &test_vmac_dst,
        &test_bip6_address);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac_src == test_vmac_src);
    ct_test(pTest, vmac_dst == test_vmac_dst);
    test_BVLC6_Address(pTest, bip6_address, &test_bip6_address);
}

static void test_BVLC6_Forwarded_Address_Resolution(
    Test * pTest)
{
    BACNET_IP6_ADDRESS bip6_address = {{0}};
    uint32_t vmac_src = 0;
    uint32_t vmac_target = 0;
    uint16_t i = 0;

    test_BVLC6_Forwarded_Address_Resolution_Message(pTest,
        vmac_src, vmac_target, &bip6_address);
    /* now with some address data */
    for (i = 0; i < sizeof(bip6_address.address); i++) {
        bip6_address.address[i] = i;
    }
    bip6_address.port = 47808;
    vmac_src = 4194303;
    vmac_target = 4194302;
    test_BVLC6_Forwarded_Address_Resolution_Message(pTest,
        vmac_src, vmac_target, &bip6_address);
}

static void test_BVLC6_Address_Resolution_Ack_Message(
    Test * pTest,
    uint32_t vmac_src,
    uint32_t vmac_dst)
{
    uint8_t pdu[60] = { 0 };
    uint32_t test_vmac_src = 0;
    uint32_t test_vmac_dst = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;
    const int msg_len = 10;

    len = bvlc6_encode_address_resolution_ack(pdu, sizeof(pdu),
        vmac_src, vmac_dst);
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_ADDRESS_RESOLUTION_ACK);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_address_resolution_ack(&pdu[4], length-4,
        &test_vmac_src, &test_vmac_dst);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac_src == test_vmac_src);
    ct_test(pTest, vmac_dst == test_vmac_dst);
}

static void test_BVLC6_Address_Resolution_Ack(
    Test * pTest)
{
    uint32_t vmac_src = 0;
    uint32_t vmac_dst = 0;

    test_BVLC6_Address_Resolution_Ack_Message(pTest,
        vmac_src, vmac_dst);
    vmac_src = 4194303;
    vmac_dst = 4194302;
    test_BVLC6_Address_Resolution_Ack_Message(pTest,
        vmac_src, vmac_dst);
}

static void test_BVLC6_Virtual_Address_Resolution_Message(
    Test * pTest,
    uint32_t vmac_src)
{
    uint8_t pdu[60] = { 0 };
    uint32_t test_vmac_src = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;
    const int msg_len = 7;

    len = bvlc6_encode_virtual_address_resolution(pdu, sizeof(pdu),
        vmac_src);
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_VIRTUAL_ADDRESS_RESOLUTION);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_virtual_address_resolution(&pdu[4], length-4,
        &test_vmac_src);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac_src == test_vmac_src);
}

static void test_BVLC6_Virtual_Address_Resolution(
    Test * pTest)
{
    uint32_t vmac_src = 0;

    test_BVLC6_Virtual_Address_Resolution_Message(pTest,
        vmac_src);
    vmac_src = 0x1234;
    test_BVLC6_Virtual_Address_Resolution_Message(pTest,
        vmac_src);
}

static void test_BVLC6_Virtual_Address_Resolution_Ack_Message(
    Test * pTest,
    uint32_t vmac_src,
    uint32_t vmac_dst)
{
    uint8_t pdu[60] = { 0 };
    uint32_t test_vmac_src = 0;
    uint32_t test_vmac_dst = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;
    const int msg_len = 10;

    len = bvlc6_encode_virtual_address_resolution_ack(pdu, sizeof(pdu),
        vmac_src, vmac_dst);
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_VIRTUAL_ADDRESS_RESOLUTION_ACK);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_virtual_address_resolution_ack(&pdu[4], length-4,
        &test_vmac_src, &test_vmac_dst);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac_src == test_vmac_src);
    ct_test(pTest, vmac_dst == test_vmac_dst);
}

static void test_BVLC6_Virtual_Address_Resolution_Ack(
    Test * pTest)
{
    uint32_t vmac_src = 0;
    uint32_t vmac_dst = 0;

    test_BVLC6_Virtual_Address_Resolution_Ack_Message(pTest,
        vmac_src, vmac_dst);
    vmac_src = 4194303;
    vmac_dst = 4194302;
    test_BVLC6_Virtual_Address_Resolution_Ack_Message(pTest,
        vmac_src, vmac_dst);
}

static void test_BVLC6_Forwarded_NPDU_Message(
    Test * pTest,
    uint8_t * npdu,
    uint16_t npdu_len,
    uint32_t vmac_src,
    BACNET_IP6_ADDRESS * bip6_address)
{
    uint8_t test_npdu[50] = { 0 };
    uint8_t pdu[75] = { 0 };
    uint32_t test_vmac_src = 0;
    BACNET_IP6_ADDRESS test_bip6_address = {{0}};
    uint16_t test_npdu_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc6_encode_forwarded_npdu(pdu, sizeof(pdu),
        vmac_src, bip6_address,
        npdu, npdu_len);
    msg_len = 1+1+2+3+BIP6_ADDRESS_MAX+npdu_len;
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_FORWARDED_NPDU);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_forwarded_npdu(&pdu[4], length-4,
        &test_vmac_src, &test_bip6_address,
        test_npdu, sizeof(test_npdu), &test_npdu_len);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac_src == test_vmac_src);
    test_BVLC6_Address(pTest, bip6_address, &test_bip6_address);
    ct_test(pTest, npdu_len == test_npdu_len);
    for (i = 0; i < npdu_len; i++) {
        ct_test(pTest, npdu[i] == test_npdu[i]);
    }
}

static void test_BVLC6_Forwarded_NPDU(
    Test * pTest)
{
    uint8_t npdu[50] = { 0 };
    uint32_t vmac_src = 0;
    BACNET_IP6_ADDRESS bip6_address = {{0}};
    uint16_t npdu_len = 0;
    uint16_t i = 0;

    test_BVLC6_Forwarded_NPDU_Message(pTest,
        npdu, npdu_len, vmac_src, &bip6_address);
    for (i = 0; i < sizeof(bip6_address.address); i++) {
        bip6_address.address[i] = i;
    }
    bip6_address.port = 47808;
    /* now with some NPDU data */
    for (i = 0; i < sizeof(npdu); i++) {
        npdu[i] = i;
    }
    npdu_len = sizeof(npdu);
    vmac_src = 4194303;
    test_BVLC6_Forwarded_NPDU_Message(pTest,
        npdu, npdu_len, vmac_src, &bip6_address);
}

static void test_BVLC6_Register_Foreign_Device_Message(
    Test * pTest,
    uint32_t vmac_src,
    uint16_t ttl_seconds)
{
    uint8_t pdu[60] = { 0 };
    uint32_t test_vmac_src = 0;
    uint16_t test_ttl_seconds = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;
    const int msg_len = 9;

    len = bvlc6_encode_register_foreign_device(pdu, sizeof(pdu),
        vmac_src, ttl_seconds);
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_REGISTER_FOREIGN_DEVICE);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_register_foreign_device(&pdu[4], length-4,
        &test_vmac_src, &test_ttl_seconds);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac_src == test_vmac_src);
    ct_test(pTest, ttl_seconds == test_ttl_seconds);
}

static void test_BVLC6_Register_Foreign_Device(
    Test * pTest)
{
    uint32_t vmac_src = 0;
    uint16_t ttl_seconds = 0;

    test_BVLC6_Register_Foreign_Device_Message(pTest,
        vmac_src, ttl_seconds);
    vmac_src = 4194303;
    ttl_seconds = 600;
    test_BVLC6_Register_Foreign_Device_Message(pTest,
        vmac_src, ttl_seconds);
}

static void test_BVLC6_Delete_Foreign_Device_Message(
    Test * pTest,
    uint32_t vmac_src,
    BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry)
{
    uint8_t pdu[64] = { 0 };
    uint32_t test_vmac_src = 0;
    BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY test_fdt_entry = {0};
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;
    const int msg_len = 0x0019;

    len = bvlc6_encode_delete_foreign_device(pdu, sizeof(pdu),
        vmac_src, fdt_entry);
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_DELETE_FOREIGN_DEVICE);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_delete_foreign_device(&pdu[4], length-4,
        &test_vmac_src, &test_fdt_entry);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac_src == test_vmac_src);
    test_BVLC6_Address(pTest, &fdt_entry->bip6_address,
        &test_fdt_entry.bip6_address);
    ct_test(pTest, fdt_entry->ttl_seconds == test_fdt_entry.ttl_seconds);
    ct_test(pTest, fdt_entry->ttl_seconds_remaining ==
        test_fdt_entry.ttl_seconds_remaining);
}

static void test_BVLC6_Delete_Foreign_Device(
    Test * pTest)
{
    uint32_t vmac_src = 0;
    BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY fdt_entry = {0};
    unsigned int i = 0;

    /* test with zeros */
    test_BVLC6_Delete_Foreign_Device_Message(pTest,
        vmac_src, &fdt_entry);
    /* test with valid values */
    vmac_src = 4194303;
    for (i = 0; i < sizeof(fdt_entry.bip6_address.address); i++) {
        fdt_entry.bip6_address.address[i] = i;
    }
    fdt_entry.bip6_address.port = 47808;
    fdt_entry.ttl_seconds = 600;
    fdt_entry.ttl_seconds_remaining = 42;
    fdt_entry.next = NULL;
    test_BVLC6_Delete_Foreign_Device_Message(pTest,
        vmac_src, &fdt_entry);
}

static void test_BVLC6_Secure_BVLL_Message(
    Test * pTest,
    uint8_t * sbuf,
    uint16_t sbuf_len)
{
    uint8_t test_sbuf[50] = { 0 };
    uint8_t pdu[60] = { 0 };
    uint16_t test_sbuf_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc6_encode_secure_bvll(pdu, sizeof(pdu),
        sbuf, sbuf_len);
    msg_len = 1+1+2+sbuf_len;
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_SECURE_BVLL);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_secure_bvll(&pdu[4], length-4,
        test_sbuf, sizeof(test_sbuf), &test_sbuf_len);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, sbuf_len == test_sbuf_len);
    for (i = 0; i < sbuf_len; i++) {
        ct_test(pTest, sbuf[i] == test_sbuf[i]);
    }
}

static void test_BVLC6_Secure_BVLL(
    Test * pTest)
{
    uint8_t sbuf[50] = { 0 };
    uint16_t sbuf_len = 0;
    uint16_t i = 0;

    test_BVLC6_Secure_BVLL_Message(pTest,
        sbuf, sbuf_len);
    /* now with some NPDU data */
    for (i = 0; i < sizeof(sbuf); i++) {
        sbuf[i] = i;
    }
    sbuf_len = sizeof(sbuf);
    test_BVLC6_Secure_BVLL_Message(pTest,
        sbuf, sbuf_len);
}

static void test_BVLC6_Distribute_Broadcast_To_Network_Message(
    Test * pTest,
    uint8_t * npdu,
    uint16_t npdu_len,
    uint32_t vmac)
{
    uint8_t test_npdu[50] = { 0 };
    uint8_t pdu[60] = { 0 };
    uint32_t test_vmac = 0;
    uint16_t test_npdu_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc6_encode_distribute_broadcast_to_network(pdu, sizeof(pdu),
        vmac, npdu, npdu_len);
    msg_len = 7 + npdu_len;
    ct_test(pTest, len == msg_len);
    test_len = test_BVLC6_Header(pTest,
        pdu, len, &message_type, &length);
    ct_test(pTest, test_len == 4);
    ct_test(pTest, message_type == BVLC6_DISTRIBUTE_BROADCAST_TO_NETWORK);
    ct_test(pTest, length == msg_len);
    test_len += bvlc6_decode_distribute_broadcast_to_network(
        &pdu[4], length-4,
        &test_vmac, test_npdu, sizeof(test_npdu), &test_npdu_len);
    ct_test(pTest, len == test_len);
    ct_test(pTest, msg_len == test_len);
    ct_test(pTest, vmac == test_vmac);
    ct_test(pTest, npdu_len == test_npdu_len);
    for (i = 0; i < npdu_len; i++) {
        ct_test(pTest, npdu[i] == test_npdu[i]);
    }
}

static void test_BVLC6_Distribute_Broadcast_To_Network(
    Test * pTest)
{
    uint8_t npdu[50] = { 0 };
    uint32_t vmac = 0;
    uint16_t npdu_len = 0;
    uint16_t i = 0;

    test_BVLC6_Distribute_Broadcast_To_Network_Message(pTest,
        npdu, npdu_len, vmac);
    /* now with some NPDU data */
    for (i = 0; i < sizeof(npdu); i++) {
        npdu[i] = i;
    }
    npdu_len = sizeof(npdu);
    vmac = 4194303;
    test_BVLC6_Distribute_Broadcast_To_Network_Message(pTest,
        npdu, npdu_len, vmac);
}

static void test_BVLC6_Address_Copy(
    Test * pTest)
{
    unsigned int i = 0;
    BACNET_IP6_ADDRESS src = {{0}};
    BACNET_IP6_ADDRESS dst = {{0}};
    bool status = false;

    /* test with zeros */
    status = bvlc6_address_copy(&dst, &src);
    ct_test(pTest, status);
    status = bvlc6_address_different(&dst, &src);
    ct_test(pTest, !status);
    /* test with valid values */
    for (i = 0; i < sizeof(src.address); i++) {
        src.address[i] = 1 + i;
    }
    src.port = 47808;
    status = bvlc6_address_copy(&dst, &src);
    ct_test(pTest, status);
    status = bvlc6_address_different(&dst, &src);
    ct_test(pTest, !status);
    /* test for different port */
    dst.port = 47809;
    status = bvlc6_address_different(&dst, &src);
    ct_test(pTest, status);
    /* test for different address */
    dst.port = src.port;
    for (i = 0; i < sizeof(src.address); i++) {
        dst.address[i] = 0;
        status = bvlc6_address_different(&dst, &src);
        ct_test(pTest, status);
        dst.address[i] = 1 + i;
    }
}

static void test_BVLC6_Address_Get_Set(
    Test * pTest)
{
    uint16_t i = 0;
    BACNET_IP6_ADDRESS src = {{0}};
    uint16_t group = 1;
    uint16_t test_group = 0;
    bool status = false;

    for (i = 0; i < 16; i++) {
        status = bvlc6_address_set(&src,
            group, 0, 0, 0, 0, 0, 0, 0);
        ct_test(pTest, status);
        status = bvlc6_address_get(&src,
            &test_group, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        ct_test(pTest, status);
        ct_test(pTest, group == test_group);
        group = group<<1;
    }
}

static void test_BVLC6_VMAC_Address_Get_Set(
    Test * pTest)
{
    uint16_t i = 0;
    BACNET_ADDRESS addr;
    uint32_t device_id = 1;
    uint32_t test_device_id = 0;
    bool status = false;

    for (i = 0; i < 24; i++) {
        status = bvlc6_vmac_address_set(&addr, device_id);
        ct_test(pTest, status);
        ct_test(pTest, addr.mac_len == 3);
        ct_test(pTest, addr.net == 0);
        ct_test(pTest, addr.len == 0);
        status = bvlc6_vmac_address_get(&addr, &test_device_id);
        ct_test(pTest, status);
        ct_test(pTest, device_id == test_device_id);
        device_id = device_id<<1;
    }
}

void test_BVLC6(
    Test * pTest)
{
    bool rc;

    /* individual tests */
    rc = ct_addTestFunction(pTest, test_BVLC6_Result);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Original_Unicast_NPDU);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Original_Broadcast_NPDU);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Address_Resolution);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Forwarded_Address_Resolution);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Address_Resolution_Ack);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Virtual_Address_Resolution);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Virtual_Address_Resolution_Ack);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Forwarded_NPDU);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Register_Foreign_Device);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Delete_Foreign_Device);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Secure_BVLL);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Distribute_Broadcast_To_Network);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Address_Copy);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_Address_Get_Set);
    assert(rc);
    rc = ct_addTestFunction(pTest, test_BVLC6_VMAC_Address_Get_Set);
    assert(rc);
}

#ifdef TEST_BVLC6
int main(
    void)
{
    Test *pTest;

    pTest = ct_create("BACnet Virtual Link Control IP/v6", NULL);
    test_BVLC6(pTest);
    /* configure output */
    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_BBMD */
#endif /* TEST */
