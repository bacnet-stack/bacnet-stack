/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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
#ifndef BACSTR_H
#define BACSTR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bacdef.h"

/* bit strings
   They could be as large as 256/8=32 octets */
#define MAX_BITSTRING_BYTES 15
typedef struct BACnet_Bit_String
{
  uint8_t bits_used;
  uint8_t value[MAX_BITSTRING_BYTES];
} BACNET_BIT_STRING;

typedef struct BACnet_Character_String
{
  size_t length;
  char value[MAX_APDU];
} BACNET_CHARACTER_STRING;

/* FIXME: convert the bacdcode library to use BACNET_OCTET_STRING
   for APDU buffer to prevent buffer overflows */
typedef struct BACnet_Octet_String
{
  size_t length;
  uint8_t value[MAX_APDU];
} BACNET_OCTET_STRING;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bitstring_init(BACNET_BIT_STRING *bit_string);
void bitstring_set_bit(BACNET_BIT_STRING *bit_string, uint8_t bit, bool value);
bool bitstring_bit(BACNET_BIT_STRING *bit_string, uint8_t bit);
uint8_t bitstring_bits_used(BACNET_BIT_STRING *bit_string);

/* returns false if the string exceeds capacity
   initialize by using length=0 */
bool characterstring_init(
  BACNET_CHARACTER_STRING *char_string,
  char *value,
  size_t length);
/* returns false if the string exceeds capacity */
bool characterstring_append(
  BACNET_CHARACTER_STRING *char_string,
  char *value,
  size_t length);
/* This function sets a new length without changing the value.
   If length exceeds capacity, no modification happens and
   function returns false.  */
bool characterstring_truncate(
  BACNET_CHARACTER_STRING *char_string,
  size_t length);
/* returns the length.  Returns the value in parameter. */
size_t characterstring_value(BACNET_CHARACTER_STRING *char_string, char *value);
size_t characterstring_length(BACNET_CHARACTER_STRING *char_string);

/* returns false if the string exceeds capacity
   initialize by using length=0 */
bool octetstring_init(
  BACNET_OCTET_STRING *octet_string,
  uint8_t *value,
  size_t length);
/* returns false if the string exceeds capacity */
bool octetstring_append(
    BACNET_OCTET_STRING *octet_string,
  uint8_t *value,
  size_t length);
/* This function sets a new length without changing the value.
   If length exceeds capacity, no modification happens and
   function returns false.  */
bool octetstring_truncate(
    BACNET_OCTET_STRING *octet_string,
  size_t length);
/* returns the length.  Returns the value in parameter. */
size_t octetstring_value(BACNET_OCTET_STRING *octet_string, uint8_t *value);
size_t octetstring_length(BACNET_OCTET_STRING *octet_string);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
