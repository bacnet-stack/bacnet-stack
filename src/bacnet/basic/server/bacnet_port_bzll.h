/**
 * @file
 * @brief The BACnet BZLL datalink tasks for handling the device specific
 *  data link network port layer
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2026
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACNET_PORT_BZLL_H
#define BACNET_PORT_BZLL_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/datalink/bzll.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacnet_port_bzll_task(uint16_t elapsed_seconds);
BACNET_STACK_EXPORT
bool bacnet_port_bzll_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
