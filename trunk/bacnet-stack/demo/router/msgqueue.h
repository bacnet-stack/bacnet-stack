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
#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "bacdef.h"
#include "npdu.h"

extern pthread_mutex_t msg_lock;

#define INVALID_MSGBOX_ID -1

typedef int MSGBOX_ID;

typedef enum {
	DATA,
	SERVICE
} MSGTYPE;

typedef enum {
	SHUTDOWN,
	CHG_IP,
	CHG_MAC
} MSGSUBTYPE;

typedef struct _message {
	MSGBOX_ID origin;
	MSGTYPE type;
	MSGSUBTYPE subtype;
	void *data;
	// add timestamp
} BACMSG;

// specific message type data structures
typedef struct _msg_data {
	BACNET_ADDRESS dest;
	BACNET_ADDRESS src;
	uint8_t *pdu;
	uint16_t pdu_len;
	uint8_t ref_count;
} MSG_DATA;

MSGBOX_ID create_msgbox();

// returns sent byte count
bool send_to_msgbox(
		MSGBOX_ID dest,
		BACMSG *msg);

// returns received message
BACMSG* recv_from_msgbox(
		MSGBOX_ID src,
		BACMSG *msg);

void del_msgbox(
		MSGBOX_ID msgboxid);

// free message data structure
void free_data(
		MSG_DATA *data);

// check message reference counter and delete data if needed
void check_data(
		MSG_DATA *data);

#endif /* end of MSGQUEUE_H */
