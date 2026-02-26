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
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/object/piv.h"

#ifndef MAX_POSITIVEINTEGER_VALUES
#define MAX_POSITIVEINTEGER_VALUES 4
#endif

/* Key List for storing object data sorted by instance number */
static OS_Keylist Object_List = NULL;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_POSITIVE_INTEGER_VALUE;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_UNITS,
    -1
};

static const int32_t Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_OUT_OF_SERVICE, -1
};

static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of always writable properties */
    PROP_PRESENT_VALUE, PROP_OUT_OF_SERVICE, PROP_UNITS, -1
};

/**
 * @brief Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 *
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void PositiveInteger_Value_Property_Lists(
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
 * @brief Get list of writable properties for a Positive Integer Value object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void PositiveInteger_Value_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * @brief Finds a Positive Integer Value object descriptor by instance number.
 * @param object_instance Object instance number.
 * @return Pointer to object descriptor, or NULL if not found.
 */
static POSITIVEINTEGER_VALUE_DESCR *PositiveInteger_Value_Object(
    uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Creates a Positive Integer Value object instance.
 * @param object_instance Requested object instance number, or BACNET_MAX_INSTANCE for auto-allocation.
 * @return Created instance number, or BACNET_MAX_INSTANCE on failure.
 */
uint32_t PositiveInteger_Value_Create(uint32_t object_instance)
{
    POSITIVEINTEGER_VALUE_DESCR *pObject = NULL;
    int index = 0;

    if (!Object_List) {
        Object_List = Keylist_Create();
    }
    if (object_instance > BACNET_MAX_INSTANCE) {
        return BACNET_MAX_INSTANCE;
    } else if (object_instance == BACNET_MAX_INSTANCE) {
        object_instance = Keylist_Next_Empty_Key(Object_List, 1);
    }
    pObject = PositiveInteger_Value_Object(object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(POSITIVEINTEGER_VALUE_DESCR));
        if (!pObject) {
            return BACNET_MAX_INSTANCE;
        }
        index = Keylist_Data_Add(Object_List, object_instance, pObject);
        if (index < 0) {
            free(pObject);
            return BACNET_MAX_INSTANCE;
        }
        pObject->Out_Of_Service = false;
        pObject->Present_Value = 0;
        pObject->Units = UNITS_NO_UNITS;
    }

    return object_instance;
}

/**
 * @brief Deletes a Positive Integer Value object instance.
 * @param object_instance Object instance number.
 * @return true if object existed and was deleted.
 */
bool PositiveInteger_Value_Delete(uint32_t object_instance)
{
    POSITIVEINTEGER_VALUE_DESCR *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        return true;
    }

    return false;
}

/**
 * @brief Initializes the Positive Integer Value objects
 */
void PositiveInteger_Value_Init(void)
{
    unsigned i = 0;

    if (!Object_List) {
        Object_List = Keylist_Create();
    }
    for (i = 0; i < MAX_POSITIVEINTEGER_VALUES; i++) {
        PositiveInteger_Value_Create(i);
    }
}

/**
 * @brief Checks whether a Positive Integer Value instance exists.
 * @param object_instance Object instance number.
 * @return true if the instance exists.
 */
bool PositiveInteger_Value_Valid_Instance(uint32_t object_instance)
{
    return (PositiveInteger_Value_Object(object_instance) != NULL);
}

/**
 * @brief Gets the number of Positive Integer Value instances.
 * @return Number of object instances.
 */
unsigned PositiveInteger_Value_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Maps an object list index to an instance number.
 * @param index Zero-based object index.
 * @return Object instance number, or UINT32_MAX if index is invalid.
 */
uint32_t PositiveInteger_Value_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * @brief Maps an instance number to object list index.
 * @param object_instance Object instance number.
 * @return Zero-based object index.
 */
unsigned PositiveInteger_Value_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
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
    POSITIVEINTEGER_VALUE_DESCR *pObject = NULL;
    bool status = false;

    (void)priority;
    pObject = PositiveInteger_Value_Object(object_instance);
    if (pObject) {
        pObject->Present_Value = value;
        status = true;
    }

    return status;
}

/**
 * @brief Gets the present value for a Positive Integer Value object.
 * @param object_instance Object instance number.
 * @return Present value, or 0 if object does not exist.
 */
uint32_t PositiveInteger_Value_Present_Value(uint32_t object_instance)
{
    uint32_t value = 0;
    POSITIVEINTEGER_VALUE_DESCR *pObject = NULL;

    pObject = PositiveInteger_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Present_Value;
    }

    return value;
}

/**
 * @brief Gets the object name for a Positive Integer Value object.
 * @param object_instance Object instance number.
 * @param object_name Pointer to string storage for resulting object name.
 * @return true if object name is generated successfully.
 */
bool PositiveInteger_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;
    POSITIVEINTEGER_VALUE_DESCR *pObject = NULL;

    pObject = PositiveInteger_Value_Object(object_instance);
    if (pObject) {
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
    bool state = false;
    uint8_t *apdu = NULL;
    POSITIVEINTEGER_VALUE_DESCR *pObject = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    apdu = rpdata->application_data;

    pObject = PositiveInteger_Value_Object(rpdata->object_instance);
    if (!pObject) {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return BACNET_STATUS_ERROR;
    }

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type,
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
                &apdu[0], Object_Type);
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
                pObject->Out_Of_Service);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_UNITS:
            apdu_len = encode_application_enumerated(
                &apdu[0], (uint32_t)pObject->Units);
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
            state = pObject->Out_Of_Service;
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
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    POSITIVEINTEGER_VALUE_DESCR *pObject = NULL;

    if (wp_data == NULL) {
        return false;
    }
    if (wp_data->application_data_len == 0) {
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
    pObject = PositiveInteger_Value_Object(wp_data->object_instance);
    if (!pObject) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
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
                pObject->Out_Of_Service = value.type.Boolean;
            }
            break;
        case PROP_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= UINT16_MAX) {
                    pObject->Units = (uint16_t)value.type.Enumerated;
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
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

/**
 * @brief Performs intrinsic reporting for a Positive Integer Value object.
 * @param object_instance Object instance number.
 */
void PositiveInteger_Value_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
