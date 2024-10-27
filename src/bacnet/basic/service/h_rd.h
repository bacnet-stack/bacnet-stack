/**
 * @file
 * @brief Header file for a basic ReinitializeDevice service handler
 * @author Steve Karg
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_HANDLER_REINITIALIZE_DEVICE_H
#define BACNET_BASIC_SERVICE_HANDLER_REINITIALIZE_DEVICE_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void handler_reinitialize_device(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
