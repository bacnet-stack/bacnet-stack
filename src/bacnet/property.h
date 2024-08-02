/**
 * @file
 * @brief Library of all required and optional object properties
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_PROPERTY_H
#define BACNET_PROPERTY_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/rp.h"
#include "bacnet/proplist.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    const int * property_list_optional(
        BACNET_OBJECT_TYPE object_type);
    BACNET_STACK_EXPORT
    const int * property_list_required(
        BACNET_OBJECT_TYPE object_type);
    BACNET_STACK_EXPORT
    void property_list_special(
        BACNET_OBJECT_TYPE object_type,
        struct special_property_list_t *pPropertyList);
    BACNET_STACK_EXPORT
    BACNET_PROPERTY_ID property_list_special_property(
        BACNET_OBJECT_TYPE object_type,
        BACNET_PROPERTY_ID special_property,
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned property_list_special_count(
        BACNET_OBJECT_TYPE object_type,
        BACNET_PROPERTY_ID special_property);

    BACNET_STACK_EXPORT
    const int *property_list_bacnet_array(
        void);
    BACNET_STACK_EXPORT
    bool property_list_bacnet_array_member(
        BACNET_OBJECT_TYPE object_type,
        BACNET_PROPERTY_ID object_property);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
