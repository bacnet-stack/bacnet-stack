/**
 * @file
 * @brief BACnet Address structure utilities
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacaddr.h"

/**
 * @brief Copy a #BACNET_ADDRESS value to another
 * @param dest - #BACNET_ADDRESS to be copied into
 * @param src -  #BACNET_ADDRESS to be copied from
 */
void bacnet_address_copy(BACNET_ADDRESS *dest, const BACNET_ADDRESS *src)
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
bool bacnet_address_same(const BACNET_ADDRESS *dest, const BACNET_ADDRESS *src)
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
bool bacnet_address_init(
    BACNET_ADDRESS *dest,
    const BACNET_MAC_ADDRESS *mac,
    uint16_t dnet,
    const BACNET_MAC_ADDRESS *adr)
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
        dest->mac_len = 0;
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
bool bacnet_address_mac_same(
    const BACNET_MAC_ADDRESS *dest, const BACNET_MAC_ADDRESS *src)
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
void bacnet_address_mac_init(
    BACNET_MAC_ADDRESS *mac, const uint8_t *adr, uint8_t len)
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
        c = sscanf(
            arg, "%2x:%2x:%2x:%2x:%2x:%2x", &a[0], &a[1], &a[2], &a[3], &a[4],
            &a[5]);

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
    const uint8_t *apdu, uint32_t apdu_size, BACNET_ADDRESS *value)
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
int bacnet_address_context_decode(
    const uint8_t *apdu,
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
int encode_bacnet_address(uint8_t *apdu, const BACNET_ADDRESS *destination)
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
int decode_bacnet_address(const uint8_t *apdu, BACNET_ADDRESS *value)
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
    uint8_t *apdu, uint8_t tag_number, const BACNET_ADDRESS *destination)
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
    const uint8_t *apdu, uint8_t tag_number, BACNET_ADDRESS *value)
{
    return bacnet_address_context_decode(apdu, MAX_APDU, tag_number, value);
}

/**
 * @brief Encode a BACnetVMACEntry value
 *
 * BACnetVMACEntry ::= SEQUENCE {
 *   virtual-mac-address[0]OctetString, -- maximum size 6 octets
 *   native-mac-address[1]OctetString
 *
 * @param apdu - buffer of data to be encoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - decoded value, if decoded (if not NULL)
 *
 * @return the number of apdu bytes consumed, or #BACNET_STATUS_ERROR (-1)
 */
int bacnet_vmac_entry_data_encode(uint8_t *apdu, const BACNET_VMAC_ENTRY *value)
{
    int apdu_len = 0, len;
    BACNET_OCTET_STRING address = { 0 };

    if (!value) {
        return 0;
    }
    /* virtual-mac-address [0] OctetString */
    octetstring_init(
        &address, value->virtual_mac_address.adr,
        value->virtual_mac_address.len);
    len = encode_context_octet_string(apdu, 0, &address);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* native-mac-address */
    octetstring_init(
        &address, value->native_mac_address, value->native_mac_address_len);
    len = encode_context_octet_string(apdu, 1, &address);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode a BACnetVMACEntry value
 * @param apdu - buffer of data to be encoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - value to encode
 *
 * @return the number of apdu bytes encoded, or 0 if not encoded
 */
int bacnet_vmac_entry_encode(
    uint8_t *apdu, uint32_t apdu_size, const BACNET_VMAC_ENTRY *value)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = bacnet_vmac_entry_data_encode(NULL, value);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = bacnet_vmac_entry_data_encode(apdu, value);
    }

    return apdu_len;
}

/**
 * @brief Decodes a BACnetVMACEntry value from a buffer
 *  From clause 21. FORMAL DESCRIPTION OF APPLICATION PROTOCOL DATA UNITS
 *
 * BACnetVMACEntry ::= SEQUENCE {
 *   virtual-mac-address [0] OctetString, -- maximum size 6 octets
 *   native-mac-address [1] OctetString
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - decoded value, if decoded (if not NULL)
 *
 * @return the number of apdu bytes consumed, or #BACNET_STATUS_ERROR (-1)
 */
int bacnet_vmac_entry_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_VMAC_ENTRY *value)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t i = 0;
    BACNET_OCTET_STRING mac_addr = { 0 };

    /* virtual-mac-address [0] OctetString */
    len = bacnet_octet_string_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &mac_addr);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        if (mac_addr.length > sizeof(value->virtual_mac_address.adr)) {
            return BACNET_STATUS_ERROR;
        }
        /* bounds checking - passed! */
        value->virtual_mac_address.len = mac_addr.length;
        /* copy address */
        for (i = 0; i < mac_addr.length; i++) {
            value->virtual_mac_address.adr[i] = mac_addr.value[i];
        }
    }
    apdu_len += len;
    /* native-mac-address [1] OctetString */
    len = bacnet_octet_string_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &mac_addr);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        if (mac_addr.length > sizeof(value->native_mac_address)) {
            return BACNET_STATUS_ERROR;
        }
        /* bounds checking - passed! */
        value->native_mac_address_len = mac_addr.length;
        /* copy address */
        for (i = 0; i < mac_addr.length; i++) {
            value->native_mac_address[i] = mac_addr.value[i];
        }
    }
    apdu_len += len;

    return apdu_len;
}
