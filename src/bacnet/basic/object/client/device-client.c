/**
 * @file
 * @brief Lightweight base "class" for handling all
 * BACnet objects belonging to a BACnet device, as well as
 * Device-specific properties.  This Device instance is designed to
 * meet minimal functionality for simple clients.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/datetime.h"
#include "bacnet/apdu.h"
#include "bacnet/proplist.h"
#include "bacnet/rp.h" /* ReadProperty handling */
#include "bacnet/dcc.h" /* DeviceCommunicationControl handling */
#include "bacnet/version.h"
/* BACnet basic library */
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/sys/mstimer.h"
/* BACnet basic objects API */
#include "bacnet/basic/object/device.h"
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif

static BACNET_CHARACTER_STRING My_Object_Name;
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static char *Vendor_Name = BACNET_VENDOR_NAME;
static char *Model_Name = "GNU";
static char *Application_Software_Version = "1.0";
static const char *BACnet_Version = BACNET_VERSION_TEXT;
static char *Location = "USA";
static char *Description = "command line client";
static BACNET_TIME Local_Time;
static BACNET_DATE Local_Date;
static int16_t UTC_Offset;
static bool Daylight_Savings_Status;
#if defined(BACNET_TIME_MASTER)
static bool Align_Intervals;
static uint32_t Interval_Minutes;
static uint32_t Interval_Offset_Minutes;
/* Time_Synchronization_Recipients */
#endif
/* List_Of_Session_Keys */
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

/* external prototypes */
extern int Routed_Device_Read_Property_Local(BACNET_READ_PROPERTY_DATA *rpdata);
extern bool
Routed_Device_Write_Property_Local(BACNET_WRITE_PROPERTY_DATA *wp_data);

/* All included BACnet objects */
static object_functions_t Object_Table[] = {
    { OBJECT_DEVICE,
      NULL /* Init - don't init Device or it will recourse! */,
      Device_Count,
      Device_Index_To_Instance,
      Device_Valid_Object_Instance_Number,
      Device_Object_Name,
      Device_Read_Property_Local,
      NULL /* Write_Property */,
      Device_Property_Lists,
      NULL /* ReadRangeInfo */,
      NULL /* Iterator */,
      NULL /* Value_Lists */,
      NULL /* COV */,
      NULL /* COV Clear */,
      NULL /* Intrinsic Reporting */,
      NULL /* Add_List_Element */,
      NULL /* Remove_List_Element */,
      NULL /* Create */,
      NULL /* Delete */,
      NULL /* Timer */ },
#if (BACNET_PROTOCOL_REVISION >= 17)
    { OBJECT_NETWORK_PORT,
      Network_Port_Init,
      Network_Port_Count,
      Network_Port_Index_To_Instance,
      Network_Port_Valid_Instance,
      Network_Port_Object_Name,
      Network_Port_Read_Property,
      Network_Port_Write_Property,
      Network_Port_Property_Lists,
      NULL /* ReadRangeInfo */,
      NULL /* Iterator */,
      NULL /* Value_Lists */,
      NULL /* COV */,
      NULL /* COV Clear */,
      NULL /* Intrinsic Reporting */,
      NULL /* Add_List_Element */,
      NULL /* Remove_List_Element */,
      NULL /* Create */,
      NULL /* Delete */,
      NULL /* Timer */ },
#endif
    { MAX_BACNET_OBJECT_TYPE,
      NULL /* Init */,
      NULL /* Count */,
      NULL /* Index_To_Instance */,
      NULL /* Valid_Instance */,
      NULL /* Object_Name */,
      NULL /* Read_Property */,
      NULL /* Write_Property */,
      NULL /* Property_Lists */,
      NULL /* ReadRangeInfo */,
      NULL /* Iterator */,
      NULL /* Value_Lists */,
      NULL /* COV */,
      NULL /* COV Clear */,
      NULL /* Intrinsic Reporting */,
      NULL /* Add_List_Element */,
      NULL /* Remove_List_Element */,
      NULL /* Create */,
      NULL /* Delete */,
      NULL /* Timer */ },
};

/** Glue function to let the Device object, when called by a handler,
 * lookup which Object type needs to be invoked.
 * @ingroup ObjHelpers
 * @param Object_Type [in] The type of BACnet Object the handler wants to
 * access.
 * @return Pointer to the group of object helper functions that implement this
 *         type of Object.
 */
static struct object_functions *
Device_Objects_Find_Functions(BACNET_OBJECT_TYPE Object_Type)
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

/** Try to find a rr_info_function helper function for the requested object
 * type.
 * @ingroup ObjIntf
 *
 * @param object_type [in] The type of BACnet Object the handler wants to
 * access.
 * @return Pointer to the object helper function that implements the
 *         ReadRangeInfo function, Object_RR_Info, for this type of Object on
 *         success, else a NULL pointer if the type of Object isn't supported
 *         or doesn't have a ReadRangeInfo function.
 */
rr_info_function Device_Objects_RR_Info(BACNET_OBJECT_TYPE object_type)
{
    struct object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    return (pObject != NULL ? pObject->Object_RR_Info : NULL);
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
    uint32_t object_instance,
    struct special_property_list_t *pPropertyList)
{
    struct object_functions *pObject = NULL;

    (void)object_instance;
    pPropertyList->Required.pList = NULL;
    pPropertyList->Optional.pList = NULL;
    pPropertyList->Proprietary.pList = NULL;

    /* If we can find an entry for the required object type
     * and there is an Object_List_RPM fn ptr then call it
     * to populate the pointers to the individual list counters.
     */

    pObject = Device_Objects_Find_Functions(object_type);
    if ((pObject != NULL) && (pObject->Object_RPM_List != NULL)) {
        pObject->Object_RPM_List(
            &pPropertyList->Required.pList, &pPropertyList->Optional.pList,
            &pPropertyList->Proprietary.pList);
    }

    /* Fetch the counts if available otherwise zero them */
    pPropertyList->Required.count = pPropertyList->Required.pList == NULL
        ? 0
        : property_list_count(pPropertyList->Required.pList);

    pPropertyList->Optional.count = pPropertyList->Optional.pList == NULL
        ? 0
        : property_list_count(pPropertyList->Optional.pList);

    pPropertyList->Proprietary.count = pPropertyList->Proprietary.pList == NULL
        ? 0
        : property_list_count(pPropertyList->Proprietary.pList);

    return;
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
    PROP_DEVICE_ADDRESS_BINDING,
    PROP_DATABASE_REVISION,
    -1
};

static const int Device_Properties_Optional[] = {
#if defined(BACDL_MSTP)
    PROP_MAX_MASTER,  PROP_MAX_INFO_FRAMES,
#endif
    PROP_DESCRIPTION, PROP_LOCATION,        PROP_ACTIVE_COV_SUBSCRIPTIONS, -1
};

static const int Device_Properties_Proprietary[] = { -1 };

/**
 * @brief Get the Object property lists for the Device Object.
 * @param pRequired [out] The list of required properties
 * @param pOptional [out] The list of optional properties
 * @param pProprietary [out] The list of proprietary properties
 */
void Device_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Device_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Device_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Device_Properties_Proprietary;
    }

    return;
}

static BACNET_REINITIALIZED_STATE Reinitialize_State = BACNET_REINIT_IDLE;
static const char *Reinit_Password = "filister";

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
bool Device_Reinitialize(BACNET_REINITIALIZE_DEVICE_DATA *rd_data)
{
    bool status = false;

    /* Note: you could use a mix of state and password to multiple things */
    if (characterstring_ansi_same(&rd_data->password, Reinit_Password)) {
        switch (rd_data->state) {
            case BACNET_REINIT_COLDSTART:
                dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
                /* note: you probably want to restart *after* the
                   simple ack has been sent from the return handler
                   so just set a flag from here */
                Reinitialize_State = rd_data->state;
                status = true;
                break;
            case BACNET_REINIT_WARMSTART:
                dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
                /* note: restart *after* the simple ack has been sent */
                Reinitialize_State = rd_data->state;
                status = true;
                break;
            case BACNET_REINIT_STARTBACKUP:
            case BACNET_REINIT_ENDBACKUP:
            case BACNET_REINIT_STARTRESTORE:
            case BACNET_REINIT_ENDRESTORE:
            case BACNET_REINIT_ABORTRESTORE:
                if (dcc_communication_disabled()) {
                    rd_data->error_class = ERROR_CLASS_SERVICES;
                    rd_data->error_code = ERROR_CODE_COMMUNICATION_DISABLED;
                } else {
                    rd_data->error_class = ERROR_CLASS_SERVICES;
                    rd_data->error_code =
                        ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                }
                break;
            case BACNET_REINIT_ACTIVATE_CHANGES:
                /* note: activate changes *after* the simple ack is sent */
                Reinitialize_State = rd_data->state;
                status = true;
                break;
            default:
                rd_data->error_class = ERROR_CLASS_SERVICES;
                rd_data->error_code = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
                break;
        }
    } else {
        rd_data->error_class = ERROR_CLASS_SECURITY;
        rd_data->error_code = ERROR_CODE_PASSWORD_FAILURE;
    }

    return status;
}

BACNET_REINITIALIZED_STATE Device_Reinitialized_State(void)
{
    return Reinitialize_State;
}

unsigned Device_Count(void)
{
    return 1;
}

/**
 * @brief Get the instance number of the device object
 * @param index [in] The index of the device object
 * @return The instance number of the device object
 */
uint32_t Device_Index_To_Instance(unsigned index)
{
    (void)index;
    return handler_device_object_instance_number();
}

/* methods to manipulate the data */

/** Return the Object Instance number for our (single) Device Object.
 * This is a key function, widely invoked by the handler code, since
 * it provides "our" (ie, local) address.
 * @ingroup ObjIntf
 * @return The Instance number used in the BACNET_OBJECT_ID for the Device.
 */
uint32_t Device_Object_Instance_Number(void)
{
#ifdef BAC_ROUTING
    return Routed_Device_Object_Instance_Number();
#else
    return handler_device_object_instance_number();
#endif
}

/**
 * @brief Set the object instance number of the device object
 * @param object_id [in] The object instance number of the device object
 * @return True if the object instance number was set
 */
bool Device_Set_Object_Instance_Number(uint32_t object_id)
{
    bool status = true; /* return value */

    if (object_id <= BACNET_MAX_INSTANCE) {
        /* Make the change and update the database revision */
        handler_device_object_instance_number_set(object_id);
    } else {
        status = false;
    }

    return status;
}

/**
 * @brief Determine if a given device object instance number is valid
 * @param object_id [in] The object instance number of the device object
 *
 */
bool Device_Valid_Object_Instance_Number(uint32_t object_id)
{
    return (handler_device_object_instance_number() == object_id);
}

/**
 * @brief Get the object name of the device object
 * @param object_instance [in] The instance number of the device object
 * @return The object name of the device object
 */
bool Device_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;

    if (object_instance == handler_device_object_instance_number()) {
        status = characterstring_copy(object_name, &My_Object_Name);
    }

    return status;
}

bool Device_Set_Object_Name(const BACNET_CHARACTER_STRING *object_name)
{
    bool status = false; /*return value */

    if (!characterstring_same(&My_Object_Name, object_name)) {
        /* Make the change and update the database revision */
        status = characterstring_copy(&My_Object_Name, object_name);
        handler_device_object_database_revision_increment();
    }

    return status;
}

bool Device_Object_Name_ANSI_Init(const char *value)
{
    return characterstring_init_ansi(&My_Object_Name, value);
}

BACNET_DEVICE_STATUS Device_System_Status(void)
{
    return System_Status;
}

/**
 * @brief Set the system status of the device object
 * @param status [in] The system status of the device object
 * @param local [in] True if the value is local
 * @return 0 if the value was set, -1 if the value was bad,
 *  -2 if the value was not allowed
 */
int Device_Set_System_Status(BACNET_DEVICE_STATUS status, bool local)
{
    int result = 0; /*return value - 0 = ok, -1 = bad value, -2 = not allowed */

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

/**
 * @brief Get the vendor name of the device object
 * @return The vendor name of the device object
 */
const char *Device_Vendor_Name(void)
{
    return Vendor_Name;
}

/** Returns the Vendor ID for this Device.
 * See the assignments at
 * http://www.bacnet.org/VendorID/BACnet%20Vendor%20IDs.htm
 * @return The Vendor ID of this Device.
 */
uint16_t Device_Vendor_Identifier(void)
{
    return handler_device_vendor_identifier();
}

/**
 * @brief Set the vendor identifier of the device object
 * @param vendor_id [in] The vendor identifier of the device object
 */
void Device_Set_Vendor_Identifier(uint16_t vendor_id)
{
    handler_device_vendor_identifier_set(vendor_id);
}

/**
 * @brief Get the model name of the device object
 * @return The model name of the device object
 */
const char *Device_Model_Name(void)
{
    return Model_Name;
}

/**
 * @brief Set the model name of the device object
 * @param name [in] The model name of the device object
 * @param length [in] The length of the model name
 * @return True if the model name was set
 */
bool Device_Set_Model_Name(const char *name, size_t length)
{
    bool status = false; /*return value */

    if (length < sizeof(Model_Name)) {
        memmove(Model_Name, name, length);
        Model_Name[length] = 0;
        status = true;
    }

    return status;
}

/**
 * @brief Get the firmware revision of the device object
 * @return The firmware revision of the device object
 */
const char *Device_Firmware_Revision(void)
{
    return BACnet_Version;
}

/**
 * @brief Get the application software version of the device object
 * @return the application software version of the device object
 */
const char *Device_Application_Software_Version(void)
{
    return Application_Software_Version;
}

/**
 * @brief Set the application software version of the device object
 * @param name [in] The application software version of the device object
 * @param length [in] The length of the application software version
 * @return True if the application software version was set
 */
bool Device_Set_Application_Software_Version(const char *name, size_t length)
{
    bool status = false; /*return value */

    if (length < sizeof(Application_Software_Version)) {
        memmove(Application_Software_Version, name, length);
        Application_Software_Version[length] = 0;
        status = true;
    }

    return status;
}

/**
 * @brief Get the description of the device object
 * @return The description of the device object
 *
 */
const char *Device_Description(void)
{
    return Description;
}

/**
 * @brief Set the description of the device object
 * @param name [in] The description of the device object
 * @param length [in] The length of the description
 * @return True if the description was set
 */
bool Device_Set_Description(const char *name, size_t length)
{
    bool status = false; /*return value */

    if (length < sizeof(Description)) {
        memmove(Description, name, length);
        Description[length] = 0;
        status = true;
    }

    return status;
}

/**
 * @brief Get the location of the device object
 * @return The location of the device object
 */
const char *Device_Location(void)
{
    return Location;
}

/**
 * @brief Set the location of the device object
 * @param name [in] The location of the device object
 * @param length [in] The length of the location
 * @return True if the location was set
 */
bool Device_Set_Location(const char *name, size_t length)
{
    bool status = false; /*return value */

    if (length < sizeof(Location)) {
        memmove(Location, name, length);
        Location[length] = 0;
        status = true;
    }

    return status;
}

/**
 * @brief Get the device protocol version value
 * @return The device protocol version value
 */
uint8_t Device_Protocol_Version(void)
{
    return BACNET_PROTOCOL_VERSION;
}

/**
 * @brief Get the device protocol revision value
 * @return The device protocol revision value
 */
uint8_t Device_Protocol_Revision(void)
{
    return BACNET_PROTOCOL_REVISION;
}

/**
 * @brief Get the segmented message supported enumeration
 * @return The segmented message supported enumeration
 */
BACNET_SEGMENTATION Device_Segmentation_Supported(void)
{
    return SEGMENTATION_NONE;
}

/**
 * @brief Get the device database revision value
 * @return The device database revision value
 */
uint32_t Device_Database_Revision(void)
{
    return handler_device_object_database_revision();
}

/**
 * @brief Set the device database revision value
 * @param revision [in] The device database revision value
 */
void Device_Set_Database_Revision(uint32_t revision)
{
    handler_device_object_database_revision_set(revision);
}

/**
 * @brief Shortcut for incrementing database revision as this is potentially
 * the most common operation if changing object names and ids is
 * implemented.
 */
void Device_Inc_Database_Revision(void)
{
    handler_device_object_database_revision_increment();
}

/**
 * @brief Update the current time and date variables
 */
int Device_Object_List_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_OBJECT_TYPE object_type;
    uint32_t instance;
    bool found;

    if (object_instance == Device_Object_Instance_Number()) {
        /* single element is zero based, add 1 for BACnetARRAY which is one
         * based */
        array_index++;
        found =
            Device_Object_List_Identifier(array_index, &object_type, &instance);
        if (found) {
            apdu_len =
                encode_application_object_id(apdu, object_type, instance);
        }
    }

    return apdu_len;
}

/** Determine if we have an object with the given object_name.
 * If the object_type and object_instance pointers are not null,
 * and the lookup succeeds, they will be given the resulting values.
 * @param object_name [in] The desired Object Name to look for.
 * @param object_type [out] The BACNET_OBJECT_TYPE of the matching Object.
 * @param object_instance [out] The object instance number of the matching
 * Object.
 * @return True on success or else False if not found.
 */
bool Device_Valid_Object_Name(
    const BACNET_CHARACTER_STRING *object_name1,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance)
{
    bool found = false;
    BACNET_OBJECT_TYPE type = OBJECT_NONE;
    uint32_t instance;
    uint32_t max_objects = 0, i = 0;
    bool check_id = false;
    BACNET_CHARACTER_STRING object_name2;
    struct object_functions *pObject = NULL;

    max_objects = Device_Object_List_Count();
    for (i = 1; i <= max_objects; i++) {
        check_id = Device_Object_List_Identifier(i, &type, &instance);
        if (check_id) {
            pObject = Device_Objects_Find_Functions(type);
            if ((pObject != NULL) && (pObject->Object_Name != NULL) &&
                (pObject->Object_Name(instance, &object_name2) &&
                 characterstring_same(object_name1, &object_name2))) {
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
 * @return True if found, else False if no such Object in this device.
 */
bool Device_Valid_Object_Id(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    bool status = false; /* return value */
    struct object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if ((pObject != NULL) && (pObject->Object_Valid_Instance != NULL)) {
        status = pObject->Object_Valid_Instance(object_instance);
    }

    return status;
}

/** Copy a child object's object_name value, given its ID.
 * @param object_type [in] The BACNET_OBJECT_TYPE of the child Object.
 * @param object_instance [in] The object instance number of the child Object.
 * @param object_name [out] The Object Name found for this child Object.
 * @return True on success or else False if not found.
 */
bool Device_Object_Name_Copy(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    struct object_functions *pObject = NULL;
    bool found = false;

    pObject = Device_Objects_Find_Functions(object_type);
    if ((pObject != NULL) && (pObject->Object_Name != NULL)) {
        found = pObject->Object_Name(object_instance, object_name);
    }

    return found;
}

static void Update_Current_Time(void)
{
    datetime_local(
        &Local_Date, &Local_Time, &UTC_Offset, &Daylight_Savings_Status);
}

/**
 * @brief Get the current date and time
 * @param DateTime [out] The current date and time
 */
void Device_getCurrentDateTime(BACNET_DATE_TIME *DateTime)
{
    Update_Current_Time();

    DateTime->date = Local_Date;
    DateTime->time = Local_Time;
}

/**
 * @brief Get the current UTC offset
 * @return The current UTC offset
 */
int32_t Device_UTC_Offset(void)
{
    Update_Current_Time();

    return UTC_Offset;
}

/**
 * @brief Get the current daylight savings status
 * @return The current daylight savings status
 */
bool Device_Daylight_Savings_Status(void)
{
    return Daylight_Savings_Status;
}

#if defined(BACNET_TIME_MASTER)
/**
 * Sets the time sync interval in minutes
 *
 * @param flag
 * This property, of type BOOLEAN, specifies whether (TRUE)
 * or not (FALSE) clock-aligned periodic time synchronization is
 * enabled. If periodic time synchronization is enabled and the
 * time synchronization interval is a factor of (divides without
 * remainder) an hour or day, then the beginning of the period
 * specified for time synchronization shall be aligned to the hour or
 * day, respectively. If this property is present, it shall be writable.
 */
bool Device_Align_Intervals_Set(bool flag)
{
    Align_Intervals = flag;

    return true;
}

/**
 * @brief Get the Align-Intervals flag
 * @return The Align-Intervals flag
 */
bool Device_Align_Intervals(void)
{
    return Align_Intervals;
}

/**
 * Sets the time sync interval in minutes
 *
 * @param minutes
 * This property, of type Unsigned, specifies the periodic
 * interval in minutes at which TimeSynchronization and
 * UTCTimeSynchronization requests shall be sent. If this
 * property has a value of zero, then periodic time synchronization is
 * disabled. If this property is present, it shall be writable.
 */
bool Device_Time_Sync_Interval_Set(uint32_t minutes)
{
    Interval_Minutes = minutes;

    return true;
}

/**
 * @brief Get the time sync interval in minutes
 * @return The time sync interval in minutes
 */
uint32_t Device_Time_Sync_Interval(void)
{
    return Interval_Minutes;
}

/**
 * Sets the time sync interval offset value.
 *
 * @param minutes
 * This property, of type Unsigned, specifies the offset in
 * minutes from the beginning of the period specified for time
 * synchronization until the actual time synchronization requests
 * are sent. The offset used shall be the value of Interval_Offset
 * modulo the value of Time_Synchronization_Interval;
 * e.g., if Interval_Offset has the value 31 and
 * Time_Synchronization_Interval is 30, the offset used shall be 1.
 * Interval_Offset shall have no effect if Align_Intervals is
 * FALSE. If this property is present, it shall be writable.
 */
bool Device_Interval_Offset_Set(uint32_t minutes)
{
    Interval_Offset_Minutes = minutes;

    return true;
}

/**
 * @brief Get the TimeSync interval offset minutes
 * @return The TimeSync interval offset minutes
 */
uint32_t Device_Interval_Offset(void)
{
    return Interval_Offset_Minutes;
}
#endif

/* return the length of the apdu encoded or BACNET_STATUS_ERROR for error or
   BACNET_STATUS_ABORT for abort message */
int Device_Read_Property_Local(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint32_t count = 0;
    uint8_t *apdu = NULL;
    int apdu_size = 0;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_DEVICE, Device_Object_Instance_Number());
            break;
        case PROP_OBJECT_NAME:
            apdu_len =
                encode_application_character_string(&apdu[0], &My_Object_Name);
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
            apdu_len = encode_application_unsigned(
                &apdu[0], Device_Vendor_Identifier());
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
            characterstring_init_ansi(
                &char_string, Application_Software_Version);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_LOCATION:
            characterstring_init_ansi(&char_string, Location);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_PROTOCOL_VERSION:
            apdu_len = encode_application_unsigned(
                &apdu[0], Device_Protocol_Version());
            break;
        case PROP_PROTOCOL_REVISION:
            apdu_len = encode_application_unsigned(
                &apdu[0], Device_Protocol_Revision());
            break;
        case PROP_PROTOCOL_SERVICES_SUPPORTED:
            /* Note: list of services that are executed, not initiated. */
            bitstring_init(&bit_string);
            for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++) {
                /* automatic lookup based on handlers set */
                bitstring_set_bit(
                    &bit_string, (uint8_t)i,
                    apdu_service_supported((BACNET_SERVICES_SUPPORTED)i));
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
            handler_device_object_types_supported(&bit_string);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OBJECT_LIST:
            count = Device_Object_List_Count();
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Device_Object_List_Element_Encode, count, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_MAX_APDU_LENGTH_ACCEPTED:
            apdu_len = encode_application_unsigned(&apdu[0], MAX_APDU);
            break;
        case PROP_SEGMENTATION_SUPPORTED:
            apdu_len = encode_application_enumerated(
                &apdu[0], Device_Segmentation_Supported());
            break;
        case PROP_APDU_TIMEOUT:
            apdu_len = encode_application_unsigned(&apdu[0], apdu_timeout());
            break;
        case PROP_NUMBER_OF_APDU_RETRIES:
            apdu_len = encode_application_unsigned(&apdu[0], apdu_retries());
            break;
        case PROP_DEVICE_ADDRESS_BINDING:
#if (MAX_ADDRESS_CACHE > 0)
            apdu_len = address_list_encode(&apdu[0], apdu_size);
#endif
            break;
        case PROP_DATABASE_REVISION:
            apdu_len = encode_application_unsigned(
                &apdu[0], handler_device_object_database_revision());
            break;
#if defined(BACDL_MSTP)
        case PROP_MAX_INFO_FRAMES:
            apdu_len =
                encode_application_unsigned(&apdu[0], dlmstp_max_info_frames());
            break;
        case PROP_MAX_MASTER:
            apdu_len =
                encode_application_unsigned(&apdu[0], dlmstp_max_master());
            break;
#endif
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

/** Looks up the requested Object and Property, and encodes its Value in an
 * APDU.
 * @ingroup ObjIntf
 * If the Object or Property can't be found, sets the error class and code.
 *
 * @param rpdata [in,out] Structure with the desired Object and Property info
 *                 on entry, and APDU message on return.
 * @return The length of the APDU on success, else BACNET_STATUS_ERROR
 */
int Device_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = BACNET_STATUS_ERROR;
    struct object_functions *pObject = NULL;
#if (BACNET_PROTOCOL_REVISION >= 14)
    struct special_property_list_t property_list;
#endif

    /* initialize the default return values */
    rpdata->error_class = ERROR_CLASS_OBJECT;
    rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    pObject = Device_Objects_Find_Functions(rpdata->object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(rpdata->object_instance)) {
            if (pObject->Object_Read_Property) {
#if (BACNET_PROTOCOL_REVISION >= 14)
                if ((int)rpdata->object_property == PROP_PROPERTY_LIST) {
                    Device_Objects_Property_List(
                        rpdata->object_type, rpdata->object_instance,
                        &property_list);
                    apdu_len = property_list_encode(
                        rpdata, property_list.Required.pList,
                        property_list.Optional.pList,
                        property_list.Proprietary.pList);
                } else
#endif
                {
                    apdu_len = pObject->Object_Read_Property(rpdata);
                }
            }
        } else {
            rpdata->error_class = ERROR_CLASS_OBJECT;
            rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
    } else {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return apdu_len;
}

/**
 * @brief Updates all the object timers with elapsed milliseconds
 * @param milliseconds - number of milliseconds elapsed
 */
void Device_Timer(uint16_t milliseconds)
{
    handler_device_timer(milliseconds);
}

/**
 * @brief Initialize the Device Object.
 *  Initialize the group of object helper functions for any supported Object.
 *  Initialize each of the Device Object child Object instances.
 * @ingroup ObjIntf
 * @param object_table [in,out] array of structure with object functions.
 *  Each Child Object must provide some implementation of each of these
 *  functions in order to properly support the default handlers.
 */
void Device_Init(object_functions_t *object_table)
{
    characterstring_init_ansi(&My_Object_Name, "SimpleClient");
    datetime_init();
    if (object_table) {
        handler_device_object_table_set(object_table);
    } else {
        handler_device_object_table_set(&Object_Table[0]);
    }
    handler_device_object_init();
}
