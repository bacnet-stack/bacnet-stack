/**
 * @file
 * @brief API for I-Am service encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_I_AM_H
#define BACNET_I_AM_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaddr.h"
#include "bacnet/npdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int iam_encode_apdu(
    uint8_t *apdu,
    uint32_t device_id,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id);

BACNET_STACK_EXPORT
int iam_decode_service_request(
    const uint8_t *apdu,
    uint32_t *pDevice_id,
    unsigned *pMax_apdu,
    int *pSegmentation,
    uint16_t *pVendor_id);

BACNET_STACK_EXPORT
int iam_request_decode(
    const uint8_t *apdu,
    unsigned apdu_size,
    uint32_t *pDevice_id,
    unsigned *pMax_apdu,
    int *pSegmentation,
    uint16_t *pVendor_id);

BACNET_STACK_EXPORT
int iam_request_encode(
    uint8_t *apdu,
    uint32_t device_id,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
