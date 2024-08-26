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
#include "bacnet/wpm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    uint8_t Send_Write_Property_Multiple_Request(
        uint8_t * pdu,
        size_t max_pdu,
        uint32_t device_id,
        BACNET_WRITE_ACCESS_DATA * write_access_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
