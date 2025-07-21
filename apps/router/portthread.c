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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portthread.h"

ROUTER_PORT *find_snet(MSGBOX_ID id)
{
    ROUTER_PORT *port = head;

    while (port != NULL) {
        if (port->port_id == id) {
            return port;
        }
        port = port->next;
    }

    return NULL;
}

ROUTER_PORT *find_dnet(uint16_t net, BACNET_ADDRESS *addr)
{
    ROUTER_PORT *port = head;
    DNET *dnet;

    /* for broadcast messages no search is needed */
    if (net == BACNET_BROADCAST_NETWORK) {
        return port;
    }

    while (port != NULL) {
        /* check if DNET is directly connected to the router */
        if (net == port->route_info.net) {
            return port;

            /* else search router ports DNET list */
        } else if (port->route_info.dnets) {
            dnet = port->route_info.dnets;
            while (dnet != NULL) {
                if (net == dnet->net) {
                    if (addr) {
                        memmove(&addr->len, &dnet->mac_len, 1);
                        memmove(&addr->adr[0], &dnet->mac[0], MAX_MAC_LEN);
                    }
                    return port;
                }
                dnet = dnet->next;
            }
        }
        port = port->next;
    }

    return NULL;
}

void add_dnet(RT_ENTRY *route_info, uint16_t net, BACNET_ADDRESS addr)
{
    DNET *dnet = route_info->dnets;
    DNET *tmp;

    if (dnet == NULL) {
        route_info->dnets = (DNET *)malloc(sizeof(DNET));
        memmove(&route_info->dnets->mac_len, &addr.len, 1);
        memmove(&route_info->dnets->mac[0], &addr.adr[0], MAX_MAC_LEN);
        route_info->dnets->net = net;
        route_info->dnets->state = true;
        route_info->dnets->next = NULL;
    } else {
        while (dnet != NULL) {
            if (dnet->net == net) { /* make sure NETs are not repeated */
                return;
            }
            tmp = dnet;
            dnet = dnet->next;
        }

        dnet = (DNET *)malloc(sizeof(DNET));
        memmove(&dnet->mac_len, &addr.len, 1);
        memmove(&dnet->mac[0], &addr.adr[0], MAX_MAC_LEN);
        dnet->net = net;
        dnet->state = true;
        dnet->next = NULL;
        tmp->next = dnet;
    }
}

void cleanup_dnets(DNET *dnets)
{
    DNET *dnet = dnets;
    while (dnet != NULL) {
        dnet = dnet->next;
        free(dnets);
        dnets = dnet;
    }
}
