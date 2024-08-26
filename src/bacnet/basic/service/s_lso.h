/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic LifeSafetyOperation service send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_LIFE_SAFETY_OPERATION_H
#define SEND_LIFE_SAFETY_OPERATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"
#include "bacnet/lso.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint8_t Send_Life_Safety_Operation_Data(uint32_t device_id,
                                        BACNET_LSO_DATA* data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
