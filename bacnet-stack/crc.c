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
 Boston, MA  02111-1307
 USA.

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
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Accumulate "dataValue" into the CRC in crcValue. */
/* Return value is updated CRC */
/* */
/*  The ^ operator means exclusive OR. */
/* Note: This function is copied directly from the BACnet standard. */
uint8_t CRC_Calc_Header(uint8_t dataValue, uint8_t crcValue)
{
    uint16_t crc;

    crc = crcValue ^ dataValue; /* XOR C7..C0 with D7..D0 */

    /* Exclusive OR the terms in the table (top down) */
    crc = crc ^ (crc << 1) ^ (crc << 2) ^ (crc << 3)
        ^ (crc << 4) ^ (crc << 5) ^ (crc << 6)
        ^ (crc << 7);

    /* Combine bits shifted out left hand end */
    return (crc & 0xfe) ^ ((crc >> 8) & 1);
}

/* Accumulate "dataValue" into the CRC in crcValue. */
/*  Return value is updated CRC */
/* */
/*  The ^ operator means exclusive OR. */
/* Note: This function is copied directly from the BACnet standard. */
uint16_t CRC_Calc_Data(uint8_t dataValue, uint16_t crcValue)
{
    uint16_t crcLow;

    crcLow = (crcValue & 0xff) ^ dataValue;     /* XOR C7..C0 with D7..D0 */

    /* Exclusive OR the terms in the table (top down) */
    return (crcValue >> 8) ^ (crcLow << 8) ^ (crcLow << 3)
        ^ (crcLow << 12) ^ (crcLow >> 4)
        ^ (crcLow & 0x0f) ^ ((crcLow & 0x0f) << 7);
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"
#include "bytes.h"

/* test from Annex G 1.0 of BACnet Standard */
void testCRC8(Test * pTest)
{
    uint8_t crc = 0xff;         /* accumulates the crc value */
    uint8_t frame_crc;          /* appended to the end of the frame */

    crc = CRC_Calc_Header(0x00, crc);
    ct_test(pTest, crc == 0x55);
    crc = CRC_Calc_Header(0x10, crc);
    ct_test(pTest, crc == 0xC2);
    crc = CRC_Calc_Header(0x05, crc);
    ct_test(pTest, crc == 0xBC);
    crc = CRC_Calc_Header(0x00, crc);
    ct_test(pTest, crc == 0x95);
    crc = CRC_Calc_Header(0x00, crc);
    ct_test(pTest, crc == 0x73);
    /* send the ones complement of the CRC in place of */
    /* the CRC, and the resulting CRC will always equal 0x55. */
    frame_crc = ~crc;
    ct_test(pTest, frame_crc == 0x8C);
    /* use the ones complement value and the next to last CRC value */
    crc = CRC_Calc_Header(frame_crc, crc);
    ct_test(pTest, crc == 0x55);
}

/* test from Annex G 2.0 of BACnet Standard */
void testCRC16(Test * pTest)
{
    uint16_t crc = 0xffff;
    uint16_t data_crc;

    crc = CRC_Calc_Data(0x01, crc);
    ct_test(pTest, crc == 0x1E0E);
    crc = CRC_Calc_Data(0x22, crc);
    ct_test(pTest, crc == 0xEB70);
    crc = CRC_Calc_Data(0x30, crc);
    ct_test(pTest, crc == 0x42EF);
    /* send the ones complement of the CRC in place of */
    /* the CRC, and the resulting CRC will always equal 0xF0B8. */
    data_crc = ~crc;
    ct_test(pTest, data_crc == 0xBD10);
    crc = CRC_Calc_Data(LO_BYTE(data_crc), crc);
    ct_test(pTest, crc == 0x0F3A);
    crc = CRC_Calc_Data(HI_BYTE(data_crc), crc);
    ct_test(pTest, crc == 0xF0B8);
}

#endif

#ifdef TEST_CRC
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("crc", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testCRC8);
    assert(rc);
    rc = ct_addTestFunction(pTest, testCRC16);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
#endif
