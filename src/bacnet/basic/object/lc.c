/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @brief The Load Control Objects from 135-2004-Addendum e
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
#include "bacnet/bactext.h"
#include "bacnet/datetime.h"
#include "bacnet/shed_level.h"
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/keylist.h"

/* from Table 12-33. Requested_Shed_Level Default Values and Power Targets */
#define DEFAULT_VALUE_PERCENT 100
#define DEFAULT_VALUE_LEVEL 0
#define DEFAULT_VALUE_AMOUNT 0.0f
/* note: load control objects are required to support LEVEL */

/* minimum interval the load control state machine should process */
#ifndef LOAD_CONTROL_TASK_INTERVAL_MS
#define LOAD_CONTROL_TASK_INTERVAL_MS 1000UL
#endif

struct object_data {
    /* indicates the current load shedding state of the object */
    BACNET_SHED_STATE Present_Value;
    /* tracking for the Load Control finite state machine */
    BACNET_SHED_STATE Previous_Value;
    /* indicates the desired load shedding */
    BACNET_SHED_LEVEL Requested_Shed_Level;
    /* Indicates the amount of power that the object expects
    to be able to shed in response to a load shed request. */
    BACNET_SHED_LEVEL Expected_Shed_Level;
    /* Indicates the actual amount of power being shed in response
    to a load shed request. */
    BACNET_SHED_LEVEL Actual_Shed_Level;
    /* indicates the start of the duty window in which the load controlled
    by the Load Control object must be compliant with the requested shed. */
    BACNET_DATE_TIME Start_Time;
    BACNET_DATE_TIME End_Time;
    /* indicates the duration of the load shed action,
    starting at Start_Time in minutes */
    uint32_t Shed_Duration;
    /* indicates the time window used for load shed accounting in minutes */
    uint32_t Duty_Window;
    /* indicates and controls whether the Load Control object is
    currently enabled to respond to load shed requests.  */
    bool Load_Control_Enable : 1;
    /* indicates when the object receives a write to any of the properties
    Requested_Shed_Level, Shed_Duration, Duty_Window */
    bool Load_Control_Request_Written : 1;
    /* indicates when the object receives a write to Start_Time */
    bool Start_Time_Property_Written : 1;
    /* optional: indicates the baseline power consumption value
    for the sheddable load controlled by this object,
    if a fixed baseline is used.
    The units of Full_Duty_Baseline are kilowatts.*/
    float Full_Duty_Baseline;
    /* The elements of the Shed Level array are required to be writable,
    allowing local configuration of how this Load Control
    object will participate in load shedding for the
    facility. This array is not required to be resizable
    through BACnet write services. The size of this array
    shall be equal to the size of the Shed_Level_Descriptions
    array. The behavior of this object when the Shed_Levels
    array contains duplicate entries is a local matter. */
    OS_Keylist Shed_Level_List;
    /* the load control manipulates and references
       another object present-value in this device */
    BACNET_OBJECT_TYPE Manipulated_Object_Type;
    uint32_t Manipulated_Object_Instance;
    BACNET_PROPERTY_ID Manipulated_Object_Property;
    uint8_t Priority_For_Writing;
    load_control_manipulated_object_write_callback Manipulated_Object_Write;
    load_control_manipulated_object_relinquish_callback
        Manipulated_Object_Relinquish;
    load_control_manipulated_object_read_callback Manipulated_Object_Read;
    /* state machine task time tracking per object */
    uint32_t Update_Interval;
    uint32_t Task_Milliseconds;
    void *Context;
    const char *Object_Name;
    const char *Description;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;

/* clang-format off */
/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Load_Control_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_REQUESTED_SHED_LEVEL,
    PROP_START_TIME,
    PROP_SHED_DURATION,
    PROP_DUTY_WINDOW,
    PROP_ENABLE,
    PROP_EXPECTED_SHED_LEVEL,
    PROP_ACTUAL_SHED_LEVEL,
    PROP_SHED_LEVELS,
    PROP_SHED_LEVEL_DESCRIPTIONS,
    -1
};

static const int32_t Load_Control_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_FULL_DUTY_BASELINE,
    -1
};

static const int32_t Load_Control_Properties_Proprietary[] = {
    -1
};
/* clang-format on */

void Load_Control_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
{
    if (pRequired) {
        *pRequired = Load_Control_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Load_Control_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Load_Control_Properties_Proprietary;
    }

    return;
}

/**
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct object_data *Object_Instance_Data(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Determines if a given object instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Load_Control_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of object instances
 * @return Number of object instances
 */
unsigned Load_Control_Count(void)
{
    return Keylist_Count(Object_List);
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Load_Control_Index_To_Instance(unsigned index)
{
    uint32_t instance = UINT32_MAX;

    (void)Keylist_Index_Key(Object_List, index, &instance);

    return instance;
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * of objects where N is the number of objects.
 * @param  index - 0..N where N is the number of objects
 * @return  object instance-number for the given index
 */
unsigned Load_Control_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief For a given object instance-number, read the present-value.
 * @param  object_instance - object-instance number of the object
 * @param  value - Pointer to the new value
 * @return  true if value is within range and copied
 */
BACNET_SHED_STATE Load_Control_Present_Value(uint32_t object_instance)
{
    BACNET_SHED_STATE value = BACNET_SHED_INACTIVE;
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        value = pObject->Present_Value;
    }

    return value;
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
bool Load_Control_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[32] = "LOAD_CONTROL-4194303";

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "LOAD_CONTROL-%lu",
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
bool Load_Control_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
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
const char *Load_Control_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * @brief convert the shed level request into a percentage of full duty
 * baseline power
 * @param pObject - object instance to get
 * @return the requested shed level as a percentage of full duty baseline
 */
static float Requested_Shed_Level_Value(struct object_data *pObject)
{
    float requested_level = 0.0f;
    struct shed_level_data *shed_level;
    unsigned shed_level_max = 0, i;
    KEY key;

    switch (pObject->Requested_Shed_Level.type) {
        case BACNET_SHED_TYPE_PERCENT:
            /* (current baseline) * Requested_Shed_Level / 100 */
            requested_level =
                (float)pObject->Requested_Shed_Level.value.percent;
            break;
        case BACNET_SHED_TYPE_AMOUNT:
            /* (current baseline) - Requested_Shed_Level */
            requested_level = pObject->Full_Duty_Baseline -
                pObject->Requested_Shed_Level.value.amount;
            requested_level /= pObject->Full_Duty_Baseline;
            requested_level *= 100.0f;
            break;
        case BACNET_SHED_TYPE_LEVEL:
        default:
            shed_level = Keylist_Data(
                pObject->Shed_Level_List,
                pObject->Requested_Shed_Level.value.level);
            if (shed_level) {
                requested_level = shed_level->Value;
            } else {
                /* If the Load Control object is commanded to go to a level
                    that is not in the Shed_Levels array, it shall go to the
                    Shed_Level whose entry in the Shed_Levels array has the
                    nearest numerically lower value.*/
                /* get the numerically lowest */
                shed_level = Keylist_Data_Index(pObject->Shed_Level_List, 0);
                /* find the nearest */
                shed_level_max = Keylist_Count(pObject->Shed_Level_List);
                for (i = 0; i < shed_level_max; i++) {
                    if (Keylist_Index_Key(pObject->Shed_Level_List, i, &key)) {
                        if (key <= pObject->Requested_Shed_Level.value.level) {
                            shed_level =
                                Keylist_Data_Index(pObject->Shed_Level_List, i);
                        }
                    }
                }
                if (shed_level) {
                    requested_level = shed_level->Value;
                } else {
                    /* no level found so use 100% of baseline (no shed) */
                    requested_level = 100.0f;
                }
            }
            break;
    }

    return requested_level;
}

/**
 * @brief Copy the Shed Level data from source to destination
 * @param dest - destination data
 * @param src - source data
 */
static void
Shed_Level_Copy(BACNET_SHED_LEVEL *dest, const BACNET_SHED_LEVEL *src)
{
    if (dest && src) {
        dest->type = src->type;
        switch (src->type) {
            case BACNET_SHED_TYPE_PERCENT:
                dest->value.percent = src->value.percent;
                break;
            case BACNET_SHED_TYPE_AMOUNT:
                dest->value.amount = src->value.amount;
                break;
            case BACNET_SHED_TYPE_LEVEL:
            default:
                dest->value.level = src->value.level;
                break;
        }
    }
}

/**
 * @brief Set the Shed Level data to the default value
 * @param dest - destination data
 * @param type - type of shed level
 */
static void
Shed_Level_Default_Set(BACNET_SHED_LEVEL *dest, BACNET_SHED_LEVEL_TYPE type)
{
    if (dest) {
        dest->type = type;
        switch (type) {
            case BACNET_SHED_TYPE_PERCENT:
                dest->value.percent = 100;
                break;
            case BACNET_SHED_TYPE_AMOUNT:
                dest->value.amount = 0.0f;
                break;
            case BACNET_SHED_TYPE_LEVEL:
            default:
                dest->value.level = 0;
                break;
        }
    }
}

/**
 * @brief Determine if the object can immediately meet the shed request
 * @param pObject - object instance to get
 * @return true if the object can meet the shed request
 */
static bool Able_To_Meet_Shed_Request(struct object_data *pObject)
{
    float level = 0.0f;
    float requested_level = 0.0f;
    uint8_t priority = 0;
    bool status = false;

    if (pObject->Manipulated_Object_Read) {
        pObject->Manipulated_Object_Read(
            pObject->Manipulated_Object_Type,
            pObject->Manipulated_Object_Instance,
            pObject->Manipulated_Object_Property, &priority, &level);
        requested_level = Requested_Shed_Level_Value(pObject);
        if (level < requested_level) {
            status = true;
        }
    }

    return status;
}

/**
 * @brief Determine if the object can now comply with the shed request
 * @param pObject - object instance to get
 * @return true if the object can comply with the shed request
 */
static bool Can_Now_Comply_With_Shed(struct object_data *pObject)
{
    float level = 0.0f;
    float requested_level = 0.0f;
    uint8_t priority = 0;
    bool status = false;

    if (pObject->Manipulated_Object_Read) {
        pObject->Manipulated_Object_Read(
            pObject->Manipulated_Object_Type,
            pObject->Manipulated_Object_Instance,
            pObject->Manipulated_Object_Property, &priority, &level);
        requested_level = Requested_Shed_Level_Value(pObject);
        if (level <= requested_level) {
            status = true;
        }
    }
    if (!status) {
        /* the object attempts to meet the shed request
           until the shed is achieved. */
        if (pObject->Manipulated_Object_Write) {
            pObject->Manipulated_Object_Write(
                pObject->Manipulated_Object_Type,
                pObject->Manipulated_Object_Instance,
                pObject->Manipulated_Object_Property,
                pObject->Priority_For_Writing,
                Requested_Shed_Level_Value(pObject));
        }
    }

    return status;
}

/**
 * @brief Load Control State Machine
 * @param object_index - object index in the list
 * @param bdatetime - current date and time
 */
void Load_Control_State_Machine(
    int object_index, const BACNET_DATE_TIME *bdatetime)
{
    int diff = 0; /* used for datetime comparison */
    float amount;
    unsigned percent;
    unsigned level;
    struct object_data *pObject;

    pObject = Keylist_Data_Index(Object_List, object_index);
    if (!pObject) {
        return;
    }
    /* is the state machine enabled? */
    if (!pObject->Load_Control_Enable) {
        pObject->Present_Value = BACNET_SHED_INACTIVE;
        return;
    }

    switch (pObject->Present_Value) {
        case BACNET_SHED_REQUEST_PENDING:
            if (pObject->Load_Control_Request_Written) {
                pObject->Load_Control_Request_Written = false;
                /* request to cancel using default values? */
                switch (pObject->Requested_Shed_Level.type) {
                    case BACNET_SHED_TYPE_PERCENT:
                        percent = pObject->Requested_Shed_Level.value.percent;
                        if (percent == DEFAULT_VALUE_PERCENT) {
                            pObject->Present_Value = BACNET_SHED_INACTIVE;
                        }
                        break;
                    case BACNET_SHED_TYPE_AMOUNT:
                        amount = pObject->Requested_Shed_Level.value.amount;
                        if (islessequal(amount, DEFAULT_VALUE_AMOUNT)) {
                            pObject->Present_Value = BACNET_SHED_INACTIVE;
                        }
                        break;
                    case BACNET_SHED_TYPE_LEVEL:
                    default:
                        level = pObject->Requested_Shed_Level.value.level;
                        if (level == DEFAULT_VALUE_LEVEL) {
                            pObject->Present_Value = BACNET_SHED_INACTIVE;
                        }
                        break;
                }
                if (pObject->Present_Value == BACNET_SHED_INACTIVE) {
                    debug_printf(
                        "Load Control[%d]:Requested Shed Level=Default\n",
                        object_index);
                    break;
                }
            }
            /* clear the flag for Start time if it is written */
            if (pObject->Start_Time_Property_Written) {
                pObject->Start_Time_Property_Written = false;
                /* request to cancel using wildcards in start time? */
                if (datetime_wildcard(&pObject->Start_Time)) {
                    pObject->Present_Value = BACNET_SHED_INACTIVE;
                    debug_printf(
                        "Load Control[%d]:Start Time=Wildcard\n", object_index);
                    break;
                }
            }
            /* cancel because current time is after start time + duration? */
            datetime_copy(&pObject->End_Time, &pObject->Start_Time);
            datetime_add_minutes(&pObject->End_Time, pObject->Shed_Duration);
            diff = datetime_compare(&pObject->End_Time, bdatetime);
            if (diff < 0) {
                /* CancelShed */
                /* FIXME: stop shedding! i.e. relinquish */
                debug_printf(
                    "Load Control[%d]:Current Time"
                    " is after Start Time + Duration\n",
                    object_index);
                pObject->Present_Value = BACNET_SHED_INACTIVE;
                break;
            }
            diff = datetime_compare(bdatetime, &pObject->Start_Time);
            if (diff < 0) {
                /* current time prior to start time */
                /* ReconfigurePending */
                Shed_Level_Copy(
                    &pObject->Expected_Shed_Level,
                    &pObject->Requested_Shed_Level);
                Shed_Level_Default_Set(
                    &pObject->Actual_Shed_Level,
                    pObject->Requested_Shed_Level.type);
            } else if (diff > 0) {
                /* current time after to start time */
                debug_printf(
                    "Load Control[%d]:Current Time"
                    " is after Start Time\n",
                    object_index);
                /* AbleToMeetShed */
                /* If the current time is after Start_Time and the object
                   is able to achieve the shed request immediately,
                   it shall shed its loads, calculate Expected_Shed_Level
                   and Actual_Shed_Level, and enter the SHED_COMPLIANT state. */
                if (Able_To_Meet_Shed_Request(pObject)) {
                    Shed_Level_Copy(
                        &pObject->Expected_Shed_Level,
                        &pObject->Requested_Shed_Level);
                    Shed_Level_Copy(
                        &pObject->Actual_Shed_Level,
                        &pObject->Requested_Shed_Level);
                    pObject->Present_Value = BACNET_SHED_COMPLIANT;
                } else {
                    /* CannotMeetShed */
                    /* If the current time is after Start_Time,
                       and the object is unable to meet the shed
                       request immediately, it shall begin shedding
                       its loads, calculate Expected_Shed_Level and
                       Actual_Shed_Level, and enter the SHED_NON_COMPLIANT
                       state. */
                    Shed_Level_Default_Set(
                        &pObject->Expected_Shed_Level,
                        pObject->Requested_Shed_Level.type);
                    Shed_Level_Default_Set(
                        &pObject->Actual_Shed_Level,
                        pObject->Requested_Shed_Level.type);
                    pObject->Present_Value = BACNET_SHED_NON_COMPLIANT;
                }
            }
            break;
        case BACNET_SHED_NON_COMPLIANT:
            /* In the SHED_NON_COMPLIANT state, the object attempts
               to meet the shed request until the shed is achieved,
               the object is reconfigured, or the request has completed
               unsuccessfully. */
            datetime_copy(&pObject->End_Time, &pObject->Start_Time);
            datetime_add_minutes(&pObject->End_Time, pObject->Shed_Duration);
            diff = datetime_compare(&pObject->End_Time, bdatetime);
            if (diff < 0) {
                /* FinishedUnsuccessfulShed */
                debug_printf(
                    "Load Control[%d]:Current Time is after Start Time + "
                    "Duration\n",
                    object_index);
                pObject->Present_Value = BACNET_SHED_INACTIVE;
                break;
            }
            if (pObject->Load_Control_Request_Written ||
                pObject->Start_Time_Property_Written) {
                /* UnsuccessfulShedReconfigured */
                debug_printf(
                    "Load Control[%d]:Control Property written\n",
                    object_index);
                /* The Written flags will cleared in the next state */
                pObject->Present_Value = BACNET_SHED_REQUEST_PENDING;
                break;
            }
            if (Can_Now_Comply_With_Shed(pObject)) {
                /* CanNowComplyWithShed */
                debug_printf(
                    "Load Control[%d]:Able to meet Shed Request\n",
                    object_index);
                Shed_Level_Copy(
                    &pObject->Expected_Shed_Level,
                    &pObject->Requested_Shed_Level);
                Shed_Level_Copy(
                    &pObject->Actual_Shed_Level,
                    &pObject->Requested_Shed_Level);
                pObject->Present_Value = BACNET_SHED_COMPLIANT;
            }
            break;
        case BACNET_SHED_COMPLIANT:
            datetime_copy(&pObject->End_Time, &pObject->Start_Time);
            datetime_add_minutes(&pObject->End_Time, pObject->Shed_Duration);
            diff = datetime_compare(&pObject->End_Time, bdatetime);
            if (diff < 0) {
                /* FinishedSuccessfulShed */
                debug_printf(
                    "Load Control[%d]:Current Time is after Start Time + "
                    "Duration\n",
                    object_index);
                datetime_wildcard_set(&pObject->Start_Time);
                if (pObject->Manipulated_Object_Relinquish) {
                    pObject->Manipulated_Object_Relinquish(
                        pObject->Manipulated_Object_Type,
                        pObject->Manipulated_Object_Instance,
                        pObject->Manipulated_Object_Property,
                        pObject->Priority_For_Writing);
                }
                pObject->Present_Value = BACNET_SHED_INACTIVE;
                break;
            }
            if (pObject->Load_Control_Request_Written ||
                pObject->Start_Time_Property_Written) {
                /* UnsuccessfulShedReconfigured */
                debug_printf(
                    "Load Control[%d]:Control Property written\n",
                    object_index);
                /* The Written flags will cleared in the next state */
                pObject->Present_Value = BACNET_SHED_REQUEST_PENDING;
                break;
            }
            if (!Able_To_Meet_Shed_Request(pObject)) {
                /* CanNoLongerComplyWithShed */
                debug_printf(
                    "Load Control[%d]:Not able to meet Shed Request\n",
                    object_index);
                Shed_Level_Default_Set(
                    &pObject->Expected_Shed_Level,
                    pObject->Requested_Shed_Level.type);
                Shed_Level_Default_Set(
                    &pObject->Actual_Shed_Level,
                    pObject->Requested_Shed_Level.type);
                pObject->Present_Value = BACNET_SHED_NON_COMPLIANT;
            }
            break;
        case BACNET_SHED_INACTIVE:
        default:
            if (pObject->Start_Time_Property_Written) {
                debug_printf(
                    "Load Control[%d]:Start Time written\n", object_index);
                /* The Written flag will cleared in the next state */
                Shed_Level_Copy(
                    &pObject->Expected_Shed_Level,
                    &pObject->Requested_Shed_Level);
                Shed_Level_Default_Set(
                    &pObject->Actual_Shed_Level,
                    pObject->Requested_Shed_Level.type);
                pObject->Present_Value = BACNET_SHED_REQUEST_PENDING;
            }
            break;
    }

    return;
}

/**
 * @brief This property, of type Unsigned, indicates the interval
 *  in milliseconds at which the state machine updates the output.
 * @param object_instance [in] BACnet object instance number
 * @return the update-interval for a specific object instance
 */
uint32_t Load_Control_Update_Interval(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Update_Interval;
    }

    return value;
}

/**
 * @brief This property, of type Unsigned, sets the interval
 *  in milliseconds at which the state machine updates the output.
 * @param object_instance [in] BACnet object instance number
 * @param value [in] the update-interval for a specific object instance
 * @return true if the update-interval for a specific object instance was set
 */
bool Load_Control_Update_Interval_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Update_Interval = value;
        status = true;
    }

    return status;
}

/**
 * @brief Load Control State Machine Handler
 * @param object_instance - object-instance number of the object
 * @param milliseconds - elapsed time in milliseconds from last call
 */
void Load_Control_Timer(uint32_t object_instance, uint16_t milliseconds)
{
    BACNET_DATE_TIME bdatetime = { 0 };
    struct object_data *pObject;
    int index = 0;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        pObject->Task_Milliseconds += milliseconds;
        if (pObject->Task_Milliseconds >= pObject->Update_Interval) {
            pObject->Task_Milliseconds = 0;
            datetime_local(&bdatetime.date, &bdatetime.time, NULL, NULL);
            index = Keylist_Index(Object_List, object_instance);
            Load_Control_State_Machine(index, &bdatetime);
            if (pObject->Present_Value != pObject->Previous_Value) {
                debug_printf(
                    "Load Control[%d]=%s\n", index,
                    bactext_shed_state_name(pObject->Present_Value));
                pObject->Previous_Value = pObject->Present_Value;
            }
        }
    }
}

/**
 * @brief Load Control State Machine Handler
 * @note call every #LOAD_CONTROL_TASK_INTERVAL_MS milliseconds
 * @deprecated Use Load_Control_Timer() instead
 */
void Load_Control_State_Machine_Handler(void)
{
    unsigned count, index;
    uint32_t object_instance;

    count = Keylist_Count(Object_List);
    while (count) {
        count--;
        index = count;
        object_instance = Load_Control_Index_To_Instance(index);
        Load_Control_Timer(object_instance, LOAD_CONTROL_TASK_INTERVAL_MS);
    }
}

/**
 * @brief Get the priority for writing to the Manipulated Variable.
 * @param object_instance [in] The object instance number.
 */
unsigned Load_Control_Priority_For_Writing(uint32_t object_instance)
{
    unsigned priority = 0;
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        priority = pObject->Priority_For_Writing;
    }

    return priority;
}

/**
 * @brief Set the priority for writing to the Manipulated Variable.
 * @param object_instance [in] The object instance number.
 * @param priority [in] The priority for writing.
 * @return True if successful, else False.
 */
bool Load_Control_Priority_For_Writing_Set(
    uint32_t object_instance, unsigned priority)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        pObject->Priority_For_Writing = priority;
        status = true;
    }

    return status;
}

/**
 * @brief Get the Manipulated Variable Reference for the Load Control object.
 * @param object_instance [in] The object instance number.
 * @param object_property_reference [out] The object property reference.
 * @return True if successful, else False.
 */
bool Load_Control_Manipulated_Variable_Reference(
    uint32_t object_instance,
    BACNET_OBJECT_PROPERTY_REFERENCE *object_property_reference)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        if (object_property_reference) {
            object_property_reference->object_identifier.type =
                pObject->Manipulated_Object_Type;
            object_property_reference->object_identifier.instance =
                pObject->Manipulated_Object_Instance;
            object_property_reference->property_identifier =
                pObject->Manipulated_Object_Property;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Set the Manipulated Variable Reference for the Load Control object.
 * @param object_instance [in] The object instance number.
 * @param object_id [in] The object identifier.
 * @return True if successful, else False.
 */
bool Load_Control_Manipulated_Variable_Reference_Set(
    uint32_t object_instance,
    const BACNET_OBJECT_PROPERTY_REFERENCE *object_property_reference)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        if (object_property_reference) {
            pObject->Manipulated_Object_Type =
                object_property_reference->object_identifier.type;
            pObject->Manipulated_Object_Instance =
                object_property_reference->object_identifier.instance;
            pObject->Manipulated_Object_Property =
                object_property_reference->property_identifier;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet object instance number
 * @param index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Load_Control_Shed_Levels_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0, len = 0;
    struct shed_level_data *entry;
    BACNET_UNSIGNED_INTEGER unsigned_value;
    KEY key;
    int count;
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (!pObject) {
        return BACNET_STATUS_ERROR;
    }
    count = Keylist_Count(pObject->Shed_Level_List);
    if (index >= count) {
        return BACNET_STATUS_ERROR;
    }
    key = index + 1;
    entry = Keylist_Data(pObject->Shed_Level_List, key);
    if (entry) {
        unsigned_value = key;
        len = encode_application_unsigned(apdu, unsigned_value);
        apdu_len += len;
    } else {
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet object instance number
 * @param index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Load_Control_Shed_Level_Descriptions_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0, len = 0;
    struct shed_level_data *entry;
    BACNET_CHARACTER_STRING char_string;
    KEY key;
    int count;
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (!pObject) {
        return BACNET_STATUS_ERROR;
    }
    count = Keylist_Count(pObject->Shed_Level_List);
    if (index >= count) {
        return BACNET_STATUS_ERROR;
    }
    key = index + 1;
    entry = Keylist_Data(pObject->Shed_Level_List, key);
    if (entry) {
        characterstring_init_ansi(&char_string, entry->Description);
        len = encode_application_character_string(apdu, &char_string);
        apdu_len += len;
    } else {
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Encode the BACnetShedLevel property
 * @param apdu [out] Buffer in which the APDU contents are built
 * @param value [in] The value to encode
 * @return The length of the apdu encoded
 */
static int
BACnet_Shed_Level_Encode(uint8_t *apdu, const BACNET_SHED_LEVEL *value)
{
    int apdu_len = 0;

    if (!value) {
        return 0;
    }
    switch (value->type) {
        case BACNET_SHED_TYPE_PERCENT:
            apdu_len = encode_context_unsigned(apdu, 0, value->value.percent);
            break;
        case BACNET_SHED_TYPE_AMOUNT:
            apdu_len = encode_context_real(apdu, 2, value->value.amount);
            break;
        case BACNET_SHED_TYPE_LEVEL:
        default:
            apdu_len = encode_context_unsigned(apdu, 1, value->value.level);
            break;
    }

    return apdu_len;
}

/**
 * @brief ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int Load_Control_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    int enumeration = 0;
    unsigned count = 0;
    bool state = false;
    uint8_t *apdu = NULL;
    int apdu_size = 0;
    struct object_data *pObject;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    pObject = Object_Instance_Data(rpdata->object_instance);
    if (pObject == NULL) {
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return BACNET_STATUS_ERROR;
    }
    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_LOAD_CONTROL, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Load_Control_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_LOAD_CONTROL);
            break;
        case PROP_PRESENT_VALUE:
            enumeration = Load_Control_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enumeration);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            /* IN_ALARM - Logical FALSE (0) if the Event_State property
               has a value of NORMAL, otherwise logical TRUE (1). */
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            /* FAULT - Logical      TRUE (1) if the Reliability property is
               present and does not have a value of NO_FAULT_DETECTED,
               otherwise logical FALSE (0). */
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            /* OVERRIDDEN - Logical TRUE (1) if the point has been
               overridden by some mechanism local to the BACnet Device,
               otherwise logical FALSE (0). */
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            /* OUT_OF_SERVICE - This bit shall always be Logical FALSE (0). */
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_REQUESTED_SHED_LEVEL:
            apdu_len =
                BACnet_Shed_Level_Encode(apdu, &pObject->Requested_Shed_Level);
            break;
        case PROP_START_TIME:
            len = encode_application_date(&apdu[0], &pObject->Start_Time.date);
            apdu_len = len;
            len = encode_application_time(
                &apdu[apdu_len], &pObject->Start_Time.time);
            apdu_len += len;
            break;
        case PROP_SHED_DURATION:
            apdu_len =
                encode_application_unsigned(&apdu[0], pObject->Shed_Duration);
            break;
        case PROP_DUTY_WINDOW:
            apdu_len =
                encode_application_unsigned(&apdu[0], pObject->Duty_Window);
            break;
        case PROP_ENABLE:
            state = pObject->Load_Control_Enable;
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_FULL_DUTY_BASELINE: /* optional */
            apdu_len =
                encode_application_real(&apdu[0], pObject->Full_Duty_Baseline);
            break;
        case PROP_EXPECTED_SHED_LEVEL:
            apdu_len =
                BACnet_Shed_Level_Encode(apdu, &pObject->Expected_Shed_Level);
            break;
        case PROP_ACTUAL_SHED_LEVEL:
            apdu_len =
                BACnet_Shed_Level_Encode(apdu, &pObject->Actual_Shed_Level);
            break;
        case PROP_SHED_LEVELS:
            count = Keylist_Count(pObject->Shed_Level_List);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Load_Control_Shed_Levels_Encode, count, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_SHED_LEVEL_DESCRIPTIONS:
            count = Keylist_Count(pObject->Shed_Level_List);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Load_Control_Shed_Level_Descriptions_Encode, count, apdu,
                apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
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
 * @brief For a given object instance-number, writes to the property value
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and property is set.
 */
static bool Load_Control_Requested_Shed_Level_Write(
    uint32_t object_instance,
    const BACNET_SHED_LEVEL *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    int count, index;
    KEY key = 0;

    (void)priority;
    pObject = Object_Instance_Data(object_instance);
    if (!pObject) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    switch (value->type) {
        case BACNET_SHED_TYPE_PERCENT:
            if (value->value.percent <= 100) {
                pObject->Requested_Shed_Level.type = value->type;
                pObject->Requested_Shed_Level.value.percent =
                    value->value.percent;
                pObject->Load_Control_Request_Written = true;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
            break;
        case BACNET_SHED_TYPE_AMOUNT:
            if (value->value.amount >= 0.0f) {
                pObject->Requested_Shed_Level.type = value->type;
                pObject->Requested_Shed_Level.value.amount =
                    value->value.amount;
                pObject->Load_Control_Request_Written = true;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
            break;
        case BACNET_SHED_TYPE_LEVEL:
            /* can be 0 (default) or any value <= largest level */
            if (value->value.level == 0) {
                pObject->Requested_Shed_Level.type = value->type;
                pObject->Requested_Shed_Level.value.level = value->value.level;
                pObject->Load_Control_Request_Written = true;
                status = true;
            } else {
                count = Keylist_Count(pObject->Shed_Level_List);
                if (count > 0) {
                    /* keylist is sorted by key,
                       so the last index should be the largest key value */
                    index = count - 1;
                    if (Keylist_Index_Key(
                            pObject->Shed_Level_List, index, &key) &&
                        (value->value.level <= key)) {
                        pObject->Requested_Shed_Level.type = value->type;
                        pObject->Requested_Shed_Level.value.level =
                            value->value.level;
                        pObject->Load_Control_Request_Written = true;
                        status = true;
                    } else {
                        *error_class = ERROR_CLASS_PROPERTY;
                        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            break;
    }

    return status;
}

/**
 * @brief For a given object instance-number, writes to the property value
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and property is set.
 */
static bool Load_Control_Start_Time_Write(
    uint32_t object_instance,
    const BACNET_DATE_TIME *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    (void)priority;
    pObject = Object_Instance_Data(object_instance);
    if (!pObject) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* Write time and date and set written flag */
    datetime_copy_date(&pObject->Start_Time.date, &value->date);
    datetime_copy_time(&pObject->Start_Time.time, &value->time);
    pObject->Start_Time_Property_Written = true;
    status = true;

    return status;
}

/**
 * @brief For a given object instance-number, writes to the property value
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and property is set.
 */
static bool Load_Control_Shed_Duration_Write(
    uint32_t object_instance,
    BACNET_UNSIGNED_INTEGER value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    struct object_data *pObject;

    (void)priority;
    pObject = Object_Instance_Data(object_instance);
    if (!pObject) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* limited in our object to int32_t to work with datetime utility */
    if (value > INT32_MAX) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    pObject->Shed_Duration = (uint32_t)value;
    pObject->Load_Control_Request_Written = true;

    return true;
}

/**
 * @brief For a given object instance-number, writes to the property value
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and property is set.
 */
static bool Load_Control_Duty_Window_Write(
    uint32_t object_instance,
    BACNET_UNSIGNED_INTEGER value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    struct object_data *pObject;

    (void)priority;
    pObject = Object_Instance_Data(object_instance);
    if (!pObject) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* limited in our object to int32_t to work with datetime utility */
    if (value > INT32_MAX) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    pObject->Duty_Window = (uint32_t)value;
    pObject->Load_Control_Request_Written = true;

    return true;
}

/**
 * @brief For a given object instance-number, writes to the property value
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return false if an error is loaded, true if no errors
 */
static bool Load_Control_Shed_Levels_Write(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    struct object_data *pObject;
    BACNET_UNSIGNED_INTEGER unsigned_value;
    struct shed_level_data *entry;
    int len = 0, index = 0, count = 0, apdu_len = 0, apdu_size = 0;
    KEY key;
    uint8_t *apdu;

    pObject = Object_Instance_Data(wp_data->object_instance);
    if (!pObject) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    count = Keylist_Count(pObject->Shed_Level_List);
    if (wp_data->array_index == 0) {
        /* This array is not required to be resizable
            through BACnet write services */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        return false;
    } else if (wp_data->array_index == BACNET_ARRAY_ALL) {
        /* The size of this array shall be equal to the
            size of the Shed_Level_Descriptions array.*/
        /* will the array elements sent fit in the whole array? */
        apdu = wp_data->application_data;
        apdu_size = wp_data->application_data_len;
        while (count > 0) {
            len = bacnet_unsigned_application_decode(
                &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
            if (len > 0) {
                if (unsigned_value > UINT32_MAX) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    return false;
                }
                apdu_len += len;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                return false;
            }
            count--;
        }
        if (apdu_len != wp_data->application_data_len) {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            return false;
        }
        /* write entire array - we know the size and values are valid */
        count = Keylist_Count(pObject->Shed_Level_List);
        apdu = wp_data->application_data;
        apdu_size = wp_data->application_data_len;
        while (count > 0) {
            len = bacnet_unsigned_application_decode(
                &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
            if (len > 0) {
                apdu_len += len;
                if (unsigned_value <= UINT32_MAX) {
                    index = count - 1;
                    entry = Keylist_Data_Delete_By_Index(
                        pObject->Shed_Level_List, index);
                    key = (uint32_t)unsigned_value;
                    index =
                        Keylist_Data_Add(pObject->Shed_Level_List, key, entry);
                    if (index < 0) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code =
                            ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                        return false;
                    }
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    return false;
                }
            }
            count--;
        }
    } else if (wp_data->array_index <= count) {
        len = bacnet_unsigned_application_decode(
            wp_data->application_data, wp_data->application_data_len,
            &unsigned_value);
        if (len > 0) {
            if (unsigned_value <= UINT32_MAX) {
                index = wp_data->array_index - 1;
                entry = Keylist_Data_Delete_By_Index(
                    pObject->Shed_Level_List, index);
                key = (uint32_t)unsigned_value;
                index = Keylist_Data_Add(pObject->Shed_Level_List, key, entry);
                if (index < 0) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                    return false;
                }

            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                return false;
            }
        } else {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            return false;
        }
    } else {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
        return false;
    }

    return true;
}

/**
 * @brief For a given object instance-number, writes to the property value
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 * @return  true if values are within range and property is set.
 */
static bool Load_Control_Enable_Write(
    uint32_t object_instance,
    bool value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    struct object_data *pObject;

    (void)priority;
    pObject = Object_Instance_Data(object_instance);
    if (!pObject) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    pObject->Load_Control_Enable = value;

    return true;
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
bool Load_Control_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    if (wp_data == NULL) {
        debug_printf("Load_Control_Write_Property(): invalid data\n");
        return false;
    }
    if (wp_data->application_data_len < 0) {
        debug_printf("Load_Control_Write_Property(): invalid data length\n");
        /* error while decoding - a smaller larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /* decode the the request or the first element in array */
    len = bacapp_decode_known_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        wp_data->object_type, wp_data->object_property);
    if (len < 0) {
        debug_printf("Load_Control_Write_Property(): decoding error\n");
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_REQUESTED_SHED_LEVEL:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_SHED_LEVEL);
            if (status) {
                status = Load_Control_Requested_Shed_Level_Write(
                    wp_data->object_instance, &value.type.Shed_Level,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            }
            break;
        case PROP_START_TIME:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_DATETIME);
            if (status) {
                status = Load_Control_Start_Time_Write(
                    wp_data->object_instance, &value.type.Date_Time,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            }
            break;

        case PROP_SHED_DURATION:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Load_Control_Shed_Duration_Write(
                    wp_data->object_instance, value.type.Unsigned_Int,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            }
            break;

        case PROP_DUTY_WINDOW:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Load_Control_Duty_Window_Write(
                    wp_data->object_instance, value.type.Unsigned_Int,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            }
            break;

        case PROP_SHED_LEVELS:
            status = Load_Control_Shed_Levels_Write(wp_data);
            break;

        case PROP_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                status = Load_Control_Enable_Write(
                    wp_data->object_instance, value.type.Boolean,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            }
            break;
        default:
            debug_printf(
                "Load_Control_Write_Property() failure detected point Z\n");
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    debug_printf("Load_Control_Write_Property() returning status=%d\n", status);
    return status;
}

/**
 * @brief Sets a callback used when the manipulated object is written
 * @param object_instance - object-instance number of the object
 * @param cb - callback used to provide manipulations
 */
void Load_Control_Manipulated_Object_Write_Callback_Set(
    uint32_t object_instance, load_control_manipulated_object_write_callback cb)
{
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        pObject->Manipulated_Object_Write = cb;
    }
}

/**
 * @brief Sets a callback used when the manipulated object is relinquished
 * @param object_instance - object-instance number of the object
 * @param cb - callback used to provide manipulations
 */
void Load_Control_Manipulated_Object_Relinquish_Callback_Set(
    uint32_t object_instance,
    load_control_manipulated_object_relinquish_callback cb)
{
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        pObject->Manipulated_Object_Relinquish = cb;
    }
}

/**
 * @brief Sets a callback used when the manipulated object is read
 * @param object_instance - object-instance number of the object
 * @param cb - callback used to provide manipulations
 */
void Load_Control_Manipulated_Object_Read_Callback_Set(
    uint32_t object_instance, load_control_manipulated_object_read_callback cb)
{
    struct object_data *pObject;

    pObject = Object_Instance_Data(object_instance);
    if (pObject) {
        pObject->Manipulated_Object_Read = cb;
    }
}

/**
 * @brief For a given object instance-number, adds an array entity to a list.
 * @param  object_instance - object-instance number of the object
 * @param  array_index - index of the BACnetARRAY 1..N
 * @param  description - description of the array entity
 * @param  Value - value of the array entity
 * @return  true if the entity is added successfully.
 */
bool Load_Control_Shed_Level_Array_Set(
    uint32_t object_instance,
    uint32_t array_index,
    const struct shed_level_data *value)
{
    int key_index;
    struct shed_level_data *entry;
    struct object_data *pObject;
    KEY key = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return false;
    }
    if (array_index == 0) {
        return false;
    }
    key = array_index;
    entry = Keylist_Data(pObject->Shed_Level_List, key);
    if (!entry) {
        entry = calloc(1, sizeof(struct shed_level_data));
        if (!entry) {
            return false;
        }
        key_index = Keylist_Data_Add(pObject->Shed_Level_List, key, entry);
        if (key_index < 0) {
            free(entry);
            return false;
        }
    }
    entry->Value = value->Value;
    entry->Description = value->Description;

    return true;
}

/**
 * @brief Gets an entry from a list for a given object instance-number
 * @param  object_instance - object-instance number of the object
 * @param  array_entry - BACnetARRAY index of the array 1..N
 * @param  entry - data entry values are copied into
 * @return true if the data entry is found
 */
bool Load_Control_Shed_Level_Array(
    uint32_t object_instance,
    uint32_t array_entry,
    struct shed_level_data *value)
{
    struct shed_level_data *entry;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return false;
    }
    entry = Keylist_Data(pObject->Shed_Level_List, array_entry);
    if (!entry) {
        return false;
    }
    if (value) {
        value->Value = entry->Value;
        value->Description = entry->Description;
    }

    return true;
}

/**
 * @brief For a given object instance-number, gets the requested shed level
 *  property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be retrieved
 * @return the requested shed level of this object instance.
 */
bool Load_Control_Requested_Shed_Level(
    uint32_t object_instance, BACNET_SHED_LEVEL *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_shed_level_copy(value, &pObject->Requested_Shed_Level);
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the requested shed level
 *  property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 * @return true if requested shed level was set
 */
bool Load_Control_Requested_Shed_Level_Set(
    uint32_t object_instance, BACNET_SHED_LEVEL *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_shed_level_copy(&pObject->Requested_Shed_Level, value);
        pObject->Load_Control_Request_Written = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the expected shed level
 *  property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be retrieved
 * @return the expected shed level of this object instance.
 */
bool Load_Control_Expected_Shed_Level(
    uint32_t object_instance, BACNET_SHED_LEVEL *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_shed_level_copy(value, &pObject->Expected_Shed_Level);
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the expected shed level
 *  property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 * @return true if expected shed level was set
 */
bool Load_Control_Expected_Shed_Level_Set(
    uint32_t object_instance, BACNET_SHED_LEVEL *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_shed_level_copy(&pObject->Expected_Shed_Level, value);
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the actual shed level
 *  property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be retrieved
 * @return the actual shed level of this object instance.
 */
bool Load_Control_Actual_Shed_Level(
    uint32_t object_instance, BACNET_SHED_LEVEL *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_shed_level_copy(value, &pObject->Expected_Shed_Level);
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the actual shed level
 *  property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 * @return true if actual shed level was set
 */
bool Load_Control_Actual_Shed_Level_Set(
    uint32_t object_instance, BACNET_SHED_LEVEL *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_shed_level_copy(&pObject->Expected_Shed_Level, value);
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the start-time property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be retrieved
 * @return the start-time property value of this object instance.
 */
bool Load_Control_Start_Time(uint32_t object_instance, BACNET_DATE_TIME *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        datetime_copy_date(&value->date, &pObject->Start_Time.date);
        datetime_copy_time(&value->time, &pObject->Start_Time.time);
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the start-time property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 * @return true if property value was set
 */
bool Load_Control_Start_Time_Set(
    uint32_t object_instance, BACNET_DATE_TIME *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        datetime_copy_date(&pObject->Start_Time.date, &value->date);
        datetime_copy_time(&pObject->Start_Time.time, &value->time);
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the shed-duration property
 * @param object_instance - object-instance number of the object
 * @return the shed-duration property value of this object instance.
 */
uint32_t Load_Control_Shed_Duration(uint32_t object_instance)
{
    struct object_data *pObject;
    uint32_t value = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Shed_Duration;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the shed-duration property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 * @return true if property value was set
 */
bool Load_Control_Shed_Duration_Set(uint32_t object_instance, uint32_t value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Shed_Duration = value;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the duty-window property
 * @param object_instance - object-instance number of the object
 * @return the duty-window property value of this object instance.
 */
uint32_t Load_Control_Duty_Window(uint32_t object_instance)
{
    struct object_data *pObject;
    uint32_t value = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Duty_Window;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the duty-window property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 * @return true if property value was set
 */
bool Load_Control_Duty_Window_Set(uint32_t object_instance, uint32_t value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Duty_Window = value;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the full-duty-baseline
 *  property value
 * @param object_instance - object-instance number of the object
 * @return the full-duty-baseline property value of this object instance.
 */
float Load_Control_Full_Duty_Baseline(uint32_t object_instance)
{
    struct object_data *pObject;
    float value = 0.0f;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Full_Duty_Baseline;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the full-duty-baseline
 *  property value
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 * @return true if property value was set
 */
bool Load_Control_Full_Duty_Baseline_Set(uint32_t object_instance, float value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Full_Duty_Baseline = value;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the enable property value
 * @param object_instance - object-instance number of the object
 * @return the enable property value of this object instance.
 */
bool Load_Control_Enable(uint32_t object_instance)
{
    struct object_data *pObject;
    bool value = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Load_Control_Enable;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the enable property
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 * @return true if property value was set
 */
bool Load_Control_Enable_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Load_Control_Enable = value;
        status = true;
    }

    return status;
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Load_Control_Context_Get(uint32_t object_instance)
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
void Load_Control_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Creates a Load Control object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Load_Control_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;
    /* The Shed Level array shall be ordered by increasing shed amount */
    struct shed_level_data shed_levels[] = { { 90.0f, "Special" },
                                             { 80.0f, "Medium" },
                                             { 70.0f, "High" } };
    struct shed_level_data *entry;
    unsigned i = 0;

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
            /* defaults */
            pObject->Present_Value = BACNET_SHED_INACTIVE;
            pObject->Requested_Shed_Level.type = BACNET_SHED_TYPE_LEVEL;
            pObject->Requested_Shed_Level.value.level = 0;
            datetime_wildcard_set(&pObject->Start_Time);
            datetime_wildcard_set(&pObject->End_Time);
            pObject->Shed_Duration = 0;
            pObject->Duty_Window = 0;
            pObject->Load_Control_Enable = true;
            pObject->Full_Duty_Baseline = 1500.0f;
            pObject->Expected_Shed_Level.type = BACNET_SHED_TYPE_LEVEL;
            pObject->Expected_Shed_Level.value.level = 0;
            pObject->Actual_Shed_Level.type = BACNET_SHED_TYPE_LEVEL;
            pObject->Actual_Shed_Level.value.level = 0;
            pObject->Load_Control_Request_Written = false;
            pObject->Start_Time_Property_Written = false;
            pObject->Shed_Level_List = Keylist_Create();
            for (i = 0; i < ARRAY_SIZE(shed_levels); i++) {
                entry = calloc(1, sizeof(struct shed_level_data));
                if (entry) {
                    entry->Value = shed_levels[i].Value;
                    entry->Description = shed_levels[i].Description;
                    index = Keylist_Data_Add(
                        pObject->Shed_Level_List, 1 + i, entry);
                    if (index < 0) {
                        free(entry);
                    }
                }
            }
            pObject->Priority_For_Writing = 4;
            pObject->Manipulated_Object_Read = NULL;
            pObject->Manipulated_Object_Write = NULL;
            pObject->Manipulated_Object_Relinquish = NULL;
            pObject->Manipulated_Object_Type = OBJECT_ANALOG_OUTPUT;
            pObject->Manipulated_Object_Instance = object_instance;
            pObject->Manipulated_Object_Property = PROP_PRESENT_VALUE;
            /* some state machine variables */
            pObject->Previous_Value = BACNET_SHED_INACTIVE;
            pObject->Update_Interval = LOAD_CONTROL_TASK_INTERVAL_MS;
            pObject->Task_Milliseconds = 0;
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
 * Deletes an Load Control object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Load_Control_Delete(uint32_t object_instance)
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
 * Deletes all the Load Controls and their data
 */
void Load_Control_Cleanup(void)
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
 * Initializes the Load Control object data
 */
void Load_Control_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
