/**
 * @file
 * @brief Command objects, customize for your use
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2026
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
#include <stdlib.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaction.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/lighting.h"
#include "bacnet/proplist.h"
#include "bacnet/timestamp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* BACnet Stack Objects */
#include "bacnet/basic/object/device.h"
/* me!*/
#include "bacnet/basic/object/command.h"

struct object_data {
    uint32_t Present_Value;
    bool In_Process;
    bool All_Writes_Successful;
    char *Description;
    char *Object_Name;
    OS_Keylist Action_List;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_Lists[MAX_NUM_DEVICES];
#ifdef BAC_ROUTING
#define Object_List (Object_Lists[Routed_Device_Object_Index()])
#else
#define Object_List (Object_Lists[0])
#endif

/**
 * @brief Get Command object data by object instance.
 * @param object_instance [in] BACnet object instance number.
 * @return Pointer to object data, or NULL if not found.
 */
static struct object_data *Object_Data(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Free all action entries and delete an action keylist.
 * @param list [in] Action list keylist.
 */
static void Action_List_Free(OS_Keylist list)
{
    BACNET_ACTION_LIST *pAction;

    if (list) {
        do {
            pAction = Keylist_Data_Pop(list);
            free(pAction);
        } while (pAction);
        Keylist_Delete(list);
    }
}

/**
 * @brief Initialize the action list for a Command object.
 * @param pObject [in,out] Pointer to object data.
 * @return true if all action entries were allocated and added.
 */
static bool Action_List_Init(struct object_data *pObject)
{
    BACNET_ACTION_LIST *pAction = NULL;
    int index = 0;
    unsigned i = 0;

    if (!pObject) {
        return false;
    }
    pObject->Action_List = Keylist_Create();
    if (!pObject->Action_List) {
        return false;
    }
    for (i = 0; i < MAX_COMMAND_ACTIONS; i++) {
        pAction = calloc(1, sizeof(BACNET_ACTION_LIST));
        if (!pAction) {
            Action_List_Free(pObject->Action_List);
            pObject->Action_List = NULL;
            return false;
        }
        index = Keylist_Data_Add(pObject->Action_List, i, pAction);
        if (index < 0) {
            free(pAction);
            Action_List_Free(pObject->Action_List);
            pObject->Action_List = NULL;
            return false;
        }
    }

    return true;
}

/**
 * @brief Free all memory owned by a Command object data record.
 * @param pObject [in] Pointer to object data.
 */
static void Object_Data_Free(struct object_data *pObject)
{
    if (pObject) {
        free(pObject->Description);
        free(pObject->Object_Name);
        Action_List_Free(pObject->Action_List);
        free(pObject);
    }
}

/**
 * @brief Add a Command object instance if it is not already present.
 * @param object_instance [in] BACnet object instance number.
 * @return true if the instance exists after the call.
 */
static bool Command_Object_Instance_Add(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;
    int index = 0;

    if (object_instance >= MAX_COMMANDS) {
        return false;
    }
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
    if (!Object_List) {
        return false;
    }
    pObject = Object_Data(object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (!pObject) {
            return false;
        }
        pObject->Description = NULL;
        pObject->Object_Name = NULL;
        pObject->All_Writes_Successful = true;
        if (!Action_List_Init(pObject)) {
            free(pObject);
            return false;
        }
        index = Keylist_Data_Add(Object_List, object_instance, pObject);
        if (index < 0) {
            Object_Data_Free(pObject);
            return false;
        }
    }
    status = true;

    return status;
}

/* These arrays are used by the ReadPropertyMultiple handler */
static const int32_t Command_Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_IN_PROCESS,
    PROP_ALL_WRITES_SUCCESSFUL,
    PROP_ACTION,
    -1
};

static const int32_t Command_Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int32_t Command_Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of always writable properties */
    PROP_PRESENT_VALUE, PROP_OBJECT_NAME, PROP_DESCRIPTION, -1
};

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
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
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
 * @brief Get the list of writable properties for an Command object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Command_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * Initializes the Command object data
 */
void Command_Init(void)
{
    uint16_t dev_id;
    uint32_t i;
#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
#endif

    Command_Cleanup();
    for (dev_id = 0; dev_id < MAX_NUM_DEVICES; dev_id++) {
#ifdef BAC_ROUTING
        Set_Routed_Device_Object_Index(dev_id);
#endif
        if (!Object_List) {
            Object_List = Keylist_Create();
        }
        if (Object_List) {
            for (i = 0; i < MAX_COMMANDS; i++) {
                (void)Command_Create(i);
            }
        }
    }

#ifdef BAC_ROUTING
    Set_Routed_Device_Object_Index(current_dev_id);
#endif
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
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
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
    return Keylist_Count(Object_List);
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
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
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
    return Keylist_Index(Object_List, object_instance);
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
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Present_Value;
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
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Present_Value = value;
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
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->In_Process;
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
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->In_Process = value;
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
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->All_Writes_Successful;
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
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->All_Writes_Successful = value;
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
    struct object_data *pObject;
    bool status = false;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                text, sizeof(text), "COMMAND %lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text);
        }
    }

    return status;
}

/**
 * @brief Set the Command object-name for an instance.
 * @param object_instance [in] BACnet object instance number.
 * @param new_name [in] New object-name as a C string.
 * @return true if the name was set.
 */
bool Command_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        free(pObject->Object_Name);
        if (new_name) {
            pObject->Object_Name = bacnet_strdup(new_name);
            status = (pObject->Object_Name != NULL);
        } else {
            pObject->Object_Name = NULL;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Get the Command description as a C string.
 * @param instance [in] BACnet object instance number.
 * @return Description string, empty string, or NULL if object not found.
 */
const char *Command_Description(uint32_t instance)
{
    struct object_data *pObject;

    pObject = Object_Data(instance);
    if (pObject) {
        if (pObject->Description) {
            return pObject->Description;
        }

        return "";
    }

    return NULL;
}

/**
 * @brief Set the object-name property value using write-property context.
 * @param wp_data [in,out] Write property request/response context.
 * @param cstring [in] New object-name value.
 * @return true if object-name was set.
 */
static bool Command_Object_Name_Write(
    BACNET_WRITE_PROPERTY_DATA *wp_data, BACNET_CHARACTER_STRING *cstring)
{
    bool status = false;
    struct object_data *pObject;
    char *utf8_name = NULL;

    pObject = Object_Data(wp_data->object_instance);
    if (pObject) {
        utf8_name =
            write_property_characterstring_utf8_strdup(wp_data, cstring);
        if (utf8_name) {
            free(pObject->Object_Name);
            pObject->Object_Name = utf8_name;
            status = true;
        }
    } else {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * @brief Set the description property value using write-property context.
 * @param wp_data [in,out] Write property request/response context.
 * @param cstring [in] New description value.
 * @return true if description was set.
 */
static bool Command_Description_Write(
    BACNET_WRITE_PROPERTY_DATA *wp_data, BACNET_CHARACTER_STRING *cstring)
{
    bool status = false;
    struct object_data *pObject;
    char *utf8_name = NULL;

    pObject = Object_Data(wp_data->object_instance);
    if (pObject) {
        utf8_name =
            write_property_characterstring_utf8_strdup(wp_data, cstring);
        if (utf8_name) {
            free(pObject->Description);
            pObject->Description = utf8_name;
            status = true;
        }
    } else {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * @brief Set the Command description for an instance.
 * @param instance [in] BACnet object instance number.
 * @param new_name [in] New description as a C string.
 * @return true if the description was set.
 */
bool Command_Description_Set(uint32_t instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(instance);
    if (pObject) {
        free(pObject->Description);
        if (new_name) {
            pObject->Description = bacnet_strdup(new_name);
            status = (pObject->Description != NULL);
        } else {
            pObject->Description = NULL;
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the object data
 * @param object_instance [in] BACnet network port object instance number
 * @return pointer to the object data
 */
BACNET_ACTION_LIST *Command_Action_List_Entry(uint32_t instance, unsigned index)
{
    struct object_data *pObject;
    BACNET_ACTION_LIST *pAction = NULL;

    pObject = Object_Data(instance);
    if (pObject && (index < MAX_COMMAND_ACTIONS)) {
        pAction = Keylist_Data(pObject->Action_List, index);
    }

    return pAction;
}

/**
 * @brief For a given object instance-number, returns the number of actions
 */
unsigned Command_Action_List_Count(uint32_t instance)
{
    unsigned count = 0;
    struct object_data *pObject;

    pObject = Object_Data(instance);
    if (pObject) {
        count = Keylist_Count(pObject->Action_List);
    }

    return count;
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
    BACNET_ACTION_LIST *pAction;

    pAction = Command_Action_List_Entry(object_instance, index);
    if (pAction) {
        apdu_len = bacnet_action_command_encode(apdu, pAction);
    }

    return apdu_len;
}

/**
 * @brief Resize the Command Action_List for an object instance.
 * @param pObject [in,out] Pointer to object data.
 * @param new_array_size [in] New number of action entries.
 * @return BACNET_ERROR_CODE_SUCCESS on success, else error code.
 */
static BACNET_ERROR_CODE Command_Action_List_Resize(
    struct object_data *pObject, BACNET_UNSIGNED_INTEGER new_array_size)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    BACNET_ACTION_LIST *pAction;
    BACNET_UNSIGNED_INTEGER old_array_size = 0;
    KEY key = 0;
    int index = 0;

    if (!pObject) {
        return ERROR_CODE_UNKNOWN_OBJECT;
    }
    if (new_array_size > MAX_COMMAND_ACTIONS) {
        return ERROR_CODE_VALUE_OUT_OF_RANGE;
    }
    old_array_size = Keylist_Count(pObject->Action_List);
    if (new_array_size < old_array_size) {
        key = new_array_size;
        while (key < old_array_size) {
            pAction = Keylist_Data_Delete(pObject->Action_List, key);
            free(pAction);
            key++;
        }
    } else if (new_array_size > old_array_size) {
        key = old_array_size;
        while (key < new_array_size) {
            pAction = calloc(1, sizeof(BACNET_ACTION_LIST));
            if (!pAction) {
                error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                break;
            }
            index = Keylist_Data_Add(pObject->Action_List, key, pAction);
            if (index < 0) {
                free(pAction);
                error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                break;
            }
            key++;
        }
    }

    return error_code;
}

/**
 * @brief Decode a Command Action_List member element to determine length.
 * @param object_instance [in] BACnet object instance number.
 * @param apdu [in] Buffer containing encoded element value.
 * @param apdu_size [in] Size of buffer.
 * @return Decoded length, or BACNET_STATUS_ERROR on error.
 */
static int Command_Action_List_Member_Decode(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    int len = BACNET_STATUS_ERROR;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        len = bacnet_action_command_decode(apdu, apdu_size, NULL);
    }

    return len;
}

/**
 * @brief Write a value to a Command Action_List BACnetARRAY element.
 * @param object_instance [in] BACnet object instance number.
 * @param array_index [in] Array index to write.
 * @param array_size [in] Array size, used for index 0 writes.
 * @param apdu [in] Encoded element value.
 * @param apdu_size [in] Size of encoded element value.
 * @return BACNET_ERROR_CODE value.
 */
static BACNET_ERROR_CODE Command_Action_List_Member_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *apdu,
    size_t apdu_size)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_ACTION_LIST action = { 0 };
    BACNET_ACTION_LIST *pAction;
    int len = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (array_index == 0) {
            error_code = Command_Action_List_Resize(pObject, array_size);
        } else {
            array_index--; /* array index is 1..N, but we want 0..(N-1) */
            if (array_index >= MAX_COMMAND_ACTIONS) {
                return ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
            len = bacnet_action_command_decode(apdu, apdu_size, &action);
            if (len > 0) {
                pAction = Keylist_Data(pObject->Action_List, array_index);
                if (pAction) {
                    *pAction = action;
                    pAction->next = NULL;
                    pAction->Value.next = NULL;
                    error_code = ERROR_CODE_SUCCESS;
                } else {
                    error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                }
            } else {
                error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
        }
    }

    return error_code;
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

        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, Command_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
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
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Command_Action_List_Encode,
                Command_Action_List_Count(rpdata->object_instance), apdu,
                apdu_size);
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
    BACNET_UNSIGNED_INTEGER array_size = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    /* Valid data? */
    if (wp_data == NULL) {
        return false;
    }
    /* decode the some of the request */
    len = bacapp_decode_known_array_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        OBJECT_COMMAND, wp_data->object_property, wp_data->array_index);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    object_index = Command_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_COMMANDS) {
        return false;
    }

    switch ((int)wp_data->object_property) {
        case PROP_OBJECT_NAME:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (status) {
                status = Command_Object_Name_Write(
                    wp_data, &value.type.Character_String);
            }
            break;
        case PROP_DESCRIPTION:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (status) {
                status = Command_Description_Write(
                    wp_data, &value.type.Character_String);
            }
            break;
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int >=
                    Command_Action_List_Count(wp_data->object_instance)) {
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
        case PROP_ACTION:
            array_size = Command_Action_List_Count(wp_data->object_instance);
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                Command_Action_List_Member_Decode,
                Command_Action_List_Member_Write, array_size,
                wp_data->application_data, wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
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

/**
 * @brief Intrinsic reporting task hook for Command object.
 * @param object_instance [in] BACnet object instance number.
 */
void Command_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}

/**
 * @brief Create a Command object instance.
 * @param object_instance [in] Instance number, or BACNET_MAX_INSTANCE for
 * wildcard allocation.
 * @return Object instance on success, or BACNET_MAX_INSTANCE on failure.
 */
uint32_t Command_Create(uint32_t object_instance)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
    if (!Object_List) {
        return BACNET_MAX_INSTANCE;
    }
    if (object_instance > BACNET_MAX_INSTANCE) {
        return BACNET_MAX_INSTANCE;
    } else if (object_instance == BACNET_MAX_INSTANCE) {
        object_instance = Keylist_Next_Empty_Key(Object_List, 0);
    }
    if (object_instance >= MAX_COMMANDS) {
        return BACNET_MAX_INSTANCE;
    }
    if (!Command_Object_Instance_Add(object_instance)) {
        return BACNET_MAX_INSTANCE;
    }

    return object_instance;
}

/**
 * @brief Delete a Command object instance.
 * @param object_instance [in] BACnet object instance number.
 * @return true if the object was deleted.
 */
bool Command_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        Object_Data_Free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Delete all Command objects for all routed device contexts.
 */
void Command_Cleanup(void)
{
    struct object_data *pObject;
    uint16_t dev_id;
#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
#endif

    for (dev_id = 0; dev_id < MAX_NUM_DEVICES; dev_id++) {
#ifdef BAC_ROUTING
        Set_Routed_Device_Object_Index(dev_id);
#endif
        if (Object_List) {
            do {
                pObject = Keylist_Data_Pop(Object_List);
                Object_Data_Free(pObject);
            } while (pObject);
            Keylist_Delete(Object_List);
            Object_List = NULL;
        }
    }

#ifdef BAC_ROUTING
    Set_Routed_Device_Object_Index(current_dev_id);
#endif
}
