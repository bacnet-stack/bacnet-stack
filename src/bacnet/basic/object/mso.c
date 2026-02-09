/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @brief The Multi-State object is an object with a present-value that
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
#include "bacnet/bacerror.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/cov.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/proplist.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "mso.h"

struct object_data {
    bool Out_Of_Service : 1;
    bool Changed : 1;
    bool Relinquished[BACNET_MAX_PRIORITY];
    uint8_t Priority_Array[BACNET_MAX_PRIORITY];
    uint8_t Relinquish_Default;
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
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_MULTI_STATE_OUTPUT;
/* callback for present value writes */
static multistate_output_write_present_value_callback
    Multistate_Output_Write_Present_Value_Callback;
/* default state text when none is specified */
static const char *Default_State_Text = "State 1\0"
                                        "State 2\0"
                                        "State 3\0";
/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* list of required properties in the object */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_NUMBER_OF_STATES,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
#if (BACNET_PROTOCOL_REVISION >= 17)
    PROP_CURRENT_COMMAND_PRIORITY,
#endif
    -1
};

static const int32_t Properties_Optional[] = {
    /* list of required properties in the object */
    PROP_STATE_TEXT, PROP_DESCRIPTION, PROP_RELIABILITY, -1
};

static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of always writable properties */
    PROP_PRESENT_VALUE, PROP_OUT_OF_SERVICE, PROP_RELINQUISH_DEFAULT, -1
};

/**
 * @brief Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optkional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void Multistate_Output_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
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
 * @brief Get the list of writable properties for a Multi-State Output object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Multistate_Output_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * @brief Determines if a given Multistate instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Multistate_Output_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of Multistate objects
 * @return  Number of Multistate objects
 */
unsigned Multistate_Output_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * of Multistate objects where N is Multistate_Output_Count().
 * @param  index - 0..Multistate_Output_Count() value
 * @return  object instance-number for the given index
 */
uint32_t Multistate_Output_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * @brief For a given object instance-number, determines a 0..N index
 * of Multistate objects where N is Multistate_Output_Count().
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number, or Multistate_Output_Count()
 * if not valid.
 */
unsigned Multistate_Output_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
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
 * @return  number of states
 */
uint32_t Multistate_Output_Max_States(uint32_t object_instance)
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
 * @brief For a given object instance-number, determines the present-value
 * @param  object_instance - object-instance number of the object
 * @return  present-value of the object
 */
static uint32_t Object_Present_Value(const struct object_data *pObject)
{
    uint32_t value = 1;
    uint8_t priority = 0; /* loop counter */

    if (pObject) {
        value = pObject->Relinquish_Default;
        for (priority = 0; priority < BACNET_MAX_PRIORITY; priority++) {
            if (!pObject->Relinquished[priority]) {
                value = pObject->Priority_Array[priority];
                break;
            }
        }
    }

    return value;
}

/**
 * @brief For a given object instance-number, determines the present-value
 * @param  object_instance - object-instance number of the object
 * @return  present-value of the object
 */
uint32_t Multistate_Output_Present_Value(uint32_t object_instance)
{
    uint32_t value = 1;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    value = Object_Present_Value(pObject);

    return value;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param priority [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Multistate_Output_Priority_Array_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX priority, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    uint32_t value = 1;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && (priority < BACNET_MAX_PRIORITY)) {
        if (pObject->Relinquished[priority]) {
            apdu_len = encode_application_null(apdu);
        } else {
            value = pObject->Priority_Array[priority];
            apdu_len = encode_application_unsigned(apdu, value);
        }
    }

    return apdu_len;
}

/**
 * @brief For a given object instance-number, determines the active priority
 * @param  object_instance - object-instance number of the object
 * @return  active priority 1..16, or 0 if no priority is active
 */
unsigned Multistate_Output_Present_Value_Priority(uint32_t object_instance)
{
    unsigned p = 0; /* loop counter */
    unsigned priority = 0; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        for (p = 0; p < BACNET_MAX_PRIORITY; p++) {
            if (!pObject->Relinquished[p]) {
                priority = p + 1;
                break;
            }
        }
    }

    return priority;
}

/**
 * @brief For a given object instance-number, determines the
 *  relinquish-default value
 * @param object_instance - object-instance number
 * @return relinquish-default value of the object
 */
uint32_t Multistate_Output_Relinquish_Default(uint32_t object_instance)
{
    uint32_t value = 1;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Relinquish_Default;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the relinquish-default value
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point analog output relinquish-default value
 * @return  true if values are within range and relinquish-default value is set.
 */
bool Multistate_Output_Relinquish_Default_Set(
    uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Relinquish_Default = value;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, writes the present-value to the
 *  remote node
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point analog value
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and present-value is set.
 */
static bool Multistate_Output_Relinquish_Default_Write(
    uint32_t object_instance,
    uint32_t value,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    uint32_t old_value = 0;
    uint32_t new_value = 0;
    unsigned max_states = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        max_states = state_name_count(pObject->State_Text);
        if ((value >= 1) && (value <= max_states)) {
            old_value = Object_Present_Value(pObject);
            Multistate_Output_Relinquish_Default_Set(object_instance, value);
            if (pObject->Out_Of_Service) {
                /* The physical point that the object represents
                    is not in service. This means that changes to the
                    Present_Value property are decoupled from the
                    physical output when the value of Out_Of_Service
                    is true. */
            } else if (Multistate_Output_Write_Present_Value_Callback) {
                new_value = Object_Present_Value(pObject);
                Multistate_Output_Write_Present_Value_Callback(
                    object_instance, old_value, new_value);
            }
            status = true;
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
 * @brief For a given object instance-number, sets the present-value
 * @param  object_instance - object-instance number of the object
 * @param  value - integer multi-state value 1..N
 * @param  priority - priority-array index value 1..16
 * @return  true if values are within range and present-value is set.
 */
bool Multistate_Output_Present_Value_Set(
    uint32_t object_instance, uint32_t value, unsigned priority)
{
    bool status = false;
    uint32_t old_value = 0;
    uint32_t new_value = 0;
    struct object_data *pObject;
    unsigned max_states = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        max_states = state_name_count(pObject->State_Text);
        if ((value >= 1) && (value <= max_states) && (priority >= 1) &&
            (priority <= BACNET_MAX_PRIORITY)) {
            old_value = Object_Present_Value(pObject);
            pObject->Relinquished[priority - 1] = false;
            pObject->Priority_Array[priority - 1] = value;
            new_value = Object_Present_Value(pObject);
            if (old_value != new_value) {
                pObject->Changed = true;
            }
            status = true;
        }
    }

    return status;
}

/**
 * @brief Determine if a priority-array slot is relinquished
 * @param object_instance [in] BACnet network port object instance number
 * @param  priority - priority-array index value 1..16
 * @return true if the priority-array slot is relinquished
 */
bool Multistate_Output_Priority_Array_Relinquished(
    uint32_t object_instance, unsigned priority)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY)) {
            status = pObject->Relinquished[priority - 1];
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, determines the
 *  priority-array value
 * @param object_instance - object-instance number
 * @param priority - priority-array index value 1..16
 * @return priority-array value of the object, or 0 if
 *  object not found, or priority out of range, or relinquished
 */
uint32_t Multistate_Output_Priority_Array_Value(
    uint32_t object_instance, unsigned priority)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY)) {
            value = pObject->Priority_Array[priority - 1];
        }
    }

    return value;
}

/**
 * @brief For a given object instance-number, relinquishes the present-value
 * @param  object_instance - object-instance number of the object
 * @param  priority - priority-array index value 1..16
 * @return  true if values are within range and present-value is relinquished.
 */
bool Multistate_Output_Present_Value_Relinquish(
    uint32_t object_instance, unsigned priority)
{
    bool status = false;
    uint32_t old_value = 0;
    uint32_t new_value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY)) {
            old_value = Object_Present_Value(pObject);
            pObject->Relinquished[priority - 1] = true;
            pObject->Priority_Array[priority - 1] = 0;
            new_value = Object_Present_Value(pObject);
            if (old_value != new_value) {
                pObject->Changed = true;
            }
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, writes the present-value to the
 *  remote node
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point analog value
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and present-value is set.
 */
static bool Multistate_Output_Present_Value_Write(
    uint32_t object_instance,
    uint32_t value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    uint32_t old_value = 0;
    uint32_t new_value = 0;
    unsigned max_states = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        max_states = state_name_count(pObject->State_Text);
        if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY) &&
            (value >= 1) && (value <= max_states)) {
            if (priority != 6) {
                old_value = Object_Present_Value(pObject);
                Multistate_Output_Present_Value_Set(
                    object_instance, value, priority);
                if (pObject->Out_Of_Service) {
                    /* The physical point that the object represents
                        is not in service. This means that changes to the
                        Present_Value property are decoupled from the
                        physical output when the value of Out_Of_Service
                        is true. */
                } else if (Multistate_Output_Write_Present_Value_Callback) {
                    new_value = Object_Present_Value(pObject);
                    Multistate_Output_Write_Present_Value_Callback(
                        object_instance, old_value, new_value);
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
 * @brief For a given object instance-number, writes the present-value to the
 *  remote node
 * @param  object_instance - object-instance number of the object
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and write is requested
 */
static bool Multistate_Output_Present_Value_Relinquish_Write(
    uint32_t object_instance,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    uint32_t old_value = 0;
    uint32_t new_value = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY)) {
            if (priority != 6) {
                old_value = Object_Present_Value(pObject);
                Multistate_Output_Present_Value_Relinquish(
                    object_instance, priority);
                if (pObject->Out_Of_Service) {
                    /* The physical point that the object represents
                        is not in service. This means that changes to the
                        Present_Value property are decoupled from the
                        physical output when the value of Out_Of_Service
                        is true. */
                } else if (Multistate_Output_Write_Present_Value_Callback) {
                    new_value = Object_Present_Value(pObject);
                    Multistate_Output_Write_Present_Value_Callback(
                        object_instance, old_value, new_value);
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
bool Multistate_Output_Out_Of_Service(uint32_t object_instance)
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
 * @brief For a given object instance-number, sets the out-of-service state
 * @param  object_instance - object-instance number of the object
 * @param  value - out-of-service state
 */
void Multistate_Output_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Out_Of_Service != value) {
            pObject->Out_Of_Service = value;
            pObject->Changed = true;
        }
    }
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
bool Multistate_Output_Object_Name(
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
                name_text, sizeof(name_text), "MULTI-STATE OUTPUT %lu",
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
bool Multistate_Output_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
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
const char *Multistate_Output_Name_ASCII(uint32_t object_instance)
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
 * @brief For a given object instance-number, returns the state-text in
 *  a C string.
 * @param  object_instance - object-instance number of the object
 * @param  state_index - state index number 1..N of the text requested
 * @return  C string retrieved
 */
const char *
Multistate_Output_State_Text(uint32_t object_instance, uint32_t state_index)
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
static int Multistate_Output_State_Text_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    const char *pName = NULL; /* return value */
    BACNET_CHARACTER_STRING char_string = { 0 };
    uint32_t state_index = 1;

    state_index += index;
    pName = Multistate_Output_State_Text(object_instance, state_index);
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
bool Multistate_Output_State_Text_List_Set(
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
 * @brief For a given object instance-number, gets the reliability.
 * @param  object_instance - object-instance number of the object
 * @return reliability value
 */
BACNET_RELIABILITY Multistate_Output_Reliability(uint32_t object_instance)
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
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Multistate_Output_Object_Fault(const struct object_data *pObject)
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
bool Multistate_Output_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    struct object_data *pObject;
    bool status = false;
    bool fault = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= 255) {
            fault = Multistate_Output_Object_Fault(pObject);
            pObject->Reliability = value;
            if (fault != Multistate_Output_Object_Fault(pObject)) {
                pObject->Changed = true;
            }
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Multistate_Output_Fault(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);

    return Multistate_Output_Object_Fault(pObject);
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
const char *Multistate_Output_Description(uint32_t object_instance)
{
    const char *name = NULL;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
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
bool Multistate_Output_Description_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
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
bool Multistate_Output_Change_Of_Value(uint32_t object_instance)
{
    bool changed = false;

    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        changed = pObject->Changed;
    }

    return changed;
}

/**
 * @brief Clear the COV change flag
 * @param object_instance - object-instance number of the object
 */
void Multistate_Output_Change_Of_Value_Clear(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Changed = false;
    }
}

/**
 * @brief Encode the Value List for Present-Value and Status-Flags
 * @param object_instance - object-instance number of the object
 * @param  value_list - #BACNET_PROPERTY_VALUE with at least 2 entries
 * @return true if values were encoded
 */
bool Multistate_Output_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;
    struct object_data *pObject;
    const bool in_alarm = false;
    bool fault = false;
    const bool overridden = false;
    uint32_t present_value = 1;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        fault = Multistate_Output_Object_Fault(pObject);
        present_value = Object_Present_Value(pObject);
        status = cov_value_list_encode_unsigned(
            value_list, present_value, in_alarm, fault, overridden,
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
int Multistate_Output_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    int apdu_size = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint32_t present_value = 0;
    unsigned i = 0;
    uint32_t max_states = 0;
    bool state = false;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    if (!property_lists_member(
            Properties_Required, Properties_Optional, Properties_Proprietary,
            rpdata->object_property)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
        return BACNET_STATUS_ERROR;
    }
    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Multistate_Output_Object_Name(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            present_value =
                Multistate_Output_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], present_value);
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            state = Multistate_Output_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Multistate_Output_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0],
                Multistate_Output_Reliability(rpdata->object_instance));
            break;
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Multistate_Output_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_NUMBER_OF_STATES:
            apdu_len = encode_application_unsigned(
                &apdu[apdu_len],
                Multistate_Output_Max_States(rpdata->object_instance));
            break;
        case PROP_PRIORITY_ARRAY:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Multistate_Output_Priority_Array_Encode, BACNET_MAX_PRIORITY,
                apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_RELINQUISH_DEFAULT:
            present_value =
                Multistate_Output_Relinquish_Default(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], present_value);
            break;
        case PROP_STATE_TEXT:
            max_states = Multistate_Output_Max_States(rpdata->object_instance);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Multistate_Output_State_Text_Encode, max_states, apdu,
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
                Multistate_Output_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_CURRENT_COMMAND_PRIORITY:
            i = Multistate_Output_Present_Value_Priority(
                rpdata->object_instance);
            if ((i >= BACNET_MIN_PRIORITY) && (i <= BACNET_MAX_PRIORITY)) {
                apdu_len = encode_application_unsigned(&apdu[0], i);
            } else {
                apdu_len = encode_application_null(&apdu[0]);
            }
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
bool Multistate_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    /* decode the first chunk of the request */
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
                status = false;
                if (value.type.Unsigned_Int <= UINT32_MAX) {
                    status = Multistate_Output_Present_Value_Write(
                        wp_data->object_instance, value.type.Unsigned_Int,
                        wp_data->priority, &wp_data->error_class,
                        &wp_data->error_code);
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_NULL);
                if (status) {
                    status = Multistate_Output_Present_Value_Relinquish_Write(
                        wp_data->object_instance, wp_data->priority,
                        &wp_data->error_class, &wp_data->error_code);
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Multistate_Output_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_RELINQUISH_DEFAULT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = false;
                if (value.type.Unsigned_Int <= UINT32_MAX) {
                    status = Multistate_Output_Relinquish_Default_Write(
                        wp_data->object_instance, value.type.Unsigned_Int,
                        &wp_data->error_class, &wp_data->error_code);
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
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
void Multistate_Output_Write_Present_Value_Callback_Set(
    multistate_output_write_present_value_callback cb)
{
    Multistate_Output_Write_Present_Value_Callback = cb;
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Multistate_Output_Context_Get(uint32_t object_instance)
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
void Multistate_Output_Context_Set(uint32_t object_instance, void *context)
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
uint32_t Multistate_Output_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;
    unsigned priority = 0;

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
            pObject->Changed = false;
            for (priority = 0; priority < BACNET_MAX_PRIORITY; priority++) {
                pObject->Relinquished[priority] = true;
                pObject->Priority_Array[priority] = 0;
            }
            pObject->Relinquish_Default = 1;
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
bool Multistate_Output_Delete(uint32_t object_instance)
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
void Multistate_Output_Cleanup(void)
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
void Multistate_Output_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
