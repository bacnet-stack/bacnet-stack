/**
 * @file
 * @brief BACnet/IPv6 virtual link control module encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 * @defgroup DLBIP6 BACnet/IPv6 DataLink Network Layer
 * @ingroup DataLink
 */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <stdio.h>
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bvlc6.h"
#include "bacnet/hostnport.h"

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
    uint8_t *pdu, uint16_t pdu_size, uint8_t message_type, uint16_t length)
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *message_type,
    uint16_t *length)
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
 *                                  X'00A0' Delete-Foreign-Device-Table-Entry
 * NAK X'00C0' Distribute-Broadcast-To-Network NAK
 */
int bvlc6_encode_result(
    uint8_t *pdu, uint16_t pdu_size, uint32_t vmac, uint16_t result_code)
{
    int bytes_encoded = 0;
    const uint16_t length = 9;

    if (pdu && (pdu_size >= 9) && (vmac <= 0xFFFFFF)) {
        bytes_encoded =
            bvlc6_encode_header(pdu, pdu_size, BVLC6_RESULT, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac);
            encode_unsigned16(&pdu[7], result_code);
            bytes_encoded = (int)length;
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
    const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac, uint16_t *result_code)
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
    uint8_t *pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_dst,
    const uint8_t *npdu,
    uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 10;
    uint16_t i = 0;

    length += npdu_len;
    if (pdu && (pdu_size >= length) && (vmac_src <= 0xFFFFFF) &&
        (vmac_dst <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(
            pdu, pdu_size, BVLC6_ORIGINAL_UNICAST_NPDU, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac_src);
            encode_unsigned24(&pdu[7], vmac_dst);
            if (npdu && length) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[10 + i] = npdu[i];
                }
            }
            bytes_encoded = (int)length;
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac_src,
    uint32_t *vmac_dst,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len)
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
                npdu[i] = pdu[6 + i];
            }
        }
        if (npdu_len) {
            *npdu_len = length;
        }
        bytes_consumed = (int)pdu_len;
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
    uint8_t *pdu,
    uint16_t pdu_size,
    uint32_t vmac,
    const uint8_t *npdu,
    uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 7;
    uint16_t i = 0;

    length += npdu_len;
    if (pdu && (pdu_size >= length) && (vmac <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(
            pdu, pdu_size, BVLC6_ORIGINAL_BROADCAST_NPDU, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac);
            if (npdu && npdu_len) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[7 + i] = npdu[i];
                }
            }
            bytes_encoded = (int)length;
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len)
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
                npdu[i] = pdu[3 + i];
            }
        }
        if (npdu_len) {
            *npdu_len = length;
        }
        bytes_consumed = (int)pdu_len;
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
    uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint32_t vmac_target)
{
    int bytes_encoded = 0;
    uint16_t length = 10;

    if (pdu && (pdu_size >= length) && (vmac_src <= 0xFFFFFF) &&
        (vmac_target <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(
            pdu, pdu_size, BVLC6_ADDRESS_RESOLUTION, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac_src);
            encode_unsigned24(&pdu[7], vmac_target);
            bytes_encoded = (int)length;
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac_src,
    uint32_t *vmac_target)
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
    uint8_t *pdu, uint16_t pdu_size, const BACNET_IP6_ADDRESS *bip6_address)
{
    int bytes_encoded = 0;
    uint16_t length = BIP6_ADDRESS_MAX;
    unsigned i = 0;

    if (pdu && (pdu_size >= length) && bip6_address) {
        for (i = 0; i < IP6_ADDRESS_MAX; i++) {
            pdu[i] = bip6_address->address[i];
        }
        encode_unsigned16(&pdu[IP6_ADDRESS_MAX], bip6_address->port);
        bytes_encoded = (int)length;
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
    const uint8_t *pdu, uint16_t pdu_len, BACNET_IP6_ADDRESS *bip6_address)
{
    int bytes_consumed = 0;
    uint16_t length = BIP6_ADDRESS_MAX;
    unsigned i = 0;

    if (pdu && (pdu_len >= length) && bip6_address) {
        for (i = 0; i < IP6_ADDRESS_MAX; i++) {
            bip6_address->address[i] = pdu[i];
        }
        decode_unsigned16(&pdu[IP6_ADDRESS_MAX], &bip6_address->port);
        bytes_consumed = (int)length;
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
bool bvlc6_address_copy(BACNET_IP6_ADDRESS *dst, const BACNET_IP6_ADDRESS *src)
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
    const BACNET_IP6_ADDRESS *dst, const BACNET_IP6_ADDRESS *src)
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
    BACNET_IP6_ADDRESS *addr,
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
    const BACNET_IP6_ADDRESS *addr,
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

/**
 * @brief Shift the buffer pointer and decrease the size after an snprintf
 * @param len - number of bytes (excluding terminating NULL byte) from snprintf
 * @param buf - pointer to the buffer pointer
 * @param buf_size - pointer to the buffer size
 * @return number of bytes (excluding terminating NULL byte) from snprintf
 */
static int snprintf_shift(int len, char **buf, size_t *buf_size)
{
    if (buf) {
        if (*buf) {
            *buf += len;
        }
    }
    if (buf_size) {
        if ((*buf_size) >= len) {
            *buf_size -= len;
        } else {
            *buf_size = 0;
        }
    }

    return len;
}

/** Convert IPv6 Address from ASCII
 *
 * IPv6 addresses are represented as eight groups, separated by colons,
 * of four hexadecimal digits.
 *
 * For convenience, an IPv6 address may be abbreviated to shorter notations
 * by application of the following rules according to RFC 5952 [1]:
 *   - One or more leading zeros from any groups of hexadecimal digits
 *     are removed; this is usually done to either all or none of the
 *     leading zeros. For example, the group 0042 is converted to 42.
 *   - Consecutive sections of zeros are replaced with a double colon (::).
 *     The double colon may only be used once in an address, as multiple
 *     use would render the address indeterminate. RFC 5952 requires that
 *     a double colon not be used to denote an omitted single section of
 *     zeros.
 *
 * [1] https://www.rfc-editor.org/rfc/rfc5952
 *
 * Adapted from the uIP TCP/IP stack and the Contiki operating system.
 * Thank you, Adam Dunkel, and the Swedish Institute of Computer Science.
 *
 * @param addr - B/IPv6 address that is parsed
 * @param buf - B/IPv6 address in 16-bit ASCII hex compressed format
 * @param buf_size - B/IPv6 address size in bytes
 *
 * @return the number of characters which would be generated for the given
 *  input, excluding the trailing null.
 * @note buf and buf_size may be null and zero to return only the size
 */
int bvlc6_address_to_ascii(
    const BACNET_IP6_ADDRESS *addr, char *buf, size_t buf_size)
{
    uint16_t a;
    unsigned int i;
    int f = 0;
    int len = 0;
    int n = 0;

    if (!addr) {
        return n;
    }
    if (!buf) {
        return n;
    }
    for (i = 0; i < IP6_ADDRESS_MAX; i += 2) {
        a = (addr->address[i] << 8) + addr->address[i + 1];
        if ((a == 0) && (f >= 0)) {
            if (f++ == 0) {
                len = snprintf(buf, buf_size, "::");
                n += snprintf_shift(len, &buf, &buf_size);
            }
        } else {
            if (f > 0) {
                f = -1;
            } else if (i > 0) {
                len = snprintf(buf, buf_size, ":");
                n += snprintf_shift(len, &buf, &buf_size);
            }
            len = snprintf(buf, buf_size, "%x", a);
            n += snprintf_shift(len, &buf, &buf_size);
        }
    }

    return true;
}

/** Convert IPv6 Address from ASCII
 *
 * IPv6 addresses are represented as eight groups, separated by colons,
 * of four hexadecimal digits.
 *
 * For convenience, an IPv6 address may be abbreviated to shorter notations
 * by application of the following rules.
 *   - One or more leading zeros from any groups of hexadecimal digits
 *     are removed; this is usually done to either all or none of the
 *     leading zeros. For example, the group 0042 is converted to 42.
 *   - Consecutive sections of zeros are replaced with a double colon (::).
 *     The double colon may only be used once in an address, as multiple
 *     use would render the address indeterminate. RFC 5952 requires that
 *     a double colon not be used to denote an omitted single section of
 *     zeros.
 *
 * Adapted from the uIP TCP/IP stack and the Contiki operating system.
 * Thank you, Adam Dunkel, and the Swedish Institute of Computer Science.
 *
 * @param addr - B/IPv6 address that is set
 * @param addrstr - B/IPv6 address in 16-bit ASCII hex compressed format
 *
 * @return true if a valid address was set
 */
bool bvlc6_address_from_ascii(BACNET_IP6_ADDRESS *addr, const char *addrstr)
{
    uint16_t value = 0;
    int tmp, zero = -1;
    unsigned int len = 0;
    unsigned int i = 0;
    char c = 0;

    if (!addr) {
        return false;
    }
    if (!addrstr) {
        return false;
    }
    if (*addrstr == '[') {
        addrstr++;
    }
    for (len = 0; len < IP6_ADDRESS_MAX - 1; addrstr++) {
        c = *addrstr;
        if (c == ':' || c == '\0' || c == ']' || c == '/') {
            addr->address[len] = (value >> 8) & 0xff;
            addr->address[len + 1] = value & 0xff;
            len += 2;
            value = 0;
            if (c == '\0' || c == ']' || c == '/') {
                break;
            }
            if (*(addrstr + 1) == ':') {
                /* Zero compression */
                if (zero < 0) {
                    zero = len;
                }
                addrstr++;
            }
        } else {
            if (c >= '0' && c <= '9') {
                tmp = c - '0';
            } else if (c >= 'a' && c <= 'f') {
                tmp = c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                tmp = c - 'A' + 10;
            } else {
                /* illegal char! */
                return false;
            }
            value = (value << 4) + (tmp & 0xf);
        }
    }
    if (c != '\0' && c != ']' && c != '/') {
        /* expected termination. too long! */
        return false;
    }
    if (len < IP6_ADDRESS_MAX) {
        uint8_t addr_end[IP6_ADDRESS_MAX];
        if (zero < 0) {
            /* too short address! */
            return false;
        }
        /* move end address values from zero position to end position */
        for (i = 0; i < len - zero; i++) {
            addr_end[i] = addr->address[zero + i];
        }
        for (i = 0; i < len - zero; i++) {
            addr->address[zero + IP6_ADDRESS_MAX - len + i] = addr_end[i];
        }
        /* fill in middle zero values */
        for (i = 0; i < (IP6_ADDRESS_MAX - len); i++) {
            addr->address[zero + i] = 0;
        }
    }
    /* note: should we look for []:BAC0 UDP port number? */

    return true;
}

/** Set a BACnet VMAC Address from a Device ID
 *
 * @param addr - BACnet address that be set
 * @param device_id - 22-bit device ID
 *
 * @return true if the address is set
 */
bool bvlc6_vmac_address_set(BACNET_ADDRESS *addr, uint32_t device_id)
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
bool bvlc6_vmac_address_get(const BACNET_ADDRESS *addr, uint32_t *device_id)
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
    uint8_t *pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_target,
    const BACNET_IP6_ADDRESS *bip6_address)
{
    int bytes_encoded = 0;
    uint16_t length = 0x001C;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length) && (vmac_src <= 0xFFFFFF) &&
        (vmac_target <= 0xFFFFFF) && bip6_address) {
        bytes_encoded = bvlc6_encode_header(
            pdu, pdu_size, BVLC6_FORWARDED_ADDRESS_RESOLUTION, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            encode_unsigned24(&pdu[offset], vmac_target);
            offset += 3;
            bvlc6_encode_address(&pdu[offset], pdu_size - offset, bip6_address);
            bytes_encoded = (int)length;
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac_src,
    uint32_t *vmac_target,
    BACNET_IP6_ADDRESS *bip6_address)
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
            bvlc6_decode_address(&pdu[offset], pdu_len - offset, bip6_address);
        }
        bytes_consumed = (int)length;
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
    uint8_t *pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    uint32_t vmac_dst)
{
    int bytes_encoded = 0;
    const uint16_t length = 10;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length) && (vmac_src <= 0xFFFFFF) &&
        (vmac_dst <= 0xFFFFFF)) {
        bytes_encoded =
            bvlc6_encode_header(pdu, pdu_size, message_type, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            encode_unsigned24(&pdu[offset], vmac_dst);
            bytes_encoded = (int)length;
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
    uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint32_t vmac_dst)
{
    return bvlc6_encode_address_ack(
        BVLC6_ADDRESS_RESOLUTION_ACK, pdu, pdu_size, vmac_src, vmac_dst);
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac_src,
    uint32_t *vmac_dst)
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
        bytes_consumed = (int)length;
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
    uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src)
{
    int bytes_encoded = 0;
    const uint16_t length = 7;

    if (pdu && (pdu_size >= length) && (vmac_src <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(
            pdu, pdu_size, BVLC6_VIRTUAL_ADDRESS_RESOLUTION, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac_src);
            bytes_encoded = (int)length;
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
    const uint8_t *pdu, uint16_t pdu_len, uint32_t *vmac_src)
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
    uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint32_t vmac_dst)
{
    return bvlc6_encode_address_ack(
        BVLC6_VIRTUAL_ADDRESS_RESOLUTION_ACK, pdu, pdu_size, vmac_src,
        vmac_dst);
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac_src,
    uint32_t *vmac_dst)
{
    return bvlc6_decode_address_resolution_ack(
        pdu, pdu_len, vmac_src, vmac_dst);
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
    uint8_t *pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    const BACNET_IP6_ADDRESS *bip6_address,
    const uint8_t *npdu,
    uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 1 + 1 + 2 + 3 + BIP6_ADDRESS_MAX;
    uint16_t i = 0;
    uint16_t offset = 0;

    length += npdu_len;
    if (pdu && (pdu_size >= length) && (vmac_src <= 0xFFFFFF)) {
        bytes_encoded =
            bvlc6_encode_header(pdu, pdu_size, BVLC6_FORWARDED_NPDU, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            bvlc6_encode_address(&pdu[offset], pdu_size - offset, bip6_address);
            offset += BIP6_ADDRESS_MAX;
            if (npdu && length) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[offset + i] = npdu[i];
                }
            }
            bytes_encoded = (int)length;
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac_src,
    BACNET_IP6_ADDRESS *bip6_address,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len)
{
    int bytes_consumed = 0;
    uint16_t length = 0;
    uint16_t i = 0;
    const uint16_t address_len = 3 + BIP6_ADDRESS_MAX;
    uint16_t offset = 0;

    if (pdu && (pdu_len >= address_len)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[offset], vmac_src);
        }
        offset += 3;
        if (bip6_address) {
            bvlc6_decode_address(&pdu[offset], pdu_len - offset, bip6_address);
        }
        offset += BIP6_ADDRESS_MAX;
        length = pdu_len - offset;
        if (npdu && length && (length <= npdu_size)) {
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
    uint8_t *pdu, uint16_t pdu_size, uint32_t vmac_src, uint16_t ttl_seconds)
{
    int bytes_encoded = 0;
    const uint16_t length = 9;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length) && (vmac_src <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(
            pdu, pdu_size, BVLC6_REGISTER_FOREIGN_DEVICE, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            encode_unsigned16(&pdu[offset], ttl_seconds);
            bytes_encoded = (int)length;
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac_src,
    uint16_t *ttl_seconds)
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
        bytes_consumed = (int)length;
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
 * FDT Entry:                  18-octets  The FDT entry is the B/IPv6 address
 *                                        of the foreign device to be deleted.
 */
int bvlc6_encode_delete_foreign_device(
    uint8_t *pdu,
    uint16_t pdu_size,
    uint32_t vmac_src,
    const BACNET_IP6_ADDRESS *bip6_address)
{
    int bytes_encoded = 0;
    const uint16_t length = 0x0019;
    uint16_t offset = 0;

    if (pdu && (pdu_size >= length) && (vmac_src <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(
            pdu, pdu_size, BVLC6_DELETE_FOREIGN_DEVICE, length);
        if (bytes_encoded == 4) {
            offset = 4;
            encode_unsigned24(&pdu[offset], vmac_src);
            offset += 3;
            if (bip6_address) {
                bvlc6_encode_address(
                    &pdu[offset], pdu_size - offset, bip6_address);
                bytes_encoded = (int)length;
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
 * @param fdt_entry - The FDT entry is the B/IPv6 address of the
 *  foreign device to be deleted.
 *
 * @return number of bytes decoded
 */
int bvlc6_decode_delete_foreign_device(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac_src,
    BACNET_IP6_ADDRESS *bip6_address)
{
    int bytes_consumed = 0;
    /* BVLL length less the header length */
    const uint16_t length = 0x0019 - (1 + 1 + 2);
    uint16_t offset = 0;

    if (pdu && (pdu_len >= length)) {
        if (vmac_src) {
            decode_unsigned24(&pdu[offset], vmac_src);
            bytes_consumed = 3;
        }
        offset += 3;
        if (bip6_address) {
            bvlc6_decode_address(&pdu[offset], pdu_len - offset, bip6_address);
        }
        bytes_consumed = (int)length;
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
    uint8_t *pdu, uint16_t pdu_size, const uint8_t *sbuf, uint16_t sbuf_len)
{
    int bytes_encoded = 0;
    uint16_t length = 4;
    uint16_t i = 0;

    length += sbuf_len;
    if (pdu && (pdu_size >= length)) {
        bytes_encoded =
            bvlc6_encode_header(pdu, pdu_size, BVLC6_SECURE_BVLL, length);
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *sbuf,
    uint16_t sbuf_size,
    uint16_t *sbuf_len)
{
    int bytes_consumed = 0;
    uint16_t i = 0;

    if (pdu) {
        if (sbuf_len) {
            *sbuf_len = pdu_len;
        }
        if (pdu_len && sbuf && sbuf_size) {
            for (i = 0; i < pdu_len; i++) {
                if (i < sbuf_size) {
                    sbuf[i] = pdu[i];
                }
            }
        }
        bytes_consumed = (int)pdu_len;
    }

    return bytes_consumed;
}

/** Encode the BVLC Distribute-Broadcast-To-Network message
 *
 * This message provides a mechanism whereby a foreign device
 * shall cause a BBMD to distribute a Forwarded-NPDU
 * BVLC to the local multicast domain, to all BBMD's configured
 * in the BBMD's BDT, and to all foreign devices in the
 * BBMD's FDT.
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
    uint8_t *pdu,
    uint16_t pdu_size,
    uint32_t vmac,
    const uint8_t *npdu,
    uint16_t npdu_len)
{
    int bytes_encoded = 0;
    uint16_t length = 1 + 1 + 2 + 3;
    uint16_t i = 0;

    length += npdu_len;
    if (pdu && (pdu_size >= length) && (vmac <= 0xFFFFFF)) {
        bytes_encoded = bvlc6_encode_header(
            pdu, pdu_size, BVLC6_DISTRIBUTE_BROADCAST_TO_NETWORK, length);
        if (bytes_encoded == 4) {
            encode_unsigned24(&pdu[4], vmac);
            if (npdu && (npdu_len > 0)) {
                for (i = 0; i < npdu_len; i++) {
                    pdu[7 + i] = npdu[i];
                }
            }
            bytes_encoded = (int)length;
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
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint32_t *vmac,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len)
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
                npdu[i] = pdu[3 + i];
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
 * @brief Encode a BBMD Address for Network Port object
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer size
 * @param bbmd_address - HostNPort type FD BBMD Address
 * @return length of the APDU buffer
 */
int bvlc6_foreign_device_bbmd_host_address_encode(
    uint8_t *apdu,
    uint16_t apdu_size,
    const BACNET_HOST_N_PORT *const bbmd_address)
{
#if (__STDC__) && (__STDC_VERSION__ >= 199901L)
    BACNET_HOST_N_PORT address = { .host_ip_address = false,
                                   .host_name = false };
#else
    BACNET_HOST_N_PORT address = { 0 };
#endif
    int apdu_len = 0;

    if (bbmd_address) {
        address = *bbmd_address;
    }

    apdu_len = host_n_port_encode(NULL, &address);
    if (apdu_len <= apdu_size) {
        apdu_len = host_n_port_encode(apdu, &address);
    }

    return apdu_len;
}

/**
 * @brief Encode one Broadcast-Distribution-Table entry for Network Port object
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
 *        broadcast-mask [1] OCTET STRING -- shall be present if BACnet/IP, and
 * absent for BACnet/IPv6
 *    }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param bdt_head - one BACnetBDTEntry
 * @return length of the APDU buffer
 */
int bvlc6_broadcast_distribution_table_entry_encode(
    uint8_t *apdu,
    const BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry)
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
            &octet_string, &bdt_entry->bip6_address.address[0],
            IP6_ADDRESS_MAX);
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
        len = encode_context_unsigned(apdu, 1, bdt_entry->bip6_address.port);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* bbmd-address [0] BACnetHostNPort - closing */
        len = encode_closing_tag(apdu, 0);
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
int bvlc6_broadcast_distribution_table_list_encode(
    uint8_t *apdu, BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry;

    bdt_entry = bdt_head;
    while (bdt_entry) {
        if (bdt_entry->valid) {
            len = bvlc6_broadcast_distribution_table_entry_encode(
                apdu, bdt_entry);
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
int bvlc6_broadcast_distribution_table_encode(
    uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head)
{
    int len = 0;

    len = bvlc6_broadcast_distribution_table_list_encode(NULL, bdt_head);
    if (len <= apdu_size) {
        len = bvlc6_broadcast_distribution_table_list_encode(apdu, bdt_head);
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief Encode the Foreign_Device-Table for Network Port object
 *
 *    BACnetFDTEntry ::= SEQUENCE {
 *        bacnetip-address [0] OCTET STRING, -- the 6-octet B/IP or 18-octet
 * B/IPv6 address of the registrant time-to-live [1] Unsigned16, -- time to live
 * in seconds remaining-time-to-live [2] Unsigned16 -- remaining time in seconds
 *    }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param fdt_head - head of the BDT linked list
 * @return length of the APDU buffer
 */
int bvlc6_foreign_device_table_entry_encode(
    uint8_t *apdu, const BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OCTET_STRING octet_string = { 0 };

    if (fdt_entry) {
        /* bacnetip-address [0] OCTET STRING */
        len = bvlc6_encode_address(
            octetstring_value(&octet_string),
            octetstring_capacity(&octet_string), &fdt_entry->bip6_address);
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
int bvlc6_foreign_device_table_list_encode(
    uint8_t *apdu, BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry;

    fdt_entry = fdt_head;
    while (fdt_entry) {
        if (fdt_entry->valid) {
            len = bvlc6_foreign_device_table_entry_encode(apdu, fdt_entry);
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
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer size
 * @param fdt_head - head of the BDT linked list
 * @return length of the APDU buffer
 */
int bvlc6_foreign_device_table_encode(
    uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head)
{
    int len = 0;

    len = bvlc6_foreign_device_table_list_encode(NULL, fdt_head);
    if (len <= apdu_size) {
        len = bvlc6_foreign_device_table_list_encode(apdu, fdt_head);
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}
