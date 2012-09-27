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
#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacenum.h"
#include "bacdef.h"
#include "npdu.h"
#include "net.h"
#include "portthread.h"

uint16_t process_network_message(
		BACMSG *msg,
		MSG_DATA *data,
		uint8_t **buff);

uint16_t create_network_message(
		BACNET_NETWORK_MESSAGE_TYPE network_message_type,
		MSG_DATA *data,
		uint8_t **buff,
		void *val);

void send_network_message(
		BACNET_NETWORK_MESSAGE_TYPE network_message_type,
		MSG_DATA *data,
		uint8_t **buff,
		void *val);

void init_npdu(
		BACNET_NPDU_DATA *npdu_data,
		BACNET_NETWORK_MESSAGE_TYPE network_message_type,
		bool data_expecting_reply);

#endif /* end of NETWORK_LAYER_H */
