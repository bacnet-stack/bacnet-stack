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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>             /* for strlen */
#include "bacstr.h"
#include "bits.h"

void bitstring_init(BACNET_BIT_STRING * bit_string)
{
    int i;

    bit_string->bits_used = 0;
    for (i = 0; i < MAX_BITSTRING_BYTES; i++) {
        bit_string->value[i] = 0;
    }
}

void bitstring_set_bit(BACNET_BIT_STRING * bit_string, uint8_t bit,
    bool value)
{
    uint8_t byte_number = bit / 8;
    uint8_t bit_mask = 1;

    if (byte_number < MAX_BITSTRING_BYTES) {
        /* set max bits used */
        if (bit_string->bits_used < (bit + 1))
            bit_string->bits_used = bit + 1;
        bit_mask = bit_mask << (bit - (byte_number * 8));
        if (value)
            bit_string->value[byte_number] |= bit_mask;
        else
            bit_string->value[byte_number] &= (~(bit_mask));
    }
}

bool bitstring_bit(BACNET_BIT_STRING * bit_string, uint8_t bit)
{
    bool value = false;
    uint8_t byte_number = bit / 8;
    uint8_t bit_mask = 1;

    if (bit < (MAX_BITSTRING_BYTES * 8)) {
        bit_mask = bit_mask << (bit - (byte_number * 8));
        if (bit_string->value[byte_number] & bit_mask)
            value = true;
    }

    return value;
}

uint8_t bitstring_bits_used(BACNET_BIT_STRING * bit_string)
{
    return bit_string->bits_used;
}

/* returns the number of bytes that a bit string is using */
int bitstring_bytes_used(BACNET_BIT_STRING * bit_string)
{
    int len = 0;                /* return value */
    uint8_t used_bytes = 0;
    uint8_t last_bit = 0;

    if (bit_string->bits_used) {
        last_bit = bit_string->bits_used - 1;
        used_bytes = last_bit / 8;
        /* add one for the first byte */
        used_bytes++;
        len = used_bytes;
    }

    return len;
}

uint8_t bitstring_octet(BACNET_BIT_STRING * bit_string, uint8_t index)
{
    uint8_t octet = 0;

    if (bit_string) {
        if (index < MAX_BITSTRING_BYTES) {
            octet = bit_string->value[index];
        }
    }

    return octet;
}

bool bitstring_set_octet(BACNET_BIT_STRING * bit_string,
    uint8_t index, uint8_t octet)
{
    bool status = false;

    if (bit_string) {
        if (index < MAX_BITSTRING_BYTES) {
            bit_string->value[index] = octet;
            status = true;
        }
    }

    return status;
}

bool bitstring_set_bits_used(BACNET_BIT_STRING * bit_string,
    uint8_t bytes_used, uint8_t unused_bits)
{
    bool status = false;

    if (bit_string) {
        /* FIXME: check that bytes_used is at least one? */
        bit_string->bits_used = bytes_used * 8;
        bit_string->bits_used -= unused_bits;
        status = true;
    }

    return status;
}

uint8_t bitstring_bits_capacity(BACNET_BIT_STRING * bit_string)
{
    if (bit_string)
        return (sizeof(bit_string->value) * 8);
    else
        return 0;
}
