/**
 * @brief This module manages the BACnet Analog Value objects
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
#include "stack.h"
#include "adc.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/wp.h"
#include "bacnet/basic/object/av.h"

/* functions to get the present value when requested */
typedef float (*object_present_value_callback)(void);

static float adc0_value(void)
{
    return (float)adc_millivolts(0);
}

static float adc1_value(void)
{
    return (float)adc_millivolts(1);
}

static float adc2_value(void)
{
    return (float)adc_millivolts(2);
}

static float adc3_value(void)
{
    return (float)adc_millivolts(3);
}

static float stack_size_value(void)
{
    return (float)stack_size();
}

static float stack_unused_value(void)
{
    return (float)stack_unused();
}

struct object_data {
    const uint8_t object_id;
    const char *object_name;
    uint16_t units;
    object_present_value_callback callback;
    float present_value;
};
/* clang-format off */
static struct object_data Object_List[] = {
    { 0, "ADC0", UNITS_MILLIVOLTS, adc0_value, 0.0f },
    { 1, "ADC1", UNITS_MILLIVOLTS, adc1_value, 0.0f },
    { 2, "ADC2", UNITS_MILLIVOLTS, adc2_value, 0.0f },
    { 3, "ADC3", UNITS_MILLIVOLTS, adc3_value, 0.0f },
    { 96, "MCU Frequency", UNITS_HERTZ, NULL, (float)F_CPU },
    { 97, "CStack Size", UNITS_PERCENT, stack_size_value, 0.0f },
    { 98, "CStack Unused", UNITS_PERCENT, stack_unused_value, 0.0f },
    { 99, "Uptime Seconds", UNITS_SECONDS, NULL, 0.0f }
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
bool Analog_Value_Valid_Instance(uint32_t object_instance)
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
unsigned Analog_Value_Count(void)
{
    return Objects_Max;
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * @param  index - 0..N value
 * @return  object instance-number for the given index
 */
uint32_t Analog_Value_Index_To_Instance(unsigned index)
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
unsigned Analog_Value_Instance_To_Index(uint32_t object_instance)
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
bool Analog_Value_Name_Set(uint32_t object_instance, const char *value)
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
const char *Analog_Value_Name_ASCII(uint32_t object_instance)
{
    const char *object_name = "AV-X";
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
float Analog_Value_Present_Value(uint32_t object_instance)
{
    float value = 0.0;
    struct object_data *object;

    object = Object_List_Element(object_instance);
    if (object) {
        if (object->callback) {
            value = object->callback();
        } else {
            value = object->present_value;
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
bool Analog_Value_Present_Value_Set(
    uint32_t object_instance, float value, uint8_t priority)
{
    (void)priority;
    struct object_data *object;

    object = Object_List_Element(object_instance);
    if (object) {
        object->present_value = value;
        return true;
    }

    return false;
}

/**
 * @brief For a given object instance-number, determines the units
 * @param  object_instance - object-instance number of the object
 * @return units from the given object, or UNITS_NO_UNITS if not found
 */
uint16_t Analog_Value_Units(uint32_t object_instance)
{
    uint16_t units = UNITS_NO_UNITS;
    struct object_data *object;

    object = Object_List_Element(object_instance);
    if (object) {
        units = object->units;
    }

    return units;
}

/**
 * @brief For a given object instance-number, sets the units
 * @param  object_instance - object-instance number of the object
 * @param  units - property value to set
 * @return true if valid instance and property was set
 */
bool Analog_Value_Units_Set(uint32_t object_instance, uint16_t units)
{
    struct object_data *object;

    object = Object_List_Element(object_instance);
    if (object) {
        object->units = units;
        return true;
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
int Analog_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu;

    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_ANALOG_VALUE, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            characterstring_init_ansi(
                &char_string, Analog_Value_Name_ASCII(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ANALOG_VALUE);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_real(
                &apdu[0], Analog_Value_Present_Value(rpdata->object_instance));
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
            apdu_len = encode_application_enumerated(
                &apdu[0], Analog_Value_Units(rpdata->object_instance));
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

/**
 * @brief WriteProperty handler for this object. For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return false if an error is loaded, true if no errors
 */
bool Analog_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    if (!Analog_Value_Valid_Instance(wp_data->object_instance)) {
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
            if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                status = Analog_Value_Present_Value_Set(
                    wp_data->object_instance, value.type.Real,
                    wp_data->priority);
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_UNITS:
            if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                status = Analog_Value_Units_Set(
                    wp_data->object_instance, value.type.Enumerated);
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        case PROP_OUT_OF_SERVICE:
        case PROP_DESCRIPTION:
        case PROP_PRIORITY_ARRAY:
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
 * Configure some analog pins for ADC operation
 */
void Analog_Value_Init(void)
{
    adc_enable(0);
    adc_enable(1);
    adc_enable(2);
    adc_enable(3);
}
