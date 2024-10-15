/**
 * @file
 * @brief BACnet Reject message encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_REJECT_H
#define BACNET_REJECT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
BACNET_REJECT_REASON reject_convert_error_code(BACNET_ERROR_CODE error_code);
BACNET_STACK_EXPORT
bool reject_valid_error_code(BACNET_ERROR_CODE error_code);
BACNET_STACK_EXPORT
BACNET_ERROR_CODE
reject_convert_to_error_code(BACNET_REJECT_REASON reject_code);

BACNET_STACK_EXPORT
int reject_encode_apdu(uint8_t *apdu, uint8_t invoke_id, uint8_t reject_reason);

BACNET_STACK_EXPORT
int reject_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint8_t *reject_reason);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
