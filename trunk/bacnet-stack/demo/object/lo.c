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

/* Lighting Output Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "wp.h"

#define MAX_LIGHTING_OUTPUTS 5

/* we choose to have a NULL level in our system represented by */
/* a particular value.  When the priorities are not in use, they */
/* will be relinquished (i.e. set to the NULL level). */
#define LIGHTING_LEVEL_NULL 255
/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define LIGHTING_RELINQUISH_DEFAULT 0

/* note: although the standard specifies REAL values for some
   of the optional parameters, we represent them interally as
   integers. */
typedef struct LightingCommand {
    BACNET_LIGHTING_OPERATION operation;
    uint8_t level;      /* 0..100 percent, 255=not used */
    uint8_t ramp_rate;  /* 0..100 percent-per-second, 255=not used */
    uint8_t step_increment;     /* 0..100 amount to step, 255=not used */
    uint16_t fade_time; /* 1..65535 seconds to transition, 0=not used */
    uint16_t duration;  /* 1..65535 minutes until relinquish, 0=not used */
} BACNET_LIGHTING_COMMAND;

/* Here is our Priority Array.  They are supposed to be Real, but */
/* we might not have that kind of memory, so we will use a single byte */
/* and load a Real for returning the value when asked. */
static uint8_t
    Lighting_Output_Level[MAX_LIGHTING_OUTPUTS][BACNET_MAX_PRIORITY];
/* The Progress_Value tracks changes such as ramp and fade */
static uint8_t Lighting_Output_Progress[MAX_LIGHTING_OUTPUTS];
/* The minimum and maximum present values are used for clamping */
static uint8_t Lighting_Output_Min_Present_Value[MAX_LIGHTING_OUTPUTS];
static uint8_t Lighting_Output_Max_Present_Value[MAX_LIGHTING_OUTPUTS];
/* Writable out-of-service allows others to play with our Present Value */
/* without changing the physical output */
static bool Lighting_Output_Out_Of_Service[MAX_LIGHTING_OUTPUTS];
/* the lighting command is what we are doing */
static uint8_t Lighting_Command_Priority = 16;
static BACNET_LIGHTING_COMMAND Lighting_Command[MAX_LIGHTING_OUTPUTS];
/* we need to have our arrays initialized before answering any calls */
static bool Lighting_Output_Initialized = false;

int Lighting_Output_Encode_Lighting_Command(
    uint8_t * apdu,
    BACNET_LIGHTING_COMMAND * data)
{
    int apdu_len = 0;   /* total length of the apdu, return value */
    int len = 0;        /* total length of the apdu, return value */
    float real_value = 0.0;
    uint32_t unsigned_value = 0;

    if (apdu) {
        len = encode_context_enumerated(&apdu[apdu_len], 0, data->operation);
        apdu_len += len;
        /* optional level? */
        if (data->level != 255) {
            real_value = data->level;
            len = encode_context_real(&apdu[apdu_len], 1, real_value);
            apdu_len += len;
        }
        /* optional ramp-rate */
        if (data->ramp_rate != 255) {
            real_value = data->ramp_rate;
            len = encode_context_real(&apdu[apdu_len], 2, real_value);
            apdu_len += len;
        }
        /* optional step increment */
        if (data->step_increment != 255) {
            real_value = data->step_increment;
            len = encode_context_real(&apdu[apdu_len], 3, real_value);
            apdu_len += len;
        }
        /* optional fade time */
        if (data->fade_time != 0) {
            real_value = data->fade_time;
            len = encode_context_real(&apdu[apdu_len], 4, real_value);
            apdu_len += len;
        }
        /* optional duration */
        if (data->duration != 0) {
            unsigned_value = data->duration;
            len = encode_context_unsigned(&apdu[apdu_len], 5, unsigned_value);
            apdu_len += len;
        }
    }

    return apdu_len;
}

int Lighting_Output_Decode_Lighting_Command(
    uint8_t * apdu,
    unsigned apdu_max_len,
    BACNET_LIGHTING_COMMAND * data)
{
    int len = 0;
    int apdu_len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    int type = 0;       /* for decoding */
    int property = 0;   /* for decoding */
    uint32_t unsigned_value = 0;
    int i = 0;  /* loop counter */
    float real_value = 0.0;

    /* check for value pointers */
    if (apdu_len && data) {
        /* Tag 0: operation */
        if (!decode_is_context_tag(&apdu[apdu_len], 0))
            return -1;
        len =
            decode_tag_number_and_value(&apdu[apdu_len], &tag_number,
            &len_value_type);
        apdu_len += len;
        len =
            decode_enumerated(&apdu[apdu_len], len_value_type,
            &data->operation);
        apdu_len += len;
        /* Tag 1: level - OPTIONAL */
        if (decode_is_context_tag(&apdu[apdu_len], 1)) {
            len =
                decode_tag_number_and_value(&apdu[apdu_len], &tag_number,
                &len_value_type);
            apdu_len += len;
            len = decode_real(&apdu[apdu_len], &real_value);
            apdu_len += len;
            data->level = real_value;
            /* FIXME: are we going to flag errors in decoding values here? */
        }
        /* FIXME: finish me! */
        /* Tag 2:  */

    }

    return len;
}


void Lighting_Output_Init(
    void)
{
    unsigned i, j;

    if (!Lighting_Output_Initialized) {
        Lighting_Output_Initialized = true;

        /* initialize all the analog output priority arrays to NULL */
        for (i = 0; i < MAX_LIGHTING_OUTPUTS; i++) {
            for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
                Lighting_Output_Level[i][j] = LIGHTING_LEVEL_NULL;
            }
            Lighting_Command[i].operation = BACNET_LIGHTS_STOP;
            Lighting_Output_Out_Of_Service[i] = false;
            Lighting_Output_Progress[i] = LIGHTING_RELINQUISH_DEFAULT;
            Lighting_Output_Min_Present_Value[i] = 0;
            Lighting_Output_Max_Present_Value[i] = 100;
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Lighting_Output_Valid_Instance(
    uint32_t object_instance)
{
    Lighting_Output_Init();
    if (object_instance < MAX_LIGHTING_OUTPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Lighting_Output_Count(
    void)
{
    Lighting_Output_Init();
    return MAX_LIGHTING_OUTPUTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Lighting_Output_Index_To_Instance(
    unsigned index)
{
    Lighting_Output_Init();
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Lighting_Output_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_LIGHTING_OUTPUTS;

    Lighting_Output_Init();
    if (object_instance < MAX_LIGHTING_OUTPUTS)
        index = object_instance;

    return index;
}

float Lighting_Output_Present_Value(
    uint32_t object_instance)
{
    float value = LIGHTING_RELINQUISH_DEFAULT;
    unsigned index = 0;
    unsigned i = 0;

    Lighting_Output_Init();
    index = Lighting_Output_Instance_To_Index(object_instance);
    if (index < MAX_LIGHTING_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (Lighting_Output_Level[index][i] != LIGHTING_LEVEL_NULL) {
                value = Lighting_Output_Level[index][i];
                break;
            }
        }
    }

    return value;
}

unsigned Lighting_Output_Present_Value_Priority(
    uint32_t object_instance)
{
    unsigned index = 0; /* instance to index conversion */
    unsigned i = 0;     /* loop counter */
    unsigned priority = 0;      /* return value */

    Lighting_Output_Init();
    index = Lighting_Output_Instance_To_Index(object_instance);
    if (index < MAX_LIGHTING_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (Lighting_Output_Level[index][i] != LIGHTING_LEVEL_NULL) {
                priority = i + 1;
                break;
            }
        }
    }

    return priority;
}

bool Lighting_Output_Present_Value_Set(
    uint32_t object_instance,
    float value,
    unsigned priority)
{
    unsigned index = 0;
    bool status = false;

    index = Lighting_Output_Instance_To_Index(object_instance);
    if (index < MAX_LIGHTING_OUTPUTS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ ) &&
            (value >= 0.0) && (value <= 100.0)) {
            Lighting_Output_Level[index][priority - 1] = (uint8_t) value;
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

bool Lighting_Output_Present_Value_Relinquish(
    uint32_t object_instance,
    int priority)
{
    unsigned index = 0;
    bool status = false;

    index = Lighting_Output_Instance_To_Index(object_instance);
    if (index < MAX_LIGHTING_OUTPUTS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ )) {
            Lighting_Output_Level[index][priority - 1] = LIGHTING_LEVEL_NULL;
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

float Lighting_Output_Progress_Value(
    uint32_t object_instance)
{
    float value = LIGHTING_RELINQUISH_DEFAULT;
    unsigned index = 0;

    Lighting_Output_Init();
    index = Lighting_Output_Instance_To_Index(object_instance);
    if (index < MAX_LIGHTING_OUTPUTS) {
        value = Lighting_Output_Progress[index];
    }

    return value;
}

/* note: the object name must be unique within this device */
char *Lighting_Output_Name(
    uint32_t object_instance)
{
    static char text_string[32] = "";   /* okay for single thread */

    if (object_instance < MAX_LIGHTING_OUTPUTS) {
        sprintf(text_string, "LIGHTING OUTPUT %u", object_instance);
        return text_string;
    }

    return NULL;
}

/* return apdu len, or -1 on error */
int Lighting_Output_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int len = 0;
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    float real_value = (float) 1.414;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;

    Lighting_Output_Init();
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_LIGHTING_OUTPUT,
                object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            /* object name must be unique in this device. */
            /* FIXME: description could be writable and different than object name */
            characterstring_init_ansi(&char_string,
                Lighting_Output_Name(object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0],
                OBJECT_LIGHTING_OUTPUT);
            break;
        case PROP_PRESENT_VALUE:
            real_value = Lighting_Output_Present_Value(object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_PROGRESS_VALUE:
            real_value = Lighting_Output_Progress_Value(object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_LIGHTING_COMMAND:
            apdu_len =
                Lighting_Output_Encode_Lighting_Command(&apdu[0],
                &Lighting_Command[object_instance]);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            object_index = Lighting_Output_Instance_To_Index(object_instance);
            state = Lighting_Output_Out_Of_Service[object_index];
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0], UNITS_PERCENT);
            break;
        case PROP_PRIORITY_ARRAY:
            /* Array element zero is the number of elements in the array */
            if (array_index == 0)
                apdu_len =
                    encode_application_unsigned(&apdu[0], BACNET_MAX_PRIORITY);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet. */
            else if (array_index == BACNET_ARRAY_ALL) {
                object_index =
                    Lighting_Output_Instance_To_Index(object_instance);
                for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    if (Lighting_Output_Level[object_index][i] ==
                        LIGHTING_LEVEL_NULL)
                        len = encode_application_null(&apdu[apdu_len]);
                    else {
                        real_value = Lighting_Output_Level[object_index][i];
                        len =
                            encode_application_real(&apdu[apdu_len],
                            real_value);
                    }
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
                object_index =
                    Lighting_Output_Instance_To_Index(object_instance);
                if (array_index <= BACNET_MAX_PRIORITY) {
                    if (Lighting_Output_Level[object_index][array_index - 1] ==
                        LIGHTING_LEVEL_NULL)
                        apdu_len = encode_application_null(&apdu[0]);
                    else {
                        real_value =
                            Lighting_Output_Level[object_index][array_index -
                            1];
                        apdu_len =
                            encode_application_real(&apdu[0], real_value);
                    }
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = -1;
                }
            }

            break;
        case PROP_RELINQUISH_DEFAULT:
            real_value = LIGHTING_RELINQUISH_DEFAULT;
            apdu_len = encode_application_real(&apdu[0], real_value);
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
bool Lighting_Output_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    uint8_t level = LIGHTING_LEVEL_NULL;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    Lighting_Output_Init();
    if (!Lighting_Output_Valid_Instance(wp_data->object_instance)) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */
    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    /* FIXME: len == 0: unable to decode? */
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                status =
                    Lighting_Output_Present_Value_Set(wp_data->object_instance,
                    value.type.Real, wp_data->priority);
                if (wp_data->priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else if (!status) {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else if (value.tag == BACNET_APPLICATION_TAG_NULL) {
                level = LIGHTING_LEVEL_NULL;
                object_index =
                    Lighting_Output_Instance_To_Index
                    (wp_data->object_instance);
                status =
                    Lighting_Output_Present_Value_Relinquish
                    (wp_data->object_instance, wp_data->priority);
                if (wp_data->priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else if (!status) {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_LIGHTING_COMMAND:
            /* FIXME: error checking? */
            Lighting_Output_Decode_Lighting_Command(wp_data->application_data,
                wp_data->application_data_len,
                &Lighting_Command[wp_data->object_instance]);
            break;
        case PROP_OUT_OF_SERVICE:
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                object_index =
                    Lighting_Output_Instance_To_Index
                    (wp_data->object_instance);
                Lighting_Output_Out_Of_Service[object_index] =
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

void testLightingOutput(
    Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_LIGHTING_OUTPUT;
    uint32_t decoded_instance = 0;
    uint32_t instance = 123;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;


    len =
        Lighting_Output_Encode_Property_APDU(&apdu[0], instance,
        PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len =
        decode_object_id(&apdu[len], (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_LIGHTING_OUTPUT);
    ct_test(pTest, decoded_instance == instance);

    return;
}

#ifdef TEST_LIGHTING_OUTPUT
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Lighting Output", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testLightingOutput);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_LIGHTING_INPUT */
#endif /* TEST */
