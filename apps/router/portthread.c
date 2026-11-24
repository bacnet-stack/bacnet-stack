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
#include "bacnet/basic/sys/mstimer.h"

static bool dnet_expired(const DNET *dnet, unsigned long now)
{
    if (ROUTER_DNET_MAX_AGE_MS == 0) {
        return false;
    }
    return (unsigned long)(now - dnet->last_seen_ms) >= ROUTER_DNET_MAX_AGE_MS;
}

void expire_dnets(RT_ENTRY *route_info)
{
    DNET **link;
    DNET *dnet;
    unsigned long now;

    if (!route_info) {
        return;
    }
    now = mstimer_now();
    link = &route_info->dnets;
    while (*link != NULL) {
        dnet = *link;
        if (dnet_expired(dnet, now)) {
            *link = dnet->next;
            free(dnet);
            if (route_info->dnet_count > 0) {
                route_info->dnet_count--;
            }
        } else {
            link = &dnet->next;
        }
    }
}

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
        } else {
            expire_dnets(&port->route_info);
        }
        if (port->route_info.dnets) {
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

bool add_dnet(RT_ENTRY *route_info, uint16_t net, BACNET_ADDRESS addr)
{
    DNET *dnet;
    DNET *last = NULL;
    unsigned long now;

    if (!route_info) {
        return false;
    }
    expire_dnets(route_info);
    now = mstimer_now();
    dnet = route_info->dnets;
    while (dnet != NULL) {
        if (dnet->net == net) {
            dnet->mac_len = addr.len;
            memmove(&dnet->mac[0], &addr.adr[0], MAX_MAC_LEN);
            dnet->state = true;
            dnet->last_seen_ms = now;
            return true;
        }
        last = dnet;
        dnet = dnet->next;
    }

    if (route_info->dnet_count >= ROUTER_DNET_MAX_COUNT) {
        PRINT(
            ERROR, "Error: DNET limit of %u reached\n",
            (unsigned)ROUTER_DNET_MAX_COUNT);
        return false;
    }
    dnet = (DNET *)malloc(sizeof(DNET));
    if (!dnet) {
        PRINT(ERROR, "Error: Could not allocate DNET entry\n");
        return false;
    }
    dnet->mac_len = addr.len;
    memmove(&dnet->mac[0], &addr.adr[0], MAX_MAC_LEN);
    dnet->net = net;
    dnet->state = true;
    dnet->last_seen_ms = now;
    dnet->next = NULL;
    if (last) {
        last->next = dnet;
    } else {
        route_info->dnets = dnet;
    }
    route_info->dnet_count++;

    return true;
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
