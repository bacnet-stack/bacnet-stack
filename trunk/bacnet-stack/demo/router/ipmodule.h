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
#ifndef UDPMODULE_H
#define UDPMODULE_H

#include <stdint.h>
#include <stdbool.h>
#include "portthread.h"
#include "bip.h"

#define MAX_BIP_APDU	1476
#define MAX_BIP_PDU		(MAX_NPDU + MAX_BIP_APDU)
#define MAX_BIP_MPDU	(MAX_HEADER + MAX_BIP_PDU)

 typedef struct ip_data {
	int socket;
	uint16_t port;
	struct in_addr local_addr;
	struct in_addr broadcast_addr;
	uint8_t *buff;
	uint16_t max_buff;
 } IP_DATA;


void* dl_ip_thread(
		void *pArgs);

bool dl_ip_init(
		ROUTER_PORT *port,
		IP_DATA *data);

int dl_ip_send(
		IP_DATA *data,
		BACNET_ADDRESS *dest,
		uint8_t *pdu,
		unsigned pdu_len);

int dl_ip_recv(
		IP_DATA *data,
		MSG_DATA **msg,	// on recieve fill up message
		BACNET_ADDRESS *src,
		unsigned timeout);

void dl_ip_cleanup(
		IP_DATA *data);

#endif /* end of UDPMODULE_H */
