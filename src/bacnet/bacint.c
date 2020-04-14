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
#include "bacnet/config.h"
#include "bacnet/bacint.h"

/** @file bacint.c  Encode/Decode Integer Types */

int encode_unsigned16(uint8_t *apdu, uint16_t value)
{
    apdu[0] = (uint8_t)((value & 0xff00) >> 8);
    apdu[1] = (uint8_t)(value & 0x00ff);

    return 2;
}

int decode_unsigned16(uint8_t *apdu, uint16_t *value)
{
    if (value) {
        *value = (uint16_t)((((uint16_t)apdu[0]) << 8) & 0xff00);
        *value |= ((uint16_t)(((uint16_t)apdu[1]) & 0x00ff));
    }

    return 2;
}

int encode_unsigned24(uint8_t *apdu, uint32_t value)
{
    apdu[0] = (uint8_t)((value & 0xff0000) >> 16);
    apdu[1] = (uint8_t)((value & 0x00ff00) >> 8);
    apdu[2] = (uint8_t)(value & 0x0000ff);

    return 3;
}

int decode_unsigned24(uint8_t *apdu, uint32_t *value)
{
    if (value) {
        *value = ((uint32_t)((((uint32_t)apdu[0]) << 16) & 0x00ff0000));
        *value |= (uint32_t)((((uint32_t)apdu[1]) << 8) & 0x0000ff00);
        *value |= ((uint32_t)(((uint32_t)apdu[2]) & 0x000000ff));
    }

    return 3;
}

int encode_unsigned32(uint8_t *apdu, uint32_t value)
{
    apdu[0] = (uint8_t)((value & 0xff000000) >> 24);
    apdu[1] = (uint8_t)((value & 0x00ff0000) >> 16);
    apdu[2] = (uint8_t)((value & 0x0000ff00) >> 8);
    apdu[3] = (uint8_t)(value & 0x000000ff);

    return 4;
}

int decode_unsigned32(uint8_t *apdu, uint32_t *value)
{
    if (value) {
        *value = ((uint32_t)((((uint32_t)apdu[0]) << 24) & 0xff000000));
        *value |= ((uint32_t)((((uint32_t)apdu[1]) << 16) & 0x00ff0000));
        *value |= ((uint32_t)((((uint32_t)apdu[2]) << 8) & 0x0000ff00));
        *value |= ((uint32_t)(((uint32_t)apdu[3]) & 0x000000ff));
    }

    return 4;
}

#ifdef UINT64_MAX
/**
 * @brief       Encode a 40-bit unsigned value into buffer
 * @param       buffer - pointer to bytes for storing encoding
 * @param       value - 40-bit value to encode
 * @return      Returns the number of bytes encoded
 */
int encode_unsigned40(uint8_t *buffer, uint64_t value)
{
    buffer[0] = (uint8_t)((value & 0x000000ff00000000) >> 32);
    buffer[1] = (uint8_t)((value & 0x00000000ff000000) >> 24);
    buffer[2] = (uint8_t)((value & 0x0000000000ff0000) >> 16);
    buffer[3] = (uint8_t)((value & 0x000000000000ff00) >> 8);
    buffer[4] = (uint8_t)(value & 0x00000000000000ff);

    return 5;
}

/**
 * @brief       Decode a 40-bit unsigned value from a buffer
 * @param       buffer - pointer to bytes for used for decoding
 * @param       value - pointer to 64-bit value to store the decoded value
 * @return      Returns the number of bytes decoded
 */
int decode_unsigned40(uint8_t *buffer, uint64_t *value)
{
    if (value) {
        *value =
            ((uint64_t)((((uint64_t)buffer[0]) << 32) & 0x000000ff00000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[1]) << 24) & 0x00000000ff000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[2]) << 16) & 0x0000000000ff0000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[3]) << 8) & 0x000000000000ff00));
        *value |= ((uint64_t)(((uint64_t)buffer[4]) & 0x00000000000000ff));
    }

    return 5;
}

/**
 * @brief       Encode a 48-bit unsigned value into buffer
 * @param       buffer - pointer to bytes for storing encoding
 * @param       value - 48-bit value to encode
 * @return      Returns the number of bytes encoded
 */
int encode_unsigned48(uint8_t *buffer, uint64_t value)
{
    buffer[0] = (uint8_t)((value & 0x0000ff0000000000) >> 40);
    buffer[1] = (uint8_t)((value & 0x000000ff00000000) >> 32);
    buffer[2] = (uint8_t)((value & 0x00000000ff000000) >> 24);
    buffer[3] = (uint8_t)((value & 0x0000000000ff0000) >> 16);
    buffer[4] = (uint8_t)((value & 0x000000000000ff00) >> 8);
    buffer[5] = (uint8_t)(value & 0x00000000000000ff);

    return 6;
}

/**
 * @brief       Decode a 48-bit unsigned value from a buffer
 * @param       buffer - pointer to bytes for used for decoding
 * @param       value - pointer to 64-bit value to store the decoded value
 * @return      Returns the number of bytes decoded
 */
int decode_unsigned48(uint8_t *buffer, uint64_t *value)
{
    if (value) {
        *value =
            ((uint64_t)((((uint64_t)buffer[0]) << 40) & 0x0000ff0000000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[1]) << 32) & 0x000000ff00000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[2]) << 24) & 0x00000000ff000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[3]) << 16) & 0x0000000000ff0000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[4]) << 8) & 0x000000000000ff00));
        *value |= ((uint64_t)(((uint64_t)buffer[5]) & 0x00000000000000ff));
    }

    return 6;
}

/**
 * @brief       Encode a 56-bit unsigned value into buffer
 * @param       buffer - pointer to bytes for storing encoding
 * @param       value - 56-bit value to encode
 * @return      Returns the number of bytes encoded
 */
int encode_unsigned56(uint8_t *buffer, uint64_t value)
{
    buffer[0] = (uint8_t)((value & 0x00ff000000000000) >> 48);
    buffer[1] = (uint8_t)((value & 0x0000ff0000000000) >> 40);
    buffer[2] = (uint8_t)((value & 0x000000ff00000000) >> 32);
    buffer[3] = (uint8_t)((value & 0x00000000ff000000) >> 24);
    buffer[4] = (uint8_t)((value & 0x0000000000ff0000) >> 16);
    buffer[5] = (uint8_t)((value & 0x000000000000ff00) >> 8);
    buffer[6] = (uint8_t)(value & 0x00000000000000ff);

    return 7;
}

/**
 * @brief       Decode a 56-bit unsigned value from a buffer
 * @param       buffer - pointer to bytes for used for decoding
 * @param       value - pointer to 64-bit value to store the decoded value
 * @return      Returns the number of bytes decoded
 */
int decode_unsigned56(uint8_t *buffer, uint64_t *value)
{
    if (value) {
        *value =
            ((uint64_t)((((uint64_t)buffer[0]) << 48) & 0x00ff000000000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[1]) << 40) & 0x0000ff0000000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[2]) << 32) & 0x000000ff00000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[3]) << 24) & 0x00000000ff000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[4]) << 16) & 0x0000000000ff0000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[5]) << 8) & 0x000000000000ff00));
        *value |= ((uint64_t)(((uint64_t)buffer[6]) & 0x00000000000000ff));
    }

    return 7;
}

/**
 * @brief       Encode a 64-bit unsigned value into buffer
 * @param       buffer - pointer to bytes for storing encoding
 * @param       value - 16-bit value to encode
 * @return      Returns the number of bytes encoded
 */
int encode_unsigned64(uint8_t *buffer, uint64_t value)
{
    buffer[0] = (uint8_t)((value & 0xff00000000000000) >> 56);
    buffer[1] = (uint8_t)((value & 0x00ff000000000000) >> 48);
    buffer[2] = (uint8_t)((value & 0x0000ff0000000000) >> 40);
    buffer[3] = (uint8_t)((value & 0x000000ff00000000) >> 32);
    buffer[4] = (uint8_t)((value & 0x00000000ff000000) >> 24);
    buffer[5] = (uint8_t)((value & 0x0000000000ff0000) >> 16);
    buffer[6] = (uint8_t)((value & 0x000000000000ff00) >> 8);
    buffer[7] = (uint8_t)(value & 0x00000000000000ff);

    return 8;
}

/**
 * @brief       Decode a 64-bit unsigned value from a buffer
 * @param       buffer - pointer to bytes for used for decoding
 * @param       value - pointer to 64-bit value to store the decoded value
 * @return      Returns the number of bytes decoded
 */
int decode_unsigned64(uint8_t *buffer, uint64_t *value)
{
    if (value) {
        *value =
            ((uint64_t)((((uint64_t)buffer[0]) << 56) & 0xff00000000000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[1]) << 48) & 0x00ff000000000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[2]) << 40) & 0x0000ff0000000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[3]) << 32) & 0x000000ff00000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[4]) << 24) & 0x00000000ff000000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[5]) << 16) & 0x0000000000ff0000));
        *value |=
            ((uint64_t)((((uint64_t)buffer[6]) << 8) & 0x000000000000ff00));
        *value |= ((uint64_t)(((uint64_t)buffer[7]) & 0x00000000000000ff));
    }

    return 8;
}
#endif
/**
 * @brief       Determine the number of bytes in the value
 *              length of unsigned is variable, as per 20.2.4
 * @param       value - pointer to 32-bit or 64-bit value
 * @return      Returns the number of bytes
 */
int bacnet_unsigned_length(BACNET_UNSIGNED_INTEGER value)
{
    int len = 0; /* return value */

    if (value <= 0xFF) {
        len = 1;
    } else if (value <= 0xFFFF) {
        len = 2;
    } else if (value <= 0xFFFFFF) {
        len = 3;
    } else {
#ifdef UINT64_MAX
        if (value <= 0xFFFFFFFF) {
            len = 4;
        } else if (value <= 0xFFFFFFFFFF) {
            len = 5;
        } else if (value <= 0xFFFFFFFFFFFF) {
            len = 6;
        } else if (value <= 0xFFFFFFFFFFFFFF) {
            len = 7;
        } else {
            len = 8;
        }
#else
        len = 4;
#endif
    }

    return len;
}

#if BACNET_USE_SIGNED
int encode_signed8(uint8_t *apdu, int8_t value)
{
    apdu[0] = (uint8_t)value;

    return 1;
}

int decode_signed8(uint8_t *apdu, int32_t *value)
{
    if (value) {
        /* negative - bit 7 is set */
        if (apdu[0] & 0x80) {
            *value = 0xFFFFFF00;
        } else {
            *value = 0;
        }
        *value |= ((int32_t)(((int32_t)apdu[0]) & 0x000000ff));
    }

    return 1;
}

int encode_signed16(uint8_t *apdu, int16_t value)
{
    apdu[0] = (uint8_t)((value & 0xff00) >> 8);
    apdu[1] = (uint8_t)(value & 0x00ff);

    return 2;
}

int decode_signed16(uint8_t *apdu, int32_t *value)
{
    if (value) {
        /* negative - bit 7 is set */
        if (apdu[0] & 0x80) {
            *value = 0xFFFF0000;
        } else {
            *value = 0;
        }
        *value |= ((int32_t)((((int32_t)apdu[0]) << 8) & 0x0000ff00));
        *value |= ((int32_t)(((int32_t)apdu[1]) & 0x000000ff));
    }

    return 2;
}

int encode_signed24(uint8_t *apdu, int32_t value)
{
    apdu[0] = (uint8_t)((value & 0xff0000) >> 16);
    apdu[1] = (uint8_t)((value & 0x00ff00) >> 8);
    apdu[2] = (uint8_t)(value & 0x0000ff);

    return 3;
}

int decode_signed24(uint8_t *apdu, int32_t *value)
{
    if (value) {
        /* negative - bit 7 is set */
        if (apdu[0] & 0x80) {
            *value = 0xFF000000;
        } else {
            *value = 0;
        }
        *value |= ((int32_t)((((int32_t)apdu[0]) << 16) & 0x00ff0000));
        *value |= ((int32_t)((((int32_t)apdu[1]) << 8) & 0x0000ff00));
        *value |= ((int32_t)(((int32_t)apdu[2]) & 0x000000ff));
    }

    return 3;
}

int encode_signed32(uint8_t *apdu, int32_t value)
{
    apdu[0] = (uint8_t)((value & 0xff000000) >> 24);
    apdu[1] = (uint8_t)((value & 0x00ff0000) >> 16);
    apdu[2] = (uint8_t)((value & 0x0000ff00) >> 8);
    apdu[3] = (uint8_t)(value & 0x000000ff);

    return 4;
}

int decode_signed32(uint8_t *apdu, int32_t *value)
{
    if (value) {
        *value = ((int32_t)((((int32_t)apdu[0]) << 24) & 0xff000000));
        *value |= ((int32_t)((((int32_t)apdu[1]) << 16) & 0x00ff0000));
        *value |= ((int32_t)((((int32_t)apdu[2]) << 8) & 0x0000ff00));
        *value |= ((int32_t)(((int32_t)apdu[3]) & 0x000000ff));
    }

    return 4;
}
#endif
/* end of decoding_encoding.c */
#ifdef TEST
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "ctest.h"

static void testBACnetUnsigned16(Test *pTest)
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

static void testBACnetUnsigned24(Test *pTest)
{
    uint8_t apdu[32] = { 0 };
    uint32_t value = 0, test_value = 0;
    int len = 0;

    for (value = 0; value == 0xffffff; value += 0xf) {
        len = encode_unsigned24(&apdu[0], value);
        ct_test(pTest, len == 3);
        len = decode_unsigned24(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
    }
}

static void testBACnetUnsigned32(Test *pTest)
{
    uint8_t apdu[32] = { 0 };
    uint32_t value = 0, test_value = 0;
    int len = 0;

    for (value = 0; value == 0xffffffff; value = (value << 8) | 0xff) {
        len = encode_unsigned32(&apdu[0], value);
        ct_test(pTest, len == 4);
        len = decode_unsigned32(&apdu[0], &test_value);
        ct_test(pTest, value == test_value);
    }
}

static void testBACnetUnsigned40(Test *pTest)
{
#ifdef UINT64_MAX
    uint8_t apdu[64] = { 0 };
    uint64_t value = 0, test_value = 0;
    int len = 0;

    for (value = 0; value == 0xffffffffff; value = (value << 8) | 0xff) {
        len = encode_unsigned40(&apdu[0], value);
        ct_test(pTest, len == 5);
        len = decode_unsigned40(&apdu[0], &test_value);
        ct_test(pTest, len == 5);
        ct_test(pTest, value == test_value);
    }
#else
#warning "UINT64_MAX not supported!"
#endif
}

static void testBACnetUnsigned48(Test *pTest)
{
#ifdef UINT64_MAX
    uint8_t apdu[64] = { 0 };
    uint64_t value = 0, test_value = 0;
    int len = 0;

    for (value = 0; value == 0xffffffffffff; value = (value << 8) | 0xff) {
        len = encode_unsigned48(&apdu[0], value);
        ct_test(pTest, len == 6);
        len = decode_unsigned48(&apdu[0], &test_value);
        ct_test(pTest, len == 6);
        ct_test(pTest, value == test_value);
    }
#else
#warning "UINT64_MAX not supported!"
#endif
}

static void testBACnetUnsigned56(Test *pTest)
{
#ifdef UINT64_MAX
    uint8_t apdu[64] = { 0 };
    uint64_t value = 0, test_value = 0;
    int len = 0;

    for (value = 0; value == 0xffffffffffffff; value = (value << 8) | 0xff) {
        len = encode_unsigned56(&apdu[0], value);
        ct_test(pTest, len == 7);
        len = decode_unsigned56(&apdu[0], &test_value);
        ct_test(pTest, len == 7);
        ct_test(pTest, value == test_value);
    }
#else
#warning "UINT64_MAX not supported!"
#endif
}

static void testBACnetUnsigned64(Test *pTest)
{
#ifdef UINT64_MAX
    uint8_t apdu[64] = { 0 };
    uint64_t value = 0, test_value = 0;
    int len = 0;

    for (value = 0; value == 0xffffffffffffffff; value = (value << 8) | 0xff) {
        len = encode_unsigned64(&apdu[0], value);
        ct_test(pTest, len == 8);
        len = decode_unsigned64(&apdu[0], &test_value);
        ct_test(pTest, len == 8);
        ct_test(pTest, value == test_value);
    }
#else
#warning "UINT64_MAX not supported!"
#endif
}

static void testBACnetSigned8(Test *pTest)
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

static void testBACnetSigned16(Test *pTest)
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

static void testBACnetSigned24(Test *pTest)
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

static void testBACnetSigned32(Test *pTest)
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

void testBACnetIntegers(Test *pTest)
{
    bool rc;

    assert(pTest);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACnetUnsigned16);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetUnsigned24);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetUnsigned32);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetUnsigned40);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetUnsigned48);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetUnsigned56);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetUnsigned64);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetSigned8);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetSigned16);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetSigned24);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetSigned32);
    assert(rc);
}

#ifdef TEST_BACINT
int main(void)
{
    Test *pTest;

    pTest = ct_create("BACint", NULL);
    testBACnetIntegers(pTest);
    /* configure output */
    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_BACINT */
#endif /* TEST */
