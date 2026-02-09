/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2024
 * @brief A basic BACnet Life Safety Zone object type implementation.
 * @details The Life Safety Zone object type defines a standardized object
 * whose properties represent the externally visible characteristics
 * associated with an arbitrary group of BACnet Life Safety Point
 * and Life Safety Zone objects in fire, life safety and security
 * applications. The condition of a Life Safety Zone object is
 * represented by a mode and a state.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/proplist.h"
/* me! */
#include "bacnet/basic/object/lsz.h"

struct object_data {
    bool Out_Of_Service : 1;
    bool Maintenance_Required : 1;
    BACNET_LIFE_SAFETY_STATE Present_Value;
    BACNET_LIFE_SAFETY_STATE Tracking_Value;
    BACNET_LIFE_SAFETY_MODE Mode;
    BACNET_SILENCED_STATE Silenced;
    BACNET_LIFE_SAFETY_OPERATION Operation_Expected;
    uint8_t Reliability;
    const char *Object_Name;
    OS_Keylist Zone_Members;
    void *Context;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_LIFE_SAFETY_ZONE;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Life_Safety_Zone_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,          PROP_PRESENT_VALUE,
    PROP_TRACKING_VALUE,       PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,          PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,       PROP_MODE,
    PROP_ACCEPTED_MODES,       PROP_SILENCED,
    PROP_OPERATION_EXPECTED,   PROP_ZONE_MEMBERS,
    PROP_MAINTENANCE_REQUIRED, -1
};

static const int32_t Life_Safety_Zone_Properties_Optional[] = { -1 };

static const int32_t Life_Safety_Zone_Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of always writable properties */
    PROP_MODE,
    PROP_PRESENT_VALUE,
    PROP_OUT_OF_SERVICE,
    PROP_SILENCED,
    PROP_OPERATION_EXPECTED,
    PROP_ZONE_MEMBERS,
    -1
};

/**
 * Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 *
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optkional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void Life_Safety_Zone_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
{
    if (pRequired) {
        *pRequired = Life_Safety_Zone_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Life_Safety_Zone_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Life_Safety_Zone_Properties_Proprietary;
    }

    return;
}

/**
 * @brief Get the list of writable properties for a Life Safety Zone object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Life_Safety_Zone_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * @brief Determines if a given object instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Life_Safety_Zone_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of objects
 * @return  Number of objects
 */
unsigned Life_Safety_Zone_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines if a given object instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is     BACNET_SILENCED_STATE Silenced;

 * @brief Determines the object instance-number for a given 0..N index
 *  where N is Life_Safety_Zone_Count().
 * @param  index - 0..N value
 * @return object instance-number for the given index
 */
uint32_t Life_Safety_Zone_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * @brief For a given object instance-number, determines a 0..N index
 * of where N is Life_Safety_Zone_Count().
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number
 */
unsigned Life_Safety_Zone_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief For a given object instance-number, determines the present-value
 * @param  object_instance - object-instance number of the object
 * @return  present-value of the object
 */
BACNET_LIFE_SAFETY_STATE
Life_Safety_Zone_Present_Value(uint32_t object_instance)
{
    BACNET_LIFE_SAFETY_STATE value = LIFE_SAFETY_STATE_QUIET;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Present_Value;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the present-value
 * @param  object_instance - object-instance number of the object
 * @param  value - present-value property value
 * @return  true if values are within range and relinquish-default value is set.
 */
bool Life_Safety_Zone_Present_Value_Set(
    uint32_t object_instance, BACNET_LIFE_SAFETY_STATE value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Present_Value = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, loads the object-name into
 * a characterstring. Note that the object name must be unique
 * within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Life_Safety_Zone_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[32];

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "LIFE-SAFETY-ZONE-%lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, name_text);
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the object-name
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 * @return  true if object-name was set
 */
bool Life_Safety_Zone_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        pObject->Object_Name = new_name;
    }

    return status;
}

/**
 * @brief Return the object name C string
 * @param object_instance [in] BACnet object instance number
 * @return object name or NULL if not found
 */
const char *Life_Safety_Zone_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * @brief For a given object instance-number, gets the property value
 * @param  object_instance - object-instance number of the object
 * @return property value
 */
BACNET_SILENCED_STATE Life_Safety_Zone_Silenced(uint32_t object_instance)
{
    BACNET_SILENCED_STATE value = SILENCED_STATE_UNSILENCED;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Silenced;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the property value
 * @param  object_instance - object-instance number of the object
 * @param  value - enumerated value
 * @return  true if values are within range and property is set.
 */
bool Life_Safety_Zone_Silenced_Set(
    uint32_t object_instance, BACNET_SILENCED_STATE value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= SILENCED_STATE_PROPRIETARY_MAX) {
            pObject->Silenced = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the property value
 * @param  object_instance - object-instance number of the object
 * @return property value
 */
BACNET_LIFE_SAFETY_MODE Life_Safety_Zone_Mode(uint32_t object_instance)
{
    BACNET_LIFE_SAFETY_MODE value = LIFE_SAFETY_MODE_OFF;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Mode;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the property value
 * @param  object_instance - object-instance number of the object
 * @param  value - enumerated value
 * @return  true if values are within range and property is set.
 */
bool Life_Safety_Zone_Mode_Set(
    uint32_t object_instance, BACNET_LIFE_SAFETY_MODE value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= LIFE_SAFETY_MODE_PROPRIETARY_MAX) {
            pObject->Mode = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the property value
 * @param  object_instance - object-instance number of the object
 * @return property value
 */
BACNET_LIFE_SAFETY_OPERATION
Life_Safety_Zone_Operation_Expected(uint32_t object_instance)
{
    BACNET_LIFE_SAFETY_OPERATION value = LIFE_SAFETY_OP_NONE;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Operation_Expected;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the property value
 * @param  object_instance - object-instance number of the object
 * @param  value - enumerated value
 * @return  true if values are within range and property is set.
 */
bool Life_Safety_Zone_Operation_Expected_Set(
    uint32_t object_instance, BACNET_LIFE_SAFETY_OPERATION value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= LIFE_SAFETY_OP_PROPRIETARY_MAX) {
            pObject->Operation_Expected = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the out-of-service
 *  status flag
 * @param  object_instance - object-instance number of the object
 * @return  out-of-service status flag
 */
bool Life_Safety_Zone_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the out-of-service status
 * flag
 * @param object_instance - object-instance number of the object
 * @param value - boolean out-of-service value
 * @return true if the out-of-service status flag was set
 */
void Life_Safety_Zone_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Out_Of_Service = value;
    }
}

/**
 * @brief For a given object instance-number, gets the reliability.
 * @param  object_instance - object-instance number of the object
 * @return reliability value
 */
BACNET_RELIABILITY Life_Safety_Zone_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY reliability = RELIABILITY_NO_FAULT_DETECTED;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        reliability = (BACNET_RELIABILITY)pObject->Reliability;
    }

    return reliability;
}

/**
 * @brief For a given object instance-number, sets the reliability
 * @param  object_instance - object-instance number of the object
 * @param  value - reliability enumerated value
 * @return  true if values are within range and property is set.
 */
bool Life_Safety_Zone_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= 255) {
            pObject->Reliability = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Encode a Zone Members list complex data type
 *
 * @param object_instance - object-instance number of the object
 * @param apdu - the APDU buffer
 * @param apdu_size - size of the apdu buffer.
 *
 * @return bytes encoded or zero on error.
 */
static int Life_Safety_Zone_Members_Encode(
    uint32_t object_instance, uint8_t *apdu, int apdu_size)
{
    struct object_data *pObject;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *data;
    int apdu_len = 0, len;
    unsigned index = 0;
    unsigned size = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return BACNET_STATUS_ERROR;
    }
    size = Keylist_Count(pObject->Zone_Members);
    for (index = 0; index < size; index++) {
        data = Keylist_Data_Index(pObject->Zone_Members, index);
        len = bacapp_encode_device_obj_property_ref(NULL, data);
        apdu_len += len;
    }
    if (apdu_len > apdu_size) {
        return BACNET_STATUS_ABORT;
    }
    apdu_len = 0;
    for (index = 0; index < size; index++) {
        data = Keylist_Data_Index(pObject->Zone_Members, index);
        len = bacapp_encode_device_obj_property_ref(apdu, data);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }

    return apdu_len;
}

/**
 * @brief Add a member to the Zone Members list
 * @param object_instance - object-instance number of the object
 * @param data - the device object property reference to add
 * @return true if the member was added
 */
bool Life_Safety_Zone_Members_Add(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *data)
{
    bool status = false;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *entry;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return false;
    }
    entry = calloc(1, sizeof(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE));
    if (!entry) {
        return false;
    }
    memcpy(entry, data, sizeof(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE));
    status = Keylist_Data_Add(
        pObject->Zone_Members, Keylist_Count(pObject->Zone_Members), entry);

    return status;
}

/**
 * @brief Remove all members from the Zone Members list
 * @param object_instance - object-instance number of the object
 */
void Life_Safety_Zone_Members_Clear(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        Keylist_Data_Free(pObject->Zone_Members);
    }
}

/**
 * @brief Write a list of Zone Members to the object
 * @param wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return true if the list was written
 */
static bool Life_Safety_Zone_Members_Write(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    int len = 0, apdu_len = 0, apdu_size = 0;
    const uint8_t *apdu = NULL;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE data = { 0 };

    if (wp_data == NULL) {
        return false;
    }
    /* empty the list */
    Life_Safety_Zone_Members_Clear(wp_data->object_instance);
    apdu = wp_data->application_data;
    apdu_size = wp_data->application_data_len;
    /* decode all packed */
    while (apdu_len < apdu_size) {
        len = bacnet_device_object_property_reference_decode(
            apdu, apdu_size - apdu_len, &data);
        if (len < 0) {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            return false;
        }
        Life_Safety_Zone_Members_Add(wp_data->object_instance, &data);
        apdu_len += len;
    }

    return true;
}

/**
 * @brief For a given object instance-number, returns the maintenance-required
 *  status flag
 * @param  object_instance - object-instance number of the object
 * @return  property value status flag
 */
bool Life_Safety_Zone_Maintenance_Required(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Maintenance_Required;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the maintenance-required
 * status flag
 * @param object_instance - object-instance number of the object
 * @param value - boolean property value
 * @return true if the property flag was set
 */
void Life_Safety_Zone_Maintenance_Required_Set(
    uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Maintenance_Required = value;
    }
}

/**
 * @brief ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int Life_Safety_Zone_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_LIFE_SAFETY_STATE present_value = LIFE_SAFETY_STATE_QUIET;
    BACNET_LIFE_SAFETY_MODE mode = LIFE_SAFETY_MODE_DEFAULT;
    BACNET_SILENCED_STATE silenced_state = SILENCED_STATE_UNSILENCED;
    BACNET_LIFE_SAFETY_OPERATION operation = LIFE_SAFETY_OP_NONE;
    bool state = false;
    BACNET_RELIABILITY reliability = RELIABILITY_NO_FAULT_DETECTED;
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
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Life_Safety_Zone_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            present_value =
                Life_Safety_Zone_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
        case PROP_TRACKING_VALUE:
            /* FIXME: tracking value is a local matter how it is derived */
            present_value =
                Life_Safety_Zone_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            if (Life_Safety_Zone_Reliability(rpdata->object_instance) ==
                RELIABILITY_NO_FAULT_DETECTED) {
                bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            } else {
                bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, true);
            }
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            if (Life_Safety_Zone_Out_Of_Service(rpdata->object_instance)) {
                bitstring_set_bit(
                    &bit_string, STATUS_FLAG_OUT_OF_SERVICE, true);
            } else {
                bitstring_set_bit(
                    &bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Life_Safety_Zone_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_RELIABILITY:
            reliability = Life_Safety_Zone_Reliability(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], reliability);
            break;
        case PROP_MODE:
            mode = Life_Safety_Zone_Mode(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], mode);
            break;
        case PROP_ACCEPTED_MODES:
            for (mode = 0; mode < LIFE_SAFETY_MODE_RESERVED_MIN; mode++) {
                len = encode_application_enumerated(&apdu[apdu_len], mode);
                apdu_len += len;
            }
            break;
        case PROP_SILENCED:
            silenced_state = Life_Safety_Zone_Silenced(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], silenced_state);
            break;
        case PROP_OPERATION_EXPECTED:
            operation =
                Life_Safety_Zone_Operation_Expected(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], operation);
            break;
        case PROP_ZONE_MEMBERS:
            apdu_len = Life_Safety_Zone_Members_Encode(
                rpdata->object_instance, &apdu[0], apdu_size);
            break;
        case PROP_MAINTENANCE_REQUIRED:
            state = false;
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/* returns true if successful */
bool Life_Safety_Zone_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

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
    switch (wp_data->object_property) {
        case PROP_MODE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= LIFE_SAFETY_MODE_PROPRIETARY_MAX) {
                    Life_Safety_Zone_Mode_Set(
                        wp_data->object_instance,
                        (BACNET_LIFE_SAFETY_MODE)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= UINT16_MAX) {
                    Life_Safety_Zone_Present_Value_Set(
                        wp_data->object_instance,
                        (BACNET_LIFE_SAFETY_STATE)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Life_Safety_Zone_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_SILENCED:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= UINT16_MAX) {
                    Life_Safety_Zone_Silenced_Set(
                        wp_data->object_instance,
                        (BACNET_SILENCED_STATE)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OPERATION_EXPECTED:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= UINT16_MAX) {
                    Life_Safety_Zone_Operation_Expected_Set(
                        wp_data->object_instance,
                        (BACNET_LIFE_SAFETY_OPERATION)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_ZONE_MEMBERS:
            status = Life_Safety_Zone_Members_Write(wp_data);
            break;
        default:
            if (property_lists_member(
                    Life_Safety_Zone_Properties_Required,
                    Life_Safety_Zone_Properties_Optional,
                    Life_Safety_Zone_Properties_Proprietary,
                    wp_data->object_property)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            }
            break;
    }

    return status;
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Life_Safety_Zone_Context_Get(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return pObject->Context;
    }

    return NULL;
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void Life_Safety_Zone_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Creates an object and initialize its properties to defaults
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Life_Safety_Zone_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;

    if (!Object_List) {
        Object_List = Keylist_Create();
    }
    if (object_instance > BACNET_MAX_INSTANCE) {
        return BACNET_MAX_INSTANCE;
    } else if (object_instance == BACNET_MAX_INSTANCE) {
        /* wildcard instance */
        /* the Object_Identifier property of the newly created object
            shall be initialized to a value that is unique within the
            responding BACnet-user device. The method used to generate
            the object identifier is a local matter.*/
        object_instance = Keylist_Next_Empty_Key(Object_List, 1);
    }
    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (pObject) {
            pObject->Object_Name = NULL;
            pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
            pObject->Mode = LIFE_SAFETY_MODE_DEFAULT;
            pObject->Present_Value = LIFE_SAFETY_STATE_QUIET;
            pObject->Silenced = SILENCED_STATE_UNSILENCED;
            pObject->Operation_Expected = LIFE_SAFETY_OP_NONE;
            pObject->Maintenance_Required = false;
            pObject->Out_Of_Service = false;
            pObject->Zone_Members = Keylist_Create();
            /* add to list */
            index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if (index < 0) {
                free(pObject);
                return BACNET_MAX_INSTANCE;
            }
        } else {
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * @brief Deletes an object and its property
 * @param object_instance - object-instance number of the object
 * @return true if the object-instance was deleted
 */
bool Life_Safety_Zone_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        Keylist_Data_Free(pObject->Zone_Members);
        Keylist_Delete(pObject->Zone_Members);
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all the objects and their property data
 */
void Life_Safety_Zone_Cleanup(void)
{
    struct object_data *pObject;

    if (Object_List) {
        do {
            pObject = Keylist_Data_Pop(Object_List);
            if (pObject) {
                free(pObject);
            }
        } while (pObject);
        Keylist_Delete(Object_List);
        Object_List = NULL;
    }
}

/**
 * @brief Initializes the object data
 */
void Life_Safety_Zone_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
