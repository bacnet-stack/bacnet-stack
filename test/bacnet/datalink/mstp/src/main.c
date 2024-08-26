/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @brief test BACnet MSTP datalink state machines
 *
 * SPDX-License-Identifier: MIT
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/ztest.h>
#/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacint.h"
#include <bacnet/datalink/crc.h>
#include <bacnet/datalink/cobs.h>
#include <bacnet/datalink/datalink.h>
#include <bacnet/datalink/mstp.h>
#include <bacnet/datalink/mstpdef.h>
#include <bacnet/datalink/mstptext.h>
#include <bacnet/datalink/crc.h>
#include <bacnet/basic/sys/bytes.h>
#include <bacnet/basic/sys/fifo.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

#define INCREMENT_AND_LIMIT_UINT8(x) \
    {                                \
        if ((x) < 0xFF)              \
            (x)++;                   \
    }

static uint8_t RxBuffer[MAX_MPDU];
static uint8_t TxBuffer[MAX_MPDU];
/* test stub functions */

/**
 * @brief Send a frame to the RS-485 network
 * @param mstp_port port specific context data
 * @param buffer pointer to the frame data
 * @param nbytes number of bytes to send
 */
void RS485_Send_Frame(
    struct mstp_port_struct_t *mstp_port, uint8_t *buffer, uint16_t nbytes)
{
    (void)mstp_port;
    (void)buffer;
    (void)nbytes;
}

/* A data queue for the test */
FIFO_DATA_STORE(Test_Queue_Data, MAX_MPDU);
static FIFO_BUFFER Test_Queue;

/**
 * @brief Load the input buffer with data
 * @param buffer pointer to the data
 * @param len number of bytes to load
 */
static void Load_Input_Buffer(uint8_t *buffer, size_t len)
{
    static bool initialized = false; /* tracks our init */
    if (!initialized) {
        initialized = true;
        FIFO_Init(&Test_Queue, Test_Queue_Data, MAX_MPDU);
    }
    /* empty any the existing data */
    FIFO_Flush(&Test_Queue);
    FIFO_Add(&Test_Queue, buffer, len);
}

/**
 * @brief Check the UART for data
 * @param mstp_port port specific context data
 */
void RS485_Check_UART_Data(struct mstp_port_struct_t *mstp_port)
{
    if (!FIFO_Empty(&Test_Queue)) {
        if (mstp_port) {
            mstp_port->DataRegister = FIFO_Get(&Test_Queue);
            mstp_port->DataAvailable = true;
        }
    }
}

/**
 * @brief Store data about a RS-485 network received packet
 * @param mstp_port port specific context data
 */
uint16_t MSTP_Put_Receive(struct mstp_port_struct_t *mstp_port)
{
    return mstp_port->DataLength;
}

/**
 * @brief MS/TP state machine calls this to get data to send
 * @param mstp_port port specific context data
 * @param timeout milliseconds to wait for a packet to send
 * @return amount of PDU data
 */
uint16_t MSTP_Get_Send(struct mstp_port_struct_t *mstp_port, unsigned timeout)
{
    (void)mstp_port;
    (void)timeout;

    return 0;
}

/**
 * @brief MS/TP state machine calls this to get data to send
 * @param mstp_port port specific context data
 * @param timeout milliseconds to wait for a packet
 * @return amount of PDU data
 */
uint16_t MSTP_Get_Reply(struct mstp_port_struct_t *mstp_port, unsigned timeout)
{
    (void)mstp_port;
    (void)timeout;

    return 0;
}

/* track the RS485 line silence time in milliseconds */
uint32_t SilenceTime = 0;
/**
 * @brief MS/TP state machine calls this to get the silence time
 * @param pArg pointer to the port specific context data
 * @return amount of time in milliseconds
 */
static uint32_t Timer_Silence(void *pArg)
{
    (void)pArg;
    return SilenceTime;
}

/**
 * @brief MS/TP state machine calls this to reset the silence time
 * @param pArg pointer to the port specific context data
 */
static void Timer_Silence_Reset(void *pArg)
{
    (void)pArg;
    SilenceTime = 0;
}

/**
 * @brief MS/TP state machine calls this to send a frame
 * @param mstp_port port specific context data
 * @param buffer pointer to the frame data
 * @param nbytes number of bytes to send
 */
void MSTP_Send_Frame(
    struct mstp_port_struct_t *mstp_port, uint8_t *buffer, uint16_t nbytes)
{
    if (mstp_port && mstp_port->OutputBuffer && buffer && (nbytes > 0) &&
        (nbytes <= mstp_port->OutputBufferSize)) {
        memcpy(mstp_port->OutputBuffer, buffer, nbytes);
    }
}

static void testReceiveNodeFSM(void)
{
    struct mstp_port_struct_t mstp_port; /* port data */
    unsigned EventCount = 0; /* local counter */
    uint8_t my_mac = 0x05; /* local MAC address */
    uint8_t HeaderCRC = 0; /* for local CRC calculation */
    uint8_t FrameType = 0; /* type of packet that was sent */
    unsigned len; /* used for the size of the message packet */
    size_t i; /* used to loop through the message bytes */
    uint8_t buffer[MAX_MPDU] = { 0 };
    uint8_t data[MAX_PDU] = { 0 };
    uint8_t data_proprietary[MSTP_FRAME_NPDU_MAX] = { 0 };

    mstp_port.InputBuffer = &RxBuffer[0];
    mstp_port.InputBufferSize = sizeof(RxBuffer);
    mstp_port.OutputBuffer = &TxBuffer[0];
    mstp_port.OutputBufferSize = sizeof(TxBuffer);
    mstp_port.SilenceTimer = Timer_Silence;
    mstp_port.SilenceTimerReset = Timer_Silence_Reset;
    mstp_port.This_Station = my_mac;
    mstp_port.Nmax_info_frames = 1;
    mstp_port.Nmax_master = 127;
    MSTP_Init(&mstp_port);
    /* check the receive error during idle */
    mstp_port.receive_state = MSTP_RECEIVE_STATE_IDLE;
    mstp_port.ReceiveError = true;
    SilenceTime = 255;
    mstp_port.EventCount = 0;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.ReceiveError == false, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* check for bad packet header */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x11;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* check for good packet header, but timeout */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x55;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    /* force the timeout */
    SilenceTime = mstp_port.Tframe_abort + 1;
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* check for good packet header preamble, but receive error */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x55;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    /* force the error */
    mstp_port.ReceiveError = true;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.ReceiveError == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* check for good packet header preamble1, but bad preamble2 */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x55;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    MSTP_Receive_Frame_FSM(&mstp_port);
    /* no change of state if no data yet */
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    /* repeated preamble1 */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x55;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    /* repeated preamble1 */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x55;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    /* bad data */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x11;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.ReceiveError == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* check for good packet header preamble, but timeout in packet */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x55;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    MSTP_Receive_Frame_FSM(&mstp_port);
    /* preamble2 */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0xFF;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.Index == 0, NULL);
    zassert_true(mstp_port.HeaderCRC == 0xFF, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    /* force the timeout */
    SilenceTime = mstp_port.Tframe_abort + 1;
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    zassert_true(mstp_port.ReceivedInvalidFrame == true, NULL);
    /* check for good packet header preamble, but error in packet */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x55;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    MSTP_Receive_Frame_FSM(&mstp_port);
    /* preamble2 */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0xFF;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.Index == 0, NULL);
    zassert_true(mstp_port.HeaderCRC == 0xFF, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    /* force the error */
    mstp_port.ReceiveError = true;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.ReceiveError == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* check for good packet header preamble */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x55;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_PREAMBLE, NULL);
    MSTP_Receive_Frame_FSM(&mstp_port);
    /* preamble2 */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0xFF;
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.Index == 0, NULL);
    zassert_true(mstp_port.HeaderCRC == 0xFF, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    /* no change of state if no data yet */
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    /* Data is received - index is incremented */
    /* FrameType */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = FRAME_TYPE_TOKEN;
    HeaderCRC = 0xFF;
    HeaderCRC = CRC_Calc_Header(mstp_port.DataRegister, HeaderCRC);
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.Index == 1, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    zassert_true(FrameType == FRAME_TYPE_TOKEN, NULL);
    /* Destination */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0x10;
    HeaderCRC = CRC_Calc_Header(mstp_port.DataRegister, HeaderCRC);
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.Index == 2, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    zassert_true(mstp_port.DestinationAddress == 0x10, NULL);
    /* Source */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = my_mac;
    HeaderCRC = CRC_Calc_Header(mstp_port.DataRegister, HeaderCRC);
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.Index == 3, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    zassert_true(mstp_port.SourceAddress == my_mac, NULL);
    /* Length1 = length*256 */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0;
    HeaderCRC = CRC_Calc_Header(mstp_port.DataRegister, HeaderCRC);
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.Index == 4, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    zassert_true(mstp_port.DataLength == 0, NULL);
    /* Length2 */
    mstp_port.DataAvailable = true;
    mstp_port.DataRegister = 0;
    HeaderCRC = CRC_Calc_Header(mstp_port.DataRegister, HeaderCRC);
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.Index == 5, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_HEADER, NULL);
    zassert_true(mstp_port.DataLength == 0, NULL);
    /* HeaderCRC */
    mstp_port.DataAvailable = true;
    zassert_true(HeaderCRC == 0x73, NULL); /* per Annex G example */
    mstp_port.DataRegister = ~HeaderCRC; /* one's compliment of CRC is sent */
    INCREMENT_AND_LIMIT_UINT8(EventCount);
    MSTP_Receive_Frame_FSM(&mstp_port);
    zassert_true(mstp_port.DataAvailable == false, NULL);
    zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
    zassert_true(mstp_port.EventCount == EventCount, NULL);
    zassert_true(mstp_port.Index == 5, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    zassert_true(mstp_port.HeaderCRC == 0x55, NULL);
    /* BadCRC in header check */
    mstp_port.ReceivedInvalidFrame = false;
    mstp_port.ReceivedValidFrame = false;
    len = MSTP_Create_Frame(
        buffer, sizeof(buffer), FRAME_TYPE_TOKEN, 0x10, /* destination */
        my_mac, /* source */
        NULL, /* data */
        0); /* data size */
    zassert_true(len > 0, NULL);
    /* make the header CRC bad */
    buffer[7] = 0x00;
    Load_Input_Buffer(buffer, len);
    for (i = 0; i < len; i++) {
        RS485_Check_UART_Data(&mstp_port);
        INCREMENT_AND_LIMIT_UINT8(EventCount);
        MSTP_Receive_Frame_FSM(&mstp_port);
        zassert_true(mstp_port.DataAvailable == false, NULL);
        zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
        zassert_true(
            mstp_port.EventCount == EventCount, "i=%u %u!=%u len=%u", i,
            mstp_port.EventCount, EventCount, len);
    }
    zassert_true(mstp_port.ReceivedInvalidFrame == true, NULL);
    zassert_true(mstp_port.ReceivedValidFrame == false, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* NoData for us */
    mstp_port.ReceivedInvalidFrame = false;
    mstp_port.ReceivedValidFrame = false;
    len = MSTP_Create_Frame(
        buffer, sizeof(buffer), FRAME_TYPE_TOKEN, my_mac, /* destination */
        my_mac, /* source */
        NULL, /* data */
        0); /* data size */
    zassert_true(len > 0, NULL);
    Load_Input_Buffer(buffer, len);
    for (i = 0; i < len; i++) {
        RS485_Check_UART_Data(&mstp_port);
        INCREMENT_AND_LIMIT_UINT8(EventCount);
        MSTP_Receive_Frame_FSM(&mstp_port);
        zassert_true(mstp_port.DataAvailable == false, NULL);
        zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
        zassert_true(mstp_port.EventCount == EventCount, NULL);
    }
    zassert_true(mstp_port.ReceivedInvalidFrame == false, NULL);
    zassert_true(mstp_port.ReceivedValidFrame == true, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* FrameTooLong */
    mstp_port.ReceivedInvalidFrame = false;
    mstp_port.ReceivedValidFrame = false;
    len = MSTP_Create_Frame(
        buffer, sizeof(buffer), FRAME_TYPE_TOKEN, my_mac, /* destination */
        my_mac, /* source */
        NULL, /* data */
        0); /* data size */
    zassert_true(len > 0, NULL);
    /* make the header data length bad */
    buffer[5] = 0x02;
    Load_Input_Buffer(buffer, len);
    for (i = 0; i < len; i++) {
        RS485_Check_UART_Data(&mstp_port);
        INCREMENT_AND_LIMIT_UINT8(EventCount);
        MSTP_Receive_Frame_FSM(&mstp_port);
        zassert_true(mstp_port.DataAvailable == false, NULL);
        zassert_true(mstp_port.SilenceTimer(&mstp_port) == 0, NULL);
        zassert_true(mstp_port.EventCount == EventCount, NULL);
    }
    zassert_true(mstp_port.ReceivedInvalidFrame == true, NULL);
    zassert_true(mstp_port.ReceivedValidFrame == false, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* Proprietary Data */
    mstp_port.ReceivedInvalidFrame = false;
    mstp_port.ReceivedValidFrame = false;
    memset(data_proprietary, 0, sizeof(data_proprietary));
    len = MSTP_Create_Frame(
        buffer, sizeof(buffer), FRAME_TYPE_PROPRIETARY_MIN, my_mac, my_mac,
        data_proprietary, sizeof(data_proprietary));
    zassert_true(len > 0, NULL);
    Load_Input_Buffer(buffer, len);
    RS485_Check_UART_Data(&mstp_port);
    MSTP_Receive_Frame_FSM(&mstp_port);
    while (mstp_port.receive_state != MSTP_RECEIVE_STATE_IDLE) {
        RS485_Check_UART_Data(&mstp_port);
        MSTP_Receive_Frame_FSM(&mstp_port);
    }
    zassert_true(mstp_port.DataLength == sizeof(data_proprietary), NULL);
    zassert_true(mstp_port.ReceivedInvalidFrame == false, NULL);
    zassert_true(mstp_port.ReceivedValidFrame == true, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    /* Extended-Data-Expecting-Reply */
    zassert_true(Nmin_COBS_length_BACnet <= sizeof(data), NULL);
    mstp_port.ReceivedInvalidFrame = false;
    mstp_port.ReceivedValidFrame = false;
    memset(data, 0, sizeof(data));
    len = MSTP_Create_Frame(
        buffer, sizeof(buffer), FRAME_TYPE_BACNET_EXTENDED_DATA_EXPECTING_REPLY,
        my_mac, my_mac, data, Nmin_COBS_length_BACnet);
    zassert_true(len > 0, NULL);
    Load_Input_Buffer(buffer, len);
    RS485_Check_UART_Data(&mstp_port);
    MSTP_Receive_Frame_FSM(&mstp_port);
    while (mstp_port.receive_state != MSTP_RECEIVE_STATE_IDLE) {
        RS485_Check_UART_Data(&mstp_port);
        MSTP_Receive_Frame_FSM(&mstp_port);
    }
    zassert_true(mstp_port.DataLength == Nmin_COBS_length_BACnet, NULL);
    zassert_true(mstp_port.ReceivedInvalidFrame == false, NULL);
    zassert_true(mstp_port.ReceivedValidFrame == true, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    zassert_true(
        mstp_port.FrameType == FRAME_TYPE_BACNET_EXTENDED_DATA_EXPECTING_REPLY,
        NULL);
    /* Extended-Data-Not-Expecting-Reply */
    mstp_port.ReceivedInvalidFrame = false;
    mstp_port.ReceivedValidFrame = false;
    memset(data, 0, sizeof(data));
    len = MSTP_Create_Frame(
        buffer, sizeof(buffer),
        FRAME_TYPE_BACNET_EXTENDED_DATA_NOT_EXPECTING_REPLY, my_mac, my_mac,
        data, Nmin_COBS_length_BACnet);
    zassert_true(len > 0, NULL);
    Load_Input_Buffer(buffer, len);
    RS485_Check_UART_Data(&mstp_port);
    MSTP_Receive_Frame_FSM(&mstp_port);
    while (mstp_port.receive_state != MSTP_RECEIVE_STATE_IDLE) {
        RS485_Check_UART_Data(&mstp_port);
        MSTP_Receive_Frame_FSM(&mstp_port);
    }
    zassert_true(mstp_port.DataLength == Nmin_COBS_length_BACnet, NULL);
    zassert_true(mstp_port.ReceivedInvalidFrame == false, NULL);
    zassert_true(mstp_port.ReceivedValidFrame == true, NULL);
    zassert_true(mstp_port.receive_state == MSTP_RECEIVE_STATE_IDLE, NULL);
    zassert_true(
        mstp_port.FrameType ==
            FRAME_TYPE_BACNET_EXTENDED_DATA_NOT_EXPECTING_REPLY,
        NULL);
}

static void testMasterNodeFSM(void)
{
    struct mstp_port_struct_t MSTP_Port; /* port data */
    uint8_t my_mac = 0x05; /* local MAC address */
    MSTP_Port.InputBuffer = &RxBuffer[0];
    MSTP_Port.InputBufferSize = sizeof(RxBuffer);
    MSTP_Port.OutputBuffer = &TxBuffer[0];
    MSTP_Port.OutputBufferSize = sizeof(TxBuffer);

    MSTP_Port.Nmax_info_frames = 1;
    MSTP_Port.Nmax_master = 127;

    MSTP_Port.Tframe_abort = DEFAULT_Tframe_abort;
    MSTP_Port.Treply_delay = DEFAULT_Treply_delay;
    MSTP_Port.Treply_timeout = DEFAULT_Treply_timeout;
    MSTP_Port.Tusage_timeout = DEFAULT_Tusage_timeout;

    MSTP_Port.SilenceTimer = Timer_Silence;
    MSTP_Port.SilenceTimerReset = Timer_Silence_Reset;

    MSTP_Port.This_Station = my_mac;

    MSTP_Init(&MSTP_Port);
    zassert_true(MSTP_Port.master_state == MSTP_MASTER_STATE_INITIALIZE, NULL);
    /* FIXME: write a unit test for the Master Node State Machine */
}

static void testSlaveNodeFSM(void)
{
    struct mstp_port_struct_t MSTP_Port = { 0 }; /* port data */

    MSTP_Port.InputBuffer = &RxBuffer[0];
    MSTP_Port.InputBufferSize = sizeof(RxBuffer);
    MSTP_Port.OutputBuffer = &TxBuffer[0];
    MSTP_Port.OutputBufferSize = sizeof(TxBuffer);

    MSTP_Port.Nmax_info_frames = 0;
    MSTP_Port.Nmax_master = 0;

    MSTP_Port.Tframe_abort = DEFAULT_Tframe_abort;
    MSTP_Port.Treply_delay = DEFAULT_Treply_delay;
    MSTP_Port.Treply_timeout = DEFAULT_Treply_timeout;
    MSTP_Port.Tusage_timeout = DEFAULT_Tusage_timeout;

    MSTP_Port.SilenceTimer = Timer_Silence;
    MSTP_Port.SilenceTimerReset = Timer_Silence_Reset;

    MSTP_Port.This_Station = 128;

    MSTP_Init(&MSTP_Port);
    MSTP_Slave_Node_FSM(&MSTP_Port);
    zassert_true(MSTP_Port.master_state == MSTP_MASTER_STATE_IDLE, NULL);
}

static void testZeroConfigNode_Init(struct mstp_port_struct_t *mstp_port)
{
    bool transition_now, non_zero;
    unsigned slots, silence, i;

    mstp_port->InputBuffer = &RxBuffer[0];
    mstp_port->InputBufferSize = sizeof(RxBuffer);
    mstp_port->OutputBuffer = &TxBuffer[0];
    mstp_port->OutputBufferSize = sizeof(TxBuffer);

    mstp_port->Nmax_info_frames = 1;
    mstp_port->Nmax_master = 127;

    mstp_port->Tframe_abort = DEFAULT_Tframe_abort;
    mstp_port->Treply_delay = DEFAULT_Treply_delay;
    mstp_port->Treply_timeout = DEFAULT_Treply_timeout;
    mstp_port->Tusage_timeout = DEFAULT_Tusage_timeout;

    mstp_port->SilenceTimer = Timer_Silence;
    mstp_port->SilenceTimerReset = Timer_Silence_Reset;

    /* configure for Zero Config */
    mstp_port->ZeroConfigEnabled = true;
    mstp_port->This_Station = 255;
    MSTP_Zero_Config_UUID_Init(mstp_port);

    MSTP_Init(mstp_port);
    zassert_true(mstp_port->master_state == MSTP_MASTER_STATE_INITIALIZE, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_INIT, NULL);
    zassert_true(mstp_port->Tframe_abort == DEFAULT_Tframe_abort, NULL);
    zassert_true(mstp_port->Treply_delay == DEFAULT_Treply_delay, NULL);
    zassert_true(mstp_port->Treply_timeout == DEFAULT_Treply_timeout, NULL);
    zassert_true(mstp_port->Tusage_timeout == DEFAULT_Tusage_timeout, NULL);
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_IDLE, NULL);
    zassert_true(mstp_port->Poll_Count == 0, NULL);
    zassert_true(mstp_port->Zero_Config_Station == 64, NULL);
    zassert_true(mstp_port->Npoll_slot >= 1, NULL);
    zassert_true(mstp_port->Npoll_slot <= 64, NULL);
    slots = 128 + mstp_port->Npoll_slot;
    silence = Tno_token + Tslot * slots;
    zassert_true(mstp_port->Zero_Config_Silence == silence, NULL);
    non_zero = false;
    for (i = 0; i < MSTP_UUID_SIZE; i++) {
        if (mstp_port->UUID[i] > 0) {
            non_zero = true;
        }
    }
    zassert_true(non_zero, NULL);
    zassert_true(mstp_port->Zero_Config_Max_Master == 0, NULL);
}

static void
testZeroConfigNode_No_Events_Timeout(struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;

    SilenceTime = mstp_port->Zero_Config_Silence + 1;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_CONFIRM, NULL);
}

static void testZeroConfigNode_Test_Request_Unsupported(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;

    /* test case: remote node does not support Test-Request; timeout */
    SilenceTime = mstp_port->Treply_timeout + 1;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_true(transition_now, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_USE, NULL);
    zassert_true(
        mstp_port->This_Station == mstp_port->Zero_Config_Station, NULL);
}

static void
testZeroConfigNode_Test_IDLE_InvalidFrame(struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;

    /* test case: waiting for a invalid frame */
    SilenceTime = 0;
    mstp_port->ReceivedInvalidFrame = true;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_IDLE, NULL);
    zassert_true(mstp_port->ReceivedInvalidFrame == false, NULL);
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_IDLE, NULL);
    zassert_true(mstp_port->ReceivedInvalidFrame == false, NULL);
}

static void testZeroConfigNode_Test_IDLE_ValidFrameTimeout(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;

    /* test case: get a valid frame, followed by timeout  */
    SilenceTime = 0;
    mstp_port->SourceAddress = 0;
    mstp_port->DestinationAddress = 1;
    mstp_port->ReceivedValidFrame = true;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_LURK, NULL);
    zassert_true(mstp_port->ReceivedValidFrame == true, NULL);
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_LURK, NULL);
    zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
    SilenceTime = mstp_port->Zero_Config_Silence + 1;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_IDLE, NULL);
}

static void
testZeroConfigNode_Test_IDLE_ValidFrame(struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;

    /* test case: get a valid frame, followed by timeout  */
    SilenceTime = 0;
    mstp_port->SourceAddress = 0;
    mstp_port->DestinationAddress = 1;
    mstp_port->ReceivedValidFrame = true;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(
        mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_LURK, NULL);
    zassert_true(mstp_port->ReceivedValidFrame == true, NULL);
}

static void
testZeroConfigNode_Test_LURK_AddressInUse(struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;
    uint8_t src, dst;

    /* test case: src emits a token from each MAC in the zero-config range */
    SilenceTime = 0;
    mstp_port->FrameType = FRAME_TYPE_TOKEN;
    for (src = Nmin_poll_station; src <= Nmax_poll_station; src++) {
        mstp_port->ReceivedValidFrame = true;
        mstp_port->SourceAddress = src;
        dst = (src + 1) % (Nmax_master_station + 1);
        mstp_port->DestinationAddress = dst;
        zassert_true(mstp_port->Zero_Config_Station == src, NULL);
        transition_now = MSTP_Master_Node_FSM(mstp_port);
        zassert_false(transition_now, NULL);
        zassert_true(
            mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_LURK, NULL);
        zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
        zassert_true(
            mstp_port->Zero_Config_Station != src, "src=%u zc=%u", src,
            mstp_port->Zero_Config_Station);
    }
}

static void testZeroConfigNode_Test_LURK_LearnMaxMaster(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;
    uint8_t dst;

    /* test case: src emits a token from each MAC in the zero-config range */
    SilenceTime = 0;
    mstp_port->SourceAddress = 0;
    mstp_port->FrameType = FRAME_TYPE_POLL_FOR_MASTER;
    for (dst = 1; dst <= Nmax_master_station; dst++) {
        mstp_port->ReceivedValidFrame = true;
        mstp_port->DestinationAddress = dst;
        transition_now = MSTP_Master_Node_FSM(mstp_port);
        zassert_false(transition_now, NULL);
        zassert_true(
            mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_LURK, NULL);
        zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
        zassert_true(mstp_port->Zero_Config_Max_Master == dst, NULL);
    }
}

static void
testZeroConfigNode_Test_LURK_Claim(struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;
    uint8_t src = 0, dst, count, count_max, count_claim;

    /* test case: src emits a PFM from each MAC in the zero-config range */
    SilenceTime = 0;
    mstp_port->SourceAddress = src;
    mstp_port->FrameType = FRAME_TYPE_POLL_FOR_MASTER;
    dst = Nmin_poll_station;
    count_claim = Nmin_poll + mstp_port->Npoll_slot;
    count_max = Nmin_poll + Nmax_poll_station;
    for (count = 0; count < count_max; count++) {
        /* simulate receiving PFM */
        mstp_port->ReceivedValidFrame = true;
        mstp_port->DestinationAddress = dst;
        transition_now = MSTP_Master_Node_FSM(mstp_port);
        zassert_false(transition_now, NULL);
        zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
        zassert_true((mstp_port->Zero_Config_Station) == dst, NULL);
        if (mstp_port->Npoll_slot == count_claim) {
            zassert_true(
                mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_CLAIM,
                NULL);
        } else if (
            mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_LURK) {
            zassert_true(
                mstp_port->Poll_Count == (count + 1), "count=%u", count);
            zassert_true(
                mstp_port->Zero_Config_State == MSTP_ZERO_CONFIG_STATE_LURK,
                NULL);
        } else {
            break;
        }
    }
    /* verify the Reply To Poll For Master was sent for confirmation */
    zassert_equal(
        mstp_port->OutputBuffer[2], FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER, NULL);
    zassert_equal(mstp_port->OutputBuffer[3], mstp_port->SourceAddress, NULL);
    zassert_equal(
        mstp_port->OutputBuffer[4], mstp_port->Zero_Config_Station, NULL);
}

static void testZeroConfigNode_Test_LURK_ClaimTokenForUs(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;
    uint8_t src = 0, dst;

    /* ClaimTokenForUs */
    dst = mstp_port->Zero_Config_Station;
    mstp_port->SourceAddress = src;
    mstp_port->DestinationAddress = dst;
    mstp_port->FrameType = FRAME_TYPE_TOKEN;
    mstp_port->ReceivedValidFrame = true;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
    zassert_equal(
        mstp_port->Zero_Config_State, MSTP_ZERO_CONFIG_STATE_CONFIRM, NULL);
    /* verify the Test Request Frame was sent for confirmation */
    zassert_equal(mstp_port->OutputBuffer[2], FRAME_TYPE_TEST_REQUEST, NULL);
    zassert_equal(mstp_port->OutputBuffer[3], mstp_port->SourceAddress, NULL);
    zassert_equal(
        mstp_port->OutputBuffer[4], mstp_port->Zero_Config_Station, NULL);
}

static void testZeroConfigNode_Test_LURK_ConfirmationSuccessful(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;
    uint8_t src = 0, dst;

    /* ConfirmationSuccessful */
    dst = mstp_port->Zero_Config_Station;
    mstp_port->SourceAddress = src;
    mstp_port->DestinationAddress = dst;
    mstp_port->FrameType = FRAME_TYPE_TEST_RESPONSE;
    memcpy(mstp_port->InputBuffer, mstp_port->UUID, MSTP_UUID_SIZE);
    mstp_port->DataLength = MSTP_UUID_SIZE;
    mstp_port->ReceivedValidFrame = true;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_true(transition_now, NULL);
    zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
    zassert_equal(
        mstp_port->This_Station, mstp_port->Zero_Config_Station, NULL);
    zassert_equal(
        mstp_port->Zero_Config_State, MSTP_ZERO_CONFIG_STATE_USE, NULL);
}

static void testZeroConfigNode_Test_LURK_ConfirmationAddressInUse(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;
    uint8_t src, dst, test_station;

    /* ConfirmationAddressInUse */
    dst = mstp_port->Zero_Config_Station;
    src = mstp_port->Zero_Config_Station;
    mstp_port->SourceAddress = src;
    mstp_port->DestinationAddress = dst;
    mstp_port->FrameType = FRAME_TYPE_PROPRIETARY_MIN;
    encode_unsigned16(&mstp_port->InputBuffer[0], BACNET_VENDOR_ID);
    memcpy(&mstp_port->InputBuffer[2], mstp_port->UUID, MSTP_UUID_SIZE);
    mstp_port->DataLength = MSTP_UUID_SIZE + 2;
    mstp_port->ReceivedValidFrame = true;
    test_station = mstp_port->Zero_Config_Station + 1;

    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
    zassert_equal(test_station, mstp_port->Zero_Config_Station, NULL);
    zassert_equal(
        mstp_port->Zero_Config_State, MSTP_ZERO_CONFIG_STATE_LURK, NULL);
}

static void testZeroConfigNode_Test_LURK_ConfirmationUnuccessful_UUID_Size(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;
    uint8_t src = 0, dst;

    /* ConfirmationFailed */
    dst = mstp_port->Zero_Config_Station;
    mstp_port->SourceAddress = src;
    mstp_port->DestinationAddress = dst;
    mstp_port->FrameType = FRAME_TYPE_TEST_RESPONSE;
    memcpy(mstp_port->InputBuffer, mstp_port->UUID, MSTP_UUID_SIZE);
    /* set to an invalid size */
    mstp_port->DataLength = MSTP_UUID_SIZE - 1;
    mstp_port->ReceivedValidFrame = true;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
    zassert_equal(
        mstp_port->Zero_Config_State, MSTP_ZERO_CONFIG_STATE_IDLE, NULL);
}

static void testZeroConfigNode_Test_LURK_ConfirmationUnuccessful_UUID(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;
    uint8_t src = 0, dst;

    /* ConfirmationFailed */
    dst = mstp_port->Zero_Config_Station;
    mstp_port->SourceAddress = src;
    mstp_port->DestinationAddress = dst;
    mstp_port->FrameType = FRAME_TYPE_TEST_RESPONSE;
    memcpy(mstp_port->InputBuffer, mstp_port->UUID, MSTP_UUID_SIZE);
    /* make the UUID invalid */
    mstp_port->InputBuffer[0] = ~mstp_port->InputBuffer[0];
    mstp_port->DataLength = MSTP_UUID_SIZE;
    mstp_port->ReceivedValidFrame = true;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
    zassert_equal(
        mstp_port->Zero_Config_State, MSTP_ZERO_CONFIG_STATE_IDLE, NULL);
}

static void testZeroConfigNode_Test_LURK_ClaimAddressInUse(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;
    uint8_t station;

    /* ClaimAddressInUse */
    station = mstp_port->Zero_Config_Station;
    mstp_port->SourceAddress = station;
    mstp_port->DestinationAddress = 0;
    mstp_port->FrameType = FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER;
    mstp_port->ReceivedValidFrame = true;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(mstp_port->ReceivedValidFrame == false, NULL);
    zassert_equal(
        mstp_port->Zero_Config_State, MSTP_ZERO_CONFIG_STATE_LURK, NULL);
    zassert_equal(mstp_port->Zero_Config_Station, station + 1, NULL);
}

static void testZeroConfigNode_Test_LURK_ClaimInvalidFrame(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;

    /* ClaimInvalidFrame */
    mstp_port->ReceivedValidFrame = false;
    mstp_port->ReceivedInvalidFrame = true;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_true(mstp_port->ReceivedInvalidFrame == false, NULL);
    zassert_equal(
        mstp_port->Zero_Config_State, MSTP_ZERO_CONFIG_STATE_CLAIM, NULL);
}

static void testZeroConfigNode_Test_LURK_ClaimLostToken(
    struct mstp_port_struct_t *mstp_port)
{
    bool transition_now;

    /* ClaimLostToken */
    mstp_port->ReceivedValidFrame = false;
    mstp_port->ReceivedInvalidFrame = false;
    SilenceTime = mstp_port->Zero_Config_Silence + 1;
    transition_now = MSTP_Master_Node_FSM(mstp_port);
    zassert_false(transition_now, NULL);
    zassert_equal(
        mstp_port->Zero_Config_State, MSTP_ZERO_CONFIG_STATE_IDLE, NULL);
}

static void testZeroConfigNodeFSM(void)
{
    struct mstp_port_struct_t MSTP_Port = { 0 }; /* port data */
    unsigned station, next_station, test_station;

    /* test case: timeout event */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_No_Events_Timeout(&MSTP_Port);
    testZeroConfigNode_Test_Request_Unsupported(&MSTP_Port);
    /* test case: invalid frame event */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_InvalidFrame(&MSTP_Port);
    /* test case: valid frame event and timeout */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrameTimeout(&MSTP_Port);
    /* test case: valid frame event LURK Tokens: AddressInUse */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrame(&MSTP_Port);
    testZeroConfigNode_Test_LURK_AddressInUse(&MSTP_Port);
    /* test case: valid frame event LURK PFMs: LearnMaxMaster */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrame(&MSTP_Port);
    testZeroConfigNode_Test_LURK_LearnMaxMaster(&MSTP_Port);
    /* test case: valid frame event LURK PFMs: ClaimAddress
       ConfirmationSuccessful */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrame(&MSTP_Port);
    testZeroConfigNode_Test_LURK_Claim(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ClaimTokenForUs(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ConfirmationSuccessful(&MSTP_Port);
    /* test case: valid frame event LURK PFMs: ClaimAddress
       ConfirmationAddressInUse */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrame(&MSTP_Port);
    testZeroConfigNode_Test_LURK_Claim(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ClaimTokenForUs(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ConfirmationAddressInUse(&MSTP_Port);
    /* test case: valid frame event LURK PFMs: ClaimAddress
       but Confirmation is Unsuccessful - UUID is invalid */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrame(&MSTP_Port);
    testZeroConfigNode_Test_LURK_Claim(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ClaimTokenForUs(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ConfirmationUnuccessful_UUID(&MSTP_Port);
    /* test case: valid frame event LURK PFMs: ClaimAddress
       but Confirmation is Unsuccessful - UUID is too short */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrame(&MSTP_Port);
    testZeroConfigNode_Test_LURK_Claim(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ClaimTokenForUs(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ConfirmationUnuccessful_UUID_Size(&MSTP_Port);
    /* test case: valid frame event LURK PFMs: ClaimAddressInUse  */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrame(&MSTP_Port);
    testZeroConfigNode_Test_LURK_Claim(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ClaimAddressInUse(&MSTP_Port);
    /* test case: valid frame event LURK PFMs: ClaimInvalidFrame  */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrame(&MSTP_Port);
    testZeroConfigNode_Test_LURK_Claim(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ClaimInvalidFrame(&MSTP_Port);
    /* test case: valid frame event LURK PFMs: ClaimLostToken  */
    testZeroConfigNode_Init(&MSTP_Port);
    testZeroConfigNode_Test_IDLE_ValidFrame(&MSTP_Port);
    testZeroConfigNode_Test_LURK_Claim(&MSTP_Port);
    testZeroConfigNode_Test_LURK_ClaimLostToken(&MSTP_Port);
    /* test next station rollover */
    station = 0;
    test_station = Nmin_poll_station;
    next_station = MSTP_Zero_Config_Station_Increment(station);
    zassert_equal(next_station, test_station, "station=%u next_station=%u",
        station, next_station);
    station = Nmin_poll_station;
    test_station = Nmin_poll_station + 1;
    next_station = MSTP_Zero_Config_Station_Increment(station);
    zassert_equal(next_station, test_station, "station=%u next_station=%u",
        station, next_station);
    station = Nmax_poll_station - 1;
    test_station = Nmax_poll_station;
    next_station = MSTP_Zero_Config_Station_Increment(station);
    zassert_equal(next_station, test_station,"station=%u next_station=%u",
        station, next_station);
    station = Nmax_poll_station;
    test_station = Nmin_poll_station;
    next_station = MSTP_Zero_Config_Station_Increment(station);
    zassert_equal(next_station, test_station, "station=%u next_station=%u",
        station, next_station);
}

/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(
        crc_tests, ztest_unit_test(testReceiveNodeFSM),
        ztest_unit_test(testMasterNodeFSM), ztest_unit_test(testSlaveNodeFSM),
        ztest_unit_test(testZeroConfigNodeFSM));

    ztest_run_test_suite(crc_tests);
}
