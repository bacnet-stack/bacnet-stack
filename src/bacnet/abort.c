/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <stdint.h>
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/abort.h"

/** @file abort.c  Abort Encoding/Decoding */

/**
 * @brief Convert error-code into abort-reason
 *
 * Helper function to avoid needing additional entries in service data
 * structures when passing back abort status. Convert from error code to abort
 * code. Anything not defined converts to ABORT_REASON_OTHER. Alternate
 * methods are required to return proprietary abort codes.
 *
 * @param error_code - BACnet Error Code to convert
 * @return abort code or ABORT_REASON_OTHER if not found
 */
BACNET_ABORT_REASON abort_convert_error_code(BACNET_ERROR_CODE error_code)
{
    BACNET_ABORT_REASON abort_code = ABORT_REASON_OTHER;

    switch (error_code) {
        case ERROR_CODE_ABORT_BUFFER_OVERFLOW:
            abort_code = ABORT_REASON_BUFFER_OVERFLOW;
            break;
        case ERROR_CODE_ABORT_INVALID_APDU_IN_THIS_STATE:
            abort_code = ABORT_REASON_INVALID_APDU_IN_THIS_STATE;
            break;
        case ERROR_CODE_ABORT_PREEMPTED_BY_HIGHER_PRIORITY_TASK:
            abort_code = ABORT_REASON_PREEMPTED_BY_HIGHER_PRIORITY_TASK;
            break;
        case ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED:
            abort_code = ABORT_REASON_SEGMENTATION_NOT_SUPPORTED;
            break;
        case ERROR_CODE_ABORT_SECURITY_ERROR:
            abort_code = ABORT_REASON_SECURITY_ERROR;
            break;
        case ERROR_CODE_ABORT_INSUFFICIENT_SECURITY:
            abort_code = ABORT_REASON_INSUFFICIENT_SECURITY;
            break;
        case ERROR_CODE_ABORT_PROPRIETARY:
            abort_code = ABORT_REASON_PROPRIETARY_FIRST;
            break;
        case ERROR_CODE_ABORT_OTHER:
        default:
            abort_code = ABORT_REASON_OTHER;
            break;
    }

    return (abort_code);
}

/**
 * @brief Convert error-code from abort-reason
 *
 * Helper function to avoid needing additional entries in service data
 * structures when passing back abort status. Converts to error code from
 * abort code. Anything not defined converts to ABORT_REASON_OTHER.
 * Alternate methods are required to return proprietary abort codes.
 *
 * @param abort_code - BACnet Error Code to convert
 * @return error code or ERROR_CODE_ABORT_OTHER if not found
 */
BACNET_ERROR_CODE abort_convert_to_error_code(BACNET_ABORT_REASON abort_code)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_ABORT_OTHER;

    switch (abort_code) {
        case ABORT_REASON_BUFFER_OVERFLOW:
            error_code = ERROR_CODE_ABORT_BUFFER_OVERFLOW;
            break;
        case ABORT_REASON_INVALID_APDU_IN_THIS_STATE:
            error_code = ERROR_CODE_ABORT_INVALID_APDU_IN_THIS_STATE;
            break;
        case ABORT_REASON_PREEMPTED_BY_HIGHER_PRIORITY_TASK:
            error_code = ERROR_CODE_ABORT_PREEMPTED_BY_HIGHER_PRIORITY_TASK;
            break;
        case ABORT_REASON_SEGMENTATION_NOT_SUPPORTED:
            error_code = ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            break;
        case ABORT_REASON_SECURITY_ERROR:
            error_code = ERROR_CODE_ABORT_SECURITY_ERROR;
            break;
        case ABORT_REASON_INSUFFICIENT_SECURITY:
            error_code = ERROR_CODE_ABORT_INSUFFICIENT_SECURITY;
            break;
        case ABORT_REASON_OTHER:
            error_code = ERROR_CODE_ABORT_OTHER;
            break;
        default:
            if ((abort_code >= ABORT_REASON_PROPRIETARY_FIRST) &&
                (abort_code <= ABORT_REASON_PROPRIETARY_LAST)) {
                error_code = ERROR_CODE_ABORT_PROPRIETARY;
            }
            break;
    }

    return (error_code);
}

/**
 * @brief Encode the BACnet Abort Service, returning the reason
 *        for the operation being aborted.
 *
 * @param apdu  Transmit buffer
 * @param invoke_id  ID invoked
 * @param abort_reason  Abort reason, see ABORT_REASON_X enumeration for details
 * @param server  True, if the abort has been issued by this device.
 *
 * @return Total length of the apdu, typically 3 on success, zero otherwise.
 */
int abort_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        if (server) {
            apdu[0] = PDU_TYPE_ABORT | 1;
        } else {
            apdu[0] = PDU_TYPE_ABORT;
        }
        apdu[1] = invoke_id;
        apdu[2] = abort_reason;
        apdu_len = 3;
    }

    return apdu_len;
}

#if !BACNET_SVC_SERVER
/**
 * @brief Decode the BACnet Abort Service, returning the reason
 *        for the operation being aborted.
 *
 * @param apdu  Receive buffer
 * @param apdu_len  Count of bytes valid in the received buffer.
 * @param invoke_id  Pointer to a variable, taking the invoked ID from the
 * message.
 * @param abort_reason  Pointer to a variable, taking the abort reason, see
 * ABORT_REASON_X enumeration for details
 *
 * @return Total length of the apdu, typically 2 on success, zero otherwise.
 */
int abort_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, uint8_t *invoke_id, uint8_t *abort_reason)
{
    int len = 0;

    if (apdu_len > 0) {
        if (invoke_id) {
            *invoke_id = apdu[0];
        }
        len++;

        if (apdu_len > 1) {
            if (abort_reason) {
                *abort_reason = apdu[1];
            }

            len++;
        }
    }

    return len;
}
#endif
