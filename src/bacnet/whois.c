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

/* encode I-Am service  - use -1 for limit if you want unlimited */
int whois_encode_apdu(uint8_t *apdu, int32_t low_limit, int32_t high_limit)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_WHO_IS; /* service choice */
        apdu_len = 2;
        /* optional limits - must be used as a pair */
        if ((low_limit >= 0) && (low_limit <= BACNET_MAX_INSTANCE) &&
            (high_limit >= 0) && (high_limit <= BACNET_MAX_INSTANCE)) {
            len = encode_context_unsigned(&apdu[apdu_len], 0, low_limit);
            apdu_len += len;
            len = encode_context_unsigned(&apdu[apdu_len], 1, high_limit);
            apdu_len += len;
        }
    }

    return apdu_len;
}

/* decode the service request only */
int whois_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_len,
    int32_t *pLow_limit,
    int32_t *pHigh_limit)
{
    unsigned int len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* optional limits - must be used as a pair */
    if (apdu_len) {
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number != 0) {
            return BACNET_STATUS_ERROR;
        }
        if (apdu_len > (unsigned)len) {
            len += decode_unsigned(&apdu[len], len_value, &unsigned_value);
            if (unsigned_value <= BACNET_MAX_INSTANCE) {
                if (pLow_limit) {
                    *pLow_limit = (int32_t)unsigned_value;
                }
            }
            if (apdu_len > (unsigned)len) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                if (tag_number != 1) {
                    return BACNET_STATUS_ERROR;
                }
                if (apdu_len > (unsigned)len) {
                    len +=
                        decode_unsigned(&apdu[len], len_value, &unsigned_value);
                    if (unsigned_value <= BACNET_MAX_INSTANCE) {
                        if (pHigh_limit) {
                            *pHigh_limit = (int32_t)unsigned_value;
                            ;
                        }
                    }
                } else {
                    return BACNET_STATUS_ERROR;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (pLow_limit) {
            *pLow_limit = -1;
        }
        if (pHigh_limit) {
            *pHigh_limit = -1;
        }
        len = 0;
    }

    return (int)len;
}
