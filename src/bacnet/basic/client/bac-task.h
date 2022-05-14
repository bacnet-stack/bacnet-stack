/**
 * @file
 * @author Steve Karg
 * @date 2013
 * @brief High level BACnet task handling
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_TASK_H
#define BACNET_TASK_H

#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacnet_task_init(void);
BACNET_STACK_EXPORT
void bacnet_task(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
