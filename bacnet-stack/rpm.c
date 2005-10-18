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
#include "rpm.h"

/* encode the initial portion of the service */
int rpm_encode_apdu_init(
  uint8_t *apdu, 
  uint8_t invoke_id)
{
  int apdu_len = 0; /* total length of the apdu, return value */

  if (apdu)
  {
    apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
    apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
    apdu[2] = invoke_id; 
    apdu[3] = SERVICE_CONFIRMED_READ_PROPERTY_MULTIPLE; /* service choice */
    apdu_len = 4;
  }

  return apdu_len;
}

int rpm_encode_apdu_object_begin(
  uint8_t *apdu,
  BACNET_OBJECT_TYPE object_type,
  uint32_t object_instance)
{
  int apdu_len = 0; /* total length of the apdu, return value */
  
  if (apdu)
  {
    apdu_len = encode_context_object_id(&apdu[0], 0,
      object_type, object_instance);
    /* sequence of BACnetPropertyReference */
    apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
  }

  return apdu_len;
}

int rpm_encode_apdu_object_property(
  uint8_t *apdu,
  BACNET_PROPERTY_ID object_property,
  int32_t array_index)
{
  int apdu_len = 0; /* total length of the apdu, return value */
  
  if (apdu)
  {
    apdu_len = encode_context_enumerated(&apdu[0], 0,
      object_property);
    /* optional array index */
    if (array_index != BACNET_ARRAY_ALL)
      apdu_len += encode_context_unsigned(&apdu[apdu_len], 1,
        array_index);
  }

  return apdu_len;
}

int rpm_encode_apdu_object_end(
  uint8_t *apdu)
{
  int apdu_len = 0; /* total length of the apdu, return value */
  
  if (apdu)
  {
    apdu_len = encode_closing_tag(&apdu[0], 1);
  }

  return apdu_len;
}

/* decode the object portion of the service request only */
int rpm_decode_service_request_object_id(
  uint8_t *apdu,
  unsigned apdu_len,
  BACNET_OBJECT_TYPE *object_type,
  uint32_t *object_instance)
{
  unsigned len = 0;
  int type = 0; /* for decoding */

  /* check for value pointers */
  if (apdu && apdu_len && object_type && object_instance)
  {
    /* Tag 0: Object ID */
    if (!decode_is_context_tag(&apdu[len++], 0))
      return -1;
    len += decode_object_id(&apdu[len], &type, object_instance);
    if (object_type)
      *object_type = type;
    /* Tag 1: sequence of BACnetPropertyReference */
    if (!decode_is_opening_tag_number(&apdu[len], 1))
      return -1;
    len++; /* opening tag is only one octet */
  }
        
  return (int)len;
}

int rpm_decode_apdu_object_end(
  uint8_t *apdu,
  unsigned apdu_len)
{
  int len = 0; /* total length of the apdu, return value */
  
  if (apdu && apdu_len)
  {
    if (decode_is_closing_tag_number(apdu,1))
      len = 1;
  }

  return len;
}

/* decode the object property portion of the service request only */
int rpm_decode_service_request_object_property(
  uint8_t *apdu,
  unsigned apdu_len,
  BACNET_PROPERTY_ID *object_property,
  int32_t *array_index)
{
  unsigned len = 0;
  unsigned option_len = 0;
  uint8_t tag_number = 0;
  uint32_t len_value_type = 0;
  int property = 0; /* for decoding */
  unsigned array_value = 0; /* for decoding */

  /* check for valid pointers */
  if (apdu && apdu_len && object_property && array_index)
  {
    /* Tag 0: propertyIdentifier */
    len += decode_tag_number_and_value(&apdu[len],
        &tag_number, &len_value_type);
    if (tag_number != 0)
      return -1;
    len += decode_enumerated(&apdu[len], len_value_type, &property);
    if (object_property)
      *object_property = property;
    /* Tag 1: Optional propertyArrayIndex */
    if (len < apdu_len)
    {
      option_len = decode_tag_number_and_value(&apdu[len],&tag_number,
        &len_value_type);
      if (tag_number == 2)
      {
        len += option_len;
        len += decode_unsigned(&apdu[len], len_value_type,
          &array_value);
        *array_index = array_value;
      }
      else
        *array_index = BACNET_ARRAY_ALL;
    }
    else
      *array_index = BACNET_ARRAY_ALL;
  }

  return (int)len;
}

int rpm_decode_apdu(
  uint8_t *apdu,
  unsigned apdu_len,
  uint8_t *invoke_id,
  uint8_t **service_request,
  unsigned *service_request_len)
{
  unsigned offset = 0;

  if (!apdu)
    return -1;
  /* optional checking - most likely was already done prior to this call */
  if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
    return -1;
  /*  apdu[1] = encode_max_segs_max_apdu(0, Device_Max_APDU_Length_Accepted()); */
  *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
  if (apdu[3] != SERVICE_CONFIRMED_READ_PROPERTY_MULTIPLE)
    return -1;
  offset = 4;

  if (apdu_len > offset)
  {
    if (service_request)
      *service_request = &apdu[offset];
    if (service_request_len)
      *service_request_len = apdu_len - offset;
  }
  
  return offset;
}

int rpm_ack_encode_apdu_init(
  uint8_t *apdu,
  uint8_t invoke_id)
{
  int apdu_len = 0; /* total length of the apdu, return value */

  if (apdu)
  {
    apdu[0] = PDU_TYPE_COMPLEX_ACK;     /* complex ACK service */
    apdu[1] = invoke_id;        /* original invoke id from request */
    apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY_MULTIPLE; /* service choice */
    apdu_len = 3;
  }

  return apdu_len;
}

int rpm_ack_encode_apdu_object_begin(
  uint8_t *apdu,
  BACNET_OBJECT_TYPE object_type,
  uint32_t object_instance)
{
  int apdu_len = 0; /* total length of the apdu, return value */
  
  if (apdu)
  {
    apdu_len = encode_context_object_id(&apdu[0], 0,
      object_type, object_instance);
    /* sequence of BACnetPropertyReference */
    apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
  }

  return apdu_len;
}

int rpm_ack_encode_apdu_object_property_value(
  uint8_t *apdu,
  BACNET_PROPERTY_ID object_property,
  int32_t array_index,
  uint8_t *application_data,
  unsigned application_data_len)
{
  int apdu_len = 0; /* total length of the apdu, return value */
  unsigned len = 0;
  
  if (apdu)
  {
    apdu_len = encode_context_enumerated(&apdu[0], 2,
      object_property);
    /* optional array index */
    if (array_index != BACNET_ARRAY_ALL)
      apdu_len += encode_context_unsigned(&apdu[apdu_len], 3,
        array_index);
    /* propertyValue */
    apdu_len += encode_opening_tag(&apdu[apdu_len], 4);
    for (len = 0; len < application_data_len; len++)
    {
      apdu[apdu_len++] = application_data[len];
    }
    apdu_len += encode_closing_tag(&apdu[apdu_len], 4);
  }

  return apdu_len;
}

int rpm_ack_encode_apdu_object_property_error(
  uint8_t *apdu,
  BACNET_PROPERTY_ID object_property,
  int32_t array_index,
  BACNET_ERROR_CLASS error_class,
  BACNET_ERROR_CODE error_code)
{
  int apdu_len = 0; /* total length of the apdu, return value */
  
  if (apdu)
  {
    apdu_len = encode_context_enumerated(&apdu[0], 2,
      object_property);
    /* optional array index */
    if (array_index != BACNET_ARRAY_ALL)
      apdu_len += encode_context_unsigned(&apdu[apdu_len], 3,
        array_index);
    /* errorCode */
    apdu_len += encode_opening_tag(&apdu[apdu_len], 4);
    apdu_len += encode_tagged_enumerated(&apdu[apdu_len], error_class);
    apdu_len += encode_tagged_enumerated(&apdu[apdu_len], error_code);
    apdu_len += encode_closing_tag(&apdu[apdu_len], 4);
  }

  return apdu_len;
}

int rpm_ack_encode_apdu_object_end(
  uint8_t *apdu)
{
  int apdu_len = 0; /* total length of the apdu, return value */
  
  if (apdu)
  {
    apdu_len = encode_closing_tag(&apdu[0], 1);
  }

  return apdu_len;
}

int rpm_ack_decode_apdu(
  uint8_t *apdu,
  int apdu_len, /* total length of the apdu */
  uint8_t *invoke_id,
  uint8_t **service_request,
  unsigned *service_request_len)
{
  int offset = 0;

  if (!apdu)
    return -1;
  /* optional checking - most likely was already done prior to this call */
  if (apdu[0] != PDU_TYPE_COMPLEX_ACK)
    return -1;
  *invoke_id = apdu[1];
  if (apdu[2] != SERVICE_CONFIRMED_READ_PROPERTY_MULTIPLE)
    return -1;
  offset = 3;
  if (apdu_len > offset)
  {
    if (service_request)
      *service_request = &apdu[offset];
    if (service_request_len)
      *service_request_len = apdu_len - offset;
  }
  
  return offset;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testReadPropertyMultiple(Test * pTest)
{
  uint8_t apdu[480] = {0};
  int len = 0;
  int test_len = 0;
  int apdu_len = 0;
  uint8_t invoke_id = 12;
  uint8_t test_invoke_id = 0;
  uint8_t *service_request = NULL;
  unsigned service_request_len = 0;
  BACNET_OBJECT_TYPE object_type = OBJECT_DEVICE;
  uint32_t object_instance = 0;
  BACNET_PROPERTY_ID object_property = PROP_OBJECT_IDENTIFIER;
  int32_t array_index = 0;

  /* build the RPM - try to make it easy for the Application Layer development */
  /* IDEA: similar construction, but pass apdu, apdu_len pointer, size of apdu to
     let the called function handle the out of space problem that these get into
     by returning a boolean of success/failure.
     It almost needs to use the keylist library or something similar.
     Also check case of storing a backoff point (i.e. save enough room for object_end) */
  apdu_len = rpm_encode_apdu_init(&apdu[0], invoke_id);
  /* each object has a beginning and an end */
  apdu_len += rpm_encode_apdu_object_begin(&apdu[apdu_len],
    OBJECT_DEVICE, 123);
  /* then stuff as many properties into it as APDU length will allow */
  apdu_len += rpm_encode_apdu_object_property(&apdu[apdu_len],
    PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL);
  apdu_len += rpm_encode_apdu_object_property(&apdu[apdu_len],
    PROP_OBJECT_NAME, BACNET_ARRAY_ALL);
  apdu_len += rpm_encode_apdu_object_end(&apdu[apdu_len]);
  /* each object has a beginning and an end */
  apdu_len += rpm_encode_apdu_object_begin(&apdu[apdu_len],
    OBJECT_ANALOG_INPUT, 33);
  apdu_len += rpm_encode_apdu_object_property(&apdu[apdu_len],
    PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL);
  apdu_len += rpm_encode_apdu_object_property(&apdu[apdu_len],
    PROP_ALL, BACNET_ARRAY_ALL);
  apdu_len += rpm_encode_apdu_object_end(&apdu[apdu_len]);

  ct_test(pTest, apdu_len != 0);
  
  test_len = rpm_decode_apdu(
    &apdu[0],
    apdu_len,
    &test_invoke_id,
    &service_request, /* will point to the service request in the apdu */
    &service_request_len);
  ct_test(pTest, test_len != -1);
  ct_test(pTest, test_invoke_id == invoke_id);
  ct_test(pTest, service_request != NULL);
  ct_test(pTest, service_request_len > 0);
  
  test_len = rpm_decode_service_request_object_id(
    service_request,
    service_request_len,
    &object_type,
    &object_instance);
  ct_test(pTest, test_len != -1);
  ct_test(pTest, object_type == OBJECT_DEVICE);
  ct_test(pTest, object_instance == 123);
  len = test_len;
  /* decode the object property portion of the service request */
  test_len = rpm_decode_service_request_object_property(
    &service_request[len],
    service_request_len - len,
    &object_property,
    &array_index);
  ct_test(pTest, test_len != -1);
  ct_test(pTest, object_property == PROP_OBJECT_IDENTIFIER);
  ct_test(pTest, array_index == BACNET_ARRAY_ALL);
  len += test_len;
  test_len = rpm_decode_service_request_object_property(
    &service_request[len],
    service_request_len - len,
    &object_property,
    &array_index);
  ct_test(pTest, test_len != -1);
  ct_test(pTest, object_property == PROP_OBJECT_NAME);
  ct_test(pTest, array_index == BACNET_ARRAY_ALL);
  len += test_len;
  /* try again - we should fail */
  test_len = rpm_decode_service_request_object_property(
    &service_request[len],
    service_request_len - len,
    &object_property,
    &array_index);
  ct_test(pTest, test_len == -1);
  /* is it the end of this object? */
  test_len = rpm_decode_apdu_object_end(
    &service_request[len],
    service_request_len - len);
  ct_test(pTest, test_len == 1);
  len += test_len;
  /* try to decode an object id */
  test_len = rpm_decode_service_request_object_id(
    &service_request[len],
    service_request_len - len,
    &object_type,
    &object_instance);
  ct_test(pTest, test_len != -1);
  ct_test(pTest, object_type == OBJECT_ANALOG_INPUT);
  ct_test(pTest, object_instance == 33);
  len += test_len;
  /* decode the object property portion of the service request only */
  test_len = rpm_decode_service_request_object_property(
    &service_request[len],
    service_request_len - len,
    &object_property,
    &array_index);
  ct_test(pTest, test_len != -1);
  ct_test(pTest, object_property == PROP_OBJECT_IDENTIFIER);
  ct_test(pTest, array_index == BACNET_ARRAY_ALL);
  len += test_len;
  test_len = rpm_decode_service_request_object_property(
    &service_request[len],
    service_request_len - len,
    &object_property,
    &array_index);
  ct_test(pTest, test_len != -1);
  ct_test(pTest, object_property == PROP_ALL);
  ct_test(pTest, array_index == BACNET_ARRAY_ALL);
  len += test_len;
  test_len = rpm_decode_service_request_object_property(
    &service_request[len],
    service_request_len - len,
    &object_property,
    &array_index);
  ct_test(pTest, test_len == -1);
  /* got an error -1, is it the end of this object? */
  test_len = rpm_decode_apdu_object_end(
    &service_request[len],
    service_request_len - len);
  ct_test(pTest, test_len == 1);
  len += test_len;
  ct_test(pTest, len == service_request_len);
}

#ifdef TEST_READ_PROPERTY_MULTIPLE
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet ReadPropertyMultiple", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testReadPropertyMultiple);
    assert(rc);
    //rc = ct_addTestFunction(pTest, testReadPropertyMultipleAck);
    //assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_READ_PROPERTY_MULTIPLE */
#endif                          /* TEST */
