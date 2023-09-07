/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg

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
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "bacnet/config.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacint.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacaddr.h"

/** @file bacaddr.c  BACnet Address structure utilities */

/**
 * @brief Copy a #BACNET_ADDRESS value to another
 * @param dest - #BACNET_ADDRESS to be copied into
 * @param src -  #BACNET_ADDRESS to be copied from
 */
void bacnet_address_copy(BACNET_ADDRESS *dest, BACNET_ADDRESS *src)
{
    int i = 0;

    if (dest && src) {
        dest->mac_len = src->mac_len;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->mac[i] = src->mac[i];
        }
        dest->net = src->net;
        dest->len = src->len;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = src->adr[i];
        }
    }
}

/**
 * @brief Compare two #BACNET_ADDRESS values
 * @param dest - #BACNET_ADDRESS to be compared
 * @param src -  #BACNET_ADDRESS to be compared
 * @return true if the same values
 */
bool bacnet_address_same(BACNET_ADDRESS *dest, BACNET_ADDRESS *src)
{
    uint8_t i = 0; /* loop counter */

    if (!dest || !src) {
        return false;
    }
    if (dest == src) {
        return true;
    }
    if (dest->mac_len != src->mac_len) {
        return false;
    }
    for (i = 0; i < MAX_MAC_LEN; i++) {
        if (i < dest->mac_len) {
            if (dest->mac[i] != src->mac[i]) {
                return false;
            }
        }
    }
    if (dest->net != src->net) {
        return false;
    }
    /* if local, ignore remaining fields */
    if (dest->net == 0) {
        return true;
    }
    if (dest->len != src->len) {
        return false;
    }
    for (i = 0; i < MAX_MAC_LEN; i++) {
        if (i < dest->len) {
            if (dest->adr[i] != src->adr[i]) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Configure a #BACNET_ADDRESS from mac, dnet, and adr
 * @param dest - #BACNET_ADDRESS to be configured
 * @param mac - #BACNET_MAC_ADDRESS used in configuration
 * @param dnet - remote network number 0..65535 (0=local, 65535=broadcast)
 * @param adr - #BACNET_MAC_ADDRESS behind the remote network
 * @return true if configured
 */
bool bacnet_address_init(BACNET_ADDRESS *dest,
    BACNET_MAC_ADDRESS *mac,
    uint16_t dnet,
    BACNET_MAC_ADDRESS *adr)
{
    uint8_t i = 0; /* loop counter */

    if (!dest) {
        return false;
    }
    if (adr && adr->len && mac && mac->len) {
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->mac[i] = mac->adr[i];
        }
        dest->mac_len = mac->len;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = adr->adr[i];
        }
        dest->len = adr->len;
        dest->net = dnet;
    } else if (mac && mac->len) {
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->mac[i] = mac->adr[i];
        }
        dest->mac_len = mac->len;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
        dest->len = 0;
        dest->net = dnet;
    } else {
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->mac[i] = 0;
        }
        dest->mac_len = mac->len;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
        dest->len = 0;
        dest->net = dnet;
    }

    return true;
}

/**
 * @brief Compare two #BACNET_MAC_ADDRESS values
 * @param dest - #BACNET_MAC_ADDRESS to be compared
 * @param src -  #BACNET_MAC_ADDRESS to be compared
 * @return true if the same values
 */
bool bacnet_address_mac_same(BACNET_MAC_ADDRESS *dest, BACNET_MAC_ADDRESS *src)
{
    uint8_t i = 0; /* loop counter */

    if (!dest || !src) {
        return false;
    }
    if (dest->len != src->len) {
        return false;
    }
    for (i = 0; i < MAX_MAC_LEN; i++) {
        if (i < dest->len) {
            if (dest->adr[i] != src->adr[i]) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Initialize a BACNET_MAC_ADDRESS
 * @param mac [out] BACNET_MAC_ADDRESS structure
 * @param adr [in] address to initialize, null if empty
 * @param len [in] length of address in bytes
 */
void bacnet_address_mac_init(BACNET_MAC_ADDRESS *mac, uint8_t *adr, uint8_t len)
{
    uint8_t i = 0;

    if (mac) {
        if (adr && (len <= sizeof(mac->adr))) {
            for (i = 0; i < len; i++) {
                mac->adr[i] = adr[i];
            }
            mac->len = len;
        } else {
            mac->len = 0;
        }
    }
}

/**
 * @brief Parse an ASCII string for a bacnet-address
 * @param mac [out] BACNET_MAC_ADDRESS structure to store the results
 * @param arg [in] nul terminated ASCII string to parse
 * @return true if the address was parsed
 */
bool bacnet_address_mac_from_ascii(BACNET_MAC_ADDRESS *mac, const char *arg)
{
    unsigned a[6] = { 0 }, p = 0;
    uint16_t port = 0;
    int c, i;
    bool status = false;

    if (!(mac && arg)) {
        return false;
    }
    c = sscanf(arg, "%3u.%3u.%3u.%3u:%5u", &a[0], &a[1], &a[2], &a[3], &p);
    if ((c == 4) || (c == 5)) {
        mac->adr[0] = a[0];
        mac->adr[1] = a[1];
        mac->adr[2] = a[2];
        mac->adr[3] = a[3];
        if (c == 4) {
            port = 0xBAC0U;
        } else {
            port = (uint16_t)p;
        }
        encode_unsigned16(&mac->adr[4], port);
        mac->len = 6;
        status = true;
    } else {
        c = sscanf(arg, "%2x:%2x:%2x:%2x:%2x:%2x", &a[0], &a[1], &a[2], &a[3],
            &a[4], &a[5]);

        if (c > 0) {
            for (i = 0; i < c; i++) {
                mac->adr[i] = a[i];
            }
            mac->len = c;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Decodes a BACnetAddress value from APDU buffer
 *  From clause 21. FORMAL DESCRIPTION OF APPLICATION PROTOCOL DATA UNITS
 *
 *  BACnetAddress ::= SEQUENCE {
 *      network-number Unsigned16, -- A value of 0 indicates the local network
 *      mac-address OCTET STRING -- A string of length 0 indicates a broadcast
 *  }
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - decoded value, if decoded (if not NULL)
 *
 * @return the number of apdu bytes consumed, or #BACNET_STATUS_ERROR (-1)
 */
int bacnet_address_decode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_ADDRESS *value)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t i = 0;
    BACNET_UNSIGNED_INTEGER decoded_unsigned = 0;
    BACNET_OCTET_STRING mac_addr = { 0 };

    /* network number */
    len = bacnet_unsigned_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &decoded_unsigned);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (decoded_unsigned <= UINT16_MAX) {
        /* bounds checking - passed! */
        if (value) {
            value->net = (uint16_t)decoded_unsigned;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    /* mac address as an octet-string */
    len = bacnet_octet_string_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &mac_addr);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        if (mac_addr.length > sizeof(value->mac)) {
            return BACNET_STATUS_ERROR;
        }
        /* bounds checking - passed! */
        value->mac_len = mac_addr.length;
        /* copy address */
        for (i = 0; i < value->mac_len; i++) {
            value->mac[i] = mac_addr.value[i];
        }
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decodes a context tagged BACnetAddress value from APDU buffer
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer size
 * @param tag_number - context tag number to be encoded
 * @param value - parameter to store the value after decoding
 * @return length of the APDU buffer decoded, or BACNET_STATUS_ERROR
 */
int bacnet_address_context_decode(uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_ADDRESS *value)
{
    int len = 0;
    int apdu_len = 0;

    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    len = bacnet_address_decode(&apdu[apdu_len], apdu_size - apdu_len, value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * Encode a BACnetAddress and returns the number of apdu bytes consumed.
 *
 * @param apdu - buffer to hold encoded data, or NULL for length
 * @param destination  Pointer to the destination address to be encoded.
 *
 * @return number of apdu bytes created
 */
int encode_bacnet_address(uint8_t *apdu, BACNET_ADDRESS *destination)
{
    int apdu_len = 0;
    BACNET_OCTET_STRING mac_addr;

    if (destination) {
        /* network number */
        apdu_len += encode_application_unsigned(apdu, destination->net);
        /* encode mac address as an octet-string */
        if (destination->len != 0) {
            octetstring_init(&mac_addr, destination->adr, destination->len);
        } else {
            octetstring_init(&mac_addr, destination->mac, destination->mac_len);
        }
        if (apdu) {
            apdu += apdu_len;
        }
        apdu_len += encode_application_octet_string(apdu, &mac_addr);
    }
    return apdu_len;
}

/**
 * @brief Decode a BACnetAddress and returns the number of apdu bytes consumed.
 * @param apdu  Receive buffer
 * @param value - parameter to store the value after decoding
 * @return length of the APDU buffer decoded, or BACNET_STATUS_ERROR
 * @deprecated use bacnet_address_decode() instead
 */
int decode_bacnet_address(uint8_t *apdu, BACNET_ADDRESS *value)
{
    return bacnet_address_decode(apdu, MAX_APDU, value);
}

/**
 * @brief Encode a context encoded BACnetAddress
 * @param apdu - buffer to hold encoded data, or NULL for length
 * @param destination  Pointer to the destination address to be encoded.
 * @return number of apdu bytes created
 */
int encode_context_bacnet_address(
    uint8_t *apdu, uint8_t tag_number, BACNET_ADDRESS *destination)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    len += encode_opening_tag(apdu, tag_number);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_address(apdu_offset, destination);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_closing_tag(apdu_offset, tag_number);
    return len;
}

/*
 * @brief Decodes a context tagged BACnetAddress value from APDU buffer
 * @param apdu - the APDU buffer
 * @param tag_number - context tag number to be encoded
 * @param value - parameter to store the value after decoding
 * @return length of the APDU buffer decoded, or BACNET_STATUS_ERROR
 * @deprecated use bacnet_address_context_decode() instead
 */
int decode_context_bacnet_address(
    uint8_t *apdu, uint8_t tag_number, BACNET_ADDRESS *value)
{
    return bacnet_address_context_decode(apdu, MAX_APDU, tag_number, value);
}
