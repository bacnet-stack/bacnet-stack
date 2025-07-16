/**
 * @file
 * @brief The Event Enrollment object type defines a standardized object
 *  that represents and contains the information required for
 *  algorithmic reporting of events.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2025
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
#include "bacnet/cov.h"
#include "bacnet/apdu.h"
#include "bacnet/event.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bacnet/basic/object/event_enrollment.h"

struct object_data {
    const char *Object_Name;
    const char *Description;
    BACNET_EVENT_TYPE Event_Type;
    BACNET_NOTIFY_TYPE Notify_Type;
    BACNET_EVENT_PARAMETER Event_Parameters;
    BACNET_OBJECT_PROPERTY_REFERENCE Object_Property_Reference;
    BACNET_EVENT_STATE Event_State;
    BACNET_EVENT_ENABLE Event_Enable;
    BACNET_EVENT_TRANSITION_BITS Acked_Transitions;
    uint32_t Notification_Class;
    BACNET_TIMESTAMP Event_Time_Stamps[3];
    bool Event_Detection_Enable;
    BACNET_RELIABILITY Reliability;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Event_Enrollment_Properties_Required[] = {
    /* unordered list of properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_EVENT_TYPE,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_PARAMETERS,
    PROP_OBJECT_PROPERTY_REFERENCE,
    PROP_EVENT_STATE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFICATION_CLASS,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_DETECTION_ENABLE,
    PROP_STATUS_FLAGS,
    PROP_RELIABILITY,
    -1
};

static const int Event_Enrollment_Properties_Optional[] = {
    /* unordered list of properties */
    PROP_DESCRIPTION,
#if 0
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_EVENT_MESSAGE_TEXTS_CONFIG,
    PROP_EVENT_ALGORITHM_INHIBIT_REF,
    PROP_EVENT_ALGORITHM_INHIBIT,
    PROP_TIME_DELAY_NORMAL,
    PROP_FAULT_TYPE,
    PROP_FAULT_PARAMETERS,
    PROP_AUDIT_LEVEL,
    PROP_AUDITABLE_OPERATIONS,
    PROP_TAGS,
    PROP_PROFILE_LOCATION,
    PROP_PROFILE_NAME,
#endif
    -1
};

static const int Event_Enrollment_Properties_Proprietary[] = { -1 };

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
void Event_Enrollment_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Event_Enrollment_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Event_Enrollment_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Event_Enrollment_Properties_Proprietary;
    }

    return;
}

/**
 * Determines if a given Color instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Event_Enrollment_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Color objects
 *
 * @return  Number of Color objects
 */
unsigned Event_Enrollment_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Color objects where N is Event_Enrollment_Count().
 *
 * @param  index - 0..N where N is Event_Enrollment_Count()
 *
 * @return  object instance-number for the given index
 */
uint32_t Event_Enrollment_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Color objects where N is Event_Enrollment_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or Event_Enrollment_Count()
 * if not valid.
 */
unsigned Event_Enrollment_Instance_To_Index(uint32_t object_instance)
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
bool Event_Enrollment_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[25] = "EVENT-ENROLLMENT-4194303";

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "EVENT-ENROLLMENT-%u",
                object_instance);
            status = characterstring_init_ansi(object_name, name_text);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the object-name
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 *
 * @return  true if object-name was set
 */
bool Event_Enrollment_Name_ASCII_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Object_Name = new_name;
        status = true;
    }

    return status;
}

/**
 * @brief Return the object name ANSI-C string
 * @param object_instance [in] BACnet object instance number
 * @return object name or NULL if not found
 */
const char *Event_Enrollment_Name_ASCII(uint32_t object_instance)
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
const char *Event_Enrollment_Description(uint32_t object_instance)
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
bool Event_Enrollment_Description_Set(
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
 * Updates the color object tracking value per ramp or fade
 *
 * @param  object_instance - object-instance number of the object
 * @param milliseconds - number of milliseconds elapsed
 */
void Event_Enrollment_Timer(uint32_t object_instance, uint16_t milliseconds)
{
    struct object_data *pObject;

    (void)milliseconds;
    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        /* do something interesting */
    }
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
int Event_Enrollment_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], rpdata->object_type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Event_Enrollment_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], rpdata->object_type);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Event_Enrollment_Description(rpdata->object_instance));
            apdu_len = encode_application_character_string(apdu, &char_string);
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
bool Event_Enrollment_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    int apdu_size = 0;
    const uint8_t *apdu = NULL;

    /* decode the some of the request */
    apdu = wp_data->application_data;
    apdu_size = wp_data->application_data_len;
    len = bacapp_decode_known_property(
        apdu, apdu_size, &value, wp_data->object_type,
        wp_data->object_property);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    switch (wp_data->object_property) {
        default:
            if (property_lists_member(
                    Event_Enrollment_Properties_Required,
                    Event_Enrollment_Properties_Optional,
                    Event_Enrollment_Properties_Proprietary,
                    wp_data->object_property)) {
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
 * @brief Creates a Color object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Event_Enrollment_Create(uint32_t object_instance)
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
        if (pObject) {
            pObject->Object_Name = NULL;
            pObject->Description = NULL;
            pObject->Event_Type = EVENT_NONE;
            pObject->Notify_Type = NOTIFY_EVENT;
            pObject->Object_Property_Reference.object_identifier.type =
                OBJECT_NONE;
            pObject->Object_Property_Reference.object_identifier.instance =
                BACNET_MAX_INSTANCE;
            pObject->Object_Property_Reference.property_array_index =
                BACNET_ARRAY_ALL;
            pObject->Object_Property_Reference.property_identifier =
                MAX_BACNET_PROPERTY_ID;
            pObject->Event_State = EVENT_STATE_NORMAL;
            pObject->Event_Enable = false;
            pObject->Acked_Transitions = TRANSITION_TO_OFFNORMAL;
            pObject->Notification_Class = 0;
            bacapp_timestamp_sequence_set(&pObject->Event_Time_Stamps[0], 0);
            bacapp_timestamp_sequence_set(&pObject->Event_Time_Stamps[1], 0);
            bacapp_timestamp_sequence_set(&pObject->Event_Time_Stamps[2], 0);
            pObject->Event_Detection_Enable = false;
            pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
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
 * Deletes an object instance
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Event_Enrollment_Delete(uint32_t object_instance)
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
 * Deletes all the objects and their data
 */
void Event_Enrollment_Cleanup(void)
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
void Event_Enrollment_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
