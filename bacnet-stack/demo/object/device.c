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
#include "bacapp.h"
#include "config.h"             /* the custom stuff */
#include "apdu.h"
#include "ai.h"                 /* object list dependency */
#include "ao.h"                 /* object list dependency */
#include "av.h"                 /* object list dependency */
#include "bi.h"                 /* object list dependency */
#include "bo.h"                 /* object list dependency */
#include "bv.h"                 /* object list dependency */
#include "lc.h"                 /* object list dependency */
#include "lsp.h"                /* object list dependency */
#include "mso.h"                /* object list dependency */
#include "wp.h"                 /* write property handling */
#include "device.h"             /* me */
#if BACFILE
#include "bacfile.h"            /* object list dependency */
#endif

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Device_Properties_Required[] =
{
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_SYSTEM_STATUS,
    PROP_VENDOR_NAME,
    PROP_VENDOR_IDENTIFIER,
    PROP_MODEL_NAME,
    PROP_FIRMWARE_REVISION,
    PROP_APPLICATION_SOFTWARE_VERSION,
    PROP_DESCRIPTION,
    PROP_PROTOCOL_VERSION,
    PROP_PROTOCOL_REVISION,
    PROP_PROTOCOL_SERVICES_SUPPORTED,
    PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
    PROP_OBJECT_LIST,
    PROP_MAX_APDU_LENGTH_ACCEPTED,
    PROP_SEGMENTATION_SUPPORTED,
    PROP_APDU_TIMEOUT,
    PROP_NUMBER_OF_APDU_RETRIES,
#if defined(BACDL_MSTP)
    PROP_MAX_MASTER,
    PROP_MAX_INFO_FRAMES,
#endif
    PROP_DEVICE_ADDRESS_BINDING,
    PROP_DATABASE_REVISION,
    -1
};

static const int Device_Properties_Optional[] =
{
    PROP_DESCRIPTION,
    PROP_LOCAL_TIME,
    PROP_UTC_OFFSET,
    PROP_LOCAL_DATE,
    PROP_DAYLIGHT_SAVINGS_STATUS,
    PROP_PROTOCOL_CONFORMANCE_CLASS,
    -1
};

static const int Device_Properties_Proprietary[] =
{
    -1
};

void Device_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Device_Properties_Required;
    if (pOptional)
        *pOptional = Device_Properties_Optional;
    if (pProprietary)
        *pProprietary = Device_Properties_Proprietary;

    return;
}

/* note: you really only need to define variables for
   properties that are writable or that may change.
   The properties that are constant can be hard coded
   into the read-property encoding. */
static uint32_t Object_Instance_Number = 0;
static char Object_Name[16] = "SimpleServer";
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static char Vendor_Name[16] = "ASHRAE";
/* FIXME: your vendor id assigned by ASHRAE */
static uint16_t Vendor_Identifier = 0;
static char Model_Name[16] = "GNU";
static char Firmware_Revision[16] = "0.3.2";
static char Application_Software_Version[16] = "1.0";
static char Location[16] = "USA";
static char Description[16] = "server";
/* static uint8_t Protocol_Version = 1; - constant, not settable */
/* static uint8_t Protocol_Revision = 4; - constant, not settable */
/* Protocol_Services_Supported - dynamically generated */
/* Protocol_Object_Types_Supported - in RP encoding */
/* Object_List - dynamically generated */
/* static uint16_t Max_APDU_Length_Accepted = MAX_APDU; - constant */
/* static BACNET_SEGMENTATION Segmentation_Supported = SEGMENTATION_NONE; */
/* static uint8_t Max_Segments_Accepted = 0; */
/* VT_Classes_Supported */
/* Active_VT_Sessions */
BACNET_TIME Local_Time;         /* rely on OS, if there is one */
BACNET_DATE Local_Date;         /* rely on OS, if there is one */
/* BACnet UTC is inverse of standard offset - i.e. relative to local */
static int UTC_Offset = 5;
static bool Daylight_Savings_Status = false;    /* rely on OS */
/* APDU_Segment_Timeout */
static uint16_t APDU_Timeout = 3000;
static uint8_t Number_Of_APDU_Retries = 3;
/* List_Of_Session_Keys */
/* Time_Synchronization_Recipients */
/* Max_Master - rely on MS/TP subsystem, if there is one */
/* Max_Info_Frames - rely on MS/TP subsystem, if there is one */
/* Device_Address_Binding - required, but relies on binding cache */
static uint8_t Database_Revision = 0;
/* Configuration_Files */
/* Last_Restore_Time */
/* Backup_Failure_Timeout */
/* Active_COV_Subscriptions */
/* Slave_Proxy_Enable */
/* Manual_Slave_Address_Binding */
/* Auto_Slave_Discovery */
/* Slave_Address_Binding */
/* Profile_Name */

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

const char *Device_Object_Name(void)
{
    return Object_Name;
}

bool Device_Set_Object_Name(const char *name, size_t length)
{
    bool status = false;        /*return value */

    /* FIXME:  All the object names in a device must be unique.
       Disallow setting the Device Object Name to any objects in
       the device. */
    if (length < sizeof(Object_Name)) {
        memmove(Object_Name, name, length);
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
    /* FIXME: bounds check? */
    System_Status = status;
}

const char *Device_Vendor_Name(void)
{
    return Vendor_Name;
}

bool Device_Set_Vendor_Name(const char *name, size_t length)
{
    bool status = false;        /*return value */

    if (length < sizeof(Vendor_Name)) {
        memmove(Vendor_Name, name, length);
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
    bool status = false;        /*return value */

    if (length < sizeof(Model_Name)) {
        memmove(Model_Name, name, length);
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
    bool status = false;        /*return value */

    if (length < sizeof(Firmware_Revision)) {
        memmove(Firmware_Revision, name, length);
        Firmware_Revision[length] = 0;
        status = true;
    }

    return status;
}

const char *Device_Application_Software_Version(void)
{
    return Application_Software_Version;
}

bool Device_Set_Application_Software_Version(const char *name,
    size_t length)
{
    bool status = false;        /*return value */

    if (length < sizeof(Application_Software_Version)) {
        memmove(Application_Software_Version, name, length);
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
    bool status = false;        /*return value */

    if (length < sizeof(Description)) {
        memmove(Description, name, length);
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
    bool status = false;        /*return value */

    if (length < sizeof(Location)) {
        memmove(Location, name, length);
        Location[length] = 0;
        status = true;
    }

    return status;
}

uint8_t Device_Protocol_Version(void)
{
    return BACNET_PROTOCOL_VERSION;
}

uint8_t Device_Protocol_Revision(void)
{
    return BACNET_PROTOCOL_REVISION;
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
    return Database_Revision;
}

void Device_Set_Database_Revision(uint8_t revision)
{
    Database_Revision = revision;
}

/* Since many network clients depend on the object list */
/* for discovery, it must be consistent! */
unsigned Device_Object_List_Count(void)
{
    unsigned count = 1;

    count += Analog_Input_Count();
    count += Analog_Output_Count();
    count += Analog_Value_Count();
    count += Binary_Input_Count();
    count += Binary_Output_Count();
    count += Binary_Value_Count();
    count += Life_Safety_Point_Count();
    count += Load_Control_Count();
    count += Multistate_Output_Count();
#if BACFILE
    count += bacfile_count();
#endif

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
    /* analog input objects */
    if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index = array_index - 2;
        object_count = Analog_Input_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_ANALOG_INPUT;
            *instance = Analog_Input_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* analog output objects */
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = Analog_Output_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_ANALOG_OUTPUT;
            *instance = Analog_Output_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* analog value objects */
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = Analog_Value_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_ANALOG_VALUE;
            *instance = Analog_Value_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* binary input objects */
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = Binary_Input_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_BINARY_INPUT;
            *instance = Binary_Input_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* binary output objects */
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = Binary_Output_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_BINARY_OUTPUT;
            *instance = Binary_Output_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* binary value objects */
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = Binary_Value_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_BINARY_VALUE;
            *instance = Binary_Value_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* life safety point objects */
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = Life_Safety_Point_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_LIFE_SAFETY_POINT;
            *instance = Life_Safety_Point_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* load control objects */
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = Load_Control_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_LOAD_CONTROL;
            *instance = Load_Control_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* multi-state output objects */
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = Multistate_Output_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_MULTI_STATE_OUTPUT;
            *instance = Multistate_Output_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* file objects */
#if BACFILE
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = bacfile_count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_FILE;
            *instance = bacfile_index_to_instance(object_index);
            status = true;
        }
    }
#endif

    return status;
}

bool Device_Valid_Object_Name(const char *object_name,
    int *object_type, uint32_t * object_instance)
{
    bool found = false;
    int type = 0;
    uint32_t instance;
    unsigned max_objects = 0, i = 0;
    bool check_id = false;
    char *name = NULL;

    max_objects = Device_Object_List_Count();
    for (i = 0; i < max_objects; i++) {
        check_id = Device_Object_List_Identifier(i, &type, &instance);
        if (check_id) {
            name = Device_Valid_Object_Id(type, instance);
            if (strcmp(name, object_name) == 0) {
                found = true;
                if (object_type)
                    *object_type = type;
                if (object_instance)
                    *object_instance = instance;
                break;
            }
        }
    }

    return found;
}

/* returns the name or NULL if not found */
char *Device_Valid_Object_Id(int object_type, uint32_t object_instance)
{
    char *name = NULL;          /* return value */

    switch (object_type) {
    case OBJECT_ANALOG_INPUT:
        name = Analog_Input_Name(object_instance);
        break;
    case OBJECT_ANALOG_OUTPUT:
        name = Analog_Output_Name(object_instance);
        break;
    case OBJECT_ANALOG_VALUE:
        name = Analog_Value_Name(object_instance);
        break;
    case OBJECT_BINARY_INPUT:
        name = Binary_Input_Name(object_instance);
        break;
    case OBJECT_BINARY_OUTPUT:
        name = Binary_Output_Name(object_instance);
        break;
    case OBJECT_BINARY_VALUE:
        name = Binary_Value_Name(object_instance);
        break;
    case OBJECT_LIFE_SAFETY_POINT:
        name = Life_Safety_Point_Name(object_instance);
        break;
    case OBJECT_LOAD_CONTROL:
        name = Load_Control_Name(object_instance);
        break;
    case OBJECT_MULTI_STATE_OUTPUT:
        name = Multistate_Output_Name(object_instance);
        break;
#if BACFILE
    case OBJECT_FILE:
        name = bacfile_name(object_instance);
        break;
#endif
    case OBJECT_DEVICE:
        if (object_instance == Object_Instance_Number)
            name = Object_Name;
        break;
    default:
        break;
    }

    return name;
}

/* return the length of the apdu encoded or -1 for error or
   -2 for abort message */
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

    switch (property) {
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
        characterstring_init_ansi(&char_string,
            Application_Software_Version);
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
        /* FIXME: if you support time */
    case PROP_LOCAL_TIME:
        /* FIXME: get the actual value */
        Local_Time.hour = 7;
        Local_Time.min = 0;
        Local_Time.sec = 3;
        Local_Time.hundredths = 1;
        apdu_len = encode_tagged_time(&apdu[0], &Local_Time);
        break;
        /* FIXME: if you support time */
    case PROP_UTC_OFFSET:
        /* NOTE: if your UTC offset is -5, then BACnet UTC offset is 5 */
        apdu_len = encode_tagged_signed(&apdu[0], UTC_Offset);
        break;
        /* FIXME: if you support date */
    case PROP_LOCAL_DATE:
        /* FIXME: get the actual value instead of April Fool's Day */
        Local_Date.year = 2006; /* AD */
        Local_Date.month = 4;   /* 1=Jan */
        Local_Date.day = 1;     /* 1..31 */
        Local_Date.wday = 6;    /* 1=Monday */
        apdu_len = encode_tagged_date(&apdu[0], &Local_Date);
        break;
    case PROP_DAYLIGHT_SAVINGS_STATUS:
        apdu_len =
            encode_tagged_boolean(&apdu[0], Daylight_Savings_Status);
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
                apdu_service_supported((BACNET_SERVICES_SUPPORTED)i));
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
        if (Analog_Input_Count())
            bitstring_set_bit(&bit_string, OBJECT_ANALOG_INPUT, true);
        if (Analog_Output_Count())
            bitstring_set_bit(&bit_string, OBJECT_ANALOG_OUTPUT, true);
        if (Analog_Value_Count())
            bitstring_set_bit(&bit_string, OBJECT_ANALOG_VALUE, true);
        if (Binary_Input_Count())
            bitstring_set_bit(&bit_string, OBJECT_BINARY_INPUT, true);
        if (Binary_Output_Count())
            bitstring_set_bit(&bit_string, OBJECT_BINARY_OUTPUT, true);
        if (Binary_Value_Count())
            bitstring_set_bit(&bit_string, OBJECT_BINARY_VALUE, true);
        if (Life_Safety_Point_Count())
            bitstring_set_bit(&bit_string, OBJECT_LIFE_SAFETY_POINT, true);
        if (Load_Control_Count())
            bitstring_set_bit(&bit_string, OBJECT_LOAD_CONTROL, true);
        if (Multistate_Output_Count())
            bitstring_set_bit(&bit_string, OBJECT_MULTI_STATE_OUTPUT,
                true);
#if BACFILE
        if (bacfile_count())
            bitstring_set_bit(&bit_string, OBJECT_FILE, true);
#endif
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_OBJECT_LIST:
        count = Device_Object_List_Count();
        /* Array element zero is the number of objects in the list */
        if (array_index == 0)
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
                        /* reject message */
                        apdu_len = -2;
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
            encode_tagged_unsigned(&apdu[0], Number_Of_APDU_Retries);
        break;
    case PROP_DEVICE_ADDRESS_BINDING:
        /* FIXME: encode the list here, if it exists */
        break;
    case PROP_DATABASE_REVISION:
        apdu_len = encode_tagged_unsigned(&apdu[0], Database_Revision);
        break;
#if defined(BACDL_MSTP)
    case PROP_MAX_INFO_FRAMES:
        apdu_len =
            encode_tagged_unsigned(&apdu[0], dlmstp_max_info_frames());
        break;
    case PROP_MAX_MASTER:
        apdu_len = encode_tagged_unsigned(&apdu[0], dlmstp_max_master());
        break;
#endif
    default:
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
        apdu_len = -1;
        break;
    }

    return apdu_len;
}

/* returns true if successful */
bool Device_Write_Property(BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    if (!Device_Valid_Object_Instance_Number(wp_data->object_instance)) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */
    len = bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    /* FIXME: len == 0: unable to decode? */
    switch (wp_data->object_property) {
    case PROP_OBJECT_IDENTIFIER:
        if (value.tag == BACNET_APPLICATION_TAG_OBJECT_ID) {
            if ((value.type.Object_Id.type == OBJECT_DEVICE) &&
                (Device_Set_Object_Instance_Number(value.type.Object_Id.
                        instance))) {
                /* FIXME: we could send an I-Am broadcast to let the world know */
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;

    case PROP_NUMBER_OF_APDU_RETRIES:
        if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            /* FIXME: bounds check? */
            Device_Set_Number_Of_APDU_Retries((uint8_t) value.
                type.Unsigned_Int);
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;

    case PROP_APDU_TIMEOUT:
        if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            /* FIXME: bounds check? */
            Device_Set_APDU_Timeout((uint16_t) value.type.Unsigned_Int);
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;

    case PROP_VENDOR_IDENTIFIER:
        if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            /* FIXME: bounds check? */
            Device_Set_Vendor_Identifier((uint16_t) value.type.
                Unsigned_Int);
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;

    case PROP_SYSTEM_STATUS:
        if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
            /* FIXME: bounds check? */
            Device_Set_System_Status((BACNET_DEVICE_STATUS)value.type.Enumerated);
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;

    case PROP_OBJECT_NAME:
        if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
            uint8_t encoding;
            encoding =
                characterstring_encoding(&value.type.Character_String);
            if (encoding == CHARACTER_ANSI_X34) {
                status =
                    Device_Set_Object_Name(characterstring_value(&value.
                        type.Character_String),
                    characterstring_length(&value.type.Character_String));
                if (!status) {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
            }
        } else {
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
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE / 2);
    ct_test(pTest,
        Device_Object_Instance_Number() == (BACNET_MAX_INSTANCE / 2));
    ct_test(pTest, status == true);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE + 1);
    ct_test(pTest,
        Device_Object_Instance_Number() != (BACNET_MAX_INSTANCE + 1));
    ct_test(pTest, status == false);


    Device_Set_System_Status(STATUS_NON_OPERATIONAL);
    ct_test(pTest, Device_System_Status() == STATUS_NON_OPERATIONAL);

    Device_Set_Vendor_Name(name, strlen(name));
    ct_test(pTest, strcmp(Device_Vendor_Name(), name) == 0);

    Device_Set_Vendor_Identifier(42);
    ct_test(pTest, Device_Vendor_Identifier() == 42);

    Device_Set_Model_Name(name, strlen(name));
    ct_test(pTest, strcmp(Device_Model_Name(), name) == 0);

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

char *Analog_Value_Name(uint32_t object_instance)
{
    (void) object_instance;
    return "";
}

unsigned Analog_Value_Count(void)
{
    return 0;
}

uint32_t Analog_Value_Index_To_Instance(unsigned index)
{
    return index;
}

char *Binary_Input_Name(uint32_t object_instance)
{
    (void) object_instance;
    return "";
}

unsigned Binary_Input_Count(void)
{
    return 0;
}

uint32_t Binary_Input_Index_To_Instance(unsigned index)
{
    return index;
}

char *Binary_Output_Name(uint32_t object_instance)
{
    (void) object_instance;
    return "";
}

unsigned Binary_Output_Count(void)
{
    return 0;
}

uint32_t Binary_Output_Index_To_Instance(unsigned index)
{
    return index;
}

char *Binary_Value_Name(uint32_t object_instance)
{
    (void) object_instance;
    return "";
}

unsigned Binary_Value_Count(void)
{
    return 0;
}

uint32_t Binary_Value_Index_To_Instance(unsigned index)
{
    return index;
}

char *Life_Safety_Point_Name(uint32_t object_instance)
{
    (void) object_instance;
    return "";
}

unsigned Life_Safety_Point_Count(void)
{
    return 0;
}

uint32_t Life_Safety_Point_Index_To_Instance(unsigned index)
{
    return index;
}

char *Load_Control_Name(uint32_t object_instance)
{
    (void) object_instance;
    return "";
}

unsigned Load_Control_Count(void)
{
    return 0;
}

uint32_t Load_Control_Index_To_Instance(unsigned index)
{
    return index;
}

char *Multistate_Output_Name(uint32_t object_instance)
{
    (void) object_instance;
    return "";
}

unsigned Multistate_Output_Count(void)
{
    return 0;
}

uint32_t Multistate_Output_Index_To_Instance(unsigned index)
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
