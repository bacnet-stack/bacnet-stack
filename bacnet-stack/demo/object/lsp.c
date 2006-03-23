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

/* Life Safety Point Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "config.h"             /* the custom stuff */
#include "wp.h"

#define MAX_LIFE_SAFETY_POINTS 7

/* Here are our stored levels.*/
static BACNET_LIFE_SAFETY_MODE
    Life_Safety_Point_Mode[MAX_LIFE_SAFETY_POINTS];
static BACNET_LIFE_SAFETY_STATE
    Life_Safety_Point_State[MAX_LIFE_SAFETY_POINTS];
static BACNET_SILENCED_STATE
    Life_Safety_Point_Silenced_State[MAX_LIFE_SAFETY_POINTS];
static BACNET_LIFE_SAFETY_OPERATION
    Life_Safety_Point_Operation[MAX_LIFE_SAFETY_POINTS];
/* Writable out-of-service allows others to play with our Present Value */
/* without changing the physical output */
static bool Life_Safety_Point_Out_Of_Service[MAX_LIFE_SAFETY_POINTS];

void Life_Safety_Point_Init(void)
{
    static bool initialized = false;
    unsigned i;

    if (!initialized) {
        initialized = true;

        /* initialize all the analog output priority arrays to NULL */
        for (i = 0; i < MAX_LIFE_SAFETY_POINTS; i++) {
            Life_Safety_Point_Mode[i] = LIFE_SAFETY_MODE_DEFAULT;
            Life_Safety_Point_State[i] = LIFE_SAFETY_STATE_QUIET;
            Life_Safety_Point_Silenced_State[i] = SILENCED_STATE_UNSILENCED;
            Life_Safety_Point_Operation[i] = LIFE_SAFETY_OPERATION_NONE;
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Life_Safety_Point_Valid_Instance(uint32_t object_instance)
{
    Life_Safety_Point_Init();
    if (object_instance < MAX_LIFE_SAFETY_POINTS)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Life_Safety_Point_Count(void)
{
    Life_Safety_Point_Init();
    return MAX_LIFE_SAFETY_POINTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Life_Safety_Point_Index_To_Instance(unsigned index)
{
    Life_Safety_Point_Init();
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Life_Safety_Point_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_LIFE_SAFETY_POINTS;

    Life_Safety_Point_Init();
    if (object_instance < MAX_LIFE_SAFETY_POINTS)
        index = object_instance;

    return index;
}

static BACNET_LIFE_SAFETY_STATE Life_Safety_Point_Present_Value(uint32_t object_instance)
{
    BACNET_LIFE_SAFETY_STATE present_value = LIFE_SAFETY_STATE_QUIET;
    unsigned index = 0;

    Life_Safety_Point_Init();
    index = Life_Safety_Point_Instance_To_Index(object_instance);
    if (index < MAX_LIFE_SAFETY_POINTS)
        present_value = Life_Safety_Point_State[index];

    return present_value;
}

/* note: the object name must be unique within this device */
char *Life_Safety_Point_Name(uint32_t object_instance)
{
    static char text_string[32] = "";   /* okay for single thread */

    if (object_instance < MAX_LIFE_SAFETY_POINTS) {
        sprintf(text_string, "LS POINT %u", object_instance);
        return text_string;
    }

    return NULL;
}

/* return apdu len, or -1 on error */
int Life_Safety_Point_Encode_Property_APDU(uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    int len = 0;
    int apdu_len = 0;           /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_LIFE_SAFETY_STATE present_value = LIFE_SAFETY_STATE_QUIET;
    BACNET_LIFE_SAFETY_MODE mode = LIFE_SAFETY_MODE_DEFAULT;
    BACNET_SILENCED_STATE silenced_state = SILENCED_STATE_UNSILENCED;
    BACNET_LIFE_SAFETY_OPERATION operation = LIFE_SAFETY_OPERATION_NONE;
    unsigned object_index = 0;
    bool state = false;
    BACNET_RELIABILITY reliability = RELIABILITY_NO_FAULT_DETECTED;

    Life_Safety_Point_Init();
    switch (property) {
    case PROP_OBJECT_IDENTIFIER:
        apdu_len = encode_tagged_object_id(&apdu[0], OBJECT_LIFE_SAFETY_POINT,
            object_instance);
        break;
    case PROP_OBJECT_NAME:
    case PROP_DESCRIPTION:
        characterstring_init_ansi(&char_string,
            Life_Safety_Point_Name(object_instance));
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_OBJECT_TYPE:
        apdu_len =
            encode_tagged_enumerated(&apdu[0], OBJECT_LIFE_SAFETY_POINT);
        break;
    case PROP_PRESENT_VALUE:
        present_value = Life_Safety_Point_Present_Value(object_instance);
        apdu_len = encode_tagged_enumerated(&apdu[0], present_value);
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
        object_index = Life_Safety_Point_Instance_To_Index(object_instance);
        state = Life_Safety_Point_Out_Of_Service[object_index];
        apdu_len = encode_tagged_boolean(&apdu[0], state);
        break;
    case PROP_RELIABILITY:
        /* see standard for details about this property */
        reliability = RELIABILITY_NO_FAULT_DETECTED;
        apdu_len = encode_tagged_enumerated(&apdu[0], reliability);
        break;
    case PROP_MODE:
        object_index = Life_Safety_Point_Instance_To_Index(object_instance);
        mode = Life_Safety_Point_Mode[object_index];
        apdu_len = encode_tagged_enumerated(&apdu[0], mode);
        break;
    case PROP_ACCEPTED_MODES:
        for (mode = MIN_LIFE_SAFETY_MODE; mode < MAX_LIFE_SAFETY_MODE; mode++)
        {
          len = encode_tagged_enumerated(&apdu[apdu_len], mode);
          apdu_len += len;
        }
        break;
    case PROP_SILENCED:
        object_index = Life_Safety_Point_Instance_To_Index(object_instance);
        silenced_state = Life_Safety_Point_Silenced_State[object_index];
        apdu_len = encode_tagged_enumerated(&apdu[0], silenced_state);
        break;
    case PROP_OPERATION_EXPECTED:
        object_index = Life_Safety_Point_Instance_To_Index(object_instance);
        operation = Life_Safety_Point_Operation[object_index];
        apdu_len = encode_tagged_enumerated(&apdu[0], operation);
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
bool Life_Safety_Point_Write_Property(BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;

    Life_Safety_Point_Init();
    if (!Life_Safety_Point_Valid_Instance(wp_data->object_instance)) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */
    switch (wp_data->object_property) {
    case PROP_MODE:
        if (wp_data->value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
          if ((wp_data->value.type.Enumerated >= MIN_LIFE_SAFETY_MODE) &&
                (wp_data->value.type.Enumerated <= MIN_LIFE_SAFETY_MODE)) {
                object_index = Life_Safety_Point_Instance_To_Index(wp_data->object_instance);
                Life_Safety_Point_Mode[object_index] = wp_data->value.type.Enumerated;
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
                Life_Safety_Point_Instance_To_Index(wp_data->object_instance);
            Life_Safety_Point_Out_Of_Service[object_index] =
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

void testAnalogOutput(Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_LIFE_SAFETY_POINT;
    uint32_t decoded_instance = 0;
    uint32_t instance = 123;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;


    len = Life_Safety_Point_Encode_Property_APDU(&apdu[0],
        instance,
        PROP_OBJECT_IDENTIFIER,
        BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len],
        (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_LIFE_SAFETY_POINT);
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
