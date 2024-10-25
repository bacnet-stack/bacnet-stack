/**
 * @file
 * @brief Header file for a basic BACnet WriteGroup-Reqeust service send
 * @author Steve Karg
 * @date October 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SERVICE_SEND_WRITE_GROUP_H
#define BACNET_SERVICE_SEND_WRITE_GROUP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/write_group.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int Send_Write_Group(BACNET_ADDRESS *dest, const BACNET_WRITE_GROUP_DATA *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
