/**
 * @file
 * @author Andriy Sukhynyuk, Vasyl Tkhir, Andriy Ivasiv
 * @date 2012
 * @brief Network layer for BACnet routing
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network_layer.h"
#include "bacnet/bacint.h"
#include "bacnet/bacenum.h"

/**
 * @brief Verify that an Initialize-Routing-Table(-Ack) payload is complete
 * @param pdu - payload starting at the number-of-ports octet
 * @param pdu_len - number of valid octets in the payload
 * @return true if every port mapping entry lies within the payload
 */
static bool routing_table_payload_valid(const uint8_t *pdu, int pdu_len)
{
    int offset = 1;
    int count;
    int info_len;

    if (pdu_len < 1) {
        return false;
    }
    count = pdu[0];
    while (count--) {
        /* DNET (2), port ID (1), port info length (1) */
        if ((pdu_len - offset) < 4) {
            return false;
        }
        info_len = pdu[offset + 3];
        if ((pdu_len - offset - 4) < info_len) {
            return false;
        }
        offset += 4 + info_len;
    }

    return true;
}

/**
 * @brief Add the networks of a validated Initialize-Routing-Table(-Ack)
 *  payload to the routing table
 * @param srcport - port the message was received on
 * @param src - source address of the message
 * @param pdu - payload starting at the number-of-ports octet
 */
static void routing_table_payload_apply(
    ROUTER_PORT *srcport, BACNET_ADDRESS src, const uint8_t *pdu)
{
    int offset = 1;
    int count = pdu[0];
    uint16_t net;

    while (count--) {
        decode_unsigned16(&pdu[offset], &net);
        if ((net != 0) && (net != BACNET_BROADCAST_NETWORK)) {
            add_dnet(&srcport->route_info, net, src);
        }
        offset += 4 + pdu[offset + 3];
    }
}

static bool
append_octet(uint8_t *buffer, size_t capacity, int *offset, uint8_t value)
{
    size_t length = (size_t)*offset;

    if ((*offset < 0) || (length >= capacity)) {
        return false;
    }
    buffer[*offset] = value;
    (*offset)++;

    return true;
}

static bool append_unsigned_network(
    uint8_t *buffer, size_t capacity, int *offset, uint16_t value)
{
    size_t length = (size_t)*offset;

    if ((*offset < 0) || (length > capacity) || ((capacity - length) < 2)) {
        return false;
    }
    *offset += encode_unsigned16(&buffer[*offset], value);

    return true;
}

int process_network_message(const BACMSG *msg, MSG_DATA *data, uint8_t **buff)
{
    BACNET_ADDRESS addr = { 0 };
    BACNET_NPDU_DATA npdu_data;
    ROUTER_PORT *srcport;
    ROUTER_PORT *destport;
    uint16_t net;
    uint8_t error_code;
    int buff_len = 0;
    int apdu_offset;
    int apdu_len;
    int net_count;
    int i;

    if (!msg || !msg->data || !data || !buff) {
        return -2;
    }
    *buff = NULL;
    memmove(data, msg->data, sizeof(MSG_DATA));
    if (!data->pdu || (data->pdu_len == 0)) {
        return -2;
    }

    apdu_offset = bacnet_npdu_decode(
        data->pdu, data->pdu_len, &data->dest, &addr, &npdu_data);
    if (apdu_offset <= 0) {
        PRINT(ERROR, "Error: Malformed network message\n");
        return -2;
    }
    apdu_len = data->pdu_len - apdu_offset;
    if (apdu_len < 0) {
        PRINT(ERROR, "Error: Malformed network message\n");
        return -2;
    }

    srcport = find_snet(msg->origin);
    if (!srcport) {
        PRINT(ERROR, "Error: Unknown source port\n");
        return -2;
    }
    data->src.net = srcport->route_info.net;
    /* traffic received from a learned network */
    if ((addr.net > 0) && (addr.net < BACNET_BROADCAST_NETWORK) &&
        (addr.net != data->src.net)) {
        refresh_dnet(&srcport->route_info, addr.net);
    }

    switch (npdu_data.network_message_type) {
        case NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK:
            PRINT(INFO, "Recieved Who-Is-Router-To-Network message\n");
            if (apdu_len == 1) {
                PRINT(ERROR, "Error: Malformed network message\n");
                return -2;
            }
            if (apdu_len) {
                /* if NET specified */
                decode_unsigned16(&data->pdu[apdu_offset], &net);
                if (srcport->route_info.net == net) {
                    PRINT(INFO, "Message discarded: NET directly connected\n");
                    return -2;
                }

                destport = find_dnet(net, NULL); /* see if NET can be reached */
                if (destport) {
                    /* if TRUE send reply */
                    PRINT(INFO, "Sending I-Am-Router-To-Network message\n");
                    buff_len = create_network_message(
                        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, data, buff,
                        &net);
                } else {
                    data->dest.net = net; /* NET to look for */
                    return -1; /* else initiate NET search procedure */
                }
            } else {
                /* if NET is omitted (message sent with -1) */
                PRINT(INFO, "Sending I-Am-Router-To-Network message\n");
                buff_len = create_network_message(
                    NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, data, buff, NULL);
            }

            break;

        case NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK: {
            PRINT(INFO, "Recieved I-Am-Router-To-Network message\n");
            net_count = apdu_len / 2;
            for (i = 0; i < net_count; i++) {
                decode_unsigned16(
                    &data->pdu[apdu_offset + 2 * i],
                    &net); /* decode received NET values */
                if ((net != 0) && (net != BACNET_BROADCAST_NETWORK)) {
                    add_dnet(
                        &srcport->route_info, net,
                        data->src); /* and update routing table */
                }
            }
            break;
        }
        case NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK: {
            /* first octet of the message contains rejection reason */
            /* next two octets contain NET (can be decoded for additional info
             * on error) */
            if (apdu_len < 1) {
                PRINT(ERROR, "Error: Malformed network message\n");
                return -2;
            }
            error_code = data->pdu[apdu_offset];
            switch (error_code) {
                case 0:
                    PRINT(ERROR, "Error!\n");
                    break;
                case 1:
                    PRINT(ERROR, "Error: Network unreachable\n");
                    break;
                case 2:
                    PRINT(ERROR, "Error: Network is busy\n");
                    break;
                case 3:
                    PRINT(ERROR, "Error: Unknown network message type\n");
                    break;
                case 4:
                    PRINT(ERROR, "Error: Message too long\n");
                    break;
                default:
                    break;
            }
            break;
        }
        case NETWORK_MESSAGE_INIT_RT_TABLE:
            PRINT(INFO, "Recieved Initialize-Routing-Table message\n");
            if (!routing_table_payload_valid(
                    &data->pdu[apdu_offset], apdu_len)) {
                PRINT(ERROR, "Error: Malformed network message\n");
                return -2;
            }
            if (data->pdu[apdu_offset] > 0) {
                routing_table_payload_apply(
                    srcport, data->src, &data->pdu[apdu_offset]);
                buff_len = create_network_message(
                    NETWORK_MESSAGE_INIT_RT_TABLE_ACK, data, buff, NULL);
            } else {
                buff_len = create_network_message(
                    NETWORK_MESSAGE_INIT_RT_TABLE_ACK, data, buff, &buff);
            }
            break;

        case NETWORK_MESSAGE_INIT_RT_TABLE_ACK:
            PRINT(INFO, "Recieved Initialize-Routing-Table-Ack message\n");
            if (!routing_table_payload_valid(
                    &data->pdu[apdu_offset], apdu_len)) {
                PRINT(ERROR, "Error: Malformed network message\n");
                return -2;
            }
            routing_table_payload_apply(
                srcport, data->src, &data->pdu[apdu_offset]);
            break;

        case NETWORK_MESSAGE_INVALID:
        case NETWORK_MESSAGE_I_COULD_BE_ROUTER_TO_NETWORK:
        case NETWORK_MESSAGE_ROUTER_BUSY_TO_NETWORK:
        case NETWORK_MESSAGE_ROUTER_AVAILABLE_TO_NETWORK:
        case NETWORK_MESSAGE_ESTABLISH_CONNECTION_TO_NETWORK:
        case NETWORK_MESSAGE_DISCONNECT_CONNECTION_TO_NETWORK:
            /* hell if I know what to do with these messages */
            break;
        case NETWORK_MESSAGE_CHALLENGE_REQUEST:
        case NETWORK_MESSAGE_SECURITY_PAYLOAD:
        case NETWORK_MESSAGE_SECURITY_RESPONSE:
        case NETWORK_MESSAGE_REQUEST_KEY_UPDATE:
        case NETWORK_MESSAGE_UPDATE_KEY_SET:
        case NETWORK_MESSAGE_UPDATE_DISTRIBUTION_KEY:
        case NETWORK_MESSAGE_REQUEST_MASTER_KEY:
        case NETWORK_MESSAGE_SET_MASTER_KEY:
        case NETWORK_MESSAGE_NETWORK_NUMBER_IS:
            /* security messages */
            break;
        case NETWORK_MESSAGE_WHAT_IS_NETWORK_NUMBER:
            buff_len = create_network_message(
                NETWORK_MESSAGE_NETWORK_NUMBER_IS, data, buff, &buff);
            break;

        default:
            PRINT(ERROR, "Error: Message unsupported\n");
            break;
    }

    /* create_network_message() returns -1 when the reply cannot be built.
     * That is a discard, not a request to search for a network. */
    if (buff_len < 0) {
        return -2;
    }

    return buff_len;
}

/**
 * @brief Build Reject-Message-To-Network, reason 4.
 * @param origin - NPDU source of the rejected message. When it carries
 *  SNET/SADR for a network other than local_net, the reject's DNET/DADR
 *  is that device (ASHRAE 135 6.4.4). The frame is still returned to the
 *  source port; reply_to is the previous hop that will forward it.
 *  With no SNET, or an SNET equal to this port's own network, the reject
 *  is local. The forward path ignores that specifier, and so does this.
 * @param local_net - network directly connected to the receiving port
 * @return ROUTER_MSG_REJECT, or -2 when the reject cannot be built.
 */
static int reject_message_too_long(
    MSG_DATA *data,
    uint8_t **buff,
    const BACNET_ADDRESS *reply_to,
    const BACNET_ADDRESS *origin,
    uint16_t local_net,
    uint16_t dnet)
{
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS remote_dest = { 0 };
    BACNET_ADDRESS *npdu_dest;
    uint8_t header[MAX_NPDU];
    uint8_t *pdu;
    int npdu_len;
    int total;
    bool remote_origin;

    npdu_dest = NULL;
    remote_origin = false;
    if (origin != NULL) {
        /* SNET equal to the receiving port is not a remote device. */
        if ((origin->net > 0) && (origin->net < BACNET_BROADCAST_NETWORK) &&
            (origin->len > 0) && (origin->net != local_net)) {
            remote_origin = true;
        }
    }
    /* Original source device. A local origin has no DNET/DADR. */
    if (remote_origin) {
        memmove(&remote_dest, origin, sizeof(remote_dest));
        npdu_dest = &remote_dest;
    }
    init_npdu(&npdu_data, NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK, false);
    npdu_len = npdu_encode_pdu(NULL, npdu_dest, NULL, &npdu_data);
    if ((npdu_len <= 0) || (npdu_len > MAX_NPDU) ||
        ((size_t)npdu_len > (MAX_PDU - 3))) {
        return -2;
    }
    if (npdu_encode_pdu(header, npdu_dest, NULL, &npdu_data) != npdu_len) {
        return -2;
    }
    total = npdu_len + 3;
    pdu = (uint8_t *)malloc((size_t)total);
    if (!pdu) {
        return -2;
    }
    memcpy(pdu, header, (size_t)npdu_len);
    pdu[npdu_len] = (uint8_t)NETWORK_REJECT_MESSAGE_TOO_LONG;
    encode_unsigned16(&pdu[npdu_len + 1], dnet);
    *buff = pdu;
    data->pdu_len = (uint16_t)total;
    /* Previous hop on the source port. The port uses this MAC locally. */
    if (reply_to) {
        memmove(&data->dest, reply_to, sizeof(data->dest));
    }
    /* A broadcast network number would make the port broadcast the reject. */
    if (data->dest.net == BACNET_BROADCAST_NETWORK) {
        data->dest.net = 0;
    }

    return ROUTER_MSG_REJECT;
}

/**
 * @brief Forward an application NPDU toward its destination network.
 * @param msg - received router message
 * @param data - working copy of the message; the PDU stays shared with
 *  the received message
 * @param buff - set to the forwarded PDU, or NULL when nothing is sent.
 *  For ROUTER_MSG_REJECT, *buff is the reject PDU.
 * @return positive length to send, -1 to search for the destination
 *  network, ROUTER_MSG_REJECT to answer the source, or -2 to discard
 */
int process_msg(BACMSG *msg, MSG_DATA *data, uint8_t **buff)
{
    BACNET_ADDRESS addr = { 0 };
    BACNET_ADDRESS reply_to = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    ROUTER_PORT *srcport;
    ROUTER_PORT *destport;
    uint8_t npdu[MAX_NPDU];
    int buff_len = 0;
    int apdu_offset;
    int apdu_len;
    int npdu_len;

    if (buff) {
        *buff = NULL;
    }
    if ((!msg) || (!msg->data) || (!data) || (!buff)) {
        return -2;
    }
    memmove(data, msg->data, sizeof(MSG_DATA));
    memmove(&reply_to, &data->src, sizeof(reply_to));

    apdu_offset = bacnet_npdu_decode(
        data->pdu, data->pdu_len, &data->dest, &addr, &npdu_data);
    if (apdu_offset <= 0) {
        /* malformed NPDU: discard */
        return -2;
    }
    apdu_len = data->pdu_len - apdu_offset;

    srcport = find_snet(msg->origin);
    if (srcport == NULL) {
        /* Unknown source port is a discard, including NDEBUG builds. */
        return -2;
    }
    destport = find_dnet(data->dest.net, NULL);
    if (destport == NULL) {
        /* request net search */
        return -1;
    }

    data->src.net = srcport->route_info.net;

    /* if received from another router save real source address (not other
     * router source address) */
    if ((addr.net > 0) && (addr.net < BACNET_BROADCAST_NETWORK) &&
        (data->src.net != addr.net)) {
        refresh_dnet(&srcport->route_info, addr.net);
        memmove(&data->src, &addr, sizeof(BACNET_ADDRESS));
    }

    /* encode both source and destination for broadcast and router-to-router
     * communication */
    if ((data->dest.net == BACNET_BROADCAST_NETWORK) ||
        (destport->route_info.net != data->dest.net)) {
        npdu_len = npdu_encode_pdu(NULL, &data->dest, &data->src, &npdu_data);
    } else {
        npdu_len = npdu_encode_pdu(NULL, NULL, &data->src, &npdu_data);
    }

    if ((npdu_len <= 0) || (apdu_len < 0) || (npdu_len > MAX_NPDU)) {
        return -2;
    }
    if ((size_t)apdu_len > (MAX_PDU - (size_t)npdu_len)) {
        return reject_message_too_long(
            data, buff, &reply_to, &addr, srcport->route_info.net,
            data->dest.net);
    }
    if ((data->dest.net == BACNET_BROADCAST_NETWORK) ||
        (destport->route_info.net != data->dest.net)) {
        npdu_len = npdu_encode_pdu(npdu, &data->dest, &data->src, &npdu_data);
    } else {
        npdu_len = npdu_encode_pdu(npdu, NULL, &data->src, &npdu_data);
    }
    if ((npdu_len <= 0) || (npdu_len > MAX_NPDU)) {
        return -2;
    }
    buff_len = npdu_len + apdu_len;
    *buff = (uint8_t *)malloc((size_t)buff_len);
    if (!*buff) {
        PRINT(ERROR, "Error: Could not allocate message\n");
        return -2;
    }
    memmove(*buff, npdu, npdu_len); /* copy newly formed NPDU */
    memmove(
        *buff + npdu_len, &data->pdu[apdu_offset], apdu_len); /* copy APDU */

    return buff_len;
}

bool is_network_msg(const BACMSG *msg)
{
    uint8_t control_byte; /* NPDU control byte */
    const MSG_DATA *data = (const MSG_DATA *)msg->data;

    if (data->pdu_len < 2) {
        return false;
    }
    control_byte = data->pdu[1];

    return control_byte & 0x80; /* check 7th bit */
}

int create_network_message(
    BACNET_NETWORK_MESSAGE_TYPE network_message_type,
    MSG_DATA *data,
    uint8_t **buff,
    void *val)
{
    const size_t buffer_size = MAX_PDU;
    int buff_len;
    int npdu_len;
    bool data_expecting_reply = false;
    BACNET_NPDU_DATA npdu_data;

    if (!data || !buff) {
        return -1;
    }
    *buff = NULL;
    if (network_message_type == NETWORK_MESSAGE_INIT_RT_TABLE) {
        data_expecting_reply = true;
    }
    init_npdu(&npdu_data, network_message_type, data_expecting_reply);

    /* manual destination setup for Init-RT-Table-Ack message */
    data->dest.net = BACNET_BROADCAST_NETWORK;
    npdu_len = npdu_encode_pdu(NULL, &data->dest, NULL, &npdu_data);
    if ((npdu_len <= 0) || ((size_t)npdu_len > buffer_size)) {
        return -1;
    }
    *buff = (uint8_t *)malloc(buffer_size);
    if (!*buff) {
        PRINT(ERROR, "Error: Could not allocate network message\n");
        return -1;
    }
    buff_len = npdu_encode_pdu(*buff, &data->dest, NULL, &npdu_data);
    if (buff_len != npdu_len) {
        free(*buff);
        *buff = NULL;
        return -1;
    }

    switch (network_message_type) {
        case NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK:
            if (val != NULL) {
                uint16_t val16;
                memmove(&val16, val, sizeof(val16));
                if (!append_unsigned_network(
                        *buff, buffer_size, &buff_len, val16)) {
                    free(*buff);
                    *buff = NULL;
                    return -1;
                }
            }
            break;

        case NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK:
            if (val != NULL) {
                uint16_t val16;
                memmove(&val16, val, sizeof(val16));
                if (!append_unsigned_network(
                        *buff, buffer_size, &buff_len, val16)) {
                    free(*buff);
                    *buff = NULL;
                    return -1;
                }
            } else {
                ROUTER_PORT *port = head;
                DNET *dnet;
                bool buffer_full = false;
                while (port != NULL) {
                    expire_dnets(&port->route_info);
                    if (port->route_info.net != data->src.net) {
                        if (!append_unsigned_network(
                                *buff, buffer_size, &buff_len,
                                port->route_info.net)) {
                            buffer_full = true;
                        }
                    }
                    dnet = port->route_info.dnets;
                    while (!buffer_full && (dnet != NULL)) {
                        if (!append_unsigned_network(
                                *buff, buffer_size, &buff_len, dnet->net)) {
                            buffer_full = true;
                            break;
                        }
                        dnet = dnet->next;
                    }
                    if (buffer_full) {
                        PRINT(
                            ERROR, "Error: I-Am-Router-To-Network truncated\n");
                        break;
                    }
                    port = port->next;
                }
            }
            break;

        case NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK: {
            uint16_t val16;
            if (!val) {
                free(*buff);
                *buff = NULL;
                return -1;
            }
            memmove(&val16, val, sizeof(val16));
            if (!append_unsigned_network(
                    *buff, buffer_size, &buff_len, val16)) {
                free(*buff);
                *buff = NULL;
                return -1;
            }
            break;
        }
        case NETWORK_MESSAGE_INIT_RT_TABLE:
        case NETWORK_MESSAGE_INIT_RT_TABLE_ACK:
            if ((uint8_t *)val) {
                ROUTER_PORT *port = head;
                int count_offset = buff_len;
                uint8_t count = 0;

                if (!append_octet(*buff, buffer_size, &buff_len, 0)) {
                    free(*buff);
                    *buff = NULL;
                    return -1;
                }
                while ((port != NULL) && (count < UINT8_MAX)) {
                    if ((size_t)(buffer_size - (size_t)buff_len) < 4) {
                        break;
                    }
                    if (!append_unsigned_network(
                            *buff, buffer_size, &buff_len,
                            port->route_info.net) ||
                        !append_octet(
                            *buff, buffer_size, &buff_len, count + 1) ||
                        !append_octet(*buff, buffer_size, &buff_len, 0)) {
                        free(*buff);
                        *buff = NULL;
                        return -1;
                    }
                    count++;
                    port = port->next;
                }
                (*buff)[count_offset] = count;
            } else {
                if (!append_octet(*buff, buffer_size, &buff_len, 0)) {
                    free(*buff);
                    *buff = NULL;
                    return -1;
                }
            }
            break;

        case NETWORK_MESSAGE_INVALID:
        case NETWORK_MESSAGE_I_COULD_BE_ROUTER_TO_NETWORK:
        case NETWORK_MESSAGE_ROUTER_BUSY_TO_NETWORK:
        case NETWORK_MESSAGE_ROUTER_AVAILABLE_TO_NETWORK:
        case NETWORK_MESSAGE_ESTABLISH_CONNECTION_TO_NETWORK:
        case NETWORK_MESSAGE_DISCONNECT_CONNECTION_TO_NETWORK:
            /* hell if I know what to do with these messages */
            break;
        case NETWORK_MESSAGE_CHALLENGE_REQUEST:
        case NETWORK_MESSAGE_SECURITY_PAYLOAD:
        case NETWORK_MESSAGE_SECURITY_RESPONSE:
        case NETWORK_MESSAGE_REQUEST_KEY_UPDATE:
        case NETWORK_MESSAGE_UPDATE_KEY_SET:
        case NETWORK_MESSAGE_UPDATE_DISTRIBUTION_KEY:
        case NETWORK_MESSAGE_REQUEST_MASTER_KEY:
        case NETWORK_MESSAGE_SET_MASTER_KEY:
            /* security messages */
            break;
        case NETWORK_MESSAGE_NETWORK_NUMBER_IS:
            /* fixme: needs message constructed */
            break;
        default:
            break;
    }

    return buff_len;
}

/**
 * @brief Build a network message and queue it to every active port.
 * @return positive length of the queued PDU, or -1 when it was not sent.
 *  On failure the caller still owns @p data when it supplied that pointer.
 *  A NULL @p data is allocated and released here.
 */
int send_network_message(
    BACNET_NETWORK_MESSAGE_TYPE network_message_type,
    MSG_DATA *data,
    uint8_t **buff,
    void *val)
{
    BACMSG msg;
    ROUTER_PORT *port = head;
    int buff_len;
    bool allocated_data = false;

    if (!buff) {
        return -1;
    }
    *buff = NULL;
    if (!data) {
        data = (MSG_DATA *)malloc(sizeof(MSG_DATA));
        if (!data) {
            PRINT(ERROR, "Error: Could not allocate network message data\n");
            return -1;
        }
        allocated_data = true;
        data->dest.net = BACNET_BROADCAST_NETWORK;
        data->dest.len = 0;
    }

    buff_len = create_network_message(network_message_type, data, buff, val);
    if (buff_len <= 0) {
        if (allocated_data) {
            free(data);
        }
        return -1;
    }

    /* form network message */
    data->pdu = *buff;
    data->pdu_len = buff_len;
    msg.origin = head->main_id;
    msg.type = DATA;
    msg.data = data;

    data->ref_count = port_count;
    while (port != NULL) {
        if (port->state == FINISHED) {
            port = port->next;
            continue;
        }
        send_to_msgbox(port->port_id, &msg);
        port = port->next;
    }

    return buff_len;
}

void init_npdu(
    BACNET_NPDU_DATA *npdu_data,
    BACNET_NETWORK_MESSAGE_TYPE network_message_type,
    bool data_expecting_reply)
{
    if (npdu_data) {
        npdu_data->data_expecting_reply = data_expecting_reply;
        npdu_data->protocol_version = BACNET_PROTOCOL_VERSION;
        npdu_data->network_layer_message = true;
        npdu_data->network_message_type = network_message_type;
        npdu_data->vendor_id = 0;
        npdu_data->priority = MESSAGE_PRIORITY_NORMAL;
        npdu_data->hop_count = HOP_COUNT_DEFAULT;
    }
}
