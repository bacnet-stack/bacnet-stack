/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2023
 * @brief A basic BACnet Binary Lighting Output object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/lighting.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/proplist.h"
/* me! */
#include "bacnet/basic/object/blo.h"

/* object property values */
struct object_data {
    const char *Object_Name;
    const char *Description;
    BACNET_RELIABILITY Reliability;
    uint32_t Egress_Time;
    BACNET_BINARY_LIGHTING_PV Feedback_Value;
    BACNET_BINARY_LIGHTING_PV Priority_Array[BACNET_MAX_PRIORITY];
    uint16_t Priority_Active_Bits;
    BACNET_BINARY_LIGHTING_PV Relinquish_Default;
    float Power;
    uint32_t Elapsed_Active_Time;
    BACNET_DATE_TIME Time_Of_Active_Time_Reset;
    uint32_t Strike_Count;
    BACNET_DATE_TIME Time_Of_Strike_Count_Reset;
    /* internal operational properties */
    BACNET_BINARY_LIGHTING_PV Target_Value;
    uint8_t Target_Priority;
    uint32_t Egress_Timer;
    /* bit properties */
    bool Out_Of_Service : 1;
    bool Blink_Warn_Enable : 1;
    bool Egress_Active : 1;
    bool Changed : 1;
    bool Polarity : 1;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* callback for present value writes */
static binary_lighting_output_write_value_callback
    Binary_Lighting_Output_Write_Value_Callback;
static binary_lighting_output_blink_warn_callback
    Binary_Lighting_Output_Blink_Warn_Callback;

/* These arrays are used by the ReadPropertyMultiple handler and
   property-list property (as of protocol-revision 14) */
static const int Binary_Lighting_Output_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_OUT_OF_SERVICE,
    PROP_BLINK_WARN_ENABLE,
    PROP_EGRESS_TIME,
    PROP_EGRESS_ACTIVE,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
#if (BACNET_PROTOCOL_REVISION >= 17)
    PROP_CURRENT_COMMAND_PRIORITY,
#endif
    -1
};
static const int Binary_Lighting_Output_Properties_Optional[] = {
    PROP_DESCRIPTION, PROP_RELIABILITY, PROP_FEEDBACK_VALUE, -1
};

static const int Binary_Lighting_Output_Properties_Proprietary[] = { -1 };

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
void Binary_Lighting_Output_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Binary_Lighting_Output_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Binary_Lighting_Output_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Binary_Lighting_Output_Properties_Proprietary;
    }

    return;
}

/**
 * Determines if a given Lighting Output instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Binary_Lighting_Output_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Lighting Output objects
 *
 * @return  Number of Lighting Output objects
 */
unsigned Binary_Lighting_Output_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Lighting Output objects where N is Binary_Lighting_Output_Count().
 *
 * @param  index - 0..MAX_LIGHTING_OUTPUTS value
 *
 * @return  object instance-number for the given index
 */
uint32_t Binary_Lighting_Output_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Lighting Output objects where N is Binary_Lighting_Output_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or MAX_LIGHTING_OUTPUTS
 * if not valid.
 */
unsigned Binary_Lighting_Output_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief Get the priority-array active status for the specific priority
 * @param object [in] BACnet object instance
 * @param priority [in] array index requested:
 *    0 to N for individual array members
 * @return the priority-array active status for the specific priority
 */
static bool Priority_Array_Active(
    const struct object_data *pObject, BACNET_ARRAY_INDEX priority)
{
    bool active = false;

    if (priority < BACNET_MAX_PRIORITY) {
        if (BIT_CHECK(pObject->Priority_Active_Bits, priority)) {
            active = true;
        }
    }

    return active;
}

/**
 * @brief Get the value of the next highest non-NULL priority, including
 *  Relinquish_Default
 * @param object [in] BACnet object instance
 * @param priority [in] array index requested:
 *    0 to N for individual array members
 * @return The priority-array value for the specific priority
 */
static BACNET_BINARY_LIGHTING_PV Priority_Array_Next_Value(
    const struct object_data *pObject, BACNET_ARRAY_INDEX priority)
{
    BACNET_BINARY_LIGHTING_PV value = BINARY_LIGHTING_PV_OFF;
    unsigned p = 0;

    value = pObject->Relinquish_Default;
    for (p = priority; p < BACNET_MAX_PRIORITY; p++) {
        if (Priority_Array_Active(pObject, p)) {
            value = pObject->Priority_Array[p];
            break;
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
BACNET_BINARY_LIGHTING_PV
Binary_Lighting_Output_Present_Value(uint32_t object_instance)
{
    BACNET_BINARY_LIGHTING_PV value = BINARY_LIGHTING_PV_OFF;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = Priority_Array_Next_Value(pObject, 0);
    }

    return value;
}

/**
 * @brief Get the priority-array value for the specific priority
 * @param object [in] BACnet object instance
 * @param priority [in] array index requested:
 *    0 to N for individual array members
 * @return The priority-array value for the specific priority
 */
static BACNET_BINARY_LIGHTING_PV Priority_Array_Value(
    const struct object_data *pObject, BACNET_ARRAY_INDEX priority)
{
    BACNET_BINARY_LIGHTING_PV value = BINARY_LIGHTING_PV_OFF;

    if (priority < BACNET_MAX_PRIORITY) {
        if (BIT_CHECK(pObject->Priority_Active_Bits, priority)) {
            value = pObject->Priority_Array[priority];
        }
    }

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
static int Binary_Lighting_Output_Priority_Array_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX priority, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_BINARY_LIGHTING_PV value = BINARY_LIGHTING_PV_OFF;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (priority < BACNET_MAX_PRIORITY) {
            if (Priority_Array_Active(pObject, priority)) {
                value = pObject->Priority_Array[priority];
                apdu_len = encode_application_enumerated(apdu, value);
            } else {
                apdu_len = encode_application_null(apdu);
            }
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
static unsigned Present_Value_Priority(const struct object_data *pObject)
{
    unsigned p = 0; /* loop counter */
    unsigned priority = 0; /* return value */

    for (p = 0; p < BACNET_MAX_PRIORITY; p++) {
        if (BIT_CHECK(pObject->Priority_Active_Bits, p)) {
            priority = p + 1;
            break;
        }
    }

    return priority;
}

/**
 * For a given object instance, relinquishes the present-value
 * at a given priority 1..16.
 *
 * @param  object - object instance
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
static bool
Present_Value_Relinquish(struct object_data *pObject, unsigned priority)
{
    bool status = false;

    if (priority && (priority <= BACNET_MAX_PRIORITY) &&
        (priority != 6 /* reserved */)) {
        priority--;
        BIT_CLEAR(pObject->Priority_Active_Bits, priority);
        pObject->Priority_Array[priority] = BINARY_LIGHTING_PV_OFF;
        status = true;
    }

    return status;
}

/**
 * For a given object instance, sets the present-value at a given
 * priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - present value
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Present_Value_Set(
    struct object_data *pObject,
    BACNET_BINARY_LIGHTING_PV value,
    unsigned priority)
{
    bool status = false;

    if (priority && (priority <= BACNET_MAX_PRIORITY) &&
        (priority != 6 /* reserved */)) {
        priority--;
        if ((value == BINARY_LIGHTING_PV_OFF) ||
            (value == BINARY_LIGHTING_PV_ON)) {
            /* The logical state of the output shall be either ON or OFF */
            BIT_SET(pObject->Priority_Active_Bits, priority);
            pObject->Priority_Array[priority] = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, determines the active priority
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  active priority 1..16, or 0 if no priority is active
 */
unsigned Binary_Lighting_Output_Present_Value_Priority(uint32_t object_instance)
{
    unsigned priority = 0; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        priority = Present_Value_Priority(pObject);
    }

    return priority;
}

/**
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - present value
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
bool Binary_Lighting_Output_Present_Value_Set(
    uint32_t object_instance,
    BACNET_BINARY_LIGHTING_PV value,
    unsigned priority)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = Present_Value_Set(pObject, value, priority);
    }

    return status;
}

/**
 * For a given object instance-number, handles an ON or OFF target value
 *
 * @param  object_instance - object-instance number of the object
 */
static void Present_Value_On_Off_Handler(uint32_t object_instance)
{
    uint8_t current_priority;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return;
    }
    current_priority = Present_Value_Priority(pObject);
    if (pObject->Target_Priority <= current_priority) {
        /* we have priority - do something */
        if (pObject->Feedback_Value != pObject->Target_Value) {
            if ((!pObject->Out_Of_Service) &&
                (Binary_Lighting_Output_Write_Value_Callback)) {
                Binary_Lighting_Output_Write_Value_Callback(
                    object_instance, pObject->Feedback_Value,
                    pObject->Target_Value);
            }
            pObject->Feedback_Value = pObject->Target_Value;
        }
        pObject->Target_Value = BINARY_LIGHTING_PV_STOP;
        pObject->Egress_Timer = 0;
    }
}

/**
 * For a given object instance-number, handles an ON or OFF target value
 *
 * @param  object_instance - object-instance number of the object
 */
static void Present_Value_Relinquish_Handler(uint32_t object_instance)
{
    uint8_t current_priority;
    BACNET_BINARY_LIGHTING_PV value;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return;
    }
    current_priority = Present_Value_Priority(pObject);
    if (pObject->Target_Priority != current_priority) {
        /* target priority holds previous priority
           and *any* change after relinquish
           indicates something needs done */
        if (current_priority > BACNET_MAX_PRIORITY) {
            value = pObject->Relinquish_Default;
        } else {
            value = Priority_Array_Value(pObject, current_priority);
        }
        if (pObject->Feedback_Value != value) {
            pObject->Changed = true;
            if ((!pObject->Out_Of_Service) &&
                (Binary_Lighting_Output_Write_Value_Callback)) {
                Binary_Lighting_Output_Write_Value_Callback(
                    object_instance, pObject->Feedback_Value, value);
            }
            pObject->Feedback_Value = value;
        }
        pObject->Target_Value = BINARY_LIGHTING_PV_STOP;
    }
}

/**
 * For a given object instance, handles a WARN target value
 *
 * WARN
 * Executes a blink-warn notification at the
 * specified priority. After the blink-warn
 * notification has been executed the value
 * at the specified priority remains ON.
 *
 * WARN_OFF
 * Executes a blink-warn notification at the
 * specified priority and then writes the value OFF
 * to the specified slot in the priority array
 * after a delay of Egress_Time seconds.
 *
 * WARN_RELINQUISH
 * Executes a blink-warn notification at the
 * specified priority and then relinquishes the value
 * at the specified priority slot
 * after a delay of Egress_Time seconds.
 *
 * The blink-warn notification shall not occur
 * if any of the following conditions occur:
 *   (a) The specified priority is not the highest priority, or
 *   (b) The value at the specified priority is OFF, or
 *   (c) Blink_Warn_Enable is FALSE.
 *
 * In the case of WARN_RELINQUISH,
 *   (d) The value at the specified priority is NULL, or
 *   (e) The value of the next highest non-NULL priority,
 *       including Relinquish_Default, is ON.
 *
 * @param  object_instance - object-instance number of the object
 */
static void Present_Value_Warn_Handler(uint32_t object_instance)
{
    uint8_t current_priority;
    BACNET_BINARY_LIGHTING_PV lighting_value;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return;
    }
    current_priority = Present_Value_Priority(pObject);
    if (pObject->Target_Value == BINARY_LIGHTING_PV_WARN_RELINQUISH) {
        /* relinquish this priority */
        Present_Value_Relinquish(pObject, pObject->Target_Priority);
    }
    if (pObject->Target_Priority > current_priority) {
        /* The specified priority is not the highest priority */
        return;
    }
    lighting_value = Priority_Array_Next_Value(pObject, 0);
    if (lighting_value == BINARY_LIGHTING_PV_OFF) {
        /* The value at the specified priority is OFF */
        return;
    }
    if (!pObject->Blink_Warn_Enable) {
        /* Blink_Warn_Enable is FALSE */
        return;
    }
    if (pObject->Target_Value == BINARY_LIGHTING_PV_WARN_RELINQUISH) {
        if (!Priority_Array_Active(pObject, pObject->Target_Priority)) {
            /* The value at the specified priority is NULL */
            return;
        }
        lighting_value =
            Priority_Array_Next_Value(pObject, pObject->Target_Priority);
        if (lighting_value == BINARY_LIGHTING_PV_ON) {
            /* The value of the next highest non-NULL priority,
                including Relinquish_Default, is ON. */
            return;
        }
        pObject->Target_Priority = Present_Value_Priority(pObject);
    }
    /* the egress time in seconds when a WARN_RELINQUISH or WARN_OFF value
        is written to the Present_Value property. */
    pObject->Egress_Timer = 1000UL * pObject->Egress_Time;
    /* warn at least once */
    if ((!pObject->Out_Of_Service) &&
        (Binary_Lighting_Output_Blink_Warn_Callback)) {
        Binary_Lighting_Output_Blink_Warn_Callback(object_instance);
    }
    /* what to do after egress expires */
    if (pObject->Target_Value == BINARY_LIGHTING_PV_WARN) {
        pObject->Target_Value = BINARY_LIGHTING_PV_ON;
    } else if (pObject->Target_Value == BINARY_LIGHTING_PV_WARN_OFF) {
        pObject->Target_Value = BINARY_LIGHTING_PV_OFF;
    } else if (pObject->Target_Value == BINARY_LIGHTING_PV_WARN_RELINQUISH) {
        pObject->Target_Value = BINARY_LIGHTING_PV_OFF;
    }
}

/**
 * @brief Updates the lighting object feedback value per present-value
 * @param  object_instance - object-instance number of the object
 * @param milliseconds - number of milliseconds elapsed since previously
 * called.  Suggest that this is called every 1000 milliseconds.
 */
void Binary_Lighting_Output_Timer(
    uint32_t object_instance, uint16_t milliseconds)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Egress_Timer > milliseconds) {
            pObject->Egress_Timer -= milliseconds;
            if ((!pObject->Out_Of_Service) &&
                (Binary_Lighting_Output_Blink_Warn_Callback)) {
                Binary_Lighting_Output_Blink_Warn_Callback(object_instance);
            }
            return;
        } else {
            pObject->Egress_Timer = 0;
        }
        switch (pObject->Target_Value) {
            case BINARY_LIGHTING_PV_OFF:
                Present_Value_On_Off_Handler(object_instance);
                break;
            case BINARY_LIGHTING_PV_ON:
                Present_Value_On_Off_Handler(object_instance);
                break;
            case BINARY_LIGHTING_PV_WARN:
                Present_Value_Warn_Handler(object_instance);
                break;
            case BINARY_LIGHTING_PV_WARN_OFF:
                /* Executes a blink-warn notification at the
                   specified priority and then writes the value OFF
                   to the specified slot in the priority array after
                   a delay of Egress_Time seconds. */
                break;
            case BINARY_LIGHTING_PV_WARN_RELINQUISH:
                break;
            case BINARY_LIGHTING_PV_STOP:
                break;
            default:
                break;
        }
    }
}

/**
 * For a given object instance-number, writes the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to write
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Binary_Lighting_Output_Present_Value_Write(
    uint32_t object_instance,
    BACNET_BINARY_LIGHTING_PV value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (priority == 6) {
            /* Command priority 6 is reserved for use by Minimum On/Off
               algorithm and may not be used for other purposes in any
               object. */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        } else if ((priority > 0) && (priority <= BACNET_MAX_PRIORITY)) {
            if (value < BINARY_LIGHTING_PV_MAX) {
                pObject->Target_Value = value;
                pObject->Target_Priority = priority;
                status = Present_Value_Set(pObject, value, priority);
                if (status) {
                    /* ON or OFF only */
                    Present_Value_On_Off_Handler(object_instance);
                }
                status = true;
            } else if (
                (value >= BINARY_LIGHTING_PV_PROPRIETARY_MIN) &&
                (value <= BINARY_LIGHTING_PV_PROPRIETARY_MAX)) {
                pObject->Target_Priority = priority;
                pObject->Target_Value = value;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
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
 * For a given object instance-number, relinquishes the present-value
 * at a given priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
bool Binary_Lighting_Output_Present_Value_Relinquish(
    uint32_t object_instance, unsigned priority)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = Present_Value_Relinquish(pObject, priority);
    }

    return status;
}

/**
 * For a given object instance-number, relinquishes the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to write
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Binary_Lighting_Output_Present_Value_Relinquish_Write(
    uint32_t object_instance,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (priority == 6) {
            /* Command priority 6 is reserved for use by Minimum On/Off
               algorithm and may not be used for other purposes in any
               object. */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        } else if ((priority > 0) && (priority <= BACNET_MAX_PRIORITY)) {
            /* target priority will hold the previous priority */
            pObject->Target_Priority = Present_Value_Priority(pObject);
            pObject->Target_Value = BINARY_LIGHTING_PV_STOP;
            Present_Value_Relinquish(pObject, priority);
            Present_Value_Relinquish_Handler(object_instance);
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
 * For a given object instance-number, loads the object-name into
 * a characterstring. Note that the object name must be unique
 * within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Binary_Lighting_Output_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[32] = "BINARY-LIGHTING-OUTPUT-4194303";

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "BINARY-LIGHTING-OUTPUT-%u",
                object_instance);
            status = characterstring_init_ansi(object_name, name_text);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the object-name
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 *
 * @return  true if object-name was set
 */
bool Binary_Lighting_Output_Name_Set(
    uint32_t object_instance, const char *new_name)
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
const char *Binary_Lighting_Output_Name_ASCII(uint32_t object_instance)
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
 * For a given object instance-number, returns the description
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return description text or NULL if not found
 */
const char *Binary_Lighting_Output_Description(uint32_t object_instance)
{
    const char *name = NULL;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Description) {
            name = pObject->Description;
        } else {
            name = "";
        }
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
bool Binary_Lighting_Output_Description_Set(
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
 * For a given object instance-number, sets the lighting command value
 *
 * @param object_instance - object-instance number of the object
 * @param value - holds the target lighting value
 * @param priority - holds the target priority value
 *
 * @return  true if lighting target value was set
 */
bool Binary_Lighting_Output_Lighting_Command_Set(
    uint32_t object_instance,
    BACNET_BINARY_LIGHTING_PV value,
    unsigned priority)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Target_Priority = priority;
        pObject->Target_Value = value;
    }

    return status;
}

/**
 * For a given object instance-number, gets the lighting-command target value
 *
 * @param object_instance - object-instance number of the object
 * @return lighting command target value
 */
BACNET_BINARY_LIGHTING_PV
Binary_Lighting_Output_Lighting_Command_Target_Value(uint32_t object_instance)
{
    BACNET_BINARY_LIGHTING_PV value = BINARY_LIGHTING_PV_OFF;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Target_Value;
    }

    return value;
}

/**
 * For a given object instance-number, gets the lighting-command target value
 *
 * @param object_instance - object-instance number of the object
 * @return lighting command target priority
 */
unsigned Binary_Lighting_Output_Lighting_Command_Target_Priority(
    uint32_t object_instance)
{
    unsigned priority = BACNET_MAX_PRIORITY;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        priority = pObject->Target_Priority;
    }

    return priority;
}

/**
 * For a given object instance-number, gets the tracking-value property
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the tracking-value of this object instance.
 */
BACNET_BINARY_LIGHTING_PV
Binary_Lighting_Output_Feedback_Value(uint32_t object_instance)
{
    BACNET_BINARY_LIGHTING_PV value = BINARY_LIGHTING_PV_OFF;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Feedback_Value;
    }

    return value;
}

/**
 * For a given object instance-number, sets the specific property value of the
 * object.
 *
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 *
 * @return true if value was set
 */
bool Binary_Lighting_Output_Feedback_Value_Set(
    uint32_t object_instance, BACNET_BINARY_LIGHTING_PV value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((value == BINARY_LIGHTING_PV_OFF) ||
            (value == BINARY_LIGHTING_PV_ON)) {
            /* This property shall have the value
                ON (i.e. light is physically on) or
                OFF (i.e. light is physically off). */
            pObject->Feedback_Value = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the blink-warn-enable
 * property value
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the blink-warn-enable property value of this object
 */
bool Binary_Lighting_Output_Blink_Warn_Enable(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Blink_Warn_Enable;
    }

    return value;
}

/**
 * For a given object instance-number, sets the blink-warn-enable
 * property value in the object.
 *
 * @param object_instance - object-instance number of the object
 * @param enable - holds the value to be set
 *
 * @return true if value was set
 */
bool Binary_Lighting_Output_Blink_Warn_Enable_Set(
    uint32_t object_instance, bool enable)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Blink_Warn_Enable = enable;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, gets the egress-time
 * property value
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the egress-time property value of this object
 */
uint32_t Binary_Lighting_Output_Egress_Time(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Egress_Time;
    }

    return value;
}

/**
 * For a given object instance-number, sets the egress-time
 * property value of the object.
 *
 * @param object_instance - object-instance number of the object
 * @param seconds - holds the value to be set
 *
 * @return true if value was set
 */
bool Binary_Lighting_Output_Egress_Time_Set(
    uint32_t object_instance, uint32_t seconds)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Egress_Time = seconds;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, gets the egress-active
 * property value
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the egress-active property value of this object
 */
bool Binary_Lighting_Output_Egress_Active(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Egress_Timer > 0 ? true : false;
    }

    return value;
}

/**
 * For a given object instance-number, returns the out-of-service
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  out-of-service property value
 */
bool Binary_Lighting_Output_Out_Of_Service(uint32_t object_instance)
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
void Binary_Lighting_Output_Out_Of_Service_Set(
    uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Out_Of_Service = value;
    }
}

/**
 * For a given object instance-number, returns the relinquish-default
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  relinquish-default property value
 */
BACNET_BINARY_LIGHTING_PV
Binary_Lighting_Output_Relinquish_Default(uint32_t object_instance)
{
    BACNET_BINARY_LIGHTING_PV value = BINARY_LIGHTING_PV_OFF;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Relinquish_Default;
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
bool Binary_Lighting_Output_Relinquish_Default_Set(
    uint32_t object_instance, BACNET_BINARY_LIGHTING_PV value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((value == BINARY_LIGHTING_PV_OFF) ||
            (value == BINARY_LIGHTING_PV_ON)) {
            pObject->Relinquish_Default = value;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the property value
 * property value
 *
 * @param object_instance - object-instance number of the object
 *
 * @return property value
 */
BACNET_RELIABILITY Binary_Lighting_Output_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY value = RELIABILITY_NO_FAULT_DETECTED;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Reliability;
    }

    return value;
}

/**
 * For a given object instance-number, sets the property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - property value
 *
 * @return true if the property value was set
 */
bool Binary_Lighting_Output_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Reliability = value;
        status = true;
    }

    return status;
}

/**
 * ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - ReadProperty data, including requested data and
 * data for the reply, or error response.
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int Binary_Lighting_Output_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    int apdu_size = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_BINARY_LIGHTING_PV lighting_value;
    uint32_t unsigned_value = 0;
    unsigned i = 0;
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
                &apdu[0], rpdata->object_type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Binary_Lighting_Output_Object_Name(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_LIGHTING_OUTPUT);
            break;
        case PROP_PRESENT_VALUE:
            lighting_value =
                Binary_Lighting_Output_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], lighting_value);
            break;
        case PROP_FEEDBACK_VALUE:
            lighting_value =
                Binary_Lighting_Output_Feedback_Value(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], lighting_value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state =
                Binary_Lighting_Output_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OUT_OF_SERVICE:
            state =
                Binary_Lighting_Output_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_BLINK_WARN_ENABLE:
            state = Binary_Lighting_Output_Blink_Warn_Enable(
                rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_EGRESS_TIME:
            unsigned_value =
                Binary_Lighting_Output_Egress_Time(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_EGRESS_ACTIVE:
            state =
                Binary_Lighting_Output_Egress_Active(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_PRIORITY_ARRAY:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Binary_Lighting_Output_Priority_Array_Encode,
                BACNET_MAX_PRIORITY, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_RELINQUISH_DEFAULT:
            lighting_value = Binary_Lighting_Output_Relinquish_Default(
                rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], lighting_value);
            break;
#if (BACNET_PROTOCOL_REVISION >= 17)
        case PROP_CURRENT_COMMAND_PRIORITY:
            i = Binary_Lighting_Output_Present_Value_Priority(
                rpdata->object_instance);
            if ((i >= BACNET_MIN_PRIORITY) && (i <= BACNET_MAX_PRIORITY)) {
                apdu_len = encode_application_unsigned(&apdu[0], i);
            } else {
                apdu_len = encode_application_null(&apdu[0]);
            }
            break;
#endif
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Binary_Lighting_Output_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_RELIABILITY:
            unsigned_value =
                Binary_Lighting_Output_Reliability(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], unsigned_value);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
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
bool Binary_Lighting_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
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
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                status = Binary_Lighting_Output_Present_Value_Write(
                    wp_data->object_instance, value.type.Enumerated,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            } else {
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_NULL);
                if (status) {
                    status =
                        Binary_Lighting_Output_Present_Value_Relinquish_Write(
                            wp_data->object_instance, wp_data->priority,
                            &wp_data->error_class, &wp_data->error_code);
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Binary_Lighting_Output_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_TRACKING_VALUE:
        case PROP_IN_PROGRESS:
        case PROP_STATUS_FLAGS:
        case PROP_BLINK_WARN_ENABLE:
        case PROP_EGRESS_TIME:
        case PROP_EGRESS_ACTIVE:
        case PROP_PRIORITY_ARRAY:
        case PROP_RELINQUISH_DEFAULT:
        case PROP_LIGHTING_COMMAND_DEFAULT_PRIORITY:
#if (BACNET_PROTOCOL_REVISION >= 17)
        case PROP_CURRENT_COMMAND_PRIORITY:
#endif
        case PROP_DESCRIPTION:
        case PROP_RELIABILITY:
        case PROP_FEEDBACK_VALUE:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}

/**
 * @brief Sets a callback used when present-value is written from BACnet
 * @param cb - callback used to provide indications
 */
void Binary_Lighting_Output_Write_Value_Callback_Set(
    binary_lighting_output_write_value_callback cb)
{
    Binary_Lighting_Output_Write_Value_Callback = cb;
}

/**
 * @brief Sets a callback used when for blink warning notification
 * @param cb - callback used to provide indications
 */
void Binary_Lighting_Output_Blink_Warn_Callback_Set(
    binary_lighting_output_blink_warn_callback cb)
{
    Binary_Lighting_Output_Blink_Warn_Callback = cb;
}

/**
 * @brief Creates a Color object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Binary_Lighting_Output_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;
    unsigned p = 0;

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
        if (!pObject) {
            return BACNET_MAX_INSTANCE;
        }
        pObject->Object_Name = NULL;
        pObject->Description = NULL;
        pObject->Target_Priority = BACNET_MAX_PRIORITY;
        pObject->Out_Of_Service = false;
        pObject->Blink_Warn_Enable = false;
        pObject->Egress_Active = false;
        pObject->Egress_Time = 0;
        pObject->Feedback_Value = BINARY_LIGHTING_PV_OFF;
        pObject->Target_Value = BINARY_LIGHTING_PV_OFF;
        for (p = 0; p < BACNET_MAX_PRIORITY; p++) {
            pObject->Priority_Array[p] = BINARY_LIGHTING_PV_OFF;
            BIT_CLEAR(pObject->Priority_Active_Bits, p);
        }
        pObject->Relinquish_Default = BINARY_LIGHTING_PV_OFF;
        pObject->Power = 0.0;
        /* add to list */
        index = Keylist_Data_Add(Object_List, object_instance, pObject);
        if (index < 0) {
            free(pObject);
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * Deletes an object instance
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Binary_Lighting_Output_Delete(uint32_t object_instance)
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
 * Deletes all the objects and their data
 */
void Binary_Lighting_Output_Cleanup(void)
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
 * Initializes the object list
 */
void Binary_Lighting_Output_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
