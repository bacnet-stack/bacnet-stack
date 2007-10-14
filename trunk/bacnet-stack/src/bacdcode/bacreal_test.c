/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

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

#include <string.h>

#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bits.h"
#include "bacstr.h"
#include "bacint.h"
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "ctest.h"

void testBACDCodeReal(Test * pTest)
{
    uint8_t real_array[4] = { 0 };
    uint8_t encoded_array[4] = { 0 };
    float value = 42.123;
    float decoded_value = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, apdu_len = 0;
    uint8_t tag_number = 0;
    uint32_t long_value = 0;

    encode_bacnet_real(value, &real_array[0]);
    decode_real(&real_array[0], &decoded_value);
    ct_test(pTest, decoded_value == value);
    encode_bacnet_real(value, &encoded_array[0]);
    ct_test(pTest, memcmp(&real_array, &encoded_array,
            sizeof(real_array)) == 0);

    /* a real will take up 4 octects plus a one octet tag */
    apdu_len = encode_application_real(&apdu[0], value);
    ct_test(pTest, apdu_len == 5);
    /* len tells us how many octets were used for encoding the value */
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &long_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_REAL);
    ct_test(pTest, decode_is_context_specific(&apdu[0]) == false);
    ct_test(pTest, len == 1);
    ct_test(pTest, long_value == 4);
    decode_real(&apdu[len], &decoded_value);
    ct_test(pTest, decoded_value == value);

    return;
}
