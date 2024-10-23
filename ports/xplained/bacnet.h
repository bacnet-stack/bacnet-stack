/**
 * @file
 * @brief API for BACnet task and initialization for the Xplained board
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_H
#define BACNET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  void bacnet_init (void);
  void bacnet_task (void);
  void bacnet_task_timed(
      void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
