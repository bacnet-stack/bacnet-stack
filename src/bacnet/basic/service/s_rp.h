/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic ReadProperty service send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_READ_PROPERTY_H
#define SEND_READ_PROPERTY_H

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
    uint8_t Send_Read_Property_Request_Address(
        BACNET_ADDRESS * dest,
        uint16_t max_apdu,
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance,
        BACNET_PROPERTY_ID object_property,
        uint32_t array_index);
    BACNET_STACK_EXPORT
    uint8_t Send_Read_Property_Request(
        uint32_t device_id,     /* destination device */
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance,
        BACNET_PROPERTY_ID object_property,
        uint32_t array_index);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
