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

/* Analog Input Objects customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "config.h"

/* Analog Input */
#ifndef MAX_ANALOG_INPUTS
#define MAX_ANALOG_INPUTS 2
#endif

static uint8_t Present_Value[MAX_ANALOG_INPUTS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Analog_Input_Properties_Required[] = {
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

static const int Analog_Input_Properties_Optional[] = {
    PROP_DESCRIPTION,
    -1
};

static const int Analog_Input_Properties_Proprietary[] = {
    -1
};

void Analog_Input_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Analog_Input_Properties_Required;
    if (pOptional)
        *pOptional = Analog_Input_Properties_Optional;
    if (pProprietary)
        *pProprietary = Analog_Input_Properties_Proprietary;

    return;
}

void Analog_Input_Init(
    void)
{
    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Input_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_ANALOG_INPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances. */
unsigned Analog_Input_Count(
    void)
{
    return MAX_ANALOG_INPUTS;
}

/* we simply have 0-n object instances. */
uint32_t Analog_Input_Index_To_Instance(
    unsigned index)
{
    return index;
}

char *Analog_Input_Name(
    uint32_t object_instance)
{
    static char text_string[32];        /* okay for single thread */

    if (object_instance < MAX_ANALOG_INPUTS) {
        sprintf(text_string, "AI-%lu", object_instance);
        return text_string;
    }

    return NULL;
}

float Analog_Input_Present_Value(
    uint32_t object_instance)
{
    float value = 0.0;

    if (object_instance < MAX_ANALOG_INPUTS) {
        value = Present_Value[object_instance];
    }

    return value;
}

void Analog_Input_Present_Value_Set(
    uint32_t object_instance,
    float value)
{
    if (object_instance < MAX_ANALOG_INPUTS) {
        Present_Value[object_instance] = value;
    }
}

/* return apdu length, or -1 on error */
/* assumption - object has already exists */
int Analog_Input_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;

    (void) array_index;
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_ANALOG_INPUT,
                object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string,
                Analog_Input_Name(object_instance));
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
                Analog_Input_Present_Value(object_instance));
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
            apdu_len = encode_application_boolean(&apdu[0], false);
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0], UNITS_PERCENT);
            break;
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }

    return apdu_len;
}
