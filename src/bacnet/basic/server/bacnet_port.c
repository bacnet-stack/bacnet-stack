/**
 * @file
 * @brief The BACnet/IPv4 datalink tasks for handling the device specific
 *  data link layer
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
/* BACnet definitions */
#include "bacnet/bacdef.h"
/* BACnet library API */
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/object/netport.h"
#if defined(BACDL_BIP)
#include "bacnet/basic/server/bacnet_port_ipv4.h"
#elif defined(BACDL_BIP6)
#include "bacnet/basic/server/bacnet_port_ipv6.h"
#endif
/* me! */
#include "bacnet/basic/server/bacnet_port.h"

/* timer used to renew Foreign Device Registration */
static struct mstimer BACnet_Task_Timer;

/**
 * @brief Periodic tasks for the BACnet datalink layer
 */
void bacnet_port_task(void)
{
    uint32_t elapsed_milliseconds = 0;
    uint32_t elapsed_seconds = 0;

    if (mstimer_expired(&BACnet_Task_Timer)) {
        /* 1 second tasks */
        mstimer_reset(&BACnet_Task_Timer);
        /* presume that the elapsed time is the interval time */
        elapsed_milliseconds = mstimer_interval(&BACnet_Task_Timer);
        elapsed_seconds = elapsed_milliseconds / 1000;
#if defined(BACDL_BIP)
        bacnet_port_ipv4_task(elapsed_seconds);
#elif defined(BACDL_BIP6)
        bacnet_port_ipv6_task(elapsed_seconds);
#else
        /* nothing to do */
        (void)elapsed_seconds;
#endif
    }
}

/**
 * @brief Initialize the datalink network port
 */
bool bacnet_port_init(void)
{
    bool status = false;
    /* start the 1 second timer for non-critical cyclic tasks */
    mstimer_set(&BACnet_Task_Timer, 1000L);
#if defined(BACDL_BIP)
    status = bacnet_port_ipv4_init();
#elif defined(BACDL_BIP6)
    status = bacnet_port_ipv6_init();
#endif
    return status;
}
