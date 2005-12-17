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
#include "bacdef.h"
#include "bacenum.h"
#include "bits.h"
#include "bigend.h"

void bitstring_init(BACNET_BIT_STRING *bit_string)
{
  int i;
  
  bit_string->bits_used = 0;
  for (i = 0; i < MAX_BITSTRING_BYTES; i++)
  {
    bit_string->value[i] = 0;
  }
}

void bitstring_set_bit(BACNET_BIT_STRING *bit_string, uint8_t bit, bool value)
{
  uint8_t byte_number = bit/8;
  uint8_t bit_mask = 1;

  if (byte_number < MAX_BITSTRING_BYTES)
  {
    // set max bits used
    if (bit_string->bits_used < (bit + 1))
      bit_string->bits_used = bit + 1;
    bit_mask = bit_mask << (bit - (byte_number * 8));
    if (value)
      bit_string->value[byte_number] |= bit_mask;
    else
      bit_string->value[byte_number] &= (~(bit_mask));
  }
}

bool bitstring_bit(BACNET_BIT_STRING *bit_string, uint8_t bit)
{
  bool value = false;
  uint8_t byte_number = bit/8;
  uint8_t bit_mask = 1;
  
  if (bit < (MAX_BITSTRING_BYTES * 8))
  {
    bit_mask = bit_mask << (bit - (byte_number * 8));
    if (bit_string->value[byte_number] & bit_mask)
      value = true;
  }

  return value;
}

uint8_t bitstring_bits_used(BACNET_BIT_STRING *bit_string)
{
  return bit_string->bits_used;
}

/* returns false if the string exceeds capacity
   initialize by using length=0 */
bool characterstring_init(
  BACNET_CHARACTER_STRING *char_string,
  char *value,
  size_t length)
{
  bool status = false; /* return value */
  size_t i; /* counter */

  if (char_string)
  {
    char_string->length = 0;
    if (length < sizeof(char_string->value))
    {
      if (value)
      {
        for (i = 0; i < length; i++)
        {
          char_string->value[char_string->length] = value[i];
          char_string->length++;
        }
      }
      else
      {
        for (i = 0; i < sizeof(char_string->value); i++)
        {
          char_string->value[char_string->length] = 0;
          char_string->length++;
        }
      }
      status = true;
    }
  }

  return status;
}

/* returns false if the string exceeds capacity */
bool characterstring_append(
  BACNET_CHARACTER_STRING *char_string,
  char *value,
  size_t length)
{
  size_t i; /* counter */
  bool status = false; /* return value */

  if (char_string)
  {
    if ((length + char_string->length) < sizeof(char_string->value))
    {
      for (i = 0; i < length; i++)
      {
        char_string->value[char_string->length] = value[i];
        char_string->length++;
      }
      status = true;
    }
  }

  return status;
}

/* This function sets a new length without changing the value.
   If length exceeds capacity, no modification happens and
   function returns false.  */
bool characterstring_truncate(
  BACNET_CHARACTER_STRING *char_string,
  size_t length)
{
  bool status = false; /* return value */

  if (char_string)
  {
    if (length < sizeof(char_string->value))
    {
      char_string->length = length;
      status = true;
    }
  }

  return status;
}

/* returns the length.  Returns the value in parameter. */
size_t characterstring_value(BACNET_CHARACTER_STRING *char_string, char *value)
{
  size_t length = 0;
  size_t i; /* counter */
  
  if (char_string)
  {
    /* FIXME: validate length is within bounds? */
    length = char_string->length;
    if (value)
    {
      for (i = 0; i < length; i++)
      {
        value[i] = char_string->value[i];
      }
    }
  }

  return length;
}

/* returns the length. */
size_t characterstring_length(BACNET_CHARACTER_STRING *char_string)
{
  size_t length = 0;
  size_t i; /* counter */

  if (char_string)
  {
    /* FIXME: validate length is within bounds? */
    length = char_string->length;
  }

  return length;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testBitString(Test * pTest)
{
  uint8_t bit = 0;
  BACNET_BIT_STRING bit_string;
  BACNET_BIT_STRING decoded_bit_string;

  bitstring_init(&bit_string);
  // verify initialization 
  ct_test(pTest, bitstring_bits_used(&bit_string) == 0);
  for (bit = 0; bit < (MAX_BITSTRING_BYTES*8); bit++)
  {
    ct_test(pTest, bitstring_bit(&bit_string, bit) == false);
  }
  
  // test for true
  for (bit = 0; bit < (MAX_BITSTRING_BYTES*8); bit++)
  {
    bitstring_set_bit(&bit_string, bit, true);
    ct_test(pTest, bitstring_bits_used(&bit_string) == (bit + 1));
    ct_test(pTest, bitstring_bit(&bit_string, bit) == true);
  }
  // test for false
  bitstring_init(&bit_string);
  for (bit = 0; bit < (MAX_BITSTRING_BYTES*8); bit++)
  {
    bitstring_set_bit(&bit_string, bit, false);
    ct_test(pTest, bitstring_bits_used(&bit_string) == (bit + 1));
    ct_test(pTest, bitstring_bit(&bit_string, bit) == false);
  }
}

void testCharacterString(Test * pTest)
{
  BACNET_CHARACTER_STRING bacnet_string;
  BACNET_CHARACTER_STRING test_bacnet_string;
  char value[MAX_APDU] = "Joshua,Mary,Anna,Christopher";
  char test_value[MAX_APDU] = "Patricia";
  char test_append_value[MAX_APDU] = " and the Kids";
  char test_append_string[MAX_APDU] = "";
  bool status = false;
  size_t length = 0;
  size_t test_length = 0;
  size_t i = 0;

  // verify initialization
  status = characterstring_init(&bacnet_string,NULL,0);
  ct_test(pTest, status == true);
  ct_test(pTest,characterstring_length(&bacnet_string) == 0);
  ct_test(pTest, characterstring_value(&bacnet_string,&value[0]) == 0);
  for (i = 0; i < sizeof(value); i++)
  {
    ct_test(pTest, value[i] == 0);
  }
  /* bounds check */
  status = characterstring_init(&bacnet_string,NULL,sizeof(value)+1);
  ct_test(pTest, status == false);
  status = characterstring_init(&bacnet_string,NULL,sizeof(value));
  ct_test(pTest, status == true);
  status = characterstring_truncate(&bacnet_string,sizeof(value)+1);
  ct_test(pTest, status == false);
  status = characterstring_truncate(&bacnet_string,sizeof(value));
  ct_test(pTest, status == true);

  test_length = strlen(test_value); 
  status = characterstring_init(
    &bacnet_string,
    &test_value[0],
    test_length);
  ct_test(pTest, status == true);
  length = characterstring_value(&bacnet_string,&value[0]);
  ct_test(pTest, length == test_length);
  for (i = 0; i < test_length; i++)
  {
    ct_test(pTest, value[i] == test_value[i]);
  }

  test_length = strlen(test_append_value);
  status = characterstring_append(
    &bacnet_string,
    &test_append_value[0],
    test_length);
  strcat(test_append_string,test_value);
  strcat(test_append_string,test_append_value);
  test_length = strlen(test_append_string);
  ct_test(pTest, status == true);
  length = characterstring_value(&bacnet_string,&value[0]);
  ct_test(pTest, length == test_length);
  for (i = 0; i < test_length; i++)
  {
    ct_test(pTest, value[i] == test_append_string[i]);
  }
}

void testOctetString(Test * pTest)
{
}

#ifdef TEST_BACSTR
int main(void)
{
  Test *pTest;
  bool rc;

  pTest = ct_create("BACnet Strings", NULL);
  /* individual tests */
  rc = ct_addTestFunction(pTest, testBitString);
  assert(rc);
  rc = ct_addTestFunction(pTest, testCharacterString);
  assert(rc);
  rc = ct_addTestFunction(pTest, testOctetString);
  assert(rc);
    // configure output
  ct_setStream(pTest, stdout);
  ct_run(pTest);
  (void) ct_report(pTest);
  ct_destroy(pTest);

  return 0;
}
#endif                          /* TEST_BACSTR */
#endif                          /* TEST */
