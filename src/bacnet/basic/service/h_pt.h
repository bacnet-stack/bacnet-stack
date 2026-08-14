/**
 * @file
 * @brief Header file for a basic ConfirmedPrivateTransfer service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_CONFIRMED_PRIVATE_TRANSFER_H
#define HANDLER_CONFIRMED_PRIVATE_TRANSFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int handler_confirmed_private_transfer_encode(
    uint8_t *pdu,
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);

BACNET_STACK_EXPORT
void handler_confirmed_private_transfer(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
