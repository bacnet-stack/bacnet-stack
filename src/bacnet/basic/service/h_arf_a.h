/**
 * @file
 * @brief Header file for a basic Atomic Read File Ack handler
 * @author Steve Karg
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_ATOMIC_READ_FILE_ACK_HANDLER_H
#define BACNET_BASIC_SERVICE_ATOMIC_READ_FILE_ACK_HANDLER_H
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
void handler_atomic_read_file_ack(
    uint8_t* service_request, uint16_t service_len, BACNET_ADDRESS* src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA* service_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
