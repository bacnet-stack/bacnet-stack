/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic Who-Is service handler
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_WHO_IS_H
#define HANDLER_WHO_IS_H

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
void handler_who_is(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
void handler_who_is_unicast(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
void handler_who_is_who_am_i_unicast(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
void handler_who_is_bcast_for_routing(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
void handler_who_is_unicast_for_routing(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
