/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic WritePropertyMultiple service send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_WRITE_PROPERTY_MULTIPLE_H
#define SEND_WRITE_PROPERTY_MULTIPLE_H

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

uint8_t Send_Private_Transfer_Request(uint32_t device_id, uint16_t vendor_id,
                                      uint32_t service_number,
                                      char block_number, DATABLOCK *block);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
