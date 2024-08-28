/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2014
 * @brief Command objects, customize for your use
 * @details The Command object type defines a standardized object whose
 * properties represent the externally visible characteristics of a
 * multi-action command procedure. A Command object is used to
 * write a set of values to a group of object properties, based on
 * the "action code" that is written to the Present_Value of the
 * Command object. Whenever the Present_Value property of the
 * Command object is written to, it triggers the Command object
 * to take a set of actions that change the values of a set of other
 * objects' properties.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaction.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/lighting.h"
#include "bacnet/proplist.h"
#include "bacnet/timestamp.h"
#include "bacnet/basic/services.h"
/* me!*/
#include "bacnet/basic/object/command.h"

static COMMAND_DESCR Command_Descr[MAX_COMMANDS];

/* clang-format off */
/* These arrays are used by the ReadPropertyMultiple handler */
static const int Command_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_IN_PROCESS,
    PROP_ALL_WRITES_SUCCESSFUL,
    PROP_ACTION,
    -1 };

static const int Command_Properties_Optional[] = { -1 };

static const int Command_Properties_Proprietary[] = { -1 };
/* clang-format on */

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
void Command_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Command_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Command_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Command_Properties_Proprietary;
    }

    return;
}

/**
 * Initializes the Command object data
 */
void Command_Init(void)
{
    unsigned i;
    for (i = 0; i < MAX_COMMANDS; i++) {
        Command_Descr[i].Present_Value = 0;
        Command_Descr[i].In_Process = false;
        Command_Descr[i].All_Writes_Successful = true; /* Optimistic default */
    }
}

/**
 * Determines if a given object instance is valid
 *
 * @param object_instance - object-instance number of the object
 *
 * @return true if the instance is valid, and false if not
 */
bool Command_Valid_Instance(uint32_t object_instance)
{
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        return true;
    }

    return false;
}

/**
 * Determines the number of this object instances
 *
 * @return Number of objects
 */
unsigned Command_Count(void)
{
    return MAX_COMMANDS;
}

/**
 * Determines the object instance-number for a given 0..N index
 * of objects where N is the total number of this object instances.
 *
 * @param index - 0..total number of this object instances
 *
 * @return object instance-number for the given index
 */
uint32_t Command_Index_To_Instance(unsigned index)
{
    return index;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of this object where N is the total number of this object instances
 *
 * @param object_instance - object-instance number of the object.
 *
 * @return index for the given instance-number, or
 * the total number of this object instances if not valid.
 */
unsigned Command_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_COMMANDS;

    if (object_instance < MAX_COMMANDS) {
        index = object_instance;
    }

    return index;
}

/**
 * For a given object instance-number, determines the present-value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  present-value of the object
 */
uint32_t Command_Present_Value(uint32_t object_instance)
{
    uint32_t value = 0;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        value = Command_Descr[index].Present_Value;
    }

    return value;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - present-value to set
 *
 * @return  true if values are within range and present-value is set.
 */
bool Command_Present_Value_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        Command_Descr[index].Present_Value = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, determines if the command
 * is in-process.  This true value indicates that the Command object has
 * begun processing one of a set of action sequences. Once all of the
 * writes have been attempted by the Command object, the In_Process
 * property shall be set back to false.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if this object-instance is in-process.
 */
bool Command_In_Process(uint32_t object_instance)
{
    bool value = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        value = Command_Descr[index].In_Process;
    }

    return value;
}

/**
 * For a given object instance-number, sets the command in-process value.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - true or false value to set
 *
 * @return  true if values are within range and in-process flag is set.
 */
bool Command_In_Process_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        Command_Descr[index].In_Process = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, indicates the success or failure
 * of the sequence of actions that are triggered when the Present_Value
 * property is written to.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if all writes were successful for this object-instance
 */
bool Command_All_Writes_Successful(uint32_t object_instance)
{
    bool value = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        value = Command_Descr[index].All_Writes_Successful;
    }

    return value;
}

/**
 * For a given object instance-number, sets the all-writes-successful value.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - true or false value to set
 *
 * @return  true if values are within range and all-writes-succcessful is set.
 */
bool Command_All_Writes_Successful_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        Command_Descr[index].All_Writes_Successful = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, loads the object-name into
 * a characterstring.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Command_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    unsigned int index;
    bool status = false;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        snprintf(
            text, sizeof(text), "COMMAND %lu", (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the object data
 * @param object_instance [in] BACnet network port object instance number
 * @return pointer to the object data
 */
static COMMAND_DESCR *Object_Data(uint32_t object_instance)
{
    unsigned int index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        return &Command_Descr[index];
    }

    return NULL;
}

BACNET_ACTION_LIST * Command_Action_List_Entry(
    uint32_t instance, unsigned index)
{
    COMMAND_DESCR *pObject;
    BACNET_ACTION_LIST *pAction = NULL;

    pObject = Object_Data(instance);
    if (pObject && (index < MAX_COMMAND_ACTIONS)) {
        pAction = &pObject->Action[index];
    }

    return pAction;
}

/**
 * @brief For a given object instance-number, returns the number of actions
 */
unsigned Command_Action_List_Count(
    uint32_t instance)
{
    (void)instance;
    return MAX_COMMAND_ACTIONS;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Command_Action_List_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    COMMAND_DESCR *pObject;

    pObject = Object_Data(object_instance);
    if (pObject && (index < MAX_COMMAND_ACTIONS)) {
        apdu_len = bacnet_action_command_encode(
            apdu, &pObject->Action[index]);
    }

    return apdu_len;
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
int Command_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
    int apdu_size = 0;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
    switch ((int)rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_COMMAND, rpdata->object_instance);
            break;

        case PROP_OBJECT_NAME:
            Command_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_COMMAND);
            break;

        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_unsigned(
                &apdu[0], Command_Present_Value(rpdata->object_instance));
            break;
        case PROP_IN_PROCESS:
            apdu_len = encode_application_boolean(
                &apdu[0], Command_In_Process(rpdata->object_instance));
            break;
        case PROP_ALL_WRITES_SUCCESSFUL:
            apdu_len = encode_application_boolean(
                &apdu[0],
                Command_All_Writes_Successful(rpdata->object_instance));
            break;
        case PROP_ACTION:
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index, Command_Action_List_Encode,
                MAX_COMMAND_ACTIONS, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_ACTION) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
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
bool Command_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
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
    /*  only array properties can have array options */
    if ((wp_data->object_property != PROP_ACTION) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    object_index = Command_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_COMMANDS) {
        return false;
    }

    switch ((int)wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int >= MAX_COMMAND_ACTIONS) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    return false;
                }
                Command_Present_Value_Set(
                    wp_data->object_instance, value.type.Unsigned_Int);
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                status = false;
            }
            break;
        default:
            if (property_lists_member(
                    Command_Properties_Required, Command_Properties_Optional,
                    Command_Properties_Proprietary, wp_data->object_property)) {
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

void Command_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
