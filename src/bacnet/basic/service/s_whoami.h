/**
 * @file
 * @brief Header file for a basic Who-Am-I service request
 * @author Steve Karg
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef SEND_WHO_AM_I_H
#define SEND_WHO_AM_I_H

#include <stddef.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int Send_Who_Am_I_To_Network(
    BACNET_ADDRESS *target_address,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number);

BACNET_STACK_EXPORT
int Send_Who_Am_I_Broadcast(
    uint16_t device_vendor_id,
    const char *device_model_name,
    const char *device_serial_number);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
