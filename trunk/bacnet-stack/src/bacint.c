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

/* BACnet Integer encoding and decoding */

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "bacint.h"

/** @file bacint.c  Encode/Decode Integer Types */

#ifndef BIG_ENDIAN
#error Define BIG_ENDIAN=0 or BIG_ENDIAN=1 for BACnet Stack in compiler settings
#endif

int encode_unsigned16(
    uint8_t * apdu,
    uint16_t value)
{
    uint8_t *in = (uint8_t *)&value;

	#if BIG_ENDIAN
		apdu[0] = in[0];
		apdu[1] = in[1];
	#else
		apdu[0] = in[1];
		apdu[1] = in[0];
	#endif

    return 2;
}

int decode_unsigned16(
    uint8_t * apdu,
    uint16_t * value)
{
    uint8_t *out = (uint8_t *)value;

    if (out) {
	#if BIG_ENDIAN
		out[0] = apdu[0];
		out[1] = apdu[1];
	#else
		out[0] = apdu[1];
		out[1] = apdu[0];
	#endif
    }
    return 2;
}

int encode_unsigned24(
    uint8_t * apdu,
    uint32_t value)
{
    uint8_t *in = (uint8_t *)&value;

	#if BIG_ENDIAN
		apdu[0] = in[1];
		apdu[1] = in[2];
		apdu[2] = in[3];
	#else
		apdu[0] = in[2];
		apdu[1] = in[1];
		apdu[2] = in[0];
	#endif

    return 3;
}

int decode_unsigned24(
    uint8_t * apdu,
    uint32_t * value)
{
    uint8_t *out = (uint8_t *)value;

    if (out) {
	#if BIG_ENDIAN
		out[0] = 0;
		out[1] = apdu[0];
		out[2] = apdu[1];
		out[3] = apdu[2];
	#else
		out[0] = apdu[2];
		out[1] = apdu[1];
		out[2] = apdu[0];
		out[3] = 0;
	#endif
    }

    return 3;
}

int encode_unsigned32(
    uint8_t * apdu,
    uint32_t value)
{
    uint8_t *in = (uint8_t *)&value;

	#if BIG_ENDIAN
		apdu[0] = in[0];
		apdu[1] = in[1];
		apdu[2] = in[2];
		apdu[3] = in[3];
	#else
		apdu[0] = in[3];
		apdu[1] = in[2];
		apdu[2] = in[1];
		apdu[3] = in[0];
	#endif

    return 4;
}

int decode_unsigned32(
    uint8_t * apdu,
    uint32_t * value)
{
    uint8_t *out = (uint8_t *)value;

    if (out) {
	#if BIG_ENDIAN
		out[0] = apdu[0];
		out[1] = apdu[1];
		out[2] = apdu[2];
		out[3] = apdu[3];
	#else
		out[0] = apdu[3];
		out[1] = apdu[2];
		out[2] = apdu[1];
		out[3] = apdu[0];
	#endif
    }
    return 4;
}

int encode_signed8(
    uint8_t * apdu,
    int8_t value)
{
    apdu[0] = (uint8_t) value;

    return 1;
}

int decode_signed8(
    uint8_t * apdu,
    int32_t * value)
{
    if (value) {
        /* negative - bit 7 is set */
        if (apdu[0] & 0x80)
            *value = 0xFFFFFF00;
        else
            *value = 0;
        *value |= ((int32_t) (((int32_t) apdu[0]) & 0x000000ff));
    }

    return 1;
}

int encode_signed16(
    uint8_t * apdu,
    int16_t value)
{
    return encode_unsigned16 (apdu, value);
}

int decode_signed16(
    uint8_t * apdu,
    int32_t * value)
{
    int16_t val;

    decode_unsigned16 (apdu, (uint16_t *)&val);
    *value = (int32_t)val;

    return 2;
}

int encode_signed24(
    uint8_t * apdu,
    int32_t value)
{
    return encode_unsigned24 (apdu, value);
}

int decode_signed24(
    uint8_t * apdu,
    int32_t * value)
{
    if (value) {
        decode_unsigned24 (apdu, (uint32_t *)value);
        if (*value & 0x00800000) {
            *value |= 0xFF000000;
        }
    }

    return 3;
}

int encode_signed32(
    uint8_t * apdu,
    int32_t value)
{
    return encode_unsigned32 (apdu, value);
}

int decode_signed32(
    uint8_t * apdu,
    int32_t * value)
{
    return decode_unsigned32 (apdu, (uint32_t *)value);
}

/* end of decoding_encoding.c */
#ifdef TEST
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "ctest.h"

void testBACnetUnsigned16(
    Test * pTest)
{
    uint8_t apdu[32] = { 0 };
    uint16_t value = 0, test_value = 0;
    int len = 0;

    for (value = 0;; value++) {
        len = encode_unsigned16(&apdu[0], value);
        ct_test(pTest, len == 2);
        len = decode_unsigned16(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
        if (value == 0xFFFF)
            break;
    }
}

void testBACnetUnsigned24(
    Test * pTest)
{
    uint8_t apdu[32] = { 0 };
    uint32_t value = 0, test_value = 0;
    int len = 0;

    for (value = 0;; value += 0xf) {
        len = encode_unsigned24(&apdu[0], value);
        ct_test(pTest, len == 3);
        len = decode_unsigned24(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
        if (value == 0xffffff)
            break;
    }
}

void testBACnetUnsigned32(
    Test * pTest)
{
    uint8_t apdu[32] = { 0 };
    uint32_t value = 0, test_value = 0;
    int len = 0;

    for (value = 0;; value += 0xff) {
        len = encode_unsigned32(&apdu[0], value);
        ct_test(pTest, len == 4);
        len = decode_unsigned32(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
        if (value == 0xffffffff)
            break;
    }
}

void testBACnetSigned8(
    Test * pTest)
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0;

    for (value = -127;; value++) {
        len = encode_signed8(&apdu[0], value);
        ct_test(pTest, len == 1);
        len = decode_signed8(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
        if (value == 127)
            break;
    }
}

void testBACnetSigned16(
    Test * pTest)
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0;

    for (value = -32767;; value++) {
        len = encode_signed16(&apdu[0], value);
        ct_test(pTest, len == 2);
        len = decode_signed16(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
        if (value == 32767)
            break;
    }
}

void testBACnetSigned24(
    Test * pTest)
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0;

    for (value = -8388607; value <= 8388607; value += 15) {
        len = encode_signed24(&apdu[0], value);
        ct_test(pTest, len == 3);
        len = decode_signed24(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
    }
}

void testBACnetSigned32(
    Test * pTest)
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0;

    for (value = -2147483647; value < 0; value += 127) {
        len = encode_signed32(&apdu[0], value);
        ct_test(pTest, len == 4);
        len = decode_signed32(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
    }
    for (value = 2147483647; value > 0; value -= 127) {
        len = encode_signed32(&apdu[0], value);
        ct_test(pTest, len == 4);
        len = decode_signed32(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
    }
}

#ifdef TEST_BACINT
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACint", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACnetUnsigned16);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetUnsigned24);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetUnsigned32);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetSigned8);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetSigned16);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetSigned24);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetSigned32);
    assert(rc);
    /* configure output */
    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_BACINT */
#endif /* TEST */
