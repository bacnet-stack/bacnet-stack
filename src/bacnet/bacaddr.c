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
#include "bacnet/bactext.h"
#include "bacnet/bacaddr.h"

/**
 * @brief Copy a #BACNET_ADDRESS value to another, or initialize if src=NULL
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
    } else if (dest) {
        dest->mac_len = 0;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->mac[i] = 0;
        }
        dest->net = 0;
        dest->len = 0;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
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
 * @brief Compare two BACnetAddress strictly from encoding based
 *  on network number.
 *  BACnetAddress ::= SEQUENCE {
 *      network-number Unsigned16,-- A value of 0 indicates the local network
 *      mac-address OctetString -- A string of length 0 indicates a broadcast
 *  }
 * @param dest - BACnetAddress to be compared
 * @param src -  BACnetAddress to be compared
 * @return true if the same values
 */
bool bacnet_address_net_same(
    const BACNET_ADDRESS *dest, const BACNET_ADDRESS *src)
{
    uint8_t i = 0; /* loop counter */

    if (!dest || !src) {
        return false;
    }
    if (dest->net != src->net) {
        return false;
    }
    if (dest->net == 0) {
        /* local address stored in MAC */
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
    } else {
        /* remote address stored in ADR */
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
 * @brief Set the #BACNET_ADDRESS of the next-hop router to the MAC of the dest
 * @param dest - #BACNET_ADDRESS to be configured
 * @param router -  #BACNET_ADDRESS MAC to be copied from
 */
void bacnet_address_router_set(
    BACNET_ADDRESS *dest, const BACNET_ADDRESS *router)
{
    uint8_t i = 0;
    uint8_t mac_len = 0;

    if (dest && router) {
        mac_len = router->mac_len;
        if (mac_len > MAX_MAC_LEN) {
            mac_len = MAX_MAC_LEN;
        }
        dest->mac_len = mac_len;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            if (i < mac_len) {
                dest->mac[i] = router->mac[i];
            } else {
                dest->mac[i] = 0;
            }
        }
    }
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
 * @details Address formats:
 *  192.168.1.42:47808 - for BACnet/IP
 *  ff:aa:ff:bb:ff:cc - for Ethernet
 *  fa or 127 - for ARCNET or MS/TP
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
 * @brief Parse an ASCII string for a bacnet-address
 * @details Address formats: mac net addr
 *  192.168.1.42:47808 - for BACnet/IP
 *  ff:aa:ff:bb:ff:cc - for Ethernet
 *  fa - for ARCNET or MS/TP
 *  65535 - network number, 0 if mac but no addr
 * @param src [out] BACNET_MAC_ADDRESS structure to store the results
 * @param arg [in] null terminated ASCII string to parse
 * @return true if the address was parsed
 */
bool bacnet_address_from_ascii(BACNET_ADDRESS *src, const char *arg)
{
    bool status = false;
    int count = 0;
    unsigned snet = 0, i;
    char mac_string[80] = { "" }, sadr_string[80] = { "" };
    BACNET_MAC_ADDRESS mac = { 0 };

    if (!(src && arg)) {
        return false;
    }
    count = sscanf(
        arg, "{%79[^,],%u,%79[^}]}", &mac_string[0], &snet, &sadr_string[0]);
    if (count > 0) {
        if (bacnet_address_mac_from_ascii(&mac, mac_string)) {
            if (src) {
                src->mac_len = mac.len;
                for (i = 0; i < MAX_MAC_LEN; i++) {
                    src->mac[i] = mac.adr[i];
                }
            }
        }
        if (src) {
            src->net = (uint16_t)snet;
        }
        if (snet) {
            if (bacnet_address_mac_from_ascii(&mac, sadr_string)) {
                if (src) {
                    src->len = mac.len;
                    for (i = 0; i < MAX_MAC_LEN; i++) {
                        src->adr[i] = mac.adr[i];
                    }
                }
            }
        } else {
            if (src) {
                src->len = 0;
                for (i = 0; i < MAX_MAC_LEN; i++) {
                    src->adr[i] = 0;
                }
            }
        }
        status = true;
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
    BACNET_UNSIGNED_INTEGER snet = 0;
    uint32_t mac_addr_len = 0;
    uint8_t mac_addr[MAX_MAC_LEN] = { 0 };

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* network number */
    len = bacnet_unsigned_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &snet);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (snet <= UINT16_MAX) {
        /* bounds checking - passed! */
        if (value) {
            value->net = (uint16_t)snet;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    /* mac address as an octet-string */
    len = bacnet_octet_string_buffer_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &mac_addr[0], sizeof(mac_addr),
        &mac_addr_len);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (mac_addr_len > MAX_MAC_LEN) {
        return BACNET_STATUS_ERROR;
    }
    if (snet) {
        if (value) {
            if (mac_addr_len > sizeof(value->adr)) {
                return BACNET_STATUS_ERROR;
            }
            /* bounds checking - passed! */
            value->len = (uint8_t)mac_addr_len;
            /* copy address */
            for (i = 0; i < value->len; i++) {
                value->adr[i] = mac_addr[i];
            }
            /* zero the router address */
            value->mac_len = 0;
            for (i = 0; i < MAX_MAC_LEN; i++) {
                value->mac[i] = 0;
            }
        }
    } else {
        if (value) {
            if (mac_addr_len > sizeof(value->mac)) {
                return BACNET_STATUS_ERROR;
            }
            /* bounds checking - passed! */
            value->mac_len = (uint8_t)mac_addr_len;
            /* copy address */
            for (i = 0; i < value->mac_len; i++) {
                value->mac[i] = mac_addr[i];
            }
            /* zero the device behind a router address */
            value->len = 0;
            for (i = 0; i < MAX_MAC_LEN; i++) {
                value->adr[i] = 0;
            }
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

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
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
    int apdu_len = 0, len, adr_len, mac_len;

    if (destination) {
        /* network number */
        len = encode_application_unsigned(apdu, destination->net);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* encode mac address as an octet-string */
        if (destination->len > 0) {
            adr_len = min(destination->len, sizeof(destination->adr));
            len = encode_application_octet_string_buffer(
                apdu, destination->adr, adr_len);
        } else {
            mac_len = min(destination->mac_len, sizeof(destination->mac));
            len = encode_application_octet_string_buffer(
                apdu, destination->mac, mac_len);
        }
        apdu_len += len;
    }

    return apdu_len;
}

#if defined(BACNET_STACK_DEPRECATED_DISABLE)
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
#endif

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

#if defined(BACNET_STACK_DEPRECATED_DISABLE)
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
#endif

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

    if (!value) {
        return 0;
    }
    /* virtual-mac-address [0] OctetString */
    len = encode_context_octet_string_buffer(
        apdu, 0, value->virtual_mac_address.adr,
        value->virtual_mac_address.len);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* native-mac-address */
    len = encode_context_octet_string_buffer(
        apdu, 1, value->native_mac_address, value->native_mac_address_len);
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
    size_t i = 0;
    BACNET_VMAC_ENTRY entry = { 0 };
    uint32_t mac_addr_len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* virtual-mac-address [0] OctetString */
    len = bacnet_octet_string_buffer_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, entry.virtual_mac_address.adr,
        sizeof(entry.virtual_mac_address.adr), &mac_addr_len);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        if (mac_addr_len > sizeof(value->virtual_mac_address.adr)) {
            return BACNET_STATUS_ERROR;
        }
        /* bounds checking - passed! */
        value->virtual_mac_address.len = mac_addr_len;
        /* copy address */
        for (i = 0; i < sizeof(value->virtual_mac_address.adr); i++) {
            if (i < mac_addr_len) {
                value->virtual_mac_address.adr[i] =
                    entry.virtual_mac_address.adr[i];
            } else {
                value->virtual_mac_address.adr[i] = 0;
            }
        }
    }
    apdu_len += len;
    /* native-mac-address [1] OctetString */
    len = bacnet_octet_string_buffer_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, entry.native_mac_address,
        sizeof(entry.native_mac_address), &mac_addr_len);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        if (mac_addr_len > sizeof(value->native_mac_address)) {
            return BACNET_STATUS_ERROR;
        }
        /* bounds checking - passed! */
        value->native_mac_address_len = mac_addr_len;
        /* copy address */
        for (i = 0; i < sizeof(value->native_mac_address); i++) {
            if (i < mac_addr_len) {
                value->native_mac_address[i] = entry.native_mac_address[i];
            } else {
                value->native_mac_address[i] = 0;
            }
        }
    }
    apdu_len += len;

    return apdu_len;
}

/** Set a BACnet VMAC Address from a Device ID
 *
 * @param addr - BACnet address that be set
 * @param device_id - 22-bit device ID
 *
 * @return true if the address is set
 */
bool bacnet_vmac_address_set(BACNET_ADDRESS *addr, uint32_t device_id)
{
    bool status = false;
    size_t i;

    if (addr) {
        encode_unsigned24(&addr->mac[0], device_id);
        addr->mac_len = 3;
        addr->net = 0;
        addr->len = 0;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            addr->adr[i] = 0;
        }
        status = true;
    }

    return status;
}

/**
 * @brief Encode a given BACnetAddressBinding
 * @details
 *  BACnetAddressBinding ::= SEQUENCE {
 *      device-identifier   BACnetObjectIdentifier,
 *      device-address      BACnetAddress
 * @param  apdu - APDU buffer for storing the encoded data, or NULL for length
 * @param  value - BACnetAddressBinding value
 * @return  number of bytes in the APDU
 */
int bacnet_address_binding_type_encode(
    uint8_t *apdu, const BACNET_ADDRESS_BINDING *value)
{
    int apdu_len = 0, len = 0;

    if (!value) {
        return 0;
    }
    len = encode_application_object_id(
        apdu, OBJECT_DEVICE, value->device_identifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_bacnet_address(apdu, &value->device_address);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode a given BACnetAddressBinding
 * @details
 *  BACnetAddressBinding ::= SEQUENCE {
 *      device-identifier   BACnetObjectIdentifier,
 *      device-address      BACnetAddress
 * @param  apdu - APDU buffer for storing the encoded data, or NULL for length
 * @param apdu_size - size of the APDU buffer
 * @param  value - BACnetAddressBinding value
 * @return  number of bytes in the APDU, or 0 if unable to fit.
 */
int bacnet_address_binding_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_ADDRESS_BINDING *value)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = bacnet_address_binding_type_encode(NULL, value);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = bacnet_address_binding_type_encode(apdu, value);
    }

    return apdu_len;
}

/**
 * @brief Decode a given BACnetAddressBinding
 * @details
 *  BACnetAddressBinding ::= SEQUENCE {
 *      device-identifier   BACnetObjectIdentifier,
 *      device-address      BACnetAddress
 * @param  apdu - APDU buffer for decoding
 * @param  apdu_size - Count of valid bytes in the buffer
 * @param  value - BACnetAddressBinding value to store the decoded data
 * @return  number of bytes decoded or BACNET_STATUS_ERROR on error
 */
int bacnet_address_binding_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_ADDRESS_BINDING *value)
{
    int apdu_len = 0, len = 0;
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_ADDRESS *address = NULL;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_object_id_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &object_type, &object_instance);
    if (len > 0) {
        apdu_len += len;
        if (object_type != OBJECT_DEVICE) {
            return BACNET_STATUS_ERROR;
        }
        if (value) {
            value->device_identifier = object_instance;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        address = &value->device_address;
    }
    len = bacnet_address_decode(&apdu[apdu_len], apdu_size - apdu_len, address);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare two BACnetAddressBinding values
 * @param value1 [in] The first BACnetAddressBinding value
 * @param value2 [in] The second BACnetAddressBinding value
 * @return True if the values are the same, else False
 */
bool bacnet_address_binding_same(
    const BACNET_ADDRESS_BINDING *value1, const BACNET_ADDRESS_BINDING *value2)
{
    if (!value1) {
        return false;
    }
    if (!value2) {
        return false;
    }
    if (value1->device_identifier != value2->device_identifier) {
        return false;
    }
    return bacnet_address_same(
        &value1->device_address, &value2->device_address);
}

/**
 * @brief Copy a BACnetAddressBinding to another
 * @param value1 [in] The first BACnetAddressBinding value
 * @param value2 [in] The second BACnetAddressBinding value
 * @return true if the value was copied, else false
 */
bool bacnet_address_binding_copy(
    BACNET_ADDRESS_BINDING *dest, const BACNET_ADDRESS_BINDING *src)
{
    if (!(dest && src)) {
        return false;
    }
    dest->device_identifier = src->device_identifier;
    bacnet_address_copy(&dest->device_address, &src->device_address);

    return true;
}

/**
 * @brief Initialize a BACnetAddressBinding from device-id and address
 * @param dest [out] The BACnetAddressBinding value
 * @param device_identifier [in] The device-identifier value
 * @param address [in] #BACNET_ADDRESS value
 * @return true if the values were copied, else false
 */
bool bacnet_address_binding_init(
    BACNET_ADDRESS_BINDING *dest,
    uint32_t device_id,
    const BACNET_ADDRESS *address)
{
    if (!dest) {
        return false;
    }
    dest->device_identifier = device_id;
    bacnet_address_copy(&dest->device_address, address);

    return true;
}

/**
 * @brief Produce a string from a BACnetAddressBinding structure
 * @param value [in] The BACnetAddressBinding value
 * @param str [out] The string to produce, NULL to get length only
 * @param str_size [in] The size of the string buffer
 * @return length of the produced string
 * @details Output format:
 *      {(device, 1234),1234,X'c0:a8:00:0f'}
 */
int bacnet_address_binding_to_ascii(
    const BACNET_ADDRESS_BINDING *value, char *str, size_t str_len)
{
    int offset = 0;
    int i;

    if (!value) {
        return 0;
    }
    offset = bacnet_snprintf(str, str_len, offset, "{");
    /* device-identifier */
    offset = bacnet_snprintf(str, str_len, offset, "(");
    offset = bacnet_snprintf(
        str, str_len, offset, "%s, ", bactext_object_type_name(OBJECT_DEVICE));
    offset = bacnet_snprintf(
        str, str_len, offset, "%lu),", (unsigned long)value->device_identifier);
    /* snet */
    offset = bacnet_snprintf(
        str, str_len, offset, "%lu,", (unsigned long)value->device_address.net);
    /* octetstring */
    offset = bacnet_snprintf(str, str_len, offset, "X'");
    if (value->device_address.net) {
        /* adr */
        for (i = 0; i < value->device_address.len; i++) {
            offset = bacnet_snprintf(
                str, str_len, offset, "%02X",
                (unsigned)value->device_address.adr[i]);
        }
    } else {
        /* mac */
        for (i = 0; i < value->device_address.mac_len; i++) {
            offset = bacnet_snprintf(
                str, str_len, offset, "%02X",
                (unsigned)value->device_address.mac[i]);
        }
    }
    offset = bacnet_snprintf(str, str_len, offset, "'");
    offset = bacnet_snprintf(str, str_len, offset, "}");

    return offset;
}

/**
 * @brief Parse a string into a BACnetAddressBinding structure
 * @param value [out] The BACnetAddressBinding value
 * @param argv [in] The string to parse
 * @return true on success, else false
 * @details format:
 *      {(device, 1234),1234,X'c0:a8:00:0f'}
 */
bool bacnet_address_binding_from_ascii(
    BACNET_ADDRESS_BINDING *value, const char *arg)
{
    int count = 0, i = 0;
    unsigned snet = 0;
    char mac_string[80] = { "" }, obj_string[80] = { "" };
    BACNET_MAC_ADDRESS mac = { 0 };
    uint32_t object_type = 0;
    unsigned long object_instance = 0;

    if (!(value && arg)) {
        return false;
    }
    count = sscanf(
        arg, "{(%79[^,], %lu),%u,X'%79[^']'}", obj_string, &object_instance,
        &snet, mac_string);
    if (count == 4) {
        if (!bactext_object_type_strtol(obj_string, &object_type)) {
            return false;
        }
        if (object_type != OBJECT_DEVICE) {
            return false;
        }
        if (!bacnet_address_mac_from_ascii(&mac, mac_string)) {
            return false;
        }
        if (value) {
            value->device_identifier = object_instance;
            value->device_address.net = snet;
            if (snet) {
                value->device_address.len = mac.len;
                for (i = 0; i < MAX_MAC_LEN; i++) {
                    value->device_address.adr[i] = mac.adr[i];
                }
            } else {
                value->device_address.mac_len = mac.len;
                for (i = 0; i < MAX_MAC_LEN; i++) {
                    value->device_address.mac[i] = mac.adr[i];
                }
            }
        }
        return true;
    }

    return false;
}
