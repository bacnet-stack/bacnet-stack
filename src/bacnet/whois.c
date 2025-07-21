/**
 * @file
 * @brief Encode/Decode Who-Is requests
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/whois.h"

/**
 * @brief Encode a Who-Is-Request APDU
 * @ingroup BIBB-DM-DDB
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL for
 *  length determination
 * @param low_limit [in] The low limit of the range of device instance numbers
 * @param high_limit [in] The high limit of the range of device instance numbers
 * @return The length of the apdu encoded (0 is a valid length)
 */
int whois_request_encode(uint8_t *apdu, int32_t low_limit, int32_t high_limit)
{
    int len = 0;
    int apdu_len = 0;

    if ((low_limit >= 0) && (low_limit <= BACNET_MAX_INSTANCE) &&
        (high_limit >= 0) && (high_limit <= BACNET_MAX_INSTANCE)) {
        len = encode_context_unsigned(apdu, 0, low_limit);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_context_unsigned(apdu, 1, high_limit);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode a Who-Is-Request unconfirmed service APDU
 * @ingroup BIBB-DM-DDB
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL for
 *  length determination
 * @param low_limit [in] The low limit of the range of device instance numbers
 * @param high_limit [in] The high limit of the range of device instance numbers
 * @return The length of the apdu encoded (0 is a valid length)
 */
int whois_encode_apdu(uint8_t *apdu, int32_t low_limit, int32_t high_limit)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_WHO_IS; /* service choice */
    }
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = whois_request_encode(apdu, low_limit, high_limit);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a Who-Is-Request APDU
 * @ingroup BIBB-DM-DDB
 * @param apdu [in] Buffer containing the APDU
 * @param apdu_size [in] The length of the APDU
 * @param pLow_limit [out] The low limit of the range of device instance numbers
 * @param pHigh_limit [out] The high limit of the range of device instance
 * numbers
 * @return The number of bytes decoded , or #BACNET_STATUS_ERROR on error
 */
int whois_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_size,
    int32_t *pLow_limit,
    int32_t *pHigh_limit)
{
    int len = 0, apdu_len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* optional limits - used as a pair */
    if (apdu && (apdu_size > 0)) {
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &unsigned_value);
        if (len > 0) {
            if (unsigned_value <= BACNET_MAX_INSTANCE) {
                if (pLow_limit) {
                    *pLow_limit = (int32_t)unsigned_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
        if (len > 0) {
            if (unsigned_value <= BACNET_MAX_INSTANCE) {
                if (pHigh_limit) {
                    *pHigh_limit = (int32_t)unsigned_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else {
        if (pLow_limit) {
            *pLow_limit = -1;
        }
        if (pHigh_limit) {
            *pHigh_limit = -1;
        }
    }

    return apdu_len;
}
