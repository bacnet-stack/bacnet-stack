/**
 * @file
 * @brief API for CreateObject service handlers
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_CREATE_OBJECT_H
#define HANDLER_CREATE_OBJECT_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacdef.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void handler_create_object(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA *service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
