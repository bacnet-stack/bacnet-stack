/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
* Multimedia Timer contribution by Cameron Crothers, 2008
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#include <zephyr/kernel.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/mstimer.h"

/**
* @brief returns the current millisecond count
* @return millisecond counter
*/
unsigned long mstimer_now(void)
{
    return (unsigned long) k_uptime_get_32();
}

/**
* @brief Initialization for timer
*/
void mstimer_init(void)
{

}
