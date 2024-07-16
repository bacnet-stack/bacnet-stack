/**
 * @file
 * @brief API for the Network Layer using BACnet MS/TP as the transport 
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink Network Layer
 * @ingroup DataLink
 */
#ifndef BACNET_DLMSTP_H
#define BACNET_DLMSTP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/datalink/mstpdef.h"
#include "bacnet/npdu.h"

/* defines specific to MS/TP */
/* preamble+type+dest+src+len+crc8+crc16 */
#define DLMSTP_HEADER_MAX (2+1+1+1+2+1+2)
#define DLMSTP_MPDU_MAX (DLMSTP_HEADER_MAX+MAX_PDU)

typedef struct dlmstp_packet {
    bool ready; /* true if ready to be sent or received */
    BACNET_ADDRESS address;     /* source address */
    uint8_t frame_type; /* type of message */
    uint16_t pdu_len;   /* packet length */
    uint8_t pdu[DLMSTP_MPDU_MAX];      /* packet */
} DLMSTP_PACKET;

/* container for packet and token statistics */
typedef struct dlmstp_statistics {
    uint32_t transmit_frame_counter;
    uint32_t receive_valid_frame_counter;
    uint32_t receive_invalid_frame_counter;
    uint32_t transmit_pdu_counter;
    uint32_t receive_pdu_counter;
    uint32_t lost_token_counter;
} DLMSTP_STATISTICS;

#ifndef DLMSTP_MAX_INFO_FRAMES
#define DLMSTP_MAX_INFO_FRAMES DEFAULT_MAX_INFO_FRAMES
#endif
#ifndef DLMSTP_MAX_MASTER
#define DLMSTP_MAX_MASTER DEFAULT_MAX_MASTER
#endif
#ifndef DLMSTP_BAUD_RATE_DEFAULT
#define DLMSTP_BAUD_RATE_DEFAULT 38400UL
#endif

/**
 * The structure of RS485 driver for BACnet MS/TP
 */
struct dlmstp_rs485_driver {
    /** Initialize the driver hardware */
    void (*init)(void);

    /** Prepare & transmit a packet. */
    void (*send)(uint8_t *payload, uint16_t payload_len);

    /** Check if one received byte is available */
    bool (*read)(uint8_t *buf);

    /** true if the driver is transmitting */
    bool (*transmitting)(void);

    /** Get the current baud rate */
    uint32_t (*baud_rate)(void);

    /** Set the current baud rate */
    bool (*baud_rate_set)(uint32_t baud);

    /** Get the current silence time */
    uint32_t (*silence_milliseconds)(void);

    /** Reset the silence time */
    void (*silence_reset)(void);
};

/**
 * An example structure of user data for BACnet MS/TP
 */
struct dlmstp_user_data_t {
    struct dlmstp_statistics Statistics;
    struct dlmstp_rs485_driver *RS485_Driver;
    /* the PDU Queue is made of Nmax_info_frames x dlmstp_packet's */
    RING_BUFFER PDU_Queue;
    struct dlmstp_packet PDU_Buffer[DLMSTP_MAX_INFO_FRAMES];
    bool Initialized;
    bool ReceivePacketPending;
    void *Context;
};

/* callback to signify the receipt of a preamble */
typedef void (*dlmstp_hook_frame_rx_start_cb)(void);

/* callback on for receiving every valid frame */
typedef void (*dlmstp_hook_frame_rx_complete_cb)(
    uint8_t src,
    uint8_t dest,
    uint8_t mstp_msg_type,
    uint8_t * pdu,
    uint16_t pdu_len);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    bool dlmstp_init(
        char *ifname);
    BACNET_STACK_EXPORT
    void dlmstp_reset(
        void);
    BACNET_STACK_EXPORT
    void dlmstp_cleanup(
        void);

    /* returns number of bytes sent on success, negative on failure */
    BACNET_STACK_EXPORT
    int dlmstp_send_pdu(
        BACNET_ADDRESS * dest,  /* destination address */
        BACNET_NPDU_DATA * npdu_data,   /* network information */
        uint8_t * pdu,  /* any data to be sent - may be null */
        unsigned pdu_len);      /* number of bytes of data */

    /* returns the number of octets in the PDU, or zero on failure */
    BACNET_STACK_EXPORT
    uint16_t dlmstp_receive(
        BACNET_ADDRESS * src,   /* source address */
        uint8_t * pdu,  /* PDU data */
        uint16_t max_pdu,       /* amount of space available in the PDU  */
        unsigned timeout);      /* milliseconds to wait for a packet */

    /* This parameter represents the value of the Max_Info_Frames property of */
    /* the node's Device object. The value of Max_Info_Frames specifies the */
    /* maximum number of information frames the node may send before it must */
    /* pass the token. Max_Info_Frames may have different values on different */
    /* nodes. This may be used to allocate more or less of the available link */
    /* bandwidth to particular nodes. If Max_Info_Frames is not writable in a */
    /* node, its value shall be 1. */
    BACNET_STACK_EXPORT
    void dlmstp_set_max_info_frames(
        uint8_t max_info_frames);
    BACNET_STACK_EXPORT
    uint8_t dlmstp_max_info_frames(
        void);

    /* This parameter represents the value of the Max_Master property of the */
    /* node's Device object. The value of Max_Master specifies the highest */
    /* allowable address for master nodes. The value of Max_Master shall be */
    /* less than or equal to 127. If Max_Master is not writable in a node, */
    /* its value shall be 127. */
    BACNET_STACK_EXPORT
    void dlmstp_set_max_master(
        uint8_t max_master);
    BACNET_STACK_EXPORT
    uint8_t dlmstp_max_master(
        void);

    /* MAC address 0-127 */
    BACNET_STACK_EXPORT
    void dlmstp_set_mac_address(
        uint8_t my_address);
    BACNET_STACK_EXPORT
    uint8_t dlmstp_mac_address(
        void);

    BACNET_STACK_EXPORT
    void dlmstp_get_my_address(
        BACNET_ADDRESS * my_address);
    BACNET_STACK_EXPORT
    void dlmstp_get_broadcast_address(
        BACNET_ADDRESS * dest); /* destination address */

    /* RS485 Baud Rate 9600, 19200, 38400, 57600, 115200 */
    BACNET_STACK_EXPORT
    void dlmstp_set_baud_rate(
        uint32_t baud);
    BACNET_STACK_EXPORT
    uint32_t dlmstp_baud_rate(
        void);

    BACNET_STACK_EXPORT
    void dlmstp_fill_bacnet_address(
        BACNET_ADDRESS * src,
        uint8_t mstp_address);

    BACNET_STACK_EXPORT
    bool dlmstp_sole_master(
        void);
    BACNET_STACK_EXPORT
    bool dlmstp_send_pdu_queue_empty(void);
    BACNET_STACK_EXPORT
    bool dlmstp_send_pdu_queue_full(void);

    BACNET_STACK_EXPORT
    uint8_t dlmstp_max_info_frames_limit(void);
    BACNET_STACK_EXPORT
    uint8_t dlmstp_max_master_limit(void);

    BACNET_STACK_EXPORT
    uint32_t dlmstp_silence_milliseconds(
        void *arg);
    BACNET_STACK_EXPORT
    void dlmstp_silence_reset(
        void *arg);

    /* Set the callback function to be called on every valid received frame */
    /* This is not necessary for normal usage, but is helpful if the caller */
    /* needs to monitor traffic on the MS/TP bus */
    /* The specified callback function should execute quickly so as to avoid */
    /* interfering with bus timing */
    BACNET_STACK_EXPORT
    void dlmstp_set_frame_rx_complete_callback(
        dlmstp_hook_frame_rx_complete_cb cb_func);

    /* Set the callback function to be called every time the start of a */
    /* frame is detected.  This is not necessary for normal usage, but is */
    /* helpful if the caller needs to know when a frame begins for timing */
    /* (timing is heavily dependent upon baud rate and the period with */
    /* which dlmstp_receive is called) */
    /* The specified callback function should execute quickly so as to avoid */
    /* interfering with bus timing */
    BACNET_STACK_EXPORT
    void dlmstp_set_frame_rx_start_callback(
        dlmstp_hook_frame_rx_start_cb cb_func);

    /* Reset the statistics counters on the MS/TP datalink */
    BACNET_STACK_EXPORT
    void dlmstp_reset_statistics(void);

    /* Retrieve statistics counters from the MS/TP datalink */
    /* Values for the current counters at the time this function is called */
    /* will be copied into *statistics */
    BACNET_STACK_EXPORT
    void dlmstp_fill_statistics(struct dlmstp_statistics * statistics);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
