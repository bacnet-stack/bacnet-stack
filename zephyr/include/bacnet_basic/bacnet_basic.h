/**
 * @file
 * @brief BACnet Basic Stack initialization and basic task handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_H
#define BACNET_BASIC_H

#include <stdint.h>

typedef void (*bacnet_basic_callback)(void *context);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bacnet_basic_init(void);
void bacnet_basic_init_callback_set(bacnet_basic_callback callback, 
    void *context);

void bacnet_basic_task(void);
void bacnet_basic_task_callback_set(bacnet_basic_callback callback, 
    void *context);

unsigned long bacnet_basic_uptime_seconds(void);
unsigned long bacnet_basic_packet_count(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
