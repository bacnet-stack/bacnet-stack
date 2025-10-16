/**
 * @file
 * @brief Provides Linux-specific DataLink functions for MS/TP.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaddr.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/basic/sys/debug.h"
/* port specific */
#include "dlmstp_port.h"
#include "rs485.h"
/* OS Specific include */
#include "bacport.h"

#define BACNET_PDU_CONTROL_BYTE_OFFSET 1
#define BACNET_DATA_EXPECTING_REPLY_BIT 2
#define BACNET_DATA_EXPECTING_REPLY(control) \
    ((control & (1 << BACNET_DATA_EXPECTING_REPLY_BIT)) > 0)

#define INCREMENT_AND_LIMIT_UINT16(x) \
    {                                 \
        if (x < 0xFFFF)               \
            x++;                      \
    }
static uint32_t Timer_Silence(void *poPort)
{
    int32_t res;
    struct timeval now, tmp_diff;
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return -1;
    }
    poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return -1;
    }

    gettimeofday(&now, NULL);
    timersub(&poSharedData->start, &now, &tmp_diff);
    res = ((tmp_diff.tv_sec) * 1000 + (tmp_diff.tv_usec) / 1000);

    return (res >= 0 ? res : -res);
}

static void Timer_Silence_Reset(void *poPort)
{
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return;
    }
    poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return;
    }

    gettimeofday(&poSharedData->start, NULL);
}

static void get_abstime(struct timespec *abstime, unsigned long milliseconds)
{
    struct timeval now, offset, result;

    gettimeofday(&now, NULL);
    offset.tv_sec = 0;
    offset.tv_usec = milliseconds * 1000;
    timeradd(&now, &offset, &result);
    abstime->tv_sec = result.tv_sec;
    abstime->tv_nsec = result.tv_usec * 1000;
}

void dlmstp_cleanup(void *poPort)
{
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return;
    }
    poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return;
    }

    /* restore the old port settings */
    termios2_tcsetattr(
        poSharedData->RS485_Handle, TCSANOW, &poSharedData->RS485_oldtio2);
    close(poSharedData->RS485_Handle);

    pthread_cond_destroy(&poSharedData->Received_Frame_Flag);
    sem_destroy(&poSharedData->Receive_Packet_Flag);
    pthread_cond_destroy(&poSharedData->Master_Done_Flag);
    pthread_mutex_destroy(&poSharedData->Received_Frame_Mutex);
    pthread_mutex_destroy(&poSharedData->Master_Done_Mutex);
}

/* returns number of bytes sent on success, zero on failure */
int dlmstp_send_pdu(
    void *poPort,
    BACNET_ADDRESS *dest, /* destination address */
    uint8_t *pdu, /* any data to be sent - may be null */
    unsigned pdu_len)
{ /* number of bytes of data */
    int bytes_sent = 0;
    struct mstp_pdu_packet *pkt;
    unsigned i = 0;
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return 0;
    }
    poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return 0;
    }

    pkt = (struct mstp_pdu_packet *)Ringbuf_Data_Peek(&poSharedData->PDU_Queue);
    if (pkt) {
        pkt->data_expecting_reply =
            BACNET_DATA_EXPECTING_REPLY(pdu[BACNET_PDU_CONTROL_BYTE_OFFSET]);
        for (i = 0; i < pdu_len; i++) {
            pkt->buffer[i] = pdu[i];
        }
        pkt->length = pdu_len;
        pkt->destination_mac = dest->mac[0];
        if (Ringbuf_Data_Put(&poSharedData->PDU_Queue, (uint8_t *)pkt)) {
            bytes_sent = pdu_len;
        }
    }

    return bytes_sent;
}

uint16_t dlmstp_receive(
    void *poPort,
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* PDU data */
    uint16_t max_pdu, /* amount of space available in the PDU  */
    unsigned timeout)
{ /* milliseconds to wait for a packet */
    uint16_t pdu_len = 0;
    struct timespec abstime;
    int rv = 0;
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return 0;
    }
    poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return 0;
    }
    (void)max_pdu;
    /* see if there is a packet available, and a place
       to put the reply (if necessary) and process it */
    get_abstime(&abstime, timeout);
    rv = sem_timedwait(&poSharedData->Receive_Packet_Flag, &abstime);
    if (rv == 0) {
        if (poSharedData->Receive_Packet.ready) {
            if (poSharedData->Receive_Packet.pdu_len) {
                poSharedData->MSTP_Packets++;
                if (src) {
                    memmove(
                        src, &poSharedData->Receive_Packet.address,
                        sizeof(poSharedData->Receive_Packet.address));
                }
                if (pdu) {
                    memmove(
                        pdu, &poSharedData->Receive_Packet.pdu,
                        sizeof(poSharedData->Receive_Packet.pdu));
                }
                pdu_len = poSharedData->Receive_Packet.pdu_len;
            }
            poSharedData->Receive_Packet.ready = false;
        }
    }

    return pdu_len;
}

static void *dlmstp_receive_fsm_task(void *pArg)
{
    bool received_frame;
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)pArg;
    if (!mstp_port) {
        return NULL;
    }

    poSharedData =
        (SHARED_MSTP_DATA *)((struct mstp_port_struct_t *)pArg)->UserData;
    if (!poSharedData) {
        return NULL;
    }

    for (;;) {
        /* only do receive state machine while we don't have a frame */
        if ((mstp_port->ReceivedValidFrame == false) &&
            (mstp_port->ReceivedValidFrameNotForUs == false) &&
            (mstp_port->ReceivedInvalidFrame == false)) {
            do {
                RS485_Check_UART_Data(mstp_port);
                MSTP_Receive_Frame_FSM((struct mstp_port_struct_t *)pArg);
                received_frame = mstp_port->ReceivedValidFrame ||
                    mstp_port->ReceivedValidFrameNotForUs ||
                    mstp_port->ReceivedInvalidFrame;
                if (received_frame) {
                    pthread_cond_signal(&poSharedData->Received_Frame_Flag);
                    break;
                }
            } while (mstp_port->DataAvailable);
        }
    }

    return NULL;
}

static void *dlmstp_master_fsm_task(void *pArg)
{
    uint32_t silence = 0;
    bool run_master = false;
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)pArg;
    if (!mstp_port) {
        return NULL;
    }

    poSharedData =
        (SHARED_MSTP_DATA *)((struct mstp_port_struct_t *)pArg)->UserData;
    if (!poSharedData) {
        return NULL;
    }

    for (;;) {
        if (mstp_port->ReceivedValidFrame == false &&
            mstp_port->ReceivedValidFrameNotForUs == false &&
            mstp_port->ReceivedInvalidFrame == false) {
            RS485_Check_UART_Data(mstp_port);
            MSTP_Receive_Frame_FSM(mstp_port);
        }
        if (mstp_port->ReceivedValidFrame || mstp_port->ReceivedInvalidFrame ||
            mstp_port->ReceivedValidFrameNotForUs) {
            run_master = true;
        } else {
            silence = mstp_port->SilenceTimer(NULL);
            switch (mstp_port->master_state) {
                case MSTP_MASTER_STATE_IDLE:
                    if (silence >= Tno_token) {
                        run_master = true;
                    }
                    break;
                case MSTP_MASTER_STATE_WAIT_FOR_REPLY:
                    if (silence >= mstp_port->Treply_timeout) {
                        run_master = true;
                    }
                    break;
                case MSTP_MASTER_STATE_POLL_FOR_MASTER:
                    if (silence >= mstp_port->Tusage_timeout) {
                        run_master = true;
                    }
                    break;
                default:
                    run_master = true;
                    break;
            }
        }
        if (run_master) {
            if (mstp_port->This_Station <= DEFAULT_MAX_MASTER) {
                while (MSTP_Master_Node_FSM(mstp_port)) {
                    /* do nothing while immediate transitioning */
                }
            } else if (mstp_port->This_Station < 255) {
                MSTP_Slave_Node_FSM(mstp_port);
            }
        }
    }

    return NULL;
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

/* for the MS/TP state machine to use for putting received data */
uint16_t MSTP_Put_Receive(struct mstp_port_struct_t *mstp_port)
{
    uint16_t pdu_len = 0;
    SHARED_MSTP_DATA *poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;

    if (!poSharedData) {
        return 0;
    }

    if (!poSharedData->Receive_Packet.ready) {
        /* bounds check - maybe this should send an abort? */
        pdu_len = mstp_port->DataLength;
        if (pdu_len > sizeof(poSharedData->Receive_Packet.pdu)) {
            pdu_len = sizeof(poSharedData->Receive_Packet.pdu);
        }
        memmove(
            (void *)&poSharedData->Receive_Packet.pdu[0],
            (void *)&mstp_port->InputBuffer[0], pdu_len);
        dlmstp_fill_bacnet_address(
            &poSharedData->Receive_Packet.address, mstp_port->SourceAddress);
        poSharedData->Receive_Packet.pdu_len = mstp_port->DataLength;
        poSharedData->Receive_Packet.ready = true;
        sem_post(&poSharedData->Receive_Packet_Flag);
    }

    return pdu_len;
}

/* for the MS/TP state machine to use for getting data to send */
/* Return: amount of PDU data */
uint16_t MSTP_Get_Send(struct mstp_port_struct_t *mstp_port, unsigned timeout)
{ /* milliseconds to wait for a packet */
    uint16_t pdu_len = 0;
    uint8_t frame_type = 0;
    struct mstp_pdu_packet *pkt;
    SHARED_MSTP_DATA *poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;

    if (!poSharedData) {
        return 0;
    }

    (void)timeout;
    if (Ringbuf_Empty(&poSharedData->PDU_Queue)) {
        return 0;
    }
    pkt = (struct mstp_pdu_packet *)Ringbuf_Peek(&poSharedData->PDU_Queue);
    if (pkt->data_expecting_reply) {
        frame_type = FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY;
    } else {
        frame_type = FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY;
    }
    /* convert the PDU into the MSTP Frame */
    pdu_len = MSTP_Create_Frame(
        &mstp_port->OutputBuffer[0], /* <-- loading this */
        mstp_port->OutputBufferSize, frame_type, pkt->destination_mac,
        mstp_port->This_Station, (uint8_t *)&pkt->buffer[0], pkt->length);
    (void)Ringbuf_Pop(&poSharedData->PDU_Queue, NULL);

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
}

/* Get the reply to a DATA_EXPECTING_REPLY frame, or nothing */
uint16_t MSTP_Get_Reply(struct mstp_port_struct_t *mstp_port, unsigned timeout)
{ /* milliseconds to wait for a packet */
    uint16_t pdu_len = 0; /* return value */
    bool matched = false;
    uint8_t frame_type = 0;
    struct mstp_pdu_packet *pkt;
    SHARED_MSTP_DATA *poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;

    (void)timeout;
    if (!poSharedData) {
        return 0;
    }
    if (Ringbuf_Empty(&poSharedData->PDU_Queue)) {
        return 0;
    }
    pkt = (struct mstp_pdu_packet *)Ringbuf_Peek(&poSharedData->PDU_Queue);
    /* is this the reply to the DER? */
    matched = npdu_data_expecting_reply_compare(
        &mstp_port->InputBuffer[0], mstp_port->DataLength,
        (uint8_t *)&pkt->buffer[0], pkt->length);
    if (!matched) {
        /* Walk the rest of the ring buffer to see if we can find a match */
        while (!matched &&
               (pkt = (struct mstp_pdu_packet *)Ringbuf_Peek_Next(
                    &poSharedData->PDU_Queue, (uint8_t *)pkt)) != NULL) {
            matched = npdu_data_expecting_reply_compare(
                &mstp_port->InputBuffer[0], mstp_port->DataLength,
                (uint8_t *)&pkt->buffer[0], pkt->length);
        }
        if (!matched) {
            /* Still didn't find a match so just bail out */
            return 0;
        }
    }
    if (pkt->data_expecting_reply) {
        frame_type = FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY;
    } else {
        frame_type = FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY;
    }
    /* convert the PDU into the MSTP Frame */
    pdu_len = MSTP_Create_Frame(
        &mstp_port->OutputBuffer[0], /* <-- loading this */
        mstp_port->OutputBufferSize, frame_type, pkt->destination_mac,
        mstp_port->This_Station, (uint8_t *)&pkt->buffer[0], pkt->length);
    /* This will pop the element no matter where we found it */
    (void)Ringbuf_Pop_Element(&poSharedData->PDU_Queue, (uint8_t *)pkt, NULL);

    return pdu_len;
}

void dlmstp_set_mac_address(void *poPort, uint8_t mac_address)
{
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return;
    }
    /* Master Nodes can only have address 0-127 */
    if (mac_address <= 127) {
        mstp_port->This_Station = mac_address;
        if (mac_address > mstp_port->Nmax_master) {
            dlmstp_set_max_master(mstp_port, mac_address);
        }
    }

    return;
}

uint8_t dlmstp_mac_address(void *poPort)
{
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return 0;
    }

    return mstp_port->This_Station;
}

/* This parameter represents the value of the Max_Info_Frames property of */
/* the node's Device object. The value of Max_Info_Frames specifies the */
/* maximum number of information frames the node may send before it must */
/* pass the token. Max_Info_Frames may have different values on different */
/* nodes. This may be used to allocate more or less of the available link */
/* bandwidth to particular nodes. If Max_Info_Frames is not writable in a */
/* node, its value shall be 1. */
void dlmstp_set_max_info_frames(void *poPort, uint8_t max_info_frames)
{
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;

    if (!mstp_port) {
        return;
    }
    if (max_info_frames >= 1) {
        mstp_port->Nmax_info_frames = max_info_frames;
    }

    return;
}

uint8_t dlmstp_max_info_frames(void *poPort)
{
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return 0;
    }

    return mstp_port->Nmax_info_frames;
}

/* This parameter represents the value of the Max_Master property of the */
/* node's Device object. The value of Max_Master specifies the highest */
/* allowable address for master nodes. The value of Max_Master shall be */
/* less than or equal to 127. If Max_Master is not writable in a node, */
/* its value shall be 127. */
void dlmstp_set_max_master(void *poPort, uint8_t max_master)
{
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return;
    }
    if (max_master <= 127) {
        if (mstp_port->This_Station <= max_master) {
            mstp_port->Nmax_master = max_master;
        }
    }

    return;
}

uint8_t dlmstp_max_master(void *poPort)
{
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;

    if (!mstp_port) {
        return 0;
    }

    return mstp_port->Nmax_master;
}

/* RS485 Baud Rate 9600, 19200, 38400, 57600, 115200 */
void dlmstp_set_baud_rate(void *poPort, uint32_t baud)
{
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return;
    }
    poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return;
    }

    switch (baud) {
        case 9600:
            poSharedData->RS485_Baud = B9600;
            break;
        case 19200:
            poSharedData->RS485_Baud = B19200;
            break;
        case 38400:
            poSharedData->RS485_Baud = B38400;
            break;
        case 57600:
            poSharedData->RS485_Baud = B57600;
            break;
        case 115200:
            poSharedData->RS485_Baud = B115200;
            break;
        default:
            break;
    }
}

uint32_t dlmstp_baud_rate(void *poPort)
{
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return false;
    }
    poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return false;
    }

    switch (poSharedData->RS485_Baud) {
        case B19200:
            return 19200;
        case B38400:
            return 38400;
        case B57600:
            return 57600;
        case B115200:
            return 115200;
        default:
        case B9600:
            return 9600;
    }
}

void dlmstp_get_my_address(void *poPort, BACNET_ADDRESS *my_address)
{
    int i = 0; /* counter */
    SHARED_MSTP_DATA *poSharedData;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return;
    }
    poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return;
    }
    my_address->mac_len = 1;
    my_address->mac[0] = mstp_port->This_Station;
    my_address->net = 0; /* local only, no routing */
    my_address->len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        my_address->adr[i] = 0;
    }

    return;
}

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

bool dlmstp_init(void *poPort, char *ifname)
{
    unsigned long hThread = 0;
    int rv = 0;
    SHARED_MSTP_DATA *poSharedData;
    struct termios2 newtio;
    struct mstp_port_struct_t *mstp_port = (struct mstp_port_struct_t *)poPort;
    if (!mstp_port) {
        return false;
    }

    poSharedData =
        (SHARED_MSTP_DATA *)((struct mstp_port_struct_t *)mstp_port)->UserData;
    if (!poSharedData) {
        return false;
    }

    poSharedData->RS485_Port_Name = ifname;
    /* initialize PDU queue */
    Ringbuf_Init(
        &poSharedData->PDU_Queue, (uint8_t *)&poSharedData->PDU_Buffer,
        sizeof(struct mstp_pdu_packet), MSTP_PDU_PACKET_COUNT);
    /* initialize packet queue */
    poSharedData->Receive_Packet.ready = false;
    poSharedData->Receive_Packet.pdu_len = 0;
    rv = sem_init(&poSharedData->Receive_Packet_Flag, 0, 0);
    if (rv != 0) {
        fprintf(
            stderr,
            "MS/TP Interface: %s\n cannot allocate PThread Condition.\n",
            ifname);
        exit(1);
    }

    printf("RS485: Initializing %s", poSharedData->RS485_Port_Name);
    /*
       Open device for reading and writing.
       Blocking mode - more CPU effecient
     */
    poSharedData->RS485_Handle = open(
        poSharedData->RS485_Port_Name,
        O_RDWR | O_NOCTTY | O_NONBLOCK /*| O_NDELAY */);
    if (poSharedData->RS485_Handle < 0) {
        perror(poSharedData->RS485_Port_Name);
        exit(-1);
    }
#if 0
    /* non blocking for the read */
    fcntl(poSharedData->RS485_Handle, F_SETFL, FNDELAY);
#else
    /* efficient blocking for the read */
    fcntl(poSharedData->RS485_Handle, F_SETFL, 0);
#endif
    /* save current serial port settings */
    termios2_tcgetattr(
        poSharedData->RS485_Handle, &poSharedData->RS485_oldtio2);
    /* clear struct for new port settings */
    memset(&newtio, 0, sizeof(newtio));
    /*
       BOTHER: Set bps rate.
       https://man7.org/linux/man-pages/man2/TCSETS.2const.html
       CRTSCTS : output hardware flow control (only used if the cable has
       all necessary lines. See sect. 7 of Serial-HOWTO)
       CLOCAL  : local connection, no modem control
       CREAD   : enable receiving characters
     */
    newtio.c_cflag =
        poSharedData->RS485MOD | CLOCAL | CREAD | BOTHER | (BOTHER << IBSHIFT);
    newtio.c_ispeed = poSharedData->RS485_Baud;
    newtio.c_ospeed = poSharedData->RS485_Baud;
    /* Raw input */
    newtio.c_iflag = 0;
    /* Raw output */
    newtio.c_oflag = 0;
    /* no processing */
    newtio.c_lflag = 0;
    /* activate the settings for the port after flushing I/O */
    termios2_tcsetattr(poSharedData->RS485_Handle, TCSAFLUSH, &newtio);
    /* flush any data waiting */
    usleep(200000);
    termios2_tcflush(poSharedData->RS485_Handle, TCIOFLUSH);
    /* ringbuffer */
    FIFO_Init(
        &poSharedData->Rx_FIFO, poSharedData->Rx_Buffer,
        sizeof(poSharedData->Rx_Buffer));
    printf("=success!\n");
    mstp_port->InputBuffer = &poSharedData->RxBuffer[0];
    mstp_port->InputBufferSize = sizeof(poSharedData->RxBuffer);
    mstp_port->OutputBuffer = &poSharedData->TxBuffer[0];
    mstp_port->OutputBufferSize = sizeof(poSharedData->TxBuffer);
    gettimeofday(&poSharedData->start, NULL);
    mstp_port->SilenceTimer = Timer_Silence;
    mstp_port->SilenceTimerReset = Timer_Silence_Reset;
    MSTP_Init(mstp_port);
    debug_fprintf(stderr, "MS/TP MAC: %02X\n", mstp_port->This_Station);
    debug_fprintf(stderr, "MS/TP Max_Master: %02X\n", mstp_port->Nmax_master);
    debug_fprintf(
        stderr, "MS/TP Max_Info_Frames: %u\n", mstp_port->Nmax_info_frames);
    rv = pthread_create(&hThread, NULL, dlmstp_master_fsm_task, mstp_port);
    if (rv != 0) {
        fprintf(stderr, "Failed to start Master Node FSM task\n");
    }

    /* You can try also this for thread. This here so we ignore
     * -Wunused-function compiler warning
     */
    dlmstp_receive_fsm_task(NULL);

    return true;
}
