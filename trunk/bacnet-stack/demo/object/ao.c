/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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

/* Analog Output Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"             /* the custom stuff */
#include "wp.h"

#define MAX_ANALOG_OUTPUTS 4

/* we choose to have a NULL level in our system represented by */
/* a particular value.  When the priorities are not in use, they */
/* will be relinquished (i.e. set to the NULL level). */
#define AO_LEVEL_NULL 255
/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define AO_RELINQUISH_DEFAULT 0
/* Here is our Priority Array.  They are supposed to be Real, but */
/* we don't have that kind of memory, so we will use a single byte */
/* and load a Real for returning the value when asked. */
static uint8_t
    Analog_Output_Level[MAX_ANALOG_OUTPUTS][BACNET_MAX_PRIORITY];
/* Writable out-of-service allows others to play with our Present Value */
/* without changing the physical output */
static bool Analog_Output_Out_Of_Service[MAX_ANALOG_OUTPUTS];

/* we need to have our arrays initialized before answering any calls */
static bool Analog_Output_Initialized = false;

void Analog_Output_Init(void)
{
    unsigned i, j;

    if (!Analog_Output_Initialized) {
        Analog_Output_Initialized = true;

        /* initialize all the analog output priority arrays to NULL */
        for (i = 0; i < MAX_ANALOG_OUTPUTS; i++) {
            for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
                Analog_Output_Level[i][j] = AO_LEVEL_NULL;
            }
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Output_Valid_Instance(uint32_t object_instance)
{
    Analog_Output_Init();
    if (object_instance < MAX_ANALOG_OUTPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Analog_Output_Count(void)
{
    Analog_Output_Init();
    return MAX_ANALOG_OUTPUTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Analog_Output_Index_To_Instance(unsigned index)
{
    Analog_Output_Init();
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Analog_Output_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_ANALOG_OUTPUTS;

    Analog_Output_Init();
    if (object_instance < MAX_ANALOG_OUTPUTS)
        index = object_instance;

    return index;
}

float Analog_Output_Present_Value(uint32_t object_instance)
{
    float value = AO_RELINQUISH_DEFAULT;
    unsigned index = 0;
    unsigned i = 0;

    Analog_Output_Init();
    index = Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (Analog_Output_Level[index][i] != AO_LEVEL_NULL) {
                value = Analog_Output_Level[index][i];
                break;
            }
        }
    }

    return value;
}

unsigned Analog_Output_Present_Value_Priority(uint32_t object_instance)
{
    unsigned index = 0;         /* instance to index conversion */
    unsigned i = 0;             /* loop counter */
    unsigned priority = 0;      /* return value */

    Analog_Output_Init();
    index = Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (Analog_Output_Level[index][i] != AO_LEVEL_NULL) {
                priority = i + 1;
                break;
            }
        }
    }

    return priority;
}

bool Analog_Output_Present_Value_Set(uint32_t object_instance,
    float value, unsigned priority)
{
    unsigned index = 0;
    bool status = false;

    index = Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_OUTPUTS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ ) &&
            (value >= 0.0) && (value <= 100.0)) {
            Analog_Output_Level[index][priority] = (uint8_t) value;
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

bool Analog_Output_Present_Value_Relinquish(uint32_t object_instance,
    int priority)
{
    unsigned index = 0;
    bool status = false;

    index = Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_ANALOG_OUTPUTS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ )) {
            Analog_Output_Level[index][priority] = AO_LEVEL_NULL;
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

/* note: the object name must be unique within this device */
char *Analog_Output_Name(uint32_t object_instance)
{
    static char text_string[32] = "";   /* okay for single thread */

    if (object_instance < MAX_ANALOG_OUTPUTS) {
        sprintf(text_string, "ANALOG OUTPUT %u", object_instance);
        return text_string;
    }

    return NULL;
}

/* return apdu len, or -1 on error */
int Analog_Output_Encode_Property_APDU(uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    int len = 0;
    int apdu_len = 0;           /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    float real_value = (float) 1.414;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;

    Analog_Output_Init();
    switch (property) {
    case PROP_OBJECT_IDENTIFIER:
        apdu_len = encode_tagged_object_id(&apdu[0], OBJECT_ANALOG_OUTPUT,
            object_instance);
        break;
    case PROP_OBJECT_NAME:
    case PROP_DESCRIPTION:
        characterstring_init_ansi(&char_string,
            Analog_Output_Name(object_instance));
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_OBJECT_TYPE:
        apdu_len =
            encode_tagged_enumerated(&apdu[0], OBJECT_ANALOG_OUTPUT);
        break;
    case PROP_PRESENT_VALUE:
        real_value = Analog_Output_Present_Value(object_instance);
        apdu_len = encode_tagged_real(&apdu[0], real_value);
        break;
    case PROP_STATUS_FLAGS:
        bitstring_init(&bit_string);
        bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
        bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
        bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
        bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_EVENT_STATE:
        apdu_len = encode_tagged_enumerated(&apdu[0], EVENT_STATE_NORMAL);
        break;
    case PROP_OUT_OF_SERVICE:
        object_index = Analog_Output_Instance_To_Index(object_instance);
        state = Analog_Output_Out_Of_Service[object_index];
        apdu_len = encode_tagged_boolean(&apdu[0], state);
        break;
    case PROP_UNITS:
        apdu_len = encode_tagged_enumerated(&apdu[0], UNITS_PERCENT);
        break;
    case PROP_PRIORITY_ARRAY:
        /* Array element zero is the number of elements in the array */
        if (array_index == 0)
            apdu_len =
                encode_tagged_unsigned(&apdu[0], BACNET_MAX_PRIORITY);
        /* if no index was specified, then try to encode the entire list */
        /* into one packet. */
        else if (array_index == BACNET_ARRAY_ALL) {
            object_index =
                Analog_Output_Instance_To_Index(object_instance);
            for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                /* FIXME: check if we have room before adding it to APDU */
                if (Analog_Output_Level[object_index][i] == AO_LEVEL_NULL)
                    len = encode_tagged_null(&apdu[apdu_len]);
                else {
                    real_value = Analog_Output_Level[object_index][i];
                    len = encode_tagged_real(&apdu[apdu_len], real_value);
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
                Analog_Output_Instance_To_Index(object_instance);
            if (array_index <= BACNET_MAX_PRIORITY) {
                if (Analog_Output_Level[object_index][array_index - 1] ==
                    AO_LEVEL_NULL)
                    apdu_len = encode_tagged_null(&apdu[0]);
                else {
                    real_value =
                        Analog_Output_Level[object_index][array_index - 1];
                    apdu_len = encode_tagged_real(&apdu[0], real_value);
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = -1;
            }
        }

        break;
    case PROP_RELINQUISH_DEFAULT:
        real_value = AO_RELINQUISH_DEFAULT;
        apdu_len = encode_tagged_real(&apdu[0], real_value);
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
bool Analog_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    uint8_t level = AO_LEVEL_NULL;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    Analog_Output_Init();
    if (!Analog_Output_Valid_Instance(wp_data->object_instance)) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */
    len = bacapp_decode_application_data(wp_data->application_data,
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
                Analog_Output_Present_Value_Set(wp_data->object_instance,
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
            level = AO_LEVEL_NULL;
            object_index =
                Analog_Output_Instance_To_Index(wp_data->object_instance);
            status =
                Analog_Output_Present_Value_Relinquish(wp_data->
                object_instance, wp_data->priority);
            if (!status) {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_OUT_OF_SERVICE:
        if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
            object_index =
                Analog_Output_Instance_To_Index(wp_data->object_instance);
            Analog_Output_Out_Of_Service[object_index] =
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

void testAnalogOutput(Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_ANALOG_OUTPUT;
    uint32_t decoded_instance = 0;
    uint32_t instance = 123;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;


    len = Analog_Output_Encode_Property_APDU(&apdu[0],
        instance,
        PROP_OBJECT_IDENTIFIER,
        BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len],
        (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_ANALOG_OUTPUT);
    ct_test(pTest, decoded_instance == instance);

    return;
}

#ifdef TEST_ANALOG_OUTPUT
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Analog Output", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testAnalogOutput);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_ANALOG_INPUT */
#endif                          /* TEST */
