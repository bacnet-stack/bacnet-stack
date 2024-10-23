/**
 * @file
 * @author Andriy Sukhynyuk, Vasyl Tkhir, Andriy Ivasiv
 * @date 2012
 * @brief Message queue module
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"

extern pthread_mutex_t msg_lock;

#define INVALID_MSGBOX_ID -1

typedef int MSGBOX_ID;

typedef enum { DATA = 1, SERVICE } MSGTYPE;

typedef enum { SHUTDOWN, CHG_IP, CHG_MAC } MSGSUBTYPE;

typedef struct _message {
    MSGTYPE type;
    MSGBOX_ID origin;
    MSGSUBTYPE subtype;
    void *data;
    /* add timestamp */
} BACMSG;

/* specific message type data structures */
typedef struct _msg_data {
    BACNET_ADDRESS dest;
    BACNET_ADDRESS src;
    uint8_t *pdu;
    uint16_t pdu_len;
    uint8_t ref_count;
} MSG_DATA;

MSGBOX_ID create_msgbox(void);

/* returns sent byte count */
bool send_to_msgbox(MSGBOX_ID dest, BACMSG *msg);

/* returns received message */
BACMSG *recv_from_msgbox(MSGBOX_ID src, BACMSG *msg, int flags);

void del_msgbox(MSGBOX_ID msgboxid);

/* free message data structure */
void free_data(MSG_DATA *data);

/* check message reference counter and delete data if needed */
void check_data(MSG_DATA *data);

#endif /* end of MSGQUEUE_H */
