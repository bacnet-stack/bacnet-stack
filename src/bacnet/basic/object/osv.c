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
#include "bacnet/basic/object/osv.h"

/* Key List for storing object data sorted by instance number */
static OS_Keylist Object_List = NULL;

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

/**
 * @brief Retrieves property identifier lists for Octet String Value objects.
 * @param pRequired Optional pointer to receive required property list.
 * @param pOptional Optional pointer to receive optional property list.
 * @param pProprietary Optional pointer to receive proprietary property list.
 */
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

/**
 * @brief Finds an Octet String Value object descriptor by instance number.
 * @param object_instance Object instance number.
 * @return Pointer to object descriptor, or NULL if not found.
 */
static OCTETSTRING_VALUE_DESCR *
OctetString_Value_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Creates an Octet String Value object instance.
 * @param object_instance Requested object instance number, or
 * BACNET_MAX_INSTANCE for auto-allocation.
 * @return Created instance number, or BACNET_MAX_INSTANCE on failure.
 */
uint32_t OctetString_Value_Create(uint32_t object_instance)
{
    OCTETSTRING_VALUE_DESCR *pObject = NULL;
    int index = 0;

    if (!Object_List) {
        Object_List = Keylist_Create();
    }
    if (object_instance > BACNET_MAX_INSTANCE) {
        return BACNET_MAX_INSTANCE;
    } else if (object_instance == BACNET_MAX_INSTANCE) {
        object_instance = Keylist_Next_Empty_Key(Object_List, 1);
    }
    pObject = OctetString_Value_Object(object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(OCTETSTRING_VALUE_DESCR));
        if (!pObject) {
            return BACNET_MAX_INSTANCE;
        }
        index = Keylist_Data_Add(Object_List, object_instance, pObject);
        if (index < 0) {
            free(pObject);
            return BACNET_MAX_INSTANCE;
        }
        octetstring_init(&pObject->Present_Value, NULL, 0);
        pObject->Out_Of_Service = false;
        pObject->Event_State = EVENT_STATE_NORMAL;
    }

    return object_instance;
}

/**
 * @brief Deletes an Octet String Value object instance.
 * @param object_instance Object instance number.
 * @return true if object existed and was deleted.
 */
bool OctetString_Value_Delete(uint32_t object_instance)
{
    OCTETSTRING_VALUE_DESCR *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        return true;
    }

    return false;
}

/**
 * @brief Initializes Octet String Value object instances.
 */
void OctetString_Value_Init(void)
{
#ifdef MAX_OCTETSTRING_VALUES
    unsigned i = 0;

    for (i = 0; i < MAX_OCTETSTRING_VALUES; i++) {
        OctetString_Value_Create(i);
    }
#endif
}

/**
 * @brief Checks whether an Octet String Value instance exists.
 * @param object_instance Object instance number.
 * @return true if the instance exists.
 */
bool OctetString_Value_Valid_Instance(uint32_t object_instance)
{
    return (OctetString_Value_Object(object_instance) != NULL);
}

/**
 * @brief Gets the number of Octet String Value instances.
 * @return Number of object instances.
 */
unsigned OctetString_Value_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Maps an object list index to an instance number.
 * @param index Zero-based object index.
 * @return Object instance number, or UINT32_MAX if index is invalid.
 */
uint32_t OctetString_Value_Index_To_Instance(unsigned index)
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
unsigned OctetString_Value_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
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
    OCTETSTRING_VALUE_DESCR *pObject = NULL;
    bool status = false;

    (void)priority;
    pObject = OctetString_Value_Object(object_instance);
    if (pObject) {
        octetstring_copy(&pObject->Present_Value, value);
        status = true;
    }

    return status;
}

/**
 * @brief Gets the present value for an Octet String Value object.
 * @param object_instance Object instance number.
 * @return Pointer to present value, or NULL if object does not exist.
 */
BACNET_OCTET_STRING *OctetString_Value_Present_Value(uint32_t object_instance)
{
    BACNET_OCTET_STRING *value = NULL;
    OCTETSTRING_VALUE_DESCR *pObject = NULL;

    pObject = OctetString_Value_Object(object_instance);
    if (pObject) {
        value = &pObject->Present_Value;
    }

    return value;
}

/**
 * @brief Gets the object name for an Octet String Value object.
 * @param object_instance Object instance number.
 * @param object_name Pointer to string storage for resulting object name.
 * @return true if object name is generated successfully.
 */
bool OctetString_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;
    OCTETSTRING_VALUE_DESCR *pObject = NULL;

    pObject = OctetString_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                text, sizeof(text), "OCTETSTRING VALUE %lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text);
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the object-name
 *  Note that the object name must be unique within this device.
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 * @return  true if object-name was set
 */
bool OctetString_Value_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    OCTETSTRING_VALUE_DESCR *pObject;

    pObject = OctetString_Value_Object(object_instance);
    if (pObject) {
        status = true;
        pObject->Object_Name = new_name;
    }

    return status;
}

/**
 * @brief Return the object name C string
 * @param object_instance [in] BACnet object instance number
 * @return object name or NULL if not found
 */
const char *OctetString_Value_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    OCTETSTRING_VALUE_DESCR *pObject;

    pObject = OctetString_Value_Object(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * @brief Encodes a read-property response for an Octet String Value object.
 * @param rpdata Read property request/response context.
 * @return Encoded APDU length, or BACNET_STATUS_ERROR on error.
 */
int OctetString_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_OCTET_STRING *real_value = NULL;
    bool state = false;
    uint8_t *apdu = NULL;
    OCTETSTRING_VALUE_DESCR *pObject = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    apdu = rpdata->application_data;

    pObject = OctetString_Value_Object(rpdata->object_instance);
    if (!pObject) {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
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
                pObject->Out_Of_Service);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;

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
 * @brief Processes a write-property request for an Octet String Value object.
 * @param wp_data Write property request/response context.
 * @return true if property write is successful.
 */
bool OctetString_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    OCTETSTRING_VALUE_DESCR *pObject = NULL;

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
    pObject = OctetString_Value_Object(wp_data->object_instance);
    if (!pObject) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
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
                pObject->Out_Of_Service = value.type.Boolean;
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
