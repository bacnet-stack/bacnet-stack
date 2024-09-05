/**
 * @file
 * @author Steve Karg
 * @date 2006
 * @brief Header file for a basic ReadProperty-Ack service handler
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_HANDLER_READ_PROPERTY_ACK_H
#define BACNET_BASIC_SERVICE_HANDLER_READ_PROPERTY_ACK_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/rp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void handler_read_property_ack(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data);
BACNET_STACK_EXPORT
void rp_ack_print_data(BACNET_READ_PROPERTY_DATA *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
