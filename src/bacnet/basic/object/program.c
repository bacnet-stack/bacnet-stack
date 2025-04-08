/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2025
 * @brief The Program object type defines a standardized object whose
 *  properties represent the externally visible characteristics of an
 *  application program.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/proplist.h"
/* basic objects and services */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bacnet/basic/object/program.h"

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List = NULL;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_PROGRAM;

struct object_data {
    BACNET_PROGRAM_STATE Program_State;
    BACNET_PROGRAM_REQUEST Program_Change;
    BACNET_PROGRAM_ERROR Reason_For_Halt;
    const char *Description_Of_Halt;
    const char *Program_Location;
    const char *Instance_Of;
    const char *Description;
    const char *Object_Name;
    BACNET_RELIABILITY Reliability;
    bool Out_Of_Service : 1;
    bool Changed : 1;
    void *Context;
    /* return 0 for success, negative on error */
    int (*Load)(void *context, const char *location);
    int (*Run)(void *context);
    int (*Halt)(void *context);
    int (*Restart)(void *context);
    int (*Unload)(void *context);
};

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    /* unordered list of properties */
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,       PROP_PROGRAM_STATE,
    PROP_PROGRAM_CHANGE,    PROP_STATUS_FLAGS,
    PROP_OUT_OF_SERVICE,    -1
};

static const int Properties_Optional[] = {
    /* unordered list of properties */
    PROP_REASON_FOR_HALT,
    PROP_DESCRIPTION_OF_HALT,
    PROP_PROGRAM_LOCATION,
    PROP_DESCRIPTION,
    PROP_INSTANCE_OF,
    PROP_RELIABILITY,
    -1
};

static const int Properties_Proprietary[] = { -1 };

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
void Program_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Properties_Required;
    }
    if (pOptional) {
        *pOptional = Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Properties_Proprietary;
    }

    return;
}

/**
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct object_data *Object_Data(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * Determines if a given Integer Value instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Program_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject = Object_Data(object_instance);

    return (pObject != NULL);
}

/**
 * Determines the number of Integer Value objects
 *
 * @return  Number of Integer Value objects
 */
unsigned Program_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Integer Value objects where N is Program_Count().
 *
 * @param  index - 0..MAX_PROGRAMS value
 *
 * @return  object instance-number for the given index
 */
uint32_t Program_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Integer Value objects where N is Program_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or MAX_PROGRAMS
 * if not valid.
 */
unsigned Program_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * For a given object instance-number, determines the program-state
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  program-state of the object
 */
BACNET_PROGRAM_STATE Program_State(uint32_t object_instance)
{
    BACNET_PROGRAM_STATE value = 0;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        value = pObject->Program_State;
    }

    return value;
}

/**
 * For a given object instance-number, sets the program-state
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - integer value
 *
 * @return  true if values are within range and present-value is set.
 */
bool Program_State_Set(uint32_t object_instance, BACNET_PROGRAM_STATE value)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Program_State = value;
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
bool Program_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                text, sizeof(text), "PROGRAM-%lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text);
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
bool Program_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
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
const char *Program_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * For a given object instance-number, return the description.
 * @param  object_instance - object-instance number of the object
 * @param  description - description pointer
 * @return  true/false
 */
bool Program_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Description) {
            status =
                characterstring_init_ansi(description, pObject->Description);
        } else {
            status = characterstring_init_ansi(description, "");
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the description
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if string was set
 */
bool Program_Description_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = true;
        pObject->Description = new_name;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
const char *Program_Description_ANSI(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Description == NULL) {
            name = "";
        } else {
            name = pObject->Description;
        }
    }

    return name;
}

/**
 * For a given object instance-number, return the Description_Of_Halt.
 *
 * @param  object_instance - object-instance number of the object
 * @param  description - description pointer
 *
 * @return  true/false
 */
bool Program_Description_Of_Halt(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Description_Of_Halt) {
            status = characterstring_init_ansi(
                description, pObject->Description_Of_Halt);
        } else {
            status = characterstring_init_ansi(description, "");
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the Description_Of_Halt
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if string was set
 */
bool Program_Description_Of_Halt_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = true;
        pObject->Description_Of_Halt = new_name;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the Description_Of_Halt
 * @param  object_instance - object-instance number of the object
 * @return text or NULL if not found
 */
const char *Program_Description_Of_Halt_ANSI(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Description_Of_Halt == NULL) {
            name = "";
        } else {
            name = pObject->Description_Of_Halt;
        }
    }

    return name;
}

/**
 * For a given object instance-number, return the Description_Of_Halt.
 *
 * @param  object_instance - object-instance number of the object
 * @param  description - description pointer
 *
 * @return  true/false
 */
bool Program_Location(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Program_Location) {
            status = characterstring_init_ansi(
                description, pObject->Program_Location);
        } else {
            status = characterstring_init_ansi(description, "");
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the Program_Location
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if string was set
 */
bool Program_Location_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = true;
        pObject->Program_Location = new_name;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the Program_Location
 * @param  object_instance - object-instance number of the object
 * @return text or NULL if not found
 */
const char *Program_Location_ANSI(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Program_Location == NULL) {
            name = "";
        } else {
            name = pObject->Program_Location;
        }
    }

    return name;
}

/**
 * For a given object instance-number, return the Instance_Of string
 *
 * @param  object_instance - object-instance number of the object
 * @param  description - description pointer
 *
 * @return  true/false if the string was found
 */
bool Program_Instance_Of(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Instance_Of) {
            status =
                characterstring_init_ansi(description, pObject->Instance_Of);
        } else {
            status = characterstring_init_ansi(description, "");
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the Instance_Of
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if string was set
 */
bool Program_Instance_Of_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = true;
        pObject->Instance_Of = new_name;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the Instance_Of
 * @param  object_instance - object-instance number of the object
 * @return text or NULL if not found
 */
const char *Program_Instance_Of_ANSI(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Instance_Of == NULL) {
            name = "";
        } else {
            name = pObject->Instance_Of;
        }
    }

    return name;
}

/**
 * For a given object instance-number, returns the program change value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  program change property value
 */
BACNET_PROGRAM_REQUEST Program_Change(uint32_t object_instance)
{
    uint16_t units = UNITS_NO_UNITS;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        units = pObject->Program_Change;
    }

    return units;
}

/**
 * For a given object instance-number, sets the program change property value
 *
 * @param object_instance - object-instance number of the object
 * @param program_change - property value
 *
 * @return true if the program change property value was set
 */
bool Program_Change_Set(
    uint32_t object_instance, BACNET_PROGRAM_REQUEST program_change)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Program_Change = program_change;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, writes the program change value
 *
 * Normally the value of the Program_Change property will be READY,
 * meaning that the program is ready to accept a new
 * request to change its operating state. If the Program_Change
 * property is not READY, then it may not be written to and any
 * attempt to write to the property shall return a Result(-).
 * If it has one of the other enumerated values, then a previous
 * request to change state has not yet been honored, so new requests
 * cannot be accepted. When the request to change state is finally
 * honored, then the Program_Change property value shall become
 * READY and the new state shall be reflected in the Program_State property.
 *
 * @param object_instance - object-instance number of the object
 * @param program_change - property value
 * @param error_class - BACNET_ERROR_CLASS
 * @param error_code - BACNET_ERROR_CODE
 * @return true if the program change property value was written
 */
static bool Program_Change_Write(
    uint32_t object_instance,
    BACNET_PROGRAM_REQUEST program_change,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        if (pObject->Program_Change == PROGRAM_REQUEST_READY) {
            if (program_change <= PROGRAM_REQUEST_MAX) {
                pObject->Program_Change = program_change;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the Reason_For_Halt
 *
 * @param object_instance - object-instance number of the object
 *
 * @return Reason_For_Halt property value
 */
BACNET_PROGRAM_ERROR Program_Reason_For_Halt(uint32_t object_instance)
{
    uint16_t units = UNITS_NO_UNITS;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        units = pObject->Reason_For_Halt;
    }

    return units;
}

/**
 * For a given object instance-number, sets the Reason_For_Halt property value
 *
 * @param object_instance - object-instance number of the object
 * @param program_change - property value
 *
 * @return true if the Reason_For_Halt property value was set
 */
bool Program_Reason_For_Halt_Set(
    uint32_t object_instance, BACNET_PROGRAM_ERROR reason)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Reason_For_Halt = reason;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the out-of-service
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  out-of-service property value
 */
bool Program_Out_Of_Service(uint32_t object_instance)
{
    struct object_data *pObject = Object_Data(object_instance);
    bool value = false;

    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * For a given object instance-number, sets the out-of-service property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - boolean out-of-service value
 *
 * @return true if the out-of-service property value was set
 */
void Program_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Out_Of_Service = value;
    }
}

/**
 * @brief For a given object instance-number, gets the reliability.
 * @param  object_instance - object-instance number of the object
 * @return reliability value
 */
BACNET_RELIABILITY Program_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY reliability = RELIABILITY_NO_FAULT_DETECTED;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        reliability = pObject->Reliability;
    }

    return reliability;
}

/**
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Program_Fault(uint32_t object_instance)
{
    struct object_data *pObject;
    bool fault = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Reliability != RELIABILITY_NO_FAULT_DETECTED) {
            fault = true;
        }
    }

    return fault;
}

/**
 * @brief For a given object instance-number, sets the reliability
 * @param  object_instance - object-instance number of the object
 * @param  value - reliability enumerated value
 * @return  true if values are within range and property is set.
 */
bool Program_Reliability_Set(uint32_t object_instance, BACNET_RELIABILITY value)
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
 * ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int Program_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
    uint32_t enum_value = 0;
    bool state = false;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Program_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_DESCRIPTION:
            if (Program_Description(rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            state = Program_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Program_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Program_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_PROGRAM_STATE:
            enum_value = Program_State(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_PROGRAM_CHANGE:
            enum_value = Program_Change(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_REASON_FOR_HALT:
            enum_value = Program_Reason_For_Halt(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_DESCRIPTION_OF_HALT:
            if (Program_Description_Of_Halt(
                    rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_PROGRAM_LOCATION:
            if (Program_Location(rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_INSTANCE_OF:
            if (Program_Instance_Of(rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_RELIABILITY:
            enum_value = Program_Reliability(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/**
 * WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return false if an error is loaded, true if no errors
 */
bool Program_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
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
        case PROP_PROGRAM_CHANGE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                status = Program_Change_Write(
                    wp_data->object_instance, value.type.Enumerated,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Program_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        default:
            if (property_lists_member(
                    Properties_Required, Properties_Optional,
                    Properties_Proprietary, wp_data->object_property)) {
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
 * @brief Set the context used with load, unload, run, halt, and restart
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void Program_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Set the Load function for the object
 * @param object_instance [in] BACnet object instance number
 * @param load [in] pointer to the Load function
 */
void Program_Load_Set(
    uint32_t object_instance, int (*load)(void *context, const char *location))
{
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Load = load;
    }
}

/**
 * @brief Set the Run function for the object
 * @param object_instance [in] BACnet object instance number
 * @param run [in] pointer to the Run function
 */
void Program_Run_Set(uint32_t object_instance, int (*run)(void *context))
{
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Run = run;
    }
}

/**
 * @brief Set the Halt function for the object
 * @param object_instance [in] BACnet object instance number
 * @param halt [in] pointer to the Halt function
 */
void Program_Halt_Set(uint32_t object_instance, int (*halt)(void *context))
{
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Halt = halt;
    }
}

/**
 * @brief Set the Restart function for the object
 * @param object_instance [in] BACnet object instance number
 * @param restart [in] pointer to the Restart function
 */
void Program_Restart_Set(
    uint32_t object_instance, int (*restart)(void *context))
{
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Restart = restart;
    }
}

/**
 * @brief Set the Unload function for the object
 * @param object_instance [in] BACnet object instance number
 * @param unload [in] pointer to the Unload function
 */
void Program_Unload_Set(uint32_t object_instance, int (*unload)(void *context))
{
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Unload = unload;
    }
}

static void Program_State_Idle_Handler(struct object_data *pObject)
{
    int err;

    if (pObject->Program_Change == PROGRAM_REQUEST_LOAD) {
        if (pObject->Load) {
            err = pObject->Load(pObject->Context, pObject->Program_Location);
            if (err == 0) {
                pObject->Program_State = PROGRAM_STATE_LOADING;
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_LOAD_FAILED;
            }
        } else {
            pObject->Program_State = PROGRAM_STATE_LOADING;
            pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
        }
    } else if (pObject->Program_Change == PROGRAM_REQUEST_RUN) {
        if (pObject->Load) {
            err = pObject->Load(pObject->Context, pObject->Program_Location);
            if (err == 0) {
                pObject->Program_State = PROGRAM_STATE_RUNNING;
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_LOAD_FAILED;
            }
        } else {
            pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            pObject->Program_State = PROGRAM_STATE_RUNNING;
        }
    } else if (pObject->Program_Change == PROGRAM_REQUEST_RESTART) {
        if (pObject->Restart) {
            err = pObject->Restart(pObject->Context);
            if (err == 0) {
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
                pObject->Program_State = PROGRAM_STATE_RUNNING;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_OTHER;
            }
        } else {
            pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            pObject->Program_State = PROGRAM_STATE_RUNNING;
        }
    }
}

static void Program_State_Halted_Handler(struct object_data *pObject)
{
    int err;

    if (pObject->Program_Change == PROGRAM_REQUEST_UNLOAD) {
        if (pObject->Unload) {
            err = pObject->Unload(pObject->Context);
            if (err == 0) {
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
                pObject->Program_State = PROGRAM_STATE_UNLOADING;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_LOAD_FAILED;
            }
        } else {
            pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            pObject->Program_State = PROGRAM_STATE_UNLOADING;
        }
    } else if (pObject->Program_Change == PROGRAM_REQUEST_LOAD) {
        if (pObject->Load) {
            err = pObject->Load(pObject->Context, pObject->Program_Location);
            if (err == 0) {
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
                pObject->Program_State = PROGRAM_STATE_LOADING;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_LOAD_FAILED;
            }
        } else {
            pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            pObject->Program_State = PROGRAM_STATE_LOADING;
        }
    } else if (pObject->Program_Change == PROGRAM_REQUEST_RUN) {
        pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
        pObject->Program_State = PROGRAM_STATE_RUNNING;
    } else if (pObject->Program_Change == PROGRAM_REQUEST_RESTART) {
        if (pObject->Restart) {
            err = pObject->Restart(pObject->Context);
            if (err == 0) {
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
                pObject->Program_State = PROGRAM_STATE_RUNNING;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_OTHER;
            }
        } else {
            pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            pObject->Program_State = PROGRAM_STATE_RUNNING;
        }
    }
}

static void Program_State_Running_Handler(struct object_data *pObject)
{
    int err;

    if (pObject->Program_Change == PROGRAM_REQUEST_UNLOAD) {
        if (pObject->Unload) {
            err = pObject->Unload(pObject->Context);
            if (err == 0) {
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
                pObject->Program_State = PROGRAM_STATE_UNLOADING;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_OTHER;
            }
        } else {
            pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            pObject->Program_State = PROGRAM_STATE_UNLOADING;
        }
    } else if (pObject->Program_Change == PROGRAM_REQUEST_LOAD) {
        if (pObject->Load) {
            err = pObject->Load(pObject->Context, pObject->Program_Location);
            if (err == 0) {
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
                pObject->Program_State = PROGRAM_STATE_LOADING;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_LOAD_FAILED;
            }
        } else {
            pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            pObject->Program_State = PROGRAM_STATE_LOADING;
        }
    } else if (pObject->Program_Change == PROGRAM_REQUEST_HALT) {
        if (pObject->Halt) {
            err = pObject->Halt(pObject->Context);
            if (err == 0) {
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
                pObject->Program_State = PROGRAM_STATE_HALTED;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_OTHER;
            }
        }
        pObject->Program_State = PROGRAM_STATE_HALTED;
    } else if (pObject->Program_Change == PROGRAM_REQUEST_RESTART) {
        if (pObject->Restart) {
            err = pObject->Restart(pObject->Context);
            if (err == 0) {
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
                pObject->Program_State = PROGRAM_STATE_RUNNING;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_OTHER;
            }
        } else {
            pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            pObject->Program_State = PROGRAM_STATE_RUNNING;
        }
    } else {
        if (pObject->Run) {
            err = pObject->Run(pObject->Context);
            if (err == 0) {
                pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
            } else {
                pObject->Reason_For_Halt = PROGRAM_ERROR_PROGRAM;
                pObject->Program_State = PROGRAM_STATE_HALTED;
            }
        }
    }
}

/**
 * @brief Updates the object program operation
 *
 * 12.22.5 Program_Change
 *
 * This property, of type BACnetProgramRequest, is used to request changes
 * to the operating state of the process this object represents.
 * The Program_Change property provides one means for changing
 * the operating state of this process. The process may change its own
 * state as a consequence of execution as well.
 *
 * The values that may be taken on by this property are:
 *   READY ready for change request (the normal state)
 *   LOAD request that the application program be loaded, if not already loaded
 *   RUN request that the process begin executing, if not already running
 *   HALT request that the process halt execution
 *   RESTART request that the process restart at its initialization point
 *   UNLOAD request that the process halt execution and unload
 *
 * Normally the value of the Program_Change property will be READY,
 * meaning that the program is ready to accept a new
 * request to change its operating state. If the Program_Change property
 * is not READY, then it may not be written to and any
 * attempt to write to the property shall return a Result(-).
 * If it has one of the other enumerated values, then a previous request to
 * change state has not yet been honored, so new requests cannot
 * be accepted. When the request to change state is finally
 * honored, then the Program_Change property value shall become READY
 * and the new state shall be reflected in the Program_State property.
 * Depending on the current Program_State, certain requested values for
 * Program_Change may be invalid and would also return a Result(-)
 * if an attempt were made to write them.
 *
 * It is important to note that program loading could be terminated
 * either due to an error or a request to HALT that occurs
 * during loading. In either case, it is possible to have Program_State=HALTED
 * and yet not have a complete or operable program in place.
 * In this case, a request to RESTART is taken to mean LOAD instead.
 * If a complete program is loaded but HALTED for any reason,
 * then RESTART simply reenters program execution at its
 * initialization entry point.
 *
 * There may be BACnet devices
 * that support Program objects but do not require "loading"
 * of the application programs, as these applications may be built in.
 * In these cases, loading is taken to mean "preparing for execution,"
 * the specifics of which are a local matter.
 *
 * @param  object_instance - object-instance number of the object
 * @param milliseconds - number of milliseconds elapsed
 */
void Program_Timer(uint32_t object_instance, uint16_t milliseconds)
{
    struct object_data *pObject;

    (void)milliseconds;
    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        switch (pObject->Program_State) {
            case PROGRAM_STATE_IDLE:
                Program_State_Idle_Handler(pObject);
                break;
            case PROGRAM_STATE_LOADING:
                pObject->Program_State = PROGRAM_STATE_HALTED;
                break;
            case PROGRAM_STATE_UNLOADING:
                pObject->Program_State = PROGRAM_STATE_IDLE;
                break;
            case PROGRAM_STATE_HALTED:
                Program_State_Halted_Handler(pObject);
                break;
            case PROGRAM_STATE_RUNNING:
                Program_State_Running_Handler(pObject);
                break;
            case PROGRAM_STATE_WAITING:
                Program_State_Running_Handler(pObject);
                break;
            default:
                /* do nothing */
                break;
        }
        pObject->Program_Change = PROGRAM_REQUEST_READY;
    }
}

/**
 * @brief Creates a Integer Value object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Program_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index;

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
    if (pObject) {
        /* already exists - signal success but don't change data */
        return object_instance;
    }
    pObject = calloc(1, sizeof(struct object_data));
    if (!pObject) {
        /* no RAM available - signal failure */
        return BACNET_MAX_INSTANCE;
    }
    index = Keylist_Data_Add(Object_List, object_instance, pObject);
    if (index < 0) {
        /* unable to add to list - signal failure */
        free(pObject);
        return BACNET_MAX_INSTANCE;
    }
    pObject->Program_State = PROGRAM_STATE_IDLE;
    pObject->Program_Change = PROGRAM_REQUEST_READY;
    pObject->Reason_For_Halt = PROGRAM_ERROR_NORMAL;
    pObject->Description_Of_Halt = NULL;
    pObject->Program_Location = NULL;
    pObject->Instance_Of = NULL;
    pObject->Description = NULL;
    pObject->Object_Name = NULL;
    pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
    pObject->Out_Of_Service = false;
    pObject->Context = NULL;
    pObject->Load = NULL;
    pObject->Run = NULL;
    pObject->Halt = NULL;
    pObject->Restart = NULL;
    pObject->Unload = NULL;

    return object_instance;
}

/**
 * @brief Deletes an object-instance
 * @param object_instance - object-instance number of the object
 * @return true if the object-instance was deleted
 */
bool Program_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject =
        Keylist_Data_Delete(Object_List, object_instance);

    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all the objects and their data
 */
void Program_Cleanup(void)
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
 * Initializes the object data
 */
void Program_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
