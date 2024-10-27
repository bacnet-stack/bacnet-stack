/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic WritePropertyMultiple service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_ATOMIC_READ_FILE_STREAM_H
#define SEND_ATOMIC_READ_FILE_STREAM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint8_t Send_Atomic_Read_File_Stream(
    uint32_t device_id,
    uint32_t file_instance,
    int fileStartPosition,
    unsigned requestedOctetCount);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
