/**
 * @file
 * @author Steve Karg
 * @date October 2019
 * @brief Header file for a basic TimeSynchronization service send
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_TIME_SYNC_H
#define SEND_TIME_SYNC_H

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
void Send_TimeSync(const BACNET_DATE *bdate, const BACNET_TIME *btime);
BACNET_STACK_EXPORT
void Send_TimeSync_Remote(
    BACNET_ADDRESS *dest, const BACNET_DATE *bdate, const BACNET_TIME *btime);
BACNET_STACK_EXPORT
void Send_TimeSyncUTC(const BACNET_DATE *bdate, const BACNET_TIME *btime);
BACNET_STACK_EXPORT
void Send_TimeSyncUTC_Remote(
    BACNET_ADDRESS *dest, const BACNET_DATE *bdate, const BACNET_TIME *btime);
BACNET_STACK_EXPORT
void Send_TimeSyncUTC_Device(void);
BACNET_STACK_EXPORT
void Send_TimeSync_Device(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
