/**
 * @file
 * @brief A POSIX BACnet File Object implementation.
 * @author Steve Karg
 * @date July 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_FILE_POSIX_H
#define BACNET_FILE_POSIX_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacfile_posix_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
