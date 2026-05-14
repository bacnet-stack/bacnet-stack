/**
 * @file
 * @brief API for BACnet state name utility functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_STATE_NAME_H
#define BACNET_SYS_STATE_NAME_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
unsigned state_name_count(const char *state_names);
BACNET_STACK_EXPORT
const char *state_name_by_index(const char *state_names, unsigned index);
BACNET_STACK_EXPORT
unsigned state_name_to_index(const char *state_names, const char *state_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
