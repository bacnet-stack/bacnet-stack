/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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

/* Multi-state Output Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "wp.h"

#define MAX_MULTISTATE_OUTPUTS 4

/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define MULTISTATE_RELINQUISH_DEFAULT 0

/* NULL part of the array */
#define MULTISTATE_NULL (255)
/* how many states? 0-253 is 254 states */
#define MULTISTATE_NUMBER_OF_STATES (254)
/* Here is our Priority Array.*/
static uint8_t
    Multistate_Output_Level[MAX_MULTISTATE_OUTPUTS][BACNET_MAX_PRIORITY];
/* Writable out-of-service allows others to play with our Present Value */
/* without changing the physical output */
static bool Multistate_Output_Out_Of_Service[MAX_MULTISTATE_OUTPUTS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Multistate_Output_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_NUMBER_OF_STATES,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    -1
};

static const int Multistate_Output_Properties_Optional[] = {
    PROP_DESCRIPTION,
    -1
};

static const int Multistate_Output_Properties_Proprietary[] = {
    -1
};

void Multistate_Output_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Multistate_Output_Properties_Required;
    if (pOptional)
        *pOptional = Multistate_Output_Properties_Optional;
    if (pProprietary)
        *pProprietary = Multistate_Output_Properties_Proprietary;

    return;
}

void Multistate_Output_Init(
    void)
{
    unsigned i, j;
    static bool initialized = false;

    if (!initialized) {
        initialized = true;

        /* initialize all the analog output priority arrays to NULL */
        for (i = 0; i < MAX_MULTISTATE_OUTPUTS; i++) {
            for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
                Multistate_Output_Level[i][j] = MULTISTATE_NULL;
            }
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Multistate_Output_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_MULTISTATE_OUTPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Multistate_Output_Count(
    void)
{
    return MAX_MULTISTATE_OUTPUTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Multistate_Output_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Multistate_Output_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_MULTISTATE_OUTPUTS;

    if (object_instance < MAX_MULTISTATE_OUTPUTS)
        index = object_instance;

    return index;
}

static uint32_t Multistate_Output_Present_Value(
    uint32_t object_instance)
{
    uint32_t value = MULTISTATE_RELINQUISH_DEFAULT;
    unsigned index = 0;
    unsigned i = 0;

    Multistate_Output_Init();
    index = Multistate_Output_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (Multistate_Output_Level[index][i] != MULTISTATE_NULL) {
                value = Multistate_Output_Level[index][i];
                break;
            }
        }
    }

    return value;
}

/* note: the object name must be unique within this device */
char *Multistate_Output_Name(
    uint32_t object_instance)
{
    static char text_string[32] = "";   /* okay for single thread */

    if (object_instance < MAX_MULTISTATE_OUTPUTS) {
        sprintf(text_string, "MULTISTATE OUTPUT %u", object_instance);
        return text_string;
    }

    return NULL;
}

/* return apdu len, or -1 on error */
int Multistate_Output_Encode_Property_APDU(
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
    uint32_t present_value = 0;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;

    Multistate_Output_Init();
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0],
                OBJECT_MULTI_STATE_OUTPUT, object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string,
                Multistate_Output_Name(object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0],
                OBJECT_MULTI_STATE_OUTPUT);
            break;
        case PROP_PRESENT_VALUE:
            present_value = Multistate_Output_Present_Value(object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], present_value);
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            object_index =
                Multistate_Output_Instance_To_Index(object_instance);
            state = Multistate_Output_Out_Of_Service[object_index];
            apdu_len = encode_application_boolean(&apdu[0], state);
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
                    Multistate_Output_Instance_To_Index(object_instance);
                for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    if (Multistate_Output_Level[object_index][i] ==
                        MULTISTATE_NULL)
                        len = encode_application_null(&apdu[apdu_len]);
                    else {
                        present_value =
                            Multistate_Output_Level[object_index][i];
                        len =
                            encode_application_unsigned(&apdu[apdu_len],
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
                    Multistate_Output_Instance_To_Index(object_instance);
                if (array_index <= BACNET_MAX_PRIORITY) {
                    if (Multistate_Output_Level[object_index][array_index -
                            1] == MULTISTATE_NULL)
                        apdu_len = encode_application_null(&apdu[0]);
                    else {
                        present_value =
                            Multistate_Output_Level[object_index][array_index -
                            1];
                        apdu_len =
                            encode_application_unsigned(&apdu[0],
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
            present_value = MULTISTATE_RELINQUISH_DEFAULT;
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
        case PROP_NUMBER_OF_STATES:
            apdu_len =
                encode_application_unsigned(&apdu[apdu_len],
                MULTISTATE_NUMBER_OF_STATES);
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
bool Multistate_Output_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    unsigned int priority = 0;
    uint32_t level = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    Multistate_Output_Init();
    if (!Multistate_Output_Valid_Instance(wp_data->object_instance)) {
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
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                priority = wp_data->priority;
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                if (priority && (priority <= BACNET_MAX_PRIORITY) &&
                    (priority != 6 /* reserved */ ) &&
                    (value.type.Unsigned_Int <= MULTISTATE_NUMBER_OF_STATES)) {
                    level = value.type.Unsigned_Int;
                    object_index =
                        Multistate_Output_Instance_To_Index(wp_data->
                        object_instance);
                    priority--;
                    Multistate_Output_Level[object_index][priority] =
                        (uint8_t) level;
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
            } else if (value.tag == BACNET_APPLICATION_TAG_NULL) {
                level = MULTISTATE_NULL;
                object_index =
                    Multistate_Output_Instance_To_Index(wp_data->
                    object_instance);
                priority = wp_data->priority;
                if (priority && (priority <= BACNET_MAX_PRIORITY)) {
                    priority--;
                    Multistate_Output_Level[object_index][priority] =
                        (uint8_t) level;
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
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                object_index =
                    Multistate_Output_Instance_To_Index(wp_data->
                    object_instance);
                Multistate_Output_Out_Of_Service[object_index] =
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

void testMultistateOutput(
    Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_MULTI_STATE_OUTPUT;
    uint32_t decoded_instance = 0;
    uint32_t instance = 123;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;


    len =
        Multistate_Output_Encode_Property_APDU(&apdu[0], instance,
        PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len =
        decode_object_id(&apdu[len], (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_MULTI_STATE_OUTPUT);
    ct_test(pTest, decoded_instance == instance);

    return;
}

#ifdef TEST_MULTISTATE_OUTPUT
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Multi-state Output", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testMultistateOutput);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_BINARY_INPUT */
#endif /* TEST */
