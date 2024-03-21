/**************************************************************************
 *
 * Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
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
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet core library */
#include "bacnet/apdu.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/dcc.h"
#include "bacnet/proplist.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/version.h"
/* BACnet basic library */
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"
/* our objects */
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bv.h"
/* local port */
#include "rs485.h"

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;

static object_functions_t Object_Table[] = {
    { OBJECT_DEVICE, NULL, /* don't init - recursive! */
        Device_Count, Device_Index_To_Instance,
        Device_Valid_Object_Instance_Number, Device_Object_Name,
        Device_Read_Property_Local, Device_Write_Property_Local,
        Device_Property_Lists, NULL /* ReadRangeInfo */, NULL /* Iterator */,
        NULL /* Value_Lists */, NULL /* COV */, NULL /* COV Clear */,
        NULL /* Intrinsic Reporting */, NULL /* Add_List_Element */,
        NULL /* Remove_List_Element */, NULL /* Create */, NULL /* Delete */,
        NULL /* Timer */ },
    { OBJECT_ANALOG_INPUT, Analog_Input_Init, Analog_Input_Count,
        Analog_Input_Index_To_Instance, Analog_Input_Valid_Instance,
        Analog_Input_Object_Name, Analog_Input_Read_Property, NULL,
        Analog_Input_Property_Lists, NULL /* ReadRangeInfo */,
        NULL /* Iterator */, NULL /* Value_Lists */, NULL /* COV */,
        NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        NULL /* Create */, NULL /* Delete */, NULL /* Timer */ },
    { OBJECT_ANALOG_VALUE, Analog_Value_Init, Analog_Value_Count,
        Analog_Value_Index_To_Instance, Analog_Value_Valid_Instance,
        Analog_Value_Object_Name, Analog_Value_Read_Property,
        Analog_Value_Write_Property, Analog_Value_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        NULL /* Create */, NULL /* Delete */, NULL /* Timer */ },
    { OBJECT_BINARY_INPUT, Binary_Input_Init, Binary_Input_Count,
        Binary_Input_Index_To_Instance, Binary_Input_Valid_Instance,
        Binary_Input_Object_Name, Binary_Input_Read_Property, NULL,
        Binary_Input_Property_Lists, NULL /* ReadRangeInfo */,
        NULL /* Iterator */, NULL /* Value_Lists */, NULL /* COV */,
        NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        NULL /* Create */, NULL /* Delete */, NULL /* Timer */ },
    { OBJECT_BINARY_VALUE, Binary_Value_Init, Binary_Value_Count,
        Binary_Value_Index_To_Instance, Binary_Value_Valid_Instance,
        Binary_Value_Object_Name, Binary_Value_Read_Property,
        Binary_Value_Write_Property, Binary_Value_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        NULL /* Create */, NULL /* Delete */, NULL /* Timer */ },
#if (BACNET_PROTOCOL_REVISION >= 17)
    { OBJECT_NETWORK_PORT, Network_Port_Init, Network_Port_Count,
        Network_Port_Index_To_Instance, Network_Port_Valid_Instance,
        Network_Port_Object_Name, Network_Port_Read_Property,
        Network_Port_Write_Property, Network_Port_Property_Lists,
        NULL /* ReadRangeInfo */, NULL /* Iterator */, NULL /* Value_Lists */,
        NULL /* COV */, NULL /* COV Clear */, NULL /* Intrinsic Reporting */,
        NULL /* Add_List_Element */, NULL /* Remove_List_Element */,
        NULL /* Create */, NULL /* Delete */, NULL /* Timer */ },
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

/* note: you really only need to define variables for
   properties that are writable or that may change.
   The properties that are constant can be hard coded
   into the read-property encoding. */
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static BACNET_CHARACTER_STRING My_Object_Name;

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
    PROP_LOCATION, PROP_MAX_MASTER, PROP_MAX_INFO_FRAMES, -1 };

static const int Device_Properties_Proprietary[] = { 9600, -1 };

/**
 * @brief Get the list of required, optional, and proprietary properties.
 * @param pRequired [out] The list of required properties
 * @param pOptional [out] The list of optional properties
 * @param pProprietary [out] The list of proprietary properties
 */
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

/**
 * @brief Get the number of device objects
 * @return The number of device objects
 */
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

/**
 * @brief Set the object name of the device object
 * @param object_name [in] The object name of the device object
 * @return True if the object name was set
 */
bool Device_Set_Object_Name(BACNET_CHARACTER_STRING *object_name)
{
    bool status = false; /*return value */

    if (!characterstring_same(&My_Object_Name, object_name)) {
        /* Make the change and update the database revision */
        status = characterstring_copy(&My_Object_Name, object_name);
        handler_device_object_database_revision_increment();
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
    return handler_device_reinitialize_password_set(password);
}

/**
 * @brief Gets the ReinitializeDevice state
 * @return the ReinitializeDevice state
 */
BACNET_REINITIALIZED_STATE Device_Reinitialized_State(void)
{
    return handler_device_reinitialized_state();
}

/**
 * @brief Get the object instance number of the device object
 * @return The object instance number of the device object
 */
uint32_t Device_Object_Instance_Number(void)
{
    return handler_device_object_instance_number();
}

/**
 * @brief Set the object instance number of the device object
 * @param object_id [in] The object instance number of the device object
 * @return True if the object instance number was set
 */
bool Device_Set_Object_Instance_Number(uint32_t object_id)
{
    handler_device_object_instance_number_set(object_id);

    return true;
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
 * @brief Get the system status of the device object
 * @return The system status of the device object
 */
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
    /*return value - 0 = ok, -1 = bad value, -2 = not allowed */
    int result = -1;

    if (status < MAX_DEVICE_STATUS) {
        System_Status = status;
        result = 0;
    }

    return result;
}

/**
 * @brief Get the vendor identifier of the device object
 * @return The vendor identifier of the device object
 */
uint16_t Device_Vendor_Identifier(void)
{
    return handler_device_vendor_identifier();
}

/**
 * @brief Determine if segmentation is supported
 * @return The segmentation supported enumeration
 */
BACNET_SEGMENTATION Device_Segmentation_Supported(void)
{
    return SEGMENTATION_NONE;
}

/**
 * @brief handle the ReadProperty-Request
 * @param rpdata [in,out] The data structure containing the ReadProperty-Request
 * @return the length of the apdu encoded or BACNET_STATUS_ERROR for error
 */
int Device_Read_Property_Local(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string = { 0 };
    BACNET_CHARACTER_STRING char_string = { 0 };
    uint32_t count = 0;
    uint8_t *apdu = NULL;
    int apdu_max = 0;

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
            handler_device_services_supported(&bit_string);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
            handler_device_object_types_supported(&bit_string);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OBJECT_LIST:
            count = handler_device_object_list_count();
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index, handler_device_object_list_element_encode,
                count, apdu, apdu_max);
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
            apdu_len = address_list_encode(&apdu[0], apdu_max);
            break;
        case PROP_DATABASE_REVISION:
            apdu_len = encode_application_unsigned(
                &apdu[0], handler_device_object_database_revision());
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

/**
 * @brief handle the WriteProperty-Request
 * @param wp_data [in,out] The data structure containing the
 * WriteProperty-Request
 * @return true if the value was written
 */
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
                        if (handler_device_valid_object_name(
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
        default:
            if (handler_device_object_property_list_member(wp_data->object_type,
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
    len = len;

    return status;
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
    characterstring_init_ansi(&My_Object_Name, "ARM7 Demo Device");
    if (object_table) {
        handler_device_object_table_set(object_table);
    } else {
        handler_device_object_table_set(&Object_Table[0]);
    }
    dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
    handler_device_reinitialize_password_set("rehmite");
    handler_device_vendor_identifier_set(BACNET_VENDOR_ID);
    handler_device_object_instance_number_set(12345);
    handler_device_object_database_revision_set(1);
    handler_device_object_init();
}
