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
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/cov.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/services.h"

#ifndef MAX_BINARY_INPUTS
#define MAX_BINARY_INPUTS 5
#endif

/* stores the current value */
static BACNET_BINARY_PV Present_Value[MAX_BINARY_INPUTS];
/* out of service decouples physical input from Present_Value */
static bool Out_Of_Service[MAX_BINARY_INPUTS];
/* Change of Value flag */
static bool Change_Of_Value[MAX_BINARY_INPUTS];
/* Polarity of Input */
static BACNET_POLARITY Polarity[MAX_BINARY_INPUTS];
/* stores the object name */
static char* Object_Name[MAX_BINARY_INPUTS];
/* stores the description */
static char* Description[MAX_BINARY_INPUTS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Binary_Input_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_STATUS_FLAGS,
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, PROP_POLARITY, -1 };

static const int Binary_Input_Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int Binary_Input_Properties_Proprietary[] = { -1 };

void Binary_Input_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
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
bool Binary_Input_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_BINARY_INPUTS) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Binary_Input_Count(void)
{
    return MAX_BINARY_INPUTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Binary_Input_Index_To_Instance(unsigned index)
{
    return index;
}

void Binary_Input_Init(void)
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
unsigned Binary_Input_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_BINARY_INPUTS;

    if (object_instance < MAX_BINARY_INPUTS) {
        index = object_instance;
    }

    return index;
}

BACNET_BINARY_PV Binary_Input_Present_Value(uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        value = Present_Value[index];
        if (Polarity[index] != POLARITY_NORMAL) {
            if (value == BINARY_INACTIVE) {
                value = BINARY_ACTIVE;
            } else {
                value = BINARY_INACTIVE;
            }
        }
    }

    return value;
}

bool Binary_Input_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        value = Out_Of_Service[index];
    }

    return value;
}

bool Binary_Input_Change_Of_Value(uint32_t object_instance)
{
    bool status = false;
    unsigned index;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        status = Change_Of_Value[index];
    }

    return status;
}

void Binary_Input_Change_Of_Value_Clear(uint32_t object_instance)
{
    unsigned index;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        Change_Of_Value[index] = false;
    }

    return;
}

/**
 * For a given object instance-number, loads the value_list with the COV data.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value_list - list of COV data
 *
 * @return  true if the value list is encoded
 */
bool Binary_Input_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;

    if (value_list) {
        value_list->propertyIdentifier = PROP_PRESENT_VALUE;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
        value_list->value.next = NULL;
        value_list->value.type.Enumerated =
            Binary_Input_Present_Value(object_instance);
        value_list->priority = BACNET_NO_PRIORITY;
        value_list = value_list->next;
    }
    if (value_list) {
        value_list->propertyIdentifier = PROP_STATUS_FLAGS;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
        value_list->value.next = NULL;
        bitstring_init(&value_list->value.type.Bit_String);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_IN_ALARM, false);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_FAULT, false);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_OVERRIDDEN, false);
        if (Binary_Input_Out_Of_Service(object_instance)) {
            bitstring_set_bit(&value_list->value.type.Bit_String,
                STATUS_FLAG_OUT_OF_SERVICE, true);
        } else {
            bitstring_set_bit(&value_list->value.type.Bit_String,
                STATUS_FLAG_OUT_OF_SERVICE, false);
        }
        value_list->priority = BACNET_NO_PRIORITY;
        value_list->next = NULL;
        status = true;
    }

    return status;
}

bool Binary_Input_Present_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    unsigned index = 0;
    bool status = false;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        if (Polarity[index] != POLARITY_NORMAL) {
            if (value == BINARY_INACTIVE) {
                value = BINARY_ACTIVE;
            } else {
                value = BINARY_INACTIVE;
            }
        }
        if (Present_Value[index] != value) {
            Change_Of_Value[index] = true;
        }
        Present_Value[index] = value;
        status = true;
    }

    return status;
}

void Binary_Input_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        if (Out_Of_Service[index] != value) {
            Change_Of_Value[index] = true;
        }
        Out_Of_Service[index] = value;
    }

    return;
}

bool Binary_Input_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = ""; /* okay for single thread */
    bool status = false;
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        if (Object_Name[index] == NULL) {
            sprintf(
                text_string, "BINARY INPUT %lu", (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text_string);
        } else {
            status = characterstring_init_ansi(object_name, Object_Name[index]);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the object-name
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 *
 * @return  true if object-name was set
 */
bool Binary_Input_Name_Set(uint32_t object_instance, char *new_name)
{
    bool status = false;
    if (object_instance < MAX_BINARY_INPUTS && new_name) {
        status = true;
        Object_Name[object_instance] = new_name;
    }

    return status;
}

BACNET_POLARITY Binary_Input_Polarity(uint32_t object_instance)
{
    BACNET_POLARITY polarity = POLARITY_NORMAL;
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        polarity = Polarity[index];
    }

    return polarity;
}

bool Binary_Input_Polarity_Set(
    uint32_t object_instance, BACNET_POLARITY polarity)
{
    bool status = false;
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_INPUTS) {
        Polarity[index] = polarity;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
char *Binary_Input_Description(uint32_t object_instance)
{
    char *name = NULL;

    if (object_instance < MAX_BINARY_INPUTS) {
        if (Description[object_instance] == NULL) {
            name = "";
        } else {
            name = Description[object_instance];
        }
    }

    return name;
}

/**
 * @brief For a given object instance-number, sets the description
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if object-name was set
 */
bool Binary_Input_Description_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */

    if (object_instance < MAX_BINARY_INPUTS && new_name) {
        status = true;
        Description[object_instance] = new_name;
    }

    return status;
}

/* return apdu length, or BACNET_STATUS_ERROR on error */
/* assumption - object already exists, and has been bounds checked */
int Binary_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
    bool state = false;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_BINARY_INPUT, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            /* note: object name must be unique in our device */
            Binary_Input_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string,
                Binary_Input_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_BINARY_INPUT);
            break;
        case PROP_PRESENT_VALUE:
            /* note: you need to look up the actual value */
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Input_Present_Value(rpdata->object_instance));
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Binary_Input_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Binary_Input_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_POLARITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Input_Polarity(rpdata->object_instance));
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Binary_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    /* decode the some of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /*  only array properties can have array options */
    if (wp_data->array_index != BACNET_ARRAY_ALL) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= MAX_BINARY_PV) {
                    Binary_Input_Present_Value_Set(wp_data->object_instance,
                        (BACNET_BINARY_PV)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Binary_Input_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_POLARITY:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated < MAX_POLARITY) {
                    Binary_Input_Polarity_Set(wp_data->object_instance,
                        (BACNET_POLARITY)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}
