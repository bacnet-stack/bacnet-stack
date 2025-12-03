/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic ReadPropertyMultiple service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_READ_PROPERTY_MULTIPLE_H
#define SEND_READ_PROPERTY_MULTIPLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"
#include "bacnet/rpm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint8_t Send_Read_Property_Multiple_Request_Address(
    BACNET_ADDRESS *dest,
    uint16_t max_apdu,
    uint8_t *pdu,
    size_t max_pdu,
    BACNET_READ_ACCESS_DATA *read_access_data);
BACNET_STACK_EXPORT
uint8_t Send_Read_Property_Multiple_Request(
    uint8_t *pdu,
    size_t max_pdu,
    uint32_t device_id, /* destination device */
    BACNET_READ_ACCESS_DATA *read_access_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
