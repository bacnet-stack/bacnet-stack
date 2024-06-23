/**
 * @file
 * @brief Header file for a basic ConfirmedCOV notification handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_HANDLER_CCOV_NOTIFICATION_H
#define BACNET_BASIC_SERVICE_HANDLER_CCOV_NOTIFICATION_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/cov.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    BACNET_STACK_EXPORT
    void handler_ccov_notification_add(
        BACNET_COV_NOTIFICATION *callback);

    BACNET_STACK_EXPORT
    void handler_ccov_notification(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
