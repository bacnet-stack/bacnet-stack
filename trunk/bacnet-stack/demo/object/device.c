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

/** @file device.c Base "class" for handling all BACnet objects belonging
 *                 to a BACnet device, as well as Device-specific properties. */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>     /* for memmove */
#include <time.h>       /* for timezone, localtime */
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "apdu.h"
#include "wp.h" /* write property handling */
#include "rp.h" /* read property handling */
#include "version.h"
#include "device.h"     /* me */
#include "handlers.h"
#include "datalink.h"
#include "address.h"
/* include the objects */
#include "device.h"
#include "ai.h"
#include "ao.h"
#include "av.h"
#include "bi.h"
#include "bo.h"
#include "bv.h"
#include "lc.h"
#include "lsp.h"
#include "mso.h"
#include "ms-input.h"
#include "trendlog.h"
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

/* forward prototypes */
static int Device_Read_Property_Local(
    BACNET_READ_PROPERTY_DATA * rpdata);
static bool Device_Write_Property_Local(
    BACNET_WRITE_PROPERTY_DATA * wp_data);

/** Defines the group of object helper functions for any supported Object. 
 * @ingroup ObjHelpers 
 * Each Object must provide some implementation of each of these helpers
 * in order to properly support the handlers.  Eg, the ReadProperty handler
 * handler_read_property() relies on the instance of Object_Read_Property
 * for each Object type.
 * In both appearance and operation, this group of functions acts like
 * they are member functions of a C++ Object base class.
 */
static struct object_functions {
    BACNET_OBJECT_TYPE Object_Type;
    object_init_function Object_Init;
    object_count_function Object_Count;
    object_index_to_instance_function Object_Index_To_Instance;
    object_valid_instance_function Object_Valid_Instance;
    object_name_function Object_Name;
    read_property_function Object_Read_Property;
    write_property_function Object_Write_Property;
    rpm_property_lists_function Object_RPM_List;
    rr_info_function Object_RR_Info;
    object_iterate_function Object_Iterator;
} Object_Table[] = {
    {
    OBJECT_DEVICE, NULL, Device_Count, Device_Index_To_Instance,
            Device_Valid_Object_Instance_Number, Device_Name,
            Device_Read_Property_Local, Device_Write_Property_Local,
            Device_Property_Lists, DeviceGetRRInfo, NULL}, {
    OBJECT_ANALOG_INPUT, Analog_Input_Init, Analog_Input_Count,
            Analog_Input_Index_To_Instance, Analog_Input_Valid_Instance,
            Analog_Input_Name, Analog_Input_Read_Property, NULL,
            Analog_Input_Property_Lists, NULL, NULL}, {
    OBJECT_ANALOG_OUTPUT, Analog_Output_Init, Analog_Output_Count,
            Analog_Output_Index_To_Instance, Analog_Output_Valid_Instance,
            Analog_Output_Name, Analog_Output_Read_Property,
            Analog_Output_Write_Property, Analog_Output_Property_Lists,
            NULL, NULL}, {
    OBJECT_ANALOG_VALUE, Analog_Value_Init, Analog_Value_Count,
            Analog_Value_Index_To_Instance, Analog_Value_Valid_Instance,
            Analog_Value_Name, Analog_Value_Read_Property,
            Analog_Value_Write_Property, Analog_Value_Property_Lists, NULL,
            NULL}, {
    OBJECT_BINARY_INPUT, Binary_Input_Init, Binary_Input_Count,
            Binary_Input_Index_To_Instance, Binary_Input_Valid_Instance,
            Binary_Input_Name, Binary_Input_Read_Property, NULL,
            Binary_Input_Property_Lists, NULL, NULL}, {
    OBJECT_BINARY_OUTPUT, Binary_Output_Init, Binary_Output_Count,
            Binary_Output_Index_To_Instance, Binary_Output_Valid_Instance,
            Binary_Output_Name, Binary_Output_Read_Property,
            Binary_Output_Write_Property, Binary_Output_Property_Lists,
            NULL, NULL}, {
    OBJECT_BINARY_VALUE, Binary_Value_Init, Binary_Value_Count,
            Binary_Value_Index_To_Instance, Binary_Value_Valid_Instance,
            Binary_Value_Name, Binary_Value_Read_Property,
            Binary_Value_Write_Property, Binary_Value_Property_Lists, NULL,
            NULL}, {
    OBJECT_LIFE_SAFETY_POINT, Life_Safety_Point_Init,
            Life_Safety_Point_Count, Life_Safety_Point_Index_To_Instance,
            Life_Safety_Point_Valid_Instance, Life_Safety_Point_Name,
            Life_Safety_Point_Read_Property,
            Life_Safety_Point_Write_Property,
            Life_Safety_Point_Property_Lists, NULL, NULL}, {
    OBJECT_LOAD_CONTROL, Load_Control_Init, Load_Control_Count,
            Load_Control_Index_To_Instance, Load_Control_Valid_Instance,
            Load_Control_Name, Load_Control_Read_Property,
            Load_Control_Write_Property, Load_Control_Property_Lists, NULL,
            NULL}, {
    OBJECT_MULTI_STATE_OUTPUT, Multistate_Output_Init,
            Multistate_Output_Count, Multistate_Output_Index_To_Instance,
            Multistate_Output_Valid_Instance, Multistate_Output_Name,
            Multistate_Output_Read_Property,
            Multistate_Output_Write_Property,
            Multistate_Output_Property_Lists, NULL, NULL}, {
    OBJECT_MULTI_STATE_INPUT, Multistate_Input_Init,
            Multistate_Input_Count, Multistate_Input_Index_To_Instance,
            Multistate_Input_Valid_Instance, Multistate_Input_Name,
            Multistate_Input_Read_Property,
            Multistate_Input_Write_Property,
            Multistate_Input_Property_Lists, NULL, NULL}, {
    OBJECT_TRENDLOG, Trend_Log_Init, Trend_Log_Count,
            Trend_Log_Index_To_Instance, Trend_Log_Valid_Instance,
            Trend_Log_Name, Trend_Log_Read_Property,
            Trend_Log_Write_Property, Trend_Log_Property_Lists,
            TrendLogGetRRInfo, NULL},
#if defined(BACFILE)
    {
    OBJECT_FILE, bacfile_init, bacfile_count, bacfile_index_to_instance,
            bacfile_valid_instance, bacfile_name, bacfile_read_property,
            bacfile_write_property, BACfile_Property_Lists, NULL},
#endif
    {
    MAX_BACNET_OBJECT_TYPE, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

/** Glue function to let the Device object, when called by a handler,
 * lookup which Object type needs to be invoked.
 * @ingroup ObjHelpers 
 * @param Object_Type [in] The type of BACnet Object the handler wants to access.
 * @return Pointer to the group of object helper functions that implement this
 *         type of Object. 
 */
static struct object_functions *Device_Objects_Find_Functions(
    BACNET_OBJECT_TYPE Object_Type)
{
    struct object_functions *pObject = NULL;

    pObject = &Object_Table[0];
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        /* handle each object type */
        if (pObject->Object_Type == Object_Type) {
            return (pObject);
        }

        pObject++;
    }

    return (NULL);
}

/** Try to find a rr_info_function helper function for the requested object type.
 * @ingroup ObjIntf
 * 
 * @param object_type [in] The type of BACnet Object the handler wants to access.
 * @return Pointer to the object helper function that implements the 
 *         ReadRangeInfo function, Object_RR_Info, for this type of Object on 
 *         success, else a NULL pointer if the type of Object isn't supported 
 *         or doesn't have a ReadRangeInfo function.
 */
rr_info_function Device_Objects_RR_Info(
    BACNET_OBJECT_TYPE object_type)
{
    struct object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    return (pObject != NULL ? pObject->Object_RR_Info : NULL);
}

static unsigned property_list_count(
    const int *pList)
{
    unsigned property_count = 0;

    if (pList) {
        while (*pList != -1) {
            property_count++;
            pList++;
        }
    }

    return property_count;
}

/** For a given object type, returns the special property list.
 * This function is used for ReadPropertyMultiple calls which want
 * just Required, just Optional, or All properties.
 * @ingroup ObjIntf
 *
 * @param object_type [in] The desired BACNET_OBJECT_TYPE whose properties
 *            are to be listed.
 * @param pPropertyList [out] Reference to the structure which will, on return,
 *            list, separately, the Required, Optional, and Proprietary object
 *            properties with their counts.
 */
void Device_Objects_Property_List(
    BACNET_OBJECT_TYPE object_type,
    struct special_property_list_t *pPropertyList)
{
    struct object_functions *pObject = NULL;

    pPropertyList->Required.pList = NULL;
    pPropertyList->Optional.pList = NULL;
    pPropertyList->Proprietary.pList = NULL;

    /* If we can find an entry for the required object type
     * and there is an Object_List_RPM fn ptr then call it
     * to populate the pointers to the individual list counters.
     */

    pObject = Device_Objects_Find_Functions(object_type);
    if ((pObject != NULL) && (pObject->Object_RPM_List != NULL)) {
        pObject->Object_RPM_List(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, &pPropertyList->Proprietary.pList);
    }

    /* Fetch the counts if available otherwise zero them */
    pPropertyList->Required.count =
        pPropertyList->Required.pList ==
        NULL ? 0 : property_list_count(pPropertyList->Required.pList);

    pPropertyList->Optional.count =
        pPropertyList->Optional.pList ==
        NULL ? 0 : property_list_count(pPropertyList->Optional.pList);

    pPropertyList->Proprietary.count =
        pPropertyList->Proprietary.pList ==
        NULL ? 0 : property_list_count(pPropertyList->Proprietary.pList);

    return;
}

/** Commands a Device re-initialization, to a given state.
 * The request's password must match for the operation to succeed.
 * This implementation provides a framework, but doesn't
 * actually *DO* anything.
 * @note You could use a mix of states and passwords to multiple outcomes.
 * @note You probably want to restart *after* the simple ack has been sent
 *       from the return handler, so just set a local flag here.
 * @ingroup ObjIntf
 *
 * @param rd_data [in,out] The information from the RD request.
 *                         On failure, the error class and code will be set.
 * @return True if succeeds (password is correct), else False.
 */
bool Device_Reinitialize(
    BACNET_REINITIALIZE_DEVICE_DATA * rd_data)
{
    bool status = false;

    if (characterstring_ansi_same(&rd_data->password, "Jesus")) {
        switch (rd_data->state) {
            case BACNET_REINIT_COLDSTART:
                break;
            case BACNET_REINIT_WARMSTART:
                break;
            case BACNET_REINIT_STARTBACKUP:
                break;
            case BACNET_REINIT_ENDBACKUP:
                break;
            case BACNET_REINIT_STARTRESTORE:
                break;
            case BACNET_REINIT_ENDRESTORE:
                break;
            case BACNET_REINIT_ABORTRESTORE:
                break;
            default:
                break;
        }
        /* Note: you could use a mix of state 
           and password to multiple things */
        /* note: you probably want to restart *after* the 
           simple ack has been sent from the return handler
           so just set a flag from here */
        status = true;
    } else {
        rd_data->error_class = ERROR_CLASS_SECURITY;
        rd_data->error_code = ERROR_CODE_PASSWORD_FAILURE;
    }

    return status;
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

/* String Lengths - excluding any nul terminator */

#define MAX_DEV_NAME_LEN 32
#define MAX_DEV_LOC_LEN  64
#define MAX_DEV_MOD_LEN  32
#define MAX_DEV_VER_LEN  16
#define MAX_DEV_DESC_LEN 64

static uint32_t Object_Instance_Number = 260001;
static char My_Object_Name[MAX_DEV_NAME_LEN + 1] = "SimpleServer";
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static char *Vendor_Name = BACNET_VENDOR_NAME;
static uint16_t Vendor_Identifier = BACNET_VENDOR_ID;
static char Model_Name[MAX_DEV_MOD_LEN + 1] = "GNU";
static char Application_Software_Version[MAX_DEV_VER_LEN + 1] = "1.0";
static char Location[MAX_DEV_LOC_LEN + 1] = "USA";
static char Description[MAX_DEV_DESC_LEN + 1] = "server";
/* static uint8_t Protocol_Version = 1; - constant, not settable */
/* static uint8_t Protocol_Revision = 4; - constant, not settable */
/* Protocol_Services_Supported - dynamically generated */
/* Protocol_Object_Types_Supported - in RP encoding */
/* Object_List - dynamically generated */
/* static BACNET_SEGMENTATION Segmentation_Supported = SEGMENTATION_NONE; */
/* static uint8_t Max_Segments_Accepted = 0; */
/* VT_Classes_Supported */
/* Active_VT_Sessions */
static BACNET_TIME Local_Time;  /* rely on OS, if there is one */
static BACNET_DATE Local_Date;  /* rely on OS, if there is one */
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
static uint32_t Database_Revision = 0;
/* Configuration_Files */
/* Last_Restore_Time */
/* Backup_Failure_Timeout */
/* Active_COV_Subscriptions */
/* Slave_Proxy_Enable */
/* Manual_Slave_Address_Binding */
/* Auto_Slave_Discovery */
/* Slave_Address_Binding */
/* Profile_Name */

unsigned Device_Count(
    void)
{
    return 1;
}

uint32_t Device_Index_To_Instance(
    unsigned index)
{
    index = index;
    return Object_Instance_Number;
}

/* methods to manipulate the data */

/** Return the Object Instance number for our (single) Device Object.
 * This is a key function, widely invoked by the handler code, since
 * it provides "our" (ie, local) address.
 * @ingroup ObjIntf
 * @return The Instance number used in the BACNET_OBJECT_ID for the Device.
 */
uint32_t Device_Object_Instance_Number(
    void)
{
    return Object_Instance_Number;
}

bool Device_Set_Object_Instance_Number(
    uint32_t object_id)
{
    bool status = true; /* return value */

    if (object_id <= BACNET_MAX_INSTANCE) {
        /* Make the change and update the database revision */
        Object_Instance_Number = object_id;
        Device_Inc_Database_Revision();
    } else
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

char *Device_Name(
    uint32_t object_instance)
{
    if (object_instance == Object_Instance_Number) {
        return My_Object_Name;
    }

    return NULL;
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
        /* Make the change and update the database revision */
        memmove(My_Object_Name, name, length);
        My_Object_Name[length] = 0;
        Device_Inc_Database_Revision();
        status = true;
    }

    return status;
}

BACNET_DEVICE_STATUS Device_System_Status(
    void)
{
    return System_Status;
}

int Device_Set_System_Status(
    BACNET_DEVICE_STATUS status,
    bool local)
{
    int result = 0;     /*return value - 0 = ok, -1 = bad value, -2 = not allowed */

    /* We limit the options available depending on whether the source is
     * internal or external. */
    if (local) {
        switch (status) {
            case STATUS_OPERATIONAL:
            case STATUS_OPERATIONAL_READ_ONLY:
            case STATUS_DOWNLOAD_REQUIRED:
            case STATUS_DOWNLOAD_IN_PROGRESS:
            case STATUS_NON_OPERATIONAL:
                System_Status = status;
                break;

                /* Don't support backup at present so don't allow setting */
            case STATUS_BACKUP_IN_PROGRESS:
                result = -2;
                break;

            default:
                result = -1;
                break;
        }
    } else {
        switch (status) {
                /* Allow these for the moment as a way to easily alter
                 * overall device operation. The lack of password protection
                 * or other authentication makes allowing writes to this
                 * property a risky facility to provide.
                 */
            case STATUS_OPERATIONAL:
            case STATUS_OPERATIONAL_READ_ONLY:
            case STATUS_NON_OPERATIONAL:
                System_Status = status;
                break;

                /* Don't allow outsider set this - it should probably
                 * be set if the device config is incomplete or 
                 * corrupted or perhaps after some sort of operator
                 * wipe operation.
                 */
            case STATUS_DOWNLOAD_REQUIRED:
                /* Don't allow outsider set this - it should be set
                 * internally at the start of a multi packet download
                 * perhaps indirectly via PT or WF to a config file.
                 */
            case STATUS_DOWNLOAD_IN_PROGRESS:
                /* Don't support backup at present so don't allow setting */
            case STATUS_BACKUP_IN_PROGRESS:
                result = -2;
                break;

            default:
                result = -1;
                break;
        }
    }

    return (result);
}

const char *Device_Vendor_Name(
    void)
{
    return Vendor_Name;
}

/** Returns the Vendor ID for this Device.
 * See the assignments at http://www.bacnet.org/VendorID/BACnet%20Vendor%20IDs.htm
 * @return The Vendor ID of this Device.
 */
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

uint32_t Device_Database_Revision(
    void)
{
    return Database_Revision;
}

void Device_Set_Database_Revision(
    uint32_t revision)
{
    Database_Revision = revision;
}

/* 
 * Shortcut for incrementing database revision as this is potentially
 * the most common operation if changing object names and ids is
 * implemented.
 */
void Device_Inc_Database_Revision(
    void)
{
    Database_Revision++;
}

/** Get the total count of objects supported by this Device Object.
 * @note Since many network clients depend on the object list
 *       for discovery, it must be consistent!
 * @return The count of objects, for all supported Object types.
 */
unsigned Device_Object_List_Count(
    void)
{
    unsigned count = 0; /* number of objects */
    struct object_functions *pObject = NULL;

    /* initialize the default return values */
    pObject = &Object_Table[0];
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Count) {
            count += pObject->Object_Count();
        }
        pObject++;
    }

    return count;
}

/** Lookup the Object at the given array index in the Device's Object List.
 * Even though we don't keep a single linear array of objects in the Device,
 * this method acts as though we do and works through a virtual, concatenated
 * array of all of our object type arrays.
 *
 * @param array_index [in] The desired array index (1 to N)
 * @param object_type [out] The object's type, if found.
 * @param instance [out] The object's instance number, if found.
 * @return True if found, else false.
 */
bool Device_Object_List_Identifier(
    unsigned array_index,
    int *object_type,
    uint32_t * instance)
{
    bool status = false;
    unsigned count = 0;
    unsigned object_index = 0;
    unsigned temp_index = 0;
    struct object_functions *pObject = NULL;

    /* array index zero is length - so invalid */
    if (array_index == 0) {
        return status;
    }
    object_index = array_index - 1;
    /* initialize the default return values */
    pObject = &Object_Table[0];
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Count) {
            object_index -= count;
            count = pObject->Object_Count();
            if (object_index < count) {
                /* Use the iterator function if available otherwise
                 * look for the index to instance to get the ID */
                if (pObject->Object_Iterator) {
                    /* First find the first object */
                    temp_index = pObject->Object_Iterator(~(unsigned) 0);
                    /* Then step through the objects to find the nth */
                    while (object_index != 0) {
                        temp_index = pObject->Object_Iterator(temp_index);
                        object_index--;
                    }
                    /* set the object_index up before falling through to next bit */
                    object_index = temp_index;
                }
                if (pObject->Object_Index_To_Instance) {
                    *object_type = pObject->Object_Type;
                    *instance =
                        pObject->Object_Index_To_Instance(object_index);
                    status = true;
                    break;
                }
            }
        }
        pObject++;
    }

    return status;
}

/** Determine if we have an object with the given object_name.
 * If the object_type and object_instance pointers are not null,
 * and the lookup succeeds, they will be given the resulting values.
 * @param object_name [in] The desired Object Name to look for.
 * @param object_type [out] The BACNET_OBJECT_TYPE of the matching Object.
 * @param object_instance [out] The object instance number of the matching Object.
 * @return True on success or else False if not found.
 */
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

/** Determine if we have an object of this type and instance number.
 * @param object_type [in] The desired BACNET_OBJECT_TYPE
 * @param object_instance [in] The object instance number to be looked up.
 * @return The Object Name or else NULL if not found
 */
char *Device_Valid_Object_Id(
    int object_type,
    uint32_t object_instance)
{
    char *name = NULL;  /* return value */
    struct object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if ((pObject != NULL) && (pObject->Object_Name != NULL))
        name = pObject->Object_Name(object_instance);

    return name;
}

static void Update_Current_Time(
    void)
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
        datetime_set_date(&Local_Date, (uint16_t) tblock->tm_year + 1900,
            (uint8_t) tblock->tm_mon + 1, (uint8_t) tblock->tm_mday);
#if !defined(_MSC_VER)
        datetime_set_time(&Local_Time, (uint8_t) tblock->tm_hour,
            (uint8_t) tblock->tm_min, (uint8_t) tblock->tm_sec,
            (uint8_t) (tv.tv_usec / 10000));
#else
        datetime_set_time(&Local_Time, (uint8_t) tblock->tm_hour,
            (uint8_t) tblock->tm_min, (uint8_t) tblock->tm_sec, 0);
#endif
        if (tblock->tm_isdst) {
            Daylight_Savings_Status = true;
        } else {
            Daylight_Savings_Status = false;
        }
        /* note: timezone is declared in <time.h> stdlib. */
        UTC_Offset = timezone / 60;
    } else {
        datetime_date_wildcard_set(&Local_Date);
        datetime_time_wildcard_set(&Local_Time);
        Daylight_Savings_Status = false;
    }
}

/* return the length of the apdu encoded or BACNET_STATUS_ERROR for error or
   BACNET_STATUS_ABORT for abort message */
static int Device_Read_Property_Local(
    BACNET_READ_PROPERTY_DATA * rpdata)
{
    int apdu_len = 0;   /* return value */
    int len = 0;        /* apdu len intermediate value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned i = 0;
    int object_type = 0;
    uint32_t instance = 0;
    unsigned count = 0;
    uint8_t *apdu = NULL;
    struct object_functions *pObject = NULL;
    bool found = false;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
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
                /* initialize all the object types to not-supported */
                bitstring_set_bit(&bit_string, (uint8_t) i, false);
            }
            /* set the object types with objects to supported */

            pObject = &Object_Table[0];
            while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
                if ((pObject->Object_Count) && (pObject->Object_Count() > 0)) {
                    bitstring_set_bit(&bit_string, pObject->Object_Type, true);
                }
                pObject++;
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OBJECT_LIST:
            count = Device_Object_List_Count();
            /* Array element zero is the number of objects in the list */
            if (rpdata->array_index == 0)
                apdu_len = encode_application_unsigned(&apdu[0], count);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet.  Note that more than likely you will have */
            /* to return an error if the number of encoded objects exceeds */
            /* your maximum APDU size. */
            else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                for (i = 1; i <= count; i++) {
                    found =
                        Device_Object_List_Identifier(i, &object_type,
                        &instance);
                    if (found) {
                        len =
                            encode_application_object_id(&apdu[apdu_len],
                            object_type, instance);
                        apdu_len += len;
                        /* assume next one is the same size as this one */
                        /* can we all fit into the APDU? */
                        if ((apdu_len + len) >= MAX_APDU) {
                            /* Abort response */
                            rpdata->error_code =
                                ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                            apdu_len = BACNET_STATUS_ABORT;
                            break;
                        }
                    } else {
                        /* error: internal error? */
                        rpdata->error_class = ERROR_CLASS_SERVICES;
                        rpdata->error_code = ERROR_CODE_OTHER;
                        apdu_len = BACNET_STATUS_ERROR;
                        break;
                    }
                }
            } else {
                found =
                    Device_Object_List_Identifier(rpdata->array_index,
                    &object_type, &instance);
                if (found) {
                    apdu_len =
                        encode_application_object_id(&apdu[0], object_type,
                        instance);
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
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
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_OBJECT_LIST) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/** Looks up the requested Object and Property, and encodes its Value in an APDU.
 * @ingroup ObjIntf
 * If the Object or Property can't be found, sets the error class and code.
 * 
 * @param rpdata [in,out] Structure with the desired Object and Property info
 * 				on entry, and APDU message on return.
 * @return The length of the APDU on success, else BACNET_STATUS_ERROR
 */
int Device_Read_Property(
    BACNET_READ_PROPERTY_DATA * rpdata)
{
    int apdu_len = BACNET_STATUS_ERROR;
    struct object_functions *pObject = NULL;

    /* initialize the default return values */
    rpdata->error_class = ERROR_CLASS_OBJECT;
    rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    pObject = Device_Objects_Find_Functions(rpdata->object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(rpdata->object_instance)) {
            if (pObject->Object_Read_Property) {
                apdu_len = pObject->Object_Read_Property(rpdata);
            }
        } else {
            rpdata->error_class = ERROR_CLASS_OBJECT;
            rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
    } else {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNSUPPORTED_OBJECT_TYPE;
    }

    return apdu_len;
}

/* returns true if successful */
static bool Device_Write_Property_Local(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    int temp;

    /* decode the some of the request */
    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    /* FIXME: len == 0: unable to decode? */
    switch (wp_data->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_OBJECT_ID,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                if ((value.type.Object_Id.type == OBJECT_DEVICE) &&
                    (Device_Set_Object_Instance_Number(value.type.
                            Object_Id.instance))) {
                    /* FIXME: we could send an I-Am broadcast to let the world know */
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_NUMBER_OF_APDU_RETRIES:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                /* FIXME: bounds check? */
                apdu_retries_set((uint8_t) value.type.Unsigned_Int);
            }
            break;
        case PROP_APDU_TIMEOUT:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                /* FIXME: bounds check? */
                apdu_timeout_set((uint16_t) value.type.Unsigned_Int);
            }
            break;
        case PROP_VENDOR_IDENTIFIER:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                /* FIXME: bounds check? */
                Device_Set_Vendor_Identifier((uint16_t) value.
                    type.Unsigned_Int);
            }
            break;
        case PROP_SYSTEM_STATUS:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_ENUMERATED,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                temp = Device_Set_System_Status((BACNET_DEVICE_STATUS)
                    value.type.Enumerated, false);
                if (temp != 0) {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    if (temp == -1) {
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    } else {
                        wp_data->error_code =
                            ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                    }
                }
            }
            break;
        case PROP_OBJECT_NAME:
            status =
                WPValidateString(&value, MAX_DEV_NAME_LEN, false,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                Device_Set_Object_Name(characterstring_value(&value.
                        type.Character_String),
                    characterstring_length(&value.type.Character_String));
            }
            break;
        case PROP_LOCATION:
            status =
                WPValidateString(&value, MAX_DEV_LOC_LEN, true,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                Device_Set_Location(characterstring_value(&value.
                        type.Character_String),
                    characterstring_length(&value.type.Character_String));
            }
            break;

        case PROP_DESCRIPTION:
            status =
                WPValidateString(&value, MAX_DEV_DESC_LEN, true,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                Device_Set_Description(characterstring_value(&value.
                        type.Character_String),
                    characterstring_length(&value.type.Character_String));
            }
            break;
        case PROP_MODEL_NAME:
            status =
                WPValidateString(&value, MAX_DEV_MOD_LEN, true,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                Device_Set_Model_Name(characterstring_value(&value.
                        type.Character_String),
                    characterstring_length(&value.type.Character_String));
            }
            break;

#if defined(BACDL_MSTP)
        case PROP_MAX_INFO_FRAMES:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                if (value.type.Unsigned_Int <= 255) {
                    dlmstp_set_max_info_frames((uint8_t) value.
                        type.Unsigned_Int);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_MAX_MASTER:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                if ((value.type.Unsigned_Int > 0) &&
                    (value.type.Unsigned_Int <= 127)) {
                    dlmstp_set_max_master((uint8_t) value.type.Unsigned_Int);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
#endif
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return status;
}

/** Looks up the requested Object and Property, and set the new Value in it,
 *  if allowed.
 * If the Object or Property can't be found, sets the error class and code.
 * @ingroup ObjIntf
 * 
 * @param wp_data [in,out] Structure with the desired Object and Property info
 *              and new Value on entry, and APDU message on return.
 * @return True on success, else False if there is an error.
 */
bool Device_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* Ever the pessamist! */
    struct object_functions *pObject = NULL;

    /* initialize the default return values */
    wp_data->error_class = ERROR_CLASS_OBJECT;
    wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    pObject = Device_Objects_Find_Functions(wp_data->object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(wp_data->object_instance)) {
            if (pObject->Object_Write_Property) {
                status = pObject->Object_Write_Property(wp_data);
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
        } else {
            wp_data->error_class = ERROR_CLASS_OBJECT;
            wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
    } else {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNSUPPORTED_OBJECT_TYPE;
    }

    return (status);
}


/** Initialize the Device Object and each of its child Object instances.
 * @ingroup ObjIntf
 */
void Device_Init(
    void)
{
    struct object_functions *pObject = NULL;

    pObject = &Object_Table[0];
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Init) {
            pObject->Object_Init();
        }
        pObject++;
    }
}

bool DeviceGetRRInfo(
    BACNET_READ_RANGE_DATA * pRequest,  /* Info on the request */
    RR_PROP_INFO * pInfo)
{       /* Where to put the response */
    bool status = false;        /* return value */

    switch (pRequest->object_property) {
        case PROP_VT_CLASSES_SUPPORTED:
        case PROP_ACTIVE_VT_SESSIONS:
        case PROP_LIST_OF_SESSION_KEYS:
        case PROP_TIME_SYNCHRONIZATION_RECIPIENTS:
        case PROP_MANUAL_SLAVE_ADDRESS_BINDING:
        case PROP_SLAVE_ADDRESS_BINDING:
        case PROP_RESTART_NOTIFICATION_RECIPIENTS:
        case PROP_UTC_TIME_SYNCHRONIZATION_RECIPIENTS:
            pInfo->RequestTypes = RR_BY_POSITION;
            pRequest->error_class = ERROR_CLASS_PROPERTY;
            pRequest->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;

        case PROP_DEVICE_ADDRESS_BINDING:
            pInfo->RequestTypes = RR_BY_POSITION;
            pInfo->Handler = rr_address_list_encode;
            status = true;
            break;

        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
            pInfo->RequestTypes = RR_BY_POSITION;
            pRequest->error_class = ERROR_CLASS_PROPERTY;
            pRequest->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
        default:
            pRequest->error_class = ERROR_CLASS_SERVICES;
            pRequest->error_code = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
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


    Device_Set_System_Status(STATUS_NON_OPERATIONAL, true);
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
