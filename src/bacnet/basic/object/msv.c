/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @brief Multi-State object is an object with a present-value that
 * uses an integer data type with a sequence of 1 to N values.
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
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/services.h"
/* me! */
#include "bacnet/basic/object/msv.h"

struct object_data {
    bool Out_Of_Service : 1;
    bool Change_Of_Value : 1;
    bool Write_Enabled : 1;
    uint8_t Present_Value;
    uint8_t Reliability;
    const char *Object_Name;
    /* The state text functions expect a list of C strings separated by '\0' */
    const char *State_Text;
    const char *Description;
    void *Context;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_MULTI_STATE_VALUE;
/* callback for present value writes */
static multistate_value_write_present_value_callback
    Multistate_Value_Write_Present_Value_Callback;
/* default state text when none is specified */
static const char *Default_State_Text = "State 1\0"
                                        "State 2\0"
                                        "State 3\0";

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,      PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,     PROP_STATUS_FLAGS,     PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,    PROP_NUMBER_OF_STATES, -1
};

static const int Properties_Optional[] = {
    /* unordered list of required properties */
    PROP_DESCRIPTION, PROP_RELIABILITY, PROP_STATE_TEXT, -1
};

static const int Properties_Proprietary[] = { -1 };

/**
 * Initialize the pointers for the required, the optional and the properitary
 * value properties.
 *
 * @param pRequired - Pointer to the pointer of required values.
 * @param pOptional - Pointer to the pointer of optional values.
 * @param pProprietary - Pointer to the pointer of properitary values.
 */
void Multistate_Value_Property_Lists(
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
static struct object_data *Multistate_Value_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief For a given object instance-number, determines a 0..N index
 * of Multistate objects where N is count.
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number, or count (object not found)
 */
unsigned Multistate_Value_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * of objects where N is the count.
 * @param  index - 0..N value
 * @return  object instance-number for a valid given index, or UINT32_MAX
 */
uint32_t Multistate_Value_Index_To_Instance(unsigned index)
{
    uint32_t instance = UINT32_MAX;

    (void)Keylist_Index_Key(Object_List, index, &instance);

    return instance;
}

/**
 * @brief Determines the number of Multistate Input objects
 * @return  Number of Multistate Input objects
 */
unsigned Multistate_Value_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines if a given Multistate Input instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Multistate_Value_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Count the number of states
 * @param state_names - string of null-terminated state names
 * @return number of states
 */
static unsigned state_name_count(const char *state_names)
{
    unsigned count = 0;
    int len = 0;

    if (state_names) {
        do {
            len = strlen(state_names);
            if (len > 0) {
                count++;
                state_names = state_names + len + 1;
            }
        } while (len > 0);
    }

    return count;
}

/**
 * @brief Get the specific state name at index 0..N
 * @param state_names - string of null-terminated state names
 * @param state_index - state index number 1..N of the state names
 * @return state name, or NULL
 */
static const char *state_name_by_index(const char *state_names, unsigned index)
{
    unsigned count = 0;
    int len = 0;

    if (state_names) {
        do {
            len = strlen(state_names);
            if (len > 0) {
                count++;
                if (index == count) {
                    return state_names;
                }
                state_names = state_names + len + 1;
            }
        } while (len > 0);
    }

    return NULL;
}

/**
 * @brief For a given object instance-number, determines number of states
 * @param  object_instance - object-instance number of the object
 * @return  number of states 1..N
 */
uint32_t Multistate_Value_Max_States(uint32_t object_instance)
{
    uint32_t count = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        count = state_name_count(pObject->State_Text);
    }

    return count;
}

/**
 * @brief For a given object instance-number, returns the state-text in
 *  a C string.
 * @param  object_instance - object-instance number of the object
 * @param  state_index - state index number 1..N of the text requested
 * @return  C string retrieved
 */
const char *
Multistate_Value_State_Text(uint32_t object_instance, uint32_t state_index)
{
    const char *pName = NULL; /* return value */
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (state_index > 0) {
            pName = state_name_by_index(pObject->State_Text, state_index);
        }
    }

    return pName;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Multistate_Value_State_Text_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    const char *pName = NULL; /* return value */
    BACNET_CHARACTER_STRING char_string = { 0 };
    uint32_t state_index = 1;

    state_index += index;
    pName = Multistate_Value_State_Text(object_instance, state_index);
    if (pName) {
        characterstring_init_ansi(&char_string, pName);
        apdu_len = encode_application_character_string(apdu, &char_string);
    }

    return apdu_len;
}

/**
 * @brief For a given object instance-number, sets the list of state-text from
 * a C string array. The state_text_list consists of C strings separated
 * by '\0'. For example:
 * {@code
 * static const char *baud_rate_names = {
 *     "9600\0"
 *     "19200\0"
 *     "38400\0"
 *     "57600\0"
 *     "76800\0"
 *     "115200\0"
 * };
 *
 * @param  object_instance - object-instance number of the object
 * @param  state_text_list - array of state names to use in this object
 * @return true if the state text was set
 */
bool Multistate_Value_State_Text_List_Set(
    uint32_t object_instance, const char *state_text_list)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->State_Text = state_text_list;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, determines the present-value
 * @param  object_instance - object-instance number of the object
 * @return  present-value 1..N of the object
 */
uint32_t Multistate_Value_Present_Value(uint32_t object_instance)
{
    uint32_t value = 1;
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Present_Value;
    }

    return value;
}

/**
 * @brief For a given object instance-number, checks the present-value for COV
 * @param  pObject - specific object with valid data
 * @param  value - multistate value
 */
static void Multistate_Value_Present_Value_COV_Detect(
    struct object_data *pObject, uint32_t value)
{
    if (pObject) {
        if (pObject->Present_Value != value) {
            pObject->Change_Of_Value = true;
        }
    }
}

/**
 * @brief For a given object instance-number, sets the present-value
 * @param  object_instance - object-instance number of the object
 * @param  value - integer multi-state value 1..N
 * @return  true if values are within range and present-value is set.
 */
bool Multistate_Value_Present_Value_Set(
    uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;
    unsigned max_states = 0;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        max_states = state_name_count(pObject->State_Text);
        if ((value >= 1) && (value <= max_states)) {
            Multistate_Value_Present_Value_COV_Detect(pObject, value);
            pObject->Present_Value = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - multistate value
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Multistate_Value_Present_Value_Write(
    uint32_t object_instance,
    uint32_t value,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    uint32_t old_value = 1;
    unsigned max_states = 0;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        max_states = state_name_count(pObject->State_Text);
        if ((value >= 1) && (value <= max_states)) {
            if (pObject->Write_Enabled) {
                old_value = pObject->Present_Value;
                Multistate_Value_Present_Value_COV_Detect(pObject, value);
                pObject->Present_Value = value;
                if (pObject->Out_Of_Service) {
                    /* The physical point that the object represents
                        is not in service. This means that changes to the
                        Present_Value property are decoupled from the
                        physical point when the value of Out_Of_Service
                        is true. */
                } else if (Multistate_Value_Write_Present_Value_Callback) {
                    Multistate_Value_Write_Present_Value_Callback(
                        object_instance, old_value, value);
                }
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * @brief For a given object instance-number, determines the
 *  out-of-service state
 * @param  object_instance - object-instance number of the object
 * @return  out-of-service state of the object
 */
bool Multistate_Value_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the out-of-service state
 * @param  object_instance - object-instance number of the object
 * @param  value - out-of-service state
 */
void Multistate_Value_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Out_Of_Service != value) {
            pObject->Change_Of_Value = true;
        }
        pObject->Out_Of_Service = value;
    }

    return;
}

/**
 * For a given object instance-number, sets the out-of-service state
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - out-of-service state
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if value is set, false if not or error occurred
 */
static bool Multistate_Value_Out_Of_Service_Write(
    uint32_t object_instance,
    bool value,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Write_Enabled) {
            Multistate_Value_Out_Of_Service_Set(object_instance, value);
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * @brief For a given object instance-number, loads the object-name into
 *  a characterstring. Note that the object name must be unique
 *  within this device.
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Multistate_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[32];

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "MULTI-STATE VALUE %lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, name_text);
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the object-name
 *  Note that the object name must be unique within this device.
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 * @return  true if object-name was set
 */
bool Multistate_Value_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
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
const char *Multistate_Value_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * @brief For a given object instance-number, gets the reliability.
 * @param  object_instance - object-instance number of the object
 * @return reliability value
 */
BACNET_RELIABILITY Multistate_Value_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY reliability = RELIABILITY_NO_FAULT_DETECTED;
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        reliability = (BACNET_RELIABILITY)pObject->Reliability;
    }

    return reliability;
}

/**
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Multistate_Value_Object_Fault(const struct object_data *pObject)
{
    bool fault = false;

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
bool Multistate_Value_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    struct object_data *pObject;
    bool status = false;
    bool fault = false;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        fault = Multistate_Value_Object_Fault(pObject);
        pObject->Reliability = value;
        if (fault != Multistate_Value_Object_Fault(pObject)) {
            pObject->Change_Of_Value = true;
        }
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Multistate_Value_Fault(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);

    return Multistate_Value_Object_Fault(pObject);
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
const char *Multistate_Value_Description(uint32_t object_instance)
{
    const char *name = NULL;
    const struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        name = pObject->Description;
    }

    return name;
}

/**
 * @brief For a given object instance-number, sets the description
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if object-name was set
 */
bool Multistate_Value_Description_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        status = true;
        pObject->Description = new_name;
    }

    return status;
}

/**
 * @brief Get the COV change flag status
 * @param object_instance - object-instance number of the object
 * @return the COV change flag status
 */
bool Multistate_Value_Change_Of_Value(uint32_t object_instance)
{
    bool changed = false;

    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        changed = pObject->Change_Of_Value;
    }

    return changed;
}

/**
 * @brief Clear the COV change flag
 * @param object_instance - object-instance number of the object
 */
void Multistate_Value_Change_Of_Value_Clear(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        pObject->Change_Of_Value = false;
    }
}

/**
 * @brief Encode the Value List for Present-Value and Status-Flags
 * @param object_instance - object-instance number of the object
 * @param  value_list - #BACNET_PROPERTY_VALUE with at least 2 entries
 * @return true if values were encoded
 */
bool Multistate_Value_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;
    struct object_data *pObject;
    const bool in_alarm = false;
    bool fault = false;
    const bool overridden = false;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        fault = Multistate_Value_Object_Fault(pObject);
        status = cov_value_list_encode_unsigned(
            value_list, pObject->Present_Value, in_alarm, fault, overridden,
            pObject->Out_Of_Service);
    }
    return status;
}

/**
 * @brief ReadProperty handler for this object.  For the given ReadProperty
 *  data, the application_data is loaded or the error flags are set.
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 *  requested data and space for the reply, or error response.
 * @return number of APDU bytes in the response, or
 *  BACNET_STATUS_ERROR on error.
 */
int Multistate_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    int apdu_size = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint32_t present_value = 0;
    uint32_t max_states = 0;
    bool state = false;
    uint8_t *apdu = NULL;

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
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
            Multistate_Value_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            present_value =
                Multistate_Value_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], present_value);
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            state = Multistate_Value_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Multistate_Value_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0],
                Multistate_Value_Reliability(rpdata->object_instance));
            break;
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Multistate_Value_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_NUMBER_OF_STATES:
            apdu_len = encode_application_unsigned(
                &apdu[apdu_len],
                Multistate_Value_Max_States(rpdata->object_instance));
            break;
        case PROP_STATE_TEXT:
            max_states = Multistate_Value_Max_States(rpdata->object_instance);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Multistate_Value_State_Text_Encode, max_states, apdu,
                apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Multistate_Value_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
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
 * @brief WriteProperty handler for this object.  For the given WriteProperty
 *  data, the application_data is loaded or the error flags are set.
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return false if an error is loaded, true if no errors
 */
bool Multistate_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    /* decode the some of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    /* len < application_data_len: extra data for arrays only */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Multistate_Value_Present_Value_Write(
                    wp_data->object_instance, value.type.Enumerated,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                status = Multistate_Value_Out_Of_Service_Write(
                    wp_data->object_instance, value.type.Boolean,
                    &wp_data->error_class, &wp_data->error_code);
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
 * @brief Sets a callback used when present-value is written from BACnet
 * @param cb - callback used to provide indications
 */
void Multistate_Value_Write_Present_Value_Callback_Set(
    multistate_value_write_present_value_callback cb)
{
    Multistate_Value_Write_Present_Value_Callback = cb;
}

/**
 * @brief Determines a object write-enabled flag state
 * @param object_instance - object-instance number of the object
 * @return  write-enabled status flag
 */
bool Multistate_Value_Write_Enabled(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Write_Enabled;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Multistate_Value_Write_Enable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        pObject->Write_Enabled = true;
    }
}

/**
 * @brief For a given object instance-number, clears the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Multistate_Value_Write_Disable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Multistate_Value_Object(object_instance);
    if (pObject) {
        pObject->Write_Enabled = false;
    }
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Multistate_Value_Context_Get(uint32_t object_instance)
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
void Multistate_Value_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Creates a new object and adds it to the object list
 * @param  object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Multistate_Value_Create(uint32_t object_instance)
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
            pObject->State_Text = Default_State_Text;
            pObject->Out_Of_Service = false;
            pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
            pObject->Change_Of_Value = false;
            pObject->Present_Value = 1;
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
 * @brief Delete an object and its data from the object list
 * @param  object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Multistate_Value_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Cleans up the object list and its data
 */
void Multistate_Value_Cleanup(void)
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
 * @brief Initializes the object list
 */
void Multistate_Value_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
