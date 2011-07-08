/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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

/* Analog Value Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "wp.h"
#include "rp.h"
#include "nc.h"
#include "av.h"
#include "device.h"
#include "handlers.h"

#ifndef MAX_ANALOG_VALUES
#define MAX_ANALOG_VALUES 4
#endif

/* we choose to have a NULL level in our system represented by */
/* a particular value.  When the priorities are not in use, they */
/* will be relinquished (i.e. set to the NULL level). */
#define ANALOG_LEVEL_NULL 255



ANALOG_VALUE_DESCR AV_Descr[MAX_ANALOG_VALUES];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Analog_Value_Properties_Required[] = {
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

static const int Analog_Value_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
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

static const int Analog_Value_Properties_Proprietary[] = {
    -1
};

void Analog_Value_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Analog_Value_Properties_Required;
    if (pOptional)
        *pOptional = Analog_Value_Properties_Optional;
    if (pProprietary)
        *pProprietary = Analog_Value_Properties_Proprietary;

    return;
}

void Analog_Value_Init(
    void)
{
    unsigned i, j;

    for (i = 0; i < MAX_ANALOG_VALUES; i++) {
        memset(&AV_Descr[i], 0x00, sizeof(ANALOG_VALUE_DESCR));
        /* initialize all the analog output priority arrays to NULL */
        for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
            AV_Descr[i].Priority_Array[j] = ANALOG_LEVEL_NULL;
        }
        AV_Descr[i].Units = UNITS_PERCENT;
#if defined(INTRINSIC_REPORTING)
        AV_Descr[i].Event_State = EVENT_STATE_NORMAL;
        /* notification class not connected */
        AV_Descr[i].Notification_Class = BACNET_MAX_INSTANCE;
        /* initialize Event time stamps using wildcards */
        for (j = 0; j < MAX_BACNET_EVENT_TRANSITION; j++) {
            datetime_wildcard_set(&AV_Descr[i].Event_Time_Stamps[j]);
        }
#endif
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Value_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_ANALOG_VALUES)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Analog_Value_Count(
    void)
{
    return MAX_ANALOG_VALUES;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Analog_Value_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Analog_Value_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_ANALOG_VALUES;

    if (object_instance < MAX_ANALOG_VALUES)
        index = object_instance;

    return index;
}

bool Analog_Value_Present_Value_Set(
    uint32_t object_instance,
    float value,
    uint8_t priority)
{
    ANALOG_VALUE_DESCR *CurrentAV;
    unsigned index = 0;
    bool status = false;

    index = Analog_Value_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_VALUES) {
        CurrentAV = &AV_Descr[index];
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ ) &&
            (value >= 0.0) && (value <= 100.0))
        {
            CurrentAV->Priority_Array[priority - 1] = (uint8_t) value;
            /* Note: you could set the physical output here to the next
               highest priority, or to the relinquish default if no
               priorities are set.
               However, if Out of Service is TRUE, then don't set the
               physical output.  This comment may apply to the
               main loop (i.e. check out of service before changing output) */
            status = true;
        }
    }
    return status;
}


float Analog_Value_Present_Value(
    uint32_t object_instance)
{
    ANALOG_VALUE_DESCR *CurrentAV;
    float value = 0;
    unsigned index = 0;
    unsigned i = 0;

    index = Analog_Value_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_VALUES) {
        CurrentAV = &AV_Descr[index];
        /* When all the priorities are level null, the present value returns */
        /* the Relinquish Default value */
        value = CurrentAV->Relinquish_Default;
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (CurrentAV->Priority_Array[i] != ANALOG_LEVEL_NULL) {
                value = CurrentAV->Priority_Array[i];
                break;
            }
        }
    }

    return value;
}

/* note: the object name must be unique within this device */
bool Analog_Value_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = "";   /* okay for single thread */
    bool status = false;

    if (object_instance < MAX_ANALOG_VALUES) {
        sprintf(text_string, "ANALOG VALUE %lu",
            (unsigned long) object_instance);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Analog_Value_Read_Property(
    BACNET_READ_PROPERTY_DATA * rpdata)
{
    int len = 0;
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    float real_value = (float) 1.414;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;
    uint8_t *apdu = NULL;
    ANALOG_VALUE_DESCR *CurrentAV;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    apdu = rpdata->application_data;

    object_index = Analog_Value_Instance_To_Index(rpdata->object_instance);
    if (object_index < MAX_ANALOG_VALUES)
        CurrentAV = &AV_Descr[object_index];
    else
        return BACNET_STATUS_ERROR;

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_ANALOG_VALUE,
                rpdata->object_instance);
            break;

        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Analog_Value_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ANALOG_VALUE);
            break;

        case PROP_PRESENT_VALUE:
            real_value = Analog_Value_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;

        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
#if defined(INTRINSIC_REPORTING)
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM,
                                CurrentAV->Event_State ? true : false);
#else
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
#endif
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, CurrentAV->Out_Of_Service);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_EVENT_STATE:
#if defined(INTRINSIC_REPORTING)
            apdu_len = encode_application_enumerated(&apdu[0],
                                CurrentAV->Event_State);
#else
            apdu_len = encode_application_enumerated(&apdu[0],
                                EVENT_STATE_NORMAL);
#endif
            break;

        case PROP_OUT_OF_SERVICE:
            state = CurrentAV->Out_Of_Service;
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;

        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0], CurrentAV->Units);
            break;

        case PROP_PRIORITY_ARRAY:
            /* Array element zero is the number of elements in the array */
            if (rpdata->array_index == 0)
                apdu_len =
                    encode_application_unsigned(&apdu[0], BACNET_MAX_PRIORITY);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet. */
            else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    if (CurrentAV->Priority_Array[i] == ANALOG_LEVEL_NULL)
                        len = encode_application_null(&apdu[apdu_len]);
                    else {
                        real_value = CurrentAV->Priority_Array[i];
                        len =
                            encode_application_real(&apdu[apdu_len],
                            real_value);
                    }
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
            } else {
                if (rpdata->array_index <= BACNET_MAX_PRIORITY) {
                    if (CurrentAV->Priority_Array[rpdata->array_index - 1]
                            == ANALOG_LEVEL_NULL)
                        apdu_len = encode_application_null(&apdu[0]);
                    else {
                        real_value = CurrentAV->Priority_Array
                            [rpdata->array_index - 1];
                        apdu_len =
                            encode_application_real(&apdu[0], real_value);
                    }
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;

        case PROP_RELINQUISH_DEFAULT:
            real_value = CurrentAV->Relinquish_Default;
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;

#if defined(INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            apdu_len = encode_application_unsigned(&apdu[0], CurrentAV->Time_Delay);
            break;

        case PROP_NOTIFICATION_CLASS:
            apdu_len = encode_application_unsigned(&apdu[0], CurrentAV->Notification_Class);
            break;

        case PROP_HIGH_LIMIT:
            apdu_len = encode_application_real(&apdu[0], CurrentAV->High_Limit);
            break;

        case PROP_LOW_LIMIT:
            apdu_len = encode_application_real(&apdu[0], CurrentAV->Low_Limit);
            break;

        case PROP_DEADBAND:
            apdu_len = encode_application_real(&apdu[0], CurrentAV->Deadband);
            break;

        case PROP_LIMIT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, 0,
                    (CurrentAV->Limit_Enable & EVENT_LOW_LIMIT_ENABLE ) ? true : false );
            bitstring_set_bit(&bit_string, 1,
                    (CurrentAV->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ? true : false );

            apdu_len = encode_application_bitstring(&apdu[0],&bit_string);
            break;

        case PROP_EVENT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                    (CurrentAV->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true : false );
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                    (CurrentAV->Event_Enable & EVENT_ENABLE_TO_FAULT    ) ? true : false );
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                    (CurrentAV->Event_Enable & EVENT_ENABLE_TO_NORMAL   ) ? true : false );

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_ACKED_TRANSITIONS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL, true);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,     true);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,    true);

            /// Fixme: finish it

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_NOTIFY_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0],
                    CurrentAV->Notify_Type ? NOTIFY_EVENT : NOTIFY_ALARM);
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
                                &CurrentAV->Event_Time_Stamps[i].date);
                    len += encode_application_time(&apdu[apdu_len + len],
                                &CurrentAV->Event_Time_Stamps[i].time);
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
                                &CurrentAV->Event_Time_Stamps[rpdata->array_index].date);
                apdu_len += encode_application_time(&apdu[apdu_len],
                                &CurrentAV->Event_Time_Stamps[rpdata->array_index].time);
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

        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->object_property != PROP_EVENT_TIME_STAMPS) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code  = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Analog_Value_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    unsigned int priority = 0;
    uint8_t level = ANALOG_LEVEL_NULL;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    ANALOG_VALUE_DESCR *CurrentAV;

    /* decode the some of the request */
    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }

    object_index = Analog_Value_Instance_To_Index(wp_data->object_instance);
    if (object_index < MAX_ANALOG_VALUES)
        CurrentAV = &AV_Descr[object_index];
    else
        return false;

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                if (Analog_Value_Present_Value_Set(wp_data->object_instance,
                        value.type.Real, wp_data->priority)) {
                    status = true;
                } else if (wp_data->priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                status =
                    WPValidateArgType(&value, BACNET_APPLICATION_TAG_NULL,
                    &wp_data->error_class, &wp_data->error_code);
                if (status) {
                    level = ANALOG_LEVEL_NULL;
                    priority = wp_data->priority;
                    if (priority && (priority <= BACNET_MAX_PRIORITY)) {
                        priority--;
                        CurrentAV->Priority_Array[priority] = level;
                        /* Note: you could set the physical output here to the next
                           highest priority, or to the relinquish default if no
                           priorities are set.
                           However, if Out of Service is TRUE, then don't set the
                           physical output.  This comment may apply to the
                           main loop (i.e. check out of service before changing output) */
                    } else {
                        status = false;
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            }
            break;

        case PROP_OUT_OF_SERVICE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                CurrentAV->Out_Of_Service = value.type.Boolean;
            }
            break;

        case PROP_UNITS:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_ENUMERATED,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                CurrentAV->Units = value.type.Enumerated;
            }
            break;

        case PROP_RELINQUISH_DEFAULT:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_REAL,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                CurrentAV->Relinquish_Default = value.type.Real;
            }
            break;

#if defined(INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAV->Time_Delay = value.type.Unsigned_Int;
                CurrentAV->Remaining_Time_Delay = CurrentAV->Time_Delay;
            }
            break;

        case PROP_NOTIFICATION_CLASS:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAV->Notification_Class = value.type.Unsigned_Int;
            }
            break;

        case PROP_HIGH_LIMIT:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_REAL,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAV->High_Limit = value.type.Real;
            }
            break;

        case PROP_LOW_LIMIT:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_REAL,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAV->Low_Limit = value.type.Real;
            }
            break;

        case PROP_DEADBAND:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_REAL,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                CurrentAV->Deadband = value.type.Real;
            }
            break;

        case PROP_LIMIT_ENABLE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BIT_STRING,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                if(value.type.Bit_String.bits_used == 2) {
                    CurrentAV->Limit_Enable = value.type.Bit_String.value[0];
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
                    CurrentAV->Event_Enable = value.type.Bit_String.value[0];
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
                    CurrentAV->Event_Enable = value.type.Enumerated;
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


void Analog_Value_Intrinsic_Reporting(uint32_t object_instance)
{
#if defined(INTRINSIC_REPORTING)
    BACNET_EVENT_NOTIFICATION_DATA event_data;
    BACNET_CHARACTER_STRING msgText;
    ANALOG_VALUE_DESCR *CurrentAV;
    unsigned int object_index;
    uint8_t FromState;
    uint8_t ToState;
    float ExceededLimit;
    float PresentVal;


    object_index = Analog_Value_Instance_To_Index(object_instance);
    if (object_index < MAX_ANALOG_VALUES)
        CurrentAV = &AV_Descr[object_index];
    else
        return;

    /* check limits */
    if (!CurrentAV->Limit_Enable)
        return;  /* limits are not configured */

    /* actual Present_Value */
    PresentVal = Analog_Value_Present_Value(object_instance);
    FromState  = CurrentAV->Event_State;
    switch (CurrentAV->Event_State)
    {
        case EVENT_STATE_NORMAL:
            /* A TO-OFFNORMAL event is generated under these conditions:
                (a) the Present_Value must exceed the High_Limit for a minimum
                    period of time, specified in the Time_Delay property, and
                (b) the HighLimitEnable flag must be set in the Limit_Enable property, and
                (c) the TO-OFFNORMAL flag must be set in the Event_Enable property. */
            if ((PresentVal > CurrentAV->High_Limit) &&
                ((CurrentAV->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) == EVENT_HIGH_LIMIT_ENABLE) &&
                ((CurrentAV->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) == EVENT_ENABLE_TO_OFFNORMAL))
            {
                if(!CurrentAV->Remaining_Time_Delay)
                    CurrentAV->Event_State = EVENT_STATE_HIGH_LIMIT;
                else
                    CurrentAV->Remaining_Time_Delay--;
                break;
            }

            /* A TO-OFFNORMAL event is generated under these conditions:
                (a) the Present_Value must exceed the Low_Limit plus the Deadband
                    for a minimum period of time, specified in the Time_Delay property, and
                (b) the LowLimitEnable flag must be set in the Limit_Enable property, and
                (c) the TO-NORMAL flag must be set in the Event_Enable property. */
            if ((PresentVal < CurrentAV->Low_Limit) &&
                ((CurrentAV->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) == EVENT_LOW_LIMIT_ENABLE) &&
                ((CurrentAV->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) == EVENT_ENABLE_TO_OFFNORMAL))
            {
                if(!CurrentAV->Remaining_Time_Delay)
                    CurrentAV->Event_State = EVENT_STATE_LOW_LIMIT;
                else
                    CurrentAV->Remaining_Time_Delay--;
                break;
            }
            /* value of the object is still in the same event state */
            CurrentAV->Remaining_Time_Delay = CurrentAV->Time_Delay;
            break;

        case EVENT_STATE_HIGH_LIMIT:
            /* Once exceeded, the Present_Value must fall below the High_Limit minus
               the Deadband before a TO-NORMAL event is generated under these conditions:
                (a) the Present_Value must fall below the High_Limit minus the Deadband
                    for a minimum period of time, specified in the Time_Delay property, and
                (b) the HighLimitEnable flag must be set in the Limit_Enable property, and
                (c) the TO-NORMAL flag must be set in the Event_Enable property. */
            if ((PresentVal < CurrentAV->High_Limit - CurrentAV->Deadband) &&
                ((CurrentAV->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) == EVENT_HIGH_LIMIT_ENABLE) &&
                ((CurrentAV->Event_Enable & EVENT_ENABLE_TO_NORMAL) == EVENT_ENABLE_TO_NORMAL))
            {
                if(!CurrentAV->Remaining_Time_Delay)
                    CurrentAV->Event_State = EVENT_STATE_NORMAL;
                else
                    CurrentAV->Remaining_Time_Delay--;
                break;
            }
            /* value of the object is still in the same event state */
            CurrentAV->Remaining_Time_Delay = CurrentAV->Time_Delay;
            break;

        case EVENT_STATE_LOW_LIMIT:
            /* Once the Present_Value has fallen below the Low_Limit,
               the Present_Value must exceed the Low_Limit plus the Deadband
               before a TO-NORMAL event is generated under these conditions:
                (a) the Present_Value must exceed the Low_Limit plus the Deadband
                    for a minimum period of time, specified in the Time_Delay property, and
                (b) the LowLimitEnable flag must be set in the Limit_Enable property, and
                (c) the TO-NORMAL flag must be set in the Event_Enable property. */
            if ((PresentVal > CurrentAV->Low_Limit + CurrentAV->Deadband) &&
                ((CurrentAV->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) == EVENT_LOW_LIMIT_ENABLE) &&
                ((CurrentAV->Event_Enable & EVENT_ENABLE_TO_NORMAL) == EVENT_ENABLE_TO_NORMAL))
            {
                if(!CurrentAV->Remaining_Time_Delay)
                    CurrentAV->Event_State = EVENT_STATE_NORMAL;
                else
                    CurrentAV->Remaining_Time_Delay--;
                break;
            }
            /* value of the object is still in the same event state */
            CurrentAV->Remaining_Time_Delay = CurrentAV->Time_Delay;
            break;

        default:
            return;  /* shouldn't happen */
    }  /* switch (FromState) */

    ToState = CurrentAV->Event_State;

    if (FromState != ToState)
    {
        /* Event_State has changed.
           Need to fill only the basic parameters of this type of event.
           Other parameters will be filled in common function. */

        switch (ToState)
        {
            case EVENT_STATE_HIGH_LIMIT:
                ExceededLimit = CurrentAV->High_Limit;
                characterstring_init_ansi(&msgText,
                                "Goes to high limit");
                break;

            case EVENT_STATE_LOW_LIMIT:
                ExceededLimit = CurrentAV->Low_Limit;
                characterstring_init_ansi(&msgText,
                                "Goes to low limit");
                break;

            case EVENT_STATE_NORMAL:
                if(FromState == EVENT_STATE_HIGH_LIMIT) {
                    ExceededLimit = CurrentAV->High_Limit;
                    characterstring_init_ansi(&msgText,
                                "Back to normal state from high limit");
                }
                else {
                    ExceededLimit = CurrentAV->Low_Limit;
                    characterstring_init_ansi(&msgText,
                                "Back to normal state from low limit");
                }
                break;

            default:
                ExceededLimit = 0;
                break;
        }   /* switch (ToState) */


        /* Event Object Identifier */
        event_data.eventObjectIdentifier.type = OBJECT_ANALOG_VALUE;
        event_data.eventObjectIdentifier.instance = object_instance;

        /* Time Stamp */
        event_data.timeStamp.tag = TIME_STAMP_DATETIME;
        Device_getCurrentDateTime(&event_data.timeStamp.value.dateTime);
        /* fill Event_Time_Stamps */
        switch (ToState)
        {
            case EVENT_STATE_HIGH_LIMIT:
            case EVENT_STATE_LOW_LIMIT:
                CurrentAV->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL] =
                                event_data.timeStamp.value.dateTime;
                break;

            case EVENT_STATE_FAULT:
                CurrentAV->Event_Time_Stamps[TRANSITION_TO_FAULT] =
                                event_data.timeStamp.value.dateTime;
                break;

            case EVENT_STATE_NORMAL:
                CurrentAV->Event_Time_Stamps[TRANSITION_TO_NORMAL] =
                                event_data.timeStamp.value.dateTime;
                break;
        }

        /* Notification Class */
        event_data.notificationClass = CurrentAV->Notification_Class;

        /* Event Type */
        event_data.eventType = EVENT_OUT_OF_RANGE;

        /* Message Text */
        event_data.messageText = &msgText;

        /* Notify Type */
        event_data.notifyType = CurrentAV->Notify_Type;

        /* From State */
        event_data.fromState = FromState;

        /* To State */
        event_data.toState   = CurrentAV->Event_State;

        /* Event Values */
        event_data.notificationParams.outOfRange.exceedingValue = PresentVal;

        bitstring_init(&event_data.notificationParams.outOfRange.statusFlags);
        bitstring_set_bit(&event_data.notificationParams.outOfRange.statusFlags,
                    STATUS_FLAG_IN_ALARM, CurrentAV->Event_State ? true : false);
        bitstring_set_bit(&event_data.notificationParams.outOfRange.statusFlags,
                    STATUS_FLAG_FAULT, false);
        bitstring_set_bit(&event_data.notificationParams.outOfRange.statusFlags,
                    STATUS_FLAG_OVERRIDDEN, false);
        bitstring_set_bit(&event_data.notificationParams.outOfRange.statusFlags,
                    STATUS_FLAG_OUT_OF_SERVICE, CurrentAV->Out_Of_Service);

        event_data.notificationParams.outOfRange.deadband = CurrentAV->Deadband;

        event_data.notificationParams.outOfRange.exceededLimit = ExceededLimit;

        /* add data from notification class */
        Notification_Class_common_reporting_function(&event_data);
    }
#endif /* defined(INTRINSIC_REPORTING) */
}



#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

bool WPValidateArgType(
    BACNET_APPLICATION_DATA_VALUE * pValue,
    uint8_t ucExpectedTag,
    BACNET_ERROR_CLASS * pErrorClass,
    BACNET_ERROR_CODE * pErrorCode)
{
    pValue=pValue;
    ucExpectedTag=ucExpectedTag;
    pErrorClass=pErrorClass;
    pErrorCode=pErrorCode;

    return false;
}

void testAnalog_Value(
    Test * pTest)
{
    BACNET_READ_PROPERTY_DATA rpdata;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint16_t decoded_type = 0;
    uint32_t decoded_instance = 0;

    Analog_Value_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ANALOG_VALUE;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Analog_Value_Read_Property(&rpdata);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == rpdata.object_type);
    ct_test(pTest, decoded_instance == rpdata.object_instance);

    return;
}

#ifdef TEST_ANALOG_VALUE
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Analog Value", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testAnalog_Value);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_ANALOG_VALUE */
#endif /* TEST */
