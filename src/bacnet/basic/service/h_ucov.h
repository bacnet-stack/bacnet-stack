/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic UnconfirmedCOV notification handler
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef HANDLER_UCOV_NOTIFICATION_H
#define HANDLER_UCOV_NOTIFICATION_H

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
    void handler_ucov_notification_add(
        BACNET_COV_NOTIFICATION *callback);

    BACNET_STACK_EXPORT
    void handler_ucov_notification(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
