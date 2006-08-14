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
#include "rs485.h"
#include "npdu.h"

static DLMSTP_PACKET Receive_Buffer;
static DLMSTP_PACKET Transmit_Buffer;
static uint8_t PDU_Buffer[MAX_MPDU];
/* local MS/TP port data */
volatile struct mstp_port_struct_t MSTP_Port;   /* port data */

void dlmstp_init(void)
{
    /* initialize buffer */
    Receive_Buffer.ready = false;
    Receive_Buffer.data_expecting_reply = false;
    Receive_Buffer.pdu_len = 0;
    /* initialize buffer */
    Transmit_Buffer.ready = false;
    Transmit_Buffer.data_expecting_reply = false;
    Transmit_Buffer.pdu_len = 0;
    /* initialize hardware */
    RS485_Initialize();
    MSTP_Init(&MSTP_Port, MSTP_Port.This_Station);
}

void dlmstp_cleanup(void)
{
    /* nothing to do for static buffers */
}

/* returns number of bytes sent on success, zero on failure */
int dlmstp_send_pdu(BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,   /* network information */
    uint8_t * pdu,              /* any data to be sent - may be null */
    unsigned pdu_len)
{                               /* number of bytes of data */
    bool status;
    int bytes_sent = 0;
    int npdu_len = 0;
    uint8_t frame_type = 0;
    uint8_t destination = 0;        /* destination address */
    BACNET_ADDRESS src;

    if (Transmit_Buffer.ready == false) {
        if (npdu_data->confirmed_message)
            frame_type = FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY;
        else
            frame_type = FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY;

        /* load destination MAC address */
        if (dest && dest->mac_len == 1) {
            destination = dest->mac[0];
        } else {
            #if PRINT_ENABLED
            fprintf(stderr, "mstp: invalid destination MAC address!\n");
            #endif
            return -2;
        }
        dlmstp_get_my_address(&src);
        npdu_len = npdu_encode_pdu(&PDU_Buffer[0], dest, &src, npdu_data);
        if ((8 + npdu_len + pdu_len) > MAX_MPDU) {
            #if PRINT_ENABLED
            fprintf(stderr, "mstp: PDU is too big to send!\n");
            #endif
            return -4;
        }
        memcpy(&PDU_Buffer[npdu_len], pdu, pdu_len);
/* copies the PDU to a buffer while calculating its CRC checksum
   The CRC checksum is appended to the last two octets. */
unsigned MSTP_Copy_PDU_CRC(uint8_t * buffer,    /* where frame is loaded */
    unsigned buffer_len,        /* amount of space available */
    uint16_t crc16,    /* used to calculate the crc value */
    uint8_t * data,             /* any data to be sent - may be null */
    unsigned data_len);
        
        bytes_sent = MSTP_Create_Frame(
            &Transmit_Buffer.pdu[0],
            sizeof(Transmit_Buffer.pdu),
            frame_type,
            destination,
            MSTP_Port.This_Station,
            &PDU_Buffer[0],
            npdu_len + pdu_len);
        Transmit_Buffer.ready = true;
    }

    return bytes_sent;
}

/* function for MS/TP to use to get a packet to transmit
   returns the number of bytes in the packet, or zero if none. */
int dlmstp_get_transmit_pdu(BACNET_ADDRESS * dest,      /* destination address */
    uint8_t * pdu)
{                               /* any data to be sent - may be null */
    bool status;
    DLMSTP_PACKET *packet;
    unsigned pdu_len = 0;

    if (Transmit_Buffer.ready) {
        memmove(dest, &packet->address, sizeof(packet->address));
        pdu_len = packet->pdu_len;
        memmove(pdu, packet->pdu, sizeof(packet.pdu));
    }

    return pdu_len;
}

int dlmstp_set_transmit_pdu_ready(bool ready)
{
    Transmit_Buffer.ready = ready;
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
    if (!Ringbuf_Empty(&Receive_Buffer)) {
        packet = (char *) Ringbuf_Pop_Front(&Receive_Buffer);
        memmove(src, &packet->address, sizeof(packet->address));
        pdu_len = packet->pdu_len;
        memmove(pdu, packet->pdu, max_pdu);
    }

    return pdu_len;
}

/* for the MS/TP state machine to use for putting received data */
uint16_t dlmstp_put_receive(BACNET_ADDRESS * src,       /* source address */
    uint8_t * pdu,              /* PDU data */
    uint16_t pdu_len)
{
    bool status;
    int bytes_put = 0;

    memmove(&Temp_Packet.address, src, sizeof(Temp_Packet.address));
    Temp_Packet.pdu_len = pdu_len;
    memmove(Temp_Packet.pdu, pdu, sizeof(Temp_Packet.pdu));
    status = Ringbuf_Put(&Receive_Buffer, &Temp_Packet);
    if (status)
        bytes_put = pdu_len;

    return bytes_put;
}

void dlmstp_set_my_address(uint8_t mac_address)
{
    /* FIXME: Master Nodes can only have address 1-127 */
    MSTP_Port.This_Station = mac_address;

    return;
}

/* This parameter represents the value of the Max_Info_Frames property of */
/* the node's Device object. The value of Max_Info_Frames specifies the */
/* maximum number of information frames the node may send before it must */
/* pass the token. Max_Info_Frames may have different values on different */
/* nodes. This may be used to allocate more or less of the available link */
/* bandwidth to particular nodes. If Max_Info_Frames is not writable in a */
/* node, its value shall be 1. */
void dlmstp_set_max_info_frames(unsigned max_info_frames)
{
    MSTP_Port.Nmax_info_frames = max_info_frames;

    return;
}

unsigned dlmstp_max_info_frames(void)
{
    return MSTP_Port.Nmax_info_frames;
}

/* This parameter represents the value of the Max_Master property of the */
/* node's Device object. The value of Max_Master specifies the highest */
/* allowable address for master nodes. The value of Max_Master shall be */
/* less than or equal to 127. If Max_Master is not writable in a node, */
/* its value shall be 127. */
void dlmstp_set_max_info_frames(uint8_t max_master)
{
    MSTP_Port.Nmax_master = max_master;

    return;
}

uint8_t dlmstp_max_master(void)
{
    return MSTP_Port.Nmax_master;
}

void dlmstp_get_my_address(BACNET_ADDRESS * my_address)
{
    my_address->mac_len = 1;
    my_address->mac[0] = MSTP_Port.This_Station;
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
