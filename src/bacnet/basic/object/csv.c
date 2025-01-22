/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @brief A basic BACnet CharacterString Value object with a CharacterString
 * as the datatype for the present-value property.
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
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/object/csv.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List = NULL;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_CHARACTERSTRING_VALUE;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    /* list of the required properties */
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,  PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,     PROP_STATUS_FLAGS, -1
};

static const int Properties_Optional[] = {
    /* list of the optional properties */
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, PROP_DESCRIPTION, -1
};

static const int Properties_Proprietary[] = { -1 };

typedef struct characterstring_object {
    /* Writable out-of-service allows others to manipulate our Present Value */
    bool Out_Of_Service : 1;
    bool Changed : 1;
    uint32_t Instance;
    /* Here is our Present Value */
    BACNET_CHARACTER_STRING Present_Value_Backup;
    BACNET_CHARACTER_STRING Present_Value;
    const char *Object_Name;
    const char *Description;
} CHARACTERSTRING_VALUE_DESCR;

/**
 * Initialize the pointers for the required, the optional and the properitary
 * value properties.
 *
 * @param pRequired - Pointer to the pointer of required values.
 * @param pOptional - Pointer to the pointer of optional values.
 * @param pProprietary - Pointer to the pointer of properitary values.
 */
void CharacterString_Value_Property_Lists(
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
 * @brief Creates a CharacterString Value object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t CharacterString_Value_Create(uint32_t object_instance)
{
    struct characterstring_object *pObject = NULL;

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
    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct characterstring_object));
        if (pObject) {
            /* add to list */
            int index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if (index < 0) {
                free(pObject);
                return BACNET_MAX_INSTANCE;
            }
            pObject->Object_Name = NULL;
            pObject->Description = NULL;
            characterstring_init_ansi(&pObject->Present_Value, "");
            characterstring_init_ansi(&pObject->Present_Value_Backup, "");
            pObject->Out_Of_Service = false;
            pObject->Changed = false;
        } else {
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * @brief Delete an object and its data from the object list
 * @param  object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Characterstring_Value_Delete(uint32_t object_instance)
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
 * @brief Cleans up the object list and its data
 */
void Characterstring_Value_Cleanup(void)
{
    struct object_data *pObject = NULL;

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
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct characterstring_object *
CharacterString_Value_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * Initialize the character string values.
 */
void CharacterString_Value_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
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
unsigned CharacterString_Value_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * We simply have 0-n object instances. Yours might be
 * more complex, and then you need to return the index
 * that correlates to the correct instance number
 *
 * @param index Object index
 *
 * @return Index in the object table.
 */
uint32_t CharacterString_Value_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * Return the count of character string values.
 *
 * @return Count of character string values.
 */
unsigned CharacterString_Value_Count(void)
{
    return Keylist_Count(Object_List);
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
bool CharacterString_Value_Valid_Instance(uint32_t object_instance)
{
    struct characterstring_object *pObject =
        CharacterString_Value_Object(object_instance);

    return (pObject != NULL);
}

/**
 * For a given object instance-number, read the present-value.
 *
 * @param  object_instance - object-instance number of the object
 * @param  present_value - Pointer to the new BACnet string value,
 *                         taking the value.
 *
 * @return  true if values are within range and present-value
 *          is returned.
 */
bool CharacterString_Value_Present_Value(
    uint32_t object_instance, BACNET_CHARACTER_STRING *present_value)
{
    bool status = false;
    struct characterstring_object *pObject =
        CharacterString_Value_Object(object_instance);

    if (pObject) {
        status = characterstring_copy(present_value, &pObject->Present_Value);
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value,
 * taken from another BACnet string.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - Pointer to the new BACnet string value.
 *
 * @return  true if values are within range and present-value is set.
 */
bool CharacterString_Value_Present_Value_Set(
    uint32_t object_instance, const BACNET_CHARACTER_STRING *present_value)
{
    bool status = false;
    struct characterstring_object *pObject =
        CharacterString_Value_Object(object_instance);

    if (pObject) {
        pObject->Changed =
            !characterstring_same(&pObject->Present_Value, present_value);

        status = characterstring_copy(&pObject->Present_Value, present_value);
    }

    return status;
}

/**
 * For a given object instance-number, sets the backed up present-value,
 * taken from another BACnet string.
 *
 * @param  object_instance - object-instance number of the object
 * @param  present_value - Pointer to the new BACnet string value.
 *
 * @return  true if values are within range and present-value is set.
 */

bool CharacterString_Value_Present_Value_Backup_Set(
    uint32_t object_instance, BACNET_CHARACTER_STRING *present_value)
{
    bool status = false;
    struct characterstring_object *pObject =
        CharacterString_Value_Object(object_instance);

    if (pObject) {
        status =
            characterstring_copy(&pObject->Present_Value_Backup, present_value);
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
bool CharacterString_Value_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct characterstring_object *pObject =
        CharacterString_Value_Object(object_instance);

    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * For a given object instance-number, set the out of service value.
 *
 * @param  object_instance - object-instance number of the object
 * @param  true/false
 */
void CharacterString_Value_Out_Of_Service_Set(
    uint32_t object_instance, bool value)
{
    struct characterstring_object *pObject =
        CharacterString_Value_Object(object_instance);

    if (pObject) {
        if (pObject->Out_Of_Service != value) {
            pObject->Changed = true;
            /* Lets backup Present_Value when going Out_Of_Service  or restore
             * when going out of Out_Of_Service */
            if ((pObject->Out_Of_Service = value)) {
                characterstring_copy(
                    &pObject->Present_Value_Backup, &pObject->Present_Value);
            } else {
                characterstring_copy(
                    &pObject->Present_Value, &pObject->Present_Value_Backup);
            }
            pObject->Out_Of_Service = value;
        }
    }

    return;
}

/**
 * @brief Get the COV change flag status
 * @param object_instance - object-instance number of the object
 * @return the COV change flag status
 */
bool CharacterString_Value_Change_Of_Value(uint32_t object_instance)
{
    bool changed = false;
    struct characterstring_object *pObject =
        CharacterString_Value_Object(object_instance);

    if (pObject) {
        changed = pObject->Changed;
    }

    return changed;
}

/**
 * @brief Clear the COV change flag
 * @param object_instance - object-instance number of the object
 */
void CharacterString_Value_Change_Of_Value_Clear(uint32_t object_instance)
{
    struct characterstring_object *pObject =
        CharacterString_Value_Object(object_instance);

    if (pObject) {
        pObject->Changed = false;
    }
}

/**
 * @brief For a given object instance-number, loads the value_list with the COV
 * data.
 * @param  object_instance - object-instance number of the object
 * @param  value_list - list of COV data
 * @return  true if the value list is encoded
 */
bool CharacterString_Value_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;
    const bool in_alarm = false;
    const bool fault = false;
    const bool overridden = false;
    struct characterstring_object *pObject =
        CharacterString_Value_Object(object_instance);

    if (pObject) {
        status = cov_value_list_encode_character_string(
            value_list, &pObject->Present_Value, in_alarm, fault, overridden,
            pObject->Out_Of_Service);
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
const char *CharacterString_Value_Description(uint32_t object_instance)
{
    const char *name = NULL;
    const struct characterstring_object *pObject;

    pObject = CharacterString_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Description == NULL) {
            name = "";
        } else {
            name = pObject->Description;
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
bool CharacterString_Value_Description_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct characterstring_object *pObject;

    pObject = CharacterString_Value_Object(object_instance);
    if (pObject) {
        status = true;
        pObject->Description = new_name;
    }

    return status;
}

/**
 * @brief Get the object name
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name to be retrieved
 * @return  true if object-name was retrieved
 */
bool CharacterString_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[64] = "";
    bool status = false;
    struct characterstring_object *pObject;

    pObject = CharacterString_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Object_Name == NULL) {
            snprintf(
                text, sizeof(text), "CHARACTER STRING VALUE %lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text);
        } else {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the object-name
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 * @return  true if object-name was set
 */
bool CharacterString_Value_Name_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct characterstring_object *pObject;

    pObject = CharacterString_Value_Object(object_instance);
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
const char *CharacterString_Value_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct characterstring_object *pObject;

    pObject = CharacterString_Value_Object(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * Return the requested property of the character string value.
 *
 * @param rpdata  Property requested, see for BACNET_READ_PROPERTY_DATA details.
 *
 * @return apdu len, or BACNET_STATUS_ERROR on error
 */
int CharacterString_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    bool state = false;
    uint8_t *apdu = NULL;
    struct characterstring_object *pObject = NULL;

    /* Valid data? */
    if (rpdata == NULL) {
        return 0;
    }
    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    pObject = CharacterString_Value_Object(rpdata->object_instance);
    /* Valid object? */
    if (!pObject) {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return BACNET_STATUS_ERROR;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
            if (CharacterString_Value_Object_Name(
                    rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                CharacterString_Value_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], OBJECT_CHARACTERSTRING_VALUE);
            break;
        case PROP_PRESENT_VALUE:
            CharacterString_Value_Present_Value(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            if (CharacterString_Value_Out_Of_Service(rpdata->object_instance)) {
                bitstring_set_bit(
                    &bit_string, STATUS_FLAG_OUT_OF_SERVICE, true);
            } else {
                bitstring_set_bit(
                    &bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
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
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_STATE_TEXT) &&
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
bool CharacterString_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;

    struct characterstring_object *pObject = NULL;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

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

    pObject = CharacterString_Value_Object(wp_data->object_instance);
    /* Valid object? */
    if (!pObject) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (status) {
                status = CharacterString_Value_Present_Value_Set(
                    wp_data->object_instance, &value.type.Character_String);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                CharacterString_Value_Out_Of_Service_Set(
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
