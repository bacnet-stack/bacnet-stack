/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2015 Nikola Jelic

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

#include <stdbool.h>
#include <stdint.h>
#include "bacdcode.h"
#include "bactimevalue.h"

int bacapp_encode_time_value(uint8_t * apdu,
    BACNET_TIME_VALUE * value)
{
    int len;
    int apdu_len = 0;

    len = encode_application_time(&apdu[apdu_len], &value->Time);
    apdu_len += len;

    len = bacapp_encode_application_data(&apdu[apdu_len], &value->Value);
    apdu_len += len;

    return apdu_len;
}

int bacapp_encode_context_time_value(uint8_t * apdu,
    uint8_t tag_number,
    BACNET_TIME_VALUE * value)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(&apdu[apdu_len], tag_number);
    apdu_len += len;

    len = bacapp_encode_time_value(&apdu[apdu_len], value);
    apdu_len += len;

    len = encode_closing_tag(&apdu[apdu_len], tag_number);
    apdu_len += len;

    return apdu_len;
}

int bacapp_decode_time_value(uint8_t * apdu,
    BACNET_TIME_VALUE * value)
{
    int len;
    int apdu_len = 0;

    len = decode_application_time(&apdu[apdu_len], &value->Time);
    if (len <= 0)
        return -1;
    apdu_len += len;

    len = bacapp_decode_application_data(&apdu[apdu_len], 2048, &value->Value);
    if (len <= 0)
        return -1;
    apdu_len += len;

    return apdu_len;
}

int bacapp_decode_context_time_value(uint8_t * apdu,
    uint8_t tag_number,
    BACNET_TIME_VALUE * value)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag_number))
        len++;
    else
        return -1;

    section_length = bacapp_decode_time_value(&apdu[len], value);
    if (section_length > 0)
        len += section_length;
    else
        return -1;

    if (decode_is_closing_tag_number(&apdu[len], tag_number))
        len++;
    else
        return -1;

    return len;
}
