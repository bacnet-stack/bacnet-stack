/**
 * @file
 * @brief BACnetAssignedAccessRights structure and codecs
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_ASSIGNED_ACCESS_RIGHTS_H
#define BACNET_ASSIGNED_ACCESS_RIGHTS_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacdevobjpropref.h"

typedef struct BACnetAssignedAccessRights {
    BACNET_DEVICE_OBJECT_REFERENCE assigned_access_rights;
    bool enable;
} BACNET_ASSIGNED_ACCESS_RIGHTS;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int bacapp_encode_assigned_access_rights(
        uint8_t * apdu,
        const BACNET_ASSIGNED_ACCESS_RIGHTS * aar);
    BACNET_STACK_EXPORT
    int bacapp_encode_context_assigned_access_rights(
        uint8_t * apdu,
        uint8_t tag,
        const BACNET_ASSIGNED_ACCESS_RIGHTS * aar);
    BACNET_STACK_EXPORT
    int bacapp_decode_assigned_access_rights(
        const uint8_t * apdu,
        BACNET_ASSIGNED_ACCESS_RIGHTS * aar);
    BACNET_STACK_EXPORT
    int bacapp_decode_context_assigned_access_rights(
        const uint8_t * apdu,
        uint8_t tag,
        BACNET_ASSIGNED_ACCESS_RIGHTS * aar);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
