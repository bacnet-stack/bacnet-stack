/**
 * @file
 * @brief The WriteGroup-Request service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_WRITE_GROUP_H
#define HANDLER_WRITE_GROUP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/write_group.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void handler_write_group(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);
BACNET_STACK_EXPORT
void handler_write_group_print_data(BACNET_WRITE_GROUP_DATA *data);
BACNET_STACK_EXPORT
void handler_write_group_notification_add(BACNET_WRITE_GROUP_NOTIFICATION *cb);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
