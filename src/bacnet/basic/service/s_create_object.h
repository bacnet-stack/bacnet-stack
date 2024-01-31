/**
 * @file
 * @brief CreateObject service initiation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_CREATE_OBJECT_H
#define SEND_CREATE_OBJECT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint8_t Send_Create_Object_Request_Data(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE *values);

BACNET_STACK_EXPORT
uint8_t Send_Create_Object_Request(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
