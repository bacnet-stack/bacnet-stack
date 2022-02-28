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
#include "bacnet/reject.h"

/** @file reject.c  Encode/Decode Reject APDUs */

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
 * @brief Convert a reject code to BACnet Error code
 * @param reject_code - code to be converted
 * @return error code converted. Anything not defined gets converted
 * to ERROR_CODE_REJECT_OTHER.
 */
BACNET_ERROR_CODE reject_convert_to_error_code(BACNET_REJECT_REASON reject_code)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_REJECT_OTHER;

    switch (reject_code) {
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
        case REJECT_REASON_OTHER:
            error_code = ERROR_CODE_REJECT_OTHER;
            break;
        default:
            if ((reject_code >= REJECT_REASON_PROPRIETARY_FIRST) &&
                (reject_code <= REJECT_REASON_PROPRIETARY_LAST)) {
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
int reject_decode_service_request(uint8_t *apdu,
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
