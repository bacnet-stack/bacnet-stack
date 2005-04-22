/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "config.h" // the custom stuff

static uint32_t Object_Instance_Number = 0;
// FIXME: it is likely that this name is configurable,
// so consider a fixed sized string
static const char *Object_Name = "SimpleServer";
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static const char *Vendor_Name = "ASHRAE";
// vendor id assigned by ASHRAE
static uint16_t Vendor_Identifier = 0;
static const char *Model_Name = "GNU";
static const char *Firmware_Revision = "1.0";
static const char *Application_Software_Version = "1.0";
//static char *Location = "USA";
static const char *Description = "server";
//static uint8_t Protocol_Version = 1; - constant, not settable
//static uint8_t Protocol_Revision = 4; - constant, not settable
//Protocol_Services_Supported
//Protocol_Object_Types_Supported
//Object_List
//static uint16_t Max_APDU_Length_Accepted = MAX_APDU; - constant
//static BACNET_SEGMENTATION Segmentation_Supported = SEGMENTATION_NONE;
//static uint8_t Max_Segments_Accepted = 0;
//VT_Classes_Supported
//Active_VT_Sessions
//Local_Time - rely on OS, if there is one
//Local_Date - rely on OS, if there is one
//UTC_Offset - rely on OS, if there is one
//Daylight_Savings_Status - rely on OS, if there is one
//APDU_Segment_Timeout
static uint16_t APDU_Timeout = 3000;
static uint8_t Number_Of_APDU_Retries = 3;
//List_Of_Session_Keys
//Time_Synchronization_Recipients
//Max_Master - rely on MS/TP subsystem, if there is one
//Max_Info_Frames - rely on MS/TP subsystem, if there is one
//Device_Address_Binding - required, but relies on binding cache
static uint8_t Database_Revision = 0;
//Configuration_Files
//Last_Restore_Time
//Backup_Failure_Timeout
//Active_COV_Subscriptions
//Slave_Proxy_Enable
//Manual_Slave_Address_Binding
//Auto_Slave_Discovery
//Slave_Address_Binding
//Profile_Name

// methods to manipulate the data
uint32_t Device_Object_Instance_Number(void)
{
  return Object_Instance_Number;
}

void Device_Set_Object_Instance_Number(uint32_t object_id)
{
  // FIXME: bounds check?
  Object_Instance_Number = object_id;
}

BACNET_DEVICE_STATUS Device_System_Status(void)
{
  return System_Status;
}

void Device_Set_System_Status(BACNET_DEVICE_STATUS status)
{
  // FIXME: bounds check?
  System_Status = status;
}

const char *Device_Vendor_Name(void)
{
  return Vendor_Name;
}

void Device_Set_Vendor_Name(const char *name)
{
  Vendor_Name = name;
}

uint16_t Device_Vendor_Identifier(void)
{
  return Vendor_Identifier;
}

void Device_Set_Vendor_Identifier(uint16_t vendor_id)
{
  Vendor_Identifier = vendor_id;
}

const char *Device_Model_Name(void)
{
  return Model_Name;
}

void Device_Set_Model_Name(const char *name)
{
  Model_Name = name;
}

const char *Device_Firmware_Revision(void)
{
  return Firmware_Revision;
}

void Device_Set_Firmware_Revision(const char *name)
{
  Firmware_Revision = name;
}

const char *Device_Application_Software_Version(void)
{
  return Application_Software_Version;
}

void Device_Set_Application_Software_Version(const char *name)
{
  Application_Software_Version = name;
}

const char *Device_Description(void)
{
  return Description;
}

void Device_Set_Description(const char *name)
{
  Description = name;
}

uint8_t Device_Protocol_Version(void)
{
  return 1;
}

uint8_t Device_Protocol_Revision(void)
{
  return 4;
}

uint16_t Device_Max_APDU_Length_Accepted(void)
{
  return MAX_APDU;
}

BACNET_SEGMENTATION Device_Segmentation_Supported(void)
{
  return SEGMENTATION_NONE;
}

//FIXME: Probably return one at a time, or be supported by
// an object module, or encode the APDU here.
//Object_List


uint16_t Device_APDU_Timeout(void)
{
  return APDU_Timeout;
}

// in milliseconds
void Device_Set_APDU_Timeout(uint16_t timeout)
{
  APDU_Timeout = timeout;
}

uint8_t Device_Number_Of_APDU_Retries(void)
{
  return Number_Of_APDU_Retries;
}

void Device_Set_Number_Of_APDU_Retries(uint8_t retries)
{
  Number_Of_APDU_Retries = retries;
}

uint8_t Device_Database_Revision(void)
{
  return Database_Revision;
}

void Device_Set_Database_Revision(uint8_t revision)
{
  Database_Revision = revision;
}

int Device_Encode_Property_APDU(
  uint8_t *apdu,
  BACNET_PROPERTY_ID property,
  int32_t array_index)
{
  int apdu_len = 0; // return value
  int len = 0; // apdu len intermediate value
  BACNET_BIT_STRING bit_string;
  int i = 0;
  
  switch (property)
  {
    case PROP_OBJECT_IDENTIFIER:
      apdu_len = encode_tagged_object_id(&apdu[0], OBJECT_DEVICE,
        Object_Instance_Number);
      break;
    case PROP_OBJECT_NAME:
      apdu_len = encode_tagged_character_string(&apdu[0], Object_Name);
      break;
    case PROP_OBJECT_TYPE:
      apdu_len = encode_tagged_enumerated(&apdu[0], OBJECT_DEVICE);
      break;
    case PROP_DESCRIPTION:
      apdu_len = encode_tagged_character_string(&apdu[0], Description);
      break;
    case PROP_SYSTEM_STATUS:
      apdu_len = encode_tagged_enumerated(&apdu[0], System_Status);
      break;
    case PROP_VENDOR_NAME:
      apdu_len = encode_tagged_character_string(&apdu[0], Vendor_Name);
      break;
    case PROP_VENDOR_IDENTIFIER:
      apdu_len = encode_tagged_unsigned(&apdu[0], Vendor_Identifier);
      break;
    case PROP_MODEL_NAME:
      apdu_len = encode_tagged_character_string(&apdu[0], Model_Name);
      break;
    case PROP_FIRMWARE_REVISION:
      apdu_len = encode_tagged_character_string(&apdu[0], Firmware_Revision);
      break;
    case PROP_APPLICATION_SOFTWARE_VERSION:
      apdu_len = encode_tagged_character_string(&apdu[0],
        Application_Software_Version);
      break;
    // if you support time
    case PROP_LOCAL_TIME:
        //t = time(NULL);
        //my_tm = localtime(&t);
        //apdu_len =
        //    encode_tagged_time(&apdu[0], my_tm->tm_hour, my_tm->tm_min,
        //    my_tm->tm_sec, 0);
        break;
    // if you support date
    case PROP_LOCAL_DATE:
        //t = time(NULL);
        //my_tm = localtime(&t);
        // year = years since 1900
        // month 1=Jan
        // day = day of month
        // wday 1=Monday...7=Sunday
        //apdu_len = encode_tagged_date(&apdu[0],
        //    my_tm->tm_year,
        //    my_tm->tm_mon + 1,
        //    my_tm->tm_mday, ((my_tm->tm_wday == 0) ? 7 : my_tm->tm_wday));
        break;
    case PROP_PROTOCOL_VERSION:
        apdu_len = encode_tagged_unsigned(&apdu[0], Device_Protocol_Version());
        break;
    // BACnet Legacy Support
    case PROP_PROTOCOL_CONFORMANCE_CLASS:
        apdu_len = encode_tagged_unsigned(&apdu[0], 1);
        break;
    case PROP_PROTOCOL_SERVICES_SUPPORTED:
        bitstring_init(&bit_string);
        for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++)
        {
          // bitstring_set_bit(&bit_string, i, apdu_service_supported(i));
          // initialize all the services to not-supported
          bitstring_set_bit(&bit_string, i, false);
        }
        // initialize those we support
        bitstring_set_bit(&bit_string, SERVICE_SUPPORTED_WHO_IS, true);
        bitstring_set_bit(&bit_string, SERVICE_SUPPORTED_I_AM, true);
        bitstring_set_bit(&bit_string, SERVICE_SUPPORTED_READ_PROPERTY, true);
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
        bitstring_init(&bit_string);
        for (i = 0; i < MAX_BACNET_OBJECT_TYPES; i++)
        {
          // initialize all the object types to not-supported
          bitstring_set_bit(&bit_string, i, false);
        }
        bitstring_set_bit(&bit_string, OBJECT_DEVICE, true);
        bitstring_set_bit(&bit_string, OBJECT_ANALOG_INPUT, true);
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_OBJECT_LIST:
      // FIXME: hook into real object list, not just device
      // Array element zero is the number of objects in the list
      if (array_index == 0)
        apdu_len = encode_tagged_unsigned(&apdu[0], 1 + MAX_ANALOG_INPUTS);
      // if no index was specified, then try to encode the entire list
      // into one packet.  Note that more than likely you will have
      // to return an error if the number of encoded objects exceeds
      // your maximum APDU size.
      else if (array_index == BACNET_ARRAY_ALL)
      {
        len = encode_tagged_object_id(&apdu[0], OBJECT_DEVICE,
            Object_Instance_Number);
        apdu_len = len;
        for (i = 0; i < MAX_ANALOG_INPUTS; i++)
        {
          // assume next one is the same size as this one
          // can we all fit into the APDU?
          if ((apdu_len + len) >= MAX_APDU)
          {
            apdu_len = 0;
            break;
          }
          len = encode_tagged_object_id(&apdu[apdu_len], 
            OBJECT_ANALOG_INPUT, i);
          apdu_len += len;
        }
      }
      else
      {
        // the first object in the list is at index=1
        if (array_index == 1)
          apdu_len = encode_tagged_object_id(&apdu[0], OBJECT_DEVICE,
              Object_Instance_Number);
        else if (array_index < (2 + MAX_ANALOG_INPUTS))
        {
          apdu_len = encode_tagged_object_id(&apdu[0], 
            OBJECT_ANALOG_INPUT, array_index - 2);
        }
        else
        {
          // FIXME: handle case where index is beyond our bounds
        }
      }
      break;
    case PROP_MAX_APDU_LENGTH_ACCEPTED:
      apdu_len = encode_tagged_unsigned(&apdu[0],
        Device_Max_APDU_Length_Accepted());
      break;
    case PROP_SEGMENTATION_SUPPORTED:
      apdu_len = encode_tagged_enumerated(&apdu[0],
        Device_Segmentation_Supported());
      break;
    case PROP_APDU_TIMEOUT:
      apdu_len = encode_tagged_unsigned(&apdu[0], APDU_Timeout);
      break;
    case PROP_NUMBER_OF_APDU_RETRIES:
      apdu_len = encode_tagged_unsigned(&apdu[0], Number_Of_APDU_Retries);
      break;
    default:
        break;
  }

  return apdu_len;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testDevice(Test * pTest)
{
  Device_Set_Object_Instance_Number(111);
  ct_test(pTest, Device_Object_Instance_Number() == 111);

  Device_Set_System_Status(STATUS_NON_OPERATIONAL);
  ct_test(pTest, Device_System_Status() == STATUS_NON_OPERATIONAL);

  Device_Set_Vendor_Name("MyName");
  ct_test(pTest, strcmp(Device_Vendor_Name(),"MyName") == 0);

  Device_Set_Vendor_Identifier(42);
  ct_test(pTest, Device_Vendor_Identifier() == 42);

  Device_Set_Model_Name("MyModel");
  ct_test(pTest, strcmp(Device_Model_Name(),"MyModel") == 0);

  return;
}

#ifdef TEST_DEVICE
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Device", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testDevice);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_DEVICE */
#endif                          /* TEST */


