/**************************************************************************
 *
 * Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/

#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lwip/def.h"
#include "lwip/udp.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "lwip/inet.h"

/* members are in network byte order */
struct sockaddr_in {
    u8_t sin_len;
    u8_t sin_family;
    u16_t sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

struct sockaddr {
    u8_t sa_len;
    u8_t sa_family;
    char sa_data[14];
};

#endif
