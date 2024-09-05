/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic ReadPropertyMultiple-Ack service handler
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_READ_PROPERTY_MULTIPLE_ACK_H
#define HANDLER_READ_PROPERTY_MULTIPLE_ACK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/rpm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void handler_read_property_multiple_ack(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data);
BACNET_STACK_EXPORT
int rpm_ack_decode_service_request(
    const uint8_t *apdu,
    int apdu_len,
    BACNET_READ_ACCESS_DATA *read_access_data);
BACNET_STACK_EXPORT
void rpm_ack_print_data(BACNET_READ_ACCESS_DATA *rpm_data);
BACNET_STACK_EXPORT
BACNET_READ_ACCESS_DATA *rpm_data_free(BACNET_READ_ACCESS_DATA *rpm_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
