/**
 * @file
 * @brief API for the BACnet datalink tasks for handling the device specific
 *  data link network port layer
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_PORT_IPV4_H
#define BACNET_PORT_IPV4_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/datalink/bvlc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bacnet_port_ipv4_foreign_device_init(
    const uint16_t ttl_seconds,
    const BACNET_IP_ADDRESS *bbmd_address);
void bacnet_port_ipv4_task(uint16_t elapsed_seconds);
bool bacnet_port_ipv4_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
