/**
 * @brief This module manages the BACnet Binary Value objects
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 *
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "hardware.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/object/bv.h"

#ifndef MAX_BINARY_VALUES
#define MAX_BINARY_VALUES 10
#endif

#if (MAX_BINARY_VALUES > 10)
#error Modify the Binary_Value_Name to handle multiple digits
#endif

static BACNET_BINARY_PV Present_Value[MAX_BINARY_VALUES];

/* we simply have 0-n object instances. */
bool Binary_Value_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_BINARY_VALUES) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances. */
unsigned Binary_Value_Count(void)
{
    return MAX_BINARY_VALUES;
}

/* we simply have 0-n object instances. */
uint32_t Binary_Value_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  */
unsigned Binary_Value_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_BINARY_VALUES;

    if (object_instance < MAX_BINARY_VALUES) {
        index = object_instance;
    }

    return index;
}

BACNET_BINARY_PV Binary_Value_Present_Value(uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;

    if (object_instance < MAX_BINARY_VALUES) {
        value = Present_Value[object_instance];
    }

    return value;
}

bool Binary_Value_Present_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    bool status = false;
    if (object_instance < MAX_BINARY_VALUES) {
        Present_Value[object_instance] = value;
        status = true;
    }

    return status;
}

/* note: the object name must be unique within this device */
char *Binary_Value_Name(uint32_t object_instance)
{
    static char text_string[5] = "BV-0"; /* okay for single thread */

    if (object_instance < MAX_BINARY_VALUES) {
        text_string[3] = '0' + (uint8_t)object_instance;
        return text_string;
    }

    return NULL;
}

/* return apdu len, or -1 on error */
int Binary_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_BINARY_PV present_value = BINARY_INACTIVE;
    BACNET_POLARITY polarity = POLARITY_NORMAL;
    uint8_t *apdu;

    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_BINARY_VALUE, rpdata->object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
            characterstring_init_ansi(
                &char_string, Binary_Value_Name(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_BINARY_VALUE);
            break;
        case PROP_PRESENT_VALUE:
            present_value = Binary_Value_Present_Value(rpdata->object_instance);
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
bool Binary_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    if (!Binary_Value_Valid_Instance(wp_data->object_instance)) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
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
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                if ((value.type.Enumerated == BINARY_ACTIVE) ||
                    (value.type.Enumerated == BINARY_INACTIVE)) {
                    object_index = Binary_Value_Instance_To_Index(
                        wp_data->object_instance);
                    /* NOTE: this Binary value has no priority array */
                    Present_Value[object_index] =
                        (BACNET_BINARY_PV)value.type.Enumerated;
                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_OUT_OF_SERVICE:
#if 0
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                object_index =
                    Binary_Value_Instance_To_Index(wp_data->object_instance);
                Binary_Value_Out_Of_Service[object_index] = value.type.Boolean;
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
#endif
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        case PROP_POLARITY:
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
