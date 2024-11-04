/**
 * @file
 * @brief BACnet ReinitializeDevice encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_REINITIALIZE_DEVICE_H
#define BACNET_REINITIALIZE_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

typedef struct BACnet_Reinitialize_Device_Data {
    BACNET_REINITIALIZED_STATE state;
    BACNET_CHARACTER_STRING password;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_REINITIALIZE_DEVICE_DATA;

typedef bool (*reinitialize_device_function)(
    BACNET_REINITIALIZE_DEVICE_DATA *rd_data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int rd_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    BACNET_REINITIALIZED_STATE state,
    const BACNET_CHARACTER_STRING *password);

BACNET_STACK_EXPORT
int reinitialize_device_encode(
    uint8_t *apdu,
    BACNET_REINITIALIZED_STATE state,
    const BACNET_CHARACTER_STRING *password);

BACNET_STACK_EXPORT
size_t reinitialize_device_request_encode(
    uint8_t *apdu,
    size_t apdu_size,
    BACNET_REINITIALIZED_STATE state,
    const BACNET_CHARACTER_STRING *password);

/* decode the service request only */
BACNET_STACK_EXPORT
int rd_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_len,
    BACNET_REINITIALIZED_STATE *state,
    BACNET_CHARACTER_STRING *password);

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DMRD Device Management-ReinitializeDevice (DM-RD)
 * @ingroup RDMS
 * 16.4 ReinitializeDevice Service <br>
 * The ReinitializeDevice service is used by a client BACnet-user to instruct
 * a remote device to reboot itself (cold start), reset itself to some
 * predefined initial state (warm start), or to control the backup or restore
 * procedure. Resetting or rebooting a device is primarily initiated by a human
 * operator for diagnostic purposes. Use of this service during the backup or
 * restore procedure is usually initiated on behalf of the user by the device
 * controlling the backup or restore. Due to the sensitive nature of this
 * service, a password may be required from the responding BACnet-user prior
 * to executing the service.
 *
 */
#endif
