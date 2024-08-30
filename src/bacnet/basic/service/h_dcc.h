/**
 * @file
 * @brief Header file for a basic DeviceCommunicationControl request handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2019
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_HANDLER_DCC_H
#define BACNET_BASIC_SERVICE_HANDLER_DCC_H
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
void handler_device_communication_control(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);
BACNET_STACK_EXPORT
void handler_dcc_password_set(const char *new_password);
BACNET_STACK_EXPORT
char *handler_dcc_password(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
