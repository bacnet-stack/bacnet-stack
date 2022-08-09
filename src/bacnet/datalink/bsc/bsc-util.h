/**
 * @file 
 * @brief module for common function for BACnet/SC implementation
 * @author Kirill Neznamov
 * @date Jule 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC 
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BACNET__SC__UTIL__INCLUDED
#define __BACNET__SC__UTIL__INCLUDED

#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/debug.h"

unsigned long bsc_seconds_left(
    unsigned long timestamp_ms, unsigned long timeout_s);

#endif