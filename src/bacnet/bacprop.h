/**
 * @file
 * @brief BACnet property tag lookup functions
 * @author John Goulah <bigjohngoulah@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_PROPERTY_H
#define BACNET_PROPERTY_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

typedef struct {
    signed prop_id;     /* index number that matches the text */
    signed tag_id;      /* text pair - use NULL to end the list */
} PROP_TAG_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    signed bacprop_tag_by_index_default(
        const PROP_TAG_DATA * data_list,
        signed index,
        signed default_ret);

    BACNET_STACK_EXPORT
    signed bacprop_property_tag(
        BACNET_OBJECT_TYPE type,
        signed prop);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
