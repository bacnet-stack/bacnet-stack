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
#ifndef PORTTHREAD_H
#define PORTTHREAD_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "msgqueue.h"
#include "bacdef.h"
#include "npdu.h"

#define ERROR 1
#define INFO 2
#define DEBUG 3

#define DEBUG_LEVEL 3
#ifdef DEBUG_LEVEL
#define PRINT(debug_level, ...) if(debug_level <= DEBUG_LEVEL) fprintf(stderr, __VA_ARGS__)
#else
#define PRINT(...)
#endif

typedef enum {
	BIP = 1,
	MSTP = 2
} DL_TYPE;

typedef enum {
	INIT,
	INIT_FAILED,
	RUNNING,
	FINISHED
} PORT_STATE;

// router port thread function
typedef void* (*PORT_FUNC)(
		void*);

typedef enum {
	PARITY_NONE,
	PARITY_EVEN,
	PARITY_ODD
}PARITY;

// port specific parameters
typedef union _port_params {
	struct {
		uint16_t port;
	} bip_params;
	struct {
		uint32_t baudrate;
		PARITY parity;
		uint8_t databits;
		uint8_t stopbits;
		uint8_t max_master;
		uint8_t max_frames;
	} mstp_params;
} PORT_PARAMS;

// list node for reacheble networks
typedef struct _dnet {
	uint8_t mac[MAX_MAC_LEN];
	uint8_t mac_len;
	uint16_t net;
	bool state; // enabled or disabled
	struct _dnet *next;
} DNET;

// information for routing table
typedef struct _routing_table_entry {
	uint8_t mac[MAX_MAC_LEN];
	uint8_t mac_len;
	uint16_t net;
	DNET *dnets;
} RT_ENTRY;

typedef struct _port {
	DL_TYPE type;
	PORT_STATE state;
	MSGBOX_ID main_id;	// same for every router port
	MSGBOX_ID port_id;	// different for every router port
	char *iface;
	PORT_FUNC func;
	RT_ENTRY route_info;
	PORT_PARAMS params;
	struct _port *next;	// pointer to next list node
} ROUTER_PORT;

extern ROUTER_PORT *head;
extern int port_count;

// get recieving router port
ROUTER_PORT* find_snet(
		MSGBOX_ID id);

// get sending router port
ROUTER_PORT* find_dnet(
		uint16_t net,
		BACNET_ADDRESS *addr);

// add reacheble network for specified router port
void add_dnet(
		RT_ENTRY *route_info,
		uint16_t net,
		BACNET_ADDRESS addr);

void cleanup_dnets(
		DNET *dnets);

#endif /* end of PORTTHREAD_H */
