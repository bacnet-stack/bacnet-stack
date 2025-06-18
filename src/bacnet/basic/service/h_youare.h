/**
 * @file
 * @brief Header file for a basic You-Are-Request service handler
 * @author Steve Karg
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef HANDLER_YOU_ARE_H
#define HANDLER_YOU_ARE_H

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
void handler_you_are_json_print(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
