/**
 * @file
 * @brief BACnet PrivateTransfer encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_PRIVATE_TRANSFER_H
#define BACNET_PRIVATE_TRANSFER_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

typedef struct BACnet_Private_Transfer_Data {
    uint16_t vendorID;
    uint32_t serviceNumber;
    uint8_t *serviceParameters;
    int serviceParametersLen;
} BACNET_PRIVATE_TRANSFER_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int ptransfer_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    const BACNET_PRIVATE_TRANSFER_DATA *private_data);
BACNET_STACK_EXPORT
int uptransfer_encode_apdu(
    uint8_t *apdu, const BACNET_PRIVATE_TRANSFER_DATA *private_data);
BACNET_STACK_EXPORT
int ptransfer_decode_service_request(
    uint8_t *apdu,
    unsigned apdu_len,
    BACNET_PRIVATE_TRANSFER_DATA *private_data);

BACNET_STACK_EXPORT
int ptransfer_error_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code,
    const BACNET_PRIVATE_TRANSFER_DATA *private_data);
BACNET_STACK_EXPORT
int ptransfer_error_decode_service_request(
    uint8_t *apdu,
    unsigned apdu_len,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code,
    BACNET_PRIVATE_TRANSFER_DATA *private_data);

BACNET_STACK_EXPORT
int ptransfer_ack_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    const BACNET_PRIVATE_TRANSFER_DATA *private_data);
/* ptransfer_ack_decode_service_request() is the same as
       ptransfer_decode_service_request */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
