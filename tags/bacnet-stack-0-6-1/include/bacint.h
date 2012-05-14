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
#ifndef BACINT_H
#define BACINT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* unsigned value encoding and decoding */
    int encode_unsigned16(
        uint8_t * apdu,
        uint16_t value);
    int decode_unsigned16(
        uint8_t * apdu,
        uint16_t * value);
    int encode_unsigned24(
        uint8_t * apdu,
        uint32_t value);
    int decode_unsigned24(
        uint8_t * apdu,
        uint32_t * value);
    int encode_unsigned32(
        uint8_t * apdu,
        uint32_t value);
    int decode_unsigned32(
        uint8_t * apdu,
        uint32_t * value);

    /* signed value encoding and decoding */
    int encode_signed8(
        uint8_t * apdu,
        int8_t value);
    int decode_signed8(
        uint8_t * apdu,
        int32_t * value);
    int encode_signed16(
        uint8_t * apdu,
        int16_t value);
    int decode_signed16(
        uint8_t * apdu,
        int32_t * value);
    int encode_signed24(
        uint8_t * apdu,
        int32_t value);
    int decode_signed24(
        uint8_t * apdu,
        int32_t * value);
    int encode_signed32(
        uint8_t * apdu,
        int32_t value);
    int decode_signed32(
        uint8_t * apdu,
        int32_t * value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
