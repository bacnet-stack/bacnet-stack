/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic GetEvent service send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_GET_EVENT_H
#define SEND_GET_EVENT_H

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
uint8_t Send_GetEvent(BACNET_ADDRESS* target_address,
                      BACNET_OBJECT_ID* lastReceivedObjectIdentifier);
BACNET_STACK_EXPORT
uint8_t Send_GetEvent_Global(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
