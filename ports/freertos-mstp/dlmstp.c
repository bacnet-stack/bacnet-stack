/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief RS-485 Interface
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <FreeRTOS.h>
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/crc.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstpdef.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bits.h"
#include "bacnet/bytes.h"
#include "bacnet/bacaddr.h"
#include "rs485.h"
#include "bacnet.h"

/**
 * Initialize this MS/TP datalink
 */
bool dlmstp_init(volatile struct mstp_port_struct_t *mstp_port)
{
    MSTP_Init(mstp_port);

    return true;
}

/**
 * @brief send an PDU via MSTP
 * @param dest - BACnet destination address
 * @param npdu_data - network layer information
 * @param pdu - PDU data to send
 * @param pdu_len - number of bytes of PDU data to send
 * @return number of bytes sent on success, zero on failure
 */
int dlmstp_send_pdu(volatile struct mstp_port_struct_t *mstp_port,
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    int bytes_sent = 0;
    unsigned i = 0; /* loop counter */
    struct dlmstp_packet_t *pkt;
    RING_BUFFER *pdu_queue;
    struct bacnet_port_data *port;

    port = MSTP_Port.UserData;
    pdu_queue = (RING_BUFFER *)&port->PDU_Queue;
    xSemaphoreTake(port->PDU_Mutex, portMAX_DELAY);
    pkt = (struct dlmstp_packet_t *)(void *)Ringbuf_Data_Peek(pdu_queue);
    if (pkt && (pdu_len <= MAX_MPDU)) {
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
        if (Ringbuf_Data_Put(pdu_queue, (uint8_t *)pkt)) {
            bytes_sent = pdu_len;
        }
    }
    xSemaphoreGive(port->PDU_Mutex);

    return bytes_sent;
}

/* for the MS/TP state machine to use for getting data to send */
/* Return: amount of PDU data */
uint16_t MSTP_Get_Send(
    volatile struct mstp_port_struct_t *mstp_port, unsigned timeout)
{
    uint16_t pdu_len = 0;
    struct dlmstp_packet_t *pkt;
    RING_BUFFER *pdu_queue;
    struct bacnet_port_data *port;

    (void)timeout;
    port = MSTP_Port.UserData;
    pdu_queue = (RING_BUFFER *)&port->PDU_Queue;
    xSemaphoreTake(port->PDU_Mutex, portMAX_DELAY);
    if (Ringbuf_Empty(pdu_queue)) {
        xSemaphoreGive(port->PDU_Mutex);
        return 0;
    }
    /* look at next PDU in queue without removing it */
    pkt = (struct dlmstp_packet_t *)(void *)Ringbuf_Peek(pdu_queue);
    /* convert the PDU into the MSTP Frame */
    pdu_len = MSTP_Create_Frame(&mstp_port->OutputBuffer[0],
        mstp_port->OutputBufferSize, pkt->frame_type, pkt->address.mac[0],
        mstp_port->This_Station, &pkt->pdu[0], pkt->pdu_len);
    mstp_port->TransmitPDUCount++;
    (void)Ringbuf_Pop(pdu_queue, NULL);
    xSemaphoreGive(port->PDU_Mutex);

    return pdu_len;
}

/* for the MS/TP state machine to use for getting data to send */
/* as the reply to a DATA_EXPECTING_REPLY frame, or nothing */
uint16_t MSTP_Get_Reply(
    volatile struct mstp_port_struct_t *mstp_port, unsigned timeout)
{                           /* milliseconds to wait for a packet */
    uint16_t frame_len = 0; /* return value */
    BACNET_ADDRESS *dest = NULL;
    uint8_t frame_type = 0; /* destination address */
    uint8_t *pdu = NULL;
    bool matched = false;
    uint16_t pdu_len = 0;
    struct dlmstp_packet_t *pkt;
    RING_BUFFER *pdu_queue;
    struct bacnet_port_data *port;

    (void)timeout;
    port = MSTP_Port.UserData;
    pdu_queue = (RING_BUFFER *)&port->PDU_Queue;
    xSemaphoreTake(port->PDU_Mutex, portMAX_DELAY);
    if (Ringbuf_Empty(pdu_queue)) {
        xSemaphoreGive(port->PDU_Mutex);
        return 0;
    }
    /* look at next PDU in queue without removing it */
    pkt = (struct dlmstp_packet_t *)(void *)Ringbuf_Peek(pdu_queue);
    /* is this the reply to the DER? */
    dest = &pkt->address;
    pdu = (uint8_t *)&pkt->pdu[0];
    pdu_len = pkt->pdu_len;
    matched = MSTP_Compare_Data_Expecting_Reply(mstp_port, pdu, pdu_len, dest);
    if (!matched) {
        xSemaphoreGive(port->PDU_Mutex);
        return 0;
    }
    /* convert the PDU into the MSTP Frame */
    frame_type = pkt->frame_type;
    frame_len = MSTP_Create_Frame(&mstp_port->OutputBuffer[0],
        mstp_port->OutputBufferSize, frame_type, dest->mac[0],
        mstp_port->This_Station, pdu, pdu_len);
    mstp_port->TransmitPDUCount++;
    (void)Ringbuf_Pop(pdu_queue, NULL);
    xSemaphoreGive(port->PDU_Mutex);

    return frame_len;
}

/* for the MS/TP state machine to use for sending a frame */
void MSTP_Send_Frame(
    volatile struct mstp_port_struct_t *mstp_port)
{
    uint32_t milliseconds;
    bool sent;
    struct bacnet_port_data *port;
    struct rs485_driver *driver;

    port = MSTP_Port.UserData;
    driver = port->RS485_Driver;
    milliseconds = driver->silence_milliseconds();
    if (milliseconds < mstp_port->TurnaroundTime) {
        bacnet_task_delay_milliseconds(
            (unsigned)mstp_port->TurnaroundTime - milliseconds);
    }
    sent = driver->send(mstp_port->OutputBuffer, mstp_port->OutputBufferLength);
    if (sent) {
        mstp_port->TransmitFrameCount++;
        mstp_port->OutputBufferLength = 0;
    }
}

/**
 * @brief Baud rate determines turnaround time.
 * The minimum time after the end of the stop bit of the final octet of a
 * received frame before a node may enable its EIA-485 driver: 40 bit times.
 * At 9600 baud, 40 bit times would be about 4.166 milliseconds
 * At 19200 baud, 40 bit times would be about 2.083 milliseconds
 * At 38400 baud, 40 bit times would be about 1.041 milliseconds
 * At 57600 baud, 40 bit times would be about 0.694 milliseconds
 * At 76800 baud, 40 bit times would be about 0.520 milliseconds
 * At 115200 baud, 40 bit times would be about 0.347 milliseconds
 * 40 bits is 4 octets including a start and stop bit with each octet
 * @return: amount of milliseconds to wait
 */
static uint16_t rs485_turnaround_time(
    volatile struct mstp_port_struct_t *mstp_port)
{
    struct bacnet_port_data *port;
    struct rs485_driver *driver;

    port = MSTP_Port.UserData;
    driver = port->RS485_Driver;
    /* delay after reception before transmitting - per MS/TP spec */
    /* wait a minimum  40 bit times since reception */
    /* at least 2 ms for errors: rounding, clock tick */
    return (2 + ((Tturnaround * 1000) / driver->baud_rate()));
}

/**
 * @brief Run the MS/TP state machines, and get packet if available
 * @param pdu - place to put PDU data for the caller
 * @param max_pdu - number of bytes of PDU data that caller can receive
 * @return number of bytes in received packet, or 0 if no packet was received
 */
uint16_t dlmstp_receive(volatile struct mstp_port_struct_t *mstp_port,
    BACNET_ADDRESS *src,
    uint8_t *pdu,
    uint16_t max_pdu,
    unsigned timeout)
{
    uint16_t pdu_len = 0;
    uint8_t data_register = 0;

    (void)timeout;
    /* set the input buffer to the same data storage for zero copy */
    if (!mstp_port->InputBuffer) {
        mstp_port->InputBuffer = pdu;
        mstp_port->InputBufferSize = max_pdu;
    }
    if (mstp_port->RS485_Driver->transmitting()) {
        /* we're transmitting; do nothing else */
        return 0;
    }
    /* only do receive state machine while we don't have a frame */
    while ((mstp_port->ReceivedValidFrame == false)
        && (mstp_port->ReceivedValidFrameNotForUs == false)
        && (mstp_port->ReceivedInvalidFrame == false)) {
        mstp_port->DataAvailable
            = mstp_port->RS485_Driver->read(&data_register);
        if (mstp_port->DataAvailable) {
            mstp_port->DataRegister = data_register;
        }
        MSTP_Receive_Frame_FSM(mstp_port);
        /* process another byte, if available */
        if (!mstp_port->RS485_Driver->read(NULL)) {
            break;
        }
    }
    if (mstp_port->ReceivedValidFrameNotForUs) {
        mstp_port->ReceivedValidFrameNotForUs = false;
        mstp_port->ReceiveFrameCount++;
    }
    if (mstp_port->ReceivedValidFrame) {
        mstp_port->ReceiveFrameCount++;
    }
    if (mstp_port->ReceivedInvalidFrame) {
        mstp_port->ReceiveFrameCount++;
    }
    /* only do master state machine while rx is idle */
    if (mstp_port->receive_state == MSTP_RECEIVE_STATE_IDLE) {
        if (mstp_port->This_Station <= DEFAULT_MAX_MASTER) {
            while (MSTP_Master_Node_FSM(mstp_port)) {
                /* do nothing while some states fast transition */
            };
        }
    }
    /* see if there is a packet available, and a place
       to put the reply (if necessary) and process it */
    if (mstp_port->ReceivePacketPending) {
        mstp_port->ReceivePacketPending = false;
        mstp_port->ReceivePDUCount++;
        pdu_len = mstp_port->DataLength;
        src->len = 0;
        src->net = 0;
        src->mac_len = 1;
        src->mac[0] = mstp_port->SourceAddress;
        /* data is already in the pdu pointer */
    }

    return pdu_len;
}

void dlmstp_fill_bacnet_address(volatile struct mstp_port_struct_t *mstp_port,
    BACNET_ADDRESS *src,
    uint8_t mstp_address)
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

void dlmstp_set_mac_address(
    volatile struct mstp_port_struct_t *mstp_port, uint8_t mac_address)
{
    /* Master Nodes can only have address 0-127 */
    if (mac_address <= 127) {
        mstp_port->This_Station = mac_address;
    }

    return;
}

uint8_t dlmstp_mac_address(volatile struct mstp_port_struct_t *mstp_port)
{
    return mstp_port->This_Station;
}

/* This parameter represents the value of the Max_Info_Frames property of */
/* the node's Device object. The value of Max_Info_Frames specifies the */
/* maximum number of information frames the node may send before it must */
/* pass the token. Max_Info_Frames may have different values on different */
/* nodes. This may be used to allocate more or less of the available link */
/* bandwidth to particular nodes. If Max_Info_Frames is not writable in a */
/* node, its value shall be 1. */
void dlmstp_set_max_info_frames(
    volatile struct mstp_port_struct_t *mstp_port, uint8_t max_info_frames)
{
    if (max_info_frames >= 1) {
        mstp_port->Nmax_info_frames = max_info_frames;
    }

    return;
}

uint8_t dlmstp_max_info_frames(volatile struct mstp_port_struct_t *mstp_port)
{
    return mstp_port->Nmax_info_frames;
}

/* This parameter represents the value of the Max_Master property of the */
/* node's Device object. The value of Max_Master specifies the highest */
/* allowable address for master nodes. The value of Max_Master shall be */
/* less than or equal to 127. If Max_Master is not writable in a node, */
/* its value shall be 127. */
void dlmstp_set_max_master(
    volatile struct mstp_port_struct_t *mstp_port, uint8_t max_master)
{
    if (max_master <= 127) {
        if (mstp_port->This_Station <= max_master) {
            mstp_port->Nmax_master = max_master;
        }
    }

    return;
}

uint8_t dlmstp_max_master(volatile struct mstp_port_struct_t *mstp_port)
{
    return mstp_port->Nmax_master;
}

void dlmstp_get_my_address(
    volatile struct mstp_port_struct_t *mstp_port, BACNET_ADDRESS *my_address)
{
    int i = 0; /* counter */

    my_address->mac_len = 1;
    my_address->mac[0] = mstp_port->This_Station;
    my_address->net = 0; /* local only, no routing */
    my_address->len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        my_address->adr[i] = 0;
    }

    return;
}

void dlmstp_get_broadcast_address(
    volatile struct mstp_port_struct_t *mstp_port, BACNET_ADDRESS *dest)
{              /* destination address */
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
bool dlmstp_send_pdu_queue_empty(volatile struct mstp_port_struct_t *mstp_port)
{
    struct bacnet_port_data *port;

    port = MSTP_Port.UserData;
    return Ringbuf_Empty(&port->PDU_Queue);
}

/**
 * @brief Determine if the send PDU queue is full
 * @return true if the send PDU is full
 */
bool dlmstp_send_pdu_queue_full(volatile struct mstp_port_struct_t *mstp_port)
{
    struct bacnet_port_data *port;

    port = MSTP_Port.UserData;
    return Ringbuf_Full((&port->PDU_Queue);
}

/**
 * @brief Initialize the RS-485 baud rate
 * @param baudrate - RS-485 baud rate in bits per second (bps)
 * @return true if the baud rate was valid
 */
bool dlmstp_set_baud_rate(
    volatile struct mstp_port_struct_t *mstp_port, uint32_t baud)
{
    bool status = false;
    struct bacnet_port_data *port;
    struct rs485_driver *driver;

    port = MSTP_Port.UserData;
    driver = port->RS485_Driver;
    status = driver->baud_rate_set(baud);
    mstp_port->TurnaroundTime = rs485_turnaround_time(mstp_port);

    return status;
}

/**
 * @brief Return the RS-485 baud rate
 * @return baud - RS-485 baud rate in bits per second (bps)
 */
uint32_t dlmstp_baud_rate(volatile struct mstp_port_struct_t *mstp_port)
{
    struct bacnet_port_data *port;
    struct rs485_driver *driver;

    port = MSTP_Port.UserData;
    driver = port->RS485_Driver;

    return driver->baud_rate();
}

/**
 * @brief Initialize the RS-485 driver
 */
void dlmstp_rs485_init(volatile struct mstp_port_struct_t *mstp_port)
{
    struct bacnet_port_data *port;
    struct rs485_driver *driver;

    port = MSTP_Port.UserData;
    driver = port->RS485_Driver;

    return driver->init();
}
