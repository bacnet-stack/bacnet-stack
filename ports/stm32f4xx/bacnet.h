/**
 * @file
 * @brief BACnet stack initialization and task processing
 * @author Steve Karg
 * @date 2021
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

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
