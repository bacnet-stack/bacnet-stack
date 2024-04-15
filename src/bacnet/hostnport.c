/**
 * @file
 * @brief BACnetHostNPort complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
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
 * @brief Decode the BACnetHostNPort complex data
 *
 *  BACnetHostNPort ::= SEQUENCE {
 *      host [0] BACnetHostAddress,
 *          BACnetHostAddress ::= CHOICE {
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
int host_n_port_decode(uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_ERROR_CODE *error_code,
    BACNET_HOST_N_PORT *address)
{
    int apdu_len = 0, len = 0;
    BACNET_OCTET_STRING *octet_string = NULL;
    BACNET_CHARACTER_STRING *char_string = NULL;
    BACNET_TAG tag = { 0 };
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
        len = bacnet_octet_string_decode(&apdu[apdu_len], apdu_size - apdu_len,
            tag.len_value_type, octet_string);
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
        len = bacnet_character_string_decode(&apdu[apdu_len],
            apdu_size - apdu_len, tag.len_value_type, char_string);
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

    count = sscanf(argv, "%3u.%3u.%3u.%3u:%5u", &a[0], &a[1], &a[2],
        &a[3], &p);
    if ((count == 4) || (count == 5)) {
        uint8_t address[4];
        value->host_ip_address = true;
        value->host_name = false;
        address[0] = (uint8_t)a[0];
        address[1] = (uint8_t)a[1];
        address[2] = (uint8_t)a[2];
        address[3] = (uint8_t)a[3];
        octetstring_init(
            &value->host.ip_address, address, 4);
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
