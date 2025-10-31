/**
 * @file
 * @brief BACnet accumulator Objects used to represent meter registers
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2017
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/proplist.h"
#include "bacnet/basic/object/acc.h"

#ifndef MAX_ACCUMULATORS
#define MAX_ACCUMULATORS 64
#endif

struct object_data {
    BACNET_UNSIGNED_INTEGER Present_Value;
    int32_t Scale;
};

static struct object_data Object_List[MAX_ACCUMULATORS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_SCALE,
    PROP_UNITS,
    PROP_MAX_PRES_VALUE,
    -1
};

static const int Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int Properties_Proprietary[] = { -1 };

/**
 * Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 *
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optkional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void Accumulator_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Properties_Required;
    }
    if (pOptional) {
        *pOptional = Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Properties_Proprietary;
    }

    return;
}

/**
 * Determines if a given Accumulator instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Accumulator_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_ACCUMULATORS) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Accumulator objects
 *
 * @return  Number of Accumulator objects
 */
unsigned Accumulator_Count(void)
{
    return MAX_ACCUMULATORS;
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Accumulator objects where N is Accumulator_Count().
 *
 * @param  index - 0..Accumulator_Count() value
 *
 * @return  object instance-number for the given index
 */
uint32_t Accumulator_Index_To_Instance(unsigned index)
{
    return index;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Accumulator objects where N is Accumulator_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or MAX_ACCUMULATORS
 * if not valid.
 */
unsigned Accumulator_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_ACCUMULATORS;

    if (object_instance < MAX_ACCUMULATORS) {
        index = object_instance;
    }

    return index;
}

/**
 * For a given object instance-number, loads the object-name into
 * a characterstring. Note that the object name must be unique
 * within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Accumulator_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32];
    bool status = false;

    if (object_instance < MAX_ACCUMULATORS) {
        snprintf(
            text, sizeof(text), "ACCUMULATOR-%lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
}

/**
 * For a given object instance-number, determines the present-value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  present-value of the object
 */
BACNET_UNSIGNED_INTEGER Accumulator_Present_Value(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER value = 0;

    if (object_instance < MAX_ACCUMULATORS) {
        value = Object_List[object_instance].Present_Value;
    }

    return value;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACNET_UNSIGNED_INTEGER value
 *
 * @return  true if values are within range and present-value is set.
 */
bool Accumulator_Present_Value_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    bool status = false;

    if (object_instance < MAX_ACCUMULATORS) {
        Object_List[object_instance].Present_Value = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the units property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  units property value
 */
uint16_t Accumulator_Units(uint32_t object_instance)
{
    uint16_t units = UNITS_NO_UNITS;

    if (object_instance < MAX_ACCUMULATORS) {
        units = UNITS_WATT_HOURS;
    }

    return units;
}

/**
 * For a given object instance-number, returns the scale property value
 *
 * Option         Datatype    Indicated Value in Units
 * float-scale    REAL        Present_Value x Scale
 * integer-scale  INTEGER     Present_Value x 10 Scale
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  scale property integer value
 */
int32_t Accumulator_Scale_Integer(uint32_t object_instance)
{
    int32_t scale = 0;

    if (object_instance < MAX_ACCUMULATORS) {
        scale = Object_List[object_instance].Scale;
    }

    return scale;
}

/**
 * For a given object instance-number, returns the scale property value
 *
 * Option         Datatype    Indicated Value in Units
 * float-scale    REAL        Present_Value x Scale
 * integer-scale  INTEGER     Present_Value x 10 Scale
 *
 * @param  object_instance - object-instance number of the object
 * @param  scale -  scale property integer value
 *
 * @return  true if valid object and value is within range
 */
bool Accumulator_Scale_Integer_Set(uint32_t object_instance, int32_t scale)
{
    bool status = false;

    if (object_instance < MAX_ACCUMULATORS) {
        Object_List[object_instance].Scale = scale;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the scale property value
 *
 * Option         Datatype    Indicated Value in Units
 * float-scale    REAL        Present_Value x Scale
 * integer-scale  INTEGER     Present_Value x 10 Scale
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  scale property integer value
 */
BACNET_UNSIGNED_INTEGER Accumulator_Max_Pres_Value(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER max_value = 0;

    if (object_instance < MAX_ACCUMULATORS) {
        max_value = BACNET_UNSIGNED_INTEGER_MAX;
    }

    return max_value;
}

/**
 * ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int Accumulator_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch ((int)rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_ACCUMULATOR, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Accumulator_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ACCUMULATOR);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_unsigned(
                &apdu[0], Accumulator_Present_Value(rpdata->object_instance));
            break;
        case PROP_SCALE:
            /* context tagged choice: [0]=REAL, [1]=INTEGER */
            apdu_len = encode_context_signed(
                &apdu[apdu_len], 1,
                Accumulator_Scale_Integer(rpdata->object_instance));
            break;
        case PROP_MAX_PRES_VALUE:
            apdu_len = encode_application_unsigned(
                &apdu[0], Accumulator_Max_Pres_Value(rpdata->object_instance));
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
                &apdu[0], Accumulator_Units(rpdata->object_instance));
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/**
 * WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return false if an error is loaded, true if no errors
 */
bool Accumulator_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

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
    switch ((int)wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if (value.type.Unsigned_Int <=
                    Accumulator_Max_Pres_Value(wp_data->object_instance)) {
                    Accumulator_Present_Value_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    return false;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                return false;
            }
            break;
        default:
            if (property_lists_member(
                    Properties_Required, Properties_Optional,
                    Properties_Proprietary, wp_data->object_property)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            }
            break;
    }

    return false;
}

/**
 * Initializes the Accumulator object data
 */
void Accumulator_Init(void)
{
    BACNET_UNSIGNED_INTEGER unsigned_value = 1;
    unsigned i = 0;

    for (i = 0; i < MAX_ACCUMULATORS; i++) {
        Accumulator_Scale_Integer_Set(i, i + 1);
        Accumulator_Present_Value_Set(i, unsigned_value);
        unsigned_value |= (unsigned_value << 1);
    }
}
