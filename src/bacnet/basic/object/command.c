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
    BACNET_CHARACTER_STRING_ANSI Description;
    BACNET_CHARACTER_STRING_ANSI Object_Name;
    /* present-value action, or NULL */
    BACNET_ACTION_LIST *Action;
    uint32_t Action_Delay_Milliseconds;
    /* key=array_index->inner OS_Keylist (key=list_index->BACNET_ACTION_LIST*)*/
    OS_Keylist Action_Array;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_Lists[MAX_NUM_DEVICES];
static write_property_function Write_Property_Internal_Callback;
#ifdef BAC_ROUTING
#define Object_List (Object_Lists[Routed_Device_Object_Index()])
#else
#define Object_List (Object_Lists[0])
#endif

/*
 * Limit on the outer Action_Array size to guard against remote resize abuse.
 * Tune this value as needed for deployment requirements.
 */
#ifndef BACNET_COMMAND_ACTION_LIST_MAX
#define BACNET_COMMAND_ACTION_LIST_MAX 1024U
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
 * @brief Initialise one BACNET_ACTION_LIST entry to the
 *  "nothing configured" state.
 * @param p [in] Pointer to BACNET_ACTION_LIST entry to initialize.
 */
static void Action_Entry_Init(BACNET_ACTION_LIST *p)
{
    if (p) {
        p->Device_Id.type = OBJECT_DEVICE;
        p->Device_Id.instance = BACNET_MAX_INSTANCE;
        p->Object_Id.type = OBJECT_NONE;
        p->Object_Id.instance = BACNET_MAX_INSTANCE;
        p->Property_Identifier = PROP_ALL;
        p->Property_Array_Index = BACNET_ARRAY_ALL;
        p->Property_Value.data_len = 0;
        p->Priority = BACNET_NO_PRIORITY;
        p->Post_Delay = UINT32_MAX;
        p->Quit_On_Failure = false;
        p->Write_Successful = false;
        p->next = NULL;
    }
}

/**
 * @brief Check if a BACNET_ACTION_LIST entry is empty (not configured).
 * @param p [in] Pointer to BACNET_ACTION_LIST entry to check.
 * @return true if the entry is empty, false otherwise.
 */
static bool Action_Entry_Empty(const BACNET_ACTION_LIST *p)
{
    if (!p) {
        return true;
    }
    return (p->Object_Id.instance == BACNET_MAX_INSTANCE);
}

/**
 * @brief Free all entries and delete an inner list keylist.
 * @param inner [in] Inner list keylist to free.
 */
static void Action_Inner_List_Free(OS_Keylist inner)
{
    BACNET_ACTION_LIST *p;

    if (inner) {
        do {
            p = Keylist_Data_Pop(inner);
            free(p);
        } while (p);
        Keylist_Delete(inner);
    }
}

/**
 * @brief Remove all entries from an inner list keylist without deleting the
 * list.
 * @param inner [in] Inner list keylist to purge.
 */
static void Action_Inner_List_Purge(OS_Keylist inner)
{
    BACNET_ACTION_LIST *p;

    if (inner) {
        do {
            p = Keylist_Data_Pop(inner);
            free(p);
        } while (p);
    }
}

/**
 * @brief Link inner keylist entries via next pointers and return the head.
 * @param inner [in] Inner list keylist to link.
 * @return Head of the linked list.
 */
static BACNET_ACTION_LIST *Action_Inner_List_Link(OS_Keylist inner)
{
    BACNET_ACTION_LIST *head = NULL;
    BACNET_ACTION_LIST *prev = NULL;
    BACNET_ACTION_LIST *cur;
    unsigned count;
    unsigned i;

    if (!inner) {
        return NULL;
    }
    count = Keylist_Count(inner);
    for (i = 0; i < count; i++) {
        cur = Keylist_Data(inner, (KEY)i);
        if (!cur) {
            continue;
        }
        cur->next = NULL;
        if (!head) {
            head = cur;
        }
        if (prev) {
            prev->next = cur;
        }
        prev = cur;
    }
    return head;
}

/**
 * @brief Create an inner list keylist pre-populated with one empty entry.
 * @return The created inner list keylist, or NULL on failure.
 */
static OS_Keylist Action_Inner_List_Create(void)
{
    OS_Keylist inner;
    BACNET_ACTION_LIST *p;

    inner = Keylist_Create();
    if (!inner) {
        return NULL;
    }
    p = calloc(1, sizeof(BACNET_ACTION_LIST));
    if (!p) {
        Keylist_Delete(inner);
        return NULL;
    }
    Action_Entry_Init(p);
    if (Keylist_Data_Add(inner, 0, p) < 0) {
        free(p);
        Keylist_Delete(inner);
        return NULL;
    }
    return inner;
}

/**
 * @brief Resize the outer Action_Array keylist; each new slot gets one empty
 * entry.
 * @param pObject [in] Pointer to the object containing the Action_Array.
 * @param new_size [in] The new size of the Action_Array.
 * @return Error code indicating success or failure.
 */
static BACNET_ERROR_CODE Action_Array_Resize(
    struct object_data *pObject, BACNET_UNSIGNED_INTEGER new_size)
{
    OS_Keylist inner;
    KEY key;
    int outer_size;

    if (!pObject) {
        return ERROR_CODE_UNKNOWN_OBJECT;
    }
    if (new_size > BACNET_COMMAND_ACTION_LIST_MAX) {
        return ERROR_CODE_VALUE_OUT_OF_RANGE;
    }
    outer_size = new_size;
    while (Keylist_Count(pObject->Action_Array) > outer_size) {
        key = (KEY)(Keylist_Count(pObject->Action_Array) - 1);
        inner = Keylist_Data_Delete(pObject->Action_Array, key);
        Action_Inner_List_Free(inner);
    }
    while (Keylist_Count(pObject->Action_Array) < outer_size) {
        key = (KEY)Keylist_Count(pObject->Action_Array);
        inner = Action_Inner_List_Create();
        if (!inner) {
            return ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
        }
        if (Keylist_Data_Add(pObject->Action_Array, key, inner) < 0) {
            Action_Inner_List_Free(inner);
            return ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
        }
    }
    return ERROR_CODE_SUCCESS;
}

/**
 * @brief Init Action_Array with one empty inner list (one array slot, one empty
 * cmd).
 * @param pObject [in] Pointer to the object containing the Action_Array.
 * @return true on success, false on failure.
 */
static bool Action_Array_Init(struct object_data *pObject)
{
    OS_Keylist inner;

    if (!pObject) {
        return false;
    }
    pObject->Action_Array = Keylist_Create();
    if (!pObject->Action_Array) {
        return false;
    }
    inner = Action_Inner_List_Create();
    if (!inner) {
        Keylist_Delete(pObject->Action_Array);
        pObject->Action_Array = NULL;
        return false;
    }
    if (Keylist_Data_Add(pObject->Action_Array, 0, inner) < 0) {
        Action_Inner_List_Free(inner);
        Keylist_Delete(pObject->Action_Array);
        pObject->Action_Array = NULL;
        return false;
    }
    return true;
}

/**
 * @brief Free all inner lists and the outer Action_Array keylist.
 * @param array [in] Outer Action_Array keylist to free.
 */
static void Action_Array_Free(OS_Keylist array)
{
    OS_Keylist inner;

    if (array) {
        do {
            inner = Keylist_Data_Pop(array);
            Action_Inner_List_Free(inner);
        } while (inner);
        Keylist_Delete(array);
    }
}

/**
 * @brief Free all memory owned by a Command object data record.
 * @param pObject [in] Pointer to object data.
 */
static void Object_Data_Free(struct object_data *pObject)
{
    if (pObject) {
        characterstring_ansi_free(&pObject->Description);
        characterstring_ansi_free(&pObject->Object_Name);
        Action_Array_Free(pObject->Action_Array);
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
        pObject->Action = NULL;
        pObject->Action_Delay_Milliseconds = 0;
        pObject->Action_Failed = false;
        pObject->All_Writes_Successful = true;
        if (!Action_Array_Init(pObject)) {
            Object_Data_Free(pObject);
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
    OS_Keylist inner;

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
            inner = Keylist_Data(pObject->Action_Array, (KEY)(value - 1));
            pObject->Action = Action_Inner_List_Link(inner);
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
    struct object_data *pObject;
    bool status = false;
    int len = 0;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = characterstring_ansi_to_characterstring(
            object_name, &pObject->Object_Name);
        if (!status) {
            len = characterstring_utf8_snprintf(
                object_name, "COMMAND-%lu", (unsigned long)object_instance);
            if (len > 0) {
                status = true;
            }
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets a BACnet character string
 *  by referencing an ANSI C string.
 * @note The object name must be unique within this device.
 * @param object_instance object-instance number of the object
 * @param new_name Holds a pointer to a static constant ANSI C string for
 *  zero copy, or NULL to clear it.
 * @return true if object-name was set
 */
bool Command_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status =
            characterstring_ansi_const_init(&pObject->Object_Name, new_name);
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
        return characterstring_ansi_value_default(&pObject->Description, "");
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

    pObject = Object_Data(wp_data->object_instance);
    if (pObject) {
        if (characterstring_utf8_valid(cstring)) {
            status = characterstring_ansi_from_characterstring_strdup(
                &pObject->Object_Name, cstring);
            if (!status) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
            }
        } else {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
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

    pObject = Object_Data(wp_data->object_instance);
    if (pObject) {
        if (characterstring_utf8_valid(cstring)) {
            status = characterstring_ansi_from_characterstring_strdup(
                &pObject->Description, cstring);
            if (!status) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
            }
        } else {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
    } else {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets a BACnet character string
 *  by referencing an ANSI C string.
 * @param object_instance object-instance number of the object
 * @param new_name Holds a pointer to a static constant ANSI C string for
 *  zero copy, or NULL to clear it.
 * @return true if description was set
 */
bool Command_Description_Set(uint32_t instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(instance);
    if (pObject) {
        status =
            characterstring_ansi_const_init(&pObject->Description, new_name);
    }

    return status;
}

/**
 * @brief Return the number of action list array slots for an instance.
 * @param instance [in] BACnet object instance number.
 * @return number of array slots (outer array size).
 */
unsigned Command_Action_Array_Count(uint32_t instance)
{
    struct object_data *pObject;

    pObject = Object_Data(instance);
    if (pObject) {
        return Keylist_Count(pObject->Action_Array);
    }

    return 0;
}

/**
 * @brief Return the number of action commands in one array slot.
 * @param instance [in] BACnet object instance number.
 * @param array_index [in] 0-based array slot index.
 * @return Number of action entries in the selected slot, or 0 if absent.
 */
unsigned
Command_Action_List_Count(uint32_t instance, BACNET_ARRAY_INDEX array_index)
{
    struct object_data *pObject;
    OS_Keylist inner;

    pObject = Object_Data(instance);
    if (pObject) {
        inner = Keylist_Data(pObject->Action_Array, (KEY)array_index);
        if (inner) {
            return Keylist_Count(inner);
        }
    }

    return 0;
}

/**
 * @brief Return one action command by slot and list position.
 * @param instance [in] BACnet object instance number.
 * @param array_index [in] 0-based array slot index.
 * @param list_index [in] 0-based position within the inner list.
 * @return Action entry at the requested position, or NULL if not found.
 */
BACNET_ACTION_LIST *Command_Action_List_Member(
    uint32_t instance, BACNET_ARRAY_INDEX array_index, unsigned list_index)
{
    struct object_data *pObject;
    OS_Keylist inner;

    pObject = Object_Data(instance);
    if (pObject) {
        inner = Keylist_Data(pObject->Action_Array, (KEY)array_index);
        if (inner) {
            return Keylist_Data(inner, (KEY)list_index);
        }
    }

    return NULL;
}

/**
 * @brief Return the linked list of action commands for one array slot.
 * @param instance [in] BACnet object instance number.
 * @param array_index [in] 0-based array slot index.
 * @note next pointers are set on the stored nodes; do not free.
 */
BACNET_ACTION_LIST *
Command_Action_List(uint32_t instance, BACNET_ARRAY_INDEX array_index)
{
    struct object_data *pObject;
    OS_Keylist inner;

    pObject = Object_Data(instance);
    if (pObject) {
        inner = Keylist_Data(pObject->Action_Array, (KEY)array_index);
        return Action_Inner_List_Link(inner);
    }

    return NULL;
}

/**
 * @brief Replace the action commands for one array slot from a linked list.
 * @param instance [in] BACnet object instance number.
 * @param array_index [in] 0-based array slot index.
 * @param action_list [in] Linked list of commands to copy; NULL to clear slot.
 */
void Command_Action_List_Set(
    uint32_t instance,
    BACNET_ARRAY_INDEX array_index,
    BACNET_ACTION_LIST *action_list)
{
    struct object_data *pObject;
    OS_Keylist inner;
    BACNET_ACTION_LIST *src;
    BACNET_ACTION_LIST *data;
    KEY key;

    pObject = Object_Data(instance);
    if (!pObject) {
        return;
    }
    inner = Keylist_Data(pObject->Action_Array, (KEY)array_index);
    if (!inner) {
        return;
    }
    Action_Inner_List_Purge(inner);
    src = action_list;
    key = 0;
    while (src) {
        data = calloc(1, sizeof(BACNET_ACTION_LIST));
        if (!data) {
            break;
        }
        memmove(data, src, sizeof(BACNET_ACTION_LIST));
        data->next = NULL;
        if (Keylist_Data_Add(inner, key, data) < 0) {
            free(data);
            break;
        }
        key++;
        src = src->next;
    }
}

/**
 * @brief Purge all action commands from one array slot.
 * @param instance [in] BACnet object instance number.
 * @param array_index [in] 0-based array slot index.
 * @return true if the object and slot exist.
 */
bool Command_Action_List_Purge(
    uint32_t instance, BACNET_ARRAY_INDEX array_index)
{
    struct object_data *pObject;
    OS_Keylist inner;

    pObject = Object_Data(instance);
    if (pObject) {
        inner = Keylist_Data(pObject->Action_Array, (KEY)array_index);
        if (inner) {
            Action_Inner_List_Purge(inner);
            return true;
        }
    }

    return false;
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
 * @brief Encode one BACnetARRAY element: the BACnetLIST at array slot @p index.
 * @param object_instance [in] BACnet object instance number.
 * @param index [in] 0-based array slot index.
 * @param apdu [out] Buffer or NULL (length-only query).
 * @return encoded byte count, or BACNET_STATUS_ERROR.
 */
static int Command_Action_List_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    struct object_data *pObject;
    OS_Keylist inner;
    BACNET_ACTION_LIST *head = NULL;
    BACNET_ACTION_LIST *prev = NULL;
    BACNET_ACTION_LIST *cur;
    unsigned count;
    unsigned i;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return BACNET_STATUS_ERROR;
    }
    inner = Keylist_Data(pObject->Action_Array, (KEY)index);
    if (!inner) {
        return BACNET_STATUS_ERROR;
    }
    /* build a linked list of non-empty entries for encoding */
    count = Keylist_Count(inner);
    for (i = 0; i < count; i++) {
        cur = Keylist_Data(inner, (KEY)i);
        if (!cur || Action_Entry_Empty(cur)) {
            continue;
        }
        cur->next = NULL;
        if (!head) {
            head = cur;
        }
        if (prev) {
            prev->next = cur;
        }
        prev = cur;
    }
    return bacnet_action_list_encode(apdu, head);
}

/**
 * @brief Decode a single BACnet action command to determine its byte length.
 * @param object_instance [in] BACnet object instance number.
 * @param apdu [in] Encoded action command bytes.
 * @param apdu_size [in] Remaining buffer size.
 * @return Encoded byte length for the command, or BACNET_STATUS_ERROR if
 * invalid.
 */
static int Command_Action_List_Member_Decode(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    /* returns one command's byte length for BACNET_ARRAY_ALL slot partitioning
     */
    if (!Object_Data(object_instance)) {
        return BACNET_STATUS_ERROR;
    }
    return bacnet_action_command_decode(apdu, apdu_size, NULL);
}

/**
 * @brief Write one BACnetARRAY element: resize (index 0) or replace a list
 * slot.
 */
static BACNET_ERROR_CODE Command_Action_List_Member_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *apdu,
    size_t apdu_size)
{
    BACNET_ACTION_LIST action = { 0 };
    BACNET_ACTION_LIST *data;
    OS_Keylist inner;
    struct object_data *pObject;
    int len;
    size_t offset;
    KEY key;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return ERROR_CODE_UNKNOWN_OBJECT;
    }
    if (array_index == 0) {
        if (array_size > BACNET_COMMAND_ACTION_LIST_MAX) {
            return ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
        return Action_Array_Resize(pObject, array_size);
    }
    array_index--; /* 1-based protocol index → 0-based internal index */
    inner = Keylist_Data(pObject->Action_Array, (KEY)array_index);
    if (!inner) {
        return ERROR_CODE_INVALID_ARRAY_INDEX;
    }

    /* first pass: validate all commands before modifying stored state */
    offset = 0;
    while (offset < apdu_size) {
        len = bacnet_action_command_decode(
            &apdu[offset], apdu_size - offset, NULL);
        if (len <= 0) {
            return ERROR_CODE_INVALID_DATA_TYPE;
        }
        offset += len;
    }

    /* second pass: purge then store validated commands in-place */
    Action_Inner_List_Purge(inner);
    offset = 0;
    key = 0;
    while (offset < apdu_size) {
        len = bacnet_action_command_decode(
            &apdu[offset], apdu_size - offset, &action);
        data = calloc(1, sizeof(BACNET_ACTION_LIST));
        if (!data) {
            return ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
        }
        memmove(data, &action, sizeof(BACNET_ACTION_LIST));
        data->next = NULL;
        if (Keylist_Data_Add(inner, key, data) < 0) {
            free(data);
            return ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
        }
        key++;
        offset += len;
    }
    return ERROR_CODE_SUCCESS;
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
    if (Action_Entry_Empty(pAction)) {
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
        if ((pObject->Present_Value == 0) || Action_Entry_Empty(pAction)) {
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
                Command_Action_Array_Count(rpdata->object_instance), apdu,
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
                Command_Action_Array_Count(wp_data->object_instance)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                return false;
            }
            status = Command_Present_Value_Set(
                wp_data->object_instance, unsigned_value);
            break;
        case PROP_ACTION:
            /* guard: action-write callback could re-enter and free pAction */
            if (Command_In_Process(wp_data->object_instance)) {
                wp_data->error_class = ERROR_CLASS_OBJECT;
                wp_data->error_code = ERROR_CODE_BUSY;
                return false;
            }
            array_size = Command_Action_Array_Count(wp_data->object_instance);
            wp_data->error_code = bacnet_array_write_resizable(
                wp_data->object_instance, wp_data->array_index,
                Command_Action_List_Member_Decode,
                Command_Action_List_Member_Write, array_size,
                wp_data->application_data, wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
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
