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

/* Binary Output Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "config.h"             /* the custom stuff */
#include "wp.h"

#define MAX_BINARY_OUTPUTS 6

/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define RELINQUISH_DEFAULT BINARY_INACTIVE
/* Here is our Priority Array.*/
static BACNET_BINARY_PV
    Binary_Output_Level[MAX_BINARY_OUTPUTS][BACNET_MAX_PRIORITY];
/* Writable out-of-service allows others to play with our Present Value */
/* without changing the physical output */
static bool Binary_Output_Out_Of_Service[MAX_BINARY_OUTPUTS];

void Binary_Output_Init(void)
{
    unsigned i, j;
    static bool initialized = false;

    if (!initialized) {
        initialized = true;

        /* initialize all the analog output priority arrays to NULL */
        for (i = 0; i < MAX_BINARY_OUTPUTS; i++) {
            for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
                Binary_Output_Level[i][j] = BINARY_NULL;
            }
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Binary_Output_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_BINARY_OUTPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Binary_Output_Count(void)
{
    return MAX_BINARY_OUTPUTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Binary_Output_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Binary_Output_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_BINARY_OUTPUTS;

    if (object_instance < MAX_BINARY_OUTPUTS)
        index = object_instance;

    return index;
}

static float Binary_Output_Present_Value(uint32_t object_instance)
{
    float value = RELINQUISH_DEFAULT;
    unsigned index = 0;
    unsigned i = 0;

    Binary_Output_Init();
    index = Binary_Output_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (Binary_Output_Level[index][i] != BINARY_NULL) {
                value = Binary_Output_Level[index][i];
                break;
            }
        }
    }

    return value;
}

/* note: the object name must be unique within this device */
char *Binary_Output_Name(uint32_t object_instance)
{
    static char text_string[32] = "";   /* okay for single thread */

    if (object_instance < MAX_BINARY_OUTPUTS) {
        sprintf(text_string, "BINARY OUTPUT %u", object_instance);
        return text_string;
    }

    return NULL;
}

/* return apdu len, or -1 on error */
int Binary_Output_Encode_Property_APDU(uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    int len = 0;
    int apdu_len = 0;           /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_BINARY_PV present_value = BINARY_INACTIVE;
    BACNET_POLARITY polarity = POLARITY_NORMAL;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;

    Binary_Output_Init();
    switch (property) {
    case PROP_OBJECT_IDENTIFIER:
        apdu_len = encode_tagged_object_id(&apdu[0], OBJECT_BINARY_OUTPUT,
            object_instance);
        break;
        /* note: Name and Description don't have to be the same.
           You could make Description writable and different */
    case PROP_OBJECT_NAME:
    case PROP_DESCRIPTION:
        characterstring_init_ansi(&char_string,
            Binary_Output_Name(object_instance));
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_OBJECT_TYPE:
        apdu_len =
            encode_tagged_enumerated(&apdu[0], OBJECT_BINARY_OUTPUT);
        break;
    case PROP_PRESENT_VALUE:
        present_value = Binary_Output_Present_Value(object_instance);
        apdu_len = encode_tagged_enumerated(&apdu[0], present_value);
        break;
    case PROP_STATUS_FLAGS:
        /* note: see the details in the standard on how to use these */
        bitstring_init(&bit_string);
        bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
        bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
        bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
        bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_EVENT_STATE:
        /* note: see the details in the standard on how to use this */
        apdu_len = encode_tagged_enumerated(&apdu[0], EVENT_STATE_NORMAL);
        break;
    case PROP_OUT_OF_SERVICE:
        object_index = Binary_Output_Instance_To_Index(object_instance);
        state = Binary_Output_Out_Of_Service[object_index];
        apdu_len = encode_tagged_boolean(&apdu[0], state);
        break;
    case PROP_POLARITY:
        apdu_len = encode_tagged_enumerated(&apdu[0], polarity);
        break;
    case PROP_PRIORITY_ARRAY:
        /* Array element zero is the number of elements in the array */
        if (array_index == BACNET_ARRAY_LENGTH_INDEX)
            apdu_len =
                encode_tagged_unsigned(&apdu[0], BACNET_MAX_PRIORITY);
        /* if no index was specified, then try to encode the entire list */
        /* into one packet. */
        else if (array_index == BACNET_ARRAY_ALL) {
            object_index =
                Binary_Output_Instance_To_Index(object_instance);
            for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                /* FIXME: check if we have room before adding it to APDU */
                if (Binary_Output_Level[object_index][i] == BINARY_NULL)
                    len = encode_tagged_null(&apdu[apdu_len]);
                else {
                    present_value = Binary_Output_Level[object_index][i];
                    len =
                        encode_tagged_enumerated(&apdu[apdu_len],
                        present_value);
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
                Binary_Output_Instance_To_Index(object_instance);
            if (array_index <= BACNET_MAX_PRIORITY) {
                if (Binary_Output_Level[object_index][array_index] ==
                    BINARY_NULL)
                    len = encode_tagged_null(&apdu[apdu_len]);
                else {
                    present_value =
                        Binary_Output_Level[object_index][array_index];
                    len =
                        encode_tagged_enumerated(&apdu[apdu_len],
                        present_value);
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = -1;
            }
        }

        break;
    case PROP_RELINQUISH_DEFAULT:
        present_value = RELINQUISH_DEFAULT;
        apdu_len = encode_tagged_enumerated(&apdu[0], present_value);
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
bool Binary_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    unsigned int priority = 0;
    BACNET_BINARY_PV level = BINARY_NULL;

    Binary_Output_Init();
    if (!Binary_Output_Valid_Instance(wp_data->object_instance)) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */
    switch (wp_data->object_property) {
    case PROP_PRESENT_VALUE:
        if (wp_data->value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
            priority = wp_data->priority;
            /* Command priority 6 is reserved for use by Minimum On/Off
               algorithm and may not be used for other purposes in any
               object. */
            if (priority && (priority <= BACNET_MAX_PRIORITY) &&
                (priority != 6 /* reserved */ ) &&
                (wp_data->value.type.Enumerated >= MIN_BINARY_PV) &&
                (wp_data->value.type.Real <= MAX_BINARY_PV)) {
                level = wp_data->value.type.Enumerated;
                object_index =
                    Binary_Output_Instance_To_Index(wp_data->
                    object_instance);
                priority--;
                Binary_Output_Level[object_index][priority] = level;
                /* Note: you could set the physical output here if we
                   are the highest priority.
                   However, if Out of Service is TRUE, then don't set the 
                   physical output.  This comment may apply to the 
                   main loop (i.e. check out of service before changing output) */
                status = true;
            } else if (priority == 6) {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else if (wp_data->value.tag == BACNET_APPLICATION_TAG_NULL) {
            level = BINARY_NULL;
            object_index =
                Binary_Output_Instance_To_Index(wp_data->object_instance);
            priority = wp_data->priority;
            if (priority && (priority <= BACNET_MAX_PRIORITY)) {
                priority--;
                Binary_Output_Level[object_index][priority] = level;
                /* Note: you could set the physical output here to the next
                   highest priority, or to the relinquish default if no
                   priorities are set.
                   However, if Out of Service is TRUE, then don't set the 
                   physical output.  This comment may apply to the 
                   main loop (i.e. check out of service before changing output) */
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_OUT_OF_SERVICE:
        if (wp_data->value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
            object_index =
                Binary_Output_Instance_To_Index(wp_data->object_instance);
            Binary_Output_Out_Of_Service[object_index] =
                wp_data->value.type.Boolean;
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

void testBinaryOutput(Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_BINARY_OUTPUT;
    uint32_t decoded_instance = 0;
    uint32_t instance = 123;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;


    len = Binary_Output_Encode_Property_APDU(&apdu[0],
        instance,
        PROP_OBJECT_IDENTIFIER,
        BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len],
        (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_BINARY_OUTPUT);
    ct_test(pTest, decoded_instance == instance);

    return;
}

#ifdef TEST_BINARY_OUTPUT
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Binary Output", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBinaryOutput);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_BINARY_INPUT */
#endif                          /* TEST */
