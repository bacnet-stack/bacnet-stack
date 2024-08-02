/**
 * @file
 * @brief BACnetHostNPort complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "bacnet/hostnport.h"
#include "bacnet/bacdcode.h"

/**
 * @brief Encode the BACnetHostAddress CHOICE
 * @param apdu - the APDU buffer
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int host_n_port_address_encode(uint8_t *apdu, BACNET_HOST_N_PORT *address)
{
    int len = 0;

    if (address) {
        if (address->host_ip_address) {
            /* CHOICE - ip-address [1] OCTET STRING */
            len =
                encode_context_octet_string(apdu, 1, &address->host.ip_address);
        } else if (address->host_name) {
            /* CHOICE - name [2] CharacterString */
            len = encode_context_character_string(apdu, 2, &address->host.name);
        } else {
            /* CHOICE - none [0] NULL */
            len = encode_context_null(apdu, 0);
        }
    }

    return len;
}

/**
 * @brief Encode a BACnetHostNPort complex data type
 *
 *  BACnetHostNPort ::= SEQUENCE {
 *      host [0] BACnetHostAddress,
 *          BACnetHostAddress ::= CHOICE {
 *              none [0] NULL,
 *              ip-address [1] OCTET STRING,
 *              -- 4 octets for B/IP or 16 octets for B/IPv6
 *              name [2] CharacterString
 *              -- Internet host name (see RFC 1123)
 *          }
 *      port [1] Unsigned16
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int host_n_port_encode(uint8_t *apdu, BACNET_HOST_N_PORT *address)
{
    int len = 0;
    int apdu_len = 0;

    if (address) {
        /*  host [0] BACnetHostAddress - opening */
        len = encode_opening_tag(apdu, 0);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* BACnetHostAddress ::= CHOICE */
        len = host_n_port_address_encode(apdu, address);
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
        len = encode_context_unsigned(apdu, 1, address->port);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode a BACnetHostNPort complex data type
 * @param apdu - the APDU buffer
 * @param tag_number - context tag number to be encoded
 * @param address - IP address and port number
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int host_n_port_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_HOST_N_PORT *address)
{
    int len = 0;
    int apdu_len = 0;

    if (address) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = host_n_port_encode(apdu, address);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief  Decode the BACnetHostAddress
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer length
 * @param error_code - error or reject or abort when error occurs
 * @param address - IP address and port number
 * @return length of the APDU buffer decoded, or BACNET_STATUS_REJECT
 */
int host_n_port_address_decode(
    uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_ERROR_CODE *error_code,
    BACNET_HOST_N_PORT *address)
{
    int apdu_len = 0, len = 0;
    BACNET_OCTET_STRING *octet_string = NULL;
    BACNET_CHARACTER_STRING *char_string = NULL;
    BACNET_TAG tag = { 0 };

    /* default reject code */
    if (error_code) {
        *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len <= 0) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    if (tag.context && (tag.number == 0)) {
        /* CHOICE - none [0] NULL */
        if (address) {
            address->host_ip_address = false;
            address->host_name = false;
        }
    } else if (tag.context && (tag.number == 1)) {
        /* CHOICE - ip-address [1] OCTET STRING */
        if (address) {
            address->host_ip_address = true;
            address->host_name = false;
            octet_string = &address->host.ip_address;
        }
        len = bacnet_octet_string_decode(
            &apdu[apdu_len], apdu_size - apdu_len, tag.len_value_type,
            octet_string);
        if (len < 0) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_BUFFER_OVERFLOW;
            }
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
    } else if (tag.context && (tag.number == 2)) {
        if (address) {
            address->host_ip_address = false;
            address->host_name = true;
            char_string = &address->host.name;
        }
        len = bacnet_character_string_decode(
            &apdu[apdu_len], apdu_size - apdu_len, tag.len_value_type,
            char_string);
        if (len == 0) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_BUFFER_OVERFLOW;
            }
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
    } else {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }

    return apdu_len;
}

/**
 * @brief Decode the BACnetHostNPort complex data
 *
 *  BACnetHostNPort ::= SEQUENCE {
 *      host [0] BACnetHostAddress,
 *          BACnetHostAddress ::= CHOICE {
 *              none [0] NULL,
 *              ip-address [1] OCTET STRING,
 *              -- 4 octets for B/IP or 16 octets for B/IPv6
 *              name [2] CharacterString
 *              -- Internet host name (see RFC 1123)
 *          }
 *      port [1] Unsigned16
 *  }
 *
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer length
 * @param error_code - error or reject or abort when error occurs
 * @param ip_address - IP address and port number
 * @return length of the APDU buffer decoded, or ERROR, REJECT, or ABORT
 */
int host_n_port_decode(
    uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_ERROR_CODE *error_code,
    BACNET_HOST_N_PORT *address)
{
    int apdu_len = 0, len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* default reject code */
    if (error_code) {
        *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
    }
    /* host [0] BACnetHostAddress - opening */
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    /* BACnetHostAddress ::= CHOICE */
    len = host_n_port_address_decode(
        &apdu[apdu_len], apdu_size - apdu_len, error_code, address);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_REJECT;
    }
    /*  host [0] BACnetHostAddress - closing */
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    /* port [1] Unsigned16 */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
    if (len > 0) {
        if (unsigned_value <= UINT16_MAX) {
            if (address) {
                address->port = unsigned_value;
            }
        } else {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
            }
            return BACNET_STATUS_REJECT;
        }
    } else {
        if (error_code) {
            if (len == 0) {
                *error_code = ERROR_CODE_REJECT_INVALID_TAG;
            } else {
                *error_code = ERROR_CODE_REJECT_OTHER;
            }
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a context encoded BACnetHostNPort complex data
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer length
 * @param tag_number - context tag number to be decoded
 * @param error_code - error or reject or abort when error occurs
 * @param address - IP address and port number
 * @return length of the APDU buffer decoded, or ERROR, REJECT, or ABORT
 */
int host_n_port_context_decode(
    uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_ERROR_CODE *error_code,
    BACNET_HOST_N_PORT *address)
{
    int len = 0;
    int apdu_len = 0;

    /* default reject code */
    if (error_code) {
        *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
    }
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }

        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    len = host_n_port_decode(
        &apdu[apdu_len], apdu_size - apdu_len, error_code, address);
    if (len > 0) {
        apdu_len += len;
    } else {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_OTHER;
        }
        return BACNET_STATUS_REJECT;
    }
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Copy the BACnetHostNPort complex data from src to dst
 * @param dst - destination structure
 * @param src - source structure
 * @return true if successfully copied
 */
bool host_n_port_copy(BACNET_HOST_N_PORT *dst, BACNET_HOST_N_PORT *src)
{
    bool status = false;

    if (dst && src) {
        dst->host_ip_address = src->host_ip_address;
        dst->host_name = src->host_name;
        if (src->host_ip_address) {
            status =
                octetstring_copy(&dst->host.ip_address, &src->host.ip_address);
        } else if (src->host_name) {
            status = characterstring_copy(&dst->host.name, &src->host.name);
        } else {
            status = true;
        }
        dst->port = src->port;
    }

    return status;
}

/**
 * @brief Compare the BACnetHostNPort complex data of src and dst
 * @param host1 - host 1 structure
 * @param host2 - host 2 structure
 * @return true if successfully copied
 */
bool host_n_port_same(BACNET_HOST_N_PORT *host1, BACNET_HOST_N_PORT *host2)
{
    bool status = false;

    if (host1 && host2) {
        if ((host1->host_ip_address == host2->host_ip_address) &&
            (host1->host_name == host2->host_name)) {
            if (host1->host_ip_address) {
                status = octetstring_value_same(
                    &host1->host.ip_address, &host2->host.ip_address);
            } else if (host1->host_name) {
                status =
                    characterstring_same(&host1->host.name, &host2->host.name);
            } else {
                status = true;
            }
            if (status) {
                if (host1->port != host2->port) {
                    status = false;
                }
            }
        }
    }
    return status;
}

/**
 * @brief Parse value from ASCII string (as entered by user)
 * @param value - struct to populate with data from the ASCII string
 * @param argv - ASCII string, zero terminated
 * @return true on success
 */
bool host_n_port_from_ascii(BACNET_HOST_N_PORT *value, const char *argv)
{
    bool status = false;
    unsigned a[4] = { 0 }, p = 0;
    int count;

    count = sscanf(argv, "%3u.%3u.%3u.%3u:%5u", &a[0], &a[1], &a[2], &a[3], &p);
    if ((count == 4) || (count == 5)) {
        uint8_t address[4];
        value->host_ip_address = true;
        value->host_name = false;
        address[0] = (uint8_t)a[0];
        address[1] = (uint8_t)a[1];
        address[2] = (uint8_t)a[2];
        address[3] = (uint8_t)a[3];
        octetstring_init(&value->host.ip_address, address, 4);
        if (count == 4) {
            value->port = 0xBAC0U;
        } else {
            value->port = (uint16_t)p;
        }
        status = true;
    } else {
        status = false;
    }

    return status;
}

/**
 * @brief Encode the BACnetBDTEntry complex data
 *
 *  BACnetBDTEntry ::= SEQUENCE {
 *      bbmd-address [0] BACnetHostNPort,
 *      broadcast-mask [1] OCTET STRING OPTIONAL
 *      -- shall be present if BACnet/IP, and absent for BACnet/IPv6
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param entry - BACnet BDT entry
 * @return length of the encoded APDU buffer
 */
int bacnet_bdt_entry_encode(uint8_t *apdu, BACNET_BDT_ENTRY *entry)
{
    int len = 0;
    int apdu_len = 0;

    if (entry) {
        /*  bbmd-address [0] BACnetHostNPort - opening */
        len = encode_opening_tag(apdu, 0);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* BACnetHostNPort ::= SEQUENCE */
        len = host_n_port_encode(apdu, &entry->bbmd_address);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /*  bbmd-address [0] BACnetHostNPort - closing */
        len = encode_closing_tag(apdu, 0);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        if (octetstring_length(&entry->broadcast_mask) > 0) {
            /* broadcast-mask [1] OCTET STRING */
            len = encode_context_octet_string(apdu, 1, &entry->broadcast_mask);
            apdu_len += len;
        }
    }

    return apdu_len;
}

/**
 * @brief Encode the BACnetBDTEntry complex data
 * @param apdu - the APDU buffer, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param entry - BACnet BDT entry
 * @return length of the encoded APDU buffer
 */
int bacnet_bdt_entry_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_BDT_ENTRY *entry)
{
    int len = 0;
    int apdu_len = 0;

    if (entry) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = bacnet_bdt_entry_encode(apdu, entry);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

int bacnet_bdt_entry_decode(
    uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_ERROR_CODE *error_code,
    BACNET_BDT_ENTRY *address)
{
    int apdu_len = 0, len = 0;

    /* default reject code */
    if (error_code) {
        *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
    }
    /*  bbmd-address [0] BACnetHostNPort - opening */
    if (!bacnet_is_opening_tag_number(&apdu[apdu_len], apdu_size, 0, &len)) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    /* BACnetHostNPort ::= SEQUENCE */
    len = host_n_port_decode(
        &apdu[apdu_len], apdu_size-apdu_len, error_code, &address->bbmd_address);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_REJECT;
    }
    /*  bbmd-address [0] BACnetHostNPort - closing */
    if (!bacnet_is_closing_tag_number(&apdu[apdu_len], apdu_size-apdu_len, 0, &len)) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    /* broadcast-mask [1] OCTET STRING */
    len = bacnet_octet_string_context_decode(
        &apdu[apdu_len], apdu_size-apdu_len, 1, &address->broadcast_mask);
    if (len > 0) {
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Copy the BACnetBDTEntry complex data
 * @param dst - destination structure
 * @param src - source structure
 * @return true if successfully copied
 */
bool bacnet_bdt_entry_copy(BACNET_BDT_ENTRY *dst, BACNET_BDT_ENTRY *src)
{
    bool status = false;

    if (dst && src) {
        status = host_n_port_copy(&dst->bbmd_address, &src->bbmd_address);
        if (!status) {
            status =
                octetstring_copy(&dst->broadcast_mask, &src->broadcast_mask);
        }
    }

    return status;
}

/**
 * @brief Compare the BACnetBDTEntry complex data
 * @param dst - destination structure
 * @param src - source structure
 * @return true if both are the same
 */
bool bacnet_bdt_entry_same(BACNET_BDT_ENTRY *dst, BACNET_BDT_ENTRY *src)
{
    bool status = false;

    if (dst && src) {
        status = host_n_port_same(&dst->bbmd_address, &src->bbmd_address);
        if (status) {
            status = octetstring_value_same(
                &dst->broadcast_mask, &src->broadcast_mask);
        }
    }

    return status;
}

/**
 * @brief Parse value from ASCII string (as entered by user)
 * @param value - struct to populate with data from the ASCII string
 * @param argv - ASCII string, zero terminated
 * @return true on success
 */
bool bacnet_bdt_entry_from_ascii(BACNET_BDT_ENTRY *value, const char *argv)
{
    bool status = false;
    unsigned a[16] = { 0 }, p = 0, m[4];
    int count;

    if (!argv) {
        return false;
    }
    if (!value) {
        return false;
    }
    count = sscanf(
        argv, "%3u.%3u.%3u.%3u:%5u,%3u.%3u.%3u.%3u", &a[0], &a[1], &a[2], &a[3],
        &p, &m[0], &m[1], &m[2], &m[3]);
    if ((count == 4) || (count == 5) || (count == 8) || (count == 9)) {
        uint8_t address[4];

        value->bbmd_address.host_ip_address = true;
        value->bbmd_address.host_name = false;
        address[0] = (uint8_t)a[0];
        address[1] = (uint8_t)a[1];
        address[2] = (uint8_t)a[2];
        address[3] = (uint8_t)a[3];
        octetstring_init(&value->bbmd_address.host.ip_address, address, 4);
        if ((count == 4) || (count == 8)) {
            value->bbmd_address.port = 0xBAC0U;
        } else if (count == 5) {
            value->bbmd_address.port = (uint16_t)p;
            octetstring_init(&value->broadcast_mask, NULL, 0);
        } else {
            value->bbmd_address.port = (uint16_t)p;
            address[0] = (uint8_t)m[0];
            address[1] = (uint8_t)m[1];
            address[2] = (uint8_t)m[2];
            address[3] = (uint8_t)m[3];
            octetstring_init(&value->broadcast_mask, address, 4);
        }
        status = true;
    }
    if (!status) {
        count = sscanf(
            argv,
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%5u",
            &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &a[8],
            &a[9], &a[10], &a[11], &a[12], &a[13], &a[14], &a[15], &p);
        if ((count == 16) || (count == 17)) {
            uint8_t address[16];

            value->bbmd_address.host_ip_address = true;
            value->bbmd_address.host_name = false;
            address[0] = (uint8_t)a[0];
            address[1] = (uint8_t)a[1];
            address[2] = (uint8_t)a[2];
            address[3] = (uint8_t)a[3];
            address[4] = (uint8_t)a[4];
            address[5] = (uint8_t)a[5];
            address[6] = (uint8_t)a[6];
            address[7] = (uint8_t)a[7];
            address[8] = (uint8_t)a[8];
            address[9] = (uint8_t)a[9];
            address[10] = (uint8_t)a[10];
            address[11] = (uint8_t)a[11];
            address[12] = (uint8_t)a[12];
            address[13] = (uint8_t)a[13];
            address[14] = (uint8_t)a[14];
            address[15] = (uint8_t)a[15];
            octetstring_init(&value->bbmd_address.host.ip_address, address, 16);
            if (count == 16) {
                value->bbmd_address.port = 0xBAC0U;
            } else {
                value->bbmd_address.port = (uint16_t)p;
            }
            status = true;
        }
    }
    if (!status) {
        const char *name, *port;
        int name_len;

        port = strchr(argv, ':');
        if (port) {
            name_len = port - argv;
            port++;
            p = strtol(port, NULL, 10);
        } else {
            name_len = strlen(argv);
            p = 0xBAC0U;
        }
        name = argv;
        if (name && isalnum(name[0])) {
            value->bbmd_address.host_ip_address = false;
            value->bbmd_address.host_name = true;
            characterstring_init(&value->bbmd_address.host.name, 
                CHARACTER_ANSI_X34,
                name, name_len);
        } else {
            value->bbmd_address.host_ip_address = false;
            value->bbmd_address.host_name = false;
        }
        value->bbmd_address.port = p;
        status = true;
    }

    return status;
}

/**
 * @brief Convert the BACnetFDTEntry complex data to ASCII string
 * @param str - destination string
 * @param str_size - size of the destination string
 * @param value - BACnet FDT entry
 * @return length of the ASCII string
 */
int bacnet_bdt_entry_to_ascii(
    char *str, size_t str_size, BACNET_BDT_ENTRY *value)
{
    int len = 0;
    int ip_len = 0;

    if (value->bbmd_address.host_ip_address) {
        ip_len = octetstring_length(&value->bbmd_address.host.ip_address);
        if (ip_len == 4) {
            len = snprintf(
                str, str_size, "%u.%u.%u.%u:%u,%u.%u.%u.%u",
                value->bbmd_address.host.ip_address.value[0],
                value->bbmd_address.host.ip_address.value[1],
                value->bbmd_address.host.ip_address.value[2],
                value->bbmd_address.host.ip_address.value[3],
                value->bbmd_address.port, value->broadcast_mask.value[0],
                value->broadcast_mask.value[1], value->broadcast_mask.value[2],
                value->broadcast_mask.value[3]);
        } else if (ip_len == 16) {
            len = snprintf(
                str, str_size,
                "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
                "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%u",
                value->bbmd_address.host.ip_address.value[0],
                value->bbmd_address.host.ip_address.value[1],
                value->bbmd_address.host.ip_address.value[2],
                value->bbmd_address.host.ip_address.value[3],
                value->bbmd_address.host.ip_address.value[4],
                value->bbmd_address.host.ip_address.value[5],
                value->bbmd_address.host.ip_address.value[6],
                value->bbmd_address.host.ip_address.value[7],
                value->bbmd_address.host.ip_address.value[8],
                value->bbmd_address.host.ip_address.value[9],
                value->bbmd_address.host.ip_address.value[10],
                value->bbmd_address.host.ip_address.value[11],
                value->bbmd_address.host.ip_address.value[12],
                value->bbmd_address.host.ip_address.value[13],
                value->bbmd_address.host.ip_address.value[14],
                value->bbmd_address.host.ip_address.value[15],
                value->bbmd_address.port);
        }
    } else if (value->bbmd_address.host_name) {
        len = snprintf(
            str, str_size, "%*s:%u",
            (int)characterstring_length(&value->bbmd_address.host.name),
            characterstring_value(&value->bbmd_address.host.name),
            value->bbmd_address.port);
    }   

    return len;
}

/**
 * @brief Encode the BACnetFDTEntry complex data
 *
 *  BACnetFDTEntry ::= SEQUENCE {
 *      bacnetip-address [0] OCTET STRING,
 *      -- the 6-octet B/IP or 18-octet B/IPv6 address of the registrant
 *      time-to-live [1] Unsigned16,
 *      -- time to live in seconds at the time of registration
 *      remaining-time-to-live [2] Unsigned16
 *      -- remaining time to live in seconds, incl. grace period
 * }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param entry - BACnet FDT entry
 * @return length of the encoded APDU buffer
 */
int bacnet_fdt_entry_encode(uint8_t *apdu, BACNET_FDT_ENTRY *entry)
{
    int len = 0;
    int apdu_len = 0;

    if (entry) {
        /* bacnetip-address [0] OCTET STRING */
        len = encode_context_octet_string(apdu, 0, &entry->bacnetip_address);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* time-to-live [1] Unsigned16 */
        len = encode_context_unsigned(apdu, 1, entry->time_to_live);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* remaining-time-to-live [2] Unsigned16 */
        len = encode_context_unsigned(apdu, 2, entry->remaining_time_to_live);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the BACnetFDTEntry complex data with context tag
 * @param apdu - the APDU buffer, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param entry - BACnet FDT entry
 * @return length of the encoded APDU buffer
 */
int bacnet_fdt_entry_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_FDT_ENTRY *entry)
{
    int len = 0;
    int apdu_len = 0;

    if (entry) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = bacnet_fdt_entry_encode(apdu, entry);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Decode the BACnetFDTEntry complex data
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer length
 * @param error_code - error or reject or abort when error occurs
 * @param address - BACnet FDT entry
 * @return length of the APDU buffer decoded, or BACNET_STATUS_REJECT
 */
int bacnet_fdt_entry_decode(
    uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_ERROR_CODE *error_code,
    BACNET_FDT_ENTRY *entry)
{
    int apdu_len = 0, len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* default reject code */
    if (error_code) {
        *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
    }
    /* bacnetip-address [0] OCTET STRING */
    len = bacnet_octet_string_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0,
        &entry->bacnetip_address);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_REJECT;
    }
    /* time-to-live [1] Unsigned16 */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
    if ((len > 0) && (unsigned_value <= UINT16_MAX)) {
        entry->time_to_live = unsigned_value;
        apdu_len += len;
    } else {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
        }
        return BACNET_STATUS_REJECT;
    }
    /* remaining-time-to-live [2] Unsigned16 */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2,
         &unsigned_value);
    if ((len > 0) && (unsigned_value <= UINT16_MAX)) {
        entry->remaining_time_to_live = unsigned_value;
        apdu_len += len;
    } else {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
        }
        return BACNET_STATUS_REJECT;
    }

    return apdu_len;
}

/**
 * @brief Decode the BACnetFDTEntry complex data with context tag
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer length
 * @param tag_number - context tag number to be decoded
 * @param error_code - error or reject or abort when error occurs
 * @param address - BACnet FDT entry
 * @return length of the APDU buffer decoded, or BACNET_STATUS_REJECT
 */
int bacnet_fdt_entry_context_decode(
    uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_ERROR_CODE *error_code,
    BACNET_FDT_ENTRY *address)
{
    int len = 0;
    int apdu_len = 0;

    /* default reject code */
    if (error_code) {
        *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
    }
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    len = bacnet_fdt_entry_decode(
        &apdu[apdu_len], apdu_size - apdu_len, error_code, address);
    if (len > 0) {
        apdu_len += len;
    } else {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_OTHER;
        }
        return BACNET_STATUS_REJECT;
    }
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Copy the BACnetFDTEntry complex data
 * @param dst - destination structure
 * @param src - source structure
 * @return true if successfully copied
 */
bool bacnet_fdt_entry_copy(BACNET_FDT_ENTRY *dst, BACNET_FDT_ENTRY *src)
{
    bool status = false;

    if (dst && src) {
        status =
            octetstring_copy(&dst->bacnetip_address, &src->bacnetip_address);
        dst->time_to_live = src->time_to_live;
        dst->remaining_time_to_live = src->remaining_time_to_live;
    }

    return status;
}

/**
 * @brief Compare the BACnetFDTEntry complex data
 * @param dst - destination structure
 * @param src - source structure
 * @return true if both are the same
 */
bool bacnet_fdt_entry_same(BACNET_FDT_ENTRY *dst, BACNET_FDT_ENTRY *src)
{
    bool status = false;

    if (dst && src) {
        status = octetstring_value_same(
            &dst->bacnetip_address, &src->bacnetip_address);
        if (status) {
            if (dst->time_to_live != src->time_to_live) {
                status = false;
            }
        }
        if (status) {
            if (dst->remaining_time_to_live != src->remaining_time_to_live) {
                status = false;
            }
        }
    }

    return status;
}

/**
 * @brief Parse value from ASCII string (as entered by user)
 * @param value - struct to populate with data from the ASCII string
 * @param argv - ASCII string, zero terminated
 * @return true on success
 */
bool bacnet_fdt_entry_from_ascii(BACNET_FDT_ENTRY *value, const char *argv)
{
    bool status = false;
    unsigned a[18] = { 0 }, p = 0, ttl = 0, rttl = 0;
    int count;

    if (!argv) {
        return false;
    }
    if (!value) {
        return false;
    }
    count = sscanf(
        argv, "%3u.%3u.%3u.%3u:%5u,%5u,%5u", &a[0], &a[1], &a[2], &a[3], &p,
        &ttl, &rttl);
    if ((count == 4) || (count == 5) || (count == 6) || (count == 7)) {
        uint8_t address[6];

        address[0] = (uint8_t)a[0];
        address[1] = (uint8_t)a[1];
        address[2] = (uint8_t)a[2];
        address[3] = (uint8_t)a[3];

        if (count == 4) {
            p = 0xBAC0U;
            value->time_to_live = 0;
            value->remaining_time_to_live = 0;
        } else if (count == 5) {
            value->time_to_live = 0;
            value->remaining_time_to_live = 0;
        } else if (count == 6) {
            value->time_to_live = (uint16_t)ttl;
        } else {
            value->time_to_live = (uint16_t)ttl;
            value->remaining_time_to_live = (uint16_t)rttl;
        }
        address[4] = (uint8_t)p >> 8;
        address[5] = (uint8_t)p & 0xFF;
        octetstring_init(&value->bacnetip_address, address, 6);
        status = true;
    } else {
        count = sscanf(
            argv,
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%5u,%5u,%5u",
            &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &a[8],
            &a[9], &a[10], &a[11], &a[12], &a[13], &a[14], &a[15], &p, &ttl, 
            &rttl);
        if ((count == 16) || (count == 17) || (count == 18) || (count == 19)) {
            uint8_t address[18];

            address[0] = (uint8_t)a[0];
            address[1] = (uint8_t)a[1];
            address[2] = (uint8_t)a[2];
            address[3] = (uint8_t)a[3];
            address[4] = (uint8_t)a[4];
            address[5] = (uint8_t)a[5];
            address[6] = (uint8_t)a[6];
            address[7] = (uint8_t)a[7];
            address[8] = (uint8_t)a[8];
            address[9] = (uint8_t)a[9];
            address[10] = (uint8_t)a[10];
            address[11] = (uint8_t)a[11];
            address[12] = (uint8_t)a[12];
            address[13] = (uint8_t)a[13];
            address[14] = (uint8_t)a[14];
            address[15] = (uint8_t)a[15];
            if (count == 16) {
                p = 0xBAC0U;
                value->time_to_live = 0;
                value->remaining_time_to_live = 0;
            } else if (count == 17) {
                value->time_to_live = 0;
                value->remaining_time_to_live = 0;
            } else if (count == 18) {
                value->time_to_live = (uint16_t)ttl;
            } else {
                value->time_to_live = (uint16_t)ttl;
                value->remaining_time_to_live = (uint16_t)rttl;
            }
            address[16] = (uint8_t)p >> 8;
            address[17] = (uint8_t)p & 0xFF;
            octetstring_init(&value->bacnetip_address, address, 18);
            status = true;
        } else {
            status = false;
        }
    }

    return status;
}

/**
 * @brief Convert the BACnetFDTEntry complex data to ASCII string
 * @param str - destination string
 * @param str_size - size of the destination string
 * @param value - BACnet FDT entry
 * @return length of the ASCII string
 */
int bacnet_fdt_entry_to_ascii(
    char *str, size_t str_size, BACNET_FDT_ENTRY *value)
{
    int len = 0;
    int ip_len = 0;

    ip_len = octetstring_length(&value->bacnetip_address);
    if (ip_len == 6) {
        len = snprintf(
            str, str_size, "%u.%u.%u.%u:%u,%u,%u",
            value->bacnetip_address.value[0], value->bacnetip_address.value[1],
            value->bacnetip_address.value[2], value->bacnetip_address.value[3],
            (value->bacnetip_address.value[4] << 8) |
                value->bacnetip_address.value[5],
            value->time_to_live, value->remaining_time_to_live);
    } else if (ip_len == 18) {
        len = snprintf(
            str, str_size,
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%u,%u,%u",
            value->bacnetip_address.value[0], value->bacnetip_address.value[1],
            value->bacnetip_address.value[2], value->bacnetip_address.value[3],
            value->bacnetip_address.value[4], value->bacnetip_address.value[5],
            value->bacnetip_address.value[6], value->bacnetip_address.value[7],
            value->bacnetip_address.value[8], value->bacnetip_address.value[9],
            value->bacnetip_address.value[10],
            value->bacnetip_address.value[11],
            value->bacnetip_address.value[12],
            value->bacnetip_address.value[13],
            value->bacnetip_address.value[14],
            value->bacnetip_address.value[15],
            (value->bacnetip_address.value[16] << 8) |
                value->bacnetip_address.value[17],
            value->time_to_live, value->remaining_time_to_live);
    }

    return len;
}
