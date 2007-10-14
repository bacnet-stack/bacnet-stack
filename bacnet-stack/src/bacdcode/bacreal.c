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

/* from clause 20.2.6 Encoding of a Real Number Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
int encode_application_real(uint8_t * apdu, float value)
{
    int len = 0;

    /* assumes that the tag only consumes 1 octet */
    len = encode_bacnet_real(value, &apdu[1]);
    len += encode_tag(&apdu[0], BACNET_APPLICATION_TAG_REAL, false, len);

    return len;
}

int encode_context_real(uint8_t * apdu, int tag_number, float value)
{
    int len = 0;

    /* assumes that the tag only consumes 1 octet */
    len = encode_bacnet_real(value, &apdu[1]);
    /* we only reserved 1 byte for encoding the tag - check the limits */
    if (tag_number <= 14)
        len += encode_tag(&apdu[0], (uint8_t) tag_number, true, len);
    else
        len = 0;

    return len;
}
