/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @brief A basic BACnet Lighting Output object implementation
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
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
#include "bacnet/basic/sys/linear.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/lighting_command.h"
#include "bacnet/bactext.h"
#include "bacnet/proplist.h"
/* me! */
#include "bacnet/basic/object/lo.h"

struct object_data {
    float Present_Value;
    BACNET_LIGHTING_COMMAND_DATA Lighting_Command;
    BACNET_LIGHTING_COMMAND Last_Lighting_Command;
    float Physical_Value;
    uint32_t Egress_Time;
    uint32_t Default_Fade_Time;
    float Default_Ramp_Rate;
    float Default_Step_Increment;
    BACNET_LIGHTING_TRANSITION Transition;
    float Feedback_Value;
    float Priority_Array[BACNET_MAX_PRIORITY];
    uint16_t Priority_Active_Bits;
    float Relinquish_Default;
    float Power;
    float Instantaneous_Power;
    uint8_t Lighting_Command_Default_Priority;
    BACNET_OBJECT_ID Color_Reference;
    BACNET_OBJECT_ID Override_Color_Reference;
    const char *Object_Name;
    const char *Description;
    /* bits */
    bool Blink_Warn_Enable : 1;
    bool Egress_Active : 1;
    bool Color_Override : 1;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* callback for present value writes */
static lighting_command_tracking_value_callback
    Lighting_Command_Tracking_Value_Callback;

/* These arrays are used by the ReadPropertyMultiple handler and
   property-list property (as of protocol-revision 14) */
static const int Lighting_Output_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_TRACKING_VALUE,
    PROP_LIGHTING_COMMAND,
    PROP_IN_PROGRESS,
    PROP_STATUS_FLAGS,
    PROP_OUT_OF_SERVICE,
    PROP_BLINK_WARN_ENABLE,
    PROP_EGRESS_TIME,
    PROP_EGRESS_ACTIVE,
    PROP_DEFAULT_FADE_TIME,
    PROP_DEFAULT_RAMP_RATE,
    PROP_DEFAULT_STEP_INCREMENT,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    PROP_LIGHTING_COMMAND_DEFAULT_PRIORITY,
#if (BACNET_PROTOCOL_REVISION >= 17)
    PROP_CURRENT_COMMAND_PRIORITY,
#endif
    -1
};
static const int Lighting_Output_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_TRANSITION,
#if (BACNET_PROTOCOL_REVISION >= 24)
    PROP_COLOR_OVERRIDE,
    PROP_COLOR_REFERENCE,
    PROP_OVERRIDE_COLOR_REFERENCE,
#endif
    -1
};

static const int Lighting_Output_Properties_Proprietary[] = { -1 };

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
void Lighting_Output_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Lighting_Output_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Lighting_Output_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Lighting_Output_Properties_Proprietary;
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
bool Lighting_Output_Valid_Instance(uint32_t object_instance)
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
unsigned Lighting_Output_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Lighting Output objects where N is Lighting_Output_Count().
 *
 * @param  index - 0..MAX_LIGHTING_OUTPUTS value
 *
 * @return  object instance-number for the given index
 */
uint32_t Lighting_Output_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Lighting Output objects where N is Lighting_Output_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or MAX_LIGHTING_OUTPUTS
 * if not valid.
 */
unsigned Lighting_Output_Instance_To_Index(uint32_t object_instance)
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
 * @brief Get the priority-array value for the specific priority
 * @param object [in] BACnet object instance
 * @param priority [in] array index requested:
 *    0 to N for individual array members
 * @return The priority-array value for the specific priority
 */
static float Priority_Array_Value(
    const struct object_data *pObject, BACNET_ARRAY_INDEX priority)
{
    float real_value = 0.0;

    if (priority < BACNET_MAX_PRIORITY) {
        if (BIT_CHECK(pObject->Priority_Active_Bits, priority)) {
            real_value = pObject->Priority_Array[priority];
        }
    }

    return real_value;
}

/**
 * @brief Get the value of the next highest non-NULL priority, including
 *  Relinquish_Default
 * @param object [in] BACnet object instance
 * @param priority [in] array index requested:
 *    0 to N for individual array members
 * @return The priority-array value for the specific priority
 */
static float Priority_Array_Next_Value(
    const struct object_data *pObject, BACNET_ARRAY_INDEX priority)
{
    float real_value = 0.0;
    unsigned p = 0;

    real_value = pObject->Relinquish_Default;
    for (p = priority; p < BACNET_MAX_PRIORITY; p++) {
        if (Priority_Array_Active(pObject, p)) {
            real_value = pObject->Priority_Array[p];
            break;
        }
    }

    return real_value;
}

/**
 * For a given object instance-number, determines the present-value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  present-value of the object
 */
float Lighting_Output_Present_Value(uint32_t object_instance)
{
    float value = 0.0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = Priority_Array_Next_Value(pObject, 0);
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
static int Lighting_Output_Priority_Array_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX priority, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    float real_value = 0.0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (priority < BACNET_MAX_PRIORITY) {
            if (Priority_Array_Active(pObject, priority)) {
                real_value = pObject->Priority_Array[priority];
                apdu_len = encode_application_real(apdu, real_value);
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
        pObject->Priority_Array[priority] = 0.0;
        status = true;
    }

    return status;
}

/**
 * For a given object instance, sets the present-value at a given
 * priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point analog value
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
static bool
Present_Value_Set(struct object_data *pObject, float value, unsigned priority)
{
    bool status = false;

    if (priority && (priority <= BACNET_MAX_PRIORITY) &&
        (priority != 6 /* reserved */)) {
        priority--;
        BIT_SET(pObject->Priority_Active_Bits, priority);
        pObject->Priority_Array[priority] = value;
        status = true;
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
unsigned Lighting_Output_Present_Value_Priority(uint32_t object_instance)
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
 * @param  value - floating point analog value
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
bool Lighting_Output_Present_Value_Set(
    uint32_t object_instance, float value, unsigned priority)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */)) {
            priority--;
            BIT_SET(pObject->Priority_Active_Bits, priority);
            pObject->Priority_Array[priority] = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Set the lighting command if the priority is active
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 */
static void
Lighting_Command_Warn(struct object_data *pObject, unsigned priority)
{
    unsigned current_priority;

    if (!pObject) {
        return;
    }
    current_priority = Present_Value_Priority(pObject);
    if ((priority <= current_priority) &&
        (Priority_Array_Active(pObject, priority - 1)) &&
        (isgreater(Priority_Array_Value(pObject, priority - 1), 0.0))) {
        /* The blink-warn notification shall not occur
            if any of the following conditions occur:
            (a) The specified priority is not the highest
                active priority, or
            (b) The value at the specified priority is 0.0%, or
            (c) Blink_Warn_Enable is FALSE. */
        lighting_command_blink_warn(
            &pObject->Lighting_Command, BACNET_LIGHTS_WARN,
            &pObject->Lighting_Command.Blink);
    }
}

/**
 * @brief Set the lighting command if the priority is active
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 */
static void
Lighting_Command_Warn_Off(struct object_data *pObject, unsigned priority)
{
    unsigned current_priority;

    if (!pObject) {
        return;
    }
    current_priority = Present_Value_Priority(pObject);
    if ((priority <= current_priority) &&
        (Priority_Array_Active(pObject, priority - 1)) &&
        (isgreater(Priority_Array_Value(pObject, priority - 1), 0.0)) &&
        (isgreater(Priority_Array_Next_Value(pObject, priority - 1), 0.0))) {
        /* The blink-warn notification shall not occur and
            the value 0.0% written at the specified
            priority immediately if any of the following
            conditions occur:
            (a) The specified priority is not the highest
                active priority, or
            (b) The Present_Value is 0.0%, or
            (c) Blink_Warn_Enable is FALSE. */
        lighting_command_blink_warn(
            &pObject->Lighting_Command, BACNET_LIGHTS_WARN_OFF,
            &pObject->Lighting_Command.Blink);
    } else {
        Present_Value_Set(pObject, 0.0, priority);
    }
}

/**
 * @brief Set the lighting command if the priority is active
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 */
static void
Lighting_Command_Warn_Relinquish(struct object_data *pObject, unsigned priority)
{
    unsigned current_priority;

    if (!pObject) {
        return;
    }
    current_priority = Present_Value_Priority(pObject);
    if ((priority <= current_priority) &&
        (Priority_Array_Active(pObject, priority - 1)) &&
        (isgreater(Priority_Array_Value(pObject, priority - 1), 0.0)) &&
        (isgreater(Priority_Array_Next_Value(pObject, priority - 1), 0.0))) {
        /* The blink-warn notification shall not occur,
            and the value at the specified priority shall be
            relinquished immediately if any of the following
            conditions occur:
            (a) The specified priority is not the highest
                active priority, or
            (b) The value at the specified priority
                is 0.0% or NULL, or
            (c) The value of the next highest non-NULL
                priority, including Relinquish_Default,
                is greater than 0.0%, or
            (d) Blink_Warn_Enable is FALSE. */
        lighting_command_blink_warn(
            &pObject->Lighting_Command, BACNET_LIGHTS_WARN_RELINQUISH,
            &pObject->Lighting_Command.Blink);
    } else {
        Present_Value_Relinquish(pObject, priority);
    }
}

/**
 * @brief Set the lighting command if the priority is active
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 * @param value [in] BACnet lighting value
 * @param fade_time [in] BACnet lighting fade time
 */
static void Lighting_Command_Fade_To(
    struct object_data *pObject,
    unsigned priority,
    float value,
    uint32_t fade_time)
{
    unsigned current_priority;

    if (!pObject) {
        return;
    }
    Present_Value_Set(pObject, value, priority);
    current_priority = Present_Value_Priority(pObject);
    if (priority <= current_priority) {
        /* we have priority - configure the Lighting Command */
        lighting_command_fade_to(&pObject->Lighting_Command, value, fade_time);
    }
}

/**
 * @brief Set the lighting command if the priority is active
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 * @param value [in] BACnet lighting value
 * @param ramp_rate [in] BACnet lighting ramp rate
 */
static void Lighting_Command_Ramp_To(
    struct object_data *pObject,
    unsigned priority,
    float value,
    float ramp_rate)
{
    unsigned current_priority;

    if (!pObject) {
        return;
    }
    Present_Value_Set(pObject, value, priority);
    current_priority = Present_Value_Priority(pObject);
    if (priority <= current_priority) {
        /* we have priority - configure the Lighting Command */
        lighting_command_ramp_to(&pObject->Lighting_Command, value, ramp_rate);
    }
}

/**
 * @brief Set the lighting command if the priority is active
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 * @param operation [in] BACnet lighting operation
 * @param step_increment [in] BACnet lighting step increment value
 */
static void Lighting_Command_Step(
    struct object_data *pObject,
    unsigned priority,
    BACNET_LIGHTING_OPERATION operation,
    float step_increment)
{
    unsigned current_priority;

    if (!pObject) {
        return;
    }
    current_priority = Present_Value_Priority(pObject);
    if (priority <= current_priority) {
        /* we have priority - configure the Lighting Command */
        lighting_command_step(
            &pObject->Lighting_Command, operation, step_increment);
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
static bool Lighting_Output_Present_Value_Write(
    uint32_t object_instance,
    float value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    uint8_t current_priority;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (priority == 6) {
            /* Command priority 6 is reserved for use by Minimum On/Off
               algorithm and may not be used for other purposes in any
               object. */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        } else if ((priority > 0) && (priority <= BACNET_MAX_PRIORITY)) {
            /*  Note: Writing a special value has the same effect as writing
                the corresponding lighting command and is subject to the same
                restrictions. The special value itself is not written to the
                priority array. */
            if (!islessgreater(value, -1.0)) {
                /* Provides the same functionality as the
                   WARN lighting command. */
                Lighting_Command_Warn(pObject, priority);
                status = true;
            } else if (!islessgreater(value, -2.0)) {
                /* Provides the same functionality as the
                   WARN_RELINQUISH lighting command. */
                Lighting_Command_Warn_Relinquish(pObject, priority);
                status = true;
            } else if (!islessgreater(value, -3.0)) {
                /* Provides the same functionality as the
                   WARN_OFF lighting command. */
                Lighting_Command_Warn_Off(pObject, priority);
                status = true;
            } else if (
                isgreaterequal(value, 0.0) && islessequal(value, 100.0)) {
                Present_Value_Set(pObject, value, priority);
                current_priority = Present_Value_Priority(pObject);
                if (priority <= current_priority) {
                    /* we have priority - configure the Lighting Command */
                    if (pObject->Transition ==
                        BACNET_LIGHTING_TRANSITION_FADE) {
                        Lighting_Command_Fade_To(
                            pObject, priority, value,
                            pObject->Default_Fade_Time);
                    } else if (
                        pObject->Transition ==
                        BACNET_LIGHTING_TRANSITION_RAMP) {
                        Lighting_Command_Ramp_To(
                            pObject, priority, value,
                            pObject->Default_Ramp_Rate);
                    } else {
                        Lighting_Command_Fade_To(pObject, priority, value, 0);
                    }
                }
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
bool Lighting_Output_Present_Value_Relinquish(
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
static bool Lighting_Output_Present_Value_Relinquish_Write(
    uint32_t object_instance,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    uint8_t old_priority, new_priority;
    float value;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (priority == 6) {
            /* Command priority 6 is reserved for use by Minimum On/Off
               algorithm and may not be used for other purposes in any
               object. */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        } else if ((priority > 0) && (priority <= BACNET_MAX_PRIORITY)) {
            old_priority = Present_Value_Priority(pObject);
            Lighting_Output_Present_Value_Relinquish(object_instance, priority);
            new_priority =
                Lighting_Output_Present_Value_Priority(object_instance);
            if (old_priority != new_priority) {
                if (new_priority > BACNET_MAX_PRIORITY) {
                    /* BACNET_LIGHTS_WARN_RELINQUISH? */
                    value = (float)Lighting_Output_Relinquish_Default(
                        object_instance);
                } else {
                    value = (float)Lighting_Output_Present_Value_Priority(
                        object_instance);
                }
                /* we have priority - configure the Lighting Command */
                if (pObject->Transition == BACNET_LIGHTING_TRANSITION_FADE) {
                    Lighting_Command_Fade_To(
                        pObject, new_priority, value,
                        pObject->Default_Fade_Time);
                } else if (
                    pObject->Transition == BACNET_LIGHTING_TRANSITION_RAMP) {
                    Lighting_Command_Ramp_To(
                        pObject, new_priority, value,
                        pObject->Default_Ramp_Rate);
                } else {
                    Lighting_Command_Fade_To(pObject, new_priority, value, 0);
                }
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
 * For a given object instance-number, loads the object-name into
 * a characterstring. Note that the object name must be unique
 * within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Lighting_Output_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[24] = "LIGHTING-OUTPUT-4194303";

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "LIGHTING-OUTPUT-%u",
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
bool Lighting_Output_Name_Set(uint32_t object_instance, const char *new_name)
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
const char *Lighting_Output_Name_ASCII(uint32_t object_instance)
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
const char *Lighting_Output_Description(uint32_t object_instance)
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
bool Lighting_Output_Description_Set(
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
 * @brief Set the lighting command if the priority is active
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 */
static void
Lighting_Command_Stop(struct object_data *pObject, unsigned priority)
{
    unsigned current_priority;

    if (!pObject) {
        return;
    }
    current_priority = Present_Value_Priority(pObject);
    if (priority <= current_priority) {
        /* we have priority - configure the Lighting Command */
        lighting_command_stop(&pObject->Lighting_Command);
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
static bool Lighting_Output_Lighting_Command_Write(
    uint32_t object_instance,
    const BACNET_LIGHTING_COMMAND *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    if (!value) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return status;
    }
    if (priority == 6) {
        /* Command priority 6 is reserved for use by Minimum On/Off
            algorithm and may not be used for other purposes in any
            object. */
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        return status;
    }
    if ((priority < BACNET_MIN_PRIORITY) || (priority > BACNET_MAX_PRIORITY)) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return status;
    }
    if (value->operation >= MAX_BACNET_LIGHTING_OPERATION) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return status;
    }
    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        debug_printf(
            "LO[%u]: Lighting-Command@%u: %s\n", object_instance, priority,
            bactext_lighting_operation_name(value->operation));
        switch (value->operation) {
            case BACNET_LIGHTS_NONE:
                status = true;
                break;
            case BACNET_LIGHTS_FADE_TO:
                debug_printf(
                    "LO[%u]: Lighting-Command@%u Fade-To "
                    "Target=%f Fade=%u\n",
                    object_instance, priority, (double)value->target_level,
                    value->fade_time);
                Lighting_Command_Fade_To(
                    pObject, priority, value->target_level, value->fade_time);
                status = true;
                break;
            case BACNET_LIGHTS_RAMP_TO:
                debug_printf(
                    "LO[%u]: Lighting-Command@%u Ramp-To "
                    "Target=%f Ramp-Rate=%f\n",
                    object_instance, priority, (double)value->target_level,
                    (double)value->ramp_rate);
                Lighting_Command_Ramp_To(
                    pObject, priority, value->target_level, value->ramp_rate);
                status = true;
                break;
            case BACNET_LIGHTS_STEP_UP:
            case BACNET_LIGHTS_STEP_DOWN:
            case BACNET_LIGHTS_STEP_ON:
            case BACNET_LIGHTS_STEP_OFF:
                debug_printf(
                    "LO[%u]: Lighting-Command@%u Step "
                    "Step-Increment=%f\n",
                    object_instance, priority, (double)value->step_increment);
                Lighting_Command_Step(
                    pObject, priority, value->operation, value->step_increment);
                status = true;
                break;
            case BACNET_LIGHTS_WARN:
                /* Provides the same functionality as the
                   WARN lighting command. */
                Lighting_Command_Warn(pObject, priority);
                status = true;
                break;
            case BACNET_LIGHTS_WARN_OFF:
                /* Provides the same functionality as the
                   WARN_OFF lighting command. */
                Lighting_Command_Warn_Off(pObject, priority);
                status = true;
                break;
            case BACNET_LIGHTS_WARN_RELINQUISH:
                /* Provides the same functionality as the
                   WARN_RELINQUISH lighting command. */
                Lighting_Command_Warn_Relinquish(pObject, priority);
                status = true;
                break;
            case BACNET_LIGHTS_STOP:
                Lighting_Command_Stop(pObject, priority);
                status = true;
                break;
            default:
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                break;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * For a given object instance-number, gets the lighting-command.
 *
 * @note The Lighting_Command property shall indicate the last written
 *  value or NONE if it has not yet been written.
 *
 * @param object_instance - object-instance number of the object
 * @param value - holds the lighting command value
 *
 * @return true if lighting command was retrieved
 */
bool Lighting_Output_Lighting_Command(
    uint32_t object_instance, BACNET_LIGHTING_COMMAND *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = lighting_command_copy(value, &pObject->Last_Lighting_Command);
    }

    return status;
}

/**
 * For a given object instance-number, sets the lighting-command.
 *
 * @param object_instance - object-instance number of the object
 * @param value - holds the lighting command value
 *
 * @return  true if lighting command was set
 */
bool Lighting_Output_Lighting_Command_Set(
    uint32_t object_instance, const BACNET_LIGHTING_COMMAND *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        switch (value->operation) {
            case BACNET_LIGHTS_NONE:
                status = true;
                break;
            case BACNET_LIGHTS_FADE_TO:
                Lighting_Command_Fade_To(
                    pObject, value->priority, value->target_level,
                    value->fade_time);
                status = true;
                break;
            case BACNET_LIGHTS_RAMP_TO:
                Lighting_Command_Ramp_To(
                    pObject, value->priority, value->target_level,
                    value->ramp_rate);
                status = true;
                break;
            case BACNET_LIGHTS_STEP_UP:
            case BACNET_LIGHTS_STEP_DOWN:
            case BACNET_LIGHTS_STEP_ON:
            case BACNET_LIGHTS_STEP_OFF:
                Lighting_Command_Step(
                    pObject, value->priority, value->operation,
                    value->step_increment);
                status = true;
                break;
            case BACNET_LIGHTS_WARN:
                /* Provides the same functionality as the
                   WARN lighting command. */
                Lighting_Command_Warn(pObject, value->priority);
                status = true;
                break;
            case BACNET_LIGHTS_WARN_OFF:
                /* Provides the same functionality as the
                   WARN_OFF lighting command. */
                Lighting_Command_Warn_Off(pObject, value->priority);
                status = true;
                break;
            case BACNET_LIGHTS_WARN_RELINQUISH:
                /* Provides the same functionality as the
                   WARN_RELINQUISH lighting command. */
                Lighting_Command_Warn_Relinquish(pObject, value->priority);
                status = true;
                break;
            case BACNET_LIGHTS_STOP:
                Lighting_Command_Stop(pObject, value->priority);
                status = true;
                break;
            default:
                break;
        }
        if (status) {
            lighting_command_copy(&pObject->Last_Lighting_Command, value);
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the in-progress property value
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the in-progress value of this object instance.
 */
BACNET_LIGHTING_IN_PROGRESS
Lighting_Output_In_Progress(uint32_t object_instance)
{
    BACNET_LIGHTING_IN_PROGRESS value = BACNET_LIGHTING_IDLE;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Lighting_Command.In_Progress;
    }

    return value;
}

/**
 * For a given object instance-number, sets the in-progress value of the
 * object.
 *
 * @param object_instance - object-instance number of the object
 * @param in_progress - holds the value to be set
 *
 * @return true if value was set
 */
bool Lighting_Output_In_Progress_Set(
    uint32_t object_instance, BACNET_LIGHTING_IN_PROGRESS in_progress)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Lighting_Command.In_Progress = in_progress;
    }

    return status;
}

/**
 * For a given object instance-number, gets the tracking-value property
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the tracking-value of this object instance.
 */
float Lighting_Output_Tracking_Value(uint32_t object_instance)
{
    float value = 0.0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Lighting_Command.Tracking_Value;
    }

    return value;
}

/**
 * For a given object instance-number, sets the in-progress value of the
 * object.
 *
 * @param object_instance - object-instance number of the object
 * @param in_progress - holds the value to be set
 *
 * @return true if value was set
 */
bool Lighting_Output_Tracking_Value_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Lighting_Command.Tracking_Value = value;
        status = true;
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
bool Lighting_Output_Blink_Warn_Enable(uint32_t object_instance)
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
bool Lighting_Output_Blink_Warn_Enable_Set(
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
uint32_t Lighting_Output_Egress_Time(uint32_t object_instance)
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
bool Lighting_Output_Egress_Time_Set(uint32_t object_instance, uint32_t seconds)
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
bool Lighting_Output_Egress_Active(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Egress_Active;
    }

    return value;
}

/**
 * For a given object instance-number, gets the fade-time
 * property value
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the fade-time property value of this object
 */
uint32_t Lighting_Output_Default_Fade_Time(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Default_Fade_Time;
    }

    return value;
}

/**
 * For a given object instance-number, sets the fade-time
 * property value of the object.
 *
 * @param object_instance - object-instance number of the object
 * @param milliseconds - holds the value to be set
 *
 * @return true if value was set
 */
bool Lighting_Output_Default_Fade_Time_Set(
    uint32_t object_instance, uint32_t milliseconds)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((milliseconds >= 100) && (milliseconds <= 86400000)) {
            pObject->Default_Fade_Time = milliseconds;
            status = true;
        }
    }

    return status;
}

/**
 * Handle a WriteProperty to a specific property.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Lighting_Output_Default_Fade_Time_Write(
    uint32_t object_instance,
    uint32_t value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if ((value >= 100) && (value <= 86400000)) {
            pObject->Default_Fade_Time = value;
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
    } else {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
    }

    return status;
}

/**
 * For a given object instance-number, gets the ramp-rate
 * property value
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the ramp-rate property value of this object
 */
float Lighting_Output_Default_Ramp_Rate(uint32_t object_instance)
{
    float value = 0.0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Default_Ramp_Rate;
    }

    return value;
}

/**
 * For a given object instance-number, sets the ramp-rate value of the
 * object.
 *
 * @param object_instance - object-instance number of the object
 * @param percent_per_second - holds the value to be set
 *
 * @return true if value was set
 */
bool Lighting_Output_Default_Ramp_Rate_Set(
    uint32_t object_instance, float percent_per_second)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (isgreaterequal(percent_per_second, 0.1f) &&
            islessequal(percent_per_second, 100.0f)) {
            pObject->Default_Ramp_Rate = percent_per_second;
            status = true;
        }
    }

    return status;
}

/**
 * Handle a WriteProperty to a specific property.
 *
 * @param  object_instance - object-instance number of the object
 * @param  percent_per_second - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Lighting_Output_Default_Ramp_Rate_Write(
    uint32_t object_instance,
    float percent_per_second,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if (isgreaterequal(percent_per_second, 0.1f) &&
            islessequal(percent_per_second, 100.0f)) {
            pObject->Default_Ramp_Rate = percent_per_second;
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
    } else {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
    }

    return status;
}

/**
 * For a given object instance-number, gets the default-step-increment
 * property value
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the default-step-increment property value of this object
 */
float Lighting_Output_Default_Step_Increment(uint32_t object_instance)
{
    float value = 0.0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Default_Step_Increment;
    }

    return value;
}

/**
 * For a given object instance-number, sets the default-step-increment
 * property value of the object.
 *
 * @param object_instance - object-instance number of the object
 * @param step_increment - holds the value to be set
 *
 * @return true if value was set
 */
bool Lighting_Output_Default_Step_Increment_Set(
    uint32_t object_instance, float step_increment)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (isgreaterequal(step_increment, 0.1f) &&
            islessequal(step_increment, 100.0f)) {
            pObject->Default_Step_Increment = step_increment;
            status = true;
        }
    }

    return status;
}

/**
 * Handle a WriteProperty to a specific property.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Lighting_Output_Default_Step_Increment_Write(
    uint32_t object_instance,
    float value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if (isgreaterequal(value, 0.1f) && islessequal(value, 100.0f)) {
            pObject->Default_Step_Increment = value;
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
    } else {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
    }

    return status;
}

/**
 * For a given object instance-number, gets the
 * lighting-command-default-priority
 * property value
 *
 * @param object_instance - object-instance number of the object
 *
 * @return the lighting-command-default-priority property value of
 * this object
 */
unsigned Lighting_Output_Default_Priority(uint32_t object_instance)
{
    unsigned value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Lighting_Command_Default_Priority;
    }

    return value;
}

/**
 * For a given object instance-number, sets the
 * lighting-command-default-priority property value of the object.
 *
 * @param object_instance - object-instance number of the object
 * @param priority - holds the value to be set
 *
 * @return true if value was set
 */
bool Lighting_Output_Default_Priority_Set(
    uint32_t object_instance, unsigned priority)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((priority >= BACNET_MIN_PRIORITY) &&
            (priority <= BACNET_MAX_PRIORITY)) {
            pObject->Lighting_Command_Default_Priority = priority;
            status = true;
        }
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
bool Lighting_Output_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Lighting_Command.Out_Of_Service;
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
void Lighting_Output_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Lighting_Command.Out_Of_Service = value;
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
float Lighting_Output_Relinquish_Default(uint32_t object_instance)
{
    float value = 0.0;
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
bool Lighting_Output_Relinquish_Default_Set(
    uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Relinquish_Default = value;
    }

    return status;
}

/**
 * For a given object instance-number, gets the property value
 *
 * @param object_instance - object-instance number of the object
 * @return property value
 */
BACNET_LIGHTING_TRANSITION Lighting_Output_Transition(uint32_t object_instance)
{
    BACNET_LIGHTING_TRANSITION value = BACNET_LIGHTING_TRANSITION_NONE;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Transition;
    }

    return value;
}

/**
 * For a given object instance-number, sets the property value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACNET_COLOR_TRANSITION
 * @return  true if values are within range and value is set.
 */
bool Lighting_Output_Transition_Set(
    uint32_t object_instance, BACNET_LIGHTING_TRANSITION value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= BACNET_LIGHTING_TRANSITION_PROPRIETARY_LAST) {
            pObject->Transition = value;
            status = true;
        }
    }

    return status;
}

/**
 * Handle a WriteProperty to a specific property.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Lighting_Output_Transition_Write(
    uint32_t object_instance,
    uint32_t value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if (value < BACNET_LIGHTING_TRANSITION_PROPRIETARY_LAST) {
            pObject->Transition = value;
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
 * For a given object instance-number, returns the color-override
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return color-override property value
 */
bool Lighting_Output_Color_Override(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Color_Override;
    }

    return value;
}

/**
 * For a given object instance-number, sets the color-override
 * property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - color-override boolean value
 *
 * @return true if the color-override property value was set
 */
bool Lighting_Output_Color_Override_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Color_Override = value;
    }

    return status;
}

/**
 * For a given object instance-number, gets the property value
 *
 * This property, of type BACnetObjectIdentifier, when present,
 * shall specify the object identifier of a Color or Color Temperature
 * object within the same device that controls the color aspects
 * of this Lighting Output. If the object instance portion of
 * the object identifier has the value 4194303, then there is no color
 * companion object associated with this output. In that case the
 * applicable color or color temperature shall be a local matter.
 *
 * @param object_instance - object-instance number of the object
 * @param value - holds the property value
 *
 * @return true if property was retrieved
 */
bool Lighting_Output_Color_Reference(
    uint32_t object_instance, BACNET_OBJECT_ID *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value) {
            value->type = pObject->Color_Reference.type;
            value->instance = pObject->Color_Reference.instance;
        }
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, sets the property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 *
 * @return  true if property value was set
 */
bool Lighting_Output_Color_Reference_Set(
    uint32_t object_instance, const BACNET_OBJECT_ID *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Color_Reference.type = value->type;
        pObject->Color_Reference.instance = value->instance;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, gets the property value
 *
 * This property, of type BACnetObjectIdentifier, when present, shall
 * specify the object identifier of a Color or Color Temperature
 * object within the same device that controls the color override
 * aspects of this Lighting Output. Color override occurs
 * when the Color_Override property of the Lighting Output is
 * written with TRUE. In this case the Override_Color_Reference points
 * to an object whose color shall be used to control the actual color
 * of the lighting output. While color-overridden, any fade that may
 * be in progress for the Color_Reference object, shall continue without
 * interruption, except that the actual color output shall use the
 * override color instead. See Clause 12.55 for a description of
 * color override. Color override shall cease when Color_Override
 * is written with FALSE.
 *
 * @param object_instance - object-instance number of the object
 * @param value - holds the property value
 *
 * @return true if property was retrieved
 */
bool Lighting_Output_Override_Color_Reference(
    uint32_t object_instance, BACNET_OBJECT_ID *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value) {
            value->type = pObject->Override_Color_Reference.type;
            value->instance = pObject->Override_Color_Reference.instance;
        }
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, sets the property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 *
 * @return  true if property value was set
 */
bool Lighting_Output_Override_Color_Reference_Set(
    uint32_t object_instance, const BACNET_OBJECT_ID *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Override_Color_Reference.type = value->type;
        pObject->Override_Color_Reference.instance = value->instance;
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
int Lighting_Output_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    int apdu_size = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_LIGHTING_COMMAND lighting_command;
#if (BACNET_PROTOCOL_REVISION >= 24)
    BACNET_OBJECT_ID object_id = { 0 };
#endif
    float real_value = (float)1.414;
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
                &apdu[0], OBJECT_LIGHTING_OUTPUT, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Lighting_Output_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_LIGHTING_OUTPUT);
            break;
        case PROP_PRESENT_VALUE:
            real_value = Lighting_Output_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_TRACKING_VALUE:
            real_value =
                Lighting_Output_Tracking_Value(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_LIGHTING_COMMAND:
            Lighting_Output_Lighting_Command(
                rpdata->object_instance, &lighting_command);
            apdu_len = lighting_command_encode(&apdu[0], &lighting_command);
            break;
        case PROP_IN_PROGRESS:
            unsigned_value =
                Lighting_Output_In_Progress(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], unsigned_value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Lighting_Output_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Lighting_Output_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_BLINK_WARN_ENABLE:
            state = Lighting_Output_Blink_Warn_Enable(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_EGRESS_TIME:
            unsigned_value =
                Lighting_Output_Egress_Time(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_EGRESS_ACTIVE:
            state = Lighting_Output_Egress_Active(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_DEFAULT_FADE_TIME:
            unsigned_value =
                Lighting_Output_Default_Fade_Time(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_DEFAULT_RAMP_RATE:
            real_value =
                Lighting_Output_Default_Ramp_Rate(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_DEFAULT_STEP_INCREMENT:
            real_value =
                Lighting_Output_Default_Step_Increment(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_TRANSITION:
            apdu_len = encode_application_enumerated(
                apdu, Lighting_Output_Transition(rpdata->object_instance));
            break;
        case PROP_PRIORITY_ARRAY:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Lighting_Output_Priority_Array_Encode, BACNET_MAX_PRIORITY,
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
            real_value =
                Lighting_Output_Relinquish_Default(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_LIGHTING_COMMAND_DEFAULT_PRIORITY:
            unsigned_value =
                Lighting_Output_Default_Priority(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
#if (BACNET_PROTOCOL_REVISION >= 17)
        case PROP_CURRENT_COMMAND_PRIORITY:
            i = Lighting_Output_Present_Value_Priority(rpdata->object_instance);
            if ((i >= BACNET_MIN_PRIORITY) && (i <= BACNET_MAX_PRIORITY)) {
                apdu_len = encode_application_unsigned(&apdu[0], i);
            } else {
                apdu_len = encode_application_null(&apdu[0]);
            }
            break;
#endif
#if (BACNET_PROTOCOL_REVISION >= 24)
        case PROP_COLOR_OVERRIDE:
            apdu_len = encode_application_boolean(
                &apdu[0],
                Lighting_Output_Color_Override(rpdata->object_instance));
            break;
        case PROP_COLOR_REFERENCE:
            (void)Lighting_Output_Color_Reference(
                rpdata->object_instance, &object_id);
            apdu_len = encode_application_object_id(
                &apdu[0], object_id.type, object_id.instance);
            break;
        case PROP_OVERRIDE_COLOR_REFERENCE:
            (void)Lighting_Output_Override_Color_Reference(
                rpdata->object_instance, &object_id);
            apdu_len = encode_application_object_id(
                &apdu[0], object_id.type, object_id.instance);
            break;
#endif
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Lighting_Output_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
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
bool Lighting_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    /* decode the some of the request */
    len = bacapp_decode_known_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        wp_data->object_type, wp_data->object_property);
    /* note: len < application_data_len means there is more data,
       such as an array or list */
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
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Lighting_Output_Present_Value_Write(
                    wp_data->object_instance, value.type.Real,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            } else {
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_NULL);
                if (status) {
                    status = Lighting_Output_Present_Value_Relinquish_Write(
                        wp_data->object_instance, wp_data->priority,
                        &wp_data->error_class, &wp_data->error_code);
                }
            }
            break;
        case PROP_LIGHTING_COMMAND:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_LIGHTING_COMMAND);
            if (status) {
                status = Lighting_Output_Lighting_Command_Write(
                    wp_data->object_instance, &value.type.Lighting_Command,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Lighting_Output_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_DEFAULT_FADE_TIME:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Lighting_Output_Default_Fade_Time_Write(
                    wp_data->object_instance, value.type.Unsigned_Int,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            }
            break;
        case PROP_DEFAULT_RAMP_RATE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Lighting_Output_Default_Ramp_Rate_Write(
                    wp_data->object_instance, value.type.Real,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            }
            break;
        case PROP_DEFAULT_STEP_INCREMENT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Lighting_Output_Default_Step_Increment_Write(
                    wp_data->object_instance, value.type.Real,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            }
            break;
        case PROP_TRANSITION:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                status = Lighting_Output_Transition_Write(
                    wp_data->object_instance, value.type.Enumerated,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
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
#if (BACNET_PROTOCOL_REVISION >= 24)
        case PROP_COLOR_OVERRIDE:
        case PROP_COLOR_REFERENCE:
        case PROP_OVERRIDE_COLOR_REFERENCE:
#endif
        case PROP_DESCRIPTION:
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
 * @brief Updates the lighting object tracking value per ramp or fade or step
 * @param  object_instance - object-instance number of the object
 * @param milliseconds - number of milliseconds elapsed since previously
 * called.  Suggest that this is called every 10 milliseconds.
 */
void Lighting_Output_Timer(uint32_t object_instance, uint16_t milliseconds)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        lighting_command_timer(&pObject->Lighting_Command, milliseconds);
    }
}

static void Lighting_Output_Tracking_Value_Callback(
    uint32_t object_instance, float old_value, float value)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (Lighting_Command_Tracking_Value_Callback) {
            Lighting_Command_Tracking_Value_Callback(
                object_instance, old_value, value);
        }
    }
}

/**
 * @brief Sets a callback used when present-value is written from BACnet
 * @param cb - callback used to provide indications
 */
void Lighting_Output_Write_Present_Value_Callback_Set(
    lighting_command_tracking_value_callback cb)
{
    Lighting_Command_Tracking_Value_Callback = cb;
}

/**
 * @brief Creates a Color object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Lighting_Output_Create(uint32_t object_instance)
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
        pObject->Present_Value = 0.0f;
        pObject->Physical_Value = 0.0f;
        lighting_command_init(&pObject->Lighting_Command);
        pObject->Lighting_Command.Key = object_instance;
        pObject->Lighting_Command.Notification_Head.callback =
            Lighting_Output_Tracking_Value_Callback;
        pObject->Last_Lighting_Command.operation = BACNET_LIGHTS_NONE;
        pObject->Last_Lighting_Command.use_target_level = false;
        pObject->Last_Lighting_Command.use_ramp_rate = false;
        pObject->Last_Lighting_Command.use_step_increment = false;
        pObject->Last_Lighting_Command.use_fade_time = false;
        pObject->Last_Lighting_Command.use_priority = false;
        pObject->Blink_Warn_Enable = false;
        pObject->Egress_Active = false;
        pObject->Egress_Time = 0;
        pObject->Default_Fade_Time = 100;
        pObject->Default_Ramp_Rate = 100.0;
        pObject->Default_Step_Increment = 1.0f;
        pObject->Transition = BACNET_LIGHTING_TRANSITION_FADE;
        pObject->Feedback_Value = 0.0;
        for (p = 0; p < BACNET_MAX_PRIORITY; p++) {
            pObject->Priority_Array[p] = 0.0f;
            BIT_CLEAR(pObject->Priority_Active_Bits, p);
        }
        pObject->Relinquish_Default = 0.0f;
        pObject->Power = 0.0f;
        pObject->Instantaneous_Power = 0.0f;
        pObject->Lighting_Command_Default_Priority = 16;
        pObject->Color_Override = false;
        pObject->Color_Reference.type = OBJECT_COLOR;
        pObject->Color_Reference.instance = BACNET_MAX_INSTANCE;
        pObject->Override_Color_Reference.type = OBJECT_COLOR;
        pObject->Override_Color_Reference.instance = BACNET_MAX_INSTANCE;
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
bool Lighting_Output_Delete(uint32_t object_instance)
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
void Lighting_Output_Cleanup(void)
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
void Lighting_Output_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
