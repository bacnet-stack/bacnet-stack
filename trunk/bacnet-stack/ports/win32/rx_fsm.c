/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307
 USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Windows includes */
#define WIN32_LEAN_AND_MEAN
#define STRICT 1
#include <windows.h>

/* local includes */
#include "bytes.h"
#include "rs485.h"
#include "mstp.h"
#include "mstptext.h"
#include "crc.h"

/* local port data - shared with RS-485 */
volatile struct mstp_port_struct_t MSTP_Port;
static uint8_t RxBuffer[MAX_MPDU];
static uint8_t TxBuffer[MAX_MPDU];
static uint16_t SilenceTime;
#define INCREMENT_AND_LIMIT_UINT16(x) {if (x < 0xFFFF) x++;}
static uint16_t Timer_Silence(
    void)
{
    return SilenceTime;
}
static void Timer_Silence_Reset(
    void)
{
    SilenceTime = 0;
}

static void dlmstp_millisecond_timer(
    void)
{
    INCREMENT_AND_LIMIT_UINT16(SilenceTime);
}

void *milliseconds_task(
    void *pArg)
{
    (void) pArg;
    for (;;) {
        Sleep(1);
        dlmstp_millisecond_timer();
    }

    /*return NULL; */
}

/* functions used by the MS/TP state machine to put or get data */
uint16_t MSTP_Put_Receive(
    volatile struct mstp_port_struct_t *mstp_port)
{
    (void) mstp_port;

    return 0;
}

/* for the MS/TP state machine to use for getting data to send */
/* Return: amount of PDU data */
uint16_t MSTP_Get_Send(
    volatile struct mstp_port_struct_t * mstp_port,
    unsigned timeout)
{       /* milliseconds to wait for a packet */
    (void) mstp_port;
    (void) timeout;
    return 0;
}

uint16_t MSTP_Get_Reply(
    volatile struct mstp_port_struct_t * mstp_port,
    unsigned timeout)
{       /* milliseconds to wait for a packet */
    (void) mstp_port;
    (void) timeout;
    return 0;
}

static void print_received_packet(
    volatile struct mstp_port_struct_t *mstp_port)
{
    unsigned i;

    /* Preamble: two octet preamble: X`55', X`FF' */
    /* Frame Type: one octet */
    /* Destination Address: one octet address */
    /* Source Address: one octet address */
    /* Length: two octets, most significant octet first, of the Data field */
    /* Header CRC: one octet */
    /* Data: (present only if Length is non-zero) */
    /* Data CRC: (present only if Length is non-zero) two octets, */
    /*           least significant octet first */
    /* (pad): (optional) at most one octet of padding: X'FF' */
    fprintf(stderr, "55 FF %02X %02X %02X %02X %02X %02X ",
        mstp_port->FrameType, mstp_port->DestinationAddress,
        mstp_port->SourceAddress, HI_BYTE(mstp_port->DataLength),
        LO_BYTE(mstp_port->DataLength), mstp_port->HeaderCRCActual);
    if (mstp_port->DataLength) {
        for (i = 0; i < mstp_port->DataLength; i++) {
            fprintf(stderr, "%02X ", mstp_port->InputBuffer[i]);
        }
        fprintf(stderr, "%02X %02X ", mstp_port->DataCRCActualMSB,
            mstp_port->DataCRCActualLSB);
    }
    fprintf(stderr, "%s", mstptext_frame_type(mstp_port->FrameType));
    fprintf(stderr, "\n");
}

static char *Network_Interface = NULL;

/* simple test to packetize the data and print it */
int main(
    int argc,
    char *argv[])
{
    volatile struct mstp_port_struct_t *mstp_port;
    int rc = 0;
    unsigned long hThread = 0;
    uint32_t arg_value = 0;
    long my_mac = 127;
    long my_baud = 38400;

    /* mimic our pointer in the state machine */
    mstp_port = &MSTP_Port;
    /* argv has the "COM4" or some other device */
    if (argc > 1) {
        Network_Interface = argv[1];
    }
    if (argc > 2) {
        my_baud = strtol(argv[2], NULL, 0);
    }
    if (argc > 3) {
        my_mac = strtol(argv[3], NULL, 0);
        if (my_mac > 127)
            my_mac = 127;
    }
    /* initialize our interface */
    RS485_Set_Interface(Network_Interface);
    RS485_Set_Baud_Rate(my_baud);
    RS485_Initialize();
    MSTP_Port.InputBuffer = &RxBuffer[0];
    MSTP_Port.InputBufferSize = sizeof(RxBuffer);
    MSTP_Port.OutputBuffer = &TxBuffer[0];
    MSTP_Port.OutputBufferSize = sizeof(TxBuffer);
    MSTP_Port.This_Station = my_mac;
    MSTP_Port.Nmax_info_frames = 1;
    MSTP_Port.Nmax_master = 127;
    MSTP_Port.SilenceTimer = Timer_Silence;
    MSTP_Port.SilenceTimerReset = Timer_Silence_Reset;
    MSTP_Init(&MSTP_Port);
    mstp_port->Lurking = true;
    /* start our MilliSec task */
    hThread = _beginthread(milliseconds_task, 4096, &arg_value);
    if (hThread == 0) {
        fprintf(stderr, "Failed to start timer task\n");
    }
    /* run forever */
    for (;;) {
        RS485_Check_UART_Data(mstp_port);
        MSTP_Receive_Frame_FSM(mstp_port);
        /* process the data portion of the frame */
        if (mstp_port->ReceivedValidFrame) {
            mstp_port->ReceivedValidFrame = false;
            print_received_packet(mstp_port);
        } else if (mstp_port->ReceivedInvalidFrame) {
            mstp_port->ReceivedInvalidFrame = false;
            fprintf(stderr, "ReceivedInvalidFrame\n");
            print_received_packet(mstp_port);
        }
    }

    /*return 0; */
}
