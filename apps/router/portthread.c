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

#if ROUTER_DNET_MAX_AGE_MS > 0
static bool dnet_expired(const DNET *dnet, unsigned long now)
{
    return (unsigned long)(now - dnet->last_seen_ms) >=
        (unsigned long)ROUTER_DNET_MAX_AGE_MS;
}
#endif

static void dnet_mark_seen(DNET *dnet)
{
    if (dnet) {
        dnet->last_seen_ms = mstimer_now();
    }
}

static bool net_directly_connected(uint16_t net)
{
    ROUTER_PORT *port;

    port = head;
    while (port != NULL) {
        if (port->route_info.net == net) {
            return true;
        }
        port = port->next;
    }

    return false;
}

static void unlink_dnet(RT_ENTRY *route_info, uint16_t net)
{
    DNET **link;
    DNET *dnet;

    if (!route_info) {
        return;
    }
    link = &route_info->dnets;
    while (*link != NULL) {
        dnet = *link;
        if (dnet->net == net) {
            *link = dnet->next;
            free(dnet);
            if (route_info->dnet_count > 0) {
                route_info->dnet_count--;
            }
            return;
        }
        link = &dnet->next;
    }
}

/* A newer advertisement wins: drop this network from every other port. */
static void forget_dnet_on_other_ports(RT_ENTRY *keep, uint16_t net)
{
    ROUTER_PORT *port;

    port = head;
    while (port != NULL) {
        if (&port->route_info != keep) {
            unlink_dnet(&port->route_info, net);
        }
        port = port->next;
    }
}

static void evict_least_recent(RT_ENTRY *route_info, unsigned long now)
{
    DNET **link;
    DNET **oldest;
    DNET *dnet;
    unsigned long oldest_age;
    unsigned long age;

    oldest = NULL;
    oldest_age = 0;
    link = &route_info->dnets;
    while (*link != NULL) {
        dnet = *link;
        age = now - dnet->last_seen_ms;
        if ((oldest == NULL) || (age > oldest_age)) {
            oldest = link;
            oldest_age = age;
        }
        link = &dnet->next;
    }
    if (oldest != NULL) {
        dnet = *oldest;
        *oldest = dnet->next;
        free(dnet);
        if (route_info->dnet_count > 0) {
            route_info->dnet_count--;
        }
    }
}

void refresh_dnet(RT_ENTRY *route_info, uint16_t net)
{
    DNET *dnet;

    if (!route_info) {
        return;
    }
    dnet = route_info->dnets;
    while (dnet != NULL) {
        if (dnet->net == net) {
            dnet_mark_seen(dnet);
            return;
        }
        dnet = dnet->next;
    }
}

void expire_dnets(RT_ENTRY *route_info)
{
#if ROUTER_DNET_MAX_AGE_MS > 0
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
#else
    /* A zero age keeps learned routes. BACnet routers do not re-announce. */
    (void)route_info;
#endif
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
    ROUTER_PORT *port;
    DNET *dnet;

    port = head;
    /* for broadcast messages no search is needed */
    if (net == BACNET_BROADCAST_NETWORK) {
        return port;
    }

    /* A port's own network wins over any learned advertisement. */
    while (port != NULL) {
        if (port->route_info.net == net) {
            return port;
        }
        port = port->next;
    }

    port = head;
    while (port != NULL) {
        expire_dnets(&port->route_info);
        dnet = port->route_info.dnets;
        while (dnet != NULL) {
            if (dnet->net == net) {
                /* traffic forwarded via this learned network */
                dnet_mark_seen(dnet);
                if (addr) {
                    memmove(&addr->len, &dnet->mac_len, 1);
                    memmove(&addr->adr[0], &dnet->mac[0], MAX_MAC_LEN);
                }
                return port;
            }
            dnet = dnet->next;
        }
        port = port->next;
    }

    return NULL;
}

bool add_dnet(RT_ENTRY *route_info, uint16_t net, BACNET_ADDRESS addr)
{
    DNET *dnet;
    DNET *last;
    unsigned long now;

    last = NULL;
    if (!route_info) {
        return false;
    }
    /* Local and global-broadcast network numbers are not routes. */
    if ((net == 0) || (net == BACNET_BROADCAST_NETWORK)) {
        return false;
    }
    /* Do not let an advertisement hide a network plugged into a port. */
    if (net_directly_connected(net)) {
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
            forget_dnet_on_other_ports(route_info, net);
            return true;
        }
        last = dnet;
        dnet = dnet->next;
    }

    if (route_info->dnet_count >= ROUTER_DNET_MAX_COUNT) {
        evict_least_recent(route_info, now);
        /* The evicted node may have been the tail. Find the new tail. */
        last = NULL;
        dnet = route_info->dnets;
        while (dnet != NULL) {
            last = dnet;
            dnet = dnet->next;
        }
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
    forget_dnet_on_other_ports(route_info, net);

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
