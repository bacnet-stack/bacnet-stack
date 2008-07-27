/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
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
#include <stdio.h>
/* Linux Includes */
#include <sched.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

/* project includes */
#include "bacdef.h"
#include "bacaddr.h"
#include "mstp.h"
#include "dlmstp.h"
#include "rs485.h"
#include "npdu.h"
#include "bits.h"

/* Number of MS/TP Packets Rx/Tx */
uint16_t MSTP_Packets = 0;
/* Posix queues are used for NPDU queues */
static mqd_t NPDU_Receive_Queue = -1;
static mqd_t NPDU_Transmit_Queue = -1;
/* local MS/TP port data - shared with RS-485 */
volatile struct mstp_port_struct_t MSTP_Port;
/* buffers needed by mstp port struct */
static uint8_t TxBuffer[MAX_MPDU];
static uint8_t RxBuffer[MAX_MPDU];
/* Timer that indicates line silence - and functions */
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

static void *dlmstp_milliseconds_task(
    void *pArg)
{
    struct timespec timeOut, remains;

    timeOut.tv_sec = 0;
    timeOut.tv_nsec = 1000000;  /* 1 millisecond */

    for (;;) {
        nanosleep(&timeOut, &remains);
        dlmstp_millisecond_timer();
    }

    return NULL;
}

void dlmstp_reinit(
    void)
{
    /*RS485_Reinit(); */
    dlmstp_set_mac_address(DEFAULT_MAC_ADDRESS);
    dlmstp_set_max_info_frames(DEFAULT_MAX_INFO_FRAMES);
    dlmstp_set_max_master(DEFAULT_MAX_MASTER);
}

/* returns number of bytes sent on success, zero on failure */
int dlmstp_send_pdu(
    BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,      /* any data to be sent - may be null */
    unsigned pdu_len)
{       /* number of bytes of data */
    DLMSTP_PACKET packet;
    int bytes_sent = 0;
    mqd_t rc = 0;

    if (pdu_len) {
#if PRINT_ENABLED
        fprintf(stderr, "MS/TP: sending packet\n");
#endif
        if (npdu_data->data_expecting_reply) {
            packet.frame_type = FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY;
        } else {
            packet.frame_type = FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY;
        }
        packet.pdu_len = pdu_len;
        memmove(&packet.pdu[0], &pdu[0], pdu_len);
        memmove(&packet.address, dest, sizeof(packet.address));
        rc = mq_send(NPDU_Transmit_Queue, (const char *)&packet, sizeof(packet), 0);
        if (rc > 0)
            bytes_sent = rc;
    }

    return bytes_sent;
}

/* copy the packet if one is received.
   Return the length of the packet */
uint16_t dlmstp_receive(
    BACNET_ADDRESS * src,       /* source address */
    uint8_t * pdu,      /* PDU data */
    uint16_t max_pdu,   /* amount of space available in the PDU  */
    unsigned timeout)
{       /* milliseconds to wait for a packet */
    uint16_t pdu_len = 0;
    mqd_t received_bytes = 0;
    DLMSTP_PACKET packet;
    struct timespec queue_timeout = {0};
    time_t epoch_time = 0;
    unsigned msg_prio = 0;
    char buffer[sizeof(struct dlmstp_packet)+1];

    if (NPDU_Receive_Queue == -1) {
        return 0;
    }

    /* configure queue timeout */
    if (timeout >= 1000) {
        queue_timeout.tv_sec = timeout / 1000;
        queue_timeout.tv_nsec =
            1000000L * (timeout - queue_timeout.tv_sec * 1000);
    } else {
        queue_timeout.tv_sec = 0;
        queue_timeout.tv_nsec = 1000000L * timeout;
    }
    /* get current time */
    epoch_time = time(NULL);
    queue_timeout.tv_sec += epoch_time;   

    received_bytes = mq_timedreceive(
        NPDU_Receive_Queue,
        buffer,
        sizeof(buffer),
        &msg_prio,
        &queue_timeout);

    /* See if there is a problem */
    if (received_bytes == -1) {
        /* EAGAIN Non-blocking I/O has been selected  */
        /* using O_NONBLOCK and no data */
        /* was immediately available for reading. */
        if ((errno != EAGAIN) && (errno != ETIMEDOUT)) {
#if PRINT_ENABLED
            fprintf(stderr, "MS/TP: NPDU Receive: %s\n",
                strerror(errno));
#endif
        } 
        return 0;
    }

    if (received_bytes == 0)
        return 0;

    /* copy the buffer into the PDU */
    memmove(&packet,buffer,sizeof(packet));
    pdu_len = packet.pdu_len;
    memmove(&pdu[0], &packet.pdu[0], pdu_len);
    memmove(src, &packet.address, sizeof(packet.address));
    /* not used in this scheme */
    packet.ready = true;

    return pdu_len;
}

static void *dlmstp_fsm_receive_task(
    void *pArg)
{
    bool received_frame = false;

    for (;;) {
        /* only do receive state machine while we don't have a frame */
        if ((MSTP_Port.ReceivedValidFrame == false) &&
            (MSTP_Port.ReceivedInvalidFrame == false)) {
            do {
                /* using blocking read with timeout on serial port */
                RS485_Check_UART_Data(&MSTP_Port);
                MSTP_Receive_Frame_FSM(&MSTP_Port);
                received_frame = MSTP_Port.ReceivedValidFrame ||
                    MSTP_Port.ReceivedInvalidFrame;
                if (received_frame)
                    break;
            } while (MSTP_Port.DataAvailable);
        }
    }

    return NULL;
}

static void *dlmstp_fsm_master_task(
    void *pArg)
{
    struct timespec timeOut, remains;

    timeOut.tv_sec = 0;
    timeOut.tv_nsec = 100000;   /* .1 millisecond */

    for (;;) {
        nanosleep(&timeOut, &remains);
        /* only do master state machine while rx is idle */
        if (MSTP_Port.receive_state == MSTP_RECEIVE_STATE_IDLE) {
            while (MSTP_Master_Node_FSM(&MSTP_Port)) {
                sched_yield();
            }
        }
    }

    return NULL;
}

void dlmstp_fill_bacnet_address(
    BACNET_ADDRESS * src,
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

/* for the MS/TP state machine to use for putting received data */
uint16_t MSTP_Put_Receive(
    volatile struct mstp_port_struct_t *mstp_port)
{
    DLMSTP_PACKET packet;
    uint16_t pdu_len = mstp_port->DataLength;

    /* bounds check - maybe this should send an abort? */
    if (pdu_len > sizeof(packet.pdu))
        pdu_len = sizeof(packet.pdu);
    if (pdu_len) {
#if PRINT_ENABLED
        fprintf(stderr,"MSTP: packet from FSM.\n");
#endif
        MSTP_Packets++;
        memmove(&packet.pdu[0], (void *) &mstp_port->InputBuffer[0], pdu_len);
        dlmstp_fill_bacnet_address(&packet.address, mstp_port->SourceAddress);
        packet.pdu_len = pdu_len;
        /* ready is not used in this scheme */
        packet.ready = true;
        mq_send(NPDU_Receive_Queue, (const char *)&packet, sizeof(packet), 0);
    }

    return pdu_len;
}

/* for the MS/TP state machine to use for getting data to send */
/* Return: amount of PDU data */
int dlmstp_get_transmit_packet(
    DLMSTP_PACKET * packet,
    unsigned timeout)
{       /* milliseconds to wait for a packet */
    int received_bytes = 0;     /* return value */
    struct timespec queue_timeout = {0};
    time_t epoch_time = 0;
    unsigned msg_prio = 0;
    char buffer[sizeof(struct dlmstp_packet)+1];

    /* Make sure the socket is open */
    if (NPDU_Transmit_Queue == -1)
        return 0;

    if (timeout >= 1000) {
        queue_timeout.tv_sec = timeout / 1000;
        queue_timeout.tv_nsec =
            1000000L * (timeout - queue_timeout.tv_sec * 1000);
    } else {
        queue_timeout.tv_sec = 0;
        queue_timeout.tv_nsec = 1000000L * timeout;
    }
    /* get current time */
    epoch_time = time(NULL);
    queue_timeout.tv_sec += epoch_time;   

    received_bytes = mq_timedreceive(
        NPDU_Transmit_Queue,
        buffer,
        sizeof(buffer),
        &msg_prio,
        &queue_timeout);

    /* See if there is a problem */
    if (received_bytes == -1) {
        /* EAGAIN Non-blocking I/O has been selected  */
        /* using O_NONBLOCK and no data */
        /* was immediately available for reading. */
        if ((errno != EAGAIN) && (errno != ETIMEDOUT)) {
#if PRINT_ENABLED
            fprintf(stderr, "MS/TP: Read error in Transmit_Client packet: %s\n",
                strerror(errno));
#endif
        }
        return 0;
    }
    memmove(packet,buffer,sizeof(packet));

    return (received_bytes);
}

/* for the MS/TP state machine to use for getting data to send */
/* Return: amount of PDU data */
uint16_t MSTP_Get_Send(
    volatile struct mstp_port_struct_t * mstp_port,
    unsigned timeout)
{       /* milliseconds to wait for a packet */
    uint16_t pdu_len = 0;       /* return value */
    int received_bytes = 0;
    DLMSTP_PACKET packet;
    uint8_t destination = 0;    /* destination address */

    received_bytes = dlmstp_get_transmit_packet(&packet, timeout);
    if (received_bytes <= 0)
        return 0;
    /* load destination MAC address */
    if (packet.address.mac_len == 1) {
        destination = packet.address.mac[0];
    } else {
        return 0;
    }
    if ((MAX_HEADER + packet.pdu_len) > MAX_MPDU) {
        return 0;
    }
#if PRINT_ENABLED
    fprintf(stderr,"MS/TP: sending packet to FSM.\n");
#endif
    /* convert the PDU into the MSTP Frame */
    pdu_len = MSTP_Create_Frame(&mstp_port->OutputBuffer[0],    /* <-- loading this */
        mstp_port->OutputBufferSize, packet.frame_type, destination,
        mstp_port->This_Station, &packet.pdu[0], packet.pdu_len);

    return pdu_len;
}

bool dlmstp_compare_data_expecting_reply(
    uint8_t * request_pdu,
    uint16_t request_pdu_len,
    uint8_t src_address,
    uint8_t * reply_pdu,
    uint16_t reply_pdu_len,
    BACNET_ADDRESS * dest_address)
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

    /* decode the request data */
    request.address.mac[0] = src_address;
    request.address.mac_len = 1;
    offset =
        npdu_decode(&request_pdu[0], NULL, &request.address,
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
    if (request_pdu[offset] & BIT3)
        request.service_choice = request_pdu[offset + 5];
    else
        request.service_choice = request_pdu[offset + 3];
    /* decode the reply data */
    bacnet_address_copy(&reply.address, dest_address);
    offset =
        npdu_decode(&reply_pdu[0], &reply.address, NULL, &reply.npdu_data);
    if (reply.npdu_data.network_layer_message) {
        return false;
    }
    /* reply could be a lot of things:
       confirmed, simple ack, abort, reject, error */
    reply.pdu_type = reply_pdu[offset] & 0xF0;
    switch (reply.pdu_type) {
        case PDU_TYPE_CONFIRMED_SERVICE_REQUEST:
            reply.invoke_id = reply_pdu[offset + 2];
            /* segmented message? */
            if (reply_pdu[offset] & BIT3)
                reply.service_choice = reply_pdu[offset + 5];
            else
                reply.service_choice = reply_pdu[offset + 3];
            break;
        case PDU_TYPE_SIMPLE_ACK:
            reply.invoke_id = reply_pdu[offset + 1];
            reply.service_choice = reply_pdu[offset + 2];
            break;
        case PDU_TYPE_COMPLEX_ACK:
            reply.invoke_id = reply_pdu[offset + 1];
            /* segmented message? */
            if (reply_pdu[offset] & BIT3)
                reply.service_choice = reply_pdu[offset + 4];
            else
                reply.service_choice = reply_pdu[offset + 2];
            break;
        case PDU_TYPE_ERROR:
            reply.invoke_id = reply_pdu[offset + 1];
            reply.service_choice = reply_pdu[offset + 2];
            break;
        case PDU_TYPE_REJECT:
        case PDU_TYPE_ABORT:
            reply.invoke_id = reply_pdu[offset + 1];
            break;
        default:
            return false;
    }
    /* these don't have service choice included */
    if ((reply.pdu_type == PDU_TYPE_REJECT) ||
        (reply.pdu_type == PDU_TYPE_ABORT)) {
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
    if (request.npdu_data.protocol_version != reply.npdu_data.protocol_version) {
        return false;
    }
    if (request.npdu_data.priority != reply.npdu_data.priority) {
        return false;
    }
    if (!bacnet_address_same(&request.address, &reply.address)) {
        return false;
    }

    return true;
}

uint16_t MSTP_Get_Reply(
    volatile struct mstp_port_struct_t * mstp_port,
    unsigned timeout)
{       /* milliseconds to wait for a packet */
    int received_bytes = 0;
    DLMSTP_PACKET packet;
    uint16_t pdu_len = 0;       /* return value */
    uint8_t destination = 0;    /* destination address */
    bool matched = false;

    received_bytes = dlmstp_get_transmit_packet(&packet, timeout);
    if (received_bytes <= 0)
        return 0;
    /* load destination MAC address */
    if (packet.address.mac_len == 1) {
        destination = packet.address.mac[0];
    } else {
        return 0;
    }
    if ((MAX_HEADER + packet.pdu_len) > MAX_MPDU) {
        return 0;
    }
    /* is this the reply to the DER? */
    matched =
        dlmstp_compare_data_expecting_reply(&mstp_port->InputBuffer[0],
        mstp_port->DataLength, mstp_port->SourceAddress,
        &packet.pdu[0], packet.pdu_len,
        &packet.address);
    if (matched) {
#if PRINT_ENABLED
        fprintf(stderr,"MSTP: sending packet to FSM.\n");
#endif
        /* convert the PDU into the MSTP Frame */
        pdu_len = MSTP_Create_Frame(&mstp_port->OutputBuffer[0],
            mstp_port->OutputBufferSize, packet.frame_type,
            destination, mstp_port->This_Station, &packet.pdu[0],
            packet.pdu_len);
        /* not used here, but setting it anyway */
        packet.ready = false;
    } else {
        /* put it back into the queue */
        (void)mq_send(NPDU_Transmit_Queue, (char *)&packet, sizeof(packet), 1);
    }

    return pdu_len;
}

void dlmstp_set_mac_address(
    uint8_t mac_address)
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

uint8_t dlmstp_my_address(
    void)
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
void dlmstp_set_max_info_frames(
    uint8_t max_info_frames)
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

uint8_t dlmstp_max_info_frames(
    void)
{
    return MSTP_Port.Nmax_info_frames;
}

/* This parameter represents the value of the Max_Master property of the */
/* node's Device object. The value of Max_Master specifies the highest */
/* allowable address for master nodes. The value of Max_Master shall be */
/* less than or equal to 127. If Max_Master is not writable in a node, */
/* its value shall be 127. */
void dlmstp_set_max_master(
    uint8_t max_master)
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

uint8_t dlmstp_max_master(
    void)
{
    return MSTP_Port.Nmax_master;
}

void dlmstp_get_my_address(
    BACNET_ADDRESS * my_address)
{
    int i = 0;  /* counter */

    my_address->mac_len = 1;
    my_address->mac[0] = MSTP_Port.This_Station;
    my_address->net = 0;        /* local only, no routing */
    my_address->len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        my_address->adr[i] = 0;
    }

    return;
}

void dlmstp_get_broadcast_address(
    BACNET_ADDRESS * dest)
{       /* destination address */
    int i = 0;  /* counter */

    if (dest) {
        dest->mac_len = 1;
        dest->mac[0] = MSTP_BROADCAST_ADDRESS;
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0;  /* always zero when DNET is broadcast */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }

    return;
}

/* RS485 Baud Rate 9600, 19200, 38400, 57600, 115200 */
void dlmstp_set_baud_rate(
    uint32_t baud)
{
    RS485_Set_Baud_Rate(baud);
}

uint32_t dlmstp_baud_rate(
    void)
{
    return RS485_Get_Baud_Rate();
}

void dlmstp_cleanup(
    void)
{
    mq_close(NPDU_Transmit_Queue);
    mq_close(NPDU_Receive_Queue);
    RS485_Cleanup();
    /* nothing to do for static buffers */
}

bool dlmstp_init(
    char *ifname)
{
    int rc = 0;
    pthread_t hThread;
    char mqname[32];
    struct mq_attr mqattr;

    mqattr.mq_flags   = 0;
    mqattr.mq_maxmsg  = 5;
    mqattr.mq_msgsize = sizeof(struct dlmstp_packet);
    /* create a queue for the NDPU data between MS/TP threads */
    snprintf(mqname, sizeof(mqname), "/MSTP_Rx_%d", getpid());
    NPDU_Transmit_Queue = mq_open(mqname,
        O_RDWR | O_CREAT, 0600, &mqattr);
    if (NPDU_Transmit_Queue == -1) {
#if PRINT_ENABLED
        fprintf(stderr, "MS/TP: Create NPDU Transmit Queue %s: %s\n",
            mqname,
            strerror(errno));
#endif
    }
    snprintf(mqname, sizeof(mqname), "/MSTP_Tx_%d", getpid());
    NPDU_Receive_Queue = mq_open(mqname,
        O_RDWR | O_CREAT, 0600, &mqattr);
    if (NPDU_Receive_Queue == -1) {
#if PRINT_ENABLED
        fprintf(stderr, "MS/TP: Create NPDU Receive Queue %s: %s\n",
            mqname,
            strerror(errno));
#endif
    }

    /* initialize hardware */
    if (ifname) {
        RS485_Set_Interface(ifname);
#if PRINT_ENABLED
        fprintf(stderr, "MS/TP Interface: %s\n", ifname);
#endif
    }
    RS485_Initialize();
    MSTP_Port.InputBuffer = &RxBuffer[0];
    MSTP_Port.InputBufferSize = sizeof(RxBuffer);
    MSTP_Port.OutputBuffer = &TxBuffer[0];
    MSTP_Port.OutputBufferSize = sizeof(TxBuffer);
    MSTP_Port.SilenceTimer = Timer_Silence;
    MSTP_Port.SilenceTimerReset = Timer_Silence_Reset;
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
#if PRINT_ENABLED
    fprintf(stderr, "MS/TP MAC: %02X\n", MSTP_Port.This_Station);
    fprintf(stderr, "MS/TP baud: %u\n", RS485_Get_Baud_Rate());
    fprintf(stderr, "MS/TP Max_Master: %02X\n", MSTP_Port.Nmax_master);
    fprintf(stderr, "MS/TP Max_Info_Frames: %u\n", MSTP_Port.Nmax_info_frames);
#endif

    /* start our MilliSec task */
    rc = pthread_create(&hThread, NULL, dlmstp_milliseconds_task, NULL);
    rc = pthread_create(&hThread, NULL, dlmstp_fsm_receive_task, NULL);
    rc = pthread_create(&hThread, NULL, dlmstp_fsm_master_task, NULL);


    return 1;
}

#ifdef TEST_DLMSTP
#include <stdio.h>

static char *Network_Interface = NULL;

int main(
    int argc,
    char *argv[])
{
    uint16_t bytes_received = 0;
    BACNET_ADDRESS src; /* source address */
    uint8_t pdu[MAX_APDU];      /* PDU data */
#if (defined(MSTP_TEST_REQUEST) && MSTP_TEST_REQUEST)
    struct timespec timeOut, remains;

    timeOut.tv_sec = 1;
    timeOut.tv_nsec = 0;        /* 1 millisecond */
#endif
    /* argv has the "/dev/ttyS0" or some other device */
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
#if (defined(MSTP_TEST_REQUEST) && MSTP_TEST_REQUEST)
        MSTP_Create_And_Send_Frame(&MSTP_Port, FRAME_TYPE_TEST_REQUEST,
            MSTP_Port.SourceAddress, MSTP_Port.This_Station, NULL, 0);
        nanosleep(&timeOut, &remains);
#endif
        bytes_received = dlmstp_receive(&src, &pdu[0], sizeof(pdu), 10000);
        if (bytes_received) {
#if PRINT_ENABLED
            fprintf(stderr, "Received NPDU!\n");
#endif

        }
    }
    return 0;
}
#endif
