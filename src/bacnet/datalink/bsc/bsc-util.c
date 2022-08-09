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
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include "bacnet/datalink/bsc/bsc-util.h"

unsigned long bsc_seconds_left(
    unsigned long timestamp_ms, unsigned long timeout_s)
{
    unsigned long cur = mstimer_now();
    unsigned long delta;

    if (cur > timestamp_ms) {
        delta = cur - timestamp_ms;
    } else {
        delta = timestamp_ms - cur;
    }

    if (delta > timeout_s * 1000) {
        return 0;
    }

    return timeout_s * 1000 - delta;
}