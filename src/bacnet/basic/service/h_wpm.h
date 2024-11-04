/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic WritePropertyMultiple service handler
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_WRITE_PROPERTY_MULTIPLE_H
#define HANDLER_WRITE_PROPERTY_MULTIPLE_H

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
void handler_write_property_multiple(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
