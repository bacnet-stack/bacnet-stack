/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic ReadProperty service handler
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_READ_PROPERTY_H
#define HANDLER_READ_PROPERTY_H

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
void handler_read_property(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
