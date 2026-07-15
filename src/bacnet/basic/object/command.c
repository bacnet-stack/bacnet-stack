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
#include <string.h>

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
    bool Action_Failed;
    char *Description;
    char *Object_Name;
    BACNET_ACTION_LIST *Action;
    uint32_t Action_Delay_Milliseconds;
    OS_Keylist Action_List;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_Lists[MAX_NUM_DEVICES];
static write_property_function Write_Property_Internal_Callback;
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
 * @brief Initialize a BACNET_ACTION_LIST entry to "empty" defaults.
 * @param pAction [in,out] Action list entry to initialize.
 */
static void Action_List_Entry_Init(BACNET_ACTION_LIST *pAction)
{
    if (pAction) {
        pAction->Device_Id.type = OBJECT_DEVICE;
        pAction->Device_Id.instance = BACNET_MAX_INSTANCE;
        pAction->Object_Id.type = OBJECT_NONE;
        pAction->Object_Id.instance = BACNET_MAX_INSTANCE;
        pAction->Property_Identifier = PROP_ALL;
        pAction->Property_Array_Index = BACNET_ARRAY_ALL;
        pAction->Property_Value.data_len = 0;
        pAction->Priority = BACNET_NO_PRIORITY;
        pAction->Post_Delay = UINT32_MAX;
        pAction->Quit_On_Failure = false;
        pAction->Write_Successful = false;
        pAction->next = NULL;
    }
}

/**
 * @brief Determine if a BACNET_ACTION_LIST entry is considered empty.
 * @param pAction [in] Action list entry.
 * @return true if empty.
 */
static bool Action_List_Entry_Empty(const BACNET_ACTION_LIST *pAction)
{
    if (!pAction) {
        return true;
    }

    return (pAction->Object_Id.instance == BACNET_MAX_INSTANCE);
}

/**
 * @brief Free all action entries in a Command object action list.
 * @param pObject [in,out] Pointer to object data.
 */
static void Action_List_Purge(struct object_data *pObject)
{
    BACNET_ACTION_LIST *pAction = NULL;

    if (!pObject) {
        return;
    }
    while (Keylist_Count(pObject->Action_List) > 0) {
        pAction = Keylist_Data_Pop(pObject->Action_List);
        free(pAction);
    }
}

/**
 * @brief Get action list element by array index.
 * @param pObject [in] Pointer to object data.
 * @param array_index [in] Action list array index.
 * @return pointer to action element, or NULL.
 */
static BACNET_ACTION_LIST *
Action_List_Member(struct object_data *pObject, BACNET_ARRAY_INDEX array_index)
{
    BACNET_ACTION_LIST *action = NULL;

    if (pObject) {
        action = Keylist_Data(pObject->Action_List, array_index);
    }

    return action;
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

    if (!pObject) {
        return false;
    }
    pObject->Action_List = Keylist_Create();
    if (!pObject->Action_List) {
        return false;
    }
    pAction = calloc(1, sizeof(BACNET_ACTION_LIST));
    if (!pAction) {
        Action_List_Free(pObject->Action_List);
        pObject->Action_List = NULL;
        return false;
    }
    Action_List_Entry_Init(pAction);
    index = Keylist_Data_Add(pObject->Action_List, 0, pAction);
    if (index < 0) {
        free(pAction);
        Action_List_Free(pObject->Action_List);
        pObject->Action_List = NULL;
        return false;
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
        pObject->Action = NULL;
        pObject->Action_Delay_Milliseconds = 0;
        pObject->Action_Failed = false;
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
        if (pObject->In_Process) {
            return false;
        }
        pObject->Present_Value = value;
        pObject->In_Process = true;
        pObject->All_Writes_Successful = false;
        pObject->Action_Failed = false;
        pObject->Action_Delay_Milliseconds = 0;
        if (value == 0) {
            pObject->Action = NULL;
        } else {
            pObject->Action =
                Command_Action_List_Entry(object_instance, value - 1);
        }
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
    return Command_Action_List_Member(instance, index);
}

/**
 * @brief Return Action list as linked list with next pointers set.
 * @param instance [in] BACnet object instance number.
 * @return first Action list element, or NULL.
 */
BACNET_ACTION_LIST *Command_Action_List(uint32_t instance)
{
    struct object_data *pObject;
    BACNET_ACTION_LIST *first_element = NULL;
    BACNET_ACTION_LIST *element = NULL;
    BACNET_ACTION_LIST *prev_element = NULL;
    unsigned count = 0;
    unsigned i = 0;

    pObject = Object_Data(instance);
    if (pObject) {
        count = Command_Action_List_Count(instance);
        for (i = 0; i < count; i++) {
            element = Action_List_Member(pObject, i);
            if (element) {
                if (i == 0) {
                    first_element = element;
                } else if (prev_element) {
                    prev_element->next = element;
                }
                element->next = NULL;
                prev_element = element;
            }
        }
    }

    return first_element;
}

/**
 * @brief Set Action list from a linked list.
 * @param instance [in] BACnet object instance number.
 * @param action_list [in] Linked list to copy from.
 */
void Command_Action_List_Set(uint32_t instance, BACNET_ACTION_LIST *action_list)
{
    struct object_data *pObject;
    BACNET_ACTION_LIST *element = NULL;
    BACNET_ACTION_LIST *data = NULL;
    KEY key;

    pObject = Object_Data(instance);
    if (pObject) {
        Action_List_Purge(pObject);
        element = action_list;
        while (element) {
            key = Keylist_Next_Empty_Key(pObject->Action_List, 0);
            data = calloc(1, sizeof(BACNET_ACTION_LIST));
            if (!data) {
                break;
            }
            memmove(data, element, sizeof(BACNET_ACTION_LIST));
            data->next = NULL;
            if (Keylist_Data_Add(pObject->Action_List, key, data) < 0) {
                free(data);
                break;
            }
            element = element->next;
        }
    }
}

/**
 * @brief Link an array of action entries with next pointers.
 * @param array [in,out] Pointer to entry zero.
 * @param size [in] Number of elements.
 */
void Command_Action_List_Link_Array(BACNET_ACTION_LIST *array, size_t size)
{
    size_t i;

    if (!array) {
        return;
    }
    for (i = 0; i < size; i++) {
        if (i > 0) {
            array[i - 1].next = &array[i];
        }
        array[i].next = NULL;
    }
}

/**
 * @brief Compare two action list elements.
 * @param element1 [in] First element.
 * @param element2 [in] Second element.
 * @return true if same.
 */
bool Command_Action_List_Element_Same(
    BACNET_ACTION_LIST *element1, BACNET_ACTION_LIST *element2)
{
    if (element1 == element2) {
        return true;
    }
    if (!element1 || !element2) {
        return false;
    }

    return bacnet_action_command_same(element1, element2);
}

/**
 * @brief Return one action list element by array index.
 * @param instance [in] BACnet object instance number.
 * @param array_index [in] Action list array index.
 * @return pointer to action element, or NULL.
 */
BACNET_ACTION_LIST *
Command_Action_List_Member(uint32_t instance, BACNET_ARRAY_INDEX array_index)
{
    struct object_data *pObject;

    pObject = Object_Data(instance);
    if (pObject) {
        return Action_List_Member(pObject, array_index);
    }

    return NULL;
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
 * @brief Check if an action element exists in the action list.
 * @param instance [in] BACnet object instance number.
 * @param element [in] Element to locate.
 * @return array index if found, else BACNET_ARRAY_ALL.
 */
BACNET_ARRAY_INDEX Command_Action_List_Element_Exist(
    uint32_t instance, BACNET_ACTION_LIST *element)
{
    BACNET_ARRAY_INDEX array_index = BACNET_ARRAY_ALL;
    BACNET_ACTION_LIST *list_element = NULL;
    unsigned count = 0;

    if (element) {
        count = Command_Action_List_Count(instance);
        for (array_index = 0; array_index < count; array_index++) {
            list_element = Command_Action_List_Member(instance, array_index);
            if (Command_Action_List_Element_Same(list_element, element)) {
                break;
            }
        }
        if (array_index >= count) {
            array_index = BACNET_ARRAY_ALL;
        }
    }

    return array_index;
}

/**
 * @brief Add unique action list element.
 * @param instance [in] BACnet object instance number.
 * @param element [in] Element to add.
 * @return array index if added/existing, else BACNET_ARRAY_ALL.
 */
BACNET_ARRAY_INDEX
Command_Action_List_Element_Add(uint32_t instance, BACNET_ACTION_LIST *element)
{
    struct object_data *pObject;
    BACNET_ARRAY_INDEX array_index = BACNET_ARRAY_ALL;
    KEY key = 0;
    BACNET_ACTION_LIST *data = NULL;

    pObject = Object_Data(instance);
    if (pObject && element) {
        array_index = Command_Action_List_Element_Exist(instance, element);
        if (array_index == BACNET_ARRAY_ALL) {
            key = Keylist_Next_Empty_Key(pObject->Action_List, 0);
            data = calloc(1, sizeof(BACNET_ACTION_LIST));
            if (data) {
                memmove(data, element, sizeof(BACNET_ACTION_LIST));
                data->next = NULL;
                if (Keylist_Data_Add(pObject->Action_List, key, data) >= 0) {
                    array_index = key;
                } else {
                    free(data);
                }
            }
        }
    }

    return array_index;
}

/**
 * @brief Remove action list element.
 * @param instance [in] BACnet object instance number.
 * @param element [in] Element to remove.
 * @return removed element array index, else BACNET_ARRAY_ALL.
 */
BACNET_ARRAY_INDEX Command_Action_List_Element_Remove(
    uint32_t instance, BACNET_ACTION_LIST *element)
{
    struct object_data *pObject;
    BACNET_ACTION_LIST *data = NULL;
    BACNET_ARRAY_INDEX array_index = BACNET_ARRAY_ALL;

    pObject = Object_Data(instance);
    if (pObject && element) {
        array_index = Command_Action_List_Element_Exist(instance, element);
        if (array_index != BACNET_ARRAY_ALL) {
            data = Keylist_Data_Delete(pObject->Action_List, array_index);
            free(data);
        }
    }

    return array_index;
}

/**
 * @brief Purge all action entries for object instance.
 * @param instance [in] BACnet object instance number.
 * @return true if object exists and purge ran.
 */
bool Command_Action_List_Purge(uint32_t instance)
{
    struct object_data *pObject;

    pObject = Object_Data(instance);
    if (pObject) {
        Action_List_Purge(pObject);
        return true;
    }

    return false;
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
            Action_List_Entry_Init(pAction);
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
            len = bacnet_action_command_decode(apdu, apdu_size, &action);
            if (len > 0) {
                pAction = Keylist_Data(pObject->Action_List, array_index);
                if (pAction) {
                    *pAction = action;
                    pAction->next = NULL;
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
 * @brief Determine if action should be executed on the local device.
 * @param pAction [in] Action entry.
 * @return true if local execution is supported.
 */
static bool Command_Action_Target_Is_Local(const BACNET_ACTION_LIST *pAction)
{
    if (!pAction) {
        return false;
    }
    if (pAction->Device_Id.instance == BACNET_MAX_INSTANCE) {
        return true;
    }
    if (pAction->Device_Id.type != OBJECT_DEVICE) {
        return false;
    }

    return (pAction->Device_Id.instance == Device_Object_Instance_Number());
}

/**
 * @brief Execute a single action entry as a local WriteProperty operation.
 * @param pAction [in,out] Action entry.
 * @return true when write succeeds.
 */
static bool Command_Action_Write(BACNET_ACTION_LIST *pAction)
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    int len;
    bool status = false;

    if (!pAction) {
        return false;
    }
    if (Action_List_Entry_Empty(pAction)) {
        pAction->Write_Successful = false;
        return false;
    }
    if (!Command_Action_Target_Is_Local(pAction)) {
        pAction->Write_Successful = false;
        return false;
    }
    wp_data.object_type = pAction->Object_Id.type;
    wp_data.object_instance = pAction->Object_Id.instance;
    wp_data.object_property = pAction->Property_Identifier;
    wp_data.array_index = pAction->Property_Array_Index;
    if ((pAction->Priority >= BACNET_MIN_PRIORITY) &&
        (pAction->Priority <= BACNET_MAX_PRIORITY)) {
        wp_data.priority = pAction->Priority;
    } else {
        wp_data.priority = BACNET_NO_PRIORITY;
    }
    len = pAction->Property_Value.data_len;
    if ((len > 0) && ((size_t)len <= sizeof(wp_data.application_data))) {
        memcpy(
            wp_data.application_data, pAction->Property_Value.data,
            pAction->Property_Value.data_len);
    } else {
        len = 0;
    }
    if ((len > 0) && Write_Property_Internal_Callback) {
        wp_data.application_data_len = len;
        status = write_property_bacnet_array_valid(&wp_data);
        if (status) {
            status = Write_Property_Internal_Callback(&wp_data);
        }
    }
    pAction->Write_Successful = status;

    return status;
}

/**
 * @brief Mark all actions in a linked list as unsuccessful.
 * @param pAction [in,out] First action entry.
 */
static void Command_Action_List_Fail(BACNET_ACTION_LIST *pAction)
{
    while (pAction) {
        pAction->Write_Successful = false;
        pAction = pAction->next;
    }
}

/**
 * @brief Convert post-delay seconds into object timer milliseconds.
 * @param pObject [in,out] Command object data.
 * @param post_delay [in] Post-delay in seconds, or UINT32_MAX if absent.
 */
static void
Command_Action_Delay_Set(struct object_data *pObject, uint32_t post_delay)
{
    if (!pObject) {
        return;
    }
    if ((post_delay == UINT32_MAX) || (post_delay == 0)) {
        pObject->Action_Delay_Milliseconds = 0;
    } else if (post_delay > (UINT32_MAX / 1000U)) {
        pObject->Action_Delay_Milliseconds = UINT32_MAX;
    } else {
        pObject->Action_Delay_Milliseconds = post_delay * 1000U;
    }
}

/**
 * @brief Execute Command action sequence over time.
 * @param object_instance [in] BACnet object instance number.
 * @param milliseconds [in] Elapsed milliseconds since last tick.
 */
void Command_Timer(uint32_t object_instance, uint16_t milliseconds)
{
    struct object_data *pObject;
    BACNET_ACTION_LIST *pAction;
    bool status;

    pObject = Object_Data(object_instance);
    if (!pObject || !pObject->In_Process) {
        return;
    }
    if (pObject->Action_Delay_Milliseconds > 0) {
        if (pObject->Action_Delay_Milliseconds > milliseconds) {
            pObject->Action_Delay_Milliseconds -= milliseconds;
            return;
        }
        pObject->Action_Delay_Milliseconds = 0;
    }
    while (pObject->In_Process && (pObject->Action_Delay_Milliseconds == 0)) {
        pAction = pObject->Action;
        if ((pObject->Present_Value == 0) || Action_List_Entry_Empty(pAction)) {
            pObject->In_Process = false;
            pObject->All_Writes_Successful = !pObject->Action_Failed;
            break;
        }
        status = Command_Action_Write(pAction);
        if (!status) {
            pObject->Action_Failed = true;
            if (pAction->Quit_On_Failure) {
                Command_Action_List_Fail(pAction->next);
                pObject->Action = NULL;
                pObject->In_Process = false;
                pObject->All_Writes_Successful = false;
                break;
            }
        }
        pObject->Action = pAction->next;
        Command_Action_Delay_Set(pObject, pAction->Post_Delay);
    }
}

/**
 * @brief Sets a callback used when Command action writes are executed.
 * @param cb [in] callback used to write referenced properties.
 */
void Command_Write_Property_Internal_Callback_Set(write_property_function cb)
{
    Write_Property_Internal_Callback = cb;
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
    BACNET_UNSIGNED_INTEGER array_size = 0;
    BACNET_CHARACTER_STRING char_string = { 0 };
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    int len = 0;

    /* Valid data? */
    if (wp_data == NULL) {
        return false;
    }
    switch ((int)wp_data->object_property) {
        case PROP_OBJECT_NAME:
            len = bacnet_character_string_application_decode(
                wp_data->application_data, wp_data->application_data_len,
                &char_string);
            if (len <= 0) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                if (len < 0) {
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
                return false;
            }
            status = Command_Object_Name_Write(wp_data, &char_string);
            break;
        case PROP_DESCRIPTION:
            len = bacnet_character_string_application_decode(
                wp_data->application_data, wp_data->application_data_len,
                &char_string);
            if (len <= 0) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                if (len < 0) {
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
                return false;
            }
            status = Command_Description_Write(wp_data, &char_string);
            break;
        case PROP_PRESENT_VALUE:
            if (Command_In_Process(wp_data->object_instance)) {
                wp_data->error_class = ERROR_CLASS_OBJECT;
                wp_data->error_code = ERROR_CODE_BUSY;
                return false;
            }
            len = bacnet_unsigned_application_decode(
                wp_data->application_data, wp_data->application_data_len,
                &unsigned_value);
            if (len <= 0) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                if (len < 0) {
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
                return false;
            }
            if (unsigned_value >
                Command_Action_List_Count(wp_data->object_instance)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                return false;
            }
            status = Command_Present_Value_Set(
                wp_data->object_instance, unsigned_value);
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
