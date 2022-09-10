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
    uint8_t *apdu_offset = NULL;

    if (address) {
        /*  host [0] BACnetHostAddress - opening */
        len = encode_opening_tag(apdu, 0);
        apdu_len += len;
        if (address->host_ip_address) {
            /* CHOICE - ip-address [1] OCTET STRING */
            if (apdu) {
                apdu_offset = &apdu[apdu_len];
            }
            len = encode_context_octet_string(
                apdu_offset, 1, &address->host.ip_address);
            apdu_len += len;
        } else if (address->host_name) {
            /* CHOICE - name [2] CharacterString */
            if (apdu) {
                apdu_offset = &apdu[apdu_len];
            }
            len = encode_context_character_string(
                apdu_offset, 1, &address->host.name);
            apdu_len += len;
        } else {
            /* none */
        }
        /*  host [0] BACnetHostAddress - closing */
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_closing_tag(apdu_offset, 0);
        apdu_len += len;
        /* port [1] Unsigned16 */
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_context_unsigned(apdu_offset, 1, address->port);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode a BACnetHostNPort complex data type
 * @param apdu - the APDU buffer
 * @param tag_number - the APDU buffer size
 * @param address - IP address and port number
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int host_n_port_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_HOST_N_PORT *address)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    if (address) {
        apdu_offset = apdu;
        len = encode_opening_tag(apdu_offset, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = host_n_port_encode(apdu_offset, address);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_closing_tag(apdu_offset, tag_number);
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
 * @param apdu_len - the APDU buffer length
 * @param ip_address - IP address and port number
 * @return length of the APDU buffer decoded, or ERROR, REJECT, or ABORT
 */
int host_n_port_decode(uint8_t *apdu,
    uint16_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_HOST_N_PORT *address)
{
    int len = 0;
    BACNET_OCTET_STRING octet_string = { 0 };
    BACNET_CHARACTER_STRING char_string = { 0 };
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
    len +=
        decode_tag_number_and_value(&apdu[len], &tag_number, &len_value_type);
    if (tag_number == 0) {
        /* CHOICE - none [0] NULL */
        address->host_ip_address = false;
        address->host_name = false;
    } else if (tag_number == 1) {
        /* CHOICE - ip-address [1] OCTET STRING */
        address->host_ip_address = true;
        address->host_name = false;
        len += decode_octet_string(&apdu[len], len_value_type, &octet_string);
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        (void)octetstring_copy(&address->host.ip_address, &octet_string);
    } else if (tag_number == 2) {
        address->host_ip_address = false;
        address->host_name = true;
        len +=
            decode_character_string(&apdu[len], len_value_type, &char_string);
        if (len > apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        (void)characterstring_copy(&address->host.name, &char_string);
    } else {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
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
    len +=
        decode_tag_number_and_value(&apdu[len], &tag_number, &len_value_type);
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
        address->port = unsigned_value;
    } else {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
        }
        return BACNET_STATUS_REJECT;
    }

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
