/**
 * @file
 * @author Steve Karg
 * @date 2004
 * @brief BACnet protocol stack version 0.0.0 - 255.255.255
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_VERSION_H_
#define BACNET_VERSION_H_
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* This BACnet protocol stack version 0.0.0 - 255.255.255 */
#ifndef BACNET_VERSION
#define BACNET_VERSION(x, y, z) (((x) << 16) + ((y) << 8) + (z))
#endif

#define BACNET_VERSION_TEXT "1.4.0.10"
#define BACNET_VERSION_CODE BACNET_VERSION(1, 4, 0)
#define BACNET_VERSION_MAJOR ((BACNET_VERSION_CODE >> 16) & 0xFF)
#define BACNET_VERSION_MINOR ((BACNET_VERSION_CODE >> 8) & 0xFF)
#define BACNET_VERSION_MAINTENANCE (BACNET_VERSION_CODE & 0xFF)

#endif
