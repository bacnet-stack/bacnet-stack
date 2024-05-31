/**
 * @file
 * @brief BACnet BACnetAuthenticationFactorFormat codecs
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include "bacnet/bacdcode.h"
#include "bacnet/authentication_factor_format.h"

int bacapp_encode_authentication_factor_format(
    uint8_t *apdu, BACNET_AUTHENTICATION_FACTOR_FORMAT *aff)
{
    int len;
    int apdu_len = 0;

    len = encode_context_enumerated(&apdu[apdu_len], 0, aff->format_type);
    if (len < 0) {
        return -1;
    } else {
        apdu_len += len;
    }

    if (aff->format_type == AUTHENTICATION_FACTOR_CUSTOM) {
        len = encode_context_unsigned(&apdu[apdu_len], 1, aff->vendor_id);
        if (len < 0) {
            return -1;
        } else {
            apdu_len += len;
        }

        len = encode_context_unsigned(&apdu[apdu_len], 2, aff->vendor_format);
        if (len < 0) {
            return -1;
        } else {
            apdu_len += len;
        }
    }
    return apdu_len;
}

int bacapp_encode_context_authentication_factor_format(
    uint8_t *apdu, uint8_t tag, BACNET_AUTHENTICATION_FACTOR_FORMAT *aff)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(&apdu[apdu_len], tag);
    apdu_len += len;

    len = bacapp_encode_authentication_factor_format(&apdu[apdu_len], aff);
    apdu_len += len;

    len = encode_closing_tag(&apdu[apdu_len], tag);
    apdu_len += len;

    return apdu_len;
}

int bacapp_decode_authentication_factor_format(
    uint8_t *apdu, BACNET_AUTHENTICATION_FACTOR_FORMAT *aff)
{
    int len;
    int apdu_len = 0;
    uint32_t format_type = aff->format_type;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    if (decode_is_context_tag(&apdu[apdu_len], 0)) {
        len = decode_context_enumerated(&apdu[apdu_len], 0, &format_type);
        if (len < 0) {
            return -1;
        } else if (format_type < AUTHENTICATION_FACTOR_MAX) {
            apdu_len += len;
            aff->format_type = (BACNET_AUTHENTICATION_FACTOR_TYPE)format_type;
        } else {
            /* FIXME: Maybe this should return BACNET_STATUS_REJECT */
            return -1;
        }
    } else {
        return -1;
    }

    if (decode_is_context_tag(&apdu[apdu_len], 1)) {
        len = decode_context_unsigned(&apdu[apdu_len], 1, &unsigned_value);
        if (len < 0) {
            return -1;
        } else {
            aff->vendor_id = unsigned_value;
            apdu_len += len;
        }
        if ((aff->format_type != AUTHENTICATION_FACTOR_CUSTOM) &&
            (aff->vendor_id != 0)) {
            return -1;
        }
    }

    if (decode_is_context_tag(&apdu[apdu_len], 2)) {
        len = decode_context_unsigned(&apdu[apdu_len], 2, &unsigned_value);
        if (len < 0) {
            return -1;
        } else {
            aff->vendor_format = unsigned_value;
            apdu_len += len;
        }
        if ((aff->format_type != AUTHENTICATION_FACTOR_CUSTOM) &&
            (aff->vendor_format != 0)) {
            return -1;
        }
    }

    return apdu_len;
}

int bacapp_decode_context_authentication_factor_format(
    uint8_t *apdu, uint8_t tag, BACNET_AUTHENTICATION_FACTOR_FORMAT *aff)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag)) {
        len++;
        section_length =
            bacapp_decode_authentication_factor_format(&apdu[len], aff);

        if (section_length == -1) {
            len = -1;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag)) {
                len++;
            } else {
                len = -1;
            }
        }
    } else {
        len = -1;
    }
    return len;
}
