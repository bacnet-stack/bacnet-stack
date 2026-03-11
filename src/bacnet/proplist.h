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
    const int32_t *pList;
    uint32_t count;
};

struct special_property_list_t {
    struct property_list_t Required;
    struct property_list_t Optional;
    struct property_list_t Proprietary;
};

/**
 * @brief Callback function type for fetching a property list for a given
 * object instance.
 * @param object_type [in] The BACNET_OBJECT_TYPE of the object instance
 *  to fetch the property list for.
 * @param object_instance [in] The object instance number of the object
 *  to fetch the property list for.
 * @param pPropertyList [out] Pointer to a property list to be
 *  filled with the property list for this object instance.
 * @return True if the object instance is valid and the property list has been
 *  filled in, false if the object instance is not valid or the property list
 *  could not be filled in.
 */
typedef bool (*property_list_function)(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    const int32_t **property_list);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint32_t property_list_count(const int32_t *pList);
BACNET_STACK_EXPORT
bool property_list_member(const int32_t *pList, int32_t object_property);
BACNET_STACK_EXPORT
bool property_lists_member(
    const int32_t *pRequired,
    const int32_t *pOptional,
    const int32_t *pProprietary,
    int32_t object_property);
BACNET_STACK_EXPORT
int property_list_encode(
    BACNET_READ_PROPERTY_DATA *rpdata,
    const int32_t *pListRequired,
    const int32_t *pListOptional,
    const int32_t *pListProprietary);
BACNET_STACK_EXPORT
int property_list_common_encode(
    BACNET_READ_PROPERTY_DATA *rpdata, uint32_t device_instance_number);
BACNET_STACK_EXPORT
bool property_list_common(BACNET_PROPERTY_ID property);

BACNET_STACK_EXPORT
const int32_t *property_list_bacnet_array(void);
BACNET_STACK_EXPORT
bool property_list_bacnet_array_member(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID object_property);
BACNET_STACK_EXPORT
const int32_t *property_list_bacnet_list(void);
BACNET_STACK_EXPORT
bool property_list_bacnet_list_member(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID object_property);
BACNET_STACK_EXPORT
bool property_list_commandable_member(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID object_property);
BACNET_STACK_EXPORT
bool property_list_read_only_member(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID object_property);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
