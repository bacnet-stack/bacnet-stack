/**************************************************************************
 *
 * Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#ifndef DLMSTP_LINUX_H
#define DLMSTP_LINUX_H

#include "bacnet/datalink/mstp.h"
/*#include "bacnet/datalink/dlmstp.h" */
#include <sys/types.h>
#include <semaphore.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define termios asmtermios
#include <asm/termbits.h>
#undef termios
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"
#include "bacnet/basic/sys/fifo.h"
#include "bacnet/basic/sys/ringbuf.h"
/* defines specific to MS/TP */
/* preamble+type+dest+src+len+crc8+crc16 */
#define DLMSTP_HEADER_MAX (2 + 1 + 1 + 1 + 2 + 1 + 2)
#define DLMSTP_MPDU_MAX (DLMSTP_HEADER_MAX + MAX_PDU)

/* count must be a power of 2 for ringbuf library */
#ifndef MSTP_PDU_PACKET_COUNT
#define MSTP_PDU_PACKET_COUNT 8
#endif

typedef struct dlmstp_packet {
    bool ready; /* true if ready to be sent or received */
    BACNET_ADDRESS address; /* source address */
    uint8_t frame_type; /* type of message */
    uint16_t pdu_len; /* packet length */
    uint8_t pdu[DLMSTP_MPDU_MAX]; /* packet */
} DLMSTP_PACKET;

/* data structure for MS/TP PDU Queue */
struct mstp_pdu_packet {
    bool data_expecting_reply;
    uint8_t destination_mac;
    uint16_t length;
    uint8_t buffer[DLMSTP_MPDU_MAX];
};

typedef struct shared_mstp_data {
    /* Number of MS/TP Packets Rx/Tx */
    uint16_t MSTP_Packets;

    /* packet queues */
    DLMSTP_PACKET Receive_Packet;
    DLMSTP_PACKET Transmit_Packet;
    /*
       RT_SEM Receive_Packet_Flag;
     */
    sem_t Receive_Packet_Flag;
    /* mechanism to wait for a frame in state machine */
    /*
       RT_COND Received_Frame_Flag;
       RT_MUTEX Received_Frame_Mutex;
     */
    pthread_cond_t Received_Frame_Flag;
    pthread_mutex_t Received_Frame_Mutex;
    pthread_cond_t Master_Done_Flag;
    pthread_mutex_t Master_Done_Mutex;
    /* buffers needed by mstp port struct */
    uint8_t TxBuffer[DLMSTP_MPDU_MAX];
    uint8_t RxBuffer[DLMSTP_MPDU_MAX];
    /* Timer that indicates line silence - and functions */
    uint16_t SilenceTime;

    /* handle returned from open() */
    int RS485_Handle;
    /* baudrate */
    unsigned int RS485_Baud;
    /* serial port name, /dev/ttyS0,
       /dev/ttyUSB0 for USB->RS485 from B&B Electronics USOPTL4 */
    char *RS485_Port_Name;
    /* serial I/O settings */
    struct termios2 RS485_oldtio2;
    /* some terminal I/O have RS-485 specific functionality */
    tcflag_t RS485MOD;
    /* Ring buffer for incoming bytes, in order to speed up the receiving. */
    FIFO_BUFFER Rx_FIFO;
    /* buffer size needs to be a power of 2 */
    uint8_t Rx_Buffer[4096];
    struct timeval start;

    RING_BUFFER PDU_Queue;

    struct mstp_pdu_packet PDU_Buffer[MSTP_PDU_PACKET_COUNT];

} SHARED_MSTP_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
bool dlmstp_init(void *poShared, char *ifname);
BACNET_STACK_EXPORT
void dlmstp_reset(void *poShared);
BACNET_STACK_EXPORT
void dlmstp_cleanup(void *poShared);

/* returns number of bytes sent on success, negative on failure */
BACNET_STACK_EXPORT
int dlmstp_send_pdu(
    void *poShared,
    BACNET_ADDRESS *dest, /* destination address */
    uint8_t *pdu, /* any data to be sent - may be null */
    unsigned pdu_len); /* number of bytes of data */

/* returns the number of octets in the PDU, or zero on failure */
BACNET_STACK_EXPORT
uint16_t dlmstp_receive(
    void *poShared,
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* PDU data */
    uint16_t max_pdu, /* amount of space available in the PDU  */
    unsigned timeout); /* milliseconds to wait for a packet */

/* This parameter represents the value of the Max_Info_Frames property of */
/* the node's Device object. The value of Max_Info_Frames specifies the */
/* maximum number of information frames the node may send before it must */
/* pass the token. Max_Info_Frames may have different values on different */
/* nodes. This may be used to allocate more or less of the available link */
/* bandwidth to particular nodes. If Max_Info_Frames is not writable in a */
/* node, its value shall be 1. */
BACNET_STACK_EXPORT
void dlmstp_set_max_info_frames(void *poShared, uint8_t max_info_frames);
BACNET_STACK_EXPORT
uint8_t dlmstp_max_info_frames(void *poShared);

/* This parameter represents the value of the Max_Master property of the */
/* node's Device object. The value of Max_Master specifies the highest */
/* allowable address for master nodes. The value of Max_Master shall be */
/* less than or equal to 127. If Max_Master is not writable in a node, */
/* its value shall be 127. */
BACNET_STACK_EXPORT
void dlmstp_set_max_master(void *poShared, uint8_t max_master);
BACNET_STACK_EXPORT
uint8_t dlmstp_max_master(void *poShared);

/* MAC address 0-127 */
BACNET_STACK_EXPORT
void dlmstp_set_mac_address(void *poShared, uint8_t my_address);
BACNET_STACK_EXPORT
uint8_t dlmstp_mac_address(void *poShared);

BACNET_STACK_EXPORT
void dlmstp_get_my_address(void *poShared, BACNET_ADDRESS *my_address);
BACNET_STACK_EXPORT
void dlmstp_get_broadcast_address(
    BACNET_ADDRESS *dest); /* destination address */

/* RS485 Baud Rate 9600, 19200, 38400, 57600, 115200 */
BACNET_STACK_EXPORT
void dlmstp_set_baud_rate(void *poShared, uint32_t baud);
BACNET_STACK_EXPORT
uint32_t dlmstp_baud_rate(void *poShared);

BACNET_STACK_EXPORT
void dlmstp_fill_bacnet_address(BACNET_ADDRESS *src, uint8_t mstp_address);

BACNET_STACK_EXPORT
bool dlmstp_sole_master(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /*DLMSTP_LINUX_H */
