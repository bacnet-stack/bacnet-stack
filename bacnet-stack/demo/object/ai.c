/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
* Copyright (C) 2011 Krzysztof Malorny <malornykrzysztof@gmail.com>
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

/* Analog Input Objects customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bactext.h"
#include "config.h"     /* the custom stuff */
#include "device.h"
#include "handlers.h"
#include "timestamp.h"
#include "nc.h"
#include "ai.h"


#ifndef MAX_ANALOG_INPUTS
#define MAX_ANALOG_INPUTS 4
#endif


ANALOG_INPUT_DESCR AI_Descr[MAX_ANALOG_INPUTS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_UNITS,
    -1
};

static const int Properties_Optional[] = {
    PROP_DESCRIPTION,
#if defined(INTRINSIC_REPORTING)
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_HIGH_LIMIT,
    PROP_LOW_LIMIT,
    PROP_DEADBAND,
    PROP_LIMIT_ENABLE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
#endif
    -1
};

static const int Properties_Proprietary[] = {
    9997,
    9998,
    9999,
    -1
};

void Analog_Input_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Properties_Required;
    if (pOptional)
        *pOptional = Properties_Optional;
    if (pProprietary)
        *pProprietary = Properties_Proprietary;

    return;
}


void Analog_Input_Init(
    void)
{
    unsigned i, j;

    for (i = 0; i < MAX_ANALOG_INPUTS; i++) {
        AI_Descr[i].Present_Value  = 0.0f;
        AI_Descr[i].Out_Of_Service = false;
        AI_Descr[i].Units = UNITS_PERCENT;
        AI_Descr[i].Reliability = RELIABILITY_NO_FAULT_DETECTED;
#if defined(INTRINSIC_REPORTING)
        AI_Descr[i].Event_State = EVENT_STATE_NORMAL;
        /* notification class not connected */
        AI_Descr[i].Notification_Class = BACNET_MAX_INSTANCE;
        /* initialize Event time stamps using wildcards
           and set Acked_transitions */
        for (j = 0; j < MAX_BACNET_EVENT_TRANSITION; j++) {
            datetime_wildcard_set(&AI_Descr[i].Event_Time_Stamps[j]);
            AI_Descr[i].Acked_Transitions[j].bIsAcked = true;
        }
#endif
    }
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Input_Valid_Instance(
    uint32_t object_instance)
{
    unsigned int index;

    index = Analog_Input_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_INPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Analog_Input_Count(
    void)
{
    return MAX_ANALOG_INPUTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Analog_Input_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Analog_Input_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_ANALOG_INPUTS;

    if (object_instance < MAX_ANALOG_INPUTS)
        index = object_instance;

    return index;
}

float Analog_Input_Present_Value(
    uint32_t object_instance)
{
    float value = 0.0;
    unsigned int index;

    index = Analog_Input_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_INPUTS) {
        value = AI_Descr[index].Present_Value;
    }

    return value;
}

void Analog_Input_Present_Value_Set(
    uint32_t object_instance,
    float value)
{
    unsigned int index;

    index = Analog_Input_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_INPUTS) {
        AI_Descr[index].Present_Value = value;
    }
}

bool Analog_Input_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = "";   /* okay for single thread */
    unsigned int index;
    bool status = false;

    index = Analog_Input_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_INPUTS) {
        sprintf(text_string, "ANALOG INPUT %lu", (unsigned long) index);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}

/* return apdu length, or BACNET_STATUS_ERROR on error */
/* assumption - object has already exists */
int Analog_Input_Read_Property(
    BACNET_READ_PROPERTY_DATA * rpdata)
{
    int len = 0;
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    ANALOG_INPUT_DESCR *CurrentAI;
    unsigned object_index = 0;
    unsigned i = 0;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    object_index = Analog_Input_Instance_To_Index(rpdata->object_instance);
    if (object_index < MAX_ANALOG_INPUTS)
        CurrentAI = &AI_Descr[object_index];
    else
        return BACNET_STATUS_ERROR;

    apdu = rpdata->application_data;
    switch ((int) rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_ANALOG_INPUT,
                rpdata->object_instance);
            break;

        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Analog_Input_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ANALOG_INPUT);
            break;

        case PROP_PRESENT_VALUE:
            apdu_len =
                encode_application_real(&apdu[0],
                Analog_Input_Present_Value(rpdata->object_instance));
            break;

        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
#if defined(INTRINSIC_REPORTING)
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM,
                                CurrentAI->Event_State ? true : false);
#else
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
#endif
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, CurrentAI->Out_Of_Service);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_EVENT_STATE:
#if defined(INTRINSIC_REPORTING)
            apdu_len = encode_application_enumerated(&apdu[0],
                                CurrentAI->Event_State);
#else
            apdu_len = encode_application_enumerated(&apdu[0],
                                EVENT_STATE_NORMAL);
#endif
            break;

        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(&apdu[0], CurrentAI->Reliability);
            break;

        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0], CurrentAI->Out_Of_Service);
            break;

        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0], CurrentAI->Units);
            break;

#if defined(INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            apdu_len = encode_application_unsigned(&apdu[0], CurrentAI->Time_Delay);
            break;

        case PROP_NOTIFICATION_CLASS:
            apdu_len = encode_application_unsigned(&apdu[0], CurrentAI->Notification_Class);
            break;

        case PROP_HIGH_LIMIT:
            apdu_len = encode_application_real(&apdu[0], CurrentAI->High_Limit);
            break;

        case PROP_LOW_LIMIT:
            apdu_len = encode_application_real(&apdu[0], CurrentAI->Low_Limit);
            break;

        case PROP_DEADBAND:
            apdu_len = encode_application_real(&apdu[0], CurrentAI->Deadband);
            break;

        case PROP_LIMIT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, 0,
                    (CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE ) ? true : false );
            bitstring_set_bit(&bit_string, 1,
                    (CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ? true : false );

            apdu_len = encode_application_bitstring(&apdu[0],&bit_string);
            break;

        case PROP_EVENT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                    (CurrentAI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true : false );
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                    (CurrentAI->Event_Enable & EVENT_ENABLE_TO_FAULT    ) ? true : false );
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                    (CurrentAI->Event_Enable & EVENT_ENABLE_TO_NORMAL   ) ? true : false );

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_ACKED_TRANSITIONS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                              CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                              CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                              CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_NOTIFY_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0],
                    CurrentAI->Notify_Type ? NOTIFY_EVENT : NOTIFY_ALARM);
            break;

        case PROP_EVENT_TIME_STAMPS:
            /* Array element zero is the number of elements in the array */
            if (rpdata->array_index == 0)
                apdu_len = encode_application_unsigned(&apdu[0],
                                MAX_BACNET_EVENT_TRANSITION);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet. */
            else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                for (i = 0; i < MAX_BACNET_EVENT_TRANSITION; i++) {;
                    len  = encode_opening_tag(&apdu[apdu_len],
                                TIME_STAMP_DATETIME);
                    len += encode_application_date(&apdu[apdu_len + len],
                                &CurrentAI->Event_Time_Stamps[i].date);
                    len += encode_application_time(&apdu[apdu_len + len],
                                &CurrentAI->Event_Time_Stamps[i].time);
                    len += encode_closing_tag(&apdu[apdu_len + len],
                                TIME_STAMP_DATETIME);

                    /* add it if we have room */
                    if ((apdu_len + len) < MAX_APDU)
                        apdu_len += len;
                    else {
                        rpdata->error_class = ERROR_CLASS_SERVICES;
                        rpdata->error_code  = ERROR_CODE_NO_SPACE_FOR_OBJECT;
                        apdu_len = BACNET_STATUS_ERROR;
                        break;
                    }
                }
            }
            else if (rpdata->array_index <= MAX_BACNET_EVENT_TRANSITION) {
                apdu_len  = encode_opening_tag(&apdu[apdu_len],
                                TIME_STAMP_DATETIME);
                apdu_len += encode_application_date(&apdu[apdu_len],
                                &CurrentAI->Event_Time_Stamps[rpdata->array_index].date);
                apdu_len += encode_application_time(&apdu[apdu_len],
                                &CurrentAI->Event_Time_Stamps[rpdata->array_index].time);
                apdu_len += encode_closing_tag(&apdu[apdu_len],
                                TIME_STAMP_DATETIME);
            }
            else {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code  = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif

        case 9997:
            /* test case for real encoding-decoding unsigned value correctly */
            apdu_len = encode_application_real(&apdu[0], 90.510F);
            break;
        case 9998:
            /* test case for unsigned encoding-decoding unsigned value correctly */
            apdu_len = encode_application_unsigned(&apdu[0], 90);
            break;
        case 9999:
            /* test case for signed encoding-decoding negative value correctly */
            apdu_len = encode_application_signed(&apdu[0], -200);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_EVENT_TIME_STAMPS) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code  = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Analog_Input_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    ANALOG_INPUT_DESCR *CurrentAI;

    /* decode the some of the request */
    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code  = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }

    object_index = Analog_Input_Instance_To_Index(wp_data->object_instance);
    if (object_index < MAX_ANALOG_INPUTS)
        CurrentAI = &AI_Descr[object_index];
    else
        return false;

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_REAL,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                if (CurrentAI->Out_Of_Service == true) {
                    Analog_Input_Present_Value_Set(wp_data->object_instance,
                                                   value.type.Real);
                }
                else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_WRITE_ACCESS_DENIED;
                    status = false;
                }
            }
            break;

        case PROP_OUT_OF_SERVICE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                CurrentAI->Out_Of_Service = value.type.Boolean;
            }
            break;

        case PROP_UNITS:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_ENUMERATED,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                CurrentAI->Units = value.type.Enumerated;
            }
            break;

#if defined(INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAI->Time_Delay = value.type.Unsigned_Int;
                CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
            }
            break;

        case PROP_NOTIFICATION_CLASS:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAI->Notification_Class = value.type.Unsigned_Int;
            }
            break;

        case PROP_HIGH_LIMIT:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_REAL,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAI->High_Limit = value.type.Real;
            }
            break;

        case PROP_LOW_LIMIT:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_REAL,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAI->Low_Limit = value.type.Real;
            }
            break;

        case PROP_DEADBAND:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_REAL,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAI->Deadband = value.type.Real;
            }
            break;

        case PROP_LIMIT_ENABLE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BIT_STRING,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                if(value.type.Bit_String.bits_used == 2) {
                    CurrentAI->Limit_Enable = value.type.Bit_String.value[0];
                }
                else {
                   wp_data->error_class = ERROR_CLASS_PROPERTY;
                   wp_data->error_code  = ERROR_CODE_VALUE_OUT_OF_RANGE;
                   status = false;
                }
            }
            break;

        case PROP_EVENT_ENABLE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BIT_STRING,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                if(value.type.Bit_String.bits_used == 3) {
                    CurrentAI->Event_Enable = value.type.Bit_String.value[0];
                }
                else {
                   wp_data->error_class = ERROR_CLASS_PROPERTY;
                   wp_data->error_code  = ERROR_CODE_VALUE_OUT_OF_RANGE;
                   status = false;
                }
            }
            break;

        case PROP_NOTIFY_TYPE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_ENUMERATED,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                if(value.type.Bit_String.bits_used > NOTIFY_EVENT) {
                    CurrentAI->Event_Enable = value.type.Enumerated;
                }
                else {
                   wp_data->error_class = ERROR_CLASS_PROPERTY;
                   wp_data->error_code  = ERROR_CODE_VALUE_OUT_OF_RANGE;
                   status = false;
                }
            }
            break;
#endif

        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code  = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return status;
}


void Analog_Input_Intrinsic_Reporting(uint32_t object_instance)
{
#if defined(INTRINSIC_REPORTING)
    BACNET_EVENT_NOTIFICATION_DATA event_data;
    BACNET_CHARACTER_STRING msgText;
    ANALOG_INPUT_DESCR *CurrentAI;
    unsigned int object_index;
    uint8_t FromState;
    uint8_t ToState;
    float ExceededLimit;
    float PresentVal;


    object_index = Analog_Input_Instance_To_Index(object_instance);
    if (object_index < MAX_ANALOG_INPUTS)
        CurrentAI = &AI_Descr[object_index];
    else
        return;

    /* check limits */
    if (!CurrentAI->Limit_Enable)
        return;  /* limits are not configured */

    /* actual Present_Value */
    PresentVal = Analog_Input_Present_Value(object_instance);
    FromState  = CurrentAI->Event_State;
    switch (CurrentAI->Event_State)
    {
        case EVENT_STATE_NORMAL:
            /* A TO-OFFNORMAL event is generated under these conditions:
                (a) the Present_Value must exceed the High_Limit for a minimum
                    period of time, specified in the Time_Delay property, and
                (b) the HighLimitEnable flag must be set in the Limit_Enable property, and
                (c) the TO-OFFNORMAL flag must be set in the Event_Enable property. */
            if ((PresentVal > CurrentAI->High_Limit) &&
                ((CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) == EVENT_HIGH_LIMIT_ENABLE) &&
                ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) == EVENT_ENABLE_TO_OFFNORMAL))
            {
                if(!CurrentAI->Remaining_Time_Delay)
                    CurrentAI->Event_State = EVENT_STATE_HIGH_LIMIT;
                else
                    CurrentAI->Remaining_Time_Delay--;
                break;
            }

            /* A TO-OFFNORMAL event is generated under these conditions:
                (a) the Present_Value must exceed the Low_Limit plus the Deadband
                    for a minimum period of time, specified in the Time_Delay property, and
                (b) the LowLimitEnable flag must be set in the Limit_Enable property, and
                (c) the TO-NORMAL flag must be set in the Event_Enable property. */
            if ((PresentVal < CurrentAI->Low_Limit) &&
                ((CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) == EVENT_LOW_LIMIT_ENABLE) &&
                ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) == EVENT_ENABLE_TO_OFFNORMAL))
            {
                if(!CurrentAI->Remaining_Time_Delay)
                    CurrentAI->Event_State = EVENT_STATE_LOW_LIMIT;
                else
                    CurrentAI->Remaining_Time_Delay--;
                break;
            }
            /* value of the object is still in the same event state */
            CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
            break;

        case EVENT_STATE_HIGH_LIMIT:
            /* Once exceeded, the Present_Value must fall below the High_Limit minus
               the Deadband before a TO-NORMAL event is generated under these conditions:
                (a) the Present_Value must fall below the High_Limit minus the Deadband
                    for a minimum period of time, specified in the Time_Delay property, and
                (b) the HighLimitEnable flag must be set in the Limit_Enable property, and
                (c) the TO-NORMAL flag must be set in the Event_Enable property. */
            if ((PresentVal < CurrentAI->High_Limit - CurrentAI->Deadband) &&
                ((CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) == EVENT_HIGH_LIMIT_ENABLE) &&
                ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_NORMAL) == EVENT_ENABLE_TO_NORMAL))
            {
                if(!CurrentAI->Remaining_Time_Delay)
                    CurrentAI->Event_State = EVENT_STATE_NORMAL;
                else
                    CurrentAI->Remaining_Time_Delay--;
                break;
            }
            /* value of the object is still in the same event state */
            CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
            break;

        case EVENT_STATE_LOW_LIMIT:
            /* Once the Present_Value has fallen below the Low_Limit,
               the Present_Value must exceed the Low_Limit plus the Deadband
               before a TO-NORMAL event is generated under these conditions:
                (a) the Present_Value must exceed the Low_Limit plus the Deadband
                    for a minimum period of time, specified in the Time_Delay property, and
                (b) the LowLimitEnable flag must be set in the Limit_Enable property, and
                (c) the TO-NORMAL flag must be set in the Event_Enable property. */
            if ((PresentVal > CurrentAI->Low_Limit + CurrentAI->Deadband) &&
                ((CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) == EVENT_LOW_LIMIT_ENABLE) &&
                ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_NORMAL) == EVENT_ENABLE_TO_NORMAL))
            {
                if(!CurrentAI->Remaining_Time_Delay)
                    CurrentAI->Event_State = EVENT_STATE_NORMAL;
                else
                    CurrentAI->Remaining_Time_Delay--;
                break;
            }
            /* value of the object is still in the same event state */
            CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
            break;

        default:
            return;  /* shouldn't happen */
    }  /* switch (FromState) */

    ToState = CurrentAI->Event_State;

    if (FromState != ToState)
    {
        /* Event_State has changed.
           Need to fill only the basic parameters of this type of event.
           Other parameters will be filled in common function. */

        switch (ToState)
        {
            case EVENT_STATE_HIGH_LIMIT:
                ExceededLimit = CurrentAI->High_Limit;
                characterstring_init_ansi(&msgText,
                                "Goes to high limit");
                break;

            case EVENT_STATE_LOW_LIMIT:
                ExceededLimit = CurrentAI->Low_Limit;
                characterstring_init_ansi(&msgText,
                                "Goes to low limit");
                break;

            case EVENT_STATE_NORMAL:
                if(FromState == EVENT_STATE_HIGH_LIMIT) {
                    ExceededLimit = CurrentAI->High_Limit;
                    characterstring_init_ansi(&msgText,
                                "Back to normal state from high limit");
                }
                else {
                    ExceededLimit = CurrentAI->Low_Limit;
                    characterstring_init_ansi(&msgText,
                                "Back to normal state from low limit");
                }
                break;

            default:
                ExceededLimit = 0;
                break;
        }   /* switch (ToState) */

#if PRINT_ENABLED
        fprintf(stderr, "Event_State for (Analog-Input,%d) goes from %s to %s.\n",
                    object_instance, bactext_event_state_name(FromState),
                    bactext_event_state_name(ToState));

#endif /* PRINT_ENABLED) */

        /* Event Object Identifier */
        event_data.eventObjectIdentifier.type = OBJECT_ANALOG_INPUT;
        event_data.eventObjectIdentifier.instance = object_instance;

        /* Time Stamp */
        event_data.timeStamp.tag = TIME_STAMP_DATETIME;
        Device_getCurrentDateTime(&event_data.timeStamp.value.dateTime);
        /* fill Event_Time_Stamps */
        switch (ToState)
        {
            case EVENT_STATE_HIGH_LIMIT:
            case EVENT_STATE_LOW_LIMIT:
                CurrentAI->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL] =
                                event_data.timeStamp.value.dateTime;
                break;

            case EVENT_STATE_FAULT:
                CurrentAI->Event_Time_Stamps[TRANSITION_TO_FAULT] =
                                event_data.timeStamp.value.dateTime;
                break;

            case EVENT_STATE_NORMAL:
                CurrentAI->Event_Time_Stamps[TRANSITION_TO_NORMAL] =
                                event_data.timeStamp.value.dateTime;
                break;
        }

        /* Notification Class */
        event_data.notificationClass = CurrentAI->Notification_Class;

        /* Event Type */
        event_data.eventType = EVENT_OUT_OF_RANGE;

        /* Message Text */
        event_data.messageText = &msgText;

        /* Notify Type */
        event_data.notifyType = CurrentAI->Notify_Type;

        /* From State */
        event_data.fromState = FromState;

        /* To State */
        event_data.toState   = CurrentAI->Event_State;

        /* Event Values */
        event_data.notificationParams.outOfRange.exceedingValue = PresentVal;

        bitstring_init(&event_data.notificationParams.outOfRange.statusFlags);
        bitstring_set_bit(&event_data.notificationParams.outOfRange.statusFlags,
                    STATUS_FLAG_IN_ALARM, CurrentAI->Event_State ? true : false);
        bitstring_set_bit(&event_data.notificationParams.outOfRange.statusFlags,
                    STATUS_FLAG_FAULT, false);
        bitstring_set_bit(&event_data.notificationParams.outOfRange.statusFlags,
                    STATUS_FLAG_OVERRIDDEN, false);
        bitstring_set_bit(&event_data.notificationParams.outOfRange.statusFlags,
                    STATUS_FLAG_OUT_OF_SERVICE, CurrentAI->Out_Of_Service);

        event_data.notificationParams.outOfRange.deadband = CurrentAI->Deadband;

        event_data.notificationParams.outOfRange.exceededLimit = ExceededLimit;

        /* add data from notification class */
        Notification_Class_common_reporting_function(&event_data);

        /* Ack required */
        if (event_data.ackRequired == true)
        {
            switch (event_data.toState)
            {
                case EVENT_STATE_OFFNORMAL:
                case EVENT_STATE_HIGH_LIMIT:
                case EVENT_STATE_LOW_LIMIT:
                    CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked = false;
                    CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].Time_Stamp =
                                        event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_FAULT:
                    CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked = false;
                    CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].Time_Stamp =
                                        event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_NORMAL:
                    CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked = false;
                    CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].Time_Stamp =
                                        event_data.timeStamp.value.dateTime;
                    break;
            }
        }
    }
#endif /* defined(INTRINSIC_REPORTING) */
}


int Analog_Input_Event_Information(unsigned index,
        BACNET_GET_EVENT_INFORMATION_DATA * getevent_data)
{
#if defined(INTRINSIC_REPORTING)
    bool IsNotAckedTransitions;
    bool IsActiveEvent;
    int  i;


    /* check index */
    if (index < MAX_ANALOG_INPUTS) {
        /* Event_State not equal to NORMAL */
        IsActiveEvent = (AI_Descr[index].Event_State != EVENT_STATE_NORMAL);

        /* Acked_Transitions property, which has at least one of the bits
           (TO-OFFNORMAL, TO-FAULT, TONORMAL) set to FALSE. */
        IsNotAckedTransitions = (AI_Descr[index].Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked == false) |
                                (AI_Descr[index].Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked  == false) |
                                (AI_Descr[index].Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked == false);
    }
    else
        return -1;  /* end of list  */

    if ((IsActiveEvent) || (IsNotAckedTransitions)) {
        /* Object Identifier */
        getevent_data->objectIdentifier.type = OBJECT_ANALOG_INPUT;
        getevent_data->objectIdentifier.instance = Analog_Input_Index_To_Instance(index);
        /* Event State */
        getevent_data->eventState = AI_Descr[index].Event_State;
        /* Acknowledged Transitions */
        bitstring_init(&getevent_data->acknowledgedTransitions);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions, TRANSITION_TO_OFFNORMAL,
                            AI_Descr[index].Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions, TRANSITION_TO_FAULT,
                            AI_Descr[index].Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions, TRANSITION_TO_NORMAL,
                            AI_Descr[index].Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);
        /* Event Time Stamps */
        for (i = 0; i < 3; i++) {
            getevent_data->eventTimeStamps[i].tag = TIME_STAMP_DATETIME;
            getevent_data->eventTimeStamps[i].value.dateTime =
                                    AI_Descr[index].Event_Time_Stamps[i];
        }
        /* Notify Type */
        getevent_data->notifyType = AI_Descr[index].Notify_Type;
        /* Event Enable */
        bitstring_init(&getevent_data->eventEnable);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_OFFNORMAL,
                (AI_Descr[index].Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true : false );
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_FAULT,
                (AI_Descr[index].Event_Enable & EVENT_ENABLE_TO_FAULT    ) ? true : false );
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_NORMAL,
                (AI_Descr[index].Event_Enable & EVENT_ENABLE_TO_NORMAL   ) ? true : false );
        /* Event Priorities */
        Notification_Class_Get_Priorities(AI_Descr[index].Notification_Class,
                getevent_data->eventPriorities);

        return 1;   /* active event */
    }
    else
        return 0;   /* no active event at this index */
#endif /* defined(INTRINSIC_REPORTING) */
}



#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testAnalogInput(
    Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint32_t decoded_instance = 0;
    uint16_t decoded_type = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Analog_Input_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ANALOG_INPUT;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Analog_Input_Read_Property(&rpdata);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == rpdata.object_type);
    ct_test(pTest, decoded_instance == rpdata.object_instance);

    return;
}

#ifdef TEST_ANALOG_INPUT
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Analog Input", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testAnalogInput);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_ANALOG_INPUT */
#endif /* TEST */
