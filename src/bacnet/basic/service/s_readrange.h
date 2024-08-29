/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic ReadRange service send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_READ_RANGE_H
#define SEND_READ_RANGE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"
#include "bacnet/readrange.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint8_t Send_ReadRange_Request(uint32_t device_id,
                               const BACNET_READ_RANGE_DATA* read_access_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
