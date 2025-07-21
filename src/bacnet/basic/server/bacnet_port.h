/**
 * @file
 * @brief The BACnet datalink tasks for handling the device specific
 *  data link network port layer
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_PORT_H
#define BACNET_PORT_H

#include <stdint.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
bool bacnet_port_init(void);
BACNET_STACK_EXPORT
void bacnet_port_task(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
