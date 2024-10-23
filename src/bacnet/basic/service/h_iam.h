/**
 * @file
 * @brief Header file for a basic I-Am service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_HANDLER_I_AM_H
#define BACNET_BASIC_SERVICE_HANDLER_I_AM_H
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
void handler_i_am_add(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
void handler_i_am_bind(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
