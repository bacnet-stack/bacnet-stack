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
#include "bacenum.h"
#include "bacdcode.h"
#include "bacdef.h"
#include "device.h"

// encode service  - use -1 for limit if you want unlimited
int rp_encode_apdu(
  uint8_t *apdu, 
  BACNET_OBJECT_TYPE object_type,
  uint32_t object_instance
  BACNET_PROPERTY_ID object_property,
  int32_t array_index)
{
  int len = 0; // length of each encoding
  int apdu_len = 0; // total length of the apdu, return value

  if (apdu)
  {
    apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
    apdu[1] = encode_max_segs_max_apdu(0, Device_Max_APDU_Length_Accepted());
    apdu[2] = 0; /* invoke id - filled in by net layer */
    apdu[3] = SERVICE_CONFIRMED_READ_PROPERTY;     // service choice
    apdu_len = 4;
    len = encode_context_object_id(&apdu[apdu_len], 0,
      object_type, object_instance);
    apdu_len += len;
    len = encode_context_enumerated(&apdu[apdu_len], 1,
      object_property);
    apdu_len += len;
    /* optional array index; ALL is -1 which is assumed when missing */
    if (array_index >= 0)
    {  
      len = encode_context_unsigned(&apdu[apdu_len], 2,
        array_index);
      apdu_len += len;
    }
  }
  
  return apdu_len;
}

// decode the service request only
int rp_decode_service_request(
  uint8_t *apdu,
  unsigned apdu_len,
  BACNET_OBJECT_TYPE *object_type,
  uint32_t *object_instance
  BACNET_PROPERTY_ID *object_property,
  int32_t *array_index)
{
  int len = 0;
  uint8_t tag_number = 0;
  uint32_t len_value_type = 0;
  unsigned int decoded_value = 0;

  // check for value pointers
  if (apdu_len && object_type && object_instance &&
      object_property && array_index)
  {
    // Tag 0: Object ID         
    if (!decode_is_context_tag(&apdu[len++], 0))
      return -1;
    len += decode_object_id(&apdu[len], object_type, object_instance);
    // Tag 1: Property ID
    len += decode_tag_number_and_value(&apdu[len],
        &tag_number, &len_value_type);
    if (tag_number != 1)
      return -1;
    len += decode_enumerated(&apdu[len], len_value_type, object_property);
    // Tag 2: Optional Array Index
    if (len < apdu_len)
    {
      len += decode_tag_number_and_value(&apdu[len],&tag_number,
        &len_value_type);
      if (tag_number == 2)
        len += decode_unsigned(&apdu[len], len_value_type,
          array_index);
      else
        *array_index = BACNET_ARRAY_ALL;
    }
  }

  return len;
}

int rp_decode_apdu(
  uint8_t *apdu,
  unsigned apdu_len,
  BACNET_OBJECT_TYPE *object_type,
  uint32_t *object_instance
  BACNET_PROPERTY_ID *object_property,
  int32_t *array_index)
{
  int len = 0;

  if (!apdu)
    return -1;
  // optional checking - most likely was already done prior to this call
  if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
    return -1;
  if (apdu[3] != SERVICE_CONFIRMED_READ_PROPERTY)
    return -1;
  //  apdu[1] = encode_max_segs_max_apdu(0, Device_Max_APDU_Length_Accepted());
  //  apdu[2] = 0; /* invoke id - filled in by net layer */

  if (apdu_len > 4)
  {
    len = rp_decode_service_request(
      &apdu[2],
      apdu_len - 2,
      object_type,
      object_instance
      object_property,
      array_index)
  }
  
  return len;
}

int rp_ack_encode_apdu(
  uint8_t *apdu, 
  int32_t low_limit,
  int32_t high_limit)
{
  int len = 0;
  
  // FIXME:  how about some code?
  
  return len;
}

int rp_ack_decode_apdu(
  uint8_t *apdu, 
  int32_t low_limit,
  int32_t high_limit)
{
  int len = 0;
  
  // FIXME:  how about some code?
  
  return len;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testReadPropertyAck(Test * pTest)
{
  // FIXME:  how about some code?
}

void testReadProperty(Test * pTest)
{
  uint8_t apdu[480] = {0};
  int len = 0;
  int apdu_len = 0;
  int32_t low_limit = -1;
  int32_t high_limit = -1;
  int32_t test_low_limit = -1;
  int32_t test_high_limit = -1;
    
  len = whois_encode_apdu(
    &apdu[0],
    low_limit,
    high_limit);
  ct_test(pTest, len != 0);
  apdu_len = len;

  len = whois_decode_apdu(
    &apdu[0],
    apdu_len,
    &test_low_limit,
    &test_high_limit);
  ct_test(pTest, len != -1);
  ct_test(pTest, test_low_limit == low_limit);
  ct_test(pTest, test_high_limit == high_limit);

  for (
    low_limit = 0;
    low_limit <= BACNET_MAX_INSTANCE;
    low_limit += (BACNET_MAX_INSTANCE/4))
  {
    for (
      high_limit = 0;
      high_limit <= BACNET_MAX_INSTANCE;
      high_limit += (BACNET_MAX_INSTANCE/4))
    {
      len = whois_encode_apdu(
        &apdu[0],
        low_limit,
        high_limit);
      apdu_len = len;
      ct_test(pTest, len != 0);
      len = whois_decode_apdu(
        &apdu[0], 
        apdu_len,
        &test_low_limit,
        &test_high_limit);
      ct_test(pTest, len != -1);
      ct_test(pTest, test_low_limit == low_limit);
      ct_test(pTest, test_high_limit == high_limit);
    }
  }
}

#ifdef TEST_WHOIS
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet ReadProperty", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testReadProperty);
    assert(rc);
    rc = ct_addTestFunction(pTest, testReadPropertyAck);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_WHOIS */
#endif                          /* TEST */
