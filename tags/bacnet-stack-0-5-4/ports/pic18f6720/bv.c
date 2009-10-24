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

/* Binary Value Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "config.h"     /* the custom stuff */
#include "wp.h"

#define MAX_BINARY_VALUES 8

static BACNET_BINARY_PV Present_Value[MAX_BINARY_VALUES];

static void Binary_Value_Initialize(
    void)
{
    static bool initialized = false;
    unsigned i;

    if (!initialized) {
        initialized = true;
        for (i = 0; i < MAX_BINARY_VALUES; i++) {
            Present_Value[i] = BINARY_INACTIVE;
        }
    }
}

/* we simply have 0-n object instances. */
bool Binary_Value_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_BINARY_VALUES)
        return true;

    return false;
}

/* we simply have 0-n object instances. */
unsigned Binary_Value_Count(
    void)
{
    return MAX_BINARY_VALUES;
}

/* we simply have 0-n object instances. */
uint32_t Binary_Value_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  */
unsigned Binary_Value_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_BINARY_VALUES;

    if (object_instance < MAX_BINARY_VALUES)
        index = object_instance;

    return index;
}

static BACNET_BINARY_PV Binary_Value_Present_Value(
    uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;

    Binary_Value_Initialize();
    if (object_instance < MAX_BINARY_VALUES) {
        value = Present_Value[object_instance];
    }

    return value;
}

/* note: the object name must be unique within this device */
char *Binary_Value_Name(
    uint32_t object_instance)
{
    static char text_string[16] = "";   /* okay for single thread */

    if (object_instance < MAX_BINARY_VALUES) {
        sprintf(text_string, "BV-%lu", object_instance);
        return text_string;
    }

    return NULL;
}

/* return apdu len, or -1 on error */
int Binary_Value_Encode_Property_APDU(
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
    BACNET_BINARY_PV present_value = BINARY_INACTIVE;
    BACNET_POLARITY polarity = POLARITY_NORMAL;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;

    Binary_Value_Initialize();
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_BINARY_VALUE,
                object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string,
                Binary_Value_Name(object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_BINARY_VALUE);
            break;
        case PROP_PRESENT_VALUE:
            present_value = Binary_Value_Present_Value(object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
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
            apdu_len = encode_application_boolean(&apdu[0], false);
            break;
        case PROP_POLARITY:
            /* FIXME: figure out the polarity */
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
bool Binary_Value_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    unsigned int priority = 0;
    BACNET_BINARY_PV level = BINARY_NULL;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    if (!Binary_Value_Valid_Instance(wp_data->object_instance)) {
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
                priority = wp_data->priority;
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                if (priority && (priority <= BACNET_MAX_PRIORITY) &&
                    (priority != 6 /* reserved */ ) &&
                    (value.type.Enumerated >= MIN_BINARY_PV) &&
                    (value.type.Enumerated <= MAX_BINARY_PV)) {
                    level = value.type.Enumerated;
                    object_index =
                        Binary_Value_Instance_To_Index
                        (wp_data->object_instance);
                    priority--;
                    /* NOTE: this Binary value has no priority array */
                    Present_Value[object_index] = level;
                    /* Note: you could set the physical output here if we
                       are the highest priority.
                       However, if Out of Service is TRUE, then don't set the 
                       physical output. */
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
#if 0
                /* NOTE: this Binary Value has no priority array */
                level = BINARY_NULL;
                object_index =
                    Binary_Value_Instance_To_Index(wp_data->object_instance);
                priority = wp_data->priority;
                if (priority && (priority <= BACNET_MAX_PRIORITY)) {
                    priority--;
                    Binary_Value_Level[object_index][priority] = level;
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
#else
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
#endif
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
#if 0
        case PROP_OUT_OF_SERVICE:
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                object_index =
                    Binary_Value_Instance_To_Index(wp_data->object_instance);
                Binary_Value_Out_Of_Service[object_index] = value.type.Boolean;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
#endif
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

void testBinary_Value(
    Test * pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_BINARY_VALUE;
    uint32_t decoded_instance = 0;
    uint32_t instance = 123;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;


    len =
        Binary_Value_Encode_Property_APDU(&apdu[0], instance,
        PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len =
        decode_object_id(&apdu[len], (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_BINARY_VALUE);
    ct_test(pTest, decoded_instance == instance);

    return;
}

#ifdef TEST_BINARY_VALUE
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Binary_Value", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBinary_Value);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_BINARY_VALUE */
#endif /* TEST */
