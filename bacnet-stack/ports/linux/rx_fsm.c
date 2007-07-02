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

uint16_t dlmstp_get_send(
        uint8_t src, /* source MS/TP address for creating packet */
    uint8_t * pdu, /* data to send */
    uint16_t max_pdu, /* amount of space available */
    unsigned timeout) /* milliseconds to wait for a packet */
{
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
    fprintf(stderr,
        "55 FF %02X %02X %02X %02X %02X %02X ",
        mstp_port->FrameType,
        mstp_port->DestinationAddress,
        mstp_port->SourceAddress,
        HI_BYTE(mstp_port->DataLength),
        LO_BYTE(mstp_port->DataLength),
        mstp_port->HeaderCRCActual);
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
            print_received_packet(mstp_port);
        } else if (mstp_port->ReceivedInvalidFrame) {
            mstp_port->ReceivedInvalidFrame = false;
            fprintf(stderr, "ReceivedInvalidFrame\n");
            print_received_packet(mstp_port);
        }
    }

    return 0;
}

