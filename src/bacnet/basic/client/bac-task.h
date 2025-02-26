/**
 * @file
 * @brief API for high level BACnet Task handling
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_CLIENT_TASK_H
#define BACNET_BASIC_CLIENT_TASK_H
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bacnet_task_init(void);
void bacnet_task(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
