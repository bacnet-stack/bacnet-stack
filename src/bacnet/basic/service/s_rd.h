/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic ReinitializeDevice service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_REINITIALIZE_DEVICE_H
#define SEND_REINITIALIZE_DEVICE_H

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
uint8_t Send_Reinitialize_Device_Request(
    uint32_t device_id, BACNET_REINITIALIZED_STATE state, const char *password);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
