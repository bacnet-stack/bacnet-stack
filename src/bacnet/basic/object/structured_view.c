/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @brief Structured View object is an object that provides a container
 * to hold references to subordinate objects, which may include other
 * Structured View objects, thereby allowing multilevel hierarchies
 * to be created.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/proplist.h"
#include "bacnet/rp.h"
#include "bacnet/basic/sys/keylist.h"
/* BACnet Stack Objects */
#include "bacnet/basic/object/device.h"
/* me! */
#include "structured_view.h"

struct object_data {
    char *Object_Name;
    char *Description;
    BACNET_NODE_TYPE Node_Type;
    char *Node_Subtype;
    void *Context;
    OS_Keylist Subordinate_List;
    BACNET_RELATIONSHIP Default_Subordinate_Relationship;
    BACNET_DEVICE_OBJECT_REFERENCE Represents;
};

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_Lists[MAX_NUM_DEVICES];
#ifdef BAC_ROUTING
#define Object_List (Object_Lists[Routed_Device_Object_Index()])
#else
#define Object_List (Object_Lists[0])
#endif

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,      PROP_OBJECT_TYPE,
    PROP_NODE_TYPE,         PROP_SUBORDINATE_LIST, -1
};

static const int32_t Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_DESCRIPTION,
    PROP_NODE_SUBTYPE,
    PROP_SUBORDINATE_ANNOTATIONS,
    PROP_SUBORDINATE_NODE_TYPES,
    PROP_SUBORDINATE_RELATIONSHIPS,
    PROP_DEFAULT_SUBORDINATE_RELATIONSHIP,
    PROP_REPRESENTS,
    -1
};

static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of writable properties */
    PROP_NODE_TYPE,
    PROP_DEFAULT_SUBORDINATE_RELATIONSHIP,
    PROP_REPRESENTS,
    PROP_SUBORDINATE_LIST,
    PROP_SUBORDINATE_ANNOTATIONS,
    PROP_SUBORDINATE_NODE_TYPES,
    PROP_SUBORDINATE_RELATIONSHIPS,
    PROP_OBJECT_NAME,
    PROP_DESCRIPTION,
    -1
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
void Structured_View_Property_Lists(
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
 * @brief Get the list of writable properties for a Structured View object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Structured_View_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * Determines if a given Structured View instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Structured_View_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Structured View objects
 *
 * @return  Number of Structured View objects
 */
unsigned Structured_View_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..(N-1) index
 * of Structured View objects where N is Structured_View_Count().
 *
 * @param  index - 0..(N-1) where N is Structured_View_Count()
 *
 * @return  object instance-number for the given index
 */
uint32_t Structured_View_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..(N-1) index
 * of Structured View objects where N is Structured_View_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or Structured_View_Count()
 * if not valid.
 */
unsigned Structured_View_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
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
bool Structured_View_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[48] = "";

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "Structured-View-%lu",
                (unsigned long)object_instance);
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
bool Structured_View_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        free(pObject->Object_Name);
        pObject->Object_Name = bacnet_strdup(new_name);
    }

    return status;
}

/**
 * @brief Return the object name C string
 * @param object_instance [in] BACnet object instance number
 * @return object name or NULL if not found
 */
const char *Structured_View_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * For a given object instance-number, returns the description
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return description text or NULL if not found
 */
const char *Structured_View_Description(uint32_t object_instance)
{
    const char *name = NULL;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
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
 * For a given object instance-number, sets the description
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 *
 * @return  true if description was set
 */
bool Structured_View_Description_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        free(pObject->Description);
        pObject->Description = bacnet_strdup(new_name);
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the Node_Type
 * @param  object_instance - object-instance number of the object
 * @return Node_Type or BACNET_NODE_UNKNOWN if not found
 */
BACNET_NODE_TYPE Structured_View_Node_Type(uint32_t object_instance)
{
    BACNET_NODE_TYPE node_type = BACNET_NODE_UNKNOWN;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        node_type = pObject->Node_Type;
    }

    return node_type;
}

/**
 * @brief For a given object instance-number, sets the Node_Type
 * @param  object_instance - object-instance number of the object
 * @param  node_type - holds the Node_Type to be set
 * @return  true if Node_Type was set
 */
bool Structured_View_Node_Type_Set(
    uint32_t object_instance, BACNET_NODE_TYPE node_type)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        pObject->Node_Type = node_type;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the Node_Subtype
 * @param  object_instance - object-instance number of the object
 * @return Node_Subtype text or NULL if not found
 */
const char *Structured_View_Node_Subtype(uint32_t object_instance)
{
    const char *name = NULL;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Node_Subtype) {
            name = pObject->Node_Subtype;
        } else {
            name = "";
        }
    }

    return name;
}

/**
 * @brief For a given object instance-number, sets the Node_Subtype
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the Node_Subtype to be set
 * @return  true if Node_Subtype was set
 */
bool Structured_View_Node_Subtype_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        free(pObject->Node_Subtype);
        pObject->Node_Subtype = bacnet_strdup(new_name);
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the number of
 * Subordinate_List elements
 * @param  object_instance - object-instance number of the object
 * @return number of Subordinate_List elements
 */
static unsigned int
Structured_View_Subordinate_List_Size(struct object_data *pObject)
{
    unsigned int count = 0;

    if (pObject) {
        count = Keylist_Count(pObject->Subordinate_List);
    }

    return count;
}

/**
 * @brief For a given object Subordinate_List, add an element to the list
 * @param  list - pointer to the Subordinate_List key list
 * @param  key - key for the new element
 * @return pointer to the Subordinate_List element
 */
static BACNET_SUBORDINATE_DATA *
Subordinate_List_Element_Add(OS_Keylist list, KEY key)
{
    BACNET_SUBORDINATE_DATA *element = NULL;
    int index;

    element = calloc(1, sizeof(BACNET_SUBORDINATE_DATA));
    if (element) {
        element->next = NULL;
        index = Keylist_Data_Add(list, key, element);
        if (index < 0) {
            free(element);
            element = NULL;
        }
    }

    return element;
}

/**
 * @brief For a given object element, free the Subordinate_List element
 * @param  element - pointer to the Subordinate_List element
 */
static void Subordinate_List_Element_Remove(OS_Keylist list, KEY key)
{
    BACNET_SUBORDINATE_DATA *element;

    element = Keylist_Data_Delete(list, key);
    if (element) {
        if (element->Annotation) {
            free(element->Annotation);
        }
        free(element);
    }
}

/**
 * @brief For a given object instance-number, free the Subordinate_List
 * @param  pObject - pointer to the object data
 */
static void Subordinate_List_Purge(struct object_data *pObject)
{
    KEY key = 0;
    int count = 0;

    if (pObject) {
        count = Keylist_Count(pObject->Subordinate_List);
        while (count > 0) {
            Subordinate_List_Element_Remove(pObject->Subordinate_List, key);
            key++;
            count--;
        }
    }
}

/**
 * @brief For a given object instance-number, returns the number of
 * Subordinate_List elements
 * @param  object_instance - object-instance number of the object
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @return Subordinate_List element or NULL if not found
 */
static BACNET_SUBORDINATE_DATA *Subordinate_List_Element(
    struct object_data *pObject, BACNET_ARRAY_INDEX array_index)
{
    BACNET_SUBORDINATE_DATA *subordinate_list = NULL;
    KEY key = 0;

    if (pObject) {
        key = array_index;
        subordinate_list = Keylist_Data(pObject->Subordinate_List, key);
    }

    return subordinate_list;
}

/**
 * @brief For a given object instance-number, determine if an element exists
 * in the Subordinate_List
 * @param  object_instance - object-instance number of the object
 * @param  element - pointer to the Subordinate_List element to be checked
 * @return array index of the element if it exists,
 *  or BACNET_ARRAY_ALL if not found
 */
BACNET_ARRAY_INDEX Structured_View_Subordinate_List_Element_Exist(
    uint32_t object_instance, BACNET_SUBORDINATE_DATA *element)
{
    struct object_data *pObject;
    BACNET_SUBORDINATE_DATA *list_element;
    unsigned count = 0;
    BACNET_ARRAY_INDEX array_index = BACNET_ARRAY_ALL;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (element) {
            count = Structured_View_Subordinate_List_Size(pObject);
            for (array_index = 0; array_index < count; array_index++) {
                list_element = Subordinate_List_Element(pObject, array_index);
                if (Structured_View_Subordinate_List_Element_Same(
                        list_element, element)) {
                    break;
                }
            }
            if (array_index >= count) {
                array_index = BACNET_ARRAY_ALL;
            }
        }
    }

    return array_index;
}

/**
 * @brief For a given object instance-number, add a unique element to the
 *  Subordinate_List
 * @param  object_instance - object-instance number of the object
 * @param  element - pointer to the Subordinate_List element to be added
 * @return array index of the element if it was added successfully,
 *  or BACNET_ARRAY_ALL if not added
 */
BACNET_ARRAY_INDEX Structured_View_Subordinate_List_Element_Add(
    uint32_t object_instance, BACNET_SUBORDINATE_DATA *element)
{
    struct object_data *pObject;
    BACNET_ARRAY_INDEX array_index = BACNET_ARRAY_ALL;
    KEY key = 0;
    BACNET_SUBORDINATE_DATA *data = NULL;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (element) {
            /* does this element already exist in the list? */
            array_index = Structured_View_Subordinate_List_Element_Exist(
                object_instance, element);
            if (array_index == BACNET_ARRAY_ALL) {
                /* append a copy of the element to the list */
                key = Keylist_Next_Empty_Key(pObject->Subordinate_List, 0);
                data = Subordinate_List_Element_Add(
                    pObject->Subordinate_List, key);
                if (!data) {
                    array_index = BACNET_ARRAY_ALL;
                } else {
                    memmove(data, element, sizeof(BACNET_SUBORDINATE_DATA));
                    if (element->Annotation) {
                        data->Annotation = bacnet_strdup(element->Annotation);
                    }
                    data->next = NULL;
                    array_index = key;
                }
            }
        }
    }

    return array_index;
}

/**
 * @brief For a given object instance-number, remove an element from the
 * Subordinate_List
 * @param  object_instance - object-instance number of the object
 * @param  element - pointer to the Subordinate_List element to be removed
 * @return array index of the element if it was removed successfully,
 *  or BACNET_ARRAY_ALL if not removed
 */
BACNET_ARRAY_INDEX Structured_View_Subordinate_List_Element_Remove(
    uint32_t object_instance, BACNET_SUBORDINATE_DATA *element)
{
    BACNET_ARRAY_INDEX array_index = BACNET_ARRAY_ALL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        array_index = Structured_View_Subordinate_List_Element_Exist(
            object_instance, element);
        if (array_index != BACNET_ARRAY_ALL) {
            Subordinate_List_Element_Remove(
                pObject->Subordinate_List, array_index);
        }
    }

    return array_index;
}

/**
 * @brief For a given object instance-number, free the Subordinate_List
 * @param  object_instance - object-instance number of the object
 * @return true if the Subordinate_List was purged successfully
 */
bool Structured_View_Subordinate_List_Purge(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        Subordinate_List_Purge(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, resize the Subordinate_List
 * @param pObject - pointer to the object data
 * @param old_array_size - current size of the Subordinate_List
 * @param new_array_size - new size of the Subordinate_List
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Structured_View_Subordinate_List_Resize(
    struct object_data *pObject, BACNET_UNSIGNED_INTEGER new_array_size)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    BACNET_SUBORDINATE_DATA *element = NULL;
    BACNET_UNSIGNED_INTEGER old_array_size = 0;
    KEY key = 0;

    old_array_size = Structured_View_Subordinate_List_Size(pObject);
    /* Array element zero is the number of elements in the list. */
    if (new_array_size < old_array_size) {
        /* free the elements at the tail of the list */
        key = new_array_size;
        while (key < old_array_size) {
            Subordinate_List_Element_Remove(pObject->Subordinate_List, key);
            key++;
        }
    } else if (new_array_size > old_array_size) {
        /* extend the list */
        key = old_array_size;
        while (key < new_array_size) {
            element =
                Subordinate_List_Element_Add(pObject->Subordinate_List, key);
            if (!element) {
                error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                break;
            }
            key++;
        }
    }

    return error_code;
}

/**
 * @brief Decode a BACnetARRAY property element to determine the length
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Structured_View_Subordinate_List_Member_Decode(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    int len = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        len = bacnet_device_object_reference_decode(apdu, apdu_size, NULL);
    }

    return len;
}

/**
 * @brief Write a value to a BACnetARRAY property element value
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param array_size [in] number of elements in the array, used writing array
 * element 0
 * @param apdu [in] encoded element value
 * @param apdu_size [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Structured_View_Subordinate_List_Member_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *apdu,
    size_t apdu_size)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_DEVICE_OBJECT_REFERENCE reference = { 0 };
    BACNET_SUBORDINATE_DATA *element = NULL;
    int len = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (array_index == 0) {
            /* Array element zero is the number of elements in the list. */
            error_code =
                Structured_View_Subordinate_List_Resize(pObject, array_size);
        } else {
            array_index--; /* array index is 1..N, but we want 0..(N-1) */
            len = bacnet_device_object_reference_decode(
                apdu, apdu_size, &reference);
            if (len > 0) {
                element = Subordinate_List_Element(pObject, array_index);
                if (element) {
                    element->Device_Instance =
                        reference.deviceIdentifier.instance;
                    element->Object_Type = reference.objectIdentifier.type;
                    element->Object_Instance =
                        reference.objectIdentifier.instance;
                    error_code = ERROR_CODE_SUCCESS;
                } else {
                    error_code = ERROR_CODE_OTHER;
                }
            } else {
                error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
        }
    }

    return error_code;
}

/**
 * @brief Decode a BACnetARRAY property element to determine the length
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Structured_View_Subordinate_Annotation_Member_Decode(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    int len = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        len = bacnet_character_string_application_decode(apdu, apdu_size, NULL);
    }

    return len;
}

/**
 * @brief Write a value to a BACnetARRAY property element value
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param array_size [in] number of elements in the array, used writing array
 * element 0
 * @param apdu [in] encoded element value
 * @param apdu_size [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Structured_View_Subordinate_Annotation_Member_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *apdu,
    size_t apdu_size)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_CHARACTER_STRING annotation = { 0 };
    BACNET_SUBORDINATE_DATA *element = NULL;
    char *annotation_string;
    int len = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (array_index == 0) {
            /* Array element zero is the number of elements in the list. */
            error_code =
                Structured_View_Subordinate_List_Resize(pObject, array_size);
        } else {
            array_index--; /* array index is 1..N, but we want 0..(N-1) */
            len = bacnet_character_string_application_decode(
                apdu, apdu_size, &annotation);
            if (len > 0) {
                if (characterstring_utf8_valid(&annotation)) {
                    element = Subordinate_List_Element(pObject, array_index);
                    if (element) {
                        annotation_string =
                            characterstring_utf8_strdup(&annotation);
                        if (annotation_string) {
                            if (element->Annotation) {
                                free((void *)element->Annotation);
                            }
                            element->Annotation = annotation_string;
                            error_code = ERROR_CODE_SUCCESS;
                        } else {
                            error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                        }
                    } else {
                        error_code = ERROR_CODE_OTHER;
                    }
                } else {
                    error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
        }
    }

    return error_code;
}

/**
 * @brief Decode a BACnetARRAY property element to determine the length
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Structured_View_Subordinate_Node_Type_Member_Decode(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    int len = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        len = bacnet_enumerated_application_decode(apdu, apdu_size, NULL);
    }

    return len;
}

/**
 * @brief Write a value to a BACnetARRAY property element value
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param array_size [in] number of elements in the array, used writing array
 * element 0
 * @param apdu [in] encoded element value
 * @param apdu_size [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Structured_View_Subordinate_Node_Type_Member_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *apdu,
    size_t apdu_size)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    uint32_t node_type = 0;
    BACNET_SUBORDINATE_DATA *element = NULL;
    int len = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (array_index == 0) {
            error_code =
                Structured_View_Subordinate_List_Resize(pObject, array_size);
        } else {
            array_index--; /* array index is 1..N, but we want 0..(N-1) */
            len = bacnet_enumerated_application_decode(
                apdu, apdu_size, &node_type);
            if (len > 0) {
                if (node_type > BACNET_NODE_TYPE_MAX) {
                    error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    element = Subordinate_List_Element(pObject, array_index);
                    if (element) {
                        element->Node_Type = node_type;
                        error_code = ERROR_CODE_SUCCESS;
                    } else {
                        error_code = ERROR_CODE_OTHER;
                    }
                }
            } else {
                error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
        }
    }

    return error_code;
}

/**
 * @brief Decode a BACnetARRAY property element to determine the length
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Structured_View_Subordinate_Relationship_Member_Decode(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    int len = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        len = bacnet_enumerated_application_decode(apdu, apdu_size, NULL);
    }

    return len;
}

/**
 * @brief Write a value to a BACnetARRAY property element value
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param array_size [in] number of elements in the array, used writing array
 * element 0
 * @param apdu [in] encoded element value
 * @param apdu_size [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Structured_View_Subordinate_Relationship_Member_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *apdu,
    size_t apdu_size)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    uint32_t relationship = 0;
    BACNET_SUBORDINATE_DATA *element = NULL;
    int len = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (array_index == 0) {
            error_code =
                Structured_View_Subordinate_List_Resize(pObject, array_size);
        } else {
            array_index--; /* array index is 1..N, but we want 0..(N-1) */
            len = bacnet_enumerated_application_decode(
                apdu, apdu_size, &relationship);
            if (len > 0) {
                if (relationship > BACNET_RELATIONSHIP_PROPRIETARY_MAX) {
                    error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    element = Subordinate_List_Element(pObject, array_index);
                    if (element) {
                        element->Relationship = relationship;
                        error_code = ERROR_CODE_SUCCESS;
                    } else {
                        error_code = ERROR_CODE_OTHER;
                    }
                }
            } else {
                error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
        }
    }

    return error_code;
}

/**
 * @brief For a given object instance-number, returns the Subordinate_List
 * as a linked list with next pointers properly set
 * @param  object_instance - object-instance number of the object
 * @return pointer to the first element of the Subordinate_List, or NULL
 */
BACNET_SUBORDINATE_DATA *
Structured_View_Subordinate_List(uint32_t object_instance)
{
    struct object_data *pObject;
    BACNET_SUBORDINATE_DATA *first_element = NULL;
    BACNET_SUBORDINATE_DATA *element;
    BACNET_SUBORDINATE_DATA *prev_element = NULL;
    unsigned int count = 0;
    unsigned int i = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        count = Structured_View_Subordinate_List_Size(pObject);
        if (count > 0) {
            /* Iterate through all elements and set up next pointers */
            for (i = 0; i < count; i++) {
                element = Subordinate_List_Element(pObject, i);
                if (element) {
                    if (i == 0) {
                        /* First element */
                        first_element = element;
                    } else if (prev_element) {
                        /* Link previous element to current element */
                        prev_element->next = element;
                    }
                    /* Clear the next pointer for the current element */
                    element->next = NULL;
                    prev_element = element;
                }
            }
        }
    }

    return first_element;
}

/**
 * @brief For a given object instance-number, sets the Subordinate_List
 * @param  object_instance - object-instance number of the object
 * @param  subordinate_list - holds the Subordinate_List to be set
 */
void Structured_View_Subordinate_List_Set(
    uint32_t object_instance, BACNET_SUBORDINATE_DATA *subordinate_list)
{
    struct object_data *pObject;
    BACNET_SUBORDINATE_DATA *element, *data;
    KEY key;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        Subordinate_List_Purge(pObject);
        /* walk the linked list and add to Keylist */
        element = subordinate_list;
        while (element) {
            key = Keylist_Next_Empty_Key(pObject->Subordinate_List, 0);
            data = Subordinate_List_Element_Add(pObject->Subordinate_List, key);
            if (data) {
                memmove(data, element, sizeof(BACNET_SUBORDINATE_DATA));
                if (element->Annotation) {
                    data->Annotation = bacnet_strdup(element->Annotation);
                }
                data->next = NULL;
            }
            element = element->next;
        }
    }
}

/**
 * @brief For a given object instance-number, determine if two
 *  BACNET_SUBORDINATE_DATA elements are the same
 * @param  element1 - pointer to the first Subordinate_List element
 * @param  element2 - pointer to the second Subordinate_List element
 * @return true if the contents of the elements are the same, false otherwise
 */
bool Structured_View_Subordinate_List_Element_Same(
    BACNET_SUBORDINATE_DATA *element1, BACNET_SUBORDINATE_DATA *element2)
{
    if (element1 == element2) {
        return true;
    }
    if (!element1 || !element2) {
        return false;
    }
    if (element1->Device_Instance != element2->Device_Instance ||
        element1->Object_Type != element2->Object_Type ||
        element1->Object_Instance != element2->Object_Instance ||
        element1->Node_Type != element2->Node_Type ||
        element1->Relationship != element2->Relationship) {
        return false;
    }
    if (element1->Annotation && element2->Annotation) {
        if (strcmp(element1->Annotation, element2->Annotation) != 0) {
            return false;
        }
    } else if (element1->Annotation || element2->Annotation) {
        /* one is NULL and the other is not */
        return false;
    }

    return true;
}

/**
 * @brief Convert an array of BACnetSubordinateData to linked list
 * @param array pointer to element zero of the array
 * @param size number of elements in the array
 */
void Structured_View_Subordinate_List_Link_Array(
    BACNET_SUBORDINATE_DATA *array, size_t size)
{
    size_t i = 0;

    for (i = 0; i < size; i++) {
        if (i > 0) {
            array[i - 1].next = &array[i];
        }
        array[i].next = NULL;
    }
}

/**
 * @brief For a given object instance-number, returns the
 * Default_Subordinate_Relationship
 * @param  object_instance - object-instance number of the object
 * @return Default_Subordinate_Relationship or BACNET_RELATIONSHIP_DEFAULT
 * if not found
 */
BACNET_RELATIONSHIP
Structured_View_Default_Subordinate_Relationship(uint32_t object_instance)
{
    BACNET_RELATIONSHIP relationship = BACNET_RELATIONSHIP_DEFAULT;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        relationship = pObject->Default_Subordinate_Relationship;
    }

    return relationship;
}

/**
 * @brief For a given object instance-number, sets the
 * Default_Subordinate_Relationship
 * @param  object_instance - object-instance number of the object
 * @param  relationship - holds the Default_Subordinate_Relationship to be
 * set
 * @return  true if Default_Subordinate_Relationship was set
 */
bool Structured_View_Default_Subordinate_Relationship_Set(
    uint32_t object_instance, BACNET_RELATIONSHIP relationship)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        pObject->Default_Subordinate_Relationship = relationship;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the Represents
 *  property value
 * @param  object_instance - object-instance number of the object
 * @return Represents or NULL if not found
 */
BACNET_DEVICE_OBJECT_REFERENCE *
Structured_View_Represents(uint32_t object_instance)
{
    BACNET_DEVICE_OBJECT_REFERENCE *represents = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        represents = &pObject->Represents;
    }

    return represents;
}

/**
 * @brief For a given object instance-number, sets the Represents
 * property value
 * @param  object_instance - object-instance number of the object
 * @param  represents - holds the Represents to be set
 * @return  true if Represents was set
 */
bool Structured_View_Represents_Set(
    uint32_t object_instance, const BACNET_DEVICE_OBJECT_REFERENCE *represents)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_device_object_reference_copy(
            &pObject->Represents, represents);
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the number of
 * Subordinate_List elements
 * @param  object_instance - object-instance number of the object
 * @return number of Subordinate_List elements
 */
unsigned int Structured_View_Subordinate_List_Count(uint32_t object_instance)
{
    unsigned int count = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        count = Structured_View_Subordinate_List_Size(pObject);
    }

    return count;
}

/**
 * @brief For a given object instance-number, returns the number of
 * Subordinate_List elements
 * @param  object_instance - object-instance number of the object
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @return Subordinate_List element or NULL if not found
 */
BACNET_SUBORDINATE_DATA *Structured_View_Subordinate_List_Member(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index)
{
    BACNET_SUBORDINATE_DATA *element = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        element = Subordinate_List_Element(pObject, array_index);
    }

    return element;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
int Structured_View_Subordinate_List_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_DEVICE_OBJECT_REFERENCE value = { 0 };
    BACNET_SUBORDINATE_DATA *subordinate_list = NULL;

    subordinate_list =
        Structured_View_Subordinate_List_Member(object_instance, array_index);
    if (subordinate_list) {
        /* BACnetDeviceObjectReference */
        value.deviceIdentifier.type = OBJECT_DEVICE;
        value.deviceIdentifier.instance = subordinate_list->Device_Instance;
        value.objectIdentifier.type = subordinate_list->Object_Type;
        value.objectIdentifier.instance = subordinate_list->Object_Instance;
        apdu_len = bacapp_encode_device_obj_ref(apdu, &value);
    }

    return apdu_len;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
int Structured_View_Subordinate_Annotations_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_CHARACTER_STRING value = { 0 };
    BACNET_SUBORDINATE_DATA *subordinate_list = NULL;

    subordinate_list =
        Structured_View_Subordinate_List_Member(object_instance, array_index);
    if (subordinate_list) {
        /* BACnetCharacterString */
        characterstring_init_ansi(&value, subordinate_list->Annotation);
        apdu_len = encode_application_character_string(apdu, &value);
    }

    return apdu_len;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
int Structured_View_Subordinate_Node_Types_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_SUBORDINATE_DATA *subordinate_list = NULL;

    subordinate_list =
        Structured_View_Subordinate_List_Member(object_instance, array_index);
    if (subordinate_list) {
        /* BACnetNodeType */
        apdu_len =
            encode_application_enumerated(apdu, subordinate_list->Node_Type);
    }

    return apdu_len;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
int Structured_View_Subordinate_Relationships_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_SUBORDINATE_DATA *subordinate_list = NULL;

    subordinate_list =
        Structured_View_Subordinate_List_Member(object_instance, array_index);
    if (subordinate_list) {
        /* BACnetRelationship */
        apdu_len =
            encode_application_enumerated(apdu, subordinate_list->Relationship);
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
int Structured_View_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_CHARACTER_STRING char_string;
    uint32_t count = 0;
    uint8_t *apdu = NULL;
    uint16_t apdu_max = 0;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_max = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], rpdata->object_type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Structured_View_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], rpdata->object_type);
            break;
        case PROP_NODE_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Structured_View_Node_Type(rpdata->object_instance));
            break;
        case PROP_SUBORDINATE_LIST:
            count =
                Structured_View_Subordinate_List_Count(rpdata->object_instance);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Structured_View_Subordinate_List_Element_Encode, count, apdu,
                apdu_max);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Structured_View_Description(rpdata->object_instance));
            apdu_len = encode_application_character_string(apdu, &char_string);
            break;
        case PROP_NODE_SUBTYPE:
            characterstring_init_ansi(
                &char_string,
                Structured_View_Node_Subtype(rpdata->object_instance));
            apdu_len = encode_application_character_string(apdu, &char_string);
            break;
        case PROP_SUBORDINATE_ANNOTATIONS:
            count =
                Structured_View_Subordinate_List_Count(rpdata->object_instance);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Structured_View_Subordinate_Annotations_Element_Encode, count,
                apdu, apdu_max);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_SUBORDINATE_NODE_TYPES:
            count =
                Structured_View_Subordinate_List_Count(rpdata->object_instance);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Structured_View_Subordinate_Node_Types_Element_Encode, count,
                apdu, apdu_max);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_SUBORDINATE_RELATIONSHIPS:
            count =
                Structured_View_Subordinate_List_Count(rpdata->object_instance);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Structured_View_Subordinate_Relationships_Element_Encode, count,
                apdu, apdu_max);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_DEFAULT_SUBORDINATE_RELATIONSHIP:
            apdu_len = encode_application_enumerated(
                &apdu[0],
                Structured_View_Default_Subordinate_Relationship(
                    rpdata->object_instance));
            break;
        case PROP_REPRESENTS:
            apdu_len = bacapp_encode_device_obj_ref(
                &apdu[0], Structured_View_Represents(rpdata->object_instance));
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
 * For a given object instance-number, sets the object-name
 *
 * @param  object_instance - object-instance number of the object
 * @param  cstring - holds the object-name to be set
 *
 * @return  true if object-name was set
 */
static bool Structured_View_Object_Name_Write(
    BACNET_WRITE_PROPERTY_DATA *wp_data, BACNET_CHARACTER_STRING *cstring)
{
    bool status = false; /* return value */
    struct object_data *pObject;
    char *utf8_name = NULL;

    pObject = Keylist_Data(Object_List, wp_data->object_instance);
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
 * For a given object instance-number, sets the description property value
 *
 * @param  object_instance - object-instance number of the object
 * @param  cstring - holds the description to be set
 *
 * @return  true if description was set
 */
static bool Structured_View_Description_Write(
    BACNET_WRITE_PROPERTY_DATA *wp_data, BACNET_CHARACTER_STRING *cstring)
{
    bool status = false; /* return value */
    struct object_data *pObject;
    char *utf8_name = NULL;

    pObject = Keylist_Data(Object_List, wp_data->object_instance);
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
 * For a given object instance-number, sets the node-subtype property value
 *
 * @param  object_instance - object-instance number of the object
 * @param  cstring - holds the node-subtype to be set
 *
 * @return  true if node-subtype was set
 */
static bool Structured_View_Node_Subtype_Write(
    BACNET_WRITE_PROPERTY_DATA *wp_data, BACNET_CHARACTER_STRING *cstring)
{
    bool status = false; /* return value */
    struct object_data *pObject;
    char *utf8_name = NULL;

    pObject = Keylist_Data(Object_List, wp_data->object_instance);
    if (pObject) {
        utf8_name =
            write_property_characterstring_utf8_strdup(wp_data, cstring);
        if (utf8_name) {
            free(pObject->Node_Subtype);
            pObject->Node_Subtype = utf8_name;
            status = true;
        }
    } else {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
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
bool Structured_View_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
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
        wp_data->object_type, wp_data->object_property, wp_data->array_index);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_OBJECT_NAME:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (status) {
                status = Structured_View_Object_Name_Write(
                    wp_data, &value.type.Character_String);
            }
            break;
        case PROP_DESCRIPTION:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (status) {
                status = Structured_View_Description_Write(
                    wp_data, &value.type.Character_String);
            }
            break;
        case PROP_NODE_SUBTYPE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (status) {
                status = Structured_View_Node_Subtype_Write(
                    wp_data, &value.type.Character_String);
            }
            break;
        case PROP_NODE_TYPE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated > BACNET_NODE_TYPE_MAX) {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    status = Structured_View_Node_Type_Set(
                        wp_data->object_instance, value.type.Enumerated);
                }
            }
            break;
        case PROP_DEFAULT_SUBORDINATE_RELATIONSHIP:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated >
                    BACNET_RELATIONSHIP_PROPRIETARY_MAX) {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    status =
                        Structured_View_Default_Subordinate_Relationship_Set(
                            wp_data->object_instance, value.type.Enumerated);
                }
            }
            break;
        case PROP_REPRESENTS:
            status = write_property_type_valid(
                wp_data, &value,
                BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE);
            if (status) {
                status = Structured_View_Represents_Set(
                    wp_data->object_instance,
                    &value.type.Device_Object_Reference);
            }
            break;
        case PROP_SUBORDINATE_LIST:
            array_size = Structured_View_Subordinate_List_Count(
                wp_data->object_instance);
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                Structured_View_Subordinate_List_Member_Decode,
                Structured_View_Subordinate_List_Member_Write, array_size,
                wp_data->application_data, wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
        case PROP_SUBORDINATE_ANNOTATIONS:
            array_size = Structured_View_Subordinate_List_Count(
                wp_data->object_instance);
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                Structured_View_Subordinate_Annotation_Member_Decode,
                Structured_View_Subordinate_Annotation_Member_Write, array_size,
                wp_data->application_data, wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
        case PROP_SUBORDINATE_NODE_TYPES:
            array_size = Structured_View_Subordinate_List_Count(
                wp_data->object_instance);
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                Structured_View_Subordinate_Node_Type_Member_Decode,
                Structured_View_Subordinate_Node_Type_Member_Write, array_size,
                wp_data->application_data, wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
        case PROP_SUBORDINATE_RELATIONSHIPS:
            array_size = Structured_View_Subordinate_List_Count(
                wp_data->object_instance);
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                Structured_View_Subordinate_Relationship_Member_Decode,
                Structured_View_Subordinate_Relationship_Member_Write,
                array_size, wp_data->application_data,
                wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
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
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Structured_View_Context_Get(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return pObject->Context;
    }

    return NULL;
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void Structured_View_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * Creates a Structured View object
 * @param object_instance - object-instance number of the object
 * @return object_instance if the object is created, else
 * BACNET_MAX_INSTANCE
 */
uint32_t Structured_View_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;

    if (!Object_List) {
        Object_List = Keylist_Create();
    }
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
        pObject = calloc(1, sizeof(struct object_data));
        if (!pObject) {
            return BACNET_MAX_INSTANCE;
        }
        pObject->Object_Name = NULL;
        pObject->Description = NULL;
        pObject->Node_Subtype = NULL;
        pObject->Subordinate_List = Keylist_Create();
        pObject->Default_Subordinate_Relationship = BACNET_RELATIONSHIP_DEFAULT;
        pObject->Represents.deviceIdentifier.type = OBJECT_NONE;
        pObject->Represents.deviceIdentifier.instance = BACNET_MAX_INSTANCE;
        pObject->Represents.objectIdentifier.type = OBJECT_DEVICE;
        pObject->Represents.objectIdentifier.instance = BACNET_MAX_INSTANCE;
        pObject->Node_Type = BACNET_NODE_UNKNOWN;
        /* add to list */
        index = Keylist_Data_Add(Object_List, object_instance, pObject);
        if (index < 0) {
            free(pObject);
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * @brief Free the memory used by a Structured View object
 * @param pObject pointer to the object data to free
 */
static void Structured_View_Object_Free(struct object_data *pObject)
{
    if (pObject) {
        free(pObject->Description);
        free(pObject->Node_Subtype);
        free(pObject->Object_Name);
        Subordinate_List_Purge(pObject);
        Keylist_Delete(pObject->Subordinate_List);
        free(pObject);
    }
}

/**
 * Deletes a Structured View object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Structured_View_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        Structured_View_Object_Free(pObject);
        status = true;
    }

    return status;
}

/**
 * Deletes all the Structured View objects and their data
 */
void Structured_View_Cleanup(void)
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
                Structured_View_Object_Free(pObject);
            } while (pObject);
            Keylist_Delete(Object_List);
            Object_List = NULL;
        }
    }

#ifdef BAC_ROUTING
    Set_Routed_Device_Object_Index(current_dev_id);
#endif
}

/**
 * @brief Returns the approximate size of each Structured View object data
 */
size_t Structured_View_Size(void)
{
    return sizeof(struct object_data);
}

/**
 * Initializes the Structured View object data
 */
void Structured_View_Init(void)
{
    uint16_t dev_id;
#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
#endif

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
