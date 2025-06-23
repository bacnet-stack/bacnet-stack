/**
 * @file
 * @brief Header file for a basic Who-Am-I-Request service handler
 * @author Steve Karg
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_WHO_AM_I_H
#define HANDLER_WHO_AM_I_H

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
void handler_who_am_i_json_print(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
