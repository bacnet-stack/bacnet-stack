/*
Copyright (C) 2012  Andriy Sukhynyuk, Vasyl Tkhir, Andriy Ivasiv

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "msgqueue.h"

pthread_mutex_t msg_lock = PTHREAD_MUTEX_INITIALIZER;

MSGBOX_ID create_msgbox(
    )
{
    MSGBOX_ID msgboxid;

    msgboxid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msgboxid == INVALID_MSGBOX_ID) {
        return INVALID_MSGBOX_ID;
    }

    return msgboxid;
}

bool send_to_msgbox(
    MSGBOX_ID dest,
    BACMSG * msg)
{

    int err;

    err = msgsnd(dest, msg, sizeof(BACMSG), 0);
    if (err) {
        return false;
    }
    return true;
}

BACMSG *recv_from_msgbox(
    MSGBOX_ID src,
    BACMSG * msg)
{

    int recv_bytes;

    recv_bytes = msgrcv(src, msg, sizeof(BACMSG), 0, IPC_NOWAIT);
    if (recv_bytes > 0) {
        return msg;
    } else {
        return NULL;
    }
}

void del_msgbox(
    MSGBOX_ID msgboxid)
{

    if (msgboxid == INVALID_MSGBOX_ID)
        return;
    else
        msgctl(msgboxid, IPC_RMID, NULL);
}

void free_data(
    MSG_DATA * data)
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

void check_data(
    MSG_DATA * data)
{

    /* lock and decrement messages reference count */
    pthread_mutex_lock(&msg_lock);
    if (--data->ref_count == 0) {
        free_data(data);
    }
    pthread_mutex_unlock(&msg_lock);
}
