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

/* Binary Input Objects customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "wp.h"
#include "cov.h"
#include "config.h"     /* the custom stuff */

#define MAX_BINARY_INPUTS 5

/* stores the current value */
static BACNET_BINARY_PV Present_Value[MAX_BINARY_INPUTS];
/* out of service decouples physical input from Present_Value */
static bool Out_Of_Service[MAX_BINARY_INPUTS];
/* Change of Value flag */
static bool Change_Of_Value[MAX_BINARY_INPUTS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Binary_Input_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_POLARITY,
    -1
};

static const int Binary_Input_Properties_Optional[] = {
    PROP_DESCRIPTION,
    -1
};

static const int Binary_Input_Properties_Proprietary[] = {
    -1
};

void Binary_Input_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Binary_Input_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Binary_Input_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Binary_Input_Properties_Proprietary;
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Binary_Input_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_BINARY_INPUTS) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Binary_Input_Count(
    void)
{
    return MAX_BINARY_INPUTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Binary_Input_Index_To_Instance(
    unsigned index)
{
    return index;
}

void Binary_Input_Init(
    void)
{
    static bool initialized = false;
    unsigned i;

    if (!initialized) {
        initialized = true;

        /* initialize all the values */
        for (i = 0; i < MAX_BINARY_INPUTS; i++) {
            Present_Value[i] = BINARY_INACTIVE;
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Binary_Input_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_BINARY_INPUTS;

    Binary_Input_Init();
    if (object_instance < MAX_BINARY_INPUTS) {
        index = object_instance;
    }

    return index;
}

static BACNET_BINARY_PV Binary_Input_Present_Value(
    uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;
    unsigned index = 0;

    Binary_Input_Init();
    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        value = Present_Value[index];
    }

    return value;
}

static bool Binary_Input_Out_Of_Service(
    uint32_t object_instance)
{
    bool value = false;
    unsigned index = 0;

    Binary_Input_Init();
    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        value = Out_Of_Service[index];
    }

    return value;
}

bool Binary_Input_Change_Of_Value(
    uint32_t object_instance)
{
    bool status = false;
    unsigned index;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        status = Change_Of_Value[index];
    }

    return status;
}

void Binary_Input_Change_Of_Value_Clear(
    uint32_t object_instance)
{
    unsigned index;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        Change_Of_Value[index] = false;
    }

    return;
}

bool Binary_Input_Encode_Value_List(
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE * value_list)
{
    value_list->propertyIdentifier = PROP_PRESENT_VALUE;
    value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
    value_list->value.context_specific = false;
    value_list->value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value_list->value.type.Enumerated =
        Binary_Input_Present_Value(object_instance);
    value_list->priority = BACNET_NO_PRIORITY;

    value_list = value_list->next;

    value_list->propertyIdentifier = PROP_STATUS_FLAGS;
    value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
    value_list->value.context_specific = false;
    value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
    bitstring_init(&value_list->value.type.Bit_String);
    bitstring_set_bit(&value_list->value.type.Bit_String, STATUS_FLAG_IN_ALARM,
        false);
    bitstring_set_bit(&value_list->value.type.Bit_String, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(&value_list->value.type.Bit_String,
        STATUS_FLAG_OVERRIDDEN, false);
    if (Binary_Input_Out_Of_Service(object_instance)) {
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OUT_OF_SERVICE, true);
    } else {
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OUT_OF_SERVICE, false);
    }
    value_list->priority = BACNET_NO_PRIORITY;

    return true;
}

static void Binary_Input_Present_Value_Set(
    uint32_t object_instance,
    BACNET_BINARY_PV value)
{
    unsigned index = 0;

    Binary_Input_Init();
    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        if (Present_Value[index] != value) {
            Change_Of_Value[index] = true;
        }
        Present_Value[index] = value;
    }

    return;
}

static void Binary_Input_Out_Of_Service_Set(
    uint32_t object_instance,
    bool value)
{
    unsigned index = 0;

    Binary_Input_Init();
    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        if (Out_Of_Service[index] != value) {
            Change_Of_Value[index] = true;
        }
    }
    Out_Of_Service[index] = value;

    return;
}

char *Binary_Input_Name(
    uint32_t object_instance)
{
    static char text_string[32] = "";   /* okay for single thread */

    Binary_Input_Init();
    if (object_instance < MAX_BINARY_INPUTS) {
        sprintf(text_string, "BINARY INPUT %u", object_instance);
        return text_string;
    }

    return NULL;
}

/* return apdu length, or -1 on error */
/* assumption - object already exists, and has been bounds checked */
int Binary_Input_Encode_Property_APDU(
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
    BACNET_POLARITY polarity = POLARITY_NORMAL;

    (void) array_index;
    Binary_Input_Init();
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_BINARY_INPUT,
                object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            /* note: object name must be unique in our device */
            characterstring_init_ansi(&char_string,
                Binary_Input_Name(object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_BINARY_INPUT);
            break;
        case PROP_PRESENT_VALUE:
            /* note: you need to look up the actual value */
            apdu_len =
                encode_application_enumerated(&apdu[0],
                Binary_Input_Present_Value(object_instance));
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            if (Binary_Input_Out_Of_Service(object_instance)) {
                bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                    true);
            } else {
                bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                    false);
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len =
                encode_application_boolean(&apdu[0],
                Binary_Input_Out_Of_Service(object_instance));
            break;
        case PROP_POLARITY:
            apdu_len = encode_application_enumerated(&apdu[0], polarity);
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
bool Binary_Input_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    Binary_Input_Init();
    if (!Binary_Input_Valid_Instance(wp_data->object_instance)) {
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
            if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                if ((value.type.Enumerated >= MIN_BINARY_PV) &&
                    (value.type.Enumerated <= MAX_BINARY_PV)) {
                    Binary_Input_Present_Value_Set(wp_data->object_instance,
                        (BACNET_BINARY_PV) value.type.Enumerated);
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
                Binary_Input_Out_Of_Service_Set(wp_data->object_instance,
                    value.type.Boolean);
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

void testBinaryInput(
    Test * pTest)
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


    /* FIXME: we should do a lot more testing here... */
    len =
        Binary_Input_Encode_Property_APDU(&apdu[0], instance,
        PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len >= 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len =
        decode_object_id(&apdu[len], (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_BINARY_INPUT);
    ct_test(pTest, decoded_instance == instance);

    return;
}

#ifdef TEST_BINARY_INPUT
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Binary Input", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBinaryInput);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_BINARY_INPUT */
#endif /* TEST */
