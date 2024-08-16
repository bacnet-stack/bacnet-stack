/**
 * @file
 * @brief Base "class" for handling all BACnet objects belonging
 *  to a BACnet device, as well as Device-specific properties.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"
#include "bacnet/dcc.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/property.h"
#include "bacnet/version.h"
#include "bacnet/basic/services.h"
/* objects */
#include "bacnet/basic/object/acc.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/object/calendar.h"
#include "bacnet/basic/object/command.h"
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/object/lsp.h"
#include "bacnet/basic/object/lsz.h"
#include "bacnet/basic/object/ms-input.h"
#include "bacnet/basic/object/mso.h"
#include "bacnet/basic/object/msv.h"
#include "bacnet/basic/object/schedule.h"
#include "bacnet/basic/object/structured_view.h"
#include "bacnet/basic/object/trendlog.h"
#if defined(INTRINSIC_REPORTING)
#include "bacnet/basic/object/nc.h"
#endif /* defined(INTRINSIC_REPORTING) */
#if defined(BACFILE)
#include "bacnet/basic/object/bacfile.h"
#endif /* defined(BACFILE) */
#if (BACNET_PROTOCOL_REVISION >= 10)
#include "bacnet/basic/object/bitstring_value.h"
#include "bacnet/basic/object/csv.h"
#include "bacnet/basic/object/iv.h"
#include "bacnet/basic/object/osv.h"
#include "bacnet/basic/object/piv.h"
#include "bacnet/basic/object/time_value.h"
#endif
#if (BACNET_PROTOCOL_REVISION >= 14)
#include "bacnet/basic/object/channel.h"
#include "bacnet/basic/object/lo.h"
#endif
#if (BACNET_PROTOCOL_REVISION >= 16)
#include "bacnet/basic/object/blo.h"
#endif
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif
#if (BACNET_PROTOCOL_REVISION >= 24)
#include "bacnet/basic/object/color_object.h"
#include "bacnet/basic/object/color_temperature.h"
#endif
#include "bacnet/basic/object/device.h"

#ifdef CONFIG_BACNET_BASIC_DEVICE_OBJECT_VERSION
#define BACNET_DEVICE_VERSION CONFIG_BACNET_BASIC_DEVICE_OBJECT_VERSION
#else
#define BACNET_DEVICE_VERSION "1.0.0"
#endif

#ifdef CONFIG_BACNET_BASIC_DEVICE_OBJECT_NAME
#define BACNET_DEVICE_OBJECT_NAME CONFIG_BACNET_BASIC_DEVICE_OBJECT_NAME
#else
#define BACNET_DEVICE_OBJECT_NAME "BACnet Basic Device"
#endif

#ifdef CONFIG_BACNET_BASIC_DEVICE_DESCRIPTION
#define BACNET_DEVICE_DESCRIPTION CONFIG_BACNET_BASIC_DEVICE_DESCRIPTION
#else
#define BACNET_DEVICE_DESCRIPTION "BACnet Basic Server Device"
#endif

#ifdef CONFIG_BACNET_BASIC_DEVICE_MODEL_NAME
#define BACNET_DEVICE_MODEL_NAME CONFIG_BACNET_BASIC_DEVICE_MODEL_NAME
#else
#define BACNET_DEVICE_MODEL_NAME "GNU Basic Server Model 42"
#endif

static object_functions_t Object_Table[] = { 
    { OBJECT_DEVICE, NULL, /* don't init - recursive! */
        Device_Count, Device_Index_To_Instance,
        Device_Valid_Object_Instance_Number,
        Device_Object_Name, Device_Read_Property_Local,
        Device_Write_Property_Local, Device_Property_Lists, 
        NULL /* ReadRangeInfo */, NULL /* Iterator */,
        NULL /* Value_Lists */, NULL /* COV */, NULL /* COV Clear */,
        NULL /* Intrinsic Reporting */, NULL /* Add_List_Element */,
        NULL /* Remove_List_Element */, NULL /* Create */, NULL /* Delete */,
        NULL /* Timer */ },
#if defined (CONFIG_BACNET_BASIC_OBJECT_ANALOG_INPUT)
    { OBJECT_ANALOG_INPUT, Analog_Input_Init, Analog_Input_Count,
        Analog_Input_Index_To_Instance, Analog_Input_Valid_Instance,
        Analog_Input_Object_Name, Analog_Input_Read_Property,
        Analog_Input_Write_Property, Analog_Input_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */,
        Analog_Input_Encode_Value_List, Analog_Input_Change_Of_Value,
        Analog_Input_Change_Of_Value_Clear, Analog_Input_Intrinsic_Reporting,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Analog_Input_Create, Analog_Input_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_ANALOG_OUTPUT)
    { OBJECT_ANALOG_OUTPUT, Analog_Output_Init, Analog_Output_Count,
        Analog_Output_Index_To_Instance, Analog_Output_Valid_Instance,
        Analog_Output_Object_Name, Analog_Output_Read_Property,
        Analog_Output_Write_Property, Analog_Output_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, 
        Analog_Output_Encode_Value_List, Analog_Output_Change_Of_Value,
        Analog_Output_Change_Of_Value_Clear, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Analog_Output_Create, Analog_Output_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_ANALOG_VALUE)
    { OBJECT_ANALOG_VALUE, Analog_Value_Init, Analog_Value_Count,
        Analog_Value_Index_To_Instance, Analog_Value_Valid_Instance,
        Analog_Value_Object_Name, Analog_Value_Read_Property,
        Analog_Value_Write_Property, Analog_Value_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */,
        Analog_Value_Encode_Value_List, Analog_Value_Change_Of_Value,
        Analog_Value_Change_Of_Value_Clear, Analog_Value_Intrinsic_Reporting,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Analog_Value_Create, Analog_Value_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_BINARY_INPUT)
    { OBJECT_BINARY_INPUT, Binary_Input_Init, Binary_Input_Count,
        Binary_Input_Index_To_Instance, Binary_Input_Valid_Instance,
        Binary_Input_Object_Name, Binary_Input_Read_Property,
        Binary_Input_Write_Property, Binary_Input_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */,
        Binary_Input_Encode_Value_List, Binary_Input_Change_Of_Value,
        Binary_Input_Change_Of_Value_Clear, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Binary_Input_Create, Binary_Input_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_BINARY_OUTPUT)
    { OBJECT_BINARY_OUTPUT, Binary_Output_Init, Binary_Output_Count,
        Binary_Output_Index_To_Instance, Binary_Output_Valid_Instance,
        Binary_Output_Object_Name, Binary_Output_Read_Property,
        Binary_Output_Write_Property, Binary_Output_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, 
        Binary_Output_Encode_Value_List, Binary_Output_Change_Of_Value,
        Binary_Output_Change_Of_Value_Clear, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Binary_Output_Create, Binary_Output_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_BINARY_VALUE)
    { OBJECT_BINARY_VALUE, Binary_Value_Init, Binary_Value_Count,
        Binary_Value_Index_To_Instance, Binary_Value_Valid_Instance,
        Binary_Value_Object_Name, Binary_Value_Read_Property,
        Binary_Value_Write_Property, Binary_Value_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, 
        Binary_Value_Encode_Value_List, Binary_Value_Change_Of_Value,
        Binary_Value_Change_Of_Value_Clear, 
        NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Binary_Value_Create, Binary_Value_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_MULTISTATE_INPUT)
    { OBJECT_MULTI_STATE_INPUT, Multistate_Input_Init, Multistate_Input_Count,
        Multistate_Input_Index_To_Instance, Multistate_Input_Valid_Instance,
        Multistate_Input_Object_Name, Multistate_Input_Read_Property,
        Multistate_Input_Write_Property, Multistate_Input_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, 
        Multistate_Input_Encode_Value_List, Multistate_Input_Change_Of_Value,
        Multistate_Input_Change_Of_Value_Clear, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Multistate_Input_Create, Multistate_Input_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_MULTISTATE_OUTPUT)
    { OBJECT_MULTI_STATE_OUTPUT, Multistate_Output_Init,
        Multistate_Output_Count, Multistate_Output_Index_To_Instance,
        Multistate_Output_Valid_Instance, Multistate_Output_Object_Name,
        Multistate_Output_Read_Property, Multistate_Output_Write_Property,
        Multistate_Output_Property_Lists, NULL /* ReadRangeInfo */,
        NULL /* Iterator */, 
        Multistate_Output_Encode_Value_List, Multistate_Output_Change_Of_Value,
        Multistate_Output_Change_Of_Value_Clear, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Multistate_Output_Create, Multistate_Output_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_MULTISTATE_VALUE)
    { OBJECT_MULTI_STATE_VALUE, Multistate_Value_Init, Multistate_Value_Count,
        Multistate_Value_Index_To_Instance, Multistate_Value_Valid_Instance,
        Multistate_Value_Object_Name, Multistate_Value_Read_Property,
        Multistate_Value_Write_Property, Multistate_Value_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */,
        Multistate_Value_Encode_Value_List, Multistate_Value_Change_Of_Value,
        Multistate_Value_Change_Of_Value_Clear, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Multistate_Value_Create, Multistate_Value_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_NETWORK_PORT)
#if (BACNET_PROTOCOL_REVISION >= 17)
    { OBJECT_NETWORK_PORT, Network_Port_Init, Network_Port_Count,
        Network_Port_Index_To_Instance, Network_Port_Valid_Instance,
        Network_Port_Object_Name, Network_Port_Read_Property,
        Network_Port_Write_Property, Network_Port_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        NULL /* Create */, NULL /* Delete */, NULL /* Timer */  },
#else
#warning "Network Port is configured, but BACnet Protocol Revision < 17"
#endif
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_CALENDAR)
    { OBJECT_CALENDAR, Calendar_Init, Calendar_Count,
        Calendar_Index_To_Instance, Calendar_Valid_Instance,
        Calendar_Object_Name, Calendar_Read_Property,
        Calendar_Write_Property, Calendar_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Calendar_Create, Calendar_Delete, NULL /* Timer */ },
        NULL /* Create */, NULL /* Delete */, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_LIFE_SAFETY_POINT)
    { OBJECT_LIFE_SAFETY_POINT, Life_Safety_Point_Init, Life_Safety_Point_Count,
        Life_Safety_Point_Index_To_Instance, Life_Safety_Point_Valid_Instance,
        Life_Safety_Point_Object_Name, Life_Safety_Point_Read_Property,
        Life_Safety_Point_Write_Property, Life_Safety_Point_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Life_Safety_Point_Create, Life_Safety_Point_Delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_LIFE_SAFETY_ZONE)
    { OBJECT_LIFE_SAFETY_ZONE, Life_Safety_Zone_Init, Life_Safety_Zone_Count,
        Life_Safety_Zone_Index_To_Instance, Life_Safety_Zone_Valid_Instance,
        Life_Safety_Zone_Object_Name, Life_Safety_Zone_Read_Property,
        Life_Safety_Zone_Write_Property, Life_Safety_Zone_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Life_Safety_Zone_Create, Life_Safety_Zone_Delete, NULL /* Timer */ },
#endif
#if (BACNET_PROTOCOL_REVISION >= 14)
#if defined (CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT)
    { OBJECT_LIGHTING_OUTPUT, Lighting_Output_Init, Lighting_Output_Count,
        Lighting_Output_Index_To_Instance, Lighting_Output_Valid_Instance,
        Lighting_Output_Object_Name, Lighting_Output_Read_Property,
        Lighting_Output_Write_Property, Lighting_Output_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Lighting_Output_Create, Lighting_Output_Delete, Lighting_Output_Timer },
#endif
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_CHANNEL)
    { OBJECT_CHANNEL, Channel_Init, Channel_Count, Channel_Index_To_Instance,
        Channel_Valid_Instance, Channel_Object_Name, Channel_Read_Property,
        Channel_Write_Property, Channel_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Channel_Create, Channel_Delete, NULL /* Timer */ },
#endif
#if (BACNET_PROTOCOL_REVISION >= 16)
#if defined (CONFIG_BACNET_BASIC_OBJECT_BINARY_LIGHTING_OUTPUT)
    { OBJECT_BINARY_LIGHTING_OUTPUT, Binary_Lighting_Output_Init,
        Binary_Lighting_Output_Count, Binary_Lighting_Output_Index_To_Instance,
        Binary_Lighting_Output_Valid_Instance,
        Binary_Lighting_Output_Object_Name,
        Binary_Lighting_Output_Read_Property,
        Binary_Lighting_Output_Write_Property,
        Binary_Lighting_Output_Property_Lists, NULL /* ReadRangeInfo */,
        NULL /* Iterator */, NULL /* Value_Lists */, NULL /* COV */,
        NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Binary_Lighting_Output_Create, Binary_Lighting_Output_Delete,
        Binary_Lighting_Output_Timer },
#endif
#endif
#if (BACNET_PROTOCOL_REVISION >= 24)
#if defined (CONFIG_BACNET_BASIC_OBJECT_COLOR)
    { OBJECT_COLOR, Color_Init, Color_Count, Color_Index_To_Instance,
        Color_Valid_Instance, Color_Object_Name, Color_Read_Property,
        Color_Write_Property, Color_Property_Lists, NULL /* ReadRangeInfo */,
        NULL /* Iterator */, NULL /* Value_Lists */, NULL /* COV */,
        NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Color_Create, Color_Delete, Color_Timer },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_COLOR_TEMPERATURE)
    { OBJECT_COLOR_TEMPERATURE, Color_Temperature_Init, Color_Temperature_Count,
        Color_Temperature_Index_To_Instance, Color_Temperature_Valid_Instance,
        Color_Temperature_Object_Name, Color_Temperature_Read_Property,
        Color_Temperature_Write_Property, Color_Temperature_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Color_Temperature_Create, Color_Temperature_Delete,
        Color_Temperature_Timer },
#endif
#endif
#if defined(CONFIG_BACNET_BASIC_OBJECT_FILE)
    { OBJECT_FILE, bacfile_init, bacfile_count, bacfile_index_to_instance,
        bacfile_valid_instance, bacfile_object_name, bacfile_read_property,
        bacfile_write_property, BACfile_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        bacfile_create, bacfile_delete, NULL /* Timer */ },
#endif
#if defined (CONFIG_BACNET_BASIC_OBJECT_STRUCTURED_VIEW)
    { OBJECT_STRUCTURED_VIEW, Structured_View_Init, Structured_View_Count,
        Structured_View_Index_To_Instance, Structured_View_Valid_Instance,
        Structured_View_Object_Name, Structured_View_Read_Property,
        NULL /* Write_Property */, Structured_View_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */,  NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        Structured_View_Create, Structured_View_Delete, NULL /* Timer */ },
#endif
    { MAX_BACNET_OBJECT_TYPE, NULL /* Init */, NULL /* Count */,
        NULL /* Index_To_Instance */, NULL /* Valid_Instance */,
        NULL /* Object_Name */, NULL /* Read_Property */,
        NULL /* Write_Property */, NULL /* Property_Lists */,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        NULL /* Create */, NULL /* Delete */, NULL /* Timer */ }
};

/* local data */
static const char *Application_Software_Version = BACNET_DEVICE_VERSION;
static uint32_t Object_Instance_Number = BACNET_MAX_INSTANCE;
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static BACNET_CHARACTER_STRING My_Object_Name;
static const char *Device_Name_Default = BACNET_DEVICE_OBJECT_NAME;
static const char *Device_Description_Default = BACNET_DEVICE_DESCRIPTION;
static const char *Model_Name = BACNET_DEVICE_MODEL_NAME;
static uint32_t Database_Revision;
static BACNET_REINITIALIZED_STATE Reinitialize_State = BACNET_REINIT_IDLE;
static BACNET_CHARACTER_STRING Reinit_Password;
static const char *BACnet_Version = BACNET_VERSION_TEXT;
static write_property_function Device_Write_Property_Store_Callback;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Device_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_SYSTEM_STATUS, PROP_VENDOR_NAME,
    PROP_VENDOR_IDENTIFIER, PROP_MODEL_NAME, PROP_FIRMWARE_REVISION,
    PROP_APPLICATION_SOFTWARE_VERSION, PROP_PROTOCOL_VERSION,
    PROP_PROTOCOL_REVISION, PROP_PROTOCOL_SERVICES_SUPPORTED,
    PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED, PROP_OBJECT_LIST,
    PROP_MAX_APDU_LENGTH_ACCEPTED, PROP_SEGMENTATION_SUPPORTED,
    PROP_APDU_TIMEOUT, PROP_NUMBER_OF_APDU_RETRIES, PROP_DEVICE_ADDRESS_BINDING,
    PROP_DATABASE_REVISION, -1 };

static const int Device_Properties_Optional[] = { PROP_DESCRIPTION,
    PROP_LOCATION, 
#if defined(BACDL_MSTP)
    PROP_MAX_MASTER, PROP_MAX_INFO_FRAMES, 
#endif
    -1 };

static const int Device_Properties_Proprietary[] = { -1 };

/** Glue function to let the Device object, when called by a handler,
 * lookup which Object type needs to be invoked.
 * @ingroup ObjHelpers
 * @param Object_Type [in] The type of BACnet Object the handler wants to
 * access.
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
void Device_Objects_Property_List(BACNET_OBJECT_TYPE object_type,
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
        pObject->Object_RPM_List(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, &pPropertyList->Proprietary.pList);
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

/**
 * @brief Determine if the object property is a member of this object instance
 * @param object_instance - object-instance number of the object
 * @param object_property - object-property to be checked
 * @return true if the property is a member of this object instance
 */
static bool Property_List_Member(
    uint32_t object_instance, int object_property)
{
    bool found = false;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;

    (void)object_instance;
    Device_Property_Lists(
        &pRequired, &pOptional, &pProprietary);
    found = property_list_member(pRequired, object_property);
    if (!found) {
        found = property_list_member(pOptional, object_property);
    }
    if (!found) {
        found = property_list_member(pProprietary, object_property);
    }

    return found;
}

/**
 * @brief Determine if the object property is a member of this object instance
 * @param object_type - object type of the object
 * @param object_instance - object-instance number of the object
 * @param object_property - object-property to be checked
 * @return true if the property is a member of this object instance
 */
bool Device_Objects_Property_List_Member(BACNET_OBJECT_TYPE object_type,
                                         uint32_t object_instance,
                                         int object_property)
{
    bool found = false;
    struct special_property_list_t property_list = { 0 };

    Device_Objects_Property_List(object_type, object_instance, &property_list);
    found = property_list_member(property_list.Required.pList, object_property);
    if (!found) {
        found =
            property_list_member(property_list.Optional.pList, object_property);
    }
    if (!found) {
        found = property_list_member(property_list.Proprietary.pList,
                                     object_property);
    }

    return found;
}

/**
 * @brief Sets the ReinitializeDevice password
 *
 * The password shall be a null terminated C string of up to
 * 20 ASCII characters for those devices that require the password.
 *
 * For those devices that do not require a password, set to NULL or
 * point to a zero length C string (null terminated).
 *
 * @param the ReinitializeDevice password; can be NULL or empty string
 */
bool Device_Reinitialize_Password_Set(const char *password)
{
    characterstring_init_ansi(&Reinit_Password, password);

    return true;
}

/**
 * @brief Commands a Device re-initialization, to a given state.
 *  The request's password must match for the operation to succeed.
 *  This implementation provides a framework, but doesn't
 *  actually *DO* anything.
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
    bool password_success = false;

    /* From 16.4.1.1.2 Password
        This optional parameter shall be a CharacterString of up to
        20 characters. For those devices that require the password as a
        protection, the service request shall be denied if the parameter
        is absent or if the password is incorrect. For those devices that
        do not require a password, this parameter shall be ignored.*/
    if (characterstring_length(&Reinit_Password) > 0) {
        if (characterstring_length(&rd_data->password) > 20) {
            rd_data->error_class = ERROR_CLASS_SERVICES;
            rd_data->error_code = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
        } else if (characterstring_same(
                       &rd_data->password, &Reinit_Password)) {
            password_success = true;
        } else {
            rd_data->error_class = ERROR_CLASS_SECURITY;
            rd_data->error_code = ERROR_CODE_PASSWORD_FAILURE;
        }
    } else {
        password_success = true;
    }
    if (password_success) {
        switch (rd_data->state) {
            case BACNET_REINIT_COLDSTART:
            case BACNET_REINIT_WARMSTART:
                dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
                /* Note: you could use a mix of state
                   and password to multiple things */
                /* note: you probably want to restart *after* the
                   simple ack has been sent from the return handler
                   so just set a flag from here */
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
            default:
                rd_data->error_class = ERROR_CLASS_SERVICES;
                rd_data->error_code = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
                break;
        }
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

uint32_t Device_Index_To_Instance(unsigned index)
{
    (void)index;
    return Object_Instance_Number;
}

bool Device_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;

    if (object_instance == Object_Instance_Number) {
        status = characterstring_copy(object_name, &My_Object_Name);
    }

    return status;
}

bool Device_Set_Object_Name(BACNET_CHARACTER_STRING *object_name)
{
    bool status = false; /*return value */

    if (!characterstring_same(&My_Object_Name, object_name)) {
        /* Make the change and update the database revision */
        status = characterstring_copy(&My_Object_Name, object_name);
        Device_Inc_Database_Revision();
    }

    return status;
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
    return Object_Instance_Number;
}

bool Device_Set_Object_Instance_Number(uint32_t object_id)
{
    bool status = true; /* return value */

    if (object_id <= BACNET_MAX_INSTANCE) {
        Object_Instance_Number = object_id;
    } else
        status = false;

    return status;
}

bool Device_Valid_Object_Instance_Number(uint32_t object_id)
{
    return (Object_Instance_Number == object_id);
}

BACNET_DEVICE_STATUS Device_System_Status(void)
{
    return System_Status;
}

int Device_Set_System_Status(BACNET_DEVICE_STATUS status, bool local)
{
    /*return value - 0 = ok, -1 = bad value, -2 = not allowed */
    int result = -1;

    (void)local;
    if (status < MAX_DEVICE_STATUS) {
        System_Status = status;
        result = 0;
    }

    return result;
}

uint16_t Device_Vendor_Identifier(void)
{
    return BACNET_VENDOR_ID;
}

BACNET_SEGMENTATION Device_Segmentation_Supported(void)
{
    return SEGMENTATION_NONE;
}

/**
 * @brief Get the Database Revision property of the Device Object
 * @return The Database Revision property of the Device Object
 */
uint32_t Device_Database_Revision(void)
{
    return Database_Revision;
}

/**
 * @brief Set the Database Revision property of the Device Object
 * @param revision [in] The new value for the Database Revision property
 */
void Device_Set_Database_Revision(uint32_t revision)
{
    Database_Revision = revision;
}

/*
 * Shortcut for incrementing database revision as this is potentially
 * the most common operation if changing object names and ids is
 * implemented.
 */
void Device_Inc_Database_Revision(void)
{
    Database_Revision++;
}

/** Get the total count of objects supported by this Device Object.
 * @note Since many network clients depend on the object list
 *       for discovery, it must be consistent!
 * @return The count of objects, for all supported Object types.
 */
unsigned Device_Object_List_Count(void)
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
    uint32_t array_index, BACNET_OBJECT_TYPE *object_type, uint32_t *instance)
{
    bool status = false;
    uint32_t count = 0;
    uint32_t object_index = 0;
    struct object_functions *pObject = NULL;

    /* array index zero is length - so invalid */
    if (array_index == 0) {
        return status;
    }
    object_index = array_index - 1;
    /* initialize the default return values */
    pObject = &Object_Table[0];
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Count && pObject->Object_Index_To_Instance) {
            object_index -= count;
            count = pObject->Object_Count();
            if (object_index < count) {
                *object_type = pObject->Object_Type;
                *instance = pObject->Object_Index_To_Instance(object_index);
                status = true;
                break;
            }
        }
        pObject++;
    }

    return status;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
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
bool Device_Valid_Object_Name(BACNET_CHARACTER_STRING *object_name1,
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
            pObject = Device_Objects_Find_Functions((BACNET_OBJECT_TYPE)type);
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

    pObject = Device_Objects_Find_Functions((BACNET_OBJECT_TYPE)object_type);
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
bool Device_Object_Name_Copy(BACNET_OBJECT_TYPE object_type,
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

/* return the length of the apdu encoded or BACNET_STATUS_ERROR for error or
   BACNET_STATUS_ABORT for abort message */
int Device_Read_Property_Local(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string = { 0 };
    BACNET_CHARACTER_STRING char_string = { 0 };
    uint32_t i = 0;
    uint32_t count = 0;
    uint8_t *apdu = NULL;
    struct object_functions *pObject = NULL;
    uint16_t apdu_max = 0;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_max = rpdata->application_data_len;
    switch ((int)rpdata->object_property) {
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string, Device_Description_Default);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_LOCATION:
            characterstring_init_ansi(&char_string, "USA");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_SYSTEM_STATUS:
            apdu_len =
                encode_application_enumerated(&apdu[0], Device_System_Status());
            break;
        case PROP_VENDOR_NAME:
            characterstring_init_ansi(&char_string, BACNET_VENDOR_NAME);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_VENDOR_IDENTIFIER:
            apdu_len = encode_application_unsigned(&apdu[0], BACNET_VENDOR_ID);
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
        case PROP_PROTOCOL_VERSION:
            apdu_len =
                encode_application_unsigned(&apdu[0], BACNET_PROTOCOL_VERSION);
            break;
        case PROP_PROTOCOL_REVISION:
            apdu_len =
                encode_application_unsigned(&apdu[0], BACNET_PROTOCOL_REVISION);
            break;
        case PROP_PROTOCOL_SERVICES_SUPPORTED:
            /* Note: list of services that are executed, not initiated. */
            bitstring_init(&bit_string);
            for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++) {
                /* automatic lookup based on handlers set */
                bitstring_set_bit(&bit_string, (uint8_t)i,
                    apdu_service_supported((BACNET_SERVICES_SUPPORTED)i));
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
            /* Note: this is the list of objects that can be in this device,
               not a list of objects that this device can access */
            bitstring_init(&bit_string);
            for (i = 0; i < MAX_ASHRAE_OBJECT_TYPE; i++) {
                /* initialize all the object types to not-supported */
                bitstring_set_bit(&bit_string, (uint8_t)i, false);
            }
            /* set the object types with objects to supported */
            pObject = Object_Table;
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
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index, Device_Object_List_Element_Encode, count,
                apdu, apdu_max);
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
            /* FIXME: encode the list here, if it exists */
            break;
        case PROP_DATABASE_REVISION:
            apdu_len = encode_application_unsigned(
                &apdu[0], Device_Database_Revision());
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

/** Looks up the common Object and Property, and encodes its Value in an
 * APDU. Sets the error class and code if request is not appropriate.
 * @param pObject - object table
 * @param rpdata [in,out] Structure with the requested Object & Property info
 *  on entry, and APDU message on return.
 * @return The length of the APDU on success, else BACNET_STATUS_ERROR
 */
static int Read_Property_Common(
    struct object_functions *pObject, BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
#if (BACNET_PROTOCOL_REVISION >= 14)
    struct special_property_list_t property_list;
#endif

    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            /*  only array properties can have array options */
            if (rpdata->array_index != BACNET_ARRAY_ALL) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
                apdu_len = BACNET_STATUS_ERROR;
            } else {
                /* Device Object exception: requested instance
                   may not match our instance if a wildcard */
                if (rpdata->object_type == OBJECT_DEVICE) {
                    rpdata->object_instance = Object_Instance_Number;
                }
                apdu_len = encode_application_object_id(
                    &apdu[0], rpdata->object_type, rpdata->object_instance);
            }
            break;
        case PROP_OBJECT_NAME:
            /*  only array properties can have array options */
            if (rpdata->array_index != BACNET_ARRAY_ALL) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
                apdu_len = BACNET_STATUS_ERROR;
            } else {
                characterstring_init_ansi(&char_string, "");
                if (pObject->Object_Name) {
                    (void)pObject->Object_Name(
                        rpdata->object_instance, &char_string);
                }
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_OBJECT_TYPE:
            /*  only array properties can have array options */
            if (rpdata->array_index != BACNET_ARRAY_ALL) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
                apdu_len = BACNET_STATUS_ERROR;
            } else {
                apdu_len = encode_application_enumerated(
                    &apdu[0], rpdata->object_type);
            }
            break;
#if (BACNET_PROTOCOL_REVISION >= 14)
        case PROP_PROPERTY_LIST:
            Device_Objects_Property_List(
                rpdata->object_type, rpdata->object_instance, &property_list);
            apdu_len = property_list_encode(rpdata,
                property_list.Required.pList, property_list.Optional.pList,
                property_list.Proprietary.pList);
            break;
#endif
        default:
            if (pObject->Object_Read_Property) {
                apdu_len = pObject->Object_Read_Property(rpdata);
            }
            break;
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

    /* initialize the default return values */
    rpdata->error_class = ERROR_CLASS_OBJECT;
    rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    pObject = Device_Objects_Find_Functions(rpdata->object_type);
    if (pObject) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(rpdata->object_instance)) {
            apdu_len = Read_Property_Common(pObject, rpdata);
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

/* returns true if successful */
bool Device_Write_Property_Local(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value - false=error */
    int len = 0;
    uint8_t encoding = 0;
    size_t length = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    /* decode the some of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    if ((wp_data->object_property != PROP_OBJECT_LIST) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch ((int)wp_data->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            if (value.tag == BACNET_APPLICATION_TAG_OBJECT_ID) {
                if ((value.type.Object_Id.type == OBJECT_DEVICE) &&
                    (Device_Set_Object_Instance_Number(
                        value.type.Object_Id.instance))) {
                    /* we could send an I-Am broadcast to let the world know */
                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
#if defined(BACDL_MSTP)
        case PROP_MAX_INFO_FRAMES:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if (value.type.Unsigned_Int <= 255) {
                    dlmstp_set_max_info_frames(value.type.Unsigned_Int);
                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_MAX_MASTER:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if ((value.type.Unsigned_Int > 0) &&
                    (value.type.Unsigned_Int <= 127)) {
                    dlmstp_set_max_master(value.type.Unsigned_Int);
                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
#endif
        case PROP_OBJECT_NAME:
            if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                length = characterstring_length(&value.type.Character_String);
                if (length < 1) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else if (length < characterstring_capacity(&My_Object_Name)) {
                    encoding =
                        characterstring_encoding(&value.type.Character_String);
                    if (encoding < MAX_CHARACTER_STRING_ENCODING) {
                        /* All the object names in a device must be unique. */
                        if (Device_Valid_Object_Name(
                                &value.type.Character_String, NULL, NULL)) {
                            wp_data->error_class = ERROR_CLASS_PROPERTY;
                            wp_data->error_code = ERROR_CODE_DUPLICATE_NAME;
                        } else {
                            Device_Set_Object_Name(
                                &value.type.Character_String);
                            status = true;
                        }
                    } else {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code =
                            ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
                    }
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        default:
            if (Property_List_Member(
                    wp_data->object_instance, wp_data->object_property)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            }
            break;
    }
    /* not using len at this time */
    (void)len;

    return status;
}

/**
 * @brief Handles the writing of the object name property
 * @param wp_data [in,out] WriteProperty data structure
 * @param Object_Write_Property object specific function to write the property
 * @return True on success, else False if there is an error.
 */
static bool Device_Write_Property_Object_Name(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    write_property_function Object_Write_Property)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_CHARACTER_STRING value;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    int apdu_size = 0;
    uint8_t *apdu = NULL;

    if (!wp_data) {
        return false;
    }
    if (wp_data->array_index != BACNET_ARRAY_ALL) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    apdu = wp_data->application_data;
    apdu_size = wp_data->application_data_len;
    len = bacnet_character_string_application_decode(apdu, apdu_size, &value);
    if (len > 0) {
        if ((characterstring_encoding(&value) != CHARACTER_ANSI_X34) ||
            (characterstring_length(&value) == 0) ||
            (!characterstring_printable(&value))) {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        } else {
            status = true;
        }
    } else if (len == 0) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
    } else {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
    }
    if (status) {
        /* All the object names in a device must be unique */
        if (Device_Valid_Object_Name(&value, &object_type, &object_instance)) {
            if ((object_type == wp_data->object_type) &&
                (object_instance == wp_data->object_instance)) {
                /* writing same name to same object */
                status = true;
            } else {
                /* name already exists in some object */
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_DUPLICATE_NAME;
                status = false;
            }
        } else {
            status = Object_Write_Property(wp_data);
        }
    }

    return status;
}

/**
 * @brief Set the callback for a WriteProperty successful operation
 * @param cb [in] The function to be called, or NULL to disable
 */
void Device_Write_Property_Store_Callback_Set(
        write_property_function cb)
{
    Device_Write_Property_Store_Callback = cb;
}

/**
 * @brief Store the value of a property when WriteProperty is successful
 */
static void Device_Write_Property_Store(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    if (Device_Write_Property_Store_Callback) {
        (void)Device_Write_Property_Store_Callback(wp_data);
    }
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
bool Device_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    struct object_functions *pObject = NULL;

    /* initialize the default return values */
    wp_data->error_class = ERROR_CLASS_OBJECT;
    wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    pObject = Device_Objects_Find_Functions(wp_data->object_type);
    if (pObject) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(wp_data->object_instance)) {
            if (pObject->Object_Write_Property) {
#if (BACNET_PROTOCOL_REVISION >= 14)
                if (wp_data->object_property == PROP_PROPERTY_LIST) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                    return status;
                }
#endif
                if (wp_data->object_property == PROP_OBJECT_NAME) {
                    status = Device_Write_Property_Object_Name(
                        wp_data, pObject->Object_Write_Property);
                } else {
                    status = pObject->Object_Write_Property(wp_data);
                }
                if (status) {
                    Device_Write_Property_Store(wp_data);
                }
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
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/** Looks up the requested Object, and fills the Property Value list.
 * If the Object or Property can't be found, returns false.
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @param [in] The object instance number to be looked up.
 * @param [out] The value list
 * @return True if the object instance supports this feature
 *         and was encoded correctly
 */
bool Device_Encode_Value_List(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE * value_list)
{
    bool status = false;        /* Ever the pessamist! */
    struct object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(object_instance)) {
            if (pObject->Object_Value_List) {
                status =
                    pObject->Object_Value_List(object_instance, value_list);
            }
        }
    }

    return (status);
}

/** Checks the COV flag in the requested Object
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @param [in] The object instance to be looked up.
 * @return True if the COV flag is set
 */
bool Device_COV(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    bool status = false;        /* Ever the pessamist! */
    struct object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(object_instance)) {
            if (pObject->Object_COV) {
                status = pObject->Object_COV(object_instance);
            }
        }
    }

    return (status);
}

/** Clears the COV flag in the requested Object
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @param [in] The object instance to be looked up.
 */
void Device_COV_Clear(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    struct object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(object_instance)) {
            if (pObject->Object_COV_Clear) {
                pObject->Object_COV_Clear(object_instance);
            }
        }
    }
}

/**
 * @brief Updates all the object timers with elapsed milliseconds
 * @param milliseconds - number of milliseconds elapsed
 */
void Device_Timer(uint16_t milliseconds)
{
    struct object_functions *pObject;
    unsigned count = 0;
    uint32_t instance;

    pObject = Object_Table;
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Count) {
            count = pObject->Object_Count();
        }
        while (count) {
            count--;
            if ((pObject->Object_Timer) &&
                (pObject->Object_Index_To_Instance)) {
                instance = pObject->Object_Index_To_Instance(count);
                pObject->Object_Timer(instance, milliseconds);
            }
        }
        pObject++;
    }
}

/** Looks up the requested Object to see if the functionality is supported.
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @return True if the object instance supports this feature.
 */
bool Device_Value_List_Supported(BACNET_OBJECT_TYPE object_type)
{
    bool status = false; /* Ever the pessamist! */
    struct object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Value_List) {
            status = true;
        }
    }

    return (status);
}

/** Initialize the Device Object.
 Initialize the group of object helper functions for any supported Object.
 Initialize each of the Device Object child Object instances.
 * @ingroup ObjIntf
 * @param object_table [in,out] array of structure with object functions.
 *  Each Child Object must provide some implementation of each of these
 *  functions in order to properly support the default handlers.
 */
void Device_Init(object_functions_t *object_table)
{
    struct object_functions *pObject = NULL;

    /* we don't use the object table passed in
       since there is extra stuff we don't need in there. */
    (void)object_table;
    /* our local object table */
    pObject = &Object_Table[0];
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Init) {
            pObject->Object_Init();
        }
        pObject++;
    }
    dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
    if (Object_Instance_Number > BACNET_MAX_INSTANCE) {
        Object_Instance_Number = BACNET_MAX_INSTANCE;
    }
    characterstring_init_ansi(&My_Object_Name, Device_Name_Default);
}
