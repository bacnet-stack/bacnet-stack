/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @brief A basic BACnet OctetString Value object implementation.
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
#include "bacnet/basic/object/osv.h"

#ifndef MAX_OCTETSTRING_VALUES
#define MAX_OCTETSTRING_VALUES 4
#endif

static OCTETSTRING_VALUE_DESCR OSV_Descr[MAX_OCTETSTRING_VALUES];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,  PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,     PROP_STATUS_FLAGS, -1
};

static const int32_t Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, PROP_DESCRIPTION, -1
};

static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of always writable properties */
    PROP_PRESENT_VALUE, PROP_OUT_OF_SERVICE, -1
};

void OctetString_Value_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
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
 * @brief Get the list of writable properties for an Octet String Value object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void OctetString_Value_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

void OctetString_Value_Init(void)
{
    unsigned i;

    for (i = 0; i < MAX_OCTETSTRING_VALUES; i++) {
        memset(&OSV_Descr[i], 0x00, sizeof(OCTETSTRING_VALUE_DESCR));
        octetstring_init(&OSV_Descr[i].Present_Value, NULL, 0);
    }
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool OctetString_Value_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_OCTETSTRING_VALUES) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned OctetString_Value_Count(void)
{
    return MAX_OCTETSTRING_VALUES;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t OctetString_Value_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned OctetString_Value_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_OCTETSTRING_VALUES;

    if (object_instance < MAX_OCTETSTRING_VALUES) {
        index = object_instance;
    }

    return index;
}

/**
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - octetstring value
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
bool OctetString_Value_Present_Value_Set(
    uint32_t object_instance,
    const BACNET_OCTET_STRING *value,
    uint8_t priority)
{
    unsigned index = 0;
    bool status = false;

    (void)priority;
    index = OctetString_Value_Instance_To_Index(object_instance);
    if (index < MAX_OCTETSTRING_VALUES) {
        octetstring_copy(&OSV_Descr[index].Present_Value, value);
        status = true;
    }
    return status;
}

BACNET_OCTET_STRING *OctetString_Value_Present_Value(uint32_t object_instance)
{
    BACNET_OCTET_STRING *value = NULL;
    unsigned index = 0;

    index = OctetString_Value_Instance_To_Index(object_instance);
    if (index < MAX_OCTETSTRING_VALUES) {
        value = &OSV_Descr[index].Present_Value;
    }

    return value;
}

/* note: the object name must be unique within this device */
bool OctetString_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;

    if (object_instance < MAX_OCTETSTRING_VALUES) {
        snprintf(
            text, sizeof(text), "OCTETSTRING VALUE %lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int OctetString_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_OCTET_STRING *real_value = NULL;
    unsigned object_index = 0;
    bool state = false;
    uint8_t *apdu = NULL;
    OCTETSTRING_VALUE_DESCR *CurrentAV;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    apdu = rpdata->application_data;

    object_index = OctetString_Value_Instance_To_Index(rpdata->object_instance);
    if (object_index < MAX_OCTETSTRING_VALUES) {
        CurrentAV = &OSV_Descr[object_index];
    } else {
        return BACNET_STATUS_ERROR;
    }

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_OCTETSTRING_VALUE, rpdata->object_instance);
            break;

        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            OctetString_Value_Object_Name(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], OBJECT_OCTETSTRING_VALUE);
            break;

        case PROP_PRESENT_VALUE:
            real_value =
                OctetString_Value_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_octet_string(&apdu[0], real_value);
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

        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;

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

/* returns true if successful */
bool OctetString_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    OCTETSTRING_VALUE_DESCR *CurrentAV;

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
        OctetString_Value_Instance_To_Index(wp_data->object_instance);
    if (object_index < MAX_OCTETSTRING_VALUES) {
        CurrentAV = &OSV_Descr[object_index];
    } else {
        return false;
    }

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_OCTET_STRING);
            if (status) {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                if (OctetString_Value_Present_Value_Set(
                        wp_data->object_instance, &value.type.Octet_String,
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

    return status;
}

void OctetString_Value_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
