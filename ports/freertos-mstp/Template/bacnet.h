/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief BACnet initialization and tasks
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef BACNET_H
#define BACNET_H
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

  void bacnet_init (void);
  void bacnet_task (void);
  void bacnet_task_delay_milliseconds(unsigned milliseconds);

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif
