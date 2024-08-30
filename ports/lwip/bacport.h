/**************************************************************************
 *
 * Copyright (C) 2020 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/

#ifndef BACPORT_H
#define BACPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lwip/def.h"
#include "lwip/udp.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/inet.h"

#define BACNET_OBJECT_TABLE(                                               \
    table_name, _type, _init, _count, _index_to_instance, _valid_instance, \
    _object_name, _read_property, _write_property, _RPM_list, _RR_info,    \
    _iterator, _value_list, _COV, _COV_clear, _intrinsic_reporting)        \
    static_assert(false, "Unsupported BACNET_OBJECT_TABLE for this platform")

#endif
