/**
 * @file
 * @brief BACnet Abort Reason Encoding and Decoding
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Peter McShane <petermcs@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_ABORT_H
#define BACNET_ABORT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    BACNET_ABORT_REASON abort_convert_error_code(
        BACNET_ERROR_CODE error_code);
    BACNET_STACK_EXPORT
    bool abort_valid_error_code(
        BACNET_ERROR_CODE error_code);
    BACNET_STACK_EXPORT
    BACNET_ERROR_CODE abort_convert_to_error_code(
        BACNET_ABORT_REASON abort_code);

    BACNET_STACK_EXPORT
    int abort_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        uint8_t abort_reason,
        bool server);

    BACNET_STACK_EXPORT
    int abort_decode_service_request(
        const uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        uint8_t * abort_reason);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
