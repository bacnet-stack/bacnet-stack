/**
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file
 * @author Mikhail Antropov
 * @date 2023
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <bacnet/config.h>
#include <bacnet/bacdef.h>
#include <bacnet/bacapp.h>
#include <bacnet/bacint.h>
#include <bacnet/sc_status.h>

static void host_n_port_to_data(
    BACNET_HOST_N_PORT *peer, BACNET_HOST_N_PORT_DATA *peer_data)
{
    peer_data->type = (peer->host_ip_address ? BACNET_HOST_N_PORT_IP : 0) +
        (peer->host_name ? BACNET_HOST_N_PORT_HOST : 0);

    if (peer->host_ip_address) {
        octetstring_copy_value(
            (uint8_t *)peer_data->host, 6, &peer->host.ip_address);
    } else if (peer->host_name) {
        characterstring_ansi_copy(
            peer_data->host, sizeof(peer_data->host), &peer->host.name);
    } else {
        peer_data->host[0] = 0;
    }

    peer_data->port = peer->port;
}

static void host_n_port_from_data(
    BACNET_HOST_N_PORT_DATA *peer_data, BACNET_HOST_N_PORT *peer)
{
    peer->host_ip_address = peer_data->type & BACNET_HOST_N_PORT_IP;
    peer->host_name = peer_data->type & BACNET_HOST_N_PORT_HOST;

    if (peer->host_ip_address) {
        octetstring_init(&peer->host.ip_address, (uint8_t *)peer_data->host, 6);
    } else if (peer->host_name) {
        characterstring_init_ansi(&peer->host.name, peer_data->host);
    }

    peer->port = peer_data->port;
}

/**
 * @brief Encode a SCHubConnection complex data type
 *
 *  BACnetSCHubConnection ::= SEQUENCE {
 *      connection-state [0] BACnetSCConnectionState,
 *      connect-timestamp [1] BACnetDateTime,
 *      disconnect-timestamp [2] BACnetDateTime,
 *      error [3] Error OPTIONAL,
 *      error-details [4] CharacterString OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_SCHubConnection(
    uint8_t *apdu, BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    BACNET_CHARACTER_STRING str;

    if (apdu && value) {
        apdu_len += encode_context_enumerated(&apdu[0], 0, value->State);
        apdu_len += bacapp_encode_context_datetime(
            &apdu[apdu_len], 1, &value->Connect_Timestamp);
        apdu_len += bacapp_encode_context_datetime(
            &apdu[apdu_len], 2, &value->Disconnect_Timestamp);

        if (value->State == BACNET_DISCONNECTED_WITH_ERRORS ||
            value->State == BACNET_FAILED_TO_CONNECT) {
            apdu_len +=
                encode_context_enumerated(&apdu[apdu_len], 3, value->Error);

            if (characterstring_init_ansi(&str, value->Error_Details)) {
                apdu_len +=
                    encode_context_character_string(&apdu[apdu_len], 4, &str);
            }
        }
    }
    return apdu_len;
}

int bacapp_decode_SCHubConnection(uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    int len;
    BACNET_CHARACTER_STRING str;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 0, &value->State)) ==
        -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 1, &value->Connect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 2, &value->Disconnect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 3, &value->Error)) >
        0) {
        apdu_len += len;
    } else {
        value->Error = ERROR_CODE_OTHER;
    }

    if ((len = decode_context_character_string(&apdu[apdu_len], 4, &str)) > 0) {
        snprintf(value->Error_Details, sizeof(value->Error_Details), "%s",
            characterstring_value(&str));
        apdu_len += len;
    } else {
        value->Error_Details[0] = 0;
    }

    return (apdu_len <= max_apdu_len) ? apdu_len : -1;
}

int bacapp_encode_context_SCHubConnection(
    uint8_t *apdu, uint8_t tag_number, BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value && apdu) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
        len = bacapp_encode_SCHubConnection(&apdu[apdu_len], value);
        apdu_len += len;
        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_context_SCHubConnection(
    uint8_t *apdu, uint8_t tag_number, BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    int len = 0;
    const uint16_t apdu_len_max = MAX_APDU;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len =
            bacapp_decode_SCHubConnection(&apdu[len], apdu_len_max, value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

/**
 * @brief Encode a SCHubFunctionConnection complex data type
 *
 *  BACnetSCHubFunctionConnection ::= SEQUENCE {
 *      connection-state [0] BACnetSCConnectionState,
 *      connect-timestamp [1] BACnetDateTime,
 *      disconnect-timestamp [2] BACnetDateTime,
 *      peer-address [3] BACnetHostNPort,
 *      peer-vmac [4] OCTET STRING (SIZE(6)),
 *      peer-uuid [5] OCTET STRING (SIZE(16)),
 *      error [6] Error OPTIONAL,
 *      error-details [7] CharacterString OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_SCHubFunctionConnection(
    uint8_t *apdu, BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    int apdu_len = 0, len = 0;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if (value) {
        len = encode_context_enumerated(apdu, 0, value->State);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len =
            bacapp_encode_context_datetime(apdu, 1, &value->Connect_Timestamp);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = bacapp_encode_context_datetime(
            apdu, 2, &value->Disconnect_Timestamp);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        host_n_port_from_data(&value->Peer_Address, &hp);
        len = host_n_port_context_encode(apdu, 3, &hp);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (octetstring_init(
                &octet, value->Peer_VMAC, sizeof(value->Peer_VMAC))) {
            len = encode_context_octet_string(apdu, 4, &octet);
        } else {
            return -1;
        }
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (octetstring_init(&octet, value->Peer_UUID.uuid.uuid128,
                sizeof(value->Peer_UUID.uuid.uuid128))) {
            len = encode_context_octet_string(apdu, 5, &octet);
        } else {
            return -1;
        }
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (value->State == BACNET_DISCONNECTED_WITH_ERRORS ||
            value->State == BACNET_FAILED_TO_CONNECT) {
            len = encode_context_enumerated(apdu, 6, value->Error);
            if (apdu) {
                apdu += len;
            }
            apdu_len += len;
            if (characterstring_init_ansi(&str, value->Error_Details)) {
                len = encode_context_character_string(apdu, 7, &str);
                apdu_len += len;
            }
        }
    }

    return apdu_len;
}

int bacapp_decode_SCHubFunctionConnection(uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    int len;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 0, &value->State)) ==
        -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 1, &value->Connect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 2, &value->Disconnect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = host_n_port_context_decode(&apdu[apdu_len], 3, &hp)) == -1) {
        return -1;
    }
    host_n_port_to_data(&hp, &value->Peer_Address);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 4, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_VMAC, sizeof(value->Peer_VMAC), &octet);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 5, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_UUID.uuid.uuid128,
        sizeof(value->Peer_UUID.uuid.uuid128), &octet);
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 6, &value->Error)) >
        0) {
        apdu_len += len;
    } else {
        value->Error = ERROR_CODE_OTHER;
    }

    if ((len = decode_context_character_string(&apdu[apdu_len], 7, &str)) > 0) {
        snprintf(value->Error_Details, sizeof(value->Error_Details), "%s",
            characterstring_value(&str));
        apdu_len += len;
    } else {
        value->Error_Details[0] = 0;
    }

    return (apdu_len <= max_apdu_len) ? apdu_len : -1;
}

/**
 * @brief Encode a context tagged SCHubFunctionConnection complex data type
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_context_SCHubFunctionConnection(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(apdu, tag_number);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = bacapp_encode_SCHubFunctionConnection(apdu, value);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = encode_closing_tag(apdu, tag_number);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
    }

    return apdu_len;
}

int bacapp_decode_context_SCHubFunctionConnection(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    int len = 0;
    const uint16_t apdu_len_max = MAX_APDU;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len = bacapp_decode_SCHubFunctionConnection(
            &apdu[len], apdu_len_max, value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

/**
 * @brief Encode a SCFailedConnectionRequest complex data type
 *
 *  BACnetSCFailedConnectionRequest ::= SEQUENCE {
 *      timestamp [0] BACnetDateTime,
 *      peer-address [1] BACnetHostNPort,
 *      peer-vmac [2] OCTET STRING (SIZE(6)) OPTIONAL,
 *      peer-uuid [3] OCTET STRING (SIZE(16)) OPTIONAL,
 *      error [4] Error,
 *      error-details [5] CharacterString OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_SCFailedConnectionRequest(
    uint8_t *apdu, BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    int apdu_len = 0, len = 0;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if (value) {
        len = bacapp_encode_context_datetime(apdu, 0, &value->Timestamp);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        host_n_port_from_data(&value->Peer_Address, &hp);
        len = host_n_port_context_encode(apdu, 1, &hp);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (octetstring_init(
                &octet, value->Peer_VMAC, sizeof(value->Peer_VMAC))) {
            len = encode_context_octet_string(apdu, 2, &octet);
        } else {
            return -1;
        }
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (octetstring_init(&octet, value->Peer_UUID.uuid.uuid128,
                sizeof(value->Peer_UUID.uuid.uuid128))) {
            len = encode_context_octet_string(apdu, 3, &octet);
        } else {
            return -1;
        }
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (value->Error != ERROR_CODE_OTHER) {
            len = encode_context_enumerated(apdu, 4, value->Error);
            if (apdu) {
                apdu += len;
            }
            apdu_len += len;

            if (characterstring_init_ansi(&str, value->Error_Details)) {
                len = encode_context_character_string(apdu, 5, &str);
                apdu_len += len;
            }
        }
    }

    return apdu_len;
}

int bacapp_decode_SCFailedConnectionRequest(uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    int apdu_len = 0;
    int len;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if ((len = bacapp_decode_context_datetime(
             &apdu[0], 0, &value->Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = host_n_port_context_decode(&apdu[apdu_len], 1, &hp)) == -1) {
        return -1;
    }
    host_n_port_to_data(&hp, &value->Peer_Address);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 2, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_VMAC, sizeof(value->Peer_VMAC), &octet);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 3, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_UUID.uuid.uuid128,
        sizeof(value->Peer_UUID.uuid.uuid128), &octet);
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 4, &value->Error)) >
        0) {
        apdu_len += len;
    } else {
        value->Error = ERROR_CODE_OTHER;
    }

    if ((len = decode_context_character_string(&apdu[apdu_len], 5, &str)) > 0) {
        snprintf(value->Error_Details, sizeof(value->Error_Details), "%s",
            characterstring_value(&str));
        apdu_len += len;
    } else {
        value->Error_Details[0] = 0;
    }

    return (apdu_len <= max_apdu_len) ? apdu_len : -1;
}

/**
 * @brief Encode a context tagged SCFailedConnectionRequest complex data type
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_context_SCFailedConnectionRequest(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    int apdu_len = 0, len = 0;

    if (value) {
        len = encode_opening_tag(apdu, tag_number);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = bacapp_encode_SCFailedConnectionRequest(apdu, value);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

int bacapp_decode_context_SCFailedConnectionRequest(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    int len = 0;
    const uint16_t apdu_len_max = MAX_APDU;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len = bacapp_decode_SCFailedConnectionRequest(
            &apdu[len], apdu_len_max, value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

/**
 * @brief Encode a SCFailedConnectionRequest complex data type
 *
 *  BACnetRouterEntry ::= SEQUENCE {
 *      network-number [0] Unsigned16,
 *      mac-address [1] OCTET STRING,
 *      status [2] ENUMERATED {
 *          available (0),
 *          busy (1),
 *          disconnected (2)
 *      },
 *      performance-index [3] Unsigned8 OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_RouterEntry(uint8_t *apdu, BACNET_ROUTER_ENTRY *value)
{
    int apdu_len = 0, len = 0;
    BACNET_OCTET_STRING octet;
    BACNET_UNSIGNED_INTEGER v;

    if (value) {
        v = value->Network_Number;
        len = encode_context_unsigned(apdu, 0, v);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (octetstring_init(
                &octet, value->Mac_Address, sizeof(value->Mac_Address))) {
            len = encode_context_octet_string(apdu, 1, &octet);
        } else {
            return -1;
        }
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = encode_context_enumerated(apdu, 2, value->Status);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (value->Performance_Index != 0) {
            v = value->Performance_Index;
            len = encode_context_unsigned(apdu, 3, v);
            apdu_len += len;
        }
    }

    return apdu_len;
}

int bacapp_decode_RouterEntry(uint8_t *apdu, BACNET_ROUTER_ENTRY *value)
{
    int apdu_len = 0;
    int len;
    BACNET_OCTET_STRING octet;
    BACNET_UNSIGNED_INTEGER v;

    if ((len = decode_context_unsigned(&apdu[0], 0, &v)) == -1) {
        return -1;
    }
    value->Network_Number = (uint16_t)v;
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 1, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(
        value->Mac_Address, sizeof(value->Mac_Address), &octet);
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 2, &value->Status)) ==
        -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = decode_context_unsigned(&apdu[apdu_len], 3, &v)) > 0) {
        value->Performance_Index = (uint8_t)v;
        apdu_len += len;
    } else {
        value->Performance_Index = 0;
    }

    return apdu_len;
}

int bacapp_encode_context_RouterEntry(
    uint8_t *apdu, uint8_t tag_number, BACNET_ROUTER_ENTRY *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value && apdu) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
        len = bacapp_encode_RouterEntry(&apdu[apdu_len], value);
        apdu_len += len;
        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_context_RouterEntry(
    uint8_t *apdu, uint8_t tag_number, BACNET_ROUTER_ENTRY *value)
{
    int len = 0;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len = bacapp_decode_RouterEntry(&apdu[len], value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

/**
 * @brief Encode a SCDirectConnection complex data type
 *
 * * BACnetSCDirectConnection ::= SEQUENCE {
 *      uri [0] CharacterString,
 *      connection-state [1] BACnetSCConnectionState,
 *      connect-timestamp [2] BACnetDateTime,
 *      disconnect-timestamp [3] BACnetDateTime,
 *      peer-address [4] BACnetHostNPort OPTIONAL,
 *      peer-vmac [5] OCTET STRING (SIZE(6)) OPTIONAL,
 *      peer-uuid [6] OCTET STRING (SIZE(16)) OPTIONAL,
 *      error [7] Error OPTIONAL,
 *      error-details [8] CharacterString OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param address - IP address and port number
 * @return length of the encoded APDU buffer
 */
int bacapp_encode_SCDirectConnection(
    uint8_t *apdu, BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    int apdu_len = 0, len = 0;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if (value) {
        if (characterstring_init_ansi(&str, value->URI)) {
            len = encode_context_character_string(apdu, 0, &str);
        } else {
            return -1;
        }
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = encode_context_enumerated(apdu, 1, value->State);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len =
            bacapp_encode_context_datetime(apdu, 2, &value->Connect_Timestamp);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = bacapp_encode_context_datetime(
            apdu, 3, &value->Disconnect_Timestamp);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        host_n_port_from_data(&value->Peer_Address, &hp);
        len = host_n_port_context_encode(apdu, 4, &hp);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (octetstring_init(
                &octet, value->Peer_VMAC, sizeof(value->Peer_VMAC))) {
            len = encode_context_octet_string(apdu, 5, &octet);
        } else {
            return -1;
        }
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (octetstring_init(&octet, value->Peer_UUID.uuid.uuid128,
                sizeof(value->Peer_UUID.uuid.uuid128))) {
            len = encode_context_octet_string(apdu, 6, &octet);
        } else {
            return -1;
        }
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        if (value->State == BACNET_DISCONNECTED_WITH_ERRORS ||
            value->State == BACNET_FAILED_TO_CONNECT) {
            len = encode_context_enumerated(apdu, 7, value->Error);
            if (apdu) {
                apdu += len;
            }
            apdu_len += len;
            if (characterstring_init_ansi(&str, value->Error_Details)) {
                len = encode_context_character_string(apdu, 8, &str);
                apdu_len += len;
            }
        }
    }

    return apdu_len;
}

int bacapp_decode_SCDirectConnection(uint8_t *apdu,
    uint16_t max_apdu_len,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    int apdu_len = 0;
    int len;
    BACNET_CHARACTER_STRING str;
    BACNET_OCTET_STRING octet;
    BACNET_HOST_N_PORT hp;

    if ((len = decode_context_character_string(&apdu[apdu_len], 0, &str)) ==
        -1) {
        return -1;
    }
    snprintf(value->URI, sizeof(value->URI), "%s", characterstring_value(&str));
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 1, &value->State)) ==
        -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 2, &value->Connect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = bacapp_decode_context_datetime(
             &apdu[apdu_len], 3, &value->Disconnect_Timestamp)) == -1) {
        return -1;
    }
    apdu_len += len;

    if ((len = host_n_port_context_decode(&apdu[apdu_len], 4, &hp)) == -1) {
        return -1;
    }
    host_n_port_to_data(&hp, &value->Peer_Address);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 5, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_VMAC, sizeof(value->Peer_VMAC), &octet);
    apdu_len += len;

    if ((len = decode_context_octet_string(&apdu[apdu_len], 6, &octet)) == -1) {
        return -1;
    }
    octetstring_copy_value(value->Peer_UUID.uuid.uuid128,
        sizeof(value->Peer_UUID.uuid.uuid128), &octet);
    apdu_len += len;

    if ((len = decode_context_enumerated(&apdu[apdu_len], 7, &value->Error)) >
        0) {
        apdu_len += len;
    } else {
        value->Error = ERROR_CODE_OTHER;
    }

    if ((len = decode_context_character_string(&apdu[apdu_len], 8, &str)) > 0) {
        snprintf(value->Error_Details, sizeof(value->Error_Details), "%s",
            characterstring_value(&str));
        apdu_len += len;
    } else {
        value->Error_Details[0] = 0;
    }

    return (apdu_len <= max_apdu_len) ? apdu_len : -1;
}

int bacapp_encode_context_SCDirectConnection(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(apdu, tag_number);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = bacapp_encode_SCDirectConnection(apdu, value);
        if (apdu) {
            apdu += len;
        }
        apdu_len += len;
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_context_SCDirectConnection(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    int len = 0;
    const uint16_t apdu_len_max = MAX_APDU;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len =
            bacapp_decode_SCDirectConnection(&apdu[len], apdu_len_max, value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

static int bacapp_snprintf_host_n_port(
    char *str, size_t str_len, BACNET_HOST_N_PORT_DATA *host_port)
{
    return snprintf(str, str_len, "%u.%u.%u.%u:%u, ",
        (uint8_t)host_port->host[0], (uint8_t)host_port->host[1],
        (uint8_t)host_port->host[2], (uint8_t)host_port->host[3],
        host_port->port);
}

static int bacapp_snprintf_vmac(char *str, size_t str_len, uint8_t *vmac)
{
    return snprintf(str, str_len, "%u.%u.%u.%u.%u.%u, ", vmac[0], vmac[1],
        vmac[2], vmac[3], vmac[4], vmac[5]);
}

static int bacapp_snprintf_uuid(char *str, size_t str_len, BACNET_UUID *uuid)
{
    struct guid *guid = &uuid->uuid.guid;
    uint8_t *p = guid->clock_seq_and_node;
    return snprintf(str, str_len,
        "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x, ",
        guid->time_low, guid->time_mid, guid->time_hi_and_version, p[0], p[1],
        p[2], p[3], p[4], p[5], p[6], p[7]);
}

static int snprintf_error_code(
    char *str, size_t str_len, int error, char *error_details)
{
    return error_details[0] ? 
        snprintf(str, str_len, "%d, \"%s\"", error, error_details) :
        snprintf(str, str_len, "%d", error);
}

#define POSITION_MOVE()     \
    if (str) {              \
        str += slen;        \
        str_len -= slen;    \
    }                       \
    ret_val += slen;

int bacapp_snprintf_SCFailedConnectionRequest(char *str, size_t str_len,
    BACNET_SC_FAILED_CONNECTION_REQUEST *req)
{
    int ret_val = 0;
    int slen;

    slen = snprintf(str, str_len, "{");
    POSITION_MOVE();
    slen = bacapp_snprintf_timestamp(str, str_len, &req->Timestamp);
    POSITION_MOVE();
    slen = snprintf(str, str_len, ", ");
    POSITION_MOVE();
    slen = bacapp_snprintf_host_n_port(str, str_len, &req->Peer_Address);
    POSITION_MOVE();
    slen = bacapp_snprintf_vmac(str, str_len, req->Peer_VMAC);
    POSITION_MOVE();
    slen = bacapp_snprintf_uuid(str, str_len, &req->Peer_UUID);
    POSITION_MOVE();
    slen = snprintf_error_code(str, str_len, req->Error, req->Error_Details);
    POSITION_MOVE();
    slen = snprintf(str, str_len, "}");
    POSITION_MOVE();
    return ret_val;
 }

int  bacapp_snprintf_SCHubFunctionConnection(char *str, size_t str_len,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *st)
{
    int ret_val = 0;
    int slen;

    slen = snprintf(str, str_len, "{%d, ", st->State);
    POSITION_MOVE();
    slen = bacapp_snprintf_timestamp(str, str_len, &st->Connect_Timestamp);
    POSITION_MOVE();
    slen = snprintf(str, str_len, ", ");
    POSITION_MOVE();
    slen = bacapp_snprintf_timestamp(str, str_len, &st->Disconnect_Timestamp);
    POSITION_MOVE();
    slen = snprintf(str, str_len, ", ");
    POSITION_MOVE();
    slen = bacapp_snprintf_host_n_port(str, str_len, &st->Peer_Address);
    POSITION_MOVE();
    slen = bacapp_snprintf_vmac(str, str_len, st->Peer_VMAC);
    POSITION_MOVE();
    slen = bacapp_snprintf_uuid(str, str_len, &st->Peer_UUID);
    POSITION_MOVE();
    slen = snprintf_error_code(str, str_len, st->Error, st->Error_Details);
    POSITION_MOVE();
    slen = snprintf(str, str_len, "}");
    POSITION_MOVE();
    return ret_val;
}

int bacapp_snprintf_SCDirectConnection(char *str, size_t str_len,
    BACNET_SC_DIRECT_CONNECTION_STATUS *st)
{
    int ret_val = 0;
    int slen;

    slen = snprintf(
        str, str_len, "{%s, %d, ", st->URI[0] ? st->URI : "NULL", st->State);
    POSITION_MOVE();
    slen = bacapp_snprintf_timestamp(str, str_len, &st->Connect_Timestamp);
    POSITION_MOVE();
    slen = snprintf(str, str_len, ", ");
    POSITION_MOVE();
    slen = bacapp_snprintf_timestamp(str, str_len, &st->Disconnect_Timestamp);
    POSITION_MOVE();
    slen = snprintf(str, str_len, ", ");
    POSITION_MOVE();
    slen = bacapp_snprintf_host_n_port(str, str_len, &st->Peer_Address);
    POSITION_MOVE();
    slen = bacapp_snprintf_vmac(str, str_len, st->Peer_VMAC);
    POSITION_MOVE();
    slen = bacapp_snprintf_uuid(str, str_len, &st->Peer_UUID);
    POSITION_MOVE();
    slen = snprintf_error_code(str, str_len, st->Error, st->Error_Details);
    POSITION_MOVE();
    slen = snprintf(str, str_len, "}");
    POSITION_MOVE();
    return ret_val;
}

int bacapp_snprintf_SCHubConnection(char *str, size_t str_len,
    BACNET_SC_HUB_CONNECTION_STATUS *st)
{
    int ret_val = 0;
    int slen;

    slen = snprintf(str, str_len, "{%d, ", st->State);
    POSITION_MOVE();
    slen = bacapp_snprintf_timestamp(str, str_len, &st->Connect_Timestamp);
    POSITION_MOVE();
    slen = snprintf(str, str_len, ", ");
    POSITION_MOVE();
    slen = bacapp_snprintf_timestamp(str, str_len, &st->Disconnect_Timestamp);
    POSITION_MOVE();
    slen = snprintf(str, str_len, ", ");
    POSITION_MOVE();
    slen = snprintf_error_code(str, str_len, st->Error, st->Error_Details);
    POSITION_MOVE();
    slen = snprintf(str, str_len, "}");
    POSITION_MOVE();
    return ret_val;
}
