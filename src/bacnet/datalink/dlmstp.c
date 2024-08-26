/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief Implementation of the Network Layer using BACnet MS/TP transport
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink Network Layer
 * @ingroup DataLink
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/crc.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstpdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacaddr.h"

/* the current MSTP port that the datalink is using */
static struct mstp_port_struct_t *MSTP_Port;

/**
 * @brief send an PDU via MSTP
 * @param dest - BACnet destination address
 * @param npdu_data - network layer information
 * @param pdu - PDU data to send
 * @param pdu_len - number of bytes of PDU data to send
 * @return number of bytes sent on success, zero on failure
 */
int dlmstp_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    int bytes_sent = 0;
    unsigned i = 0; /* loop counter */
    struct dlmstp_user_data_t *user = NULL;
    struct dlmstp_packet *pkt;

    if (!MSTP_Port) {
        return 0;
    }
    if (!MSTP_Port->UserData) {
        return 0;
    }
    user = MSTP_Port->UserData;
    pkt = (struct dlmstp_packet *)(void *)Ringbuf_Data_Peek(&user->PDU_Queue);
    if (pkt && (pdu_len <= DLMSTP_MPDU_MAX)) {
        if (npdu_data->data_expecting_reply) {
            pkt->frame_type = FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY;
        } else {
            pkt->frame_type = FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY;
        }
        for (i = 0; i < pdu_len; i++) {
            pkt->pdu[i] = pdu[i];
        }
        pkt->pdu_len = pdu_len;
        if (dest && dest->mac_len) {
            pkt->address.mac_len = 1;
            pkt->address.mac[0] = dest->mac[0];
            pkt->address.len = 0;
        } else {
            pkt->address.mac_len = 1;
            pkt->address.mac[0] = MSTP_BROADCAST_ADDRESS;
            pkt->address.len = 0;
        }
        if (Ringbuf_Data_Put(&user->PDU_Queue, (uint8_t *)pkt)) {
            bytes_sent = pdu_len;
        }
    }

    return bytes_sent;
}

/**
 * @brief The MS/TP state machine uses this function for getting data to send
 * @param mstp_port - specific MSTP port that is used for this datalink
 * @param timeout - number of milliseconds to wait for the data
 * @return amount of PDU data
 */
uint16_t MSTP_Get_Send(
    struct mstp_port_struct_t *mstp_port, unsigned timeout)
{
    uint16_t pdu_len = 0;
    struct dlmstp_packet *pkt;
    struct dlmstp_user_data_t *user;

    if (!mstp_port) {
        return 0;
    }
    user = (struct dlmstp_user_data_t *)mstp_port->UserData;
    if (!user) {
        return 0;
    }
    if (Ringbuf_Empty(&user->PDU_Queue)) {
        return 0;
    }
    /* look at next PDU in queue without removing it */
    pkt = (struct dlmstp_packet *)(void *)Ringbuf_Peek(&user->PDU_Queue);
    /* convert the PDU into the MSTP Frame */
    pdu_len = MSTP_Create_Frame(&mstp_port->OutputBuffer[0],
        mstp_port->OutputBufferSize, pkt->frame_type, pkt->address.mac[0],
        mstp_port->This_Station, &pkt->pdu[0], pkt->pdu_len);
    user->Statistics.transmit_pdu_counter++;
    (void)Ringbuf_Pop(&user->PDU_Queue, NULL);

    return pdu_len;
}

/**
 * @brief Determine if the reply packet is the data expected
 * @param mstp_port - specific MSTP port that is used for this datalink
 * @param reply_pdu - PDU of the data
 * @param reply_pdu_len - number of bytes of PDU data
 * @param dest_address - the destination address for this data
 * @return true if the reply packet is the data expected
 */
static bool MSTP_Compare_Data_Expecting_Reply(
    volatile struct mstp_port_struct_t *mstp_port,
    uint8_t *reply_pdu,
    uint16_t reply_pdu_len,
    BACNET_ADDRESS *dest_address)
{
    uint16_t offset;
    /* One way to check the message is to compare NPDU
       src, dest, along with the APDU type, invoke id.
       Seems a bit overkill */
    struct DER_compare_t {
        BACNET_NPDU_DATA npdu_data;
        BACNET_ADDRESS address;
        uint8_t pdu_type;
        uint8_t invoke_id;
        uint8_t service_choice;
    };
    struct DER_compare_t request;
    struct DER_compare_t reply;
    uint8_t *request_pdu;
    uint16_t request_pdu_len;
    uint8_t src_address;

    request_pdu = &mstp_port->InputBuffer[0];
    request_pdu_len = mstp_port->DataLength;
    src_address = mstp_port->SourceAddress;
    /* decode the request data */
    request.address.mac[0] = src_address;
    request.address.mac_len = 1;
    offset = bacnet_npdu_decode(
        &request_pdu[0], request_pdu_len, NULL, &request.address,
        &request.npdu_data);
    if (request.npdu_data.network_layer_message) {
        return false;
    }
    request.pdu_type = request_pdu[offset] & 0xF0;
    if (request.pdu_type != PDU_TYPE_CONFIRMED_SERVICE_REQUEST) {
        return false;
    }
    request.invoke_id = request_pdu[offset + 2];
    /* segmented message? */
    if (request_pdu[offset] & BIT(3))
        request.service_choice = request_pdu[offset + 5];
    else
        request.service_choice = request_pdu[offset + 3];
    /* decode the reply data */
    bacnet_address_copy(&reply.address, dest_address);
    offset = bacnet_npdu_decode(
        &reply_pdu[0], reply_pdu_len, &reply.address, NULL,
        &reply.npdu_data);
    if (reply.npdu_data.network_layer_message) {
        return false;
    }
    /* reply could be a lot of things:
       confirmed, simple ack, abort, reject, error */
    reply.pdu_type = reply_pdu[offset] & 0xF0;
    switch (reply.pdu_type) {
        case PDU_TYPE_SIMPLE_ACK:
            reply.invoke_id = reply_pdu[offset + 1];
            reply.service_choice = reply_pdu[offset + 2];
            break;
        case PDU_TYPE_COMPLEX_ACK:
            reply.invoke_id = reply_pdu[offset + 1];
            /* segmented message? */
            if (reply_pdu[offset] & BIT(3))
                reply.service_choice = reply_pdu[offset + 4];
            else
                reply.service_choice = reply_pdu[offset + 2];
            break;
        case PDU_TYPE_ERROR:
            reply.invoke_id = reply_pdu[offset + 1];
            reply.service_choice = reply_pdu[offset + 2];
            break;
        case PDU_TYPE_REJECT:
        case PDU_TYPE_ABORT:
            reply.invoke_id = reply_pdu[offset + 1];
            break;
        default:
            return false;
    }
    /* these don't have service choice included */
    if ((reply.pdu_type == PDU_TYPE_REJECT) ||
        (reply.pdu_type == PDU_TYPE_ABORT)) {
        if (request.invoke_id != reply.invoke_id) {
            return false;
        }
    } else {
        if (request.invoke_id != reply.invoke_id) {
            return false;
        }
        if (request.service_choice != reply.service_choice) {
            return false;
        }
    }
    if (request.npdu_data.protocol_version !=
        reply.npdu_data.protocol_version) {
        return false;
    }
#if 0
    /* the NDPU priority doesn't get passed through the stack, and
       all outgoing messages have NORMAL priority */
    if (request.npdu_data.priority != reply.npdu_data.priority) {
        return false;
    }
#endif
    if (!bacnet_address_same(&request.address, &reply.address)) {
        return false;
    }

    return true;
}

/**
 * @brief The MS/TP state machine uses this function for getting data to send
 *  as the reply to a DATA_EXPECTING_REPLY frame, or nothing
 * @param mstp_port MSTP port structure for this port
 * @param timeout number of milliseconds to wait for a packet
 * @return number of bytes, or 0 if no reply is available
 */
uint16_t MSTP_Get_Reply(
    struct mstp_port_struct_t *mstp_port, unsigned timeout)
{
    uint16_t pdu_len = 0;
    bool matched = false;
    struct dlmstp_user_data_t *user = NULL;
    struct dlmstp_packet *pkt;

    if (!mstp_port) {
        return 0;
    }
    user = mstp_port->UserData;
    if (!user) {
        return 0;
    }
    if (Ringbuf_Empty(&user->PDU_Queue)) {
        return 0;
    }
    /* look at next PDU in queue without removing it */
    pkt = (struct dlmstp_packet *)(void *)Ringbuf_Peek(&user->PDU_Queue);
    /* is this the reply to the DER? */
    matched = MSTP_Compare_Data_Expecting_Reply(
        mstp_port, pkt->pdu, pkt->pdu_len, &pkt->address);
    if (!matched) {
        return 0;
    }
    /* convert the PDU into the MSTP Frame */
    pdu_len = MSTP_Create_Frame(&mstp_port->OutputBuffer[0],
        mstp_port->OutputBufferSize, pkt->frame_type, pkt->address.mac[0],
        mstp_port->This_Station, &pkt->pdu[0], pkt->pdu_len);
    user->Statistics.transmit_pdu_counter++;
    (void)Ringbuf_Pop(&user->PDU_Queue, NULL);

    return pdu_len;
}

/**
 * @brief MS/TP state machine callback to use for sending a frame
 * @param mstp_port - specific MSTP port that is used for this datalink
 * @param buffer - buffer to send
 * @param nbytes - number of bytes of data to send
 */
void MSTP_Send_Frame(struct mstp_port_struct_t *mstp_port,
    uint8_t *buffer,
    uint16_t nbytes)
{
    struct dlmstp_user_data_t *user;
    struct dlmstp_rs485_driver *driver;

    if (!mstp_port) {
        return;
    }
    user = mstp_port->UserData;
    if (!user) {
        return;
    }
    driver = user->RS485_Driver;
    if (!driver) {
        return;
    }
    driver->send(buffer, nbytes);
    user->Statistics.transmit_frame_counter++;
}

/**
 * @brief MS/TP state machine received a frame
 * @return number of bytes queued, or 0 if unable to be queued
 */
uint16_t MSTP_Put_Receive(struct mstp_port_struct_t *mstp_port)
{
    struct dlmstp_user_data_t *user = NULL;

    if (!mstp_port) {
        return 0;
    }
    user = mstp_port->UserData;
    if (!user) {
        return 0;
    }
    user->ReceivePacketPending = true;

    return mstp_port->DataLength;
}

/**
 * @brief Run the MS/TP state machines, and get packet if available
 * @param pdu - place to put PDU data for the caller
 * @param max_pdu - number of bytes of PDU data that caller can receive
 * @return number of bytes in received packet, or 0 if no packet was received
 * @note Must be called at least once every 1 milliseconds, with no more than
 *  5 milliseconds jitter.
 */
uint16_t dlmstp_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    uint16_t pdu_len = 0;
    uint8_t data_register = 0;
    struct dlmstp_user_data_t *user;
    struct dlmstp_rs485_driver *driver;
    uint16_t i;
    uint32_t milliseconds;

    if (!MSTP_Port) {
        return 0;
    }
    if (!MSTP_Port->UserData) {
        return 0;
    }
    user = MSTP_Port->UserData;
    if (!user) {
        return 0;
    }
    driver = user->RS485_Driver;
    if (!driver) {
        return 0;
    }
    while (!MSTP_Port->InputBuffer) {
        /* FIXME: develop configure an input buffer! */
    }
    if (driver->transmitting()) {
        /* we're transmitting; do nothing else */
        return 0;
    }
    /* only do receive state machine while we don't have a frame */
    while ((MSTP_Port->ReceivedValidFrame == false) &&
        (MSTP_Port->ReceivedInvalidFrame == false)) {
        MSTP_Port->DataAvailable = driver->read(&data_register);
        if (MSTP_Port->DataAvailable) {
            MSTP_Port->DataRegister = data_register;
        }
        MSTP_Receive_Frame_FSM(MSTP_Port);
        /* process another byte, if available */
        if (!driver->read(NULL)) {
            break;
        }
    }
    if (MSTP_Port->ReceivedValidFrame || MSTP_Port->ReceivedInvalidFrame) {
        /* delay after reception before transmitting - per MS/TP spec */
        milliseconds = MSTP_Port->SilenceTimer(MSTP_Port);
        if (milliseconds < MSTP_Port->Tturnaround_timeout) {
            /* we're waiting; do nothing else */
            return 0;
        }
    }
    if (MSTP_Port->ReceivedValidFrame) {
        user->Statistics.receive_valid_frame_counter++;
    }
    if (MSTP_Port->ReceivedInvalidFrame) {
        user->Statistics.receive_invalid_frame_counter++;
    }
    if (MSTP_Port->receive_state == MSTP_RECEIVE_STATE_IDLE) {
        /* only node state machines while rx is idle */
        if (MSTP_Port->SlaveNodeEnabled) {
            MSTP_Slave_Node_FSM(MSTP_Port);
        } else if ((MSTP_Port->This_Station <= DEFAULT_MAX_MASTER) ||
            MSTP_Port->ZeroConfigEnabled) {
            while (MSTP_Master_Node_FSM(MSTP_Port)) {
                /* do nothing while some states fast transition */
            };
        }
    }
    /* see if there is a packet available */
    if (user->ReceivePacketPending) {
        user->ReceivePacketPending = false;
        user->Statistics.receive_pdu_counter++;
        pdu_len = MSTP_Port->DataLength;
        if (pdu_len > max_pdu) {
            /* PDU is too large */
            return 0;
        }
        if (!pdu) {
            /* no place to put a PDU */
            return 0;
        }
        /* copy input buffer to PDU */
        for (i = 0; i < pdu_len; i++) {
            pdu[i] = MSTP_Port->InputBuffer[i];
        }
        if (!src) {
            /* no place to put a source address */
            return 0;
        }
        /* copy source address */
        src->len = 0;
        src->net = 0;
        src->mac_len = 1;
        src->mac[0] = MSTP_Port->SourceAddress;
    }

    return pdu_len;
}

/**
 * @brief fill a BACNET_ADDRESS with the MSTP MAC address
 * @param src - a #BACNET_ADDRESS structure
 * @param mstp_address - a BACnet MSTP address
 */
void dlmstp_fill_bacnet_address(BACNET_ADDRESS *src, uint8_t mstp_address)
{
    int i = 0;

    if (mstp_address == MSTP_BROADCAST_ADDRESS) {
        /* mac_len = 0 if broadcast address */
        src->mac_len = 0;
        src->mac[0] = 0;
    } else {
        src->mac_len = 1;
        src->mac[0] = mstp_address;
    }
    /* fill with 0's starting with index 1; index 0 filled above */
    for (i = 1; i < MAX_MAC_LEN; i++) {
        src->mac[i] = 0;
    }
    src->net = 0;
    src->len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        src->adr[i] = 0;
    }
}

/**
 * @brief Set the MSTP MAC address
 * @param mac_address - MAC address to set
 */
void dlmstp_set_mac_address(uint8_t mac_address)
{
    if (MSTP_Port) {
        MSTP_Port->This_Station = mac_address;
    }

    return;
}

/**
 * @brief Get the MSTP MAC address
 * @return MSTP MAC address
 */
uint8_t dlmstp_mac_address(void)
{
    uint8_t value = 0;

    if (MSTP_Port) {
        value = MSTP_Port->This_Station;
    }

    return value;
}

/**
 * @brief Set the Max_Info_Frames parameter value
 *
 * @note This parameter represents the value of the Max_Info_Frames property
 *  of the node's Device object. The value of Max_Info_Frames specifies the
 *  maximum number of information frames the node may send before it must
 *  pass the token. Max_Info_Frames may have different values on different
 *  nodes. This may be used to allocate more or less of the available link
 *  bandwidth to particular nodes. If Max_Info_Frames is not writable in a
 *  node, its value shall be 1.
 *
 * @param max_info_frames - parameter value to set
 */
void dlmstp_set_max_info_frames(uint8_t max_info_frames)
{
    if (max_info_frames >= 1) {
        if (MSTP_Port) {
            MSTP_Port->Nmax_info_frames = max_info_frames;
        }
    }

    return;
}

/**
 * @brief Get the MSTP max-info-frames value
 * @return the MSTP max-info-frames value
 */
uint8_t dlmstp_max_info_frames(void)
{
    uint8_t value = 0;

    if (MSTP_Port) {
        value = MSTP_Port->Nmax_info_frames;
    }

    return value;
}

/**
 * @brief Set the Max_Master property value for this MSTP datalink
 *
 * @note This parameter represents the value of the Max_Master property of
 *  the node's Device object. The value of Max_Master specifies the highest
 *  allowable address for master nodes. The value of Max_Master shall be
 *  less than or equal to 127. If Max_Master is not writable in a node,
 *  its value shall be 127.
 *
 * @param max_master - value to be set
 */
void dlmstp_set_max_master(uint8_t max_master)
{
    if (max_master <= 127) {
        if (MSTP_Port->This_Station <= max_master) {
            MSTP_Port->Nmax_master = max_master;
        }
    }

    return;
}

/**
 * @brief Get the largest peer MAC address that we will seek
 * @return largest peer MAC address
 */
uint8_t dlmstp_max_master(void)
{
    uint8_t value = 0;

    if (MSTP_Port) {
        value = MSTP_Port->Nmax_master;
    }

    return value;
}

/**
 * @brief Initialize the data link broadcast address
 * @param my_address - address to be filled with unicast designator
 */
void dlmstp_get_my_address(BACNET_ADDRESS *my_address)
{
    int i = 0; /* counter */

    my_address->mac_len = 1;
    if (MSTP_Port) {
        my_address->mac[0] = MSTP_Port->This_Station;
    }
    my_address->net = 0; /* local only, no routing */
    my_address->len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        my_address->adr[i] = 0;
    }

    return;
}

/**
 * @brief Initialize the a data link broadcast address
 * @param dest - address to be filled with broadcast designator
 */
void dlmstp_get_broadcast_address(BACNET_ADDRESS *dest)
{ /* destination address */
    int i = 0; /* counter */

    if (dest) {
        dest->mac_len = 1;
        dest->mac[0] = MSTP_BROADCAST_ADDRESS;
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0; /* always zero when DNET is broadcast */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }

    return;
}

/**
 * @brief Determine if the send PDU queue is empty
 * @return true if the send PDU is empty
 */
bool dlmstp_send_pdu_queue_empty(void)
{
    bool status = false;
    struct dlmstp_user_data_t *user;

    if (MSTP_Port) {
        user = MSTP_Port->UserData;
        if (user) {
            status = Ringbuf_Empty(&user->PDU_Queue);
        }
    }

    return status;
}

/**
 * @brief Determine if the send PDU queue is full
 * @return true if the send PDU is full
 */
bool dlmstp_send_pdu_queue_full(void)
{
    bool status = false;
    struct dlmstp_user_data_t *user;

    if (MSTP_Port) {
        user = MSTP_Port->UserData;
        if (user) {
            status = Ringbuf_Full(&user->PDU_Queue);
        }
    }

    return status;
}

/**
 * @brief Initialize the RS-485 baud rate
 * @param baudrate - RS-485 baud rate in bits per second (bps)
 * @return true if the baud rate was valid
 */
void dlmstp_set_baud_rate(uint32_t baud)
{
    struct dlmstp_user_data_t *user;
    struct dlmstp_rs485_driver *driver;

    if (!MSTP_Port) {
        return;
    }
    if (!MSTP_Port->UserData) {
        return;
    }
    user = MSTP_Port->UserData;
    driver = user->RS485_Driver;
    if (!driver) {
        return;
    }
    if (driver->baud_rate_set(baud)) {
        /* Tframe_abort=60 bit times, not to exceed 100 milliseconds.*/
        if (MSTP_Port->Tframe_abort <= 7) {
            /* within baud range, so auto-calculate range based on baud */
            MSTP_Port->Tframe_abort = 1+((60*1000UL)/baud);
        }
        /* Tturnaround=40 bit times */
        MSTP_Port->Tturnaround_timeout = 1 + ((Tturnaround * 1000) / baud);
    }
}

/**
 * @brief Return the RS-485 baud rate
 * @return baud - RS-485 baud rate in bits per second (bps)
 */
uint32_t dlmstp_baud_rate(void)
{
    struct dlmstp_user_data_t *user;
    struct dlmstp_rs485_driver *driver;

    if (!MSTP_Port) {
        return 0;
    }
    if (!MSTP_Port->UserData) {
        return 0;
    }
    user = MSTP_Port->UserData;
    driver = user->RS485_Driver;
    if (!driver) {
        return 0;
    }

    return driver->baud_rate();
}

/**
 * @brief Copy the MSTP port statistics if they exist
 * @param statistics - MSTP port statistics
 */
void dlmstp_fill_statistics(struct dlmstp_statistics *statistics)
{
    struct dlmstp_user_data_t *user;

    if (!MSTP_Port) {
        return;
    }
    if (!MSTP_Port->UserData) {
        return;
    }
    user = MSTP_Port->UserData;
    if (!user) {
        return;
    }
    if (statistics) {
        *statistics = user->Statistics;
    }
}

/**
 * @brief Get the MSTP port Max-Info-Frames limit
 * @return Max-Info-Frames limit
 */
uint8_t dlmstp_max_info_frames_limit(void)
{
    return DLMSTP_MAX_INFO_FRAMES;
}

/**
 * @brief Get the MSTP port Max-Master limit
 * @return Max-Master limit
 */
uint8_t dlmstp_max_master_limit(void)
{
    return DLMSTP_MAX_MASTER;
}

/**
 * @brief Return the RS-485 silence time in milliseconds
 * @param arg - pointer to MSTP port structure
 * @return silence time in milliseconds
 */
uint32_t dlmstp_silence_milliseconds(void *arg)
{
    uint32_t milliseconds = 0;
    struct mstp_port_struct_t *port = arg;
    struct dlmstp_user_data_t *user = NULL;
    struct dlmstp_rs485_driver *driver = NULL;

    if (port) {
        user = port->UserData;
    }
    if (user) {
        driver = user->RS485_Driver;
    }
    if (driver) {
        milliseconds = driver->silence_milliseconds();
    }

    return milliseconds;
}

/**
 * @brief Reset the RS-485 silence time to zero
 * @param arg - pointer to MSTP port structure
 */
void dlmstp_silence_reset(void *arg)
{
    struct mstp_port_struct_t *port = arg;
    struct dlmstp_user_data_t *user = NULL;
    struct dlmstp_rs485_driver *driver = NULL;

    if (port) {
        user = port->UserData;
    }
    if (user) {
        driver = user->RS485_Driver;
    }
    if (driver) {
        driver->silence_reset();
    }
}

/**
 * @brief Initialize this MS/TP datalink
 * @param ifname user data structure
 * @return true if the MSTP datalink is initialized
 */
bool dlmstp_init(char *ifname)
{
    struct dlmstp_user_data_t *user;
    MSTP_Port = (struct mstp_port_struct_t *)ifname;
    if (MSTP_Port) {
        MSTP_Port->SilenceTimer = dlmstp_silence_milliseconds;
        MSTP_Port->SilenceTimerReset = dlmstp_silence_reset;
        user = (struct dlmstp_user_data_t *)MSTP_Port->UserData;
        if (user && !user->Initialized) {
            Ringbuf_Init(&user->PDU_Queue,
                (volatile uint8_t *)user->PDU_Buffer,
                sizeof(user->PDU_Buffer), DLMSTP_MAX_INFO_FRAMES);
            MSTP_Init(MSTP_Port);
            user->Initialized = true;
        }
    }

    return true;
}
