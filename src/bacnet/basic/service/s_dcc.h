/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic DeviceCommunicationControl service send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_DCC_H
#define SEND_DCC_H

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
uint8_t Send_Device_Communication_Control_Request(
    uint32_t device_id, uint16_t timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE state, char *password);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
