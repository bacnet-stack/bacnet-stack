/**
 * @file
 * @brief Header file for a basic UnconfirmedPrivateTransfer service handler
 * @author Steve Karg
 * @date October 2019
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_UNCONFIRMED_PRIVATE_TRANSFER_H
#define HANDLER_UNCONFIRMED_PRIVATE_TRANSFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/ptransfer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void handler_unconfirmed_private_transfer_callback_set(
    handler_private_transfer_callback_t cb);

BACNET_STACK_EXPORT
void handler_unconfirmed_private_transfer(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
void private_transfer_print_data(BACNET_PRIVATE_TRANSFER_DATA *private_data);
BACNET_STACK_EXPORT
BACNET_ERROR_CODE handler_private_transfer_print(
    BACNET_PRIVATE_TRANSFER_DATA *data, void *context);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
