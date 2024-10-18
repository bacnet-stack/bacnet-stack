/**
 * @file
 * @brief BACnet Reject message encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/reject.h"

/**
 * @brief Convert an error code BACnet Reject code
 * @param error_code - value to be converted
 * @return reject_code converted. Anything not defined gets converted
 * to REJECT_REASON_OTHER.
 */
BACNET_REJECT_REASON reject_convert_error_code(BACNET_ERROR_CODE error_code)
{
    BACNET_REJECT_REASON reject_code = REJECT_REASON_OTHER;

    switch (error_code) {
        case ERROR_CODE_REJECT_BUFFER_OVERFLOW:
            reject_code = REJECT_REASON_BUFFER_OVERFLOW;
            break;
        case ERROR_CODE_REJECT_INCONSISTENT_PARAMETERS:
            reject_code = REJECT_REASON_INCONSISTENT_PARAMETERS;
            break;
        case ERROR_CODE_REJECT_INVALID_PARAMETER_DATA_TYPE:
            reject_code = REJECT_REASON_INVALID_PARAMETER_DATA_TYPE;
            break;
        case ERROR_CODE_REJECT_INVALID_TAG:
            reject_code = REJECT_REASON_INVALID_TAG;
            break;
        case ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER:
            reject_code = REJECT_REASON_MISSING_REQUIRED_PARAMETER;
            break;
        case ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE:
            reject_code = REJECT_REASON_PARAMETER_OUT_OF_RANGE;
            break;
        case ERROR_CODE_REJECT_TOO_MANY_ARGUMENTS:
            reject_code = REJECT_REASON_TOO_MANY_ARGUMENTS;
            break;
        case ERROR_CODE_REJECT_UNDEFINED_ENUMERATION:
            reject_code = REJECT_REASON_UNDEFINED_ENUMERATION;
            break;
        case ERROR_CODE_REJECT_UNRECOGNIZED_SERVICE:
            reject_code = REJECT_REASON_UNRECOGNIZED_SERVICE;
            break;
        case ERROR_CODE_INVALID_DATA_ENCODING:
            reject_code = REJECT_REASON_INVALID_DATA_ENCODING;
            break;
        case ERROR_CODE_REJECT_PROPRIETARY:
            reject_code = REJECT_REASON_PROPRIETARY_FIRST;
            break;
        case ERROR_CODE_REJECT_OTHER:
        default:
            reject_code = REJECT_REASON_OTHER;
            break;
    }

    return (reject_code);
}

/**
 * @brief Determine if a BACnetErrorCode is a BACnetRejectReason
 * @param error_code #BACNET_ERROR_CODE enumeration
 * @return true if the BACnet Error Code is a BACnet abort reason
 */
bool reject_valid_error_code(BACNET_ERROR_CODE error_code)
{
    bool status = false;

    switch (error_code) {
        case ERROR_CODE_REJECT_OTHER:
        case ERROR_CODE_REJECT_BUFFER_OVERFLOW:
        case ERROR_CODE_REJECT_INCONSISTENT_PARAMETERS:
        case ERROR_CODE_REJECT_INVALID_PARAMETER_DATA_TYPE:
        case ERROR_CODE_REJECT_INVALID_TAG:
        case ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER:
        case ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE:
        case ERROR_CODE_REJECT_TOO_MANY_ARGUMENTS:
        case ERROR_CODE_REJECT_UNDEFINED_ENUMERATION:
        case ERROR_CODE_REJECT_UNRECOGNIZED_SERVICE:
        case ERROR_CODE_INVALID_DATA_ENCODING:
        case ERROR_CODE_REJECT_PROPRIETARY:
            status = true;
            break;
        default:
            break;
    }

    return status;
}

/**
 * @brief Convert a reject code to BACnet Error code
 * @param reject_code - code to be converted
 * @return error code converted. Anything not defined gets converted
 * to ERROR_CODE_REJECT_OTHER.
 */
BACNET_ERROR_CODE reject_convert_to_error_code(BACNET_REJECT_REASON reject_code)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_REJECT_OTHER;

    switch (reject_code) {
        case REJECT_REASON_OTHER:
            error_code = ERROR_CODE_REJECT_OTHER;
            break;
        case REJECT_REASON_BUFFER_OVERFLOW:
            error_code = ERROR_CODE_REJECT_BUFFER_OVERFLOW;
            break;
        case REJECT_REASON_INCONSISTENT_PARAMETERS:
            error_code = ERROR_CODE_REJECT_INCONSISTENT_PARAMETERS;
            break;
        case REJECT_REASON_INVALID_PARAMETER_DATA_TYPE:
            error_code = ERROR_CODE_REJECT_INVALID_PARAMETER_DATA_TYPE;
            break;
        case REJECT_REASON_INVALID_TAG:
            error_code = ERROR_CODE_REJECT_INVALID_TAG;
            break;
        case REJECT_REASON_MISSING_REQUIRED_PARAMETER:
            error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
            break;
        case REJECT_REASON_PARAMETER_OUT_OF_RANGE:
            error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
            break;
        case REJECT_REASON_TOO_MANY_ARGUMENTS:
            error_code = ERROR_CODE_REJECT_TOO_MANY_ARGUMENTS;
            break;
        case REJECT_REASON_UNDEFINED_ENUMERATION:
            error_code = ERROR_CODE_REJECT_UNDEFINED_ENUMERATION;
            break;
        case REJECT_REASON_UNRECOGNIZED_SERVICE:
            error_code = ERROR_CODE_REJECT_UNRECOGNIZED_SERVICE;
            break;
        case REJECT_REASON_INVALID_DATA_ENCODING:
            error_code = ERROR_CODE_INVALID_DATA_ENCODING;
            break;
        default:
            if (reject_code >= REJECT_REASON_PROPRIETARY_FIRST) {
                error_code = ERROR_CODE_REJECT_PROPRIETARY;
            }
            break;
    }

    return (error_code);
}

/** Encode service
 *
 * @param apdu  Pointer to the APDU buffer.
 * @param invoke_id  Invoke-Id.
 * @param reject_reason  Reason for having rejected.
 *
 * @return Bytes encoded, typically 3.
 */
int reject_encode_apdu(uint8_t *apdu, uint8_t invoke_id, uint8_t reject_reason)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_REJECT;
        apdu[1] = invoke_id;
        apdu[2] = reject_reason;
        apdu_len = 3;
    }

    return apdu_len;
}

#if !BACNET_SVC_SERVER

/** Decode the service request only.
 *
 * @param apdu  Pointer to the APDU buffer.
 * @param apdu_len  Bytes valid in the buffer
 * @param invoke_id  Invoke-Id.
 * @param reject_reason  Pointer to the variable taking
 *        the reason for having rejected.
 *
 * @return Bytes encoded, typically 3.
 */
int reject_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint8_t *reject_reason)
{
    int len = 0;

    if (apdu_len) {
        if (invoke_id) {
            *invoke_id = apdu[0];
        }
        if (reject_reason) {
            if (apdu_len > 1) {
                *reject_reason = apdu[1];
            } else {
                *reject_reason = 0;
            }
        }
    }

    return len;
}
#endif
