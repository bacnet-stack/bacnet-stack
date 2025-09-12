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
#include "bacnet/bacerror.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/proplist.h"
#include "bacnet/property.h"
#include "bacnet/reject.h"
#include "bacnet/rp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "structured_view.h"

struct object_data {
    const char *Object_Name;
    const char *Description;
    BACNET_NODE_TYPE Node_Type;
    const char *Node_Subtype;
    BACNET_SUBORDINATE_DATA *Subordinate_List;
    BACNET_RELATIONSHIP Default_Subordinate_Relationship;
    BACNET_DEVICE_OBJECT_REFERENCE Represents;
};

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,      PROP_OBJECT_TYPE,
    PROP_NODE_TYPE,         PROP_SUBORDINATE_LIST, -1
};

static const int Properties_Optional[] = {
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

static const int Properties_Proprietary[] = { -1 };

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
    char name_text[24] = "Structured-View-4194303";

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
        pObject->Object_Name = new_name;
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
 * @return  true if object-name was set
 */
bool Structured_View_Description_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        pObject->Description = new_name;
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
        pObject->Node_Subtype = new_name;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the Subordinate_List
 * @param  object_instance - object-instance number of the object
 * @return Subordinate_List or NULL if not found
 */
BACNET_SUBORDINATE_DATA *
Structured_View_Subordinate_List(uint32_t object_instance)
{
    BACNET_SUBORDINATE_DATA *subordinate_list = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        subordinate_list = pObject->Subordinate_List;
    }

    return subordinate_list;
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

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Subordinate_List = subordinate_list;
    }
}

/**
 * @brief For a given object instance-number, returns the
 * Default_Subordinate_Relationship
 * @param  object_instance - object-instance number of the object
 * @return Default_Subordinate_Relationship or BACNET_RELATIONSHIP_DEFAULT if
 * not found
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
 * @param  relationship - holds the Default_Subordinate_Relationship to be set
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
    BACNET_SUBORDINATE_DATA *subordinate_list = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        subordinate_list = pObject->Subordinate_List;
        while (subordinate_list) {
            count++;
            subordinate_list = subordinate_list->next;
        }
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
    BACNET_ARRAY_INDEX index = 0;
    BACNET_SUBORDINATE_DATA *subordinate_list = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        subordinate_list = pObject->Subordinate_List;
        while (subordinate_list) {
            if (index == array_index) {
                break;
            }
            index++;
            subordinate_list = subordinate_list->next;
        }
    }

    return subordinate_list;
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
        characterstring_init_ansi(&value, subordinate_list->Annotations);
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
 * Creates a Structured View object
 * @param object_instance - object-instance number of the object
 * @return object_instance if the object is created, else BACNET_MAX_INSTANCE
 */
uint32_t Structured_View_Create(uint32_t object_instance)
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
    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (!pObject) {
            return BACNET_MAX_INSTANCE;
        }
        pObject->Object_Name = NULL;
        pObject->Description = NULL;
        pObject->Node_Subtype = NULL;
        pObject->Subordinate_List = NULL;
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
 * Deletes an Structured View object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Structured_View_Delete(uint32_t object_instance)
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
 * Deletes all the Time Values and their data
 */
void Structured_View_Cleanup(void)
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
 * Initializes the Structured View object data
 */
void Structured_View_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
