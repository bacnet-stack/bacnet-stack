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
#include <string.h> /* for memmove*/
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "config.h" // the custom stuff
#include "ai.h" // object list dependency
#include "ao.h" // object list dependency
#include "wp.h" // write property handling
#if BACFILE
#include "bacfile.h" // object list dependency
#endif

static uint32_t Object_Instance_Number = 0;
static char Object_Name[16] = "SimpleServer";
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static char Vendor_Name[16] = "ASHRAE";
// vendor id assigned by ASHRAE
static uint16_t Vendor_Identifier = 0;
static char Model_Name[16] = "GNU";
static char Firmware_Revision[16] = "1.0";
static char Application_Software_Version[16] = "1.0";
static char Location[16] = "USA";
static char Description[16] = "server";
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

bool Device_Set_Object_Instance_Number(uint32_t object_id)
{
  bool status = true; /* return value */
  
  if (object_id <= BACNET_MAX_INSTANCE)
    Object_Instance_Number = object_id;
  else
    status = false;

  return status;
}

bool Device_Valid_Object_Instance_Number(uint32_t object_id)
{
  // BACnet allows for a wildcard instance number
  return ((Object_Instance_Number == object_id) || 
    (object_id == BACNET_MAX_INSTANCE));
}

const char *Device_Object_Name(void)
{
  return Object_Name;
}

bool Device_Set_Object_Name(const char *name, size_t length)
{
  bool status = false; /*return value*/
  
  if (length < sizeof(Object_Name))
  {
    memmove(Object_Name,name,length);
    Object_Name[length] = 0;
    status = true;
  }

  return status;
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

bool Device_Set_Vendor_Name(const char *name, size_t length)
{
  bool status = false; /*return value*/
  
  if (length < sizeof(Vendor_Name))
  {
    memmove(Vendor_Name,name,length);
    Vendor_Name[length] = 0;
    status = true;
  }

  return status;
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

bool Device_Set_Model_Name(const char *name, size_t length)
{
  bool status = false; /*return value*/
  
  if (length < sizeof(Model_Name))
  {
    memmove(Model_Name,name,length);
    Model_Name[length] = 0;
    status = true;
  }

  return status;
}

const char *Device_Firmware_Revision(void)
{
  return Firmware_Revision;
}

bool Device_Set_Firmware_Revision(const char *name, size_t length)
{
  bool status = false; /*return value*/
  
  if (length < sizeof(Firmware_Revision))
  {
    memmove(Firmware_Revision,name,length);
    Firmware_Revision[length] = 0;
    status = true;
  }

  return status;
}

const char *Device_Application_Software_Version(void)
{
  return Application_Software_Version;
}

bool Device_Set_Application_Software_Version(const char *name, size_t length)
{
  bool status = false; /*return value*/
  
  if (length < sizeof(Application_Software_Version))
  {
    memmove(Application_Software_Version,name,length);
    Application_Software_Version[length] = 0;
    status = true;
  }

  return status;
}

const char *Device_Description(void)
{
  return Description;
}

bool Device_Set_Description(const char *name, size_t length)
{
  bool status = false; /*return value*/
  
  if (length < sizeof(Description))
  {
    memmove(Description,name,length);
    Description[length] = 0;
    status = true;
  }

  return status;
}

const char *Device_Location(void)
{
  return Location;
}

bool Device_Set_Location(const char *name, size_t length)
{
  bool status = false; /*return value*/
  
  if (length < sizeof(Location))
  {
    memmove(Location,name,length);
    Location[length] = 0;
    status = true;
  }

  return status;
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

// Since many network clients depend on the object list
// for discovery, it must be consistent!
unsigned Device_Object_List_Count(void)
{
  unsigned count = 1;

  count += Analog_Input_Count();
  count += Analog_Output_Count();
  #if BACFILE
  count += bacfile_count();
  #endif

  return count;
}

bool Device_Object_List_Identifier(unsigned array_index,
  int *object_type, 
  uint32_t *instance)
{
  bool status = false;
  unsigned object_index = 0;
  
  if (array_index == 1)
  {
    *object_type = OBJECT_DEVICE; 
    *instance = Object_Instance_Number;
    status = true;
  }

  if (!status)
  {
    // array index starts at 1, and 1 for the device object
    object_index = array_index - 2;
    if (object_index < Analog_Input_Count())
    {
      *object_type = OBJECT_ANALOG_INPUT; 
      *instance = Analog_Input_Index_To_Instance(object_index);
      status = true;
    }
  }
  if (!status)
  {
    object_index -= Analog_Input_Count();
    if (object_index < Analog_Output_Count())
    {
      *object_type = OBJECT_ANALOG_OUTPUT;
      *instance = Analog_Output_Index_To_Instance(object_index);
      status = true;
    }
  }

  #if BACFILE
  if (!status)
  {
    object_index -= Analog_Output_Count();
    if (object_index < bacfile_count())
    {
      *object_type = OBJECT_FILE;
      *instance = bacfile_index_to_instance(object_index);
      status = true;
    }
  }
  #endif
    
  return status;  
}

// return the length of the apdu encoded
int Device_Encode_Property_APDU(
  uint8_t *apdu,
  BACNET_PROPERTY_ID property,
  int32_t array_index,
  BACNET_ERROR_CLASS *error_class,
  BACNET_ERROR_CODE *error_code)
{
  int apdu_len = 0; // return value
  int len = 0; // apdu len intermediate value
  BACNET_BIT_STRING bit_string;
  BACNET_CHARACTER_STRING char_string;
  unsigned i = 0;
  int object_type = 0;
  uint32_t instance = 0;
  unsigned count = 0;

  switch (property)
  {
    case PROP_OBJECT_IDENTIFIER:
      apdu_len = encode_tagged_object_id(&apdu[0], OBJECT_DEVICE,
        Object_Instance_Number);
      break;
    case PROP_OBJECT_NAME:
      characterstring_init_ansi(&char_string, Object_Name);
      apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
      break;
    case PROP_OBJECT_TYPE:
      apdu_len = encode_tagged_enumerated(&apdu[0], OBJECT_DEVICE);
      break;
    case PROP_DESCRIPTION:
      characterstring_init_ansi(&char_string, Description);
      apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
      break;
    case PROP_SYSTEM_STATUS:
      apdu_len = encode_tagged_enumerated(&apdu[0], System_Status);
      break;
    case PROP_VENDOR_NAME:
      characterstring_init_ansi(&char_string, Vendor_Name);
      apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
      break;
    case PROP_VENDOR_IDENTIFIER:
      apdu_len = encode_tagged_unsigned(&apdu[0], Vendor_Identifier);
      break;
    case PROP_MODEL_NAME:
      characterstring_init_ansi(&char_string, Model_Name);
      apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
      break;
    case PROP_FIRMWARE_REVISION:
      characterstring_init_ansi(&char_string, Firmware_Revision);
      apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
      break;
    case PROP_APPLICATION_SOFTWARE_VERSION:
      characterstring_init_ansi(&char_string, Application_Software_Version);
      apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
      break;
    // if you support time
    //case PROP_LOCAL_TIME:
      //t = time(NULL);
      //my_tm = localtime(&t);
      //apdu_len =
      //    encode_tagged_time(&apdu[0], my_tm->tm_hour, my_tm->tm_min,
      //    my_tm->tm_sec, 0);
      //break;
    // if you support date
    //case PROP_LOCAL_DATE:
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
      //break;
    case PROP_PROTOCOL_VERSION:
        apdu_len = encode_tagged_unsigned(&apdu[0], Device_Protocol_Version());
        break;
    // BACnet Legacy Support
    case PROP_PROTOCOL_CONFORMANCE_CLASS:
        apdu_len = encode_tagged_unsigned(&apdu[0], 1);
        break;
    case PROP_PROTOCOL_SERVICES_SUPPORTED:
        /* Note: list of services that are executed, not initiated. */
        bitstring_init(&bit_string);
        for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++)
        {
          /* automatic lookup based on handlers set */
          bitstring_set_bit(&bit_string, (uint8_t)i, apdu_service_supported(i));
        }
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
        /* Note: this is the list of objects that can be in this device,
           not a list of objects that this device can access */
        bitstring_init(&bit_string);
        for (i = 0; i < MAX_ASHRAE_OBJECT_TYPE; i++)
        {
          // initialize all the object types to not-supported
          bitstring_set_bit(&bit_string, (uint8_t)i, false);
        }
        /* FIXME: indicate the objects that YOU support */
        bitstring_set_bit(&bit_string, OBJECT_DEVICE, true);
        bitstring_set_bit(&bit_string, OBJECT_ANALOG_INPUT, true);
        bitstring_set_bit(&bit_string, OBJECT_ANALOG_OUTPUT, true);
        #if BACFILE
        bitstring_set_bit(&bit_string, OBJECT_FILE, true);
        #endif
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_OBJECT_LIST:
      count = Device_Object_List_Count();
      // Array element zero is the number of objects in the list
      if (array_index == BACNET_ARRAY_LENGTH_INDEX)
        apdu_len = encode_tagged_unsigned(&apdu[0], count);
      // if no index was specified, then try to encode the entire list
      // into one packet.  Note that more than likely you will have
      // to return an error if the number of encoded objects exceeds
      // your maximum APDU size.
      else if (array_index == BACNET_ARRAY_ALL)
      {
        for (i = 1; i <= count; i++)
        {
          if (Device_Object_List_Identifier(i,&object_type,&instance))
          {
            len = encode_tagged_object_id(&apdu[apdu_len], object_type,
              instance);
            apdu_len += len;
            // assume next one is the same size as this one
            // can we all fit into the APDU?
            if ((apdu_len + len) >= MAX_APDU)
            {
              *error_class = ERROR_CLASS_SERVICES;
              *error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
              apdu_len = 0;
              break;
            }
          }
          else
          {
            // error: internal error?
            *error_class = ERROR_CLASS_SERVICES;
            *error_code = ERROR_CODE_OTHER;
            apdu_len = 0;
            break;
          }
        }
      }
      else
      {
        if (Device_Object_List_Identifier(array_index,&object_type,&instance))
          apdu_len = encode_tagged_object_id(&apdu[0], object_type, instance);
        else
        {
          *error_class = ERROR_CLASS_PROPERTY;
          *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
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
      *error_class = ERROR_CLASS_PROPERTY;
      *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
      break;
  }

  return apdu_len;
}

// we can send an I-Am when our device ID changes
extern bool I_Am_Request;

// returns true if successful
bool Device_Write_Property(
  BACNET_WRITE_PROPERTY_DATA *wp_data,
  BACNET_ERROR_CLASS *error_class,
  BACNET_ERROR_CODE *error_code)
{
  bool status = false; // return value
  
  if (!Device_Valid_Object_Instance_Number(wp_data->object_instance))
  {
    *error_class = ERROR_CLASS_OBJECT;
    *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    return false;
  }

  // decode the some of the request
  switch (wp_data->object_property)
  {
    case PROP_OBJECT_IDENTIFIER:
      if (wp_data->value.tag == BACNET_APPLICATION_TAG_OBJECT_ID)
      { 
        if ((wp_data->value.type.Object_Id.type == OBJECT_DEVICE) &&
          (Device_Set_Object_Instance_Number(
             wp_data->value.type.Object_Id.instance)))
        {
          I_Am_Request = true;
          status = true;
        }
        else
        {
          *error_class = ERROR_CLASS_PROPERTY;
          *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
      }
      else
      {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_INVALID_DATA_TYPE;
      }
      break;
      
    case PROP_NUMBER_OF_APDU_RETRIES:
      if (wp_data->value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT)
      {
		  /* FIXME: bounds check? */
       Device_Set_Number_Of_APDU_Retries((uint8_t)wp_data->value.type.Unsigned_Int);
       status = true;
      }
      else
      {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_INVALID_DATA_TYPE;
      }
      break;      
      
    case PROP_APDU_TIMEOUT:
      if (wp_data->value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT)
      {
		  /* FIXME: bounds check? */
       Device_Set_APDU_Timeout((uint16_t)wp_data->value.type.Unsigned_Int);
       status = true;
      }
      else
      {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_INVALID_DATA_TYPE;
      }
      break;
            
    case PROP_VENDOR_IDENTIFIER:
      if (wp_data->value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT)
      {
		  /* FIXME: bounds check? */
       Device_Set_Vendor_Identifier((uint16_t)wp_data->value.type.Unsigned_Int);
       status = true;
      }
      else
      {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_INVALID_DATA_TYPE;
      }
      break;
    
    case PROP_SYSTEM_STATUS:
      if (wp_data->value.tag == BACNET_APPLICATION_TAG_ENUMERATED)
      {
       Device_Set_System_Status(wp_data->value.type.Enumerated);
       status = true;
      }
      else
      {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_INVALID_DATA_TYPE;
      }
      break;

    case PROP_OBJECT_NAME:
      if (wp_data->value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING)
      {
        uint8_t encoding;
        encoding = characterstring_encoding(&wp_data->value.type.Character_String);
        if (encoding == CHARACTER_ANSI_X34)
        {
          status = Device_Set_Object_Name(
            characterstring_value(&wp_data->value.type.Character_String),
            characterstring_length(&wp_data->value.type.Character_String));
          if (!status)
          {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
          }
        }
        else
        {
          *error_class = ERROR_CLASS_PROPERTY;
          *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
        }
      }
      else
      {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_INVALID_DATA_TYPE;
      }
      break;

    default:
      *error_class = ERROR_CLASS_PROPERTY;
      *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
      break;
  }

  return status;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testDevice(Test * pTest)
{
  bool status = false;
  const char *name = "Patricia";
  
  status = Device_Set_Object_Instance_Number(0);
  ct_test(pTest, Device_Object_Instance_Number() == 0);
  ct_test(pTest, status == true);
  status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
  ct_test(pTest, Device_Object_Instance_Number() == BACNET_MAX_INSTANCE);
  ct_test(pTest, status == true);
  status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE/2);
  ct_test(pTest, Device_Object_Instance_Number() == (BACNET_MAX_INSTANCE/2));
  ct_test(pTest, status == true);
  status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE+1);
  ct_test(pTest, Device_Object_Instance_Number() != (BACNET_MAX_INSTANCE+1));
  ct_test(pTest, status == false);
  
  
  Device_Set_System_Status(STATUS_NON_OPERATIONAL);
  ct_test(pTest, Device_System_Status() == STATUS_NON_OPERATIONAL);

  Device_Set_Vendor_Name(name,strlen(name));
  ct_test(pTest, strcmp(Device_Vendor_Name(),name) == 0);

  Device_Set_Vendor_Identifier(42);
  ct_test(pTest, Device_Vendor_Identifier() == 42);

  Device_Set_Model_Name(name,strlen(name));
  ct_test(pTest, strcmp(Device_Model_Name(),name) == 0);

  return;
}

#ifdef TEST_DEVICE
// stubs to dependencies to keep unit test simple
unsigned Analog_Input_Count(void)
{
  return 0;
}

uint32_t Analog_Input_Index_To_Instance(unsigned index)
{
  return index;
}

unsigned Analog_Output_Count(void)
{
  return 0;
}

uint32_t Analog_Output_Index_To_Instance(unsigned index)
{
  return index;
}

uint32_t bacfile_count(void)
{
  return 0;
}

uint32_t bacfile_index_to_instance(unsigned find_index)
{
  return find_index;
}

bool I_Am_Request = false;

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


