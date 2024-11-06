/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#ifndef DLMSTP_H
#define DLMSTP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"

/* defines specific to MS/TP */
#define DLMSTP_HEADER_MAX (2+1+1+1+2+1)
#define DLMSTP_MPDU_MAX (DLMSTP_HEADER_MAX+MAX_PDU)

typedef struct dlmstp_packet {
    bool ready; /* true if ready to be sent or received */
    BACNET_ADDRESS address;     /* source address */
    uint8_t frame_type; /* type of message */
    unsigned pdu_len;   /* packet length */
    uint8_t pdu[DLMSTP_MPDU_MAX];      /* packet */
} DLMSTP_PACKET;

/* number of MS/TP tx/rx packets */
extern uint16_t MSTP_Packets;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void dlmstp_reinit(
        void);
    void dlmstp_init(
        void);
    void dlmstp_cleanup(
        void);
    void dlmstp_millisecond_timer(
        void);
    void dlmstp_task(
        void);

    /* returns number of bytes sent on success, negative on failure */
    int dlmstp_send_pdu(
        BACNET_ADDRESS * dest,  /* destination address */
        BACNET_NPDU_DATA * npdu_data,   /* network information */
        uint8_t * pdu,  /* any data to be sent - may be null */
        unsigned pdu_len);      /* number of bytes of data */

    /* This parameter represents the value of the Max_Info_Frames property of */
    /* the node's Device object. The value of Max_Info_Frames specifies the */
    /* maximum number of information frames the node may send before it must */
    /* pass the token. Max_Info_Frames may have different values on different */
    /* nodes. This may be used to allocate more or less of the available link */
    /* bandwidth to particular nodes. If Max_Info_Frames is not writable in a */
    /* node, its value shall be 1. */
    void dlmstp_set_max_info_frames(
        uint8_t max_info_frames);
    unsigned dlmstp_max_info_frames(
        void);

    /* This parameter represents the value of the Max_Master property of the */
    /* node's Device object. The value of Max_Master specifies the highest */
    /* allowable address for master nodes. The value of Max_Master shall be */
    /* less than or equal to 127. If Max_Master is not writable in a node, */
    /* its value shall be 127. */
    void dlmstp_set_max_master(
        uint8_t max_master);
    uint8_t dlmstp_max_master(
        void);

    /* MAC address for MS/TP */
    void dlmstp_set_my_address(
        uint8_t my_address);
    uint8_t dlmstp_my_address(
        void);

    /* BACnet address used in datalink */
    void dlmstp_get_my_address(
        BACNET_ADDRESS * my_address);
    void dlmstp_get_broadcast_address(
        BACNET_ADDRESS * dest); /* destination address */

    /* MS/TP state machine functions */
    uint16_t dlmstp_put_receive(
        uint8_t src,    /* source MS/TP address */
        uint8_t * pdu,  /* PDU data */
        uint16_t pdu_len);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
