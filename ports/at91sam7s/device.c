/**************************************************************************
 *
 * Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/

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
#include "rs485.h"
#include "bacnet/version.h"
#include "bacnet/basic/services.h"
#include "bacnet/proplist.h"
/* objects */
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/wp.h"
#include "bacnet/dcc.h"

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;

/* forward prototype */
int Device_Read_Property_Local(BACNET_READ_PROPERTY_DATA *rpdata);
bool Device_Write_Property_Local(BACNET_WRITE_PROPERTY_DATA *wp_data);

static struct my_object_functions {
    BACNET_OBJECT_TYPE Object_Type;
    object_init_function Object_Init;
    object_count_function Object_Count;
    object_index_to_instance_function Object_Index_To_Instance;
    object_valid_instance_function Object_Valid_Instance;
    object_name_function Object_Name;
    read_property_function Object_Read_Property;
    write_property_function Object_Write_Property;
    rpm_property_lists_function Object_RPM_List;
} Object_Table[] = { { OBJECT_DEVICE, NULL, /* don't init - recursive! */
                         Device_Count, Device_Index_To_Instance,
                         Device_Valid_Object_Instance_Number,
                         Device_Object_Name, Device_Read_Property_Local,
                         Device_Write_Property_Local, Device_Property_Lists },
    { OBJECT_ANALOG_INPUT, Analog_Input_Init, Analog_Input_Count,
        Analog_Input_Index_To_Instance, Analog_Input_Valid_Instance,
        Analog_Input_Object_Name, Analog_Input_Read_Property, NULL,
        Analog_Input_Property_Lists },
    { OBJECT_ANALOG_VALUE, Analog_Value_Init, Analog_Value_Count,
        Analog_Value_Index_To_Instance, Analog_Value_Valid_Instance,
        Analog_Value_Object_Name, Analog_Value_Read_Property,
        Analog_Value_Write_Property, Analog_Value_Property_Lists },
    { OBJECT_BINARY_INPUT, Binary_Input_Init, Binary_Input_Count,
        Binary_Input_Index_To_Instance, Binary_Input_Valid_Instance,
        Binary_Input_Object_Name, Binary_Input_Read_Property, NULL,
        Binary_Input_Property_Lists },
    { OBJECT_BINARY_VALUE, Binary_Value_Init, Binary_Value_Count,
        Binary_Value_Index_To_Instance, Binary_Value_Valid_Instance,
        Binary_Value_Object_Name, Binary_Value_Read_Property,
        Binary_Value_Write_Property, Binary_Value_Property_Lists },
#if (BACNET_PROTOCOL_REVISION >= 17)
    { OBJECT_NETWORK_PORT, Network_Port_Init, Network_Port_Count,
        Network_Port_Index_To_Instance, Network_Port_Valid_Instance,
        Network_Port_Object_Name, Network_Port_Read_Property,
        Network_Port_Write_Property, Network_Port_Property_Lists },
#endif
    { MAX_BACNET_OBJECT_TYPE, NULL, NULL, NULL, NULL, NULL, NULL, NULL } };

/* note: you really only need to define variables for
   properties that are writable or that may change.
   The properties that are constant can be hard coded
   into the read-property encoding. */
static uint32_t Object_Instance_Number;
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static BACNET_CHARACTER_STRING My_Object_Name;
static uint32_t Database_Revision;
static BACNET_REINITIALIZED_STATE Reinitialize_State = BACNET_REINIT_IDLE;
static const char *Reinit_Password = "rehmite";

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Device_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_SYSTEM_STATUS, PROP_VENDOR_NAME,
    PROP_VENDOR_IDENTIFIER, PROP_MODEL_NAME, PROP_FIRMWARE_REVISION,
    PROP_APPLICATION_SOFTWARE_VERSION, PROP_PROTOCOL_VERSION,
    PROP_PROTOCOL_REVISION, PROP_PROTOCOL_SERVICES_SUPPORTED,
    PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED, PROP_OBJECT_LIST,
    PROP_MAX_APDU_LENGTH_ACCEPTED, PROP_SEGMENTATION_SUPPORTED,
    PROP_APDU_TIMEOUT, PROP_NUMBER_OF_APDU_RETRIES, PROP_MAX_MASTER,
    PROP_MAX_INFO_FRAMES, PROP_DEVICE_ADDRESS_BINDING, PROP_DATABASE_REVISION,
    -1 };

static const int Device_Properties_Optional[] = { PROP_DESCRIPTION,
    PROP_LOCATION, -1 };

static const int Device_Properties_Proprietary[] = { 9600, -1 };

static struct my_object_functions *Device_Objects_Find_Functions(
    BACNET_OBJECT_TYPE Object_Type)
{
    struct my_object_functions *pObject = NULL;

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

static int Read_Property_Common(
    struct my_object_functions *pObject, BACNET_READ_PROPERTY_DATA *rpdata)
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
    if (property_list_common(rpdata->object_property)) {
        apdu_len = property_list_common_encode(rpdata, Object_Instance_Number);
    } else if (rpdata->object_property == PROP_OBJECT_NAME) {
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
#if (BACNET_PROTOCOL_REVISION >= 14)
    } else if (rpdata->object_property == PROP_PROPERTY_LIST) {
        Device_Objects_Property_List(
            rpdata->object_type, rpdata->object_instance, &property_list);
        apdu_len = property_list_encode(rpdata, property_list.Required.pList,
            property_list.Optional.pList, property_list.Proprietary.pList);
#endif
    } else if (pObject->Object_Read_Property) {
        apdu_len = pObject->Object_Read_Property(rpdata);
    }

    return apdu_len;
}

/* Encodes the property APDU and returns the length,
   or sets the error, and returns BACNET_STATUS_ERROR */
int Device_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = BACNET_STATUS_ERROR;
    struct my_object_functions *pObject = NULL;

    /* initialize the default return values */
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

bool Device_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    struct my_object_functions *pObject = NULL;

    /* initialize the default return values */
    pObject = Device_Objects_Find_Functions(wp_data->object_type);
    if (pObject) {
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
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/* for a given object type, returns the special property list */
void Device_Objects_Property_List(BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    struct special_property_list_t *pPropertyList)
{
    struct my_object_functions *pObject = NULL;

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
    if (pRequired)
        *pRequired = Device_Properties_Required;
    if (pOptional)
        *pOptional = Device_Properties_Optional;
    if (pProprietary)
        *pProprietary = Device_Properties_Proprietary;

    return;
}

unsigned Device_Count(void)
{
    return 1;
}

uint32_t Device_Index_To_Instance(unsigned index)
{
    index = index;
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
    Reinit_Password = password;

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
    if (Reinit_Password && strlen(Reinit_Password) > 0) {
        if (characterstring_length(&rd_data->password) > 20) {
            rd_data->error_class = ERROR_CLASS_SERVICES;
            rd_data->error_code = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
        } else if (characterstring_ansi_same(
                       &rd_data->password, Reinit_Password)) {
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

void Device_Init(object_functions_t *object_table)
{
    struct my_object_functions *pObject = NULL;

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
    Object_Instance_Number = 12345;
    characterstring_init_ansi(&My_Object_Name, "ARM7 Demo Device");
}

/* methods to manipulate the data */
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

uint32_t Device_Database_Revision(void)
{
    return Database_Revision;
}

void Device_Inc_Database_Revision(void)
{
    Database_Revision++;
}

/* Since many network clients depend on the object list */
/* for discovery, it must be consistent! */
unsigned Device_Object_List_Count(void)
{
    unsigned count = 0; /* number of objects */
    struct my_object_functions *pObject = NULL;

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

bool Device_Object_List_Identifier(
    uint32_t array_index, BACNET_OBJECT_TYPE *object_type, uint32_t *instance)
{
    bool status = false;
    uint32_t count = 0;
    uint32_t object_index = 0;
    struct my_object_functions *pObject = NULL;

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
    struct my_object_functions *pObject = NULL;

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

bool Device_Valid_Object_Id(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    bool status = false; /* return value */
    struct my_object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions((BACNET_OBJECT_TYPE)object_type);
    if ((pObject != NULL) && (pObject->Object_Valid_Instance != NULL)) {
        status = pObject->Object_Valid_Instance(object_instance);
    }

    return status;
}

bool Device_Object_Name_Copy(BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    struct my_object_functions *pObject = NULL;
    bool found = false;

    pObject = Device_Objects_Find_Functions(object_type);
    if ((pObject != NULL) && (pObject->Object_Name != NULL)) {
        found = pObject->Object_Name(object_instance, object_name);
    }

    return found;
}

/* return the length of the apdu encoded or BACNET_STATUS_ERROR for error */
int Device_Read_Property_Local(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint32_t i = 0;
    uint32_t count = 0;
    uint8_t *apdu = NULL;
    int apdu_max = 0;
    struct my_object_functions *pObject = NULL;

    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_max = rpdata->application_data_len;
    switch ((int)rpdata->object_property) {
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string, "BACnet Demo");
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
            characterstring_init_ansi(&char_string, "GNU Demo");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FIRMWARE_REVISION:
            characterstring_init_ansi(&char_string, BACnet_Version);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_APPLICATION_SOFTWARE_VERSION:
            characterstring_init_ansi(&char_string, "1.0");
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
            i = 0;
            pObject = &Object_Table[i];
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
        case PROP_MAX_INFO_FRAMES:
            apdu_len =
                encode_application_unsigned(&apdu[0], dlmstp_max_info_frames());
            break;
        case PROP_MAX_MASTER:
            apdu_len =
                encode_application_unsigned(&apdu[0], dlmstp_max_master());
            break;
        case 9600:
            apdu_len = encode_application_unsigned(&apdu[0], rs485_baud_rate());
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
        case 9600:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if (value.type.Unsigned_Int <= 115200) {
                    status = rs485_baud_rate_set(value.type.Unsigned_Int);
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_NUMBER_OF_APDU_RETRIES:
        case PROP_APDU_TIMEOUT:
        case PROP_VENDOR_IDENTIFIER:
        case PROP_SYSTEM_STATUS:
        case PROP_LOCATION:
        case PROP_DESCRIPTION:
        case PROP_MODEL_NAME:
        case PROP_VENDOR_NAME:
        case PROP_FIRMWARE_REVISION:
        case PROP_APPLICATION_SOFTWARE_VERSION:
        case PROP_LOCAL_TIME:
        case PROP_UTC_OFFSET:
        case PROP_LOCAL_DATE:
        case PROP_DAYLIGHT_SAVINGS_STATUS:
        case PROP_PROTOCOL_VERSION:
        case PROP_PROTOCOL_REVISION:
        case PROP_PROTOCOL_SERVICES_SUPPORTED:
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
        case PROP_OBJECT_LIST:
        case PROP_MAX_APDU_LENGTH_ACCEPTED:
        case PROP_SEGMENTATION_SUPPORTED:
        case PROP_DEVICE_ADDRESS_BINDING:
        case PROP_DATABASE_REVISION:
        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }
    /* not using len at this time */
    len = len;

    return status;
}
