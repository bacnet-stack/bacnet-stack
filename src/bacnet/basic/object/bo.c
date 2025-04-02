/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @brief A basic BACnet Binary Output object implementation.
 * The Binary Lighting Output object is a commandable object, and the
 * present-value property uses a priority array and an enumerated 2-state
 * data type.
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
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bo.h"

static const char *Default_Active_Text = "Active";
static const char *Default_Inactive_Text = "Inactive";
struct object_data {
    bool Out_Of_Service : 1;
    bool Changed : 1;
    bool Relinquish_Default : 1;
    bool Polarity : 1;
    uint16_t Priority_Array;
    uint16_t Priority_Active_Bits;
    uint8_t Reliability;
    const char *Object_Name;
    const char *Active_Text;
    const char *Inactive_Text;
    const char *Description;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_BINARY_OUTPUT;
/* callback for present value writes */
static binary_output_write_present_value_callback
    Binary_Output_Write_Present_Value_Callback;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_POLARITY,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
#if (BACNET_PROTOCOL_REVISION >= 17)
    PROP_CURRENT_COMMAND_PRIORITY,
#endif
    -1
};

static const int Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_RELIABILITY, PROP_DESCRIPTION, PROP_ACTIVE_TEXT, PROP_INACTIVE_TEXT, -1
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
void Binary_Output_Property_Lists(
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
 * Determines if a given Binary Output instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Binary_Output_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Binary Output objects
 *
 * @return  Number of Binary Output objects
 */
unsigned Binary_Output_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Binary Output objects where N is Binary_Output_Count().
 *
 * @param  index - 0..MAX_BINARY_OUTPUTS value
 *
 * @return  object instance-number for the given index
 */
uint32_t Binary_Output_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Binary Output objects where N is Binary_Output_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or MAX_BINARY_OUTPUTS
 * if not valid.
 */
unsigned Binary_Output_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief Calculated the present-value property from the priority array.
 * @param pObject - pointer to the object data
 * @return The present-value of the object
 */
static BACNET_BINARY_PV Object_Present_Value(struct object_data *pObject)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;
    unsigned p = 0;

    if (pObject) {
        if (pObject->Relinquish_Default) {
            value = BINARY_ACTIVE;
        }
        for (p = 0; p < BACNET_MAX_PRIORITY; p++) {
            if (BIT_CHECK(pObject->Priority_Active_Bits, p)) {
                if (BIT_CHECK(pObject->Priority_Array, p)) {
                    value = BINARY_ACTIVE;
                } else {
                    value = BINARY_INACTIVE;
                }
                break;
            }
        }
    }

    return value;
}

/**
 * For a given object instance-number, determines the present-value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  present-value of the object
 */
BACNET_BINARY_PV Binary_Output_Present_Value(uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = Object_Present_Value(pObject);
    }

    return value;
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
static int Binary_Output_Priority_Array_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    struct object_data *pObject;
    BACNET_BINARY_PV value = BINARY_INACTIVE;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && (index < BACNET_MAX_PRIORITY)) {
        if (BIT_CHECK(pObject->Priority_Active_Bits, index)) {
            if (BIT_CHECK(pObject->Priority_Array, index)) {
                value = BINARY_ACTIVE;
            }
            apdu_len = encode_application_enumerated(apdu, value);
        } else {
            apdu_len = encode_application_null(apdu);
        }
    }

    return apdu_len;
}

/**
 * For a given object instance-number, determines the active priority
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  active priority 1..16, or 0 if no priority is active
 */
unsigned Binary_Output_Present_Value_Priority(uint32_t object_instance)
{
    unsigned p = 0; /* loop counter */
    unsigned priority = 0; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        for (p = 0; p < BACNET_MAX_PRIORITY; p++) {
            if (BIT_CHECK(pObject->Priority_Active_Bits, p)) {
                priority = p + 1;
                break;
            }
        }
    }

    return priority;
}

/**
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - enumerated 2-state active or inactive value
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
bool Binary_Output_Present_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV binary_value, unsigned priority)
{
    bool status = false;
    struct object_data *pObject;
    BACNET_BINARY_PV old_value, new_value;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */)) {
            priority--;
            old_value = Object_Present_Value(pObject);
            if (binary_value <= MAX_BINARY_PV) {
                BIT_SET(pObject->Priority_Active_Bits, priority);
                if (binary_value == BINARY_ACTIVE) {
                    BIT_SET(pObject->Priority_Array, priority);
                } else {
                    BIT_CLEAR(pObject->Priority_Array, priority);
                }
                status = true;
            }
            new_value = Object_Present_Value(pObject);
            if (old_value != new_value) {
                pObject->Changed = true;
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, relinquishes the present-value
 * at a given priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
bool Binary_Output_Present_Value_Relinquish(
    uint32_t object_instance, unsigned priority)
{
    bool status = false;
    struct object_data *pObject;
    BACNET_BINARY_PV old_value, new_value;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */)) {
            priority--;
            old_value = Object_Present_Value(pObject);
            BIT_CLEAR(pObject->Priority_Active_Bits, priority);
            BIT_CLEAR(pObject->Priority_Array, priority);
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
 * remote node
 * @param  object_instance - object-instance number of the object
 * @param  value - present-value
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and present-value is set.
 */
static bool Binary_Output_Present_Value_Write(
    uint32_t object_instance,
    BACNET_BINARY_PV value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    BACNET_BINARY_PV old_value = BINARY_INACTIVE;
    BACNET_BINARY_PV new_value = BINARY_INACTIVE;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY) &&
            (value <= MAX_BINARY_PV)) {
            if (priority != 6) {
                old_value = Object_Present_Value(pObject);
                Binary_Output_Present_Value_Set(
                    object_instance, value, priority);
                new_value = Object_Present_Value(pObject);
                if (pObject->Out_Of_Service) {
                    /* The physical point that the object represents
                        is not in service. This means that changes to the
                        Present_Value property are decoupled from the
                        physical output when the value of Out_Of_Service
                        is true. */
                } else if (Binary_Output_Write_Present_Value_Callback) {
                    Binary_Output_Write_Present_Value_Callback(
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
 * remote node
 * @param  object_instance - object-instance number of the object
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and write is requested
 */
static bool Binary_Output_Present_Value_Relinquish_Write(
    uint32_t object_instance,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    BACNET_BINARY_PV old_value = BINARY_INACTIVE;
    BACNET_BINARY_PV new_value = BINARY_INACTIVE;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY)) {
            if (priority != 6) {
                old_value = Object_Present_Value(pObject);
                Binary_Output_Present_Value_Relinquish(
                    object_instance, priority);
                new_value = Object_Present_Value(pObject);
                if (pObject->Out_Of_Service) {
                    /* The physical point that the object represents
                        is not in service. This means that changes to the
                        Present_Value property are decoupled from the
                        physical output when the value of Out_Of_Service
                        is true. */
                } else if (Binary_Output_Write_Present_Value_Callback) {
                    Binary_Output_Write_Present_Value_Callback(
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
 * For a given object instance-number, returns the out-of-service
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  out-of-service property value
 */
bool Binary_Output_Out_Of_Service(uint32_t object_instance)
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
 * For a given object instance-number, sets the out-of-service property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - boolean out-of-service value
 *
 * @return true if the out-of-service property value was set
 */
void Binary_Output_Out_Of_Service_Set(uint32_t object_instance, bool value)
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
 * For a given object instance-number, loads the object-name into
 * a characterstring. Note that the object name must be unique
 * within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Binary_Output_Object_Name(
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
                name_text, sizeof(name_text), "BINARY OUTPUT %lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, name_text);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the object-name
 * Note that the object name must be unique within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 *
 * @return  true if object-name was set
 */
bool Binary_Output_Name_Set(uint32_t object_instance, const char *new_name)
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
const char *Binary_Output_Name_ASCII(uint32_t object_instance)
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
 * For a given object instance-number, returns the polarity property.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  the polarity property of the object.
 */
BACNET_POLARITY Binary_Output_Polarity(uint32_t object_instance)
{
    BACNET_POLARITY polarity = POLARITY_NORMAL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Polarity) {
            polarity = POLARITY_REVERSE;
        }
    }

    return polarity;
}

/**
 * For a given object instance-number, sets the polarity property.
 *
 * @param object_instance - object-instance number of the object
 * @param polarity - enumerated polarity property value
 *
 * @return true if the polarity property value was set
 */
bool Binary_Output_Polarity_Set(
    uint32_t object_instance, BACNET_POLARITY polarity)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (polarity < MAX_POLARITY) {
            if (polarity == POLARITY_NORMAL) {
                pObject->Polarity = false;
            } else {
                pObject->Polarity = true;
            }
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the relinquish-default
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  relinquish-default property value
 */
BACNET_BINARY_PV Binary_Output_Relinquish_Default(uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Relinquish_Default) {
            value = BINARY_ACTIVE;
        } else {
            value = BINARY_INACTIVE;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the relinquish-default
 * property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - floating point relinquish-default value
 *
 * @return true if the relinquish-default property value was set
 */
bool Binary_Output_Relinquish_Default_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value == BINARY_ACTIVE) {
            pObject->Relinquish_Default = true;
            status = true;
        } else if (value == BINARY_INACTIVE) {
            pObject->Relinquish_Default = false;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the reliability.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return reliability value
 */
BACNET_RELIABILITY Binary_Output_Reliability(uint32_t object_instance)
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
 * For a given object, gets the Fault status flag
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true the status flag is in Fault
 */
static bool Binary_Output_Object_Fault(const struct object_data *pObject)
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
 * For a given object instance-number, sets the reliability
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - reliability enumerated value
 *
 * @return  true if values are within range and property is set.
 */
bool Binary_Output_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    struct object_data *pObject;
    bool status = false;
    bool fault = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= 255) {
            fault = Binary_Output_Object_Fault(pObject);
            pObject->Reliability = value;
            if (fault != Binary_Output_Object_Fault(pObject)) {
                pObject->Changed = true;
            }
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the Fault status flag
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true the status flag is in Fault
 */
static bool Binary_Output_Fault(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);

    return Binary_Output_Object_Fault(pObject);
}

/**
 * For a given object instance-number, returns the description
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return description text or NULL if not found
 */
const char *Binary_Output_Description(uint32_t object_instance)
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
 * For a given object instance-number, sets the description
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 *
 * @return  true if object-name was set
 */
bool Binary_Output_Description_Set(
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
 * For a given object instance-number, returns the active text value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return active text or NULL if not found
 */
const char *Binary_Output_Active_Text(uint32_t object_instance)
{
    const char *name = NULL;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        name = pObject->Active_Text;
    }

    return name;
}

/**
 * For a given object instance-number, sets the description
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 *
 * @return  true if object-name was set
 */
bool Binary_Output_Active_Text_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        pObject->Active_Text = new_name;
    }

    return status;
}

/**
 * For a given object instance-number, returns the active text value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return active text or NULL if not found
 */
const char *Binary_Output_Inactive_Text(uint32_t object_instance)
{
    const char *name = NULL;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        name = pObject->Inactive_Text;
    }

    return name;
}

/**
 * For a given object instance-number, sets the description
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 *
 * @return  true if object-name was set
 */
bool Binary_Output_Inactive_Text_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        pObject->Inactive_Text = new_name;
    }

    return status;
}

/**
 * Get the COV change flag status
 *
 * @param object_instance - object-instance number of the object
 * @return the COV change flag status
 */
bool Binary_Output_Change_Of_Value(uint32_t object_instance)
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
 * Clear the COV change flag
 *
 * @param object_instance - object-instance number of the object
 */
void Binary_Output_Change_Of_Value_Clear(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Changed = false;
    }
}

/**
 * Encode the Value List for Present-Value and Status-Flags
 *
 * @param object_instance - object-instance number of the object
 * @param  value_list - #BACNET_PROPERTY_VALUE with at least 2 entries
 *
 * @return true if values were encoded
 */
bool Binary_Output_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;
    struct object_data *pObject;
    const bool in_alarm = false;
    bool fault = false;
    const bool overridden = false;
    BACNET_BINARY_PV value;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        fault = Binary_Output_Object_Fault(pObject);
        value = Object_Present_Value(pObject);
        status = cov_value_list_encode_enumerated(
            value_list, value, in_alarm, fault, overridden,
            pObject->Out_Of_Service);
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
int Binary_Output_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_BINARY_PV present_value = BINARY_INACTIVE;
    BACNET_POLARITY polarity = POLARITY_NORMAL;
    unsigned i = 0;
    bool state = false;
    uint8_t *apdu = NULL;
    int apdu_size = 0;

    if ((rpdata->application_data == NULL) ||
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
            Binary_Output_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            present_value =
                Binary_Output_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            state = Binary_Output_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Binary_Output_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Output_Reliability(rpdata->object_instance));
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Binary_Output_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_POLARITY:
            polarity = Binary_Output_Polarity(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], polarity);
            break;
        case PROP_PRIORITY_ARRAY:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Binary_Output_Priority_Array_Encode, BACNET_MAX_PRIORITY, apdu,
                apdu_size);
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
                Binary_Output_Relinquish_Default(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Binary_Output_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_ACTIVE_TEXT:
            characterstring_init_ansi(
                &char_string,
                Binary_Output_Active_Text(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_INACTIVE_TEXT:
            characterstring_init_ansi(
                &char_string,
                Binary_Output_Inactive_Text(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_CURRENT_COMMAND_PRIORITY:
            i = Binary_Output_Present_Value_Priority(rpdata->object_instance);
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
 * WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return false if an error is loaded, true if no errors
 */
bool Binary_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
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
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                status = Binary_Output_Present_Value_Write(
                    wp_data->object_instance, value.type.Enumerated,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            } else {
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_NULL);
                if (status) {
                    status = Binary_Output_Present_Value_Relinquish_Write(
                        wp_data->object_instance, wp_data->priority,
                        &wp_data->error_class, &wp_data->error_code);
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Binary_Output_Out_Of_Service_Set(
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
    /* not using len at this time */
    (void)len;

    return status;
}

/**
 * @brief Sets a callback used when present-value is written from BACnet
 * @param cb - callback used to provide indications
 */
void Binary_Output_Write_Present_Value_Callback_Set(
    binary_output_write_present_value_callback cb)
{
    Binary_Output_Write_Present_Value_Callback = cb;
}

/**
 * @brief Creates a Binary Output object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Binary_Output_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;

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
            pObject->Out_Of_Service = false;
            pObject->Active_Text = Default_Active_Text;
            pObject->Inactive_Text = Default_Inactive_Text;
            pObject->Changed = false;
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
 * Initializes the Binary Input object data
 */
void Binary_Output_Cleanup(void)
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
 * Creates a Binary Input object
 */
bool Binary_Output_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * Initializes the Binary Input object data
 */
void Binary_Output_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
