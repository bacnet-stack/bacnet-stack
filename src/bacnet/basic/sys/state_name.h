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
/* BACnet utilities */
#include "bacnet/basic/sys/keylist.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
unsigned state_name_count(const char *state_names);
BACNET_STACK_EXPORT
const char *state_name_by_index(const char *state_names, unsigned index);
BACNET_STACK_EXPORT
unsigned state_name_to_index(const char *state_names, const char *state_name);

BACNET_STACK_EXPORT
unsigned state_name_list_count(OS_Keylist list);
BACNET_STACK_EXPORT
unsigned state_name_list_index(OS_Keylist list, const char *search_name);
BACNET_STACK_EXPORT
unsigned state_name_list_init(OS_Keylist list, const char *state_text_list);
BACNET_STACK_EXPORT
bool state_name_list_set(
    OS_Keylist list,
    const char *text,
    size_t text_length,
    unsigned array_index);
BACNET_STACK_EXPORT
BACNET_ERROR_CODE state_name_list_write(
    OS_Keylist list,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *application_data,
    size_t application_data_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
