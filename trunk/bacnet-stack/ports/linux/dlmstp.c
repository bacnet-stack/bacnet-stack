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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

/* project includes */
#include "bacdef.h"
#include "mstp.h"
#include "dlmstp.h"
#include "rs485.h"
#include "npdu.h"

/* Number of MS/TP Packets Rx/Tx */
uint16_t MSTP_Packets = 0;
/* Sockets are used for NPDU queues */
static int Receive_Client_SockFD = -1;
static int Transmit_Client_SockFD = -1;
static int Receive_Server_SockFD = -1;
static int Transmit_Server_SockFD = -1;
/* local MS/TP port data - shared with RS-485 */
volatile struct mstp_port_struct_t MSTP_Port;
/* buffers needed by mstp port struct */
static uint8_t TxBuffer[MAX_MPDU];
static uint8_t RxBuffer[MAX_MPDU];

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
    /*RS485_Reinit(); */
    dlmstp_set_mac_address(DEFAULT_MAC_ADDRESS);
    dlmstp_set_max_info_frames(DEFAULT_MAX_INFO_FRAMES);
    dlmstp_set_max_master(DEFAULT_MAX_MASTER);
}

/* returns number of bytes sent on success, zero on failure */
int dlmstp_send_pdu(BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,              /* any data to be sent - may be null */
    unsigned pdu_len)
{                               /* number of bytes of data */
    DLMSTP_PACKET packet;
    int bytes_sent = 0;
    ssize_t rc = 0;

    if (pdu_len) {
        if (npdu_data->data_expecting_reply) {
            packet.frame_type =
                FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY;
        } else {
            packet.frame_type =
                FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY;
        }
        packet.pdu_len = pdu_len;
        memmove(&packet.pdu[0], &pdu[0], pdu_len);
        memmove(&packet.address, dest, sizeof(packet.address));
        rc = write(Transmit_Server_SockFD, &packet, sizeof(packet));
        if (rc > 0)
            bytes_sent = rc;
    }

    return bytes_sent;
}

/* copy the packet if one is received.
   Return the length of the packet */
uint16_t dlmstp_receive(
    BACNET_ADDRESS * src, /* source address */
    uint8_t * pdu, /* PDU data */
    uint16_t max_pdu, /* amount of space available in the PDU  */
    unsigned timeout) /* milliseconds to wait for a packet */
{
    uint16_t pdu_len = 0;
    int received_bytes = 0;
    DLMSTP_PACKET packet;
    struct timeval select_timeout;
    fd_set read_fds;
    int max = 0;

    /* Make sure the socket is open */
    if (Receive_Client_SockFD <= 0)
        return 0;

    if (timeout >= 1000) {
        select_timeout.tv_sec = timeout / 1000;
        select_timeout.tv_usec =
            1000 * (timeout - select_timeout.tv_sec * 1000);
    } else {
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 1000 * timeout;
    }
    FD_ZERO(&read_fds);
    FD_SET(Receive_Client_SockFD, &read_fds);
    max = Receive_Client_SockFD;

    if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
        received_bytes = read(
            Receive_Client_SockFD,
            &packet,
            sizeof(packet));
    } else {
        return 0;
    }

    /* See if there is a problem */
    if (received_bytes < 0) {
        /* EAGAIN Non-blocking I/O has been selected  */
        /* using O_NONBLOCK and no data */
        /* was immediately available for reading. */
        if (errno != EAGAIN) {
#if PRINT_ENABLED
            fprintf(stderr,
                "mstp: Read error in Receive_Client packet: %s\n",
                strerror(errno));
#endif
        }
        return 0;
    }

    if (received_bytes == 0)
        return 0;

    /* copy the buffer into the PDU */
    pdu_len = packet.pdu_len;
    memmove(&pdu[0], &packet.pdu[0], pdu_len);
    memmove(src, &packet.address, sizeof(packet.address));
    /* not used in this scheme */
    packet.ready = true;


    return pdu_len;
}

static void *dlmstp_fsm_receive_task(void *pArg)
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

static void *dlmstp_fsm_master_task(void *pArg)
{
    struct timespec timeOut,remains;

    timeOut.tv_sec = 0;
    timeOut.tv_nsec = 100000; /* .1 millisecond */

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
uint16_t MSTP_Put_Receive(
        volatile struct mstp_port_struct_t *mstp_port)
{
    DLMSTP_PACKET packet;
    uint16_t pdu_len = mstp_port->DataLength;

    /* bounds check - maybe this should send an abort? */
    if (pdu_len > sizeof(packet.pdu))
        pdu_len = sizeof(packet.pdu);
    if (pdu_len) {
        MSTP_Packets++;
        memmove(&packet.pdu[0],
            (void *) & mstp_port->InputBuffer[0],
            pdu_len);
        dlmstp_fill_bacnet_address(&packet.address,
            mstp_port->SourceAddress);
        packet.pdu_len = pdu_len;
        /* ready is not used in this scheme */
        packet.ready = true; 
        write(Receive_Server_SockFD, &packet, sizeof(packet));
    }

    return pdu_len;
}

/* for the MS/TP state machine to use for getting data to send */
/* Return: amount of PDU data */
uint16_t MSTP_Get_Send(
    uint8_t src, /* source MS/TP address for creating packet */
    uint8_t * pdu, /* data to send */
    uint16_t max_pdu, /* amount of space available */
    unsigned timeout) /* milliseconds to wait for a packet */
{
    uint16_t pdu_len = 0; /* return value */
    int received_bytes = 0;
    DLMSTP_PACKET packet;
    struct timeval select_timeout;
    fd_set read_fds;
    int max = 0;
    uint8_t destination = 0;    /* destination address */

    /* Make sure the socket is open */
    if (Transmit_Client_SockFD <= 0)
        return 0;

    if (timeout >= 1000) {
        select_timeout.tv_sec = timeout / 1000;
        select_timeout.tv_usec =
            1000 * (timeout - select_timeout.tv_sec * 1000);
    } else {
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 1000 * timeout;
    }
    FD_ZERO(&read_fds);
    FD_SET(Transmit_Client_SockFD, &read_fds);
    max = Transmit_Client_SockFD;

    if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
        received_bytes = read(
            Transmit_Client_SockFD,
            &packet,
            sizeof(packet));
    } else {
        return 0;
    }

    /* See if there is a problem */
    if (received_bytes < 0) {
        /* EAGAIN Non-blocking I/O has been selected  */
        /* using O_NONBLOCK and no data */
        /* was immediately available for reading. */
        if (errno != EAGAIN) {
#if PRINT_ENABLED
            fprintf(stderr,
                "mstp: Read error in Transmit_Client packet: %s\n",
                strerror(errno));
#endif
        }
        return 0;
    }

    if (received_bytes == 0)
        return 0;

    /* load destination MAC address */
    if (packet.address.mac_len == 1) {
        destination = packet.address.mac[0];
    } else {
        return 0;
    }
    if ((8 /* header len */  + packet.pdu_len) > MAX_MPDU) {
        return 0;
    }
    /* convert the PDU into the MSTP Frame */
    pdu_len = MSTP_Create_Frame(
        pdu, /* <-- loading this */
        max_pdu,
        packet.frame_type,
        destination, src,
        &packet.pdu[0],
        packet.pdu_len);

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

/* RS485 Baud Rate 9600, 19200, 38400, 57600, 115200 */
void dlmstp_set_baud_rate(uint32_t baud)
{
    RS485_Set_Baud_Rate(baud);
}

uint32_t dlmstp_baud_rate(void)
{
  return RS485_Get_Baud_Rate();
}

static int create_named_server_socket (const char *filename)
{
    struct sockaddr_un name;
    int sock;
    size_t size;

    /* Create the socket. */
    sock = socket (PF_LOCAL, SOCK_DGRAM, 0);
    if (sock < 0) {
#if PRINT_ENABLED
        perror ("socket");
#endif
        exit (EXIT_FAILURE);
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sock, sizeof(sock));
    /* Bind a name to the socket. */
    name.sun_family = AF_LOCAL;
    strncpy (name.sun_path, filename, sizeof (name.sun_path));
    /* The size of the address is
        the offset of the start of the filename,
        plus its length,
        plus one for the terminating null byte.
        Alternatively you can just do:
        size = SUN_LEN (&name);
    */
    size = (offsetof (struct sockaddr_un, sun_path)
            + strlen (name.sun_path) + 1);
    if (bind (sock, (struct sockaddr *) &name, size) < 0) {
#if PRINT_ENABLED
        perror ("bind");
#endif
        exit (EXIT_FAILURE);
    }

    return sock;
}

/* used by the client */
static int connect_named_server_socket (const char *filename)
{
    struct sockaddr_un name;
    int sock;
    size_t size;

    /* Create the socket. */
    sock = socket (PF_LOCAL, SOCK_DGRAM, 0);
    if (sock < 0) {
#if PRINT_ENABLED
        perror ("socket");
#endif
        exit (EXIT_FAILURE);
    }
    strncpy (name.sun_path, filename, sizeof (name.sun_path));
    /* The size of the address is
        the offset of the start of the filename,
        plus its length,
        plus one for the terminating null byte.
        Alternatively you can just do:
        size = SUN_LEN (&name);
    */
    size = (offsetof (struct sockaddr_un, sun_path)
            + strlen (name.sun_path) + 1);

    if (connect(sock, (struct sockaddr *) &name, size) < 0) {
#if PRINT_ENABLED
        perror ("client: can't connect to socket");
#endif
        exit (EXIT_FAILURE);
    }

    return sock;
}

static char *dlmstp_transmit_socket_name = "/tmp/BACnet_MSTP_Tx";
static char *dlmstp_receive_socket_name = "/tmp/BACnet_MSTP_Rx";

void dlmstp_cleanup(void)
{
    remove(dlmstp_transmit_socket_name);
    remove(dlmstp_receive_socket_name);
    RS485_Cleanup();
    /* nothing to do for static buffers */
}

bool dlmstp_init(char *ifname)
{
    int rc = 0;
    pthread_t hThread;

    remove(dlmstp_transmit_socket_name);
    remove(dlmstp_receive_socket_name);
    /* create a socket for queuing the NDPU data between MS/TP */
    Transmit_Server_SockFD =
        create_named_server_socket(dlmstp_transmit_socket_name);
    Transmit_Client_SockFD =
        connect_named_server_socket(dlmstp_transmit_socket_name);
    /* creates sockets for Receiving data */
    Receive_Server_SockFD =
        create_named_server_socket(dlmstp_receive_socket_name);
    Receive_Client_SockFD =
        connect_named_server_socket(dlmstp_receive_socket_name);

    /* initialize hardware */
    if (ifname) {
        RS485_Set_Interface(ifname);
#if PRINT_ENABLED
        fprintf(stderr,"MS/TP Interface: %s\n", ifname);
#endif
    }
    RS485_Initialize();
    MSTP_Port.InputBuffer = &RxBuffer[0];
    MSTP_Port.InputBufferSize = sizeof(RxBuffer);
    MSTP_Port.OutputBuffer = &TxBuffer[0];
    MSTP_Port.OutputBufferSize = sizeof(TxBuffer);
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
    fprintf(stderr,"MS/TP MAC: %02X\n",
        MSTP_Port.This_Station);
    fprintf(stderr,"MS/TP Max_Master: %02X\n",
        MSTP_Port.Nmax_master);
    fprintf(stderr,"MS/TP Max_Info_Frames: %u\n",
        MSTP_Port.Nmax_info_frames);
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

int main(int argc, char *argv[])
{
    uint16_t bytes_received = 0;
    BACNET_ADDRESS src; /* source address */
    uint8_t pdu[MAX_APDU]; /* PDU data */
#if (defined(MSTP_TEST_REQUEST) && MSTP_TEST_REQUEST)
    struct timespec timeOut, remains;

    timeOut.tv_sec = 1;
    timeOut.tv_nsec = 0; /* 1 millisecond */
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
        MSTP_Create_And_Send_Frame(
            &MSTP_Port,
            FRAME_TYPE_TEST_REQUEST,
            MSTP_Port.SourceAddress,
            MSTP_Port.This_Station,
            NULL, 0);
        nanosleep(&timeOut, &remains);
#endif
        bytes_received =
            dlmstp_receive(&src, &pdu[0], sizeof(pdu), 10000);
        if (bytes_received) {
            fprintf(stderr,"Received NPDU!\n");
        }
    }
    return 0;
}
#endif
