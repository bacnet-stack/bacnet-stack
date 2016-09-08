/**************************************************************************
*
* Copyright (C) 2015 Nikola Jelic <nikola.jelic@euroicc.com>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

#include "bacdcode.h"
#include "authentication_factor_format.h"


int bacapp_encode_authentication_factor_format(
    uint8_t * apdu,
    BACNET_AUTHENTICATION_FACTOR_FORMAT * aff)
{
    int len;
    int apdu_len = 0;

    len = encode_context_enumerated(&apdu[apdu_len], 0, aff->format_type);
    if (len < 0)
        return -1;
    else
        apdu_len += len;

    if (aff->format_type == AUTHENTICATION_FACTOR_CUSTOM) {
        len = encode_context_unsigned(&apdu[apdu_len], 1, aff->vendor_id);
        if (len < 0)
            return -1;
        else
            apdu_len += len;

        len = encode_context_unsigned(&apdu[apdu_len], 2, aff->vendor_format);
        if (len < 0)
            return -1;
        else
            apdu_len += len;
    }
    return apdu_len;
}

int bacapp_encode_context_authentication_factor_format(
    uint8_t * apdu,
    uint8_t tag,
    BACNET_AUTHENTICATION_FACTOR_FORMAT * aff)
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
    uint8_t * apdu,
    BACNET_AUTHENTICATION_FACTOR_FORMAT * aff)
{
    int len;
    int apdu_len = 0;

    if (decode_is_context_tag(&apdu[apdu_len], 0)) {
        len =
            decode_context_enumerated(&apdu[apdu_len], 0,
            &aff->format_type);
        if (len < 0)
            return -1;
        else
            apdu_len += len;
    } else
        return -1;

    if (decode_is_context_tag(&apdu[apdu_len], 1)) {
        len = decode_context_unsigned(&apdu[apdu_len], 1, &aff->vendor_id);
        if (len < 0)
            return -1;
        else
            apdu_len += len;
        if ((aff->format_type != AUTHENTICATION_FACTOR_CUSTOM)
            && (aff->vendor_id != 0))
            return -1;
    }

    if (decode_is_context_tag(&apdu[apdu_len], 2)) {
        len = decode_context_unsigned(&apdu[apdu_len], 2, &aff->vendor_format);
        if (len < 0)
            return -1;
        else
            apdu_len += len;
        if ((aff->format_type != AUTHENTICATION_FACTOR_CUSTOM)
            && (aff->vendor_format != 0))
            return -1;
    }

    return apdu_len;
}

int bacapp_decode_context_authentication_factor_format(
    uint8_t * apdu,
    uint8_t tag,
    BACNET_AUTHENTICATION_FACTOR_FORMAT * aff)
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
