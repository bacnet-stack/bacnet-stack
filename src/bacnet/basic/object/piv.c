/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @brief API for a basic BACnet Positive Integer Value object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/object/piv.h"

#ifndef MAX_POSITIVEINTEGER_VALUES
#define MAX_POSITIVEINTEGER_VALUES 1
#endif

static POSITIVEINTEGER_VALUE_DESCR PIV_Descr[MAX_POSITIVEINTEGER_VALUES];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t PositiveInteger_Value_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_UNITS,
    -1
};

static const int32_t PositiveInteger_Value_Properties_Optional[] = {
    PROP_OUT_OF_SERVICE, -1
};

static const int32_t PositiveInteger_Value_Properties_Proprietary[] = { -1 };

void PositiveInteger_Value_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
{
    if (pRequired) {
        *pRequired = PositiveInteger_Value_Properties_Required;
    }
    if (pOptional) {
        *pOptional = PositiveInteger_Value_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = PositiveInteger_Value_Properties_Proprietary;
    }

    return;
}

void PositiveInteger_Value_Init(void)
{
    unsigned i;

    for (i = 0; i < MAX_POSITIVEINTEGER_VALUES; i++) {
        memset(&PIV_Descr[i], 0x00, sizeof(POSITIVEINTEGER_VALUE_DESCR));
    }
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool PositiveInteger_Value_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_POSITIVEINTEGER_VALUES) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned PositiveInteger_Value_Count(void)
{
    return MAX_POSITIVEINTEGER_VALUES;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t PositiveInteger_Value_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned PositiveInteger_Value_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_POSITIVEINTEGER_VALUES;

    if (object_instance < MAX_POSITIVEINTEGER_VALUES) {
        index = object_instance;
    }

    return index;
}

/**
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - positiveinteger value
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
bool PositiveInteger_Value_Present_Value_Set(
    uint32_t object_instance, uint32_t value, uint8_t priority)
{
    unsigned index = 0;
    bool status = false;

    (void)priority;
    index = PositiveInteger_Value_Instance_To_Index(object_instance);
    if (index < MAX_POSITIVEINTEGER_VALUES) {
        PIV_Descr[index].Present_Value = value;
        status = true;
    }

    return status;
}

uint32_t PositiveInteger_Value_Present_Value(uint32_t object_instance)
{
    uint32_t value = 0;
    unsigned index = 0;

    index = PositiveInteger_Value_Instance_To_Index(object_instance);
    if (index < MAX_POSITIVEINTEGER_VALUES) {
        value = PIV_Descr[index].Present_Value;
    }

    return value;
}

/* note: the object name must be unique within this device */
bool PositiveInteger_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;

    if (object_instance < MAX_POSITIVEINTEGER_VALUES) {
        snprintf(
            text, sizeof(text), "POSITIVEINTEGER VALUE %lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
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
int PositiveInteger_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index = 0;
    bool state = false;
    uint8_t *apdu = NULL;
    POSITIVEINTEGER_VALUE_DESCR *CurrentAV;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    apdu = rpdata->application_data;

    object_index =
        PositiveInteger_Value_Instance_To_Index(rpdata->object_instance);
    if (object_index < MAX_POSITIVEINTEGER_VALUES) {
        CurrentAV = &PIV_Descr[object_index];
    } else {
        return BACNET_STATUS_ERROR;
    }

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_POSITIVE_INTEGER_VALUE,
                rpdata->object_instance);
            break;

        case PROP_OBJECT_NAME:
            PositiveInteger_Value_Object_Name(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], OBJECT_POSITIVE_INTEGER_VALUE);
            break;

        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                PositiveInteger_Value_Present_Value(rpdata->object_instance));
            break;

        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                CurrentAV->Out_Of_Service);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_UNITS:
            apdu_len = encode_application_enumerated(
                &apdu[0], (uint32_t)CurrentAV->Units);
            break;
            /* BACnet Testing Observed Incident oi00109
                    Positive Integer Value / Units returned wrong datatype -
               missing break. Revealed by BACnet Test Client v1.8.16 (
               www.bac-test.com/bacnet-test-client-download ) BITS: BIT00031 BC
               135.1: 9.20.1.7 BC 135.1: 9.20.1.9 Any discussions can be
               directed to edward@bac-test.com Please feel free to remove this
               comment when my changes have been reviewed by all interested
               parties. Say 6 months -> September 2016 */

        case PROP_OUT_OF_SERVICE:
            state = CurrentAV->Out_Of_Service;
            apdu_len = encode_application_boolean(&apdu[0], state);
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
bool PositiveInteger_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    POSITIVEINTEGER_VALUE_DESCR *CurrentAV;

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
    object_index =
        PositiveInteger_Value_Instance_To_Index(wp_data->object_instance);
    if (object_index < MAX_POSITIVEINTEGER_VALUES) {
        CurrentAV = &PIV_Descr[object_index];
    } else {
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                if (PositiveInteger_Value_Present_Value_Set(
                        wp_data->object_instance, value.type.Unsigned_Int,
                        wp_data->priority)) {
                    status = true;
                } else if (wp_data->priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                CurrentAV->Out_Of_Service = value.type.Boolean;
            }
            break;
        case PROP_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= UINT16_MAX) {
                    CurrentAV->Units = (uint16_t)value.type.Enumerated;
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        default:
            if (property_lists_member(
                    PositiveInteger_Value_Properties_Required,
                    PositiveInteger_Value_Properties_Optional,
                    PositiveInteger_Value_Properties_Proprietary,
                    wp_data->object_property)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            }
            break;
    }

    return status;
}

void PositiveInteger_Value_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
