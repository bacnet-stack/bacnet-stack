/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2024
 * @brief The BitString Value object is an object with a present-value that
 * uses a BACnetBitString data type.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/config.h"
#include "bacnet/proplist.h"
#include "bacnet/property.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bitstring_value.h"

struct object_data {
    bool Change_Of_Value : 1;
    bool Write_Enabled : 1;
    bool Out_Of_Service : 1;
    BACNET_BIT_STRING Present_Value;
    BACNET_RELIABILITY Reliability;
    const char *Object_Name;
    const char *Description;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* callback for present value writes */
static bitstring_value_write_present_value_callback
    BitString_Value_Write_Present_Value_Callback;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,  PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,     PROP_STATUS_FLAGS, -1
};

static const int Properties_Optional[] = { PROP_RELIABILITY,
                                           PROP_OUT_OF_SERVICE,
                                           PROP_DESCRIPTION, -1 };

static const int Properties_Proprietary[] = { -1 };

/**
 * Initialize the pointers for the required, the optional and the properitary
 * value properties.
 *
 * @param pRequired - Pointer to the pointer of required values.
 * @param pOptional - Pointer to the pointer of optional values.
 * @param pProprietary - Pointer to the pointer of properitary values.
 */
void BitString_Value_Property_Lists(
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
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct object_data *BitString_Value_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * We simply have 0-n object instances. Yours might be
 * more complex, and then you need validate that the
 * given instance exists.
 *
 * @param object_instance Object instance
 *
 * @return true/false
 */
bool BitString_Value_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * Return the count of character string values.
 *
 * @return Count of character string values.
 */
unsigned BitString_Value_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * of objects where N is the count.
 * @param  index - 0..N value
 * @return  object instance-number for a valid given index, or UINT32_MAX
 */
uint32_t BitString_Value_Index_To_Instance(unsigned index)
{
    uint32_t instance = UINT32_MAX;

    (void)Keylist_Index_Key(Object_List, index, &instance);

    return instance;
}

/**
 * We simply have 0-n object instances. Yours might be
 * more complex, and then you need to return the instance
 * that correlates to the correct index.
 *
 * @param object_instance Object instance
 *
 * @return Object instance
 */
unsigned BitString_Value_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief For a given object instance-number, read the present-value.
 * @param  object_instance - object-instance number of the object
 * @param  value - Pointer to the new value
 * @return  true if value is within range and copied
 */
bool BitString_Value_Present_Value(
    uint32_t object_instance, BACNET_BIT_STRING *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        status = bitstring_copy(value, &pObject->Present_Value);
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the present-value,
 * taken from another bitstring.
 * @param  object_instance - object-instance number of the object
 * @param  value - Pointer to the value.
 * @return  true if value is within range and copied
 */
bool BitString_Value_Present_Value_Set(
    uint32_t object_instance, const BACNET_BIT_STRING *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        if (!bitstring_same(&pObject->Present_Value, value)) {
            pObject->Change_Of_Value = true;
        }
        status = bitstring_copy(&pObject->Present_Value, value);
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value
 * from WriteProperty service
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - new present value
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool BitString_Value_Present_Value_Write(
    uint32_t object_instance,
    BACNET_BIT_STRING *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    BACNET_BIT_STRING old_value;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        (void)priority;
        if (pObject->Write_Enabled) {
            bitstring_copy(&old_value, &pObject->Present_Value);
            bitstring_copy(&pObject->Present_Value, value);
            if (pObject->Out_Of_Service) {
                /* The physical point that the object represents
                    is not in service. This means that changes to the
                    Present_Value property are decoupled from the
                    physical point when the value of Out_Of_Service
                    is true. */
            } else if (BitString_Value_Write_Present_Value_Callback) {
                BitString_Value_Write_Present_Value_Callback(
                    object_instance, &old_value, value);
            }
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * For a given object instance-number, read the out of service value.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true/false
 */
bool BitString_Value_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * @brief For a given object instance-number, set the out of service value.
 * @param  object_instance - object-instance number of the object
 * @param  true/false
 */
void BitString_Value_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Out_Of_Service != value) {
            pObject->Change_Of_Value = true;
        }
        pObject->Out_Of_Service = value;
    }

    return;
}

/**
 * @brief For a given object instance-number, read the reliability value.
 * @param  object_instance - object-instance number of the object
 * @return  BACNET_RELIABILITY value
 */
BACNET_RELIABILITY BitString_Value_Reliablity(uint32_t object_instance)
{
    BACNET_RELIABILITY value = RELIABILITY_NO_FAULT_DETECTED;
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Reliability;
    }

    return value;
}

/**
 * @brief For a given object, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool BitString_Value_Object_Fault(const struct object_data *pObject)
{
    bool fault = false;

    if (pObject) {
        if (pObject->Reliability != RELIABILITY_NO_FAULT_DETECTED) {
            fault = true;
        }
    }

    return fault;
}

/**
 * @brief For a given object instance-number, set the reliability value.
 * @param  object_instance - object-instance number of the object
 * @param  true/false
 */
bool BitString_Value_Reliablity_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    struct object_data *pObject;
    bool status = false;
    bool fault = false;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        if (value <= 255) {
            fault = BitString_Value_Object_Fault(pObject);
            pObject->Reliability = value;
            if (fault != BitString_Value_Object_Fault(pObject)) {
                pObject->Change_Of_Value = true;
            }
            status = true;
        }
        pObject->Reliability = value;
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool BitString_Value_Fault(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);

    return BitString_Value_Object_Fault(pObject);
}

/**
 * @brief Get the COV change flag status
 * @param object_instance - object-instance number of the object
 * @return the COV change flag status
 */
bool BitString_Value_Change_Of_Value(uint32_t object_instance)
{
    bool changed = false;
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        changed = pObject->Change_Of_Value;
    }

    return changed;
}

/**
 * @brief Clear the COV change flag
 * @param object_instance - object-instance number of the object
 */
void BitString_Value_Change_Of_Value_Clear(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        pObject->Change_Of_Value = false;
    }
}

/**
 * @brief For a given object instance-number,
 *  loads the value_list with the COV data.
 * @param  object_instance - object-instance number of the object
 * @param  value_list - list of COV data
 * @return  true if the value list is encoded
 */
bool BitString_Value_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;
    const bool in_alarm = false;
    const bool fault = false;
    const bool overridden = false;
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        status = cov_value_list_encode_bit_string(
            value_list, &pObject->Present_Value, in_alarm, fault, overridden,
            pObject->Out_Of_Service);
    }

    return status;
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
bool BitString_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[32] = "BITSTRING_VALUE-4194303";

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "BITSTRING_VALUE-%u",
                object_instance);
            status = characterstring_init_ansi(object_name, name_text);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the object-name
 * Note that the object name must be unique within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 *
 * @return  true if object-name was set
 */
bool BitString_Value_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
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
const char *BitString_Value_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * @brief For a given object instance-number, return the description.
 * @param  object_instance - object-instance number of the object
 * @return  C-string pointer to the description,
 *  or NULL if object doesn't exist
 */
const char *BitString_Value_Description(uint32_t object_instance)
{
    const char *name = NULL; /* return value */
    const struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Description) {
            name = pObject->Description;
        } else {
            name = "";
        }
    }

    return name;
}

/**
 * For a given object instance-number, set the description text.
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_descr - C-String pointer to the string, representing the
 * description text
 *
 * @return True on success, false otherwise.
 */
bool BitString_Value_Description_Set(
    uint32_t object_instance, const char *value)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        status = true;
        pObject->Description = value;
    }

    return status;
}

/**
 * Return the requested property of the character string value.
 *
 * @param rpdata  Property requested, see for BACNET_READ_PROPERTY_DATA details.
 *
 * @return apdu len, or BACNET_STATUS_ERROR on error
 */
int BitString_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    bool state = false;
    bool is_array = false;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_BITSTRING_VALUE, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            if (BitString_Value_Object_Name(
                    rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                BitString_Value_Description(rpdata->object_instance));
            apdu_len = encode_application_character_string(apdu, &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_BITSTRING_VALUE);
            break;
        case PROP_PRESENT_VALUE:
            BitString_Value_Present_Value(rpdata->object_instance, &bit_string);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            state = BitString_Value_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = BitString_Value_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OUT_OF_SERVICE:
            state = BitString_Value_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], BitString_Value_Reliablity(rpdata->object_instance));
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    is_array = property_list_bacnet_array_member(
        rpdata->object_type, rpdata->object_property);
    if ((apdu_len >= 0) && (!is_array) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * Set the requested property of the character string value.
 *
 * @param wp_data  Property requested, see for BACNET_WRITE_PROPERTY_DATA
 * details.
 *
 * @return true if successful
 */
bool BitString_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    if (wp_data == NULL) {
        return false;
    }
    if (wp_data->application_data_len == 0) {
        return false;
    }
    /* Decode the some of the request. */
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
        (wp_data->object_property != PROP_EVENT_TIME_STAMPS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BIT_STRING);
            if (status) {
                status = BitString_Value_Present_Value_Write(
                    wp_data->object_instance, &value.type.Bit_String,
                    wp_data->priority, &wp_data->error_class,
                    &wp_data->error_code);
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                BitString_Value_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
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
 * @brief Sets a callback used when present-value is written from BACnet
 * @param cb - callback used to provide indications
 */
void BitString_Value_Write_Present_Value_Callback_Set(
    bitstring_value_write_present_value_callback cb)
{
    BitString_Value_Write_Present_Value_Callback = cb;
}

/**
 * @brief Determines a object write-enabled flag state
 * @param object_instance - object-instance number of the object
 * @return  write-enabled status flag
 */
bool BitString_Value_Write_Enabled(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Write_Enabled;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void BitString_Value_Write_Enable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        pObject->Write_Enabled = true;
    }
}

/**
 * @brief For a given object instance-number, clears the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void BitString_Value_Write_Disable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = BitString_Value_Object(object_instance);
    if (pObject) {
        pObject->Write_Enabled = false;
    }
}

/**
 * Creates a BitString Value object
 * @param object_instance - object-instance number of the object
 */
uint32_t BitString_Value_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;

    if (object_instance > BACNET_MAX_INSTANCE) {
        return BACNET_MAX_INSTANCE;
    } else if (object_instance == BACNET_MAX_INSTANCE) {
        /* wildcard instance */
        /* the Object_Identifier property of the newly created object
            shall be initialized to a value that is unique within the
            responding BACnet-user device. The method used to generate
            the object identifier is a local matter.*/
        object_instance = Keylist_Next_Empty_Key(Object_List, 1);
    }

    pObject = BitString_Value_Object(object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (pObject) {
            pObject->Object_Name = NULL;
            pObject->Description = NULL;
            bitstring_init(&pObject->Present_Value);
            pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
            pObject->Out_Of_Service = false;
            pObject->Change_Of_Value = false;
            pObject->Write_Enabled = true;
            /* add to list */
            index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if (index < 0) {
                free(pObject);
                return BACNET_MAX_INSTANCE;
            }
        } else {
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * Deletes an BitString Value object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool BitString_Value_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * Deletes all the BitString Value objects and their data
 */
void BitString_Value_Cleanup(void)
{
    struct object_data *pObject;

    if (Object_List) {
        do {
            pObject = Keylist_Data_Pop(Object_List);
            if (pObject) {
                free(pObject);
            }
        } while (pObject);
        Keylist_Delete(Object_List);
        Object_List = NULL;
    }
}

/**
 * Initializes the object data
 */
void BitString_Value_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
