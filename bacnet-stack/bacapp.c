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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "bacenum.h"
#include "bacdcode.h"
#include "bacdef.h"
#include "bacapp.h"

int bacapp_encode_application_data(
  uint8_t *apdu,
  BACNET_APPLICATION_DATA_VALUE *value)
{
  int apdu_len = 0; // total length of the apdu, return value

  if (apdu)
  {
    if (value->tag == BACNET_APPLICATION_TAG_NULL)
      apdu[apdu_len++] = value->tag;
    else if (value->tag == BACNET_APPLICATION_TAG_BOOLEAN)
      apdu_len += encode_tagged_boolean(&apdu[apdu_len],
        value->type.Boolean);
    else if (value->tag == BACNET_APPLICATION_TAG_UNSIGNED_INT)
      apdu_len += encode_tagged_unsigned(&apdu[apdu_len],
        value->type.Unsigned_Int);
    else if (value->tag == BACNET_APPLICATION_TAG_SIGNED_INT)
      apdu_len += encode_tagged_signed(&apdu[apdu_len],
        value->type.Signed_Int);
    else if (value->tag == BACNET_APPLICATION_TAG_REAL)
      apdu_len += encode_tagged_real(&apdu[apdu_len],
        value->type.Real);
#if 0
    else if (value->tag == BACNET_APPLICATION_TAG_DOUBLE)
      apdu_len += encode_tagged_double(&apdu[apdu_len],
        value->type.Double);
#endif
    else if (value->tag == BACNET_APPLICATION_TAG_CHARACTER_STRING)
      apdu_len += encode_tagged_character_string(
        &apdu[apdu_len],
        &value->type.Character_String);
    else if (value->tag == BACNET_APPLICATION_TAG_OCTET_STRING)
      apdu_len += encode_tagged_octet_string(
        &apdu[apdu_len],
        &value->type.Octet_String);
    else if (value->tag == BACNET_APPLICATION_TAG_ENUMERATED)
      apdu_len += encode_tagged_enumerated(&apdu[apdu_len],
        value->type.Enumerated);
    else if (value->tag == BACNET_APPLICATION_TAG_DATE)
      apdu_len += encode_tagged_date(&apdu[apdu_len],
        value->type.Date.year,
        value->type.Date.month,
        value->type.Date.day,
        value->type.Date.wday);
    else if (value->tag == BACNET_APPLICATION_TAG_TIME)
      apdu_len += encode_tagged_time(&apdu[apdu_len],
        value->type.Time.hour,
        value->type.Time.min,
        value->type.Time.sec,
        value->type.Time.hundredths);
    else if (value->tag == BACNET_APPLICATION_TAG_OBJECT_ID)
      apdu_len += encode_tagged_object_id(&apdu[apdu_len],
        value->type.Object_Id.type,value->type.Object_Id.instance);
  }

  return apdu_len;
}

int bacapp_decode_application_data(
  uint8_t *apdu,
  uint8_t apdu_len,
  BACNET_APPLICATION_DATA_VALUE *value)
{
  int len = 0;
  int tag_len = 0;
  uint8_t tag_number = 0;
  uint32_t len_value_type = 0;
  int hour, min, sec, hundredths;
  int year, month, day, wday;
  int object_type = 0;
  uint32_t instance = 0;

  /* FIXME: use apdu_len! */
  (void)apdu_len;
  if (apdu)
  {
    tag_len = decode_tag_number_and_value(&apdu[0],
      &tag_number, &len_value_type);
    if (tag_len)
    {
      len += tag_len;
      if (tag_number == BACNET_APPLICATION_TAG_NULL)
      {
        value->tag = tag_number;
      }
      else if (tag_number == BACNET_APPLICATION_TAG_BOOLEAN)
      {
        value->tag = tag_number;
        value->type.Boolean = decode_boolean(len_value_type);
      }
      else if (tag_number == BACNET_APPLICATION_TAG_UNSIGNED_INT)
      {
        value->tag = tag_number;
        len += decode_unsigned(&apdu[len],
          len_value_type,
          &value->type.Unsigned_Int);
      }
      else if (tag_number == BACNET_APPLICATION_TAG_SIGNED_INT)
      {
        value->tag = tag_number;
        len += decode_signed(&apdu[len],
          len_value_type,
          &value->type.Signed_Int);
      }
      else if (tag_number == BACNET_APPLICATION_TAG_REAL)
      {
        value->tag = tag_number;
        len += decode_real(&apdu[len],&(value->type.Real));
      }
#if 0
      else if (tag_number == BACNET_APPLICATION_TAG_DOUBLE)
      {
        value->tag = tag_number;
        len += decode_double(&apdu[len],&(value->type.Double));
      }
#endif
      else if (tag_number == BACNET_APPLICATION_TAG_CHARACTER_STRING)
      {
        value->tag = tag_number;
        len += decode_character_string(&apdu[len],
          len_value_type,
          &value->type.Character_String);
      }
      else if (tag_number == BACNET_APPLICATION_TAG_OCTET_STRING)
      {
        value->tag = tag_number;
        len += decode_octet_string(&apdu[len],
          len_value_type,
          &value->type.Octet_String);
      }
      else if (tag_number == BACNET_APPLICATION_TAG_ENUMERATED)
      {
        value->tag = tag_number;
        len += decode_enumerated(&apdu[len],
          len_value_type,
          &value->type.Enumerated);
      }
      else if (tag_number == BACNET_APPLICATION_TAG_DATE)
      {
        value->tag = tag_number;
        len += decode_date(&apdu[len], &year, &month, &day, &wday);
        value->type.Date.year = year;
        value->type.Date.month = month;
        value->type.Date.day = day;
        value->type.Date.wday = wday;
      }
      else if (tag_number == BACNET_APPLICATION_TAG_TIME)
      {
        value->tag = tag_number;
        len += decode_bacnet_time(&apdu[len], &hour, &min, &sec, &hundredths);
        value->type.Time.hour = hour;
        value->type.Time.min = min;
        value->type.Time.sec = sec;
        value->type.Time.hundredths = hundredths;
      }
      else if (tag_number == BACNET_APPLICATION_TAG_OBJECT_ID)
      {
        value->tag = tag_number;
        len += decode_object_id(&apdu[len],
          &object_type, 
          &instance);
        value->type.Object_Id.type = object_type;
        value->type.Object_Id.instance = instance;
      }
    }
  }

  return len;
}

bool bacapp_copy(
  BACNET_APPLICATION_DATA_VALUE *dest_value,
  BACNET_APPLICATION_DATA_VALUE *src_value)
{
  bool status = true; /*return value*/

  if (dest_value && src_value)
  {
    dest_value->tag = src_value->tag;
    switch (src_value->tag)
    {
      case BACNET_APPLICATION_TAG_NULL:
        break;
      case BACNET_APPLICATION_TAG_BOOLEAN:
        dest_value->type.Boolean = src_value->type.Boolean;
        break;
      case BACNET_APPLICATION_TAG_UNSIGNED_INT:
        dest_value->type.Unsigned_Int = src_value->type.Unsigned_Int;
        break;
      case BACNET_APPLICATION_TAG_SIGNED_INT:
        dest_value->type.Signed_Int = src_value->type.Signed_Int;
        break;
      case BACNET_APPLICATION_TAG_REAL:
        dest_value->type.Real = src_value->type.Real;
        break;
      case BACNET_APPLICATION_TAG_DOUBLE:
        dest_value->type.Double = src_value->type.Double;
        break;
      case BACNET_APPLICATION_TAG_ENUMERATED:
        dest_value->type.Enumerated = src_value->type.Enumerated;
        break;
      case BACNET_APPLICATION_TAG_DATE:
        dest_value->type.Date.year = src_value->type.Date.year;
        dest_value->type.Date.month = src_value->type.Date.month;
        dest_value->type.Date.day = src_value->type.Date.day;
        dest_value->type.Date.wday = src_value->type.Date.wday;
        break;
      case BACNET_APPLICATION_TAG_TIME:
        dest_value->type.Time.hour = src_value->type.Time.hour;
        dest_value->type.Time.min = src_value->type.Time.min;
        dest_value->type.Time.sec = src_value->type.Time.sec;
        dest_value->type.Time.hundredths = src_value->type.Time.hundredths;
        break;
      case BACNET_APPLICATION_TAG_OBJECT_ID:
        dest_value->type.Object_Id.type = src_value->type.Object_Id.type;
        dest_value->type.Object_Id.instance = src_value->type.Object_Id.instance;
        break;
      case BACNET_APPLICATION_TAG_CHARACTER_STRING:
        characterstring_copy(
          &dest_value->type.Character_String,
          &src_value->type.Character_String);
        break;
      case BACNET_APPLICATION_TAG_OCTET_STRING:
        octetstring_copy(
          &dest_value->type.Octet_String,
          &src_value->type.Octet_String);
        break;
      default:
        status = false;
        break;
    }
  }
  
  return status;  
}
/* generic - can be used by other unit tests */
bool bacapp_compare(
  BACNET_APPLICATION_DATA_VALUE *value,
  BACNET_APPLICATION_DATA_VALUE *test_value)
{
  bool status = true; /*return value*/

  /* does the tag match? */
  if (test_value->tag != value->tag)
    status = false;
  if (status)
  {
    /* does the value match? */
    switch (test_value->tag)
    {
      case BACNET_APPLICATION_TAG_NULL:
        break;
      case BACNET_APPLICATION_TAG_BOOLEAN:
        if (test_value->type.Boolean != value->type.Boolean)
          status = false;
        break;
      case BACNET_APPLICATION_TAG_UNSIGNED_INT:
        if (test_value->type.Unsigned_Int != value->type.Unsigned_Int)
          status = false;
        break;
      case BACNET_APPLICATION_TAG_SIGNED_INT:
        if (test_value->type.Signed_Int != value->type.Signed_Int)
          status = false;
        break;
      case BACNET_APPLICATION_TAG_REAL:
        if (test_value->type.Real != value->type.Real)
          status = false;
        break;
      case BACNET_APPLICATION_TAG_DOUBLE:
        if (test_value->type.Double != value->type.Double)
          status = false;
        break;
      case BACNET_APPLICATION_TAG_ENUMERATED:
        if (test_value->type.Enumerated != value->type.Enumerated)
          status = false;
        break;
      case BACNET_APPLICATION_TAG_DATE:
        if (test_value->type.Date.year != value->type.Date.year)
          status = false;
        if (test_value->type.Date.month != value->type.Date.month)
          status = false;
        if (test_value->type.Date.day != value->type.Date.day)
          status = false;
        if(test_value->type.Date.wday != value->type.Date.wday)
          status = false;
        break;
      case BACNET_APPLICATION_TAG_TIME:
        if (test_value->type.Time.hour != value->type.Time.hour)
          status = false;
        if (test_value->type.Time.min != value->type.Time.min)
          status = false;
        if (test_value->type.Time.sec != value->type.Time.sec)
          status = false;
        if (test_value->type.Time.hundredths != value->type.Time.hundredths)
          status = false;
        break;
      case BACNET_APPLICATION_TAG_OBJECT_ID:
        if (test_value->type.Object_Id.type != value->type.Object_Id.type)
          status = false;
        if (test_value->type.Object_Id.instance != value->type.Object_Id.instance)
          status = false;
        break;
      case BACNET_APPLICATION_TAG_CHARACTER_STRING:
        {
          size_t length, test_length, i;
          char *str,*test_str;
          length = characterstring_length(&value->type.Character_String);
          str = characterstring_value(&value->type.Character_String);
          test_length = characterstring_length(&test_value->type.Character_String);
          test_str = characterstring_value(&test_value->type.Character_String);
          if (length != test_length)
            status = false;
          for (i = 0; i < test_length; i++)
          {
            if (str[i] != test_str[i])
              status = false;
          }
        }
        break;
      default:
        status = false;
        break;
    }
  }
  return status;
}

/* used to load the app data struct with the proper data
   converted from a command line argument */
bool bacapp_parse_application_data(
  BACNET_APPLICATION_TAG tag_number,
  const char *argv,
  BACNET_APPLICATION_DATA_VALUE *value)
{
  int hour, min, sec, hundredths;
  int year, month, day, wday;
  int object_type = 0;
  uint32_t instance = 0;
  bool status = false;
  long long_value = 0;
  unsigned long unsigned_long_value = 0;
  double double_value = 0.0;
  int count = 0;

  if (value && (tag_number < MAX_BACNET_APPLICATION_TAG))
  {
    status = true;
    value->tag = tag_number;
    if (tag_number == BACNET_APPLICATION_TAG_BOOLEAN)
    {
      long_value = strtol(argv,NULL,0);
      if (long_value)
        value->type.Boolean = true;
      else
        value->type.Boolean = false;
    }
    else if (tag_number == BACNET_APPLICATION_TAG_UNSIGNED_INT)
    {
      unsigned_long_value = strtoul(argv,NULL,0);
      value->type.Unsigned_Int = unsigned_long_value;
    }
    else if (tag_number == BACNET_APPLICATION_TAG_SIGNED_INT)
    {
      long_value = strtol(argv,NULL,0);
      value->type.Signed_Int = long_value;
    }
    else if (tag_number == BACNET_APPLICATION_TAG_REAL)
    {
      double_value = strtod(argv,NULL);
      value->type.Real = double_value;
    }
    else if (tag_number == BACNET_APPLICATION_TAG_DOUBLE)
    {
      double_value = strtod(argv,NULL);
      value->type.Double = double_value;
    }
    else if (tag_number == BACNET_APPLICATION_TAG_CHARACTER_STRING)
    {
      status = characterstring_init_ansi(
        &value->type.Character_String,
        (char *)argv);
    }
    else if (tag_number == BACNET_APPLICATION_TAG_OCTET_STRING)
    {
      status = octetstring_init(
        &value->type.Octet_String,
        (uint8_t *)argv,
        strlen(argv));
    }
    else if (tag_number == BACNET_APPLICATION_TAG_ENUMERATED)
    {
      unsigned_long_value = strtoul(argv,NULL,0);
      value->type.Enumerated = unsigned_long_value;
    }
    else if (tag_number == BACNET_APPLICATION_TAG_DATE)
    {
      count = sscanf(argv,"%d/%d/%d:%d", &year, &month, &day, &wday);
      if (count == 4)
      {
        value->type.Date.year = year;
        value->type.Date.month = month;
        value->type.Date.day = day;
        value->type.Date.wday = wday;
      }
      else
        status = false;
    }
    else if (tag_number == BACNET_APPLICATION_TAG_TIME)
    {
      count = sscanf(argv,"%d:%d:%d.%d", &hour, &min, &sec, &hundredths);
      if (count == 4)
      {
        value->type.Time.hour = hour;
        value->type.Time.min = min;
        value->type.Time.sec = sec;
        value->type.Time.hundredths = hundredths;
      }
      else
        status = false;
    }
    else if (tag_number == BACNET_APPLICATION_TAG_OBJECT_ID)
    {
      count = sscanf(argv,"%d:%d", &object_type, &instance);
      if (count == 2)
      {
        value->type.Object_Id.type = object_type;
        value->type.Object_Id.instance = instance;
      }
      else
        status = false;
    }
  }

  return status;
}


#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

static bool testBACnetApplicationDataValue(BACNET_APPLICATION_DATA_VALUE *value)
{
  uint8_t apdu[480] = {0};
  int len = 0;
  int apdu_len = 0;
  BACNET_APPLICATION_DATA_VALUE test_value;

  apdu_len = bacapp_encode_application_data(&apdu[0],value);
  len = bacapp_decode_application_data(
    &apdu[0],
    apdu_len,
    &test_value);

  return bacapp_compare(value, &test_value);
}

void testBACnetApplicationData(Test * pTest)
{
  BACNET_APPLICATION_DATA_VALUE value = {0};
  bool status = false;


  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_NULL,
    NULL,
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,testBACnetApplicationDataValue(&value));

  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_BOOLEAN,
    "1",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Boolean == true);
  ct_test(pTest,testBACnetApplicationDataValue(&value));

  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_BOOLEAN,
    "0",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Boolean == false);
  ct_test(pTest,testBACnetApplicationDataValue(&value));

  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_UNSIGNED_INT,
    "0",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Unsigned_Int == 0);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_UNSIGNED_INT,
    "0xFFFF",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Unsigned_Int == 0xFFFF);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_UNSIGNED_INT,
    "0xFFFFFFFF",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Unsigned_Int == 0xFFFFFFFF);
  ct_test(pTest,testBACnetApplicationDataValue(&value));

  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_SIGNED_INT,
    "0",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Signed_Int == 0);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_SIGNED_INT,
    "-1",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Signed_Int == -1);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_SIGNED_INT,
    "32768",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Signed_Int == 32768);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_SIGNED_INT,
    "-32768",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Signed_Int == -32768);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_REAL,
    "0.0",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_REAL,
    "-1.0",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_REAL,
    "1.0",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_REAL,
    "3.14159",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_REAL,
    "-3.14159",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
    
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_ENUMERATED,
    "0",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Enumerated == 0);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_ENUMERATED,
    "0xFFFF",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Enumerated == 0xFFFF);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_ENUMERATED,
    "0xFFFFFFFF",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Enumerated == 0xFFFFFFFF);
  ct_test(pTest,testBACnetApplicationDataValue(&value));
  
  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_DATE,
    "5/5/22:1",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Date.year == 5);
  ct_test(pTest,value.type.Date.month == 5);
  ct_test(pTest,value.type.Date.day == 22);
  ct_test(pTest,value.type.Date.wday == 1);
  ct_test(pTest,testBACnetApplicationDataValue(&value));

  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_TIME,
    "23:59:59.12",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Time.hour == 23);
  ct_test(pTest,value.type.Time.min  == 59);
  ct_test(pTest,value.type.Time.sec == 59);
  ct_test(pTest,value.type.Time.hundredths == 12);
  ct_test(pTest,testBACnetApplicationDataValue(&value));

  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_OBJECT_ID,
    "0:100",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,value.type.Object_Id.type == 0);
  ct_test(pTest,value.type.Object_Id.instance == 100);
  ct_test(pTest,testBACnetApplicationDataValue(&value));

  status = bacapp_parse_application_data(
    BACNET_APPLICATION_TAG_CHARACTER_STRING,
    "Karg!",
    &value);
  ct_test(pTest,status == true);
  ct_test(pTest,testBACnetApplicationDataValue(&value));

  return;
}

#ifdef TEST_BACNET_APPLICATION_DATA
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Application Data", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACnetApplicationData);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_BACNET_APPLICATION_DATA */
#endif /* TEST */
