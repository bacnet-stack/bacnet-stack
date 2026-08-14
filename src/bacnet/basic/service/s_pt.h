/**
 * @file
 * @brief Header file for a basic ConfirmedPrivateTransfer service send
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef SEND_CONFIRMED_PRIVATE_TRANSFER_H
#define SEND_CONFIRMED_PRIVATE_TRANSFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/bacapp.h"
#include "bacnet/ptransfer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint8_t Send_Private_Transfer_Request(
    uint8_t *pdu,
    size_t max_pdu,
    uint32_t device_id,
    uint16_t vendor_id,
    uint32_t service_number,
    const BACNET_APPLICATION_DATA_VALUE *value_list);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
