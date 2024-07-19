/**
 * @file
 * @brief BACnet Stack initialization and task handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_H
#define BACNET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bacnet_init(void);
void bacnet_task(void);
unsigned long bacnet_uptime_seconds(void);
unsigned long bacnet_packet_count(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
