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

struct object_data {
    const uint8_t object_id;
    const char *object_name;
    volatile uint8_t *port;
    volatile uint8_t *pin;
    volatile uint8_t *ddr;
    const uint8_t bit;
};
/* clang-format off */
static struct object_data Object_List[] = {
    { 0, "D3", NULL, &PIND, &DDRD, PD3 },
    { 1, "D4", NULL, &PIND, &DDRD, PD4 },
    { 2, "D5", NULL, &PIND, &DDRD, PD5 },
    { 3, "D6", NULL, &PIND, &DDRD, PD6 },
    { 4, "D7", NULL, &PIND, &DDRD, PD7 },
    { 5, "D8", &PORTB, &PINB, &DDRB, PB0 },
    { 6, "D9", &PORTB, &PINB, &DDRB, PB1 },
    { 7, "D10", &PORTB, &PINB, &DDRB, PB2 },
    { 8, "D11", &PORTB, &PINB, &DDRB, PB3 },
    { 9, "D12", &PORTB, &PINB, &DDRB, PB4 },
    { 99, "LED", &PORTB, &PINB, &DDRB, PB5 }
};
/* clang-format on */

/* number of objects */
static const unsigned Objects_Max =
    sizeof(Object_List) / sizeof(Object_List[0]);

/**
 * @brief Return the object or NULL if not found.
 * @param  object_instance - object-instance number of the object
 * @return  object if the instance is found, and NULL if not
 */
static struct object_data *Object_List_Element(uint32_t object_instance)
{
    unsigned index;
    uint32_t object_id;

    for (index = 0; index < Objects_Max; index++) {
        object_id = Object_List[index].object_id;
        if (object_id == object_instance) {
            return &Object_List[index];
        }
    }

    return NULL;
}

/**
 * @brief Determines if a given Analog Value instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Binary_Value_Valid_Instance(uint32_t object_instance)
{
    if (Object_List_Element(object_instance)) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of objects
 * @return  Number of objects
 */
unsigned Binary_Value_Count(void)
{
    return Objects_Max;
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * @param  index - 0..N value
 * @return  object instance-number for the given index
 */
uint32_t Binary_Value_Index_To_Instance(unsigned index)
{
    uint32_t object_instance = UINT32_MAX;

    if (index < Objects_Max) {
        object_instance = Object_List[index].object_id;
    }

    return object_instance;
}

/**
 * @brief For a given object instance-number, determines a 0..N index
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number, or N if not valid.
 */
unsigned Binary_Value_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = 0;

    for (index = 0; index < Objects_Max; index++) {
        if (Object_List[index].object_id == object_instance) {
            break;
        }
    }

    return index;
}

/**
 * @brief For a given object instance-number, sets the object-name
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 * @return  true if object-name was set
 */
bool Binary_Value_Name_Set(uint32_t object_instance, const char *value)
{
    struct object_data *object;

    object = Object_List_Element(object_instance);
    if (object) {
        object->object_name = value;
        return true;
    }

    return false;
}

/**
 * @brief Return the object name C string
 * @param object_instance [in] BACnet object instance number
 * @return object name or NULL if not found
 */
const char *Binary_Value_Name_ASCII(uint32_t object_instance)
{
    const char *object_name = "BV-X";
    struct object_data *object;

    object = Object_List_Element(object_instance);
    if (object) {
        object_name = object->object_name;
    }

    return object_name;
}

/**
 * @brief For a given object instance-number, determines the present-value
 * @param  object_instance - object-instance number of the object
 * @return  present-value of the object
 */
BACNET_BINARY_PV Binary_Value_Present_Value(uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;
    struct object_data *object;

    object = Object_List_Element(object_instance);
    if (object) {
        if (BIT_CHECK(*object->pin, object->bit)) {
            value = BINARY_ACTIVE;
        }
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the present-value
 * @param  object_instance - object-instance number of the object
 * @param  value - value to set
 * @return true if value is within range and present-value is set.
 */
bool Binary_Value_Present_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    struct object_data *object;

    object = Object_List_Element(object_instance);
    if (object) {
        if (object->port) {
            if (value == BINARY_ACTIVE) {
                BIT_SET(*object->port, object->bit);
            } else {
                BIT_CLEAR(*object->port, object->bit);
            }
            return true;
        }
    }

    return false;
}

/**
 * @brief ReadProperty handler for this object. For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
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
                &char_string, Binary_Value_Name_ASCII(rpdata->object_instance));
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
                    status = Binary_Value_Present_Value_Set(
                        wp_data->object_instance,
                        (BACNET_BINARY_PV)value.type.Enumerated);
                    if (!status) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                    }
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

/**
 * Configure some digital pins as inputs or outputs
 */
void Binary_Value_Init(void)
{
    unsigned index = 0;

    for (index = 0; index < Objects_Max; index++) {
        if (Object_List[index].port) {
            /* Configure the pin as an output */
            BIT_CLEAR(*Object_List[index].port, Object_List[index].bit);
            BIT_SET(*Object_List[index].ddr, Object_List[index].bit);
            /* Turn off the pin */
            BIT_CLEAR(*Object_List[index].port, Object_List[index].bit);
        } else {
            /* Configure the pin as an input */
            BIT_CLEAR(*Object_List[index].ddr, Object_List[index].bit);
        }
    }
}
