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

/* Binary Output Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "config.h"     /* the custom stuff */
#include "wp.h"
#include "led.h"
#include "nvdata.h"

#ifndef MAX_BINARY_OUTPUTS
#define MAX_BINARY_OUTPUTS 2
#endif

/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define RELINQUISH_DEFAULT BINARY_INACTIVE
/* Here is our Priority Array.*/
static uint8_t Binary_Output_Level[MAX_BINARY_OUTPUTS][BACNET_MAX_PRIORITY];
/* Writable out-of-service allows others to play with our Present Value */
/* without changing the physical output */
static uint8_t Out_Of_Service[MAX_BINARY_OUTPUTS];
/* polarity - normal or inverse */
static uint8_t Polarity[MAX_BINARY_OUTPUTS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Binary_Output_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_POLARITY,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    -1
};

static const int Binary_Output_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_ACTIVE_TEXT,
    PROP_INACTIVE_TEXT,
    -1
};

static const int Binary_Output_Properties_Proprietary[] = {
    -1
};

void Binary_Output_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Binary_Output_Properties_Required;
    if (pOptional)
        *pOptional = Binary_Output_Properties_Optional;
    if (pProprietary)
        *pProprietary = Binary_Output_Properties_Proprietary;

    return;
}

void Binary_Output_Level_Set(
    unsigned int object_index,
    unsigned int priority,
    BACNET_BINARY_PV level)
{
    if (object_index < MAX_BINARY_OUTPUTS) {
        if (priority < BACNET_MAX_PRIORITY) {
            Binary_Output_Level[object_index][priority] = (uint8_t) level;
            seeprom_bytes_write(NV_SEEPROM_BINARY_OUTPUT(object_index,
                    NV_SEEPROM_BO_PRIORITY_ARRAY_1 + priority),
                &Binary_Output_Level[object_index][priority], 1);
        }
    }
}

void Binary_Output_Polarity_Set(
    unsigned int object_index,
    BACNET_POLARITY polarity)
{
    if (object_index < MAX_BINARY_OUTPUTS) {
        if (polarity < MAX_POLARITY) {
            Polarity[object_index] = POLARITY_NORMAL;
            seeprom_bytes_write(NV_SEEPROM_BINARY_OUTPUT(object_index,
                    NV_SEEPROM_BO_POLARITY), &Polarity[object_index], 1);
        }
    }
}

void Binary_Output_Out_Of_Service_Set(
    unsigned int object_index,
    bool flag)
{
    if (object_index < MAX_BINARY_OUTPUTS) {
        Out_Of_Service[object_index] = flag;
        seeprom_bytes_write(NV_SEEPROM_BINARY_OUTPUT(object_index,
                NV_SEEPROM_BO_OUT_OF_SERVICE), &Out_Of_Service[object_index],
            1);
    }
}

/* we simply have 0-n object instances. */
bool Binary_Output_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_BINARY_OUTPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances. */
unsigned Binary_Output_Count(
    void)
{
    return MAX_BINARY_OUTPUTS;
}

/* we simply have 0-n object instances. */
uint32_t Binary_Output_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  */
unsigned Binary_Output_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_BINARY_OUTPUTS;

    if (object_instance < MAX_BINARY_OUTPUTS)
        index = object_instance;

    return index;
}

static BACNET_BINARY_PV Present_Value(
    unsigned int index)
{
    BACNET_BINARY_PV value = RELINQUISH_DEFAULT;
    BACNET_BINARY_PV current_value = RELINQUISH_DEFAULT;
    unsigned i = 0;

    if (index < MAX_BINARY_OUTPUTS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            current_value = Binary_Output_Level[index][i];
            if (current_value != BINARY_NULL) {
                value = Binary_Output_Level[index][i];
                break;
            }
        }
    }

    return value;
}

static BACNET_BINARY_PV Binary_Output_Present_Value(
    uint32_t object_instance)
{
    unsigned index = 0;

    index = Binary_Output_Instance_To_Index(object_instance);

    return Present_Value(index);
}

void Binary_Output_Level_Sync(
    unsigned int index)
{
    BACNET_BINARY_PV pv;

    if (index < MAX_BINARY_OUTPUTS) {
        if (Out_Of_Service[index]) {
            return;
        }
        pv = Present_Value(index);
        if (Polarity[index] == POLARITY_REVERSE) {
            if (pv == BINARY_INACTIVE) {
                pv = BINARY_ACTIVE;
            } else if (pv == BINARY_ACTIVE) {
                pv = BINARY_INACTIVE;
            }
        }
        switch (index) {
            case 0:
                if (pv == BINARY_INACTIVE) {
                    led_off(LED_3);
                } else if (pv == BINARY_ACTIVE) {
                    led_on(LED_3);
                }
                break;
            case 1:
                if (pv == BINARY_INACTIVE) {
                    led_off(LED_4);
                } else if (pv == BINARY_ACTIVE) {
                    led_on(LED_4);
                }
                break;
            default:
                break;
        }
    }
}

/* note: the object name must be unique within this device */
char *Binary_Output_Name(
    uint32_t object_instance)
{
    static char text_string[32];        /* okay for single thread */

    if (object_instance < MAX_BINARY_OUTPUTS) {
        sprintf(text_string, "BO-%lu", object_instance);
        return text_string;
    }

    return NULL;
}

/* return apdu len, or -1 on error */
int Binary_Output_Encode_Property_APDU(
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
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;

    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_BINARY_OUTPUT,
                object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string,
                Binary_Output_Name(object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_BINARY_OUTPUT);
            break;
        case PROP_PRESENT_VALUE:
            present_value = Binary_Output_Present_Value(object_instance);
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
            object_index = Binary_Output_Instance_To_Index(object_instance);
            state = Out_Of_Service[object_index];
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_POLARITY:
            apdu_len =
                encode_application_enumerated(&apdu[0],
                Polarity[object_index]);
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
                    Binary_Output_Instance_To_Index(object_instance);
                for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    present_value = Binary_Output_Level[object_index][i];
                    if (present_value == BINARY_NULL) {
                        len = encode_application_null(&apdu[apdu_len]);
                    } else {
                        len =
                            encode_application_enumerated(&apdu[apdu_len],
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
                    present_value =
                        Binary_Output_Level[object_index][array_index - 1];
                    if (present_value == BINARY_NULL) {
                        apdu_len = encode_application_null(&apdu[apdu_len]);
                    } else {
                        apdu_len =
                            encode_application_enumerated(&apdu[apdu_len],
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
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
        case PROP_ACTIVE_TEXT:
            characterstring_init_ansi(&char_string, "on");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_INACTIVE_TEXT:
            characterstring_init_ansi(&char_string, "off");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
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
bool Binary_Output_Write_Property(
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

    object_index = Binary_Output_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_BINARY_OUTPUTS) {
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
                    (value.type.Enumerated <= MAX_BINARY_PV)) {
                    level = (BACNET_BINARY_PV) value.type.Enumerated;
                    priority--;
                    Binary_Output_Level_Set(object_index, priority, level);
                    Binary_Output_Level_Sync(object_index);
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
                level = BINARY_NULL;
                priority = wp_data->priority;
                if (priority && (priority <= BACNET_MAX_PRIORITY)) {
                    priority--;
                    Binary_Output_Level_Set(object_index, priority, level);
                    Binary_Output_Level_Sync(object_index);
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
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_OUT_OF_SERVICE:
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                Binary_Output_Out_Of_Service_Set(object_index,
                    value.type.Boolean);
                Binary_Output_Level_Sync(object_index);
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_POLARITY:
            if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                if (value.type.Enumerated < MAX_POLARITY) {
                    Binary_Output_Polarity_Set(object_index,
                        value.type.Enumerated);
                    Binary_Output_Level_Sync(object_index);
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
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return status;
}

void Binary_Output_Init(
    void)
{
    unsigned i, j;

    /* initialize all the analog output priority arrays to NULL */
    for (i = 0; i < MAX_BINARY_OUTPUTS; i++) {
        seeprom_bytes_read(NV_SEEPROM_BINARY_OUTPUT(i, NV_SEEPROM_BO_POLARITY),
            &Polarity[i], 1);
        if (Polarity[i] >= MAX_POLARITY) {
            Binary_Output_Polarity_Set(i, POLARITY_NORMAL);
        }
        seeprom_bytes_read(NV_SEEPROM_BINARY_OUTPUT(i,
                NV_SEEPROM_BO_OUT_OF_SERVICE), &Out_Of_Service[i], 1);
        if (Out_Of_Service[i] > 1) {
            Binary_Output_Out_Of_Service_Set(i, false);
        }
        for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
            seeprom_bytes_read(NV_SEEPROM_BINARY_OUTPUT(i,
                    NV_SEEPROM_BO_PRIORITY_ARRAY_1 + j),
                &Binary_Output_Level[i][j], 1);
        }
        Binary_Output_Level_Sync(i);
    }

    return;
}
