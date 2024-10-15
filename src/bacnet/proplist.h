/**
 * @file
 * @brief Property_List property encode decode helper
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_PROPLIST_H
#define BACNET_PROPLIST_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/rp.h"

struct property_list_t {
    const int *pList;
    unsigned count;
};

struct special_property_list_t {
    struct property_list_t Required;
    struct property_list_t Optional;
    struct property_list_t Proprietary;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
unsigned property_list_count(const int *pList);
BACNET_STACK_EXPORT
bool property_list_member(const int *pList, int object_property);
BACNET_STACK_EXPORT
bool property_lists_member(
    const int *pRequired,
    const int *pOptional,
    const int *pProprietary,
    int object_property);
BACNET_STACK_EXPORT
int property_list_encode(
    BACNET_READ_PROPERTY_DATA *rpdata,
    const int *pListRequired,
    const int *pListOptional,
    const int *pListProprietary);
BACNET_STACK_EXPORT
int property_list_common_encode(
    BACNET_READ_PROPERTY_DATA *rpdata, uint32_t device_instance_number);
BACNET_STACK_EXPORT
bool property_list_common(BACNET_PROPERTY_ID property);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
