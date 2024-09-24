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
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/object/csv.h"
#include "bacnet/basic/services.h"

/* number of demo objects */
#ifndef MAX_CHARACTERSTRING_VALUES
#define MAX_CHARACTERSTRING_VALUES 1
#endif

/* Here is our Present Value */
static BACNET_CHARACTER_STRING Present_Value_Backup[MAX_CHARACTERSTRING_VALUES];
static BACNET_CHARACTER_STRING Present_Value[MAX_CHARACTERSTRING_VALUES];
/* Writable out-of-service allows others to manipulate our Present Value */
static bool Out_Of_Service[MAX_CHARACTERSTRING_VALUES];
static bool Changed[MAX_CHARACTERSTRING_VALUES];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,  PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,     PROP_STATUS_FLAGS, -1
};

static const int Properties_Optional[] = { PROP_EVENT_STATE,
                                           PROP_OUT_OF_SERVICE,
                                           PROP_DESCRIPTION, -1 };

static const int Properties_Proprietary[] = { -1 };

typedef struct CharacterString_Value_descr {
    uint32_t Instance;
    char Name[MAX_CHARACTER_STRING_BYTES];
    char Description[MAX_CHARACTER_STRING_BYTES];
} CHARACTERSTRING_VALUE_DESCR;

static CHARACTERSTRING_VALUE_DESCR CSV_Descr[MAX_CHARACTERSTRING_VALUES];
static int CSV_Max_Index = MAX_CHARACTERSTRING_VALUES;

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
 * Initialize the character string values.
 */
void CharacterString_Value_Init(void)
{
    unsigned i;

    /* initialize all Present Values */
    for (i = 0; i < MAX_CHARACTERSTRING_VALUES; i++) {
        memset(&CSV_Descr[i], 0x00, sizeof(CHARACTERSTRING_VALUE_DESCR));
        snprintf(
            CSV_Descr[i].Name, sizeof(CSV_Descr[i].Name),
            "CHARACTER STRING VALUE %u", i + 1);
        snprintf(
            CSV_Descr[i].Description, sizeof(CSV_Descr[i].Description),
            "A Character String Value Example");
        CSV_Descr[i].Instance =
            BACNET_INSTANCE(BACNET_ID_VALUE(i, OBJECT_CHARACTERSTRING_VALUE));
        characterstring_init_ansi(&Present_Value[i], "");
        Changed[i] = false;
    }

    return;
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
    unsigned index = 0;

    for (;
         index < CSV_Max_Index && CSV_Descr[index].Instance != object_instance;
         index++)
        ;

    return index;
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
    if (index < CSV_Max_Index) {
        return CSV_Descr[index].Instance;
    } else {
        PRINT("index out of bounds %d", CSV_Descr[index].Instance);
    }

    return 0;
}

/**
 * Return the count of character string values.
 *
 * @return Count of character string values.
 */
unsigned CharacterString_Value_Count(void)
{
    return CSV_Max_Index;
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
    unsigned index = 0; /* offset from instance lookup */

    index = CharacterString_Value_Instance_To_Index(object_instance);

    if (index >= CSV_Max_Index) {
        return false;
    }

    return object_instance == CSV_Descr[index].Instance;
}

/**
 * Initialize the charactestring inputs. Returns false if there are errors.
 *
 * @param pInit_data pointer to initialisation values
 *
 * @return true/false
 */
bool CharacterString_Value_Set(BACNET_OBJECT_LIST_INIT_T *pInit_data)
{
    unsigned i;

    if (!pInit_data) {
        return false;
    }

    if ((int)pInit_data->length > MAX_CHARACTERSTRING_VALUES) {
        PRINT(
            "pInit_data->length = %d >= %d", (int)pInit_data->length,
            MAX_CHARACTERSTRING_VALUES);
        return false;
    }

    for (i = 0; i < pInit_data->length; i++) {
        if (pInit_data->Object_Init_Values[i].Object_Instance <
            BACNET_MAX_INSTANCE) {
            CSV_Descr[i].Instance =
                pInit_data->Object_Init_Values[i].Object_Instance;
        } else {
            PRINT(
                "Object instance %u is too big",
                pInit_data->Object_Init_Values[i].Object_Instance);
            return false;
        }

        strncpy(
            CSV_Descr[i].Name, pInit_data->Object_Init_Values[i].Object_Name,
            sizeof(CSV_Descr[i].Name) - 1);

        strncpy(
            CSV_Descr[i].Description,
            pInit_data->Object_Init_Values[i].Description,
            sizeof(CSV_Descr[i].Description) - 1);
    }

    CSV_Max_Index = (int)pInit_data->length;

    return true;
}

/**
 * For a given object instance-number, read the present-value.
 *
 * @param  object_instance - object-instance number of the object
 * @param  present_value - Pointer to the new BACnet string value,
 *                       taking the value.
 *
 * @return  true if values are within range and present-value
 *          is returned.
 */
bool CharacterString_Value_Present_Value(
    uint32_t object_instance, BACNET_CHARACTER_STRING *present_value)
{
    bool status = false;
    unsigned index = 0; /* offset from instance lookup */

    index = CharacterString_Value_Instance_To_Index(object_instance);

    if (index < CSV_Max_Index) {
        status = characterstring_copy(present_value, &Present_Value[index]);
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value,
 * taken from another BACnet string.
 *
 * @param  object_instance - object-instance number of the object
 * @param  present_value - Pointer to the new BACnet string value.
 *
 * @return  true if values are within range and present-value is set.
 */

bool CharacterString_Value_Present_Value_Set(
    uint32_t object_instance, const BACNET_CHARACTER_STRING *present_value)
{
    bool status = false;
    unsigned index = 0; /* offset from instance lookup */

    index = CharacterString_Value_Instance_To_Index(object_instance);

    if (index < CSV_Max_Index) {
        if (!characterstring_same(&Present_Value[index], present_value)) {
            Changed[index] = true;
        }
        status = characterstring_copy(&Present_Value[index], present_value);
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
    unsigned index = CharacterString_Value_Instance_To_Index(object_instance);

    if (index < CSV_Max_Index) {
        status =
            characterstring_copy(&Present_Value_Backup[index], present_value);
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
    unsigned index = 0;

    index = CharacterString_Value_Instance_To_Index(object_instance);
    if (index < MAX_CHARACTERSTRING_VALUES) {
        value = Out_Of_Service[index];
    }

    return value;
}

/**
 * For a given object instance-number, set the out of service value.
 *
 * @param  object_instance - object-instance number of the object
 * @param  true/false
 */
static void
CharacterString_Value_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    unsigned index = 0;

    index = CharacterString_Value_Instance_To_Index(object_instance);
    if (index < MAX_CHARACTERSTRING_VALUES) {
        if (Out_Of_Service[index] != value) {
            Changed[index] = true;
            /* Lets backup Present_Value when going Out_Of_Service  or restore
             * when going out of Out_Of_Service */
            if ((Out_Of_Service[index] = value)) {
                Present_Value_Backup[index] = Present_Value[index];
            } else {
                Present_Value[index] = Present_Value_Backup[index];
            }
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
    unsigned index = 0; /* offset from instance lookup */

    index = CharacterString_Value_Instance_To_Index(object_instance);
    if (index < MAX_CHARACTERSTRING_VALUES) {
        changed = Changed[index];
    }

    return changed;
}

/**
 * @brief Clear the COV change flag
 * @param object_instance - object-instance number of the object
 */
void CharacterString_Value_Change_Of_Value_Clear(uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */

    index = CharacterString_Value_Instance_To_Index(object_instance);
    if (index < MAX_CHARACTERSTRING_VALUES) {
        Changed[index] = false;
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
    unsigned index = 0; /* offset from instance lookup */

    index = CharacterString_Value_Instance_To_Index(object_instance);
    if (index < MAX_CHARACTERSTRING_VALUES) {
        status = cov_value_list_encode_character_string(
            value_list, &Present_Value[index], in_alarm, fault, overridden,
            Out_Of_Service[index]);
    }

    return status;
}

/**
 * For a given object instance-number, return the description.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  C-string pointer to the description.
 */
static char *CharacterString_Value_Description(uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */

    index = CharacterString_Value_Instance_To_Index(object_instance);
    if (index < CSV_Max_Index) {
        return CSV_Descr[index].Description;
    }

    return 0;
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
bool CharacterString_Value_Description_Set(
    uint32_t object_instance, BACNET_CHARACTER_STRING *new_descr)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false; /* return value */

    index = CharacterString_Value_Instance_To_Index(object_instance);
    if (index < CSV_Max_Index) {
        status = true;
        if (new_descr) {
            strncpy(
                CSV_Descr[index].Description, new_descr->value,
                sizeof(CSV_Descr[index].Description));
        } else {
            memset(
                &CSV_Descr->Description[index], 0,
                sizeof(CSV_Descr->Description[index]));
        }
    }

    return status;
}

/**
 * For a given object instance-number, return the object text.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - Pointer to the BACnet string object that shall take the
 * object name
 *
 * @return True on success, false otherwise.
 */
bool CharacterString_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    if (!object_name) {
        return false;
    }

    index = CharacterString_Value_Instance_To_Index(object_instance);
    if (index < CSV_Max_Index) {
        status = characterstring_init_ansi(object_name, CSV_Descr[index].Name);
    }

    return status;
}

/**
 * For a given object instance-number, set the object text.
 *
 * Note: the object name must be unique within this device
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - C-String pointer to the text, representing the object name
 *
 * @return True on success, false otherwise.
 */
bool CharacterString_Value_Name_Set(
    uint32_t object_instance, const char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false; /* return value */

    index = CharacterString_Value_Instance_To_Index(object_instance);
    if (index < CSV_Max_Index) {
        status = true;
        /* FIXME: check to see if there is a matching name */
        if (new_name) {
            strncpy(
                CSV_Descr[index].Name, new_name, sizeof(CSV_Descr[index].Name) - 1);
        }
    } else {
        memset(&CSV_Descr[index].Name, 0, sizeof(CSV_Descr[index].Name));
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
int CharacterString_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index = 0;
    bool state = false;
    uint8_t *apdu = NULL;

    /* Valid data? */
    if (rpdata == NULL) {
        return 0;
    }
    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    /* Valid object? */
    object_index =
        CharacterString_Value_Instance_To_Index(rpdata->object_instance);
    if (object_index >= MAX_CHARACTERSTRING_VALUES) {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return BACNET_STATUS_ERROR;
    }

    apdu = rpdata->application_data;

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_CHARACTERSTRING_VALUE,
                rpdata->object_instance);
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
            if (characterstring_init_ansi(
                    &char_string,
                    CharacterString_Value_Description(
                        rpdata->object_instance))) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], OBJECT_CHARACTERSTRING_VALUE);
            break;
        case PROP_PRESENT_VALUE:
            if (CharacterString_Value_Present_Value(
                    rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
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
#if (CHARACTERSTRING_INTRINSIC_REPORTING)
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
#endif
        case PROP_OUT_OF_SERVICE:
            state = Out_Of_Service[object_index];
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
    unsigned object_index = 0;
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

    /* Valid object? */
    object_index =
        CharacterString_Value_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_CHARACTERSTRING_VALUES) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (CharacterString_Value_Out_Of_Service(
                    wp_data->object_instance) == true) {
                if (status) {
                    status = CharacterString_Value_Present_Value_Set(
                        wp_data->object_instance, &value.type.Character_String);
                    if (!status) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                status = false;
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
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
#if (CHARACTERSTRING_INTRINSIC_REPORTING)
        case PROP_EVENT_STATE:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
#endif
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}
