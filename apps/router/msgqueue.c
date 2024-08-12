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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "msgqueue.h"

pthread_mutex_t msg_lock = PTHREAD_MUTEX_INITIALIZER;

MSGBOX_ID create_msgbox()
{
    MSGBOX_ID msgboxid;

    msgboxid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msgboxid == INVALID_MSGBOX_ID) {
        return INVALID_MSGBOX_ID;
    }

    return msgboxid;
}

bool send_to_msgbox(MSGBOX_ID dest, BACMSG *msg)
{
    int err;

    err = msgsnd(dest, msg, sizeof(BACMSG) - sizeof(MSGTYPE), 0);
    if (err) {
        return false;
    }
    return true;
}

BACMSG *recv_from_msgbox(MSGBOX_ID src, BACMSG *msg, int flags)
{
    int recv_bytes;

    recv_bytes = msgrcv(src, msg, sizeof(BACMSG) - sizeof(MSGTYPE), 0, flags);
    if (recv_bytes > 0) {
        return msg;
    } else {
        return NULL;
    }
}

void del_msgbox(MSGBOX_ID msgboxid)
{
    if (msgboxid == INVALID_MSGBOX_ID) {
        return;
    } else {
        msgctl(msgboxid, IPC_RMID, NULL);
    }
}

void free_data(MSG_DATA *data)
{
    if (data->pdu) {
        free(data->pdu);
        data->pdu = NULL;
    }
    if (data) {
        free(data);
        data = NULL;
    }
}

void check_data(MSG_DATA *data)
{
    /* lock and decrement messages reference count */
    pthread_mutex_lock(&msg_lock);
    if (--data->ref_count == 0) {
        free_data(data);
    }
    pthread_mutex_unlock(&msg_lock);
}
