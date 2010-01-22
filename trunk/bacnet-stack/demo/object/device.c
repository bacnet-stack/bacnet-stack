/**************************************************************************
*
* Copyright (C) 2005,2006,2009 Steve Karg <skarg@users.sourceforge.net>
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
#include <string.h>     /* for memmove */
#include <time.h> /* for timezone, localtime */
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "apdu.h"
#include "wp.h" /* write property handling */
#include "version.h"
#include "device.h"     /* me */
#include "handlers.h"
#include "datalink.h"
#include "address.h"
#if defined(BACFILE)
#include "bacfile.h"    /* object list dependency */
#endif
/* os specfic includes */
#include "timer.h"

#if defined(__BORLANDC__)
/* seems to not be defined in time.h as specified by The Open Group */
/* difference from UTC and local standard time  */
long int timezone;
#endif

static object_count_function Object_Count[MAX_BACNET_OBJECT_TYPE];
static object_index_to_instance_function
    Object_Index_To_Instance[MAX_BACNET_OBJECT_TYPE];
static object_name_function Object_Name[MAX_BACNET_OBJECT_TYPE];

void Device_Object_Function_Set(
    BACNET_OBJECT_TYPE object_type,
    object_count_function count_function,
    object_index_to_instance_function index_function,
    object_name_function name_function)
{
    if (object_type < MAX_BACNET_OBJECT_TYPE) {
        Object_Count[object_type] = count_function;
        Object_Index_To_Instance[object_type] = index_function;
        Object_Name[object_type] = name_function;
    }
}

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Device_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_SYSTEM_STATUS,
    PROP_VENDOR_NAME,
    PROP_VENDOR_IDENTIFIER,
    PROP_MODEL_NAME,
    PROP_FIRMWARE_REVISION,
    PROP_APPLICATION_SOFTWARE_VERSION,
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

static const int Device_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_LOCAL_TIME,
    PROP_UTC_OFFSET,
    PROP_LOCAL_DATE,
    PROP_DAYLIGHT_SAVINGS_STATUS,
    PROP_PROTOCOL_CONFORMANCE_CLASS,
    PROP_LOCATION,
    PROP_ACTIVE_COV_SUBSCRIPTIONS,
    -1
};

static const int Device_Properties_Proprietary[] = {
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
static uint32_t Object_Instance_Number = 260001;
static char My_Object_Name[16] = "SimpleServer";
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static char *Vendor_Name = BACNET_VENDOR_NAME;
static uint16_t Vendor_Identifier = BACNET_VENDOR_ID;
static char Model_Name[16] = "GNU";
static char Application_Software_Version[16] = "1.0";
static char Location[16] = "USA";
static char Description[16] = "server";
/* static uint8_t Protocol_Version = 1; - constant, not settable */
/* static uint8_t Protocol_Revision = 4; - constant, not settable */
/* Protocol_Services_Supported - dynamically generated */
/* Protocol_Object_Types_Supported - in RP encoding */
/* Object_List - dynamically generated */
/* static BACNET_SEGMENTATION Segmentation_Supported = SEGMENTATION_NONE; */
/* static uint8_t Max_Segments_Accepted = 0; */
/* VT_Classes_Supported */
/* Active_VT_Sessions */
static BACNET_TIME Local_Time; /* rely on OS, if there is one */
static BACNET_DATE Local_Date; /* rely on OS, if there is one */
/* NOTE: BACnet UTC Offset is inverse of common practice.
   If your UTC offset is -5hours of GMT, 
   then BACnet UTC offset is +5hours.
   BACnet UTC offset is expressed in minutes. */
static int32_t UTC_Offset = 5 * 60;
static bool Daylight_Savings_Status = false;    /* rely on OS */
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
uint32_t Device_Object_Instance_Number(
    void)
{
    return Object_Instance_Number;
}

bool Device_Set_Object_Instance_Number(
    uint32_t object_id)
{
    bool status = true; /* return value */

    if (object_id <= BACNET_MAX_INSTANCE)
        Object_Instance_Number = object_id;
    else
        status = false;

    return status;
}

bool Device_Valid_Object_Instance_Number(
    uint32_t object_id)
{
    /* BACnet allows for a wildcard instance number */
    return ((Object_Instance_Number == object_id) ||
        (object_id == BACNET_MAX_INSTANCE));
}

const char *Device_Object_Name(
    void)
{
    return My_Object_Name;
}

bool Device_Set_Object_Name(
    const char *name,
    size_t length)
{
    bool status = false;        /*return value */

    /* FIXME:  All the object names in a device must be unique.
       Disallow setting the Device Object Name to any objects in
       the device. */
    if (length < sizeof(My_Object_Name)) {
        memmove(My_Object_Name, name, length);
        My_Object_Name[length] = 0;
        status = true;
    }

    return status;
}

BACNET_DEVICE_STATUS Device_System_Status(
    void)
{
    return System_Status;
}

void Device_Set_System_Status(
    BACNET_DEVICE_STATUS status)
{
    /* FIXME: bounds check? */
    System_Status = status;
}

const char *Device_Vendor_Name(
    void)
{
    return Vendor_Name;
}

uint16_t Device_Vendor_Identifier(
    void)
{
    return Vendor_Identifier;
}

void Device_Set_Vendor_Identifier(
    uint16_t vendor_id)
{
    Vendor_Identifier = vendor_id;
}

const char *Device_Model_Name(
    void)
{
    return Model_Name;
}

bool Device_Set_Model_Name(
    const char *name,
    size_t length)
{
    bool status = false;        /*return value */

    if (length < sizeof(Model_Name)) {
        memmove(Model_Name, name, length);
        Model_Name[length] = 0;
        status = true;
    }

    return status;
}

const char *Device_Firmware_Revision(
    void)
{
    return BACnet_Version;
}

const char *Device_Application_Software_Version(
    void)
{
    return Application_Software_Version;
}

bool Device_Set_Application_Software_Version(
    const char *name,
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

const char *Device_Description(
    void)
{
    return Description;
}

bool Device_Set_Description(
    const char *name,
    size_t length)
{
    bool status = false;        /*return value */

    if (length < sizeof(Description)) {
        memmove(Description, name, length);
        Description[length] = 0;
        status = true;
    }

    return status;
}

const char *Device_Location(
    void)
{
    return Location;
}

bool Device_Set_Location(
    const char *name,
    size_t length)
{
    bool status = false;        /*return value */

    if (length < sizeof(Location)) {
        memmove(Location, name, length);
        Location[length] = 0;
        status = true;
    }

    return status;
}

uint8_t Device_Protocol_Version(
    void)
{
    return BACNET_PROTOCOL_VERSION;
}

uint8_t Device_Protocol_Revision(
    void)
{
    return BACNET_PROTOCOL_REVISION;
}

BACNET_SEGMENTATION Device_Segmentation_Supported(
    void)
{
    return SEGMENTATION_NONE;
}

uint8_t Device_Database_Revision(
    void)
{
    return Database_Revision;
}

void Device_Set_Database_Revision(
    uint8_t revision)
{
    Database_Revision = revision;
}

/* Since many network clients depend on the object list */
/* for discovery, it must be consistent! */
unsigned Device_Object_List_Count(
    void)
{
    unsigned count = 1; /* 1 for the device object */
    unsigned i = 0;     /* loop counter */

    for (i = 0; i < MAX_BACNET_OBJECT_TYPE; i++) {
        if (Object_Count[i]) {
            count += Object_Count[i] ();
        }
    }

    return count;
}

bool Device_Object_List_Identifier(
    unsigned array_index,
    int *object_type,
    uint32_t * instance)
{
    bool status = false;
    unsigned object_index = 0;
    unsigned count = 0;
    unsigned i = 0;     /* loop counter */

    if (array_index == 0) {
        return status;
    }
    /* device object */
    if (array_index == 1) {
        *object_type = OBJECT_DEVICE;
        *instance = Object_Instance_Number;
        status = true;
    }

    if (!status) {
        /* array index starts at 1, and if we are this far,
           we are not the device object, so array_index must
           be at least 2. */
        object_index = array_index - 2;
        /* look through the objects to find the right index */
        for (i = 0; i < MAX_BACNET_OBJECT_TYPE; i++) {
            if (Object_Count[i] && Object_Index_To_Instance[i]) {
                object_index -= count;
                count = Object_Count[i] ();
                if (object_index < count) {
                    *object_type = i;
                    *instance = Object_Index_To_Instance[i] (object_index);
                    status = true;
                    break;
                }
            }
        }
    }

    return status;
}

bool Device_Valid_Object_Name(
    const char *object_name,
    int *object_type,
    uint32_t * object_instance)
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
                if (object_type) {
                    *object_type = type;
                }
                if (object_instance) {
                    *object_instance = instance;
                }
                break;
            }
        }
    }

    return found;
}

/* returns the name or NULL if not found */
char *Device_Valid_Object_Id(
    int object_type,
    uint32_t object_instance)
{
    char *name = NULL;  /* return value */
    object_name_function name_function = NULL;

    if (object_type < MAX_BACNET_OBJECT_TYPE) {
        name_function = Object_Name[object_type];
    }
    if (name_function) {
        name = name_function(object_instance);
    } else {
        if ((object_type == OBJECT_DEVICE) &&
            (object_instance == Object_Instance_Number)) {
            name = My_Object_Name;
        }
    }

    return name;
}

static void Update_Current_Time(void)
{
    struct tm *tblock = NULL;
    #if defined(_MSC_VER)
    time_t tTemp;
    #else
    struct timeval tv;
    #endif
/*
struct tm
    
int    tm_sec   Seconds [0,60]. 
int    tm_min   Minutes [0,59]. 
int    tm_hour  Hour [0,23]. 
int    tm_mday  Day of month [1,31]. 
int    tm_mon   Month of year [0,11]. 
int    tm_year  Years since 1900. 
int    tm_wday  Day of week [0,6] (Sunday =0). 
int    tm_yday  Day of year [0,365]. 
int    tm_isdst Daylight Savings flag. 
*/
    #if defined(_MSC_VER)
    time(&tTemp);
    tblock = localtime(&tTemp);
    #else
    if (gettimeofday(&tv, NULL) == 0) { 
        tblock = localtime(&tv.tv_sec);
    }
    #endif
    
    if (tblock) {
        datetime_set_date(
            &Local_Date,
            (uint16_t) tblock->tm_year+1900,
            (uint8_t) tblock->tm_mon+1, 
            (uint8_t) tblock->tm_mday);
        #if !defined(_MSC_VER)
        datetime_set_time(
            &Local_Time,
            (uint8_t) tblock->tm_hour, 
            (uint8_t) tblock->tm_min,
            (uint8_t) tblock->tm_sec, 
            (uint8_t)(tv.tv_usec / 10000));
        #else
        datetime_set_time(
            &Local_Time,
            (uint8_t) tblock->tm_hour, 
            (uint8_t) tblock->tm_min,
            (uint8_t) tblock->tm_sec, 
            0);
        #endif
        if (tblock->tm_isdst) {
            Daylight_Savings_Status = true;
        } else {
            Daylight_Savings_Status = false;
        }
        /* note: timezone is declared in <time.h> stdlib. */
        UTC_Offset = timezone/60;
    } else {
        datetime_date_wildcard_set(&Local_Date);
        datetime_time_wildcard_set(&Local_Time);        
        Daylight_Savings_Status = false;
    }
}

/* return the length of the apdu encoded or -1 for error or
   -2 for abort message */
int Device_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;   /* return value */
    int len = 0;        /* apdu len intermediate value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned i = 0;
    int object_type = 0;
    uint32_t instance = 0;
    unsigned count = 0;

    object_instance = object_instance;
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_DEVICE,
                Object_Instance_Number);
            break;
        case PROP_OBJECT_NAME:
            characterstring_init_ansi(&char_string, My_Object_Name);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_DEVICE);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string, Description);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_SYSTEM_STATUS:
            apdu_len = encode_application_enumerated(&apdu[0], System_Status);
            break;
        case PROP_VENDOR_NAME:
            characterstring_init_ansi(&char_string, Vendor_Name);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_VENDOR_IDENTIFIER:
            apdu_len =
                encode_application_unsigned(&apdu[0], Vendor_Identifier);
            break;
        case PROP_MODEL_NAME:
            characterstring_init_ansi(&char_string, Model_Name);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FIRMWARE_REVISION:
            characterstring_init_ansi(&char_string, BACnet_Version);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_APPLICATION_SOFTWARE_VERSION:
            characterstring_init_ansi(&char_string,
                Application_Software_Version);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_LOCATION:
            characterstring_init_ansi(&char_string, Location);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_LOCAL_TIME:
            Update_Current_Time();
            apdu_len = encode_application_time(&apdu[0], &Local_Time);
            break;
        case PROP_UTC_OFFSET:
            Update_Current_Time();
            apdu_len = encode_application_signed(&apdu[0], UTC_Offset);
            break;
        case PROP_LOCAL_DATE:
            Update_Current_Time();
            apdu_len = encode_application_date(&apdu[0], &Local_Date);
            break;
        case PROP_DAYLIGHT_SAVINGS_STATUS:
            Update_Current_Time();
            apdu_len =
                encode_application_boolean(&apdu[0], Daylight_Savings_Status);
            break;
        case PROP_PROTOCOL_VERSION:
            apdu_len =
                encode_application_unsigned(&apdu[0],
                Device_Protocol_Version());
            break;
        case PROP_PROTOCOL_REVISION:
            apdu_len =
                encode_application_unsigned(&apdu[0],
                Device_Protocol_Revision());
            break;
            /* BACnet Legacy Support */
        case PROP_PROTOCOL_CONFORMANCE_CLASS:
            apdu_len = encode_application_unsigned(&apdu[0], 1);
            break;
        case PROP_PROTOCOL_SERVICES_SUPPORTED:
            /* Note: list of services that are executed, not initiated. */
            bitstring_init(&bit_string);
            for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++) {
                /* automatic lookup based on handlers set */
                bitstring_set_bit(&bit_string, (uint8_t) i,
                    apdu_service_supported((BACNET_SERVICES_SUPPORTED) i));
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
            /* Note: this is the list of objects that can be in this device,
               not a list of objects that this device can access */
            bitstring_init(&bit_string);
            for (i = 0; i < MAX_ASHRAE_OBJECT_TYPE; i++) {
                if ((i == OBJECT_DEVICE) || Object_Count[i]) {
                    bitstring_set_bit(&bit_string, i, true);
                } else {
                    /* initialize all the object types to not-supported */
                    bitstring_set_bit(&bit_string, (uint8_t) i, false);
                }
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OBJECT_LIST:
            count = Device_Object_List_Count();
            /* Array element zero is the number of objects in the list */
            if (array_index == 0)
                apdu_len = encode_application_unsigned(&apdu[0], count);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet.  Note that more than likely you will have */
            /* to return an error if the number of encoded objects exceeds */
            /* your maximum APDU size. */
            else if (array_index == BACNET_ARRAY_ALL) {
                for (i = 1; i <= count; i++) {
                    if (Device_Object_List_Identifier(i, &object_type,
                            &instance)) {
                        len =
                            encode_application_object_id(&apdu[apdu_len],
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
                        encode_application_object_id(&apdu[0], object_type,
                        instance);
                else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = -1;
                }
            }
            break;
        case PROP_MAX_APDU_LENGTH_ACCEPTED:
            apdu_len = encode_application_unsigned(&apdu[0], MAX_APDU);
            break;
        case PROP_SEGMENTATION_SUPPORTED:
            apdu_len =
                encode_application_enumerated(&apdu[0],
                Device_Segmentation_Supported());
            break;
        case PROP_APDU_TIMEOUT:
            apdu_len = encode_application_unsigned(&apdu[0], apdu_timeout());
            break;
        case PROP_NUMBER_OF_APDU_RETRIES:
            apdu_len = encode_application_unsigned(&apdu[0], apdu_retries());
            break;
        case PROP_DEVICE_ADDRESS_BINDING:
            /* FIXME: the real max apdu remaining should be passed into function */
            apdu_len = address_list_encode(&apdu[0], MAX_APDU);
            break;
        case PROP_DATABASE_REVISION:
            apdu_len =
                encode_application_unsigned(&apdu[0], Database_Revision);
            break;
#if defined(BACDL_MSTP)
        case PROP_MAX_INFO_FRAMES:
            apdu_len =
                encode_application_unsigned(&apdu[0],
                dlmstp_max_info_frames());
            break;
        case PROP_MAX_MASTER:
            apdu_len =
                encode_application_unsigned(&apdu[0], dlmstp_max_master());
            break;
#endif
        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
            /* FIXME: the real max apdu should be passed into function */
            apdu_len = handler_cov_encode_subscriptions(&apdu[0], MAX_APDU);
            break;
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) &&
        (property != PROP_OBJECT_LIST) &&
        (array_index != BACNET_ARRAY_ALL)) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = -1;
    }

    return apdu_len;
}

/* returns true if successful */
bool Device_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
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
    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    /* FIXME: len == 0: unable to decode? */
    switch (wp_data->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            if (value.tag == BACNET_APPLICATION_TAG_OBJECT_ID) {
                if ((value.type.Object_Id.type == OBJECT_DEVICE) &&
                    (Device_Set_Object_Instance_Number(value.type.
                            Object_Id.instance))) {
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
                apdu_retries_set((uint8_t) value.type.Unsigned_Int);
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_APDU_TIMEOUT:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                /* FIXME: bounds check? */
                apdu_timeout_set((uint16_t) value.type.Unsigned_Int);
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_VENDOR_IDENTIFIER:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                /* FIXME: bounds check? */
                Device_Set_Vendor_Identifier((uint16_t) value.
                    type.Unsigned_Int);
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_SYSTEM_STATUS:
            if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                /* FIXME: bounds check? */
                Device_Set_System_Status((BACNET_DEVICE_STATUS) value.
                    type.Enumerated);
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
                        Device_Set_Object_Name(characterstring_value
                        (&value.type.Character_String),
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
#if defined(BACDL_MSTP)
        case PROP_MAX_INFO_FRAMES:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if (value.type.Unsigned_Int <= 255) {
                    dlmstp_set_max_info_frames((uint8_t) value.
                        type.Unsigned_Int);
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
        case PROP_MAX_MASTER:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if ((value.type.Unsigned_Int > 0) &&
                    (value.type.Unsigned_Int <= 127)) {
                    dlmstp_set_max_master((uint8_t) value.type.Unsigned_Int);
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
#endif
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return status;
}

void Device_Init(
    void)
{
}

bool DeviceGetRRInfo(
    uint32_t           object,   /* Which particular object - obviously not important for device object */
    BACNET_PROPERTY_ID property, /* Which property */
    RR_PROP_INFO      *pInfo,    /* Where to put the information */
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE  *error_code)
{
    bool status = false; /* return value */
    
    object = object;
    switch(property) {
        case PROP_VT_CLASSES_SUPPORTED:
        case PROP_ACTIVE_VT_SESSIONS:
        case PROP_LIST_OF_SESSION_KEYS:
        case PROP_TIME_SYNCHRONIZATION_RECIPIENTS:
        case PROP_MANUAL_SLAVE_ADDRESS_BINDING:
        case PROP_SLAVE_ADDRESS_BINDING:
        case PROP_RESTART_NOTIFICATION_RECIPIENTS:
        case PROP_UTC_TIME_SYNCHRONIZATION_RECIPIENTS:
            pInfo->RequestTypes = RR_BY_POSITION;
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code  = ERROR_CODE_UNKNOWN_PROPERTY;
            break;

        case PROP_DEVICE_ADDRESS_BINDING:
            pInfo->RequestTypes = RR_BY_POSITION;
            pInfo->Handler = rr_address_list_encode;
            status = true;
            break;
        
        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
            pInfo->RequestTypes = RR_BY_POSITION;
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code  = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
        default:
            *error_class = ERROR_CLASS_SERVICES;
            *error_code  = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
            break;
    }

    return status;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testDevice(
    Test * pTest)
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

    ct_test(pTest, Device_Vendor_Identifier() == BACNET_VENDOR_ID);

    Device_Set_Model_Name(name, strlen(name));
    ct_test(pTest, strcmp(Device_Model_Name(), name) == 0);

    return;
}

#ifdef TEST_DEVICE
int main(
    void)
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
#endif /* TEST_DEVICE */
#endif /* TEST */
