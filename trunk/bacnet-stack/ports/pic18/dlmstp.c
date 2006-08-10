/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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
*
*********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "bacdef.h"
#include "mstp.h"
#include "dlmstp.h"
#include "ringbuf.h"
#include "rs485.h"

#define RB_PACKET_COUNT 2
#define RB_PACKET_SIZE sizeof(DLMSTP_PACKET)
static RING_BUFFER Receive_Buffer;
static char Receive_Data_Store[RB_PACKET_COUNT * RB_PACKET_SIZE];
static RING_BUFFER Transmit_Buffer;
static char Transmit_Data_Store[RB_PACKET_COUNT * RB_PACKET_SIZE];

volatile struct mstp_port_struct_t MSTP_Port;   /* port data */
static uint8_t MSTP_MAC_Address = 0x05; /* local MAC address */
DLMSTP_PACKET Temp_Packet;

void dlmstp_init(void)
{
    Ringbuf_Init(&Receive_Buffer, 
        &Receive_Data_Store[0], 
        RB_PACKET_SIZE,
        RB_PACKET_COUNT);
    Ringbuf_Init(&Transmit_Buffer, 
        &Transmit_Data_Store[0], 
        RB_PACKET_SIZE,
        RB_PACKET_COUNT);
    RS485_Initialize();
    MSTP_Init(&MSTP_Port, MSTP_MAC_Address);
}

void dlmstp_cleanup(void)
{
  /* nothing to do for static buffers */
}

/* returns number of bytes sent on success, negative on failure */
int dlmstp_send_pdu(BACNET_ADDRESS * dest,      /* destination address */
    uint8_t * pdu,              /* any data to be sent - may be null */
    unsigned pdu_len)
{                               /* number of bytes of data */
    bool status;
    int bytes_sent = 0;
    
    memmove(&Temp_Packet.address,dest,sizeof(Temp_Packet.address));
    Temp_Packet.pdu_len = pdu_len;
    memmove(Temp_Packet.pdu,pdu,sizeof(Temp_Packet.pdu));
    status = Ringbuf_Put(&Transmit_Buffer, &Temp_Packet);
    if (status)
        bytes_sent = pdu_len;

    return bytes_sent;
}

/* function for MS/TP to use to get a packet to transmit
   returns the number of bytes in the packet, or 0 if none. */
int dlmstp_get_transmit_pdu(BACNET_ADDRESS * dest,      /* destination address */
    uint8_t * pdu)              /* any data to be sent - may be null */
{
    bool status;
    DLMSTP_PACKET *packet;
    unsigned pdu_len = 0;
    
    if (!Ringbuf_Empty(&Transmit_Buffer))
    {
        packet = Ringbuf_Pop_Front(&Transmit_Buffer);
        memmove(dest,&packet->address,sizeof(packet->address));
        pdu_len = packet->pdu_len;
        memmove(pdu,packet->pdu,sizeof(packet.pdu));
    }

    return pdu_len;
}

void dlmstp_task(void)
{
    RS485_Check_UART_Data(&MSTP_Port);
    MSTP_Receive_Frame_FSM(&MSTP_Port);

    RS485_Process_Tx_Message();
    MSTP_Master_Node_FSM(&MSTP_Port);
}

/* called about once a millisecond */
void dlmstp_millisecond_timer(void)
{
    MSTP_Millisecond_Timer(&MSTP_Port);
}

/* returns the number of octets in the PDU, or zero on failure */
/* This function is expecting to be polled. */
uint16_t dlmstp_receive(BACNET_ADDRESS * src,   /* source address */
    uint8_t * pdu,              /* PDU data */
    uint16_t max_pdu,           /* amount of space available in the PDU  */
    unsigned timeout)
{
    DLMSTP_PACKET *packet;

    (void) timeout;
    /* see if there is a packet available */
    if (!Ringbuf_Empty(&Receive_Buffer))
    {
        packet = (char *)Ringbuf_Pop_Front(&Receive_Buffer);
        memmove(src,&packet->address,sizeof(packet->address));
        pdu_len = packet->pdu_len;
        memmove(pdu,packet->pdu,max_pdu);
    }

    return pdu_len;
}

/* for the MS/TP state machine to use for putting received data */
uint16_t dlmstp_put_receive(BACNET_ADDRESS * src,   /* source address */
    uint8_t * pdu,              /* PDU data */
    uint16_t pdu_len)
{                               
    bool status;
    int bytes_put = 0;

    memmove(&Temp_Packet.address,src,sizeof(Temp_Packet.address));
    Temp_Packet.pdu_len = pdu_len;
    memmove(Temp_Packet.pdu,pdu,sizeof(Temp_Packet.pdu));
    status = Ringbuf_Put(&Receive_Buffer, &Temp_Packet);
    if (status)
        bytes_put = pdu_len;

    return bytes_put;
}

void dlmstp_set_my_address(uint8_t mac_address)
{
    /* FIXME: Master Nodes can only have address 1-127 */
    MSTP_MAC_Address = mac_address;
   
    return;
}

void dlmstp_get_my_address(BACNET_ADDRESS * my_address)
{
    my_address->mac_len = 1;
    my_address->mac[0] = MSTP_MAC_Address;
    my_address->net = 0;        /* local only, no routing */
    my_address->len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        my_address->adr[i] = 0;
    }

    return;
}

void dlmstp_get_broadcast_address(BACNET_ADDRESS * dest)
{                               /* destination address */
    int i = 0;                  /* counter */

    if (dest) {
        dest->mac_len = 1;
        dest->mac[0] = MSTP_BROADCAST_ADDRESS;
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0;          /* len=0 denotes broadcast address  */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }

    return;
}
