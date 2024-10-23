/**
 * @file
 * @brief BACnet Abort Reason Encoding and Decoding
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Peter McShane <petermcs@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/abort.h"

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
        case ERROR_CODE_ABORT_WINDOW_SIZE_OUT_OF_RANGE:
            abort_code = ABORT_REASON_WINDOW_SIZE_OUT_OF_RANGE;
            break;
        case ERROR_CODE_ABORT_APPLICATION_EXCEEDED_REPLY_TIME:
            abort_code = ABORT_REASON_APPLICATION_EXCEEDED_REPLY_TIME;
            break;
        case ERROR_CODE_ABORT_OUT_OF_RESOURCES:
            abort_code = ABORT_REASON_OUT_OF_RESOURCES;
            break;
        case ERROR_CODE_ABORT_TSM_TIMEOUT:
            abort_code = ABORT_REASON_TSM_TIMEOUT;
            break;
        case ERROR_CODE_ABORT_APDU_TOO_LONG:
            abort_code = ABORT_REASON_APDU_TOO_LONG;
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
 * @brief Determine if a BACnetErrorCode is a BACnetAbortReason
 * @param error_code #BACNET_ERROR_CODE enumeration
 * @return true if the BACnet Error Code is a BACnet abort reason
 */
bool abort_valid_error_code(BACNET_ERROR_CODE error_code)
{
    bool status = false;

    switch (error_code) {
        case ERROR_CODE_ABORT_OTHER:
        case ERROR_CODE_ABORT_BUFFER_OVERFLOW:
        case ERROR_CODE_ABORT_INVALID_APDU_IN_THIS_STATE:
        case ERROR_CODE_ABORT_PREEMPTED_BY_HIGHER_PRIORITY_TASK:
        case ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED:
        case ERROR_CODE_ABORT_SECURITY_ERROR:
        case ERROR_CODE_ABORT_INSUFFICIENT_SECURITY:
        case ERROR_CODE_ABORT_WINDOW_SIZE_OUT_OF_RANGE:
        case ERROR_CODE_ABORT_APPLICATION_EXCEEDED_REPLY_TIME:
        case ERROR_CODE_ABORT_OUT_OF_RESOURCES:
        case ERROR_CODE_ABORT_TSM_TIMEOUT:
        case ERROR_CODE_ABORT_APDU_TOO_LONG:
        case ERROR_CODE_ABORT_PROPRIETARY:
            status = true;
            break;
        default:
            break;
    }

    return status;
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
        case ABORT_REASON_OTHER:
            error_code = ERROR_CODE_ABORT_OTHER;
            break;
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
        case ABORT_REASON_WINDOW_SIZE_OUT_OF_RANGE:
            error_code = ERROR_CODE_ABORT_WINDOW_SIZE_OUT_OF_RANGE;
            break;
        case ABORT_REASON_APPLICATION_EXCEEDED_REPLY_TIME:
            error_code = ERROR_CODE_ABORT_APPLICATION_EXCEEDED_REPLY_TIME;
            break;
        case ABORT_REASON_OUT_OF_RESOURCES:
            error_code = ERROR_CODE_ABORT_OUT_OF_RESOURCES;
            break;
        case ABORT_REASON_TSM_TIMEOUT:
            error_code = ERROR_CODE_ABORT_TSM_TIMEOUT;
            break;
        case ABORT_REASON_APDU_TOO_LONG:
            error_code = ERROR_CODE_ABORT_APDU_TOO_LONG;
            break;
        default:
            if (abort_code >= ABORT_REASON_PROPRIETARY_FIRST) {
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
    const uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint8_t *abort_reason)
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
