/**
 * @file
 * @brief Header file for a basic You-Are service request
 * @author Steve Karg
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef SEND_YOU_ARE_H
#define SEND_YOU_ARE_H

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
int Send_You_Are_To_Network(
    BACNET_ADDRESS *target_address,
    uint32_t device_id,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number,
    const BACNET_OCTET_STRING *mac_address);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
