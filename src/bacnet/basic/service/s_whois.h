/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic WhoIs service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_WHO_IS_H
#define SEND_WHO_IS_H

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
void Send_WhoIs(int32_t low_limit, int32_t high_limit);
BACNET_STACK_EXPORT
void Send_WhoIs_Global(int32_t low_limit, int32_t high_limit);
BACNET_STACK_EXPORT
void Send_WhoIs_Local(int32_t low_limit, int32_t high_limit);
BACNET_STACK_EXPORT
void Send_WhoIs_Remote(
    BACNET_ADDRESS *target_address, int32_t low_limit, int32_t high_limit);
BACNET_STACK_EXPORT
void Send_WhoIs_To_Network(
    BACNET_ADDRESS *target_address, int32_t low_limit, int32_t high_limit);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
