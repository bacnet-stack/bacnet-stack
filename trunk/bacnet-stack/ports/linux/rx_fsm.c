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

/* Linux includes */
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

/* local includes */
#include "bytes.h"
#include "rs485.h"
#include "mstp.h"
#include "crc.h"

#define INCREMENT_AND_LIMIT_UINT16(x) {if (x < 0xFFFF) x++;}

/* local port data - shared with RS-485 */
volatile struct mstp_port_struct_t MSTP_Port;
    
void *milliseconds_task(void *pArg)
{
    struct timespec timeOut,remains;

    timeOut.tv_sec = 0;
    timeOut.tv_nsec = 10000000; /* 1 milliseconds */

    for (;;) {
        nanosleep(&timeOut, &remains);
        dlmstp_millisecond_timer();
    }

    return NULL;
}

void dlmstp_millisecond_timer(void)
{
    INCREMENT_AND_LIMIT_UINT16(MSTP_Port.SilenceTimer);
}

uint16_t dlmstp_put_receive(
    uint8_t src,    /* source MS/TP address */
    uint8_t * pdu,          /* PDU data */
    uint16_t pdu_len)
{
    (void)src;
    (void)pdu;
    (void)pdu_len;
    
    return 0;
}

static const uint8_t CRC_Table[256] = 
{
  0000,  0xfe,  0xff,  0x01,  0xfd,  0x03,  0x02,  0xfc,
  0xf9,  0x07,  0x06,  0xf8,  0x04,  0xfa,  0xfb,  0x05,
  0xf1,  0x0f,  0x0e,  0xf0,  0x0c,  0xf2,  0xf3,  0x0d,
  0x08,  0xf6,  0xf7,  0x09,  0xf5,  0x0b,  0x0a,  0xf4,
  0xe1,  0x1f,  0x1e,  0xe0,  0x1c,  0xe2,  0xe3,  0x1d,
  0x18,  0xe6,  0xe7,  0x19,  0xe5,  0x1b,  0x1a,  0xe4,
  0x10,  0xee,  0xef,  0x11,  0xed,  0x13,  0x12,  0xec,
  0xe9,  0x17,  0x16,  0xe8,  0x14,  0xea,  0xeb,  0x15,
  0xc1,  0x3f,  0x3e,  0xc0,  0x3c,  0xc2,  0xc3,  0x3d,
  0x38,  0xc6,  0xc7,  0x39,  0xc5,  0x3b,  0x3a,  0xc4,
  0x30,  0xce,  0xcf,  0x31,  0xcd,  0x33,  0x32,  0xcc,
  0xc9,  0x37,  0x36,  0xc8,  0x34,  0xca,  0xcb,  0x35,
  0x20,  0xde,  0xdf,  0x21,  0xdd,  0x23,  0x22,  0xdc,
  0xd9,  0x27,  0x26,  0xd8,  0x24,  0xda,  0xdb,  0x25,
  0xd1,  0x2f,  0x2e,  0xd0,  0x2c,  0xd2,  0xd3,  0x2d,
  0x28,  0xd6,  0xd7,  0x29,  0xd5,  0x2b,  0x2a,  0xd4,
  0x81,  0x7f,  0x7e,  0x80,  0x7c,  0x82,  0x83,  0x7d,
  0x78,  0x86,  0x87,  0x79,  0x85,  0x7b,  0x7a,  0x84,
  0x70,  0x8e,  0x8f,  0x71,  0x8d,  0x73,  0x72,  0x8c,
  0x89,  0x77,  0x76,  0x88,  0x74,  0x8a,  0x8b,  0x75,
  0x60,  0x9e,  0x9f,  0x61,  0x9d,  0x63,  0x62,  0x9c,
  0x99,  0x67,  0x66,  0x98,  0x64,  0x9a,  0x9b,  0x65,
  0x91,  0x6f,  0x6e,  0x90,  0x6c,  0x92,  0x93,  0x6d,
  0x68,  0x96,  0x97,  0x69,  0x95,  0x6b,  0x6a,  0x94,
  0x40,  0xbe,  0xbf,  0x41,  0xbd,  0x43,  0x42,  0xbc,
  0xb9,  0x47,  0x46,  0xb8,  0x44,  0xba,  0xbb,  0x45,
  0xb1,  0x4f,  0x4e,  0xb0,  0x4c,  0xb2,  0xb3,  0x4d,
  0x48,  0xb6,  0xb7,  0x49,  0xb5,  0x4b,  0x4a,  0xb4,
  0xa1,  0x5f,  0x5e,  0xa0,  0x5c,  0xa2,  0xa3,  0x5d,
  0x58,  0xa6,  0xa7,  0x59,  0xa5,  0x5b,  0x5a,  0xa4,
  0x50,  0xae,  0xaf,  0x51,  0xad,  0x53,  0x52,  0xac,
  0xa9,  0x57,  0x56,  0xa8,  0x54,  0xaa,  0xab,  0x55
};

static uint8_t calculate_header_CRC_table(
    volatile struct mstp_port_struct_t *mstp_port)
{
    uint8_t crc8 = 0xFF;        /* used to calculate the crc value */

    crc8 = CRC_Table[mstp_port->FrameType ^ crc8];
    crc8 = CRC_Table[mstp_port->DestinationAddress ^ crc8];
    crc8 = CRC_Table[mstp_port->SourceAddress ^ crc8];
    crc8 = CRC_Table[HI_BYTE(mstp_port->DataLength) ^ crc8];
    crc8 = CRC_Table[LO_BYTE(mstp_port->DataLength) ^ crc8];
}

static uint8_t calculate_header_CRC(
    volatile struct mstp_port_struct_t *mstp_port)
{
    uint8_t crc8 = 0xFF;        /* used to calculate the crc value */

    crc8 = CRC_Calc_Header(mstp_port->FrameType, crc8);
    crc8 = CRC_Calc_Header(mstp_port->DestinationAddress, crc8);
    crc8 = CRC_Calc_Header(mstp_port->SourceAddress, crc8);
    crc8 = CRC_Calc_Header(HI_BYTE(mstp_port->DataLength), crc8);
    crc8 = CRC_Calc_Header(LO_BYTE(mstp_port->DataLength), crc8);
    
    return (~crc8);
}

static void print_received_packet(
    volatile struct mstp_port_struct_t *mstp_port,
    bool print_crc_flag)
{
    unsigned i;
    uint8_t crc8 = 0xFF;        /* used to calculate the crc value */
    
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
    fprintf(stderr,
        "55 FF %02X %02X %02X %02X %02X %02X ",
        mstp_port->FrameType,
        mstp_port->DestinationAddress,
        mstp_port->SourceAddress,
        HI_BYTE(mstp_port->DataLength),
        LO_BYTE(mstp_port->DataLength),
        mstp_port->HeaderCRCActual);
    if (print_crc_flag) {
        crc8 = calculate_header_CRC(mstp_port);
        fprintf(stderr, "[%02X] ", crc8);
        crc8 = calculate_header_CRC_table(mstp_port);
        fprintf(stderr, "[%02X] ", crc8);
    }
    if (mstp_port->DataLength) {
        for (i = 0; i < mstp_port->DataLength; i++) {
            fprintf(stderr,"%02X ",
                    mstp_port->InputBuffer[i]);
        }
        fprintf(stderr,
                "%02X %02X ",
                mstp_port->DataCRCActualMSB,
                mstp_port->DataCRCActualLSB);
    }
    fprintf(stderr,"\n");
}

/* simple test to packetize the data and print it */
int main(void)
{
    volatile struct mstp_port_struct_t *mstp_port;
    int rc = 0;
    pthread_t hThread;


    /* mimic our pointer in the state machine */
    mstp_port = &MSTP_Port;
    /* initialize our interface */
    RS485_Set_Interface("/dev/ttyS0");
    RS485_Set_Baud_Rate(38400);
    RS485_Initialize();
    MSTP_Init(mstp_port);
    mstp_port->Lurking = true;
    /* start our MilliSec task */
    rc = pthread_create(&hThread, NULL, milliseconds_task, NULL);
    /* run forever */
    for (;;) {
        RS485_Check_UART_Data(mstp_port);
        MSTP_Receive_Frame_FSM(mstp_port);
        /* process the data portion of the frame */
        if (mstp_port->ReceivedValidFrame) {
            mstp_port->ReceivedValidFrame = false;
            print_received_packet(mstp_port, false);
        } else if (mstp_port->ReceivedInvalidFrame) {
            mstp_port->ReceivedInvalidFrame = false;
            fprintf(stderr, "ReceivedInvalidFrame\n");
            print_received_packet(mstp_port, true);
        }
    }

    return 0;
}

