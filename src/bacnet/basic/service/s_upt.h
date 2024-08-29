/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic UnconfirmedPrivateTransfer service send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_UNCONFIRMED_PRIVATE_TRANSFER_H
#define SEND_UNCONFIRMED_PRIVATE_TRANSFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"
#include "bacnet/ptransfer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int Send_UnconfirmedPrivateTransfer(
    BACNET_ADDRESS* dest,
    const BACNET_PRIVATE_TRANSFER_DATA* private_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
