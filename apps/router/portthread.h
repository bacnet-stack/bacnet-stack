/**
 * @file
 * @author Andriy Sukhynyuk, Vasyl Tkhir, Andriy Ivasiv
 * @date 2012
 * @brief Network port storage and handling
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef PORTTHREAD_H
#define PORTTHREAD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"
/* router utils */
#include "msgqueue.h"

#define ERROR 1
#define INFO 2
#define DEBUG 3

#ifndef ROUTER_DNET_MAX_COUNT
#define ROUTER_DNET_MAX_COUNT 256U
#endif

/* 0 keeps learned routes. BACnet routers do not re-announce them.
 * A build may set ROUTER_DNET_MAX_AGE_MS to expire unused entries.
 */
#ifndef ROUTER_DNET_MAX_AGE_MS
#define ROUTER_DNET_MAX_AGE_MS 0
#endif

/* #define DEBUG_LEVEL 3 */
#ifdef DEBUG_LEVEL
#define PRINT(debug_level, ...)     \
    if (debug_level <= DEBUG_LEVEL) \
    fprintf(stderr, __VA_ARGS__)
#else
#define PRINT(...)
#endif

typedef enum { BIP = 1, MSTP = 2 } DL_TYPE;

typedef enum { INIT, INIT_FAILED, RUNNING, FINISHED } PORT_STATE;

/* router port thread function */
typedef void *(*PORT_FUNC)(void *);

typedef enum { PARITY_NONE, PARITY_EVEN, PARITY_ODD } PARITY;

/* port specific parameters */
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

/* list node for reacheble networks */
typedef struct _dnet {
    uint8_t mac[MAX_MAC_LEN];
    uint8_t mac_len;
    uint16_t net;
    bool state; /* enabled or disabled */
    unsigned long last_seen_ms;
    struct _dnet *next;
} DNET;

/* information for routing table */
typedef struct _routing_table_entry {
    uint8_t mac[MAX_MAC_LEN];
    uint8_t mac_len;
    uint16_t net;
    DNET *dnets;
    size_t dnet_count;
} RT_ENTRY;

typedef struct _port {
    DL_TYPE type;
    PORT_STATE state;
    MSGBOX_ID main_id; /* same for every router port */
    MSGBOX_ID port_id; /* different for every router port */
    const char *iface;
    PORT_FUNC func;
    RT_ENTRY route_info;
    PORT_PARAMS params;
    struct _port *next; /* pointer to next list node */
} ROUTER_PORT;

extern ROUTER_PORT *head;
extern int port_count;

/* get recieving router port */
ROUTER_PORT *find_snet(MSGBOX_ID id);

/* get sending router port */
ROUTER_PORT *find_dnet(uint16_t net, BACNET_ADDRESS *addr);

/* add reacheble network for specified router port */
bool add_dnet(RT_ENTRY *route_info, uint16_t net, BACNET_ADDRESS addr);

/* remove route entries that have not been updated recently */
void expire_dnets(RT_ENTRY *route_info);

/* mark a learned network as recently used, if the entry exists */
void refresh_dnet(RT_ENTRY *route_info, uint16_t net);

void cleanup_dnets(DNET *dnets);

#endif /* end of PORTTHREAD_H */
