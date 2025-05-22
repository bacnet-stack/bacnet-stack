/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2006
 * @brief Implementation of the Network Layer using BACnet MS/TP transport
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink Network Layer
 * @ingroup DataLink
 */
#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaddr.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/datalink/dlmstp.h"
/* OS common includes */
#include "bacport.h"
/* port specific */
#include "rs485.h"

/* packet queues */
static DLMSTP_PACKET Receive_Packet;
static HANDLE Receive_Packet_Flag;
/* mechanism to wait for a frame in state machine */
HANDLE Received_Frame_Flag;
static DLMSTP_PACKET Transmit_Packet;
/* local MS/TP port data - shared with RS-485 */
struct mstp_port_struct_t MSTP_Port;
/* buffers needed by mstp port struct */
static uint8_t TxBuffer[DLMSTP_MPDU_MAX];
static uint8_t RxBuffer[DLMSTP_MPDU_MAX];
/* The minimum time without a DataAvailable or ReceiveError event */
/* that a node must wait for a station to begin replying to a */
/* confirmed request: 255 milliseconds. (Implementations may use */
/* larger values for this timeout, not to exceed 300 milliseconds.) */
static uint16_t Treply_timeout = 260;
/* The time without a DataAvailable or ReceiveError event that a node must */
/* wait for a remote node to begin using a token or replying to a Poll For */
/* Master frame: 20 milliseconds. (Implementations may use larger values for */
/* this timeout, not to exceed 35 milliseconds.) */
static uint8_t Tusage_timeout = 30;
/* local timer for tracking silence on the wire */
static struct mstimer Silence_Timer;
/* local timer for tracking the last valid frame on the wire */
static struct mstimer Valid_Frame_Timer;
/* callbacks for monitoring */
static dlmstp_hook_frame_rx_start_cb Preamble_Callback;
static dlmstp_hook_frame_rx_complete_cb Valid_Frame_Rx_Callback;
static dlmstp_hook_frame_rx_complete_cb Invalid_Frame_Rx_Callback;
static DLMSTP_STATISTICS DLMSTP_Statistics;

/**
 * @brief Cleanup the MS/TP datalink
 */
void dlmstp_cleanup(void)
{
    /* nothing to do for static buffers */
    if (Received_Frame_Flag) {
        CloseHandle(Received_Frame_Flag);
    }
    if (Receive_Packet_Flag) {
        CloseHandle(Receive_Packet_Flag);
    }
}

/**
 * @brief send an PDU via MSTP
 * @param dest - BACnet destination address
 * @param npdu_data - network layer information
 * @param pdu - PDU data to send
 * @param pdu_len - number of bytes of PDU data to send
 * @return number of bytes sent on success, zero on failure
 */
int dlmstp_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    int bytes_sent = 0;
    unsigned i = 0;

    if (!Transmit_Packet.ready) {
        if (npdu_data->data_expecting_reply) {
            Transmit_Packet.frame_type = FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY;
        } else {
            Transmit_Packet.frame_type =
                FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY;
        }
        Transmit_Packet.pdu_len = (uint16_t)pdu_len;
        for (i = 0; i < pdu_len; i++) {
            Transmit_Packet.pdu[i] = pdu[i];
        }
        bacnet_address_copy(&Transmit_Packet.address, dest);
        bytes_sent = pdu_len + DLMSTP_HEADER_MAX;
        Transmit_Packet.ready = true;
    }

    return bytes_sent;
}

/**
 * @brief The MS/TP state machine uses this function for getting data to send
 * @param mstp_port - specific MSTP port that is used for this datalink
 * @param timeout - number of milliseconds to wait for the data
 * @return amount of PDU data
 */
uint16_t MSTP_Get_Send(struct mstp_port_struct_t *mstp_port, unsigned timeout)
{ /* milliseconds to wait for a packet */
    uint16_t pdu_len = 0;
    uint8_t destination = 0; /* destination address */

    (void)timeout;
    if (!Transmit_Packet.ready) {
        return 0;
    }
    /* load destination MAC address */
    if (Transmit_Packet.address.mac_len) {
        destination = Transmit_Packet.address.mac[0];
    } else {
        destination = MSTP_BROADCAST_ADDRESS;
    }
    if ((DLMSTP_HEADER_MAX + Transmit_Packet.pdu_len) > DLMSTP_MPDU_MAX) {
        return 0;
    }
    /* convert the PDU into the MSTP Frame */
    pdu_len = MSTP_Create_Frame(
        &mstp_port->OutputBuffer[0], /* <-- loading this */
        mstp_port->OutputBufferSize, Transmit_Packet.frame_type, destination,
        mstp_port->This_Station, &Transmit_Packet.pdu[0],
        Transmit_Packet.pdu_len);
    DLMSTP_Statistics.transmit_pdu_counter++;
    Transmit_Packet.ready = false;

    return pdu_len;
}

/**
 * @brief Determine if the reply packet is the data expected
 * @param request_pdu - PDU of the data
 * @param request_pdu_len - number of bytes of PDU data
 * @param src_address - source address of the request
 * @param reply_pdu - PDU of the data
 * @param reply_pdu_len - number of bytes of PDU data
 * @param dest_address - the destination address for this data
 * @return true if the reply packet is the data expected
 */
static bool dlmstp_compare_data_expecting_reply(
    const uint8_t *request_pdu,
    uint16_t request_pdu_len,
    uint8_t src_address,
    const uint8_t *reply_pdu,
    uint16_t reply_pdu_len,
    const BACNET_ADDRESS *dest_address)
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

    /* unused parameters */
    request_pdu_len = request_pdu_len;
    reply_pdu_len = reply_pdu_len;
    /* decode the request data */
    request.address.mac[0] = src_address;
    request.address.mac_len = 1;
    offset = (uint16_t)bacnet_npdu_decode(
        request_pdu, request_pdu_len, NULL, &request.address,
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
    if (request_pdu[offset] & BIT(3)) {
        request.service_choice = request_pdu[offset + 5];
    } else {
        request.service_choice = request_pdu[offset + 3];
    }
    /* decode the reply data */
    bacnet_address_copy(&reply.address, dest_address);
    offset = (uint16_t)bacnet_npdu_decode(
        reply_pdu, reply_pdu_len, &reply.address, NULL, &reply.npdu_data);
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
            if (reply_pdu[offset] & BIT(3)) {
                reply.service_choice = reply_pdu[offset + 4];
            } else {
                reply.service_choice = reply_pdu[offset + 2];
            }
            break;
        case PDU_TYPE_ERROR:
            reply.invoke_id = reply_pdu[offset + 1];
            reply.service_choice = reply_pdu[offset + 2];
            break;
        case PDU_TYPE_REJECT:
        case PDU_TYPE_ABORT:
        case PDU_TYPE_SEGMENT_ACK:
            reply.invoke_id = reply_pdu[offset + 1];
            break;
        default:
            return false;
    }
    /* these don't have service choice included */
    if ((reply.pdu_type == PDU_TYPE_REJECT) ||
        (reply.pdu_type == PDU_TYPE_ABORT) ||
        (reply.pdu_type == PDU_TYPE_SEGMENT_ACK)) {
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
uint16_t MSTP_Get_Reply(struct mstp_port_struct_t *mstp_port, unsigned timeout)
{
    uint16_t pdu_len = 0;
    uint8_t destination = 0;
    bool matched = false;

    (void)timeout;
    if (!Transmit_Packet.ready) {
        return 0;
    }
    /* load destination MAC address */
    if (Transmit_Packet.address.mac_len == 1) {
        destination = Transmit_Packet.address.mac[0];
    } else {
        return 0;
    }
    if ((DLMSTP_HEADER_MAX + Transmit_Packet.pdu_len) > DLMSTP_MPDU_MAX) {
        return 0;
    }
    /* is this the reply to the DER? */
    matched = dlmstp_compare_data_expecting_reply(
        &mstp_port->InputBuffer[0], mstp_port->DataLength,
        mstp_port->SourceAddress, &Transmit_Packet.pdu[0],
        Transmit_Packet.pdu_len, &Transmit_Packet.address);
    if (!matched) {
        return 0;
    }
    /* convert the PDU into the MSTP Frame */
    pdu_len = MSTP_Create_Frame(
        &mstp_port->OutputBuffer[0], /* <-- loading this */
        mstp_port->OutputBufferSize, Transmit_Packet.frame_type, destination,
        mstp_port->This_Station, &Transmit_Packet.pdu[0],
        Transmit_Packet.pdu_len);
    DLMSTP_Statistics.transmit_pdu_counter++;
    Transmit_Packet.ready = false;

    return pdu_len;
}

/**
 * @brief Send an MSTP frame
 * @param mstp_port - port specific data
 * @param buffer - data to send
 * @param nbytes - number of bytes of data to send
 */
void MSTP_Send_Frame(
    struct mstp_port_struct_t *mstp_port,
    const uint8_t *buffer,
    uint16_t nbytes)
{
    RS485_Send_Frame(mstp_port, buffer, nbytes);
    DLMSTP_Statistics.transmit_frame_counter++;

}

/**
 * @brief MS/TP state machine received a frame
 * @return number of bytes queued, or 0 if unable to be queued
 */
uint16_t MSTP_Put_Receive(struct mstp_port_struct_t *mstp_port)
{
    uint16_t pdu_len = 0;
    BOOL rc;

    if (!Receive_Packet.ready) {
        /* bounds check - maybe this should send an abort? */
        pdu_len = mstp_port->DataLength;
        if (pdu_len > sizeof(Receive_Packet.pdu)) {
            pdu_len = sizeof(Receive_Packet.pdu);
        }
        memmove(
            (void *)&Receive_Packet.pdu[0], (void *)&mstp_port->InputBuffer[0],
            pdu_len);
        dlmstp_fill_bacnet_address(
            &Receive_Packet.address, mstp_port->SourceAddress);
        Receive_Packet.pdu_len = mstp_port->DataLength;
        Receive_Packet.ready = true;
        rc = ReleaseSemaphore(Receive_Packet_Flag, 1, NULL);
        (void)rc;
    }

    return pdu_len;
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
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* PDU data */
    uint16_t max_pdu, /* amount of space available in the PDU  */
    unsigned timeout)
{ /* milliseconds to wait for a packet */
    uint16_t pdu_len = 0;
    DWORD wait_status = 0;

    (void)max_pdu;
    /* see if there is a packet available, and a place
       to put the reply (if necessary) and process it */
    wait_status = WaitForSingleObject(Receive_Packet_Flag, timeout);
    if (wait_status == WAIT_OBJECT_0) {
        if (Receive_Packet.ready) {
            if (Receive_Packet.pdu_len) {
                DLMSTP_Statistics.receive_pdu_counter++;
                if (src) {
                    memmove(
                        src, &Receive_Packet.address,
                        sizeof(Receive_Packet.address));
                }
                if (pdu) {
                    memmove(
                        pdu, &Receive_Packet.pdu, sizeof(Receive_Packet.pdu));
                }
                pdu_len = Receive_Packet.pdu_len;
            }
            Receive_Packet.ready = false;
        }
    }

    return pdu_len;
}

static void dlmstp_receive_fsm_task(void *pArg)
{
    (void)pArg;
    (void)SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    for (;;) {
        /* only do receive state machine while we don't have a frame */
        if ((MSTP_Port.ReceivedValidFrame == false) &&
            (MSTP_Port.ReceivedInvalidFrame == false)) {
            do {
                RS485_Check_UART_Data(&MSTP_Port);
                MSTP_Receive_Frame_FSM(&MSTP_Port);
                if (MSTP_Port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE) {
                    if (Preamble_Callback) {
                        Preamble_Callback();
                    }
                }
                if (MSTP_Port.ReceivedValidFrame) {
                    DLMSTP_Statistics.receive_valid_frame_counter++;
                    if (Valid_Frame_Rx_Callback) {
                        Valid_Frame_Rx_Callback(
                            MSTP_Port.SourceAddress,
                            MSTP_Port.DestinationAddress,
                            MSTP_Port.FrameType, MSTP_Port.InputBuffer,
                            MSTP_Port.DataLength);
                    }
                    ReleaseSemaphore(Received_Frame_Flag, 1, NULL);
                    break;
                } else if (MSTP_Port.ReceivedInvalidFrame) {
                    if (Invalid_Frame_Rx_Callback) {
                        DLMSTP_Statistics.receive_invalid_frame_counter++;
                        Invalid_Frame_Rx_Callback(
                            MSTP_Port.SourceAddress,
                            MSTP_Port.DestinationAddress,
                            MSTP_Port.FrameType, MSTP_Port.InputBuffer,
                            MSTP_Port.DataLength);
                    }
                    ReleaseSemaphore(Received_Frame_Flag, 1, NULL);
                    break;
                }
            } while (MSTP_Port.DataAvailable);
        }
    }
}

static void dlmstp_master_fsm_task(void *pArg)
{
    DWORD dwMilliseconds = 0;
    MSTP_MASTER_STATE master_state;

    (void)pArg;
    (void)SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    for (;;) {
        switch (MSTP_Port.master_state) {
            case MSTP_MASTER_STATE_IDLE:
                dwMilliseconds = Tno_token;
                break;
            case MSTP_MASTER_STATE_WAIT_FOR_REPLY:
                dwMilliseconds = Treply_timeout;
                break;
            case MSTP_MASTER_STATE_POLL_FOR_MASTER:
                dwMilliseconds = Tusage_timeout;
                break;
            default:
                dwMilliseconds = 0;
                break;
        }
        if (dwMilliseconds) {
            WaitForSingleObject(Received_Frame_Flag, dwMilliseconds);
        }
        if (MSTP_Port.SlaveNodeEnabled) {
            MSTP_Slave_Node_FSM(&MSTP_Port);
        } else if (
            (MSTP_Port.This_Station <= DEFAULT_MAX_MASTER) ||
            MSTP_Port.ZeroConfigEnabled || MSTP_Port.CheckAutoBaud) {
            master_state = MSTP_Port.master_state;
            while (MSTP_Master_Node_FSM(&MSTP_Port)) {
                if (master_state != MSTP_Port.master_state) {
                    /* state changed while some states fast transition */
                    if (MSTP_Port.master_state == MSTP_MASTER_STATE_NO_TOKEN) {
                        DLMSTP_Statistics.lost_token_counter++;
                    }
                    master_state = MSTP_Port.master_state;
                }
            }
        }
    }
}

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
    MSTP_Port.This_Station = mac_address;
}

/**
 * @brief Get the MSTP MAC address
 * @return MSTP MAC address
 */
uint8_t dlmstp_mac_address(void)
{
    return MSTP_Port.This_Station;
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
        MSTP_Port.Nmax_info_frames = max_info_frames;
    }

    return;
}

/**
 * @brief Get the MSTP max-info-frames value
 * @return the MSTP max-info-frames value
 */
uint8_t dlmstp_max_info_frames(void)
{
    return MSTP_Port.Nmax_info_frames;
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
        MSTP_Port.Nmax_master = max_master;
    }

    return;
}

/**
 * @brief Get the largest peer MAC address that we will seek
 * @return largest peer MAC address
 */
uint8_t dlmstp_max_master(void)
{
    return MSTP_Port.Nmax_master;
}

/**
 * @brief Initialize the data link broadcast address
 * @param my_address - address to be filled with unicast designator
 */
void dlmstp_get_my_address(BACNET_ADDRESS *my_address)
{
    int i = 0; /* counter */

    my_address->mac_len = 1;
    my_address->mac[0] = MSTP_Port.This_Station;
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
 * @brief Get the MSTP port SoleMaster status
 * @return true if the MSTP port is the SoleMaster
 */
bool dlmstp_sole_master(void)
{
    return MSTP_Port.SoleMaster;
}

/**
 * @brief Get the MSTP port SlaveNodeEnabled status
 * @return true if the MSTP port has SlaveNodeEnabled
 */
bool dlmstp_slave_mode_enabled(void)
{
    return MSTP_Port.SlaveNodeEnabled;
}

/**
 * @brief Set the MSTP port SlaveNodeEnabled flag
 * @param flag - true if the MSTP port has SlaveNodeEnabled
 * @return true if the MSTP port SlaveNodeEnabled was set
 * @note This flag is used to enable the Slave Node state machine
 * for the MSTP port.  The Slave Node state machine is used to
 * respond to requests from the Master Node.
 */
bool dlmstp_slave_mode_enabled_set(bool flag)
{
    MSTP_Port.SlaveNodeEnabled = flag;

    return true;
}

/**
 * @brief Get the MSTP port ZeroConfigEnabled status
 * @return true if the MSTP port has ZeroConfigEnabled
 */
bool dlmstp_zero_config_enabled(void)
{
    return MSTP_Port.ZeroConfigEnabled;
}

/**
 * @brief Set the MSTP port ZeroConfigEnabled flag
 * @param flag - true if the MSTP port has ZeroConfigEnabled
 * @return true if the MSTP port ZeroConfigEnabled was set
 * @note This flag is used to enable the Zero Configuration state machine
 * for the MSTP port.  The Zero Configuration state machine is used to
 * automatically assign a MAC address to the MSTP port.
 */
bool dlmstp_zero_config_enabled_set(bool flag)
{
    MSTP_Port.ZeroConfigEnabled = flag;

    return true;
}

/**
 * @brief Get the MSTP port AutoBaudEnabled status
 * @return true if the MSTP port has AutoBaudEnabled
 */
bool dlmstp_check_auto_baud(void)
{
    return MSTP_Port.CheckAutoBaud;
}

/**
 * @brief Set the MSTP port AutoBaudEnabled flag
 * @param flag - true if the MSTP port has AutoBaudEnabled
 * @return true if the MSTP port AutoBaudEnabled was set
 * @note This flag is used to enable the Zero Configuration state machine
 * for the MSTP port.  The Zero Configuration state machine is used to
 * automatically assign a MAC address to the MSTP port.
 */
bool dlmstp_check_auto_baud_set(bool flag)
{
    MSTP_Port.CheckAutoBaud = flag;
    if (flag) {
        MSTP_Port.Auto_Baud_State = MSTP_AUTO_BAUD_STATE_INIT;
    }

    return true;
}

/**
 * @brief Get the MSTP port MAC address that this node prefers to use.
 * @return ZeroConfigStation value, or an out-of-range value if invalid
 * @note valid values are between Nmin_poll_station and Nmax_poll_station
 *  but other values such as 0 or 255 could mean 'unconfigured'
 */
uint8_t dlmstp_zero_config_preferred_station(void)
{
    return MSTP_Port.Zero_Config_Preferred_Station;
}

/**
 * @brief Set the MSTP port MAC address that this node prefers to use.
 * @param station - Zero_Config_Preferred_Station value
 * @return true if the MSTP port Zero_Config_Preferred_Station was set
 * @note valid values are between Nmin_poll_station and Nmax_poll_station
 *  but other values such as 0 or 255 could mean 'unconfigured'
 */
bool dlmstp_zero_config_preferred_station_set(uint8_t station)
{
    MSTP_Port.Zero_Config_Preferred_Station = station;

    return true;
}

/**
 * @brief Initialize the RS-485 baud rate
 * @param baudrate - RS-485 baud rate in bits per second (bps)
 * @return true if the baud rate was valid
 */
void dlmstp_set_baud_rate(uint32_t baud)
{
    RS485_Set_Baud_Rate(baud);
}

/**
 * @brief Return the RS-485 baud rate
 * @return baud - RS-485 baud rate in bits per second (bps)
 */
uint32_t dlmstp_baud_rate(void)
{
    return RS485_Get_Baud_Rate();
}

/**
 * @brief Set the MS/TP Frame Complete callback
 * @param cb_func - callback function to be called when a frame is received
 */
void dlmstp_set_frame_rx_complete_callback(
    dlmstp_hook_frame_rx_complete_cb cb_func)
{
    Valid_Frame_Rx_Callback = cb_func;
}

/**
 * @brief Set the MS/TP Frame Complete callback
 * @param cb_func - callback function to be called when a frame is received
 */
void dlmstp_set_invalid_frame_rx_complete_callback(
    dlmstp_hook_frame_rx_complete_cb cb_func)
{
    Invalid_Frame_Rx_Callback = cb_func;
}

/**
 * @brief Set the MS/TP Preamble callback
 * @param cb_func - callback function to be called when a preamble is received
 */
void dlmstp_set_frame_rx_start_callback(dlmstp_hook_frame_rx_start_cb cb_func)
{
    Preamble_Callback = cb_func;
}

/**
 * @brief Reset the MS/TP statistics
 */
void dlmstp_reset_statistics(void)
{
    memset(&DLMSTP_Statistics, 0, sizeof(struct dlmstp_statistics));
}

/**
 * @brief Copy the MSTP port statistics if they exist
 * @param statistics - MSTP port statistics
 */
void dlmstp_fill_statistics(struct dlmstp_statistics *statistics)
{
    memmove(&DLMSTP_Statistics, statistics, sizeof(struct dlmstp_statistics));
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
    (void)arg;
    return mstimer_elapsed(&Silence_Timer);
}

/**
 * @brief Return the valid frame time in milliseconds
 * @param arg - pointer to MSTP port structure
 * @return valid frame time in milliseconds
 */
uint32_t dlmstp_valid_frame_milliseconds(void *arg)
{
    (void)arg;
    return mstimer_elapsed(&Valid_Frame_Timer);
}

/**
 * @brief Reset the valid frame timer
 * @param arg - pointer to MSTP port structure
 * @return valid frame time in milliseconds
 */
void dlmstp_valid_frame_milliseconds_reset(void *arg)
{
    (void)arg;
    mstimer_restart(&Valid_Frame_Timer);
}

/**
 * @brief Reset the RS-485 silence time to zero
 * @param arg - pointer to MSTP port structure
 */
void dlmstp_silence_reset(void *arg)
{
    (void)arg;
    mstimer_set(&Silence_Timer, 0);
}

/**
 * @brief Initialize this MS/TP datalink
 * @param ifname user data structure
 * @return true if the MSTP datalink is initialized
 */
bool dlmstp_init(char *ifname)
{
    unsigned long hThread = 0;
    uint32_t arg_value = 0;

    /* initialize packet queue */
    Receive_Packet.ready = false;
    Receive_Packet.pdu_len = 0;
    Receive_Packet_Flag = CreateSemaphore(NULL, 0, 1, "dlmstpReceivePacket");
    if (Receive_Packet_Flag == NULL) {
        exit(1);
    }
    Received_Frame_Flag = CreateSemaphore(NULL, 0, 1, "dlsmtpReceiveFrame");
    if (Received_Frame_Flag == NULL) {
        CloseHandle(Receive_Packet_Flag);
        exit(1);
    }
    /* initialize hardware */
    mstimer_set(&Silence_Timer, 0);
    if (ifname) {
        RS485_Set_Interface(ifname);
    }
    RS485_Initialize();
    MSTP_Port.InputBuffer = &RxBuffer[0];
    MSTP_Port.InputBufferSize = sizeof(RxBuffer);
    MSTP_Port.OutputBuffer = &TxBuffer[0];
    MSTP_Port.OutputBufferSize = sizeof(TxBuffer);
    MSTP_Port.SilenceTimer = dlmstp_silence_milliseconds;
    MSTP_Port.SilenceTimerReset = dlmstp_silence_reset;
    MSTP_Port.ValidFrameTimer = dlmstp_valid_frame_milliseconds;
    MSTP_Port.ValidFrameTimerReset = dlmstp_valid_frame_milliseconds_reset;
    MSTP_Port.BaudRate = dlmstp_baud_rate;
    MSTP_Port.BaudRateSet = dlmstp_set_baud_rate;
    MSTP_Init(&MSTP_Port);
#if 0
    uint8_t data;

    /* FIXME: implement your data storage */
    data = 64;  /* I2C_Read_Byte(
                   EEPROM_DEVICE_ADDRESS,
                   EEPROM_MSTP_MAC_ADDR); */
    if (data <= 127)
        MSTP_Port.This_Station = data;
    else
        dlmstp_set_my_address(DEFAULT_MAC_ADDRESS);
    /* FIXME: implement your data storage */
    data = 127; /* I2C_Read_Byte(
                   EEPROM_DEVICE_ADDRESS,
                   EEPROM_MSTP_MAX_MASTER_ADDR); */
    if ((data <= 127) && (data >= MSTP_Port.This_Station))
        MSTP_Port.Nmax_master = data;
    else
        dlmstp_set_max_master(DEFAULT_MAX_MASTER);
    /* FIXME: implement your data storage */
    data = 1;
    /* I2C_Read_Byte(
       EEPROM_DEVICE_ADDRESS,
       EEPROM_MSTP_MAX_INFO_FRAMES_ADDR); */
    if (data >= 1)
        MSTP_Port.Nmax_info_frames = data;
    else
        dlmstp_set_max_info_frames(DEFAULT_MAX_INFO_FRAMES);
#endif
#if PRINT_ENABLED
    fprintf(stderr, "MS/TP MAC: %02X\n", MSTP_Port.This_Station);
    fprintf(stderr, "MS/TP Max_Master: %02X\n", MSTP_Port.Nmax_master);
    fprintf(
        stderr, "MS/TP Max_Info_Frames: %u\n",
        (unsigned)MSTP_Port.Nmax_info_frames);
#endif
    hThread = _beginthread(dlmstp_receive_fsm_task, 4096, &arg_value);
    if (hThread == 0) {
        fprintf(stderr, "Failed to start recive FSM task\n");
    }
    hThread = _beginthread(dlmstp_master_fsm_task, 4096, &arg_value);
    if (hThread == 0) {
        fprintf(stderr, "Failed to start Master Node FSM task\n");
    }

    return true;
}

#ifdef TEST_DLMSTP
#include <stdio.h>

void apdu_handler(
    BACNET_ADDRESS *src, /* source address */
    uint8_t *apdu, /* APDU data */
    uint16_t pdu_len)
{ /* for confirmed messages */
    (void)src;
    (void)apdu;
    (void)pdu_len;
}

/* returns a delta timestamp */
uint32_t timestamp_ms(void)
{
    DWORD ticks = 0, delta_ticks = 0;
    static DWORD last_ticks = 0;

    ticks = GetTickCount();
    delta_ticks =
        (ticks >= last_ticks ? ticks - last_ticks : MAXDWORD - last_ticks);
    last_ticks = ticks;

    return delta_ticks;
}

static char *Network_Interface = NULL;

int main(int argc, char *argv[])
{
    uint16_t pdu_len = 0;

    /* argv has the "COM4" or some other device */
    if (argc > 1) {
        Network_Interface = argv[1];
    }
    dlmstp_set_baud_rate(38400);
    dlmstp_set_mac_address(0x05);
    dlmstp_set_max_info_frames(DEFAULT_MAX_INFO_FRAMES);
    dlmstp_set_max_master(DEFAULT_MAX_MASTER);
    dlmstp_init(Network_Interface);
    /* forever task */
    for (;;) {
        pdu_len = dlmstp_receive(NULL, NULL, 0, INFINITE);
#if 0
        MSTP_Create_And_Send_Frame(&MSTP_Port, FRAME_TYPE_TEST_REQUEST,
            MSTP_Port.SourceAddress, MSTP_Port.This_Station, NULL, 0);
#endif
    }

    return 0;
}
#endif
