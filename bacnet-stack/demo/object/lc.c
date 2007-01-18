/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
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

/* Load Control Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "config.h"             /* the custom stuff */
#include "lc.h"
#include "wp.h"

/* number of objects */
#define MAX_LOAD_CONTROLS 6

/*  indicates the current load shedding state of the object */
static BACNET_SHED_STATE Present_Value[MAX_LOAD_CONTROLS];

typedef enum BACnetShedLevelType {
  BACNET_SHED_TYPE_PERCENT, /* Unsigned */
  BACNET_SHED_TYPE_LEVEL,  /* Unsigned */
  BACNET_SHED_TYPE_AMOUNT  /* REAL */
} BACNET_SHED_TYPE_LEVEL_TYPE;

/* The shed levels for the LEVEL choice of BACnetShedLevel 
   that have meaning for this particular Load Control object. */
#define MAX_SHED_LEVELS 3
typedef struct {
    BACNET_SHED_LEVEL_TYPE type;
    union {
        unsigned level;
        unsigned percent;
        float amount;
    } value;
} BACNET_SHED_LEVEL;
/* indicates the desired load shedding */
static BACNET_SHED_LEVEL Requested_Shed_Level[MAX_LOAD_CONTROLS];

/* indicates the start of the duty window in which the load controlled 
   by the Load Control object must be compliant with the requested shed. */
static BACNET_DATE_TIME Load_Control_Start_Time[MAX_LOAD_CONTROLS];

/* indicates the duration of the load shed action, 
   starting at Start_Time */
static uint32_t Load_Control_Shed_Duration[MAX_LOAD_CONTROLS];

/* indicates the time window used for load shed accounting */
static uint32_t Load_Control_Duty_Window[MAX_LOAD_CONTROLS];

/* indicates and controls whether the Load Control object is 
   currently enabled to respond to load shed requests.  */
static boolean Load_Control_Enable[MAX_LOAD_CONTROLS];

/* optional: indicates the baseline power consumption value 
   for the sheddable load controlled by this object, 
   if a fixed baseline is used. 
   The units of Full_Duty_Baseline are kilowatts.*/
static float Full_Duty_Baseline[MAX_LOAD_CONTROLS];

/* Indicates the amount of power that the object expects 
   to be able to shed in response to a load shed request. */
static BACNET_SHED_LEVEL Expected_Shed_Level[MAX_LOAD_CONTROLS];

/* Indicates the actual amount of power being shed in response 
   to a load shed request. */
static BACNET_SHED_LEVEL Actual_Shed_Level[MAX_LOAD_CONTROLS];

/* Represents the shed levels for the LEVEL choice of 
   BACnetShedLevel that have meaning for this particular 
   Load Control object.  */
static unsigned Shed_Levels[MAX_LOAD_CONTROLS][MAX_SHED_LEVELS];

/* represents a description of the shed levels that the 
   Load Control object can take on.  It is the same for
   all the load control objects in this example device. */
static const char *Shed_Level_Descriptions[MAX_SHED_LEVELS] = {
  "dim lights 10%",
  "dim lights 20%",
  "dim lights 30%"
};

/* we need to have our arrays initialized before answering any calls */
static bool Load_Control_Initialized = false;

void Load_Control_Init(void)
{
    unsigned i, j;

    if (!Load_Control_Initialized) {
        Load_Control_Initialized = true;
        for (i = 0; i < MAX_LOAD_CONTROLS; i++) {
            /* FIXME: load saved data? */
            Present_Value[i] = BACNET_SHED_INACTIVE;
            Requested_Shed_Level[i].type = BACNET_SHED_TYPE_LEVEL;
            Requested_Shed_Level[i].value.level = 0;
            Load_Control_Start_Time[i].date.year = 2000;
            Load_Control_Start_Time[i].date.month = 1;
            Load_Control_Start_Time[i].date.day = 1;
            Load_Control_Start_Time[i].date.wday = 1;
            Load_Control_Start_Time[i].time.hour = 0;
            Load_Control_Start_Time[i].time.min = 0;
            Load_Control_Start_Time[i].time.sec = 0;
            Load_Control_Start_Time[i].time.hundredths = 0;
            Load_Control_Shed_Duration[i] = 0;
            Load_Control_Duty_Window[i] = 0;
            Load_Control_Enable[i] = true;
            Full_Duty_Baseline[i] = 0.0;
            for (j = 0; j < MAX_SHED_LEVELS; j++) {
                /* FIXME: fake data for lighting application */
                /* The array shall be ordered by increasing shed amount. */
                Shed_Levels[i][j] = 90 - (j * (30/MAX_SHED_LEVELS));
            }
            Expected_Shed_Level[i].type = BACNET_SHED_TYPE_LEVEL;
            Expected_Shed_Level[i].value.level = 0;
            Actual_Shed_Level[i].type = BACNET_SHED_TYPE_LEVEL;
            Actual_Shed_Level[i].value.level = 0;
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Load_Control_Valid_Instance(uint32_t object_instance)
{
    Load_Control_Init();
    if (object_instance < MAX_LOAD_CONTROLS)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Load_Control_Count(void)
{
    Load_Control_Init();
    return MAX_LOAD_CONTROLS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Load_Control_Index_To_Instance(unsigned index)
{
    Load_Control_Init();
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Load_Control_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_LOAD_CONTROLS;

    Load_Control_Init();
    if (object_instance < MAX_LOAD_CONTROLS)
        index = object_instance;

    return index;
}

static BACNET_SHED_STATE Load_Control_Present_Value(uint32_t object_instance)
{
    BACNET_SHED_STATE value = BACNET_SHED_INACTIVE;
    unsigned index = 0;

    Load_Control_Init();
    index = Load_Control_Instance_To_Index(object_instance);
    if (index < MAX_LOAD_CONTROLS) {
        value = Present_Value[index]
    }

    return value;
}

/* note: the object name must be unique within this device */
char *Load_Control_Name(uint32_t object_instance)
{
    static char text_string[32] = "";   /* okay for single thread */

    if (object_instance < MAX_LOAD_CONTROLS) {
        sprintf(text_string, "LOAD CONTROL %u", object_instance);
        return text_string;
    }

    return NULL;
}

/* return apdu len, or -1 on error */
int Load_Control_Encode_Property_APDU(uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    int len = 0;
    int apdu_len = 0;           /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    int enumeration = 0;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;

    Load_Control_Init();
    object_index = Load_Control_Instance_To_Index(object_instance);
    switch (property) {
    case PROP_OBJECT_IDENTIFIER:
        apdu_len = encode_tagged_object_id(&apdu[0], OBJECT_LOAD_CONTROL,
            object_instance);
        break;
    case PROP_OBJECT_NAME:
        characterstring_init_ansi(&char_string,
            Load_Control_Name(object_instance));
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_OBJECT_TYPE:
        apdu_len = encode_tagged_enumerated(&apdu[0], OBJECT_LOAD_CONTROL);
        break;
    /* optional property
    case PROP_DESCRIPTION:
        characterstring_init_ansi(&char_string,"optional description");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    */
    case PROP_PRESENT_VALUE:
        enumeration = Load_Control_Present_Value(object_instance);
        apdu_len = encode_tagged_enumerated(&apdu[0], enumeration);
        break;
    case PROP_STATUS_FLAGS:
        bitstring_init(&bit_string);
        /* IN_ALARM - Logical FALSE (0) if the Event_State property 
           has a value of NORMAL, otherwise logical TRUE (1). */
        bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
        /* FAULT - Logical	TRUE (1) if the Reliability property is 
           present and does not have a value of NO_FAULT_DETECTED, 
           otherwise logical FALSE (0). */
        bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
        /* OVERRIDDEN - Logical TRUE (1) if the point has been 
           overridden by some mechanism local to the BACnet Device, 
           otherwise logical FALSE (0).*/
        bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
        /* OUT_OF_SERVICE - This bit shall always be Logical FALSE (0). */
        bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_EVENT_STATE:
        apdu_len = encode_tagged_enumerated(&apdu[0], EVENT_STATE_NORMAL);
        break;
    case PROP_ENABLE:
        state = Load_Control_Enable[object_index];
        apdu_len = encode_tagged_boolean(&apdu[0], state);
        break;
    case PROP_REQUESTED_SHED_LEVEL:
        switch (Requested_Shed_Level[object_index].type)
        {
            case BACNET_SHED_TYPE_PERCENT:
                apdu_len = encode_context_unsigned(&apdu[0], 0,
                    Requested_Shed_Level[object_index].value.percent);
                break;
            case BACNET_SHED_TYPE_AMOUNT:
                apdu_len = encode_context_real(&apdu[0], 2,
                    Requested_Shed_Level[object_index].value.amount);
                break;
            case BACNET_SHED_TYPE_LEVEL:
            default:
                apdu_len = encode_context_unsigned(&apdu[0], 1,
                    Requested_Shed_Level[object_index].value.level);
                break;
        }
        break;
    case PROP_START_TIME:
        len = encode_tagged_date(&apdu[0], 
            &Load_Control_Start_Time[object_index].date);
        apdu_len = len;
        len = encode_tagged_time(&apdu[apdu_len], 
            &Load_Control_Start_Time[object_index].time);
        apdu_len += len;
        break;      
    case PROP_SHED_DURATION:
        apdu_len = encode_tagged_unsigned(&apdu[0], 
            Load_Control_Shed_Duration[object_index]);
        break;
    case PROP_DUTY_WINDOW:
        apdu_len = encode_tagged_unsigned(&apdu[0], 
            Load_Control_Duty_Window[object_index]);
        break;
    case PROP_ENABLE:
        apdu_len = encode_tagged_boolean(&apdu[0], 
            Load_Control_Enable[object_index]);
        break;
    case PROP_FULL_DUTY_BASELINE: /* optional */
        apdu_len = encode_tagged_real(&apdu[0], 
            Full_Duty_Baseline[object_index]);
        break;
    case PROP_EXPECTED_SHED_LEVEL:
        switch (Expected_Shed_Level[object_index].type)
        {
            case BACNET_SHED_TYPE_PERCENT:
                apdu_len = encode_context_unsigned(&apdu[0], 0,
                    Expected_Shed_Level[object_index].value.percent);
                break;
            case BACNET_SHED_TYPE_AMOUNT:
                apdu_len = encode_context_real(&apdu[0], 2,
                    Expected_Shed_Level[object_index].value.amount);
                break;
            case BACNET_SHED_TYPE_LEVEL:
            default:
                apdu_len = encode_context_unsigned(&apdu[0], 1,
                    Expected_Shed_Level[object_index].value.level);
                break;
        }
        break;
    case PROP_ACTUAL_SHED_LEVEL:
        switch (Actual_Shed_Level[object_index].type)
        {
            case BACNET_SHED_TYPE_PERCENT:
                apdu_len = encode_context_unsigned(&apdu[0], 0,
                    Actual_Shed_Level[object_index].value.percent);
                break;
            case BACNET_SHED_TYPE_AMOUNT:
                apdu_len = encode_context_real(&apdu[0], 2,
                    Actual_Shed_Level[object_index].value.amount);
                break;
            case BACNET_SHED_TYPE_LEVEL:
            default:
                apdu_len = encode_context_unsigned(&apdu[0], 1,
                    Actual_Shed_Level[object_index].value.level);
                break;
        }
        break;
    case PROP_SHED_LEVELS:
        /* Array element zero is the number of elements in the array */
        if (array_index == BACNET_ARRAY_LENGTH_INDEX)
            apdu_len =
                encode_tagged_unsigned(&apdu[0], MAX_SHED_LEVELS);
        /* if no index was specified, then try to encode the entire list */
        /* into one packet. */
        else if (array_index == BACNET_ARRAY_ALL) {
            apdu_len = 0;
            for (i = 0; i < MAX_SHED_LEVELS; i++) {
                /* FIXME: check if we have room before adding it to APDU */
                len = encode_tagged_unsigned(&apdu[apdu_len], Shed_Levels[object_index][i]);
                /* add it if we have room */
                if ((apdu_len + len) < MAX_APDU)
                    apdu_len += len;
                else {
                    *error_class = ERROR_CLASS_SERVICES;
                    *error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
                    apdu_len = -1;
                    break;
                }
            }
        } else {
            if (array_index <= MAX_SHED_LEVELS) {
                apdu_len = encode_tagged_unsigned(&apdu[0], 
                    Shed_Levels[object_index][array_index-1]);
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = -1;
            }
        }
        break;
    case PROP_SHED_LEVEL_DESCRIPTIONS:
        /* Array element zero is the number of elements in the array */
        if (array_index == BACNET_ARRAY_LENGTH_INDEX)
            apdu_len =
                encode_tagged_unsigned(&apdu[0], MAX_SHED_LEVELS);
        /* if no index was specified, then try to encode the entire list */
        /* into one packet. */
        else if (array_index == BACNET_ARRAY_ALL) {
            apdu_len = 0;
            for (i = 0; i < MAX_SHED_LEVELS; i++) {
                /* FIXME: check if we have room before adding it to APDU */
                characterstring_init_ansi(&char_string,
                    Shed_Level_Descriptions[i]);
                len = encode_tagged_character_string(&apdu[apdu_len], 
                    &char_string);
                /* add it if we have room */
                if ((apdu_len + len) < MAX_APDU)
                    apdu_len += len;
                else {
                    *error_class = ERROR_CLASS_SERVICES;
                    *error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
                    apdu_len = -1;
                    break;
                }
            }
        } else {
            if (array_index <= MAX_SHED_LEVELS) {
                characterstring_init_ansi(&char_string,
                    Shed_Level_Descriptions[array_index-1]);
                apdu_len = encode_tagged_character_string(&apdu[0], 
                    &char_string);
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = -1;
            }
        }
        break;
    default:
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
        apdu_len = -1;
        break;
    }

    return apdu_len;
}

/* returns true if successful */
bool Load_Control_Write_Property(BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    unsigned int priority = 0;
    uint8_t level = ANALOG_LEVEL_NULL;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    Load_Control_Init();
    if (!Load_Control_Valid_Instance(wp_data->object_instance)) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, 
        wp_data->application_data_len,
        &value);
    /* FIXME: len < application_data_len: more data? */
    /* FIXME: len == 0: unable to decode? */
    object_index = Load_Control_Instance_To_Index(wp_data->object_instance);
    switch (wp_data->object_property) {
    case PROP_REQUESTED_SHED_LEVEL:
        len = bacapp_decode_context_data(
            wp_data->application_data, 
            wp_data->application_data_len,
            &value, PROP_REQUESTED_SHED_LEVEL);
        if (value.tag == 0) {
            /* percent - Unsigned */
            Requested_Shed_Level[object_index].value.percent = 
                value.type.Unsigned;
            status = true;
        } else if (value.tag == 1) {
            /* level - Unsigned */
            Requested_Shed_Level[object_index].value.level = 
                value.type.Unsigned;
            status = true;
        } else if (value.tag == 2) {
            /* amount - REAL */
            Requested_Shed_Level[object_index].value.amount = 
                value.type.Real;
            status = true;
        } else {
            /* error! */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_START_TIME:
        if (value.tag == BACNET_APPLICATION_TAG_DATE) {
            memcpy(&Load_Control_Start_Time[object_index].date,
                &value.type.Date,sizeof(value.type.Date));
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        if (!status)
            break;
        len = bacapp_decode_application_data(
            wp_data->application_data + len, 
            wp_data->application_data_len - len,
            &value);
        if (len && value.tag == BACNET_APPLICATION_TAG_TIME) {
            memcpy(&Load_Control_Start_Time[object_index].time,
                &value.type.Time,sizeof(value.type.time));
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_SHED_DURATION:
        if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            Load_Control_Shed_Duration[object_index] =
                value.type.Unsigned;
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_DUTY_WINDOW:
        if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            Load_Control_Duty_Window[object_index] =
                value.type.Unsigned;
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_SHED_LEVELS:
        if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            /* re-write the size of the array? */
            if (wp_data->array_index == 0) {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            else if (wp_data->array_index == BACNET_ARRAY_ALL) {
                /* FIXME: write entire array */
                status = true;
            }
            else if (wp_data->array_index <= MAX_SHED_LEVELS) {
                Shed_Levels[object_index][wp_data->array_index-1] = value.type.Unsigned;
                status = true;
            } else {
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_ENABLE:
        if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
            Load_Control_Enable[object_index] =
                value.type.Boolean;
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    default:
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        break;
    }

    return status;
}


#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testLoadControl(Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_ANALOG_VALUE;
    uint32_t decoded_instance = 0;
    uint32_t instance = 123;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;


    len = Load_Control_Encode_Property_APDU(&apdu[0],
        instance,
        PROP_OBJECT_IDENTIFIER,
        BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len],
        (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_ANALOG_VALUE);
    ct_test(pTest, decoded_instance == instance);

    return;
}

#ifdef TEST_LOAD_CONTROL
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Load Control", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testLoadControl);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_ANALOG_VALUE */
#endif                          /* TEST */
