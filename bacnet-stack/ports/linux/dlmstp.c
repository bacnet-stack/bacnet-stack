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
#include <stdlib.h>
#if PRINT_ENABLED
#include <stdio.h>
#endif
/* Linux Includes */
#include <sched.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
/* project includes */
#include "bacdef.h"
#include "mstp.h"
#include "dlmstp.h"
#include "rs485.h"
#include "npdu.h"

/* Number of MS/TP Packets Rx/Tx */
uint16_t MSTP_Packets = 0;

/* receive buffer */
static DLMSTP_PACKET Receive_Buffer;
/* temp buffer for NPDU insertion */
/* local MS/TP port data - shared with RS-485 */
volatile struct mstp_port_struct_t MSTP_Port;

#define INCREMENT_AND_LIMIT_UINT16(x) {if (x < 0xFFFF) x++;}

void dlmstp_millisecond_timer(void)
{
    INCREMENT_AND_LIMIT_UINT16(MSTP_Port.SilenceTimer);
}

static void *dlmstp_milliseconds_task(void *pArg)
{
    struct timespec timeOut,remains;

    timeOut.tv_sec = 0;
    timeOut.tv_nsec = 1000000; /* 1 millisecond */

    for (;;) {
        nanosleep(&timeOut, &remains);
        dlmstp_millisecond_timer();
    }

    return NULL;
}

void dlmstp_reinit(void)
{
    //RS485_Reinit();
    dlmstp_set_mac_address(DEFAULT_MAC_ADDRESS);
    dlmstp_set_max_info_frames(DEFAULT_MAX_INFO_FRAMES);
    dlmstp_set_max_master(DEFAULT_MAX_MASTER);
}

void dlmstp_cleanup(void)
{
    RS485_Cleanup();
    /* nothing to do for static buffers */
}

/* returns number of bytes sent on success, zero on failure */
int dlmstp_send_pdu(BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,              /* any data to be sent - may be null */
    unsigned pdu_len)
{                               /* number of bytes of data */
    int bytes_sent = 0;
    uint8_t destination = 0;    /* destination address */
    BACNET_ADDRESS src;

    if (MSTP_Port.TxReady == false) {
        if (npdu_data->data_expecting_reply)
            MSTP_Port.TxFrameType = FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY;
        else
            MSTP_Port.TxFrameType =
                FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY;

        /* load destination MAC address */
        if (dest && dest->mac_len == 1) {
            destination = dest->mac[0];
        } else {
            return -2;
        }
        dlmstp_get_my_address(&src);
        if ((8 /* header len */  + pdu_len) > MAX_MPDU) {
            return -4;
        }
        bytes_sent = MSTP_Create_Frame(
            (uint8_t *) & MSTP_Port.TxBuffer[0],
            sizeof(MSTP_Port.TxBuffer),
            MSTP_Port.TxFrameType,
            destination, MSTP_Port.This_Station, pdu, pdu_len);
        MSTP_Port.TxLength = bytes_sent;
        MSTP_Port.TxReady = true;
        MSTP_Packets++;
    }

    return bytes_sent;
}

static void *dlmstp_receive_fsm_task(void *pArg)
{
    uint8_t bytes_remaining;
    bool received_frame;

    for (;;) {
        /* only do receive state machine while we don't have a frame */
        if ((MSTP_Port.ReceivedValidFrame == false) &&
            (MSTP_Port.ReceivedInvalidFrame == false)) {
            do {
                bytes_remaining = RS485_Check_UART_Data(&MSTP_Port);
                MSTP_Receive_Frame_FSM(&MSTP_Port);
                received_frame = MSTP_Port.ReceivedValidFrame ||
                    MSTP_Port.ReceivedInvalidFrame;
                if (received_frame)
                    break;
            } while (bytes_remaining);
        }
    }
}

static void *dlmstp_master_fsm_task(void *pArg)
{
    for (;;) {
        /* only do master state machine while rx is idle */
        if (MSTP_Port.receive_state == MSTP_RECEIVE_STATE_IDLE) {
            while (MSTP_Master_Node_FSM(&MSTP_Port)) {
                sched_yield();
            }
        }
    }
}

/* copy the packet if one is received.
   Return the length of the packet */
uint16_t dlmstp_receive(
    BACNET_ADDRESS * src, /* source address */
    uint8_t * pdu, /* PDU data */
    uint16_t max_pdu, /* amount of space available in the PDU  */
    unsigned timeout) /* milliseconds to wait for a packet */
{
    uint16_t len = 0;
    /* see if there is a packet available, and a place
        to put the reply (if necessary) and process it */
    if (Receive_Buffer.ready && !MSTP_Port.TxReady) {
        if (Receive_Buffer.pdu_len) {
            MSTP_Packets++;
            len = Receive_Buffer.pdu_len;
            memcpy(src,&Receive_Buffer.address,sizeof(Receive_Buffer.address));
            /* FIXME: check pdu_len and max_pdu */
            memcpy(pdu,&Receive_Buffer.pdu[0],len);
        }
        Receive_Buffer.ready = false;
    } else {
        sched_yield();
    }

    return len;
}


void dlmstp_fill_bacnet_address(BACNET_ADDRESS * src, uint8_t mstp_address)
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

/* for the MS/TP state machine to use for putting received data */
uint16_t dlmstp_put_receive(uint8_t src,        /* source MS/TP address */
    uint8_t * pdu,              /* PDU data */
    uint16_t pdu_len)
{                               /* amount of PDU data */
    /* PDU is already in the Receive_Buffer */
    dlmstp_fill_bacnet_address(&Receive_Buffer.address, src);
    Receive_Buffer.pdu_len = pdu_len;
    Receive_Buffer.ready = true;

    return pdu_len;
}

void dlmstp_set_mac_address(uint8_t mac_address)
{
    /* Master Nodes can only have address 0-127 */
    if (mac_address <= 127) {
        MSTP_Port.This_Station = mac_address;
        /* FIXME: implement your data storage */
        /* I2C_Write_Byte(
           EEPROM_DEVICE_ADDRESS,
           mac_address,
           EEPROM_MSTP_MAC_ADDR); */
        if (mac_address > MSTP_Port.Nmax_master)
            dlmstp_set_max_master(mac_address);
    }

    return;
}

uint8_t dlmstp_my_address(void)
{
    return MSTP_Port.This_Station;
}

/* This parameter represents the value of the Max_Info_Frames property of */
/* the node's Device object. The value of Max_Info_Frames specifies the */
/* maximum number of information frames the node may send before it must */
/* pass the token. Max_Info_Frames may have different values on different */
/* nodes. This may be used to allocate more or less of the available link */
/* bandwidth to particular nodes. If Max_Info_Frames is not writable in a */
/* node, its value shall be 1. */
void dlmstp_set_max_info_frames(uint8_t max_info_frames)
{
    if (max_info_frames >= 1) {
        MSTP_Port.Nmax_info_frames = max_info_frames;
        /* FIXME: implement your data storage */
        /* I2C_Write_Byte(
           EEPROM_DEVICE_ADDRESS,
           (uint8_t)max_info_frames,
           EEPROM_MSTP_MAX_INFO_FRAMES_ADDR); */
    }

    return;
}

uint8_t dlmstp_max_info_frames(void)
{
    return MSTP_Port.Nmax_info_frames;
}

/* This parameter represents the value of the Max_Master property of the */
/* node's Device object. The value of Max_Master specifies the highest */
/* allowable address for master nodes. The value of Max_Master shall be */
/* less than or equal to 127. If Max_Master is not writable in a node, */
/* its value shall be 127. */
void dlmstp_set_max_master(uint8_t max_master)
{
    if (max_master <= 127) {
        if (MSTP_Port.This_Station <= max_master) {
            MSTP_Port.Nmax_master = max_master;
            /* FIXME: implement your data storage */
            /* I2C_Write_Byte(
               EEPROM_DEVICE_ADDRESS,
               max_master,
               EEPROM_MSTP_MAX_MASTER_ADDR); */
        }
    }

    return;
}

uint8_t dlmstp_max_master(void)
{
    return MSTP_Port.Nmax_master;
}

void dlmstp_get_my_address(BACNET_ADDRESS * my_address)
{
    int i = 0;                  /* counter */

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
        dest->len = 0; /* always zero when DNET is broadcast */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }

    return;
}

bool dlmstp_init(char *ifname)
{
    int rc = 0;
    pthread_t hThread;

    /* initialize buffer */
    Receive_Buffer.ready = false;
    Receive_Buffer.pdu_len = 0;
    /* initialize hardware */
    if (ifname)
        RS485_Set_Interface(ifname);
    RS485_Initialize();
    MSTP_Init(&MSTP_Port);
#if 0
    /* FIXME: implement your data storage */
    if (data <= 127)
        MSTP_Port.This_Station = data;
    else
        dlmstp_set_mac_address(DEFAULT_MAC_ADDRESS);
    if ((data <= 127) && (data >= MSTP_Port.This_Station))
        MSTP_Port.Nmax_master = data;
    else
        dlmstp_set_max_master(DEFAULT_MAX_MASTER);
    if (data >= 1)
        MSTP_Port.Nmax_info_frames = data;
    else
        dlmstp_set_max_info_frames(DEFAULT_MAX_INFO_FRAMES);
#endif

    /* start our MilliSec task */
    rc = pthread_create(&hThread, NULL, dlmstp_milliseconds_task, NULL);
    rc = pthread_create(&hThread, NULL, dlmstp_receive_fsm_task, NULL);
    rc = pthread_create(&hThread, NULL, dlmstp_master_fsm_task, NULL);

    return 1;
}

#ifdef TEST_DLMSTP
#include <stdio.h>

void npdu_handler(
    BACNET_ADDRESS * src,     /* source address */
    uint8_t * pdu,          /* PDU data */
    uint16_t pdu_len)      /* length PDU  */
{
    (void)src;
    (void)pdu;
    (void)pdu_len;
    fprintf(stderr, "NPDU: received PDU!\n");
}

int main(int argc, char *argv[])
{
    struct timespec timeOut,remains;

    timeOut.tv_sec = 1;
    timeOut.tv_nsec = 0; /* 1 millisecond */

    /* argv has the "/dev/ttyS0" or some other device */
    if (argc > 1) {
        RS485_Set_Interface(argv[1]);
    }
    RS485_Set_Baud_Rate(38400);
    dlmstp_set_mac_address(0x05);
    dlmstp_set_max_info_frames(DEFAULT_MAX_INFO_FRAMES);
    dlmstp_set_max_master(DEFAULT_MAX_MASTER);
    dlmstp_init();
    /* forever task */
    for (;;) {
#if 0
        MSTP_Create_And_Send_Frame(
            &MSTP_Port,
            FRAME_TYPE_TEST_REQUEST,
            MSTP_Port.SourceAddress,
            MSTP_Port.This_Station,
            NULL, 0);
#endif
        nanosleep(&timeOut, &remains);
    }

    return 0;
}
#endif
