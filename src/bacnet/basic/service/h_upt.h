/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic UnconfirmedPrivateTransfer service handler
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
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
void handler_unconfirmed_private_transfer(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);
BACNET_STACK_EXPORT
void private_transfer_print_data(BACNET_PRIVATE_TRANSFER_DATA *private_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
