/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic WhoHas service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_WHO_HAS_H
#define SEND_WHO_HAS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Send_WhoHas_Name(
    int32_t low_limit, int32_t high_limit, const char *object_name);
BACNET_STACK_EXPORT
void Send_WhoHas_Object(
    int32_t low_limit,
    int32_t high_limit,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
