/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
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

/* Multi-state Input Objects */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "wp.h"

#ifndef MAX_MULTISTATE_INPUTS
#define MAX_MULTISTATE_INPUTS 1
#endif

/* how many states? 0-253 is 254 states */
#ifndef MULTISTATE_NUMBER_OF_STATES
#define MULTISTATE_NUMBER_OF_STATES (254)
#endif
/* Here is our Present Value */
static uint8_t Present_Value[MAX_MULTISTATE_INPUTS];
/* Writable out-of-service allows others to manipulate our Present Value */
static bool Out_Of_Service[MAX_MULTISTATE_INPUTS];
static char Object_Name[MAX_MULTISTATE_INPUTS][64];
static char Object_Description[MAX_MULTISTATE_INPUTS][64];
static char State_Text[MAX_MULTISTATE_INPUTS][MULTISTATE_NUMBER_OF_STATES][64];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_NUMBER_OF_STATES,
    -1
};

static const int Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_STATE_TEXT,
    -1
};

static const int Properties_Proprietary[] = {
    -1
};

void Multistate_Input_Property_Lists(
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

void Multistate_Input_Init(
    void)
{
    unsigned i;

    /* initialize all the analog output priority arrays to NULL */
    for (i = 0; i < MAX_MULTISTATE_INPUTS; i++) {
        Present_Value[i] = 0;
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Multistate_Input_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_MULTISTATE_INPUTS;

    if (object_instance < MAX_MULTISTATE_INPUTS)
        index = object_instance;

    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Multistate_Input_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Multistate_Input_Count(
    void)
{
    return MAX_MULTISTATE_INPUTS;
}

bool Multistate_Input_Valid_Instance(
    uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        return true;
    }

    return false;
}

uint32_t Multistate_Input_Present_Value(
    uint32_t object_instance)
{
    uint32_t value = 0;
    unsigned index = 0; /* offset from instance lookup */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        value = Present_Value[index];
    }

    return value;
}

bool Multistate_Input_Present_Value_Set(
    uint32_t object_instance,
    uint32_t value)
{
    bool status = false;
    unsigned index = 0; /* offset from instance lookup */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        if (value < MULTISTATE_NUMBER_OF_STATES) {
            Present_Value[index] = value;
            status = true;
        }
    }

    return status;
}

char *Multistate_Input_Description(
    uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */
    char *pName = NULL; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        pName = Object_Description[index];
    }

    return pName;
}

bool Multistate_Input_Description_Set(
    uint32_t object_instance,
    char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    size_t i = 0;       /* loop counter */
    bool status = false;        /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        status = true;
        if (new_name) {
            for (i = 0; i < sizeof(Object_Description[index]); i++) {
                Object_Description[index][i] = new_name[i];
                if (new_name[i] == 0) {
                    break;
                }
            }
        } else {
            for (i = 0; i < sizeof(Object_Description[index]); i++) {
                Object_Description[index][i] = 0;
            }
        }
    }

    return status;
}

char *Multistate_Input_Name(
    uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */
    char *pName = NULL; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        pName = Object_Name[index];
    }

    return pName;
}

/* note: the object name must be unique within this device */
bool Multistate_Input_Name_Set(
    uint32_t object_instance,
    char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    size_t i = 0;       /* loop counter */
    bool status = false;        /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        status = true;
        /* FIXME: check to see if there is a matching name */
        if (new_name) {
            for (i = 0; i < sizeof(Object_Name[index]); i++) {
                Object_Name[index][i] = new_name[i];
                if (new_name[i] == 0) {
                    break;
                }
            }
        } else {
            for (i = 0; i < sizeof(Object_Name[index]); i++) {
                Object_Name[index][i] = 0;
            }
        }
    }

    return status;
}

char *Multistate_Input_State_Text(
    uint32_t object_instance,
    uint32_t state_index)
{
    unsigned index = 0; /* offset from instance lookup */
    char *pName = NULL; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if ((index < MAX_MULTISTATE_INPUTS) &&
        (state_index < MULTISTATE_NUMBER_OF_STATES)) {
        pName = State_Text[index][state_index];
    }

    return pName;
}

/* note: the object name must be unique within this device */
bool Multistate_Input_State_Text_Set(
    uint32_t object_instance,
    uint32_t state_index,
    char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    size_t i = 0;       /* loop counter */
    bool status = false;        /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if ((index < MAX_MULTISTATE_INPUTS) &&
        (state_index < MULTISTATE_NUMBER_OF_STATES)) {
        status = true;
        if (new_name) {
            for (i = 0; i < sizeof(State_Text[index][state_index]); i++) {
                State_Text[index][state_index][i] = new_name[i];
                if (new_name[i] == 0) {
                    break;
                }
            }
        } else {
            for (i = 0; i < sizeof(State_Text[index][state_index]); i++) {
                State_Text[index][state_index][i] = 0;
            }
        }
    }

    return status;;
}


/* return apdu len, or -1 on error */
int Multistate_Input_Encode_Property_APDU(
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

    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0],
                OBJECT_MULTI_STATE_INPUT, object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
            characterstring_init_ansi(&char_string,
                Multistate_Input_Name(object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string,
                Multistate_Input_Description(object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0],
                OBJECT_MULTI_STATE_INPUT);
            break;
        case PROP_PRESENT_VALUE:
            present_value = Multistate_Input_Present_Value(object_instance);
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
            object_index = Multistate_Input_Instance_To_Index(object_instance);
            state = Out_Of_Service[object_index];
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_NUMBER_OF_STATES:
            apdu_len =
                encode_application_unsigned(&apdu[apdu_len],
                MULTISTATE_NUMBER_OF_STATES);
            break;
        case PROP_STATE_TEXT:
            if (array_index == 0) {
                /* Array element zero is the number of elements in the array */
                apdu_len =
                    encode_application_unsigned(&apdu[0],
                    MULTISTATE_NUMBER_OF_STATES);
            } else if (array_index == BACNET_ARRAY_ALL) {
                /* if no index was specified, then try to encode the entire list */
                /* into one packet. */
                object_index =
                    Multistate_Input_Instance_To_Index(object_instance);
                for (i = 0; i < MULTISTATE_NUMBER_OF_STATES; i++) {
                    characterstring_init_ansi(&char_string,
                        Multistate_Input_State_Text(object_instance, i));
                    /* FIXME: this might go beyond MAX_APDU length! */
                    len =
                        encode_application_character_string(&apdu[apdu_len],
                        &char_string);
                    /* add it if we have room */
                    if ((apdu_len + len) < MAX_APDU) {
                        apdu_len += len;
                    } else {
                        *error_class = ERROR_CLASS_SERVICES;
                        *error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
                        apdu_len = -1;
                        break;
                    }
                }
            } else {
                object_index =
                    Multistate_Input_Instance_To_Index(object_instance);
                if (array_index <= MULTISTATE_NUMBER_OF_STATES) {
                    array_index--;
                    characterstring_init_ansi(&char_string,
                        Multistate_Input_State_Text(object_instance,
                            array_index));
                    apdu_len =
                        encode_application_character_string(&apdu[0],
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
bool Multistate_Input_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    Multistate_Input_Init();
    if (!Multistate_Input_Valid_Instance(wp_data->object_instance)) {
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
                if (Out_Of_Service[object_index]) {
                    if (Multistate_Input_Present_Value_Set(wp_data->
                            object_instance, value.type.Unsigned_Int)) {
                        status = true;
                    } else {
                        *error_class = ERROR_CLASS_PROPERTY;
                        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_OUT_OF_SERVICE:
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                object_index =
                    Multistate_Input_Instance_To_Index
                    (wp_data->object_instance);
                Out_Of_Service[object_index] = value.type.Boolean;
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

void testMultistateInput(
    Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_MULTI_STATE_INPUT;
    uint32_t decoded_instance = 0;
    uint32_t instance = 123;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;

    len =
        Multistate_Input_Encode_Property_APDU(&apdu[0], instance,
        PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len =
        decode_object_id(&apdu[len], (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_MULTI_STATE_INPUT);
    ct_test(pTest, decoded_instance == instance);

    return;
}

#ifdef TEST_MULTISTATE_INPUT
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Multi-state Input", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testMultistateInput);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif
#endif /* TEST */
