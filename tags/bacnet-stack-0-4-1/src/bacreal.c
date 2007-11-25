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

/* NOTE: byte order plays a role in decoding multibyte values */
/* http://www.unixpapa.com/incnote/byteorder.html */
#ifndef BIG_ENDIAN
  #error Define BIG_ENDIAN=0 or BIG_ENDIAN=1 for BACnet Stack in compiler settings
#endif

/* from clause 20.2.6 Encoding of a Real Number Value */
/* returns the number of apdu bytes consumed */
int decode_real(uint8_t * apdu, float *real_value)
{
    union {
        uint8_t byte[4];
        float real_value;
    } my_data;

    /* NOTE: assumes the compiler stores float as IEEE-754 float */
#if BIG_ENDIAN
    my_data.byte[0] = apdu[0];
    my_data.byte[1] = apdu[1];
    my_data.byte[2] = apdu[2];
    my_data.byte[3] = apdu[3];
#else
    my_data.byte[0] = apdu[3];
    my_data.byte[1] = apdu[2];
    my_data.byte[2] = apdu[1];
    my_data.byte[3] = apdu[0];
#endif

    *real_value = my_data.real_value;

    return 4;
}

/* from clause 20.2.6 Encoding of a Real Number Value */
/* returns the number of apdu bytes consumed */
int encode_bacnet_real(float value, uint8_t * apdu)
{
    union {
        uint8_t byte[4];
        float real_value;
    } my_data;

    /* NOTE: assumes the compiler stores float as IEEE-754 float */
    my_data.real_value = value;
#if BIG_ENDIAN
    apdu[0] = my_data.byte[0];
    apdu[1] = my_data.byte[1];
    apdu[2] = my_data.byte[2];
    apdu[3] = my_data.byte[3];
#else
    apdu[0] = my_data.byte[3];
    apdu[1] = my_data.byte[2];
    apdu[2] = my_data.byte[1];
    apdu[3] = my_data.byte[0];
#endif

    return 4;
}

/* end of decoding_encoding.c */
#ifdef TEST
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "ctest.h"

void testBACreal(Test * pTest)
{
    float real_value = 3.14159F, test_real_value = 0.0;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    
    len = encode_bacnet_real(real_value, &apdu[0]);
    ct_test(pTest, len == 4);
    test_len = decode_real(&apdu[0], &test_real_value);
    ct_test(pTest, test_len == len);
}

#ifdef TEST_BACNET_REAL
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACreal", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACreal);
    assert(rc);

    /* configure output */
    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_BACNET_REAL */
#endif                          /* TEST */

