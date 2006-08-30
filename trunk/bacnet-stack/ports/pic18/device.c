/**************************************************************************
*
* Copyright (C) 2005,2006 Steve Karg <skarg@users.sourceforge.net>
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
#include <string.h>             /* for memmove */
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "config.h"             /* the custom stuff */
#include "apdu.h"
#include "device.h"             /* me */

/* note: you really only need to define variables for 
   properties that are writable or that may change. 
   The properties that are constant can be hard coded
   into the read-property encoding. */
static uint32_t Object_Instance_Number = 0;
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static uint16_t APDU_Timeout = 3000;
static uint8_t Number_Of_APDU_Retries = 3;

/* methods to manipulate the data */
uint32_t Device_Object_Instance_Number(void)
{
    return Object_Instance_Number;
}

bool Device_Set_Object_Instance_Number(uint32_t object_id)
{
    bool status = true;         /* return value */

    if (object_id <= BACNET_MAX_INSTANCE)
        Object_Instance_Number = object_id;
    else
        status = false;

    return status;
}

bool Device_Valid_Object_Instance_Number(uint32_t object_id)
{
    /* BACnet allows for a wildcard instance number */
    return ((Object_Instance_Number == object_id) ||
        (object_id == BACNET_MAX_INSTANCE));
}

BACNET_DEVICE_STATUS Device_System_Status(void)
{
    return System_Status;
}

void Device_Set_System_Status(BACNET_DEVICE_STATUS status)
{
    /* FIXME: bounds check? */
    System_Status = status;
}

/* FIXME: put your vendor ID here! */
uint16_t Device_Vendor_Identifier(void)
{
    return 0;
}

uint8_t Device_Protocol_Version(void)
{
    return 1;
}

uint8_t Device_Protocol_Revision(void)
{
    return 5;
}

/* FIXME: MAX_APDU is defined in config.ini - set it! */
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

/* in milliseconds */
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
    return 0;
}

/* Since many network clients depend on the object list */
/* for discovery, it must be consistent! */
unsigned Device_Object_List_Count(void)
{
    unsigned count = 1;

    return count;
}

bool Device_Object_List_Identifier(unsigned array_index,
    int *object_type, uint32_t * instance)
{
    bool status = false;
    unsigned object_index = 0;
    unsigned object_count = 0;

    /* device object */
    if (array_index == 1) {
        *object_type = OBJECT_DEVICE;
        *instance = Object_Instance_Number;
        status = true;
    }

    return status;
}

/* return the length of the apdu encoded or -1 for error */
int Device_Encode_Property_APDU(uint8_t * apdu,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;           /* return value */
    int len = 0;                /* apdu len intermediate value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned i = 0;
    int object_type = 0;
    uint32_t instance = 0;
    unsigned count = 0;

    /* FIXME: change the hardcoded names to suit your application */
    switch (property) {
    case PROP_OBJECT_IDENTIFIER:
        apdu_len = encode_tagged_object_id(&apdu[0], OBJECT_DEVICE,
            Object_Instance_Number);
        break;
    case PROP_OBJECT_NAME:
        characterstring_init_ansi(&char_string, (char *)"TD");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_OBJECT_TYPE:
        apdu_len = encode_tagged_enumerated(&apdu[0], OBJECT_DEVICE);
        break;
    case PROP_DESCRIPTION:
        characterstring_init_ansi(&char_string, (char *)"Tiny");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_SYSTEM_STATUS:
        apdu_len = encode_tagged_enumerated(&apdu[0], Device_System_Status());
        break;
    case PROP_VENDOR_NAME:
        characterstring_init_ansi(&char_string, (char *)"ASHRAE");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_VENDOR_IDENTIFIER:
        apdu_len = encode_tagged_unsigned(&apdu[0], Device_Vendor_Identifier());
        break;
    case PROP_MODEL_NAME:
        characterstring_init_ansi(&char_string, (char *)"GNU");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_FIRMWARE_REVISION:
        characterstring_init_ansi(&char_string, (char *)"1.0");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_APPLICATION_SOFTWARE_VERSION:
        characterstring_init_ansi(&char_string, (char *)"1.0");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_PROTOCOL_VERSION:
        apdu_len =
            encode_tagged_unsigned(&apdu[0], Device_Protocol_Version());
        break;
    case PROP_PROTOCOL_REVISION:
        apdu_len =
            encode_tagged_unsigned(&apdu[0], Device_Protocol_Revision());
        break;
        /* BACnet Legacy Support */
    case PROP_PROTOCOL_CONFORMANCE_CLASS:
        apdu_len = encode_tagged_unsigned(&apdu[0], 1);
        break;
    case PROP_PROTOCOL_SERVICES_SUPPORTED:
        /* Note: list of services that are executed, not initiated. */
        bitstring_init(&bit_string);
        for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++) {
            /* automatic lookup based on handlers set */
            bitstring_set_bit(&bit_string, (uint8_t) i,
                apdu_service_supported(i));
        }
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
        /* Note: this is the list of objects that can be in this device,
           not a list of objects that this device can access */
        bitstring_init(&bit_string);
        for (i = 0; i < MAX_ASHRAE_OBJECT_TYPE; i++) {
            /* initialize all the object types to not-supported */
            bitstring_set_bit(&bit_string, (uint8_t) i, false);
        }
        /* FIXME: indicate the objects that YOU support */
        bitstring_set_bit(&bit_string, OBJECT_DEVICE, true);
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_OBJECT_LIST:
        count = Device_Object_List_Count();
        /* Array element zero is the number of objects in the list */
        if (array_index == BACNET_ARRAY_LENGTH_INDEX)
            apdu_len = encode_tagged_unsigned(&apdu[0], count);
        /* if no index was specified, then try to encode the entire list */
        /* into one packet.  Note that more than likely you will have */
        /* to return an error if the number of encoded objects exceeds */
        /* your maximum APDU size. */
        else if (array_index == BACNET_ARRAY_ALL) {
            for (i = 1; i <= count; i++) {
                if (Device_Object_List_Identifier(i, &object_type,
                        &instance)) {
                    len =
                        encode_tagged_object_id(&apdu[apdu_len],
                        object_type, instance);
                    apdu_len += len;
                    /* assume next one is the same size as this one */
                    /* can we all fit into the APDU? */
                    if ((apdu_len + len) >= MAX_APDU) {
                        *error_class = ERROR_CLASS_SERVICES;
                        *error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
                        apdu_len = -1;
                        break;
                    }
                } else {
                    /* error: internal error? */
                    *error_class = ERROR_CLASS_SERVICES;
                    *error_code = ERROR_CODE_OTHER;
                    apdu_len = -1;
                    break;
                }
            }
        } else {
            if (Device_Object_List_Identifier(array_index, &object_type,
                    &instance))
                apdu_len =
                    encode_tagged_object_id(&apdu[0], object_type,
                    instance);
            else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = -1;
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
        apdu_len =
            encode_tagged_unsigned(&apdu[0], Device_Number_Of_APDU_Retries());
        break;
    case PROP_DEVICE_ADDRESS_BINDING:
        /* FIXME: encode the list here, if it exists */
        break;
    case PROP_DATABASE_REVISION:
        apdu_len = encode_tagged_unsigned(&apdu[0], Device_Database_Revision());
        break;
    default:
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
        apdu_len = -1;
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
    bool status = false;
    const char *name = "Patricia";

    status = Device_Set_Object_Instance_Number(0);
    ct_test(pTest, Device_Object_Instance_Number() == 0);
    ct_test(pTest, status == true);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    ct_test(pTest, Device_Object_Instance_Number() == BACNET_MAX_INSTANCE);
    ct_test(pTest, status == true);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE / 2);
    ct_test(pTest,
        Device_Object_Instance_Number() == (BACNET_MAX_INSTANCE / 2));
    ct_test(pTest, status == true);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE + 1);
    ct_test(pTest,
        Device_Object_Instance_Number() != (BACNET_MAX_INSTANCE + 1));
    ct_test(pTest, status == false);

    return;
}

#ifdef TEST_DEVICE
/* stubs to dependencies to keep unit test simple */
char *Analog_Input_Name(uint32_t object_instance)
{
    (void) object_instance;
    return "";
}

unsigned Analog_Input_Count(void)
{
    return 0;
}

uint32_t Analog_Input_Index_To_Instance(unsigned index)
{
    return index;
}

char *Analog_Output_Name(uint32_t object_instance)
{
    (void) object_instance;
    return "";
}

unsigned Analog_Output_Count(void)
{
    return 0;
}

uint32_t Analog_Output_Index_To_Instance(unsigned index)
{
    return index;
}

int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Tiny Device", NULL);
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
