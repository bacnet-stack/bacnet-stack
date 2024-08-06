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
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/datetime.h"
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"

/* number of demo objects */
#ifndef MAX_LOAD_CONTROLS
#define MAX_LOAD_CONTROLS 4
#endif

/*  indicates the current load shedding state of the object */
static BACNET_SHED_STATE Present_Value[MAX_LOAD_CONTROLS];

/* load control objects are required to support LEVEL */
#define DEFAULT_VALUE_PERCENT 100
#define DEFAULT_VALUE_LEVEL 0
#define DEFAULT_VALUE_AMOUNT 0.0f

/* indicates the desired load shedding */
static BACNET_SHED_LEVEL Requested_Shed_Level[MAX_LOAD_CONTROLS];
/* Indicates the amount of power that the object expects
   to be able to shed in response to a load shed request. */
static BACNET_SHED_LEVEL Expected_Shed_Level[MAX_LOAD_CONTROLS];
/* Indicates the actual amount of power being shed in response
   to a load shed request. */
static BACNET_SHED_LEVEL Actual_Shed_Level[MAX_LOAD_CONTROLS];

/* indicates the start of the duty window in which the load controlled
   by the Load Control object must be compliant with the requested shed. */
static BACNET_DATE_TIME Start_Time[MAX_LOAD_CONTROLS];
static BACNET_DATE_TIME End_Time[MAX_LOAD_CONTROLS];

/* indicates the duration of the load shed action,
   starting at Start_Time in minutes */
static uint32_t Shed_Duration[MAX_LOAD_CONTROLS];

/* indicates the time window used for load shed accounting in minutes */
static uint32_t Duty_Window[MAX_LOAD_CONTROLS];

/* indicates and controls whether the Load Control object is
   currently enabled to respond to load shed requests.  */
static bool Load_Control_Enable[MAX_LOAD_CONTROLS];

/* indicates when the object receives a write to any of the properties
   Requested_Shed_Level, Shed_Duration, Duty_Window */
static bool Load_Control_Request_Written[MAX_LOAD_CONTROLS];
/* indicates when the object receives a write to Start_Time */
static bool Start_Time_Property_Written[MAX_LOAD_CONTROLS];

/* optional: indicates the baseline power consumption value
   for the sheddable load controlled by this object,
   if a fixed baseline is used.
   The units of Full_Duty_Baseline are kilowatts.*/
static float Full_Duty_Baseline[MAX_LOAD_CONTROLS];

#define MAX_SHED_LEVELS 3
/* Represents the shed levels for the LEVEL choice of
   BACnetShedLevel that have meaning for this particular
   Load Control object. */
/* The elements of the array are required to be writable,
   allowing local configuration of how this Load Control
   object will participate in load shedding for the
   facility. This array is not required to be resizable
   through BACnet write services. The size of this array
   shall be equal to the size of the Shed_Level_Descriptions
   array. The behavior of this object when the Shed_Levels
   array contains duplicate entries is a local matter. */
static unsigned Shed_Levels[MAX_LOAD_CONTROLS][MAX_SHED_LEVELS];

/* represents a description of the shed levels that the
   Load Control object can take on.  It is the same for
   all the load control objects in this example device. */
static char *Shed_Level_Descriptions[MAX_SHED_LEVELS] = { "dim lights 10%",
                                                          "dim lights 20%",
                                                          "dim lights 30%" };

static float Shed_Level_Values[MAX_SHED_LEVELS] = { 90.0f, 80.0f, 70.0f };

/* borrow some properties from the Loop object */
static BACNET_OBJECT_PROPERTY_REFERENCE
    Manipulated_Variable_Reference[MAX_LOAD_CONTROLS];
static unsigned Priority_For_Writing[MAX_LOAD_CONTROLS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Load_Control_Properties_Required[] = {
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

static const int Load_Control_Properties_Optional[] = { PROP_DESCRIPTION,
                                                        PROP_FULL_DUTY_BASELINE,
                                                        -1 };

static const int Load_Control_Properties_Proprietary[] = { -1 };

void Load_Control_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
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

void Load_Control_Init(void)
{
    unsigned i, j;

    for (i = 0; i < MAX_LOAD_CONTROLS; i++) {
        /* FIXME: load saved data? */
        Present_Value[i] = BACNET_SHED_INACTIVE;
        Requested_Shed_Level[i].type = BACNET_SHED_TYPE_LEVEL;
        Requested_Shed_Level[i].value.level = 0;
        datetime_wildcard_set(&Start_Time[i]);
        datetime_wildcard_set(&End_Time[i]);
        Shed_Duration[i] = 0;
        Duty_Window[i] = 0;
        Load_Control_Enable[i] = true;
        Full_Duty_Baseline[i] = 1.500f; /* kilowatts */
        Expected_Shed_Level[i].type = BACNET_SHED_TYPE_LEVEL;
        Expected_Shed_Level[i].value.level = 0;
        Actual_Shed_Level[i].type = BACNET_SHED_TYPE_LEVEL;
        Actual_Shed_Level[i].value.level = 0;
        Load_Control_Request_Written[i] = false;
        Start_Time_Property_Written[i] = false;
        for (j = 0; j < MAX_SHED_LEVELS; j++) {
            Shed_Levels[i][j] = j + 1;
        }
        Priority_For_Writing[i] = 4;
        Manipulated_Variable_Reference[i].object_identifier.type =
            OBJECT_ANALOG_OUTPUT;
        Manipulated_Variable_Reference[i].object_identifier.instance = 1 + i;
        Manipulated_Variable_Reference[i].property_identifier =
            PROP_PRESENT_VALUE;
        switch (Manipulated_Variable_Reference[i].object_identifier.type) {
            case OBJECT_ANALOG_OUTPUT:
                if (Analog_Output_Valid_Instance(
                        Manipulated_Variable_Reference[i]
                            .object_identifier.instance)) {
                    Analog_Output_Delete(Manipulated_Variable_Reference[i]
                                             .object_identifier.instance);
                }
                Analog_Output_Create(Manipulated_Variable_Reference[i]
                                         .object_identifier.instance);
                break;
            default:
                break;
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Load_Control_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_LOAD_CONTROLS) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Load_Control_Count(void)
{
    return MAX_LOAD_CONTROLS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Load_Control_Index_To_Instance(unsigned index)
{
    if (index < MAX_LOAD_CONTROLS) {
        return index;
    }
    return MAX_LOAD_CONTROLS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Load_Control_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_LOAD_CONTROLS;

    if (object_instance < MAX_LOAD_CONTROLS) {
        index = object_instance;
    }

    return index;
}

static BACNET_SHED_STATE Load_Control_Present_Value(uint32_t object_instance)
{
    BACNET_SHED_STATE value = BACNET_SHED_INACTIVE;
    unsigned index = 0;

    index = Load_Control_Instance_To_Index(object_instance);
    if (index < MAX_LOAD_CONTROLS) {
        value = Present_Value[index];
    }

    return value;
}

/* note: the object name must be unique within this device */
bool Load_Control_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text[32] = ""; /* okay for single thread */
    bool status = false;

    if (object_instance < MAX_LOAD_CONTROLS) {
        snprintf(
            text, sizeof(text), "LOAD CONTROL %lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
}

/* convert the shed level request into an Analog Output Present_Value */
static float Requested_Shed_Level_Value(int object_index)
{
    unsigned shed_level_index = 0;
    unsigned i = 0;
    float requested_level = 0.0f;

    switch (Requested_Shed_Level[object_index].type) {
        case BACNET_SHED_TYPE_PERCENT:
            requested_level =
                (float)Requested_Shed_Level[object_index].value.percent;
            break;
        case BACNET_SHED_TYPE_AMOUNT:
            /* Assumptions: wattage is linear with analog output level */
            requested_level = Full_Duty_Baseline[object_index] -
                Requested_Shed_Level[object_index].value.amount;
            requested_level /= Full_Duty_Baseline[object_index];
            requested_level *= 100.0f;
            break;
        case BACNET_SHED_TYPE_LEVEL:
        default:
            for (i = 0; i < MAX_SHED_LEVELS; i++) {
                if (Shed_Levels[object_index][i] <=
                    Requested_Shed_Level[object_index].value.level) {
                    shed_level_index = i;
                }
            }
            requested_level = Shed_Level_Values[shed_level_index];
            break;
    }

    return requested_level;
}

static void Shed_Level_Copy(BACNET_SHED_LEVEL *dest, BACNET_SHED_LEVEL *src)
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

static bool Able_To_Meet_Shed_Request(int object_index)
{
    float level = 0.0f;
    float requested_level = 0.0f;
    unsigned priority = 0;
    bool status = false;
    int object_instance = 0;
    BACNET_OBJECT_TYPE object_type;

    object_instance =
        Manipulated_Variable_Reference[object_index].object_identifier.instance;
    object_type =
        Manipulated_Variable_Reference[object_index].object_identifier.type;
    switch (object_type) {
        case OBJECT_ANALOG_OUTPUT:
            priority = Analog_Output_Present_Value_Priority(object_instance);
            level = Analog_Output_Present_Value(object_instance);
            status = true;
            break;
        default:
            break;
    }
    if (status) {
        status = false;
        /* can we control the output? */
        if (priority >= Priority_For_Writing[object_index]) {
            /* is the level able to be lowered? */
            requested_level = Requested_Shed_Level_Value(object_index);
            if (level >= requested_level) {
                status = true;
            }
        }
    }

    return status;
}

static BACNET_LOAD_CONTROL_STATE Load_Control_State_Active[MAX_LOAD_CONTROLS];
static BACNET_LOAD_CONTROL_STATE
    Load_Control_State_Previously[MAX_LOAD_CONTROLS];

/**
 * @brief Get the current state of the Load Control object.
 * @param object_index [in] The object index number.
 * @return The current state of the Load Control object.
 */
BACNET_LOAD_CONTROL_STATE Load_Control_State(int object_index)
{
    BACNET_LOAD_CONTROL_STATE state = SHED_INACTIVE;

    if (object_index < MAX_LOAD_CONTROLS) {
        state = Load_Control_State_Active[object_index];
    }

    return state;
}

#if PRINT_ENABLED_DEBUG
static void Print_Load_Control_State(int object_index)
{
    char *Load_Control_State_Text[MAX_LOAD_CONTROLS] = { "SHED_INACTIVE",
                                                         "SHED_REQUEST_PENDING",
                                                         "SHED_NON_COMPLIANT",
                                                         "SHED_COMPLIANT" };

    if (object_index < MAX_LOAD_CONTROLS) {
        if (Load_Control_State_Active[object_index] < MAX_LOAD_CONTROL_STATE) {
            printf(
                "Load Control[%d]=%s\n", object_index,
                Load_Control_State_Text
                    [Load_Control_State_Active[object_index]]);
        }
    }
}
#endif

void Load_Control_State_Machine(int object_index, BACNET_DATE_TIME *bdatetime)
{
    unsigned i = 0; /* loop counter */
    int diff = 0; /* used for datetime comparison */
    float amount;
    unsigned percent;
    unsigned level;
    unsigned priority;
    BACNET_OBJECT_ID object_id;

    if (object_index >= MAX_LOAD_CONTROLS) {
        return;
    }
    /* is the state machine enabled? */
    if (!Load_Control_Enable[object_index]) {
        Load_Control_State_Active[object_index] = SHED_INACTIVE;
        return;
    }

    switch (Load_Control_State_Active[object_index]) {
        case SHED_REQUEST_PENDING:
            if (Load_Control_Request_Written[object_index]) {
                Load_Control_Request_Written[object_index] = false;
                /* request to cancel using default values? */
                switch (Requested_Shed_Level[object_index].type) {
                    case BACNET_SHED_TYPE_PERCENT:
                        percent =
                            Requested_Shed_Level[object_index].value.percent;
                        if (percent == DEFAULT_VALUE_PERCENT) {
                            Load_Control_State_Active[object_index] =
                                SHED_INACTIVE;
                        }
                        break;
                    case BACNET_SHED_TYPE_AMOUNT:
                        amount =
                            Requested_Shed_Level[object_index].value.amount;
                        if (islessequal(amount, DEFAULT_VALUE_AMOUNT)) {
                            Load_Control_State_Active[object_index] =
                                SHED_INACTIVE;
                        }
                        break;
                    case BACNET_SHED_TYPE_LEVEL:
                    default:
                        level = Requested_Shed_Level[object_index].value.level;
                        if (level == DEFAULT_VALUE_LEVEL) {
                            Load_Control_State_Active[object_index] =
                                SHED_INACTIVE;
                        }
                        break;
                }
                if (Load_Control_State_Active[object_index] == SHED_INACTIVE) {
#if PRINT_ENABLED_DEBUG
                    printf(
                        "Load Control[%d]:Requested Shed Level=Default\n",
                        object_index);
#endif
                    break;
                }
            }
            /* clear the flag for Start time if it is written */
            if (Start_Time_Property_Written[object_index]) {
                Start_Time_Property_Written[object_index] = false;
                /* request to cancel using wildcards in start time? */
                if (datetime_wildcard(&Start_Time[object_index])) {
                    Load_Control_State_Active[object_index] = SHED_INACTIVE;
#if PRINT_ENABLED_DEBUG
                    printf(
                        "Load Control[%d]:Start Time=Wildcard\n", object_index);
#endif
                    break;
                }
            }
            /* cancel because current time is after start time + duration? */
            datetime_copy(&End_Time[object_index], &Start_Time[object_index]);
            datetime_add_minutes(
                &End_Time[object_index], Shed_Duration[object_index]);
            diff = datetime_compare(&End_Time[object_index], bdatetime);
            if (diff < 0) {
                /* CancelShed */
                /* FIXME: stop shedding! i.e. relinquish */
#if PRINT_ENABLED_DEBUG
                printf(
                    "Load Control[%d]:Current Time"
                    " is after Start Time + Duration\n",
                    object_index);
#endif
                Load_Control_State_Active[object_index] = SHED_INACTIVE;
                break;
            }
            diff = datetime_compare(bdatetime, &Start_Time[object_index]);
            if (diff < 0) {
                /* current time prior to start time */
                /* ReconfigurePending */
                Shed_Level_Copy(
                    &Expected_Shed_Level[object_index],
                    &Requested_Shed_Level[object_index]);
                Shed_Level_Default_Set(
                    &Actual_Shed_Level[object_index],
                    Requested_Shed_Level[object_index].type);
            } else if (diff > 0) {
                /* current time after to start time */
                debug_printf(
                    "Load Control[%d]:Current Time"
                    " is after Start Time\n",
                    object_index);
                /* AbleToMeetShed */
                if (Able_To_Meet_Shed_Request(object_index)) {
                    Shed_Level_Copy(
                        &Expected_Shed_Level[object_index],
                        &Requested_Shed_Level[object_index]);
                    object_id.instance =
                        Manipulated_Variable_Reference[object_index]
                            .object_identifier.instance;
                    object_id.type =
                        Manipulated_Variable_Reference[object_index]
                            .object_identifier.type;
                    priority = Priority_For_Writing[object_index];
                    switch (object_id.type) {
                        case OBJECT_ANALOG_OUTPUT:
                            Analog_Output_Present_Value_Set(
                                object_id.instance,
                                Requested_Shed_Level_Value(object_index),
                                priority);
                            break;
                        default:
                            break;
                    }
                    Shed_Level_Copy(
                        &Actual_Shed_Level[object_index],
                        &Requested_Shed_Level[object_index]);
                    Load_Control_State_Active[object_index] = SHED_COMPLIANT;
                } else {
                    /* CannotMeetShed */
                    Shed_Level_Default_Set(
                        &Expected_Shed_Level[object_index],
                        Requested_Shed_Level[object_index].type);
                    Shed_Level_Default_Set(
                        &Actual_Shed_Level[object_index],
                        Requested_Shed_Level[object_index].type);
                    Load_Control_State_Active[object_index] =
                        SHED_NON_COMPLIANT;
                }
            }
            break;
        case SHED_NON_COMPLIANT:
            datetime_copy(&End_Time[object_index], &Start_Time[object_index]);
            datetime_add_minutes(
                &End_Time[object_index], Shed_Duration[object_index]);
            diff = datetime_compare(&End_Time[object_index], bdatetime);
            if (diff < 0) {
                /* FinishedUnsuccessfulShed */
#if PRINT_ENABLED_DEBUG
                printf(
                    "Load Control[%d]:Current Time is after Start Time + "
                    "Duration\n",
                    object_index);
#endif
                Load_Control_State_Active[object_index] = SHED_INACTIVE;
                break;
            }
            if (Load_Control_Request_Written[object_index] ||
                Start_Time_Property_Written[object_index]) {
                /* UnsuccessfulShedReconfigured */
#if PRINT_ENABLED_DEBUG
                printf(
                    "Load Control[%d]:Control Property written\n",
                    object_index);
#endif
                /* The Written flags will cleared in the next state */
                Load_Control_State_Active[object_index] = SHED_REQUEST_PENDING;
                break;
            }
            if (Able_To_Meet_Shed_Request(object_index)) {
                /* CanNowComplyWithShed */
#if PRINT_ENABLED_DEBUG
                printf(
                    "Load Control[%d]:Able to meet Shed Request\n",
                    object_index);
#endif
                Shed_Level_Copy(
                    &Expected_Shed_Level[object_index],
                    &Requested_Shed_Level[object_index]);
                object_id.instance =
                    Manipulated_Variable_Reference[object_index]
                        .object_identifier.instance;
                object_id.type = Manipulated_Variable_Reference[object_index]
                                     .object_identifier.type;
                priority = Priority_For_Writing[object_index];
                switch (object_id.type) {
                    case OBJECT_ANALOG_OUTPUT:
                        Analog_Output_Present_Value_Set(
                            object_id.instance,
                            Requested_Shed_Level_Value(object_index), priority);
                        break;
                    default:
                        break;
                }
                Shed_Level_Copy(
                    &Actual_Shed_Level[object_index],
                    &Requested_Shed_Level[object_index]);
                Load_Control_State_Active[object_index] = SHED_COMPLIANT;
            }
            break;
        case SHED_COMPLIANT:
            datetime_copy(&End_Time[object_index], &Start_Time[object_index]);
            datetime_add_minutes(
                &End_Time[object_index], Shed_Duration[object_index]);
            diff = datetime_compare(&End_Time[object_index], bdatetime);
            if (diff < 0) {
                /* FinishedSuccessfulShed */
#if PRINT_ENABLED_DEBUG
                printf(
                    "Load Control[%d]:Current Time is after Start Time + "
                    "Duration\n",
                    object_index);
#endif
                datetime_wildcard_set(&Start_Time[i]);
                object_id.instance =
                    Manipulated_Variable_Reference[object_index]
                        .object_identifier.instance;
                object_id.type = Manipulated_Variable_Reference[object_index]
                                     .object_identifier.type;
                priority = Priority_For_Writing[object_index];
                switch (object_id.type) {
                    case OBJECT_ANALOG_OUTPUT:
                        Analog_Output_Present_Value_Relinquish(
                            object_id.instance, priority);
                        break;
                    default:
                        break;
                }
                Load_Control_State_Active[object_index] = SHED_INACTIVE;
                break;
            }
            if (Load_Control_Request_Written[object_index] ||
                Start_Time_Property_Written[object_index]) {
                /* UnsuccessfulShedReconfigured */
#if PRINT_ENABLED_DEBUG
                printf(
                    "Load Control[%d]:Control Property written\n",
                    object_index);
#endif
                /* The Written flags will cleared in the next state */
                Load_Control_State_Active[object_index] = SHED_REQUEST_PENDING;
                break;
            }
            if (!Able_To_Meet_Shed_Request(object_index)) {
                /* CanNoLongerComplyWithShed */
#if PRINT_ENABLED_DEBUG
                printf(
                    "Load Control[%d]:Not able to meet Shed Request\n",
                    object_index);
#endif
                Shed_Level_Default_Set(
                    &Expected_Shed_Level[object_index],
                    Requested_Shed_Level[object_index].type);
                Shed_Level_Default_Set(
                    &Actual_Shed_Level[object_index],
                    Requested_Shed_Level[object_index].type);
                Load_Control_State_Active[object_index] = SHED_NON_COMPLIANT;
            }
            break;
        case SHED_INACTIVE:
        default:
            if (Start_Time_Property_Written[object_index]) {
#if PRINT_ENABLED_DEBUG
                printf("Load Control[%d]:Start Time written\n", object_index);
#endif
                /* The Written flag will cleared in the next state */
                Shed_Level_Copy(
                    &Expected_Shed_Level[object_index],
                    &Requested_Shed_Level[object_index]);
                Shed_Level_Default_Set(
                    &Actual_Shed_Level[object_index],
                    Requested_Shed_Level[object_index].type);
                Load_Control_State_Active[object_index] = SHED_REQUEST_PENDING;
            }
            break;
    }

    return;
}

/* call every second or so */
void Load_Control_State_Machine_Handler(void)
{
    unsigned i = 0;
    static bool initialized = false;
    BACNET_DATE_TIME bdatetime = { 0 };

    if (!initialized) {
        initialized = true;
        for (i = 0; i < MAX_LOAD_CONTROLS; i++) {
            Load_Control_State_Active[i] = SHED_INACTIVE;
            Load_Control_State_Previously[i] = SHED_INACTIVE;
        }
    }
    datetime_local(&bdatetime.date, &bdatetime.time, NULL, NULL);
    for (i = 0; i < MAX_LOAD_CONTROLS; i++) {
        Load_Control_State_Machine(i, &bdatetime);
        if (Load_Control_State_Active[i] != Load_Control_State_Previously[i]) {
#if PRINT_ENABLED_DEBUG
            Print_Load_Control_State(i);
#endif
            Load_Control_State_Previously[i] = Load_Control_State_Active[i];
        }
    }
}

/**
 * @brief Get the priority for writing to the Manipulated Variable.
 * @param object_instance [in] The object instance number.
 */
unsigned Load_Control_Priority_For_Writing(uint32_t object_instance)
{
    unsigned priority = 0;
    unsigned object_index = 0;

    object_index = Load_Control_Instance_To_Index(object_instance);
    if (object_index < MAX_LOAD_CONTROLS) {
        priority = Priority_For_Writing[object_index];
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
    unsigned object_index = 0;
    bool status = false;

    object_index = Load_Control_Instance_To_Index(object_instance);
    if (object_index < MAX_LOAD_CONTROLS) {
        Priority_For_Writing[object_index] = priority;
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
    unsigned object_index = 0;
    bool status = false;

    object_index = Load_Control_Instance_To_Index(object_instance);
    if (object_index < MAX_LOAD_CONTROLS) {
        if (object_property_reference) {
            object_property_reference->object_identifier.instance =
                Manipulated_Variable_Reference[object_index]
                    .object_identifier.instance;
            object_property_reference->object_identifier.type =
                Manipulated_Variable_Reference[object_index]
                    .object_identifier.type;
            object_property_reference->property_identifier =
                Manipulated_Variable_Reference[object_index]
                    .property_identifier;
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
    BACNET_OBJECT_PROPERTY_REFERENCE *object_property_reference)
{
    unsigned object_index = 0;
    bool status = false;

    object_index = Load_Control_Instance_To_Index(object_instance);
    if (object_index < MAX_LOAD_CONTROLS) {
        if (object_property_reference) {
            Manipulated_Variable_Reference[object_index]
                .object_identifier.instance =
                object_property_reference->object_identifier.instance;
            Manipulated_Variable_Reference[object_index]
                .object_identifier.type =
                object_property_reference->object_identifier.type;
            Manipulated_Variable_Reference[object_index].property_identifier =
                object_property_reference->property_identifier;
            status = true;
        }
    }

    return status;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Load_Control_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    int enumeration = 0;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    object_index = Load_Control_Instance_To_Index(rpdata->object_instance);
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
            switch (Requested_Shed_Level[object_index].type) {
                case BACNET_SHED_TYPE_PERCENT:
                    apdu_len = encode_context_unsigned(
                        &apdu[0], 0,
                        Requested_Shed_Level[object_index].value.percent);
                    break;
                case BACNET_SHED_TYPE_AMOUNT:
                    apdu_len = encode_context_real(
                        &apdu[0], 2,
                        Requested_Shed_Level[object_index].value.amount);
                    break;
                case BACNET_SHED_TYPE_LEVEL:
                default:
                    apdu_len = encode_context_unsigned(
                        &apdu[0], 1,
                        Requested_Shed_Level[object_index].value.level);
                    break;
            }
            break;
        case PROP_START_TIME:
            len = encode_application_date(
                &apdu[0], &Start_Time[object_index].date);
            apdu_len = len;
            len = encode_application_time(
                &apdu[apdu_len], &Start_Time[object_index].time);
            apdu_len += len;
            break;
        case PROP_SHED_DURATION:
            apdu_len = encode_application_unsigned(
                &apdu[0], Shed_Duration[object_index]);
            break;
        case PROP_DUTY_WINDOW:
            apdu_len = encode_application_unsigned(
                &apdu[0], Duty_Window[object_index]);
            break;
        case PROP_ENABLE:
            state = Load_Control_Enable[object_index];
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_FULL_DUTY_BASELINE: /* optional */
            apdu_len = encode_application_real(
                &apdu[0], Full_Duty_Baseline[object_index]);
            break;
        case PROP_EXPECTED_SHED_LEVEL:
            switch (Expected_Shed_Level[object_index].type) {
                case BACNET_SHED_TYPE_PERCENT:
                    apdu_len = encode_context_unsigned(
                        &apdu[0], 0,
                        Expected_Shed_Level[object_index].value.percent);
                    break;
                case BACNET_SHED_TYPE_AMOUNT:
                    apdu_len = encode_context_real(
                        &apdu[0], 2,
                        Expected_Shed_Level[object_index].value.amount);
                    break;
                case BACNET_SHED_TYPE_LEVEL:
                default:
                    apdu_len = encode_context_unsigned(
                        &apdu[0], 1,
                        Expected_Shed_Level[object_index].value.level);
                    break;
            }
            break;
        case PROP_ACTUAL_SHED_LEVEL:
            switch (Actual_Shed_Level[object_index].type) {
                case BACNET_SHED_TYPE_PERCENT:
                    apdu_len = encode_context_unsigned(
                        &apdu[0], 0,
                        Actual_Shed_Level[object_index].value.percent);
                    break;
                case BACNET_SHED_TYPE_AMOUNT:
                    apdu_len = encode_context_real(
                        &apdu[0], 2,
                        Actual_Shed_Level[object_index].value.amount);
                    break;
                case BACNET_SHED_TYPE_LEVEL:
                default:
                    apdu_len = encode_context_unsigned(
                        &apdu[0], 1,
                        Actual_Shed_Level[object_index].value.level);
                    break;
            }
            break;
        case PROP_SHED_LEVELS:
            /* Array element zero is the number of elements in the array */
            if (rpdata->array_index == 0) {
                apdu_len =
                    encode_application_unsigned(&apdu[0], MAX_SHED_LEVELS);
                /* if no index was specified, then try to encode the entire list
                 */
                /* into one packet. */
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                apdu_len = 0;
                for (i = 0; i < MAX_SHED_LEVELS; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    len = encode_application_unsigned(
                        &apdu[apdu_len], Shed_Levels[object_index][i]);
                    /* add it if we have room */
                    if ((apdu_len + len) < MAX_APDU) {
                        apdu_len += len;
                    } else {
                        rpdata->error_code =
                            ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                        apdu_len = BACNET_STATUS_ABORT;
                        break;
                    }
                }
            } else {
                if (rpdata->array_index <= MAX_SHED_LEVELS) {
                    apdu_len = encode_application_unsigned(
                        &apdu[0],
                        Shed_Levels[object_index][rpdata->array_index - 1]);
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        case PROP_SHED_LEVEL_DESCRIPTIONS:
            /* Array element zero is the number of elements in the array */
            if (rpdata->array_index == 0) {
                apdu_len =
                    encode_application_unsigned(&apdu[0], MAX_SHED_LEVELS);
                /* if no index was specified, then try to encode the entire list
                 */
                /* into one packet. */
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                apdu_len = 0;
                for (i = 0; i < MAX_SHED_LEVELS; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    characterstring_init_ansi(
                        &char_string, Shed_Level_Descriptions[i]);
                    len = encode_application_character_string(
                        &apdu[apdu_len], &char_string);
                    /* add it if we have room */
                    if ((apdu_len + len) < MAX_APDU) {
                        apdu_len += len;
                    } else {
                        rpdata->error_code =
                            ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                        apdu_len = BACNET_STATUS_ABORT;
                        break;
                    }
                }
            } else {
                if (rpdata->array_index <= MAX_SHED_LEVELS) {
                    characterstring_init_ansi(
                        &char_string,
                        Shed_Level_Descriptions[rpdata->array_index - 1]);
                    apdu_len = encode_application_character_string(
                        &apdu[0], &char_string);
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_SHED_LEVEL_DESCRIPTIONS) &&
        (rpdata->object_property != PROP_SHED_LEVELS) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
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
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Load_Control_Requested_Shed_Level_Write(
    uint32_t object_instance,
    BACNET_SHED_LEVEL *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    unsigned int object_index = 0;

    (void)priority;
    object_index = Load_Control_Instance_To_Index(object_instance);
    if (object_index < MAX_LOAD_CONTROLS) {
        switch (value->type) {
            case BACNET_SHED_TYPE_PERCENT:
                if (value->value.percent <= 100) {
                    Requested_Shed_Level[object_index].type = value->type;
                    Requested_Shed_Level[object_index].value.percent =
                        value->value.percent;
                    Load_Control_Request_Written[object_index] = true;
                    status = true;
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
                break;
            case BACNET_SHED_TYPE_AMOUNT:
                if (value->value.amount >= 0.0f) {
                    Requested_Shed_Level[object_index].type = value->type;
                    Requested_Shed_Level[object_index].value.amount =
                        value->value.amount;
                    Load_Control_Request_Written[object_index] = true;
                    status = true;
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
                break;
            case BACNET_SHED_TYPE_LEVEL:
                if (value->value.level <= MAX_SHED_LEVELS) {
                    Requested_Shed_Level[object_index].type = value->type;
                    Requested_Shed_Level[object_index].value.level =
                        value->value.level;
                    Load_Control_Request_Written[object_index] = true;
                    status = true;
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
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
 * @brief For a given object instance-number, writes to the property value
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Load_Control_Start_Time_Write(
    uint32_t object_instance,
    BACNET_DATE_TIME *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    unsigned int object_index = 0;

    (void)priority;
    object_index = Load_Control_Instance_To_Index(object_instance);
    if (object_index < MAX_LOAD_CONTROLS) {
        /* Write time and date and set written flag */
        datetime_copy_date(&Start_Time[object_index].date, &value->date);
        datetime_copy_time(&Start_Time[object_index].time, &value->time);
        Start_Time_Property_Written[object_index] = true;
        status = true;
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/* returns true if successful */
bool Load_Control_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    /* build here in case of error in time half of datetime */
    BACNET_DATE start_date;

    debug_printf("Load_Control_Write_Property(wp_data=%p)\n", wp_data);
    if (wp_data == NULL) {
        debug_printf(
            "Load_Control_Write_Property() failure detected point A\n");
        return false;
    }
    if (wp_data->application_data_len < 0) {
        debug_printf(
            "Load_Control_Write_Property() failure detected point A.2\n");
        /* error while decoding - a smaller larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }

    /* decode the some of the request */
    len = bacapp_decode_known_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        wp_data->object_type, wp_data->object_property);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        debug_printf(
            "Load_Control_Write_Property() failure detected point B\n");
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /*  only array properties can have array options */
    if ((wp_data->object_property != PROP_SHED_LEVELS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        debug_printf(
            "Load_Control_Write_Property() failure detected point C\n");
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    object_index = Load_Control_Instance_To_Index(wp_data->object_instance);
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
                Shed_Duration[object_index] = value.type.Unsigned_Int;
                Load_Control_Request_Written[object_index] = true;
            } else {
                debug_printf(
                    "Load_Control_Write_Property() failure detected point H\n");
            }
            break;

        case PROP_DUTY_WINDOW:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                Duty_Window[object_index] = value.type.Unsigned_Int;
                Load_Control_Request_Written[object_index] = true;
            }
            break;

        case PROP_SHED_LEVELS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                /* re-write the size of the array? */
                if (wp_data->array_index == 0) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                    status = false;
                } else if (wp_data->array_index == BACNET_ARRAY_ALL) {
                    /* FIXME: write entire array */
                } else if (wp_data->array_index <= MAX_SHED_LEVELS) {
                    Shed_Levels[object_index][wp_data->array_index - 1] =
                        value.type.Unsigned_Int;
                } else {
                    /* FIXME: Something's missing from here so I'll just put in
                     * a place holder error here for the moment*/
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_OTHER;
                    status = false;
                }
            }
            break;

        case PROP_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Load_Control_Enable[object_index] = value.type.Boolean;
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
