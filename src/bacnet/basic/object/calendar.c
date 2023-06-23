/**
 * @file
 * @author Steve Karg
 * @date June 2022
 * @brief Calendar objects, customize for your use
 *
 * @section DESCRIPTION
 *
 * The Calendar object is an object with a present-value that
 * uses an x,y color single precision floating point data type.
 *
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/cov.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "calendar.h"

struct object_data {
    bool Changed : 1;
    bool Write_Enabled : 1;
    bool Present_Value;
    OS_Keylist Date_List;
    const char *Object_Name;
    const char *Description;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* callback for present value writes */
static calendar_write_present_value_callback Calendar_Write_Present_Value_Callback;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Calendar_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_DATE_LIST,
    -1 };

static const int Calendar_Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int Calendar_Properties_Proprietary[] = { -1 };

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
void Calendar_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Calendar_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Calendar_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Calendar_Properties_Proprietary;
    }

    return;
}

/**
 * Determines if a given Calendar instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Calendar_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Calendar objects
 *
 * @return  Number of Calendar objects
 */
unsigned Calendar_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Calendar objects where N is Calendar_Count().
 *
 * @param  index - 0..N where N is Calendar_Count()
 *
 * @return  object instance-number for the given index
 */
uint32_t Calendar_Index_To_Instance(unsigned index)
{
    return Keylist_Key(Object_List, index);
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Calendar objects where N is Calendar_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or Calendar_Count()
 * if not valid.
 */
unsigned Calendar_Instance_To_Index(uint32_t object_instance)
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
bool Calendar_Present_Value(uint32_t object_instance, bool *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        *value = pObject->Present_Value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - indicator of 'present value'
 *
 * @return  true if values are within range and present-value is set.
 */
bool Calendar_Present_Value_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Present_Value = value;
        status = true;
    }

    return status;
}


BACNET_CALENDAR_ENTRY *Calendar_Date_List_Get(
    uint32_t object_instance, uint8_t index)
{
    BACNET_CALENDAR_ENTRY *entry = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        entry = Keylist_Data_Index(pObject->Date_List, index);
    }

    return entry;
}

bool Calendar_Date_List_Add(uint32_t object_instance,
    BACNET_CALENDAR_ENTRY *value)
{
    bool st = false;
    BACNET_CALENDAR_ENTRY *entry;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject)
        return false;
     
    entry = calloc(1, sizeof(BACNET_CALENDAR_ENTRY));
    if (!entry)
        return false;

    *entry = *value;
    st = Keylist_Data_Add(
        pObject->Date_List, Keylist_Count(pObject->Date_List), entry);

    return st;
}

bool Calendar_Date_List_Delete_All(uint32_t object_instance)
{
    struct object_data *pObject;
    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject)
        return false;

    Keylist_Delete(pObject->Date_List);
    pObject->Date_List = Keylist_Create();

    return true;
}

uint8_t Calendar_Date_List_Count(uint32_t object_instance)
{
    uint8_t count = 0;
    struct object_data *pObject;
    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject)
        return 0;

    count = Keylist_Count(pObject->Date_List);

    return count;
}

int Calendar_Date_List_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu)
{
    BACNET_CALENDAR_ENTRY *entry = NULL;
    int apdu_len = 0;
    unsigned index = 0;
    unsigned size = 0;

    size = Calendar_Date_List_Count(object_instance);
    for (index = 0; index < size; index++) {
        entry = Calendar_Date_List_Get(object_instance, index);
        apdu_len += bacapp_encode_CalendarEntry(NULL, entry);
    }
    if (apdu_len > max_apdu) {
        return BACNET_STATUS_ABORT;
    }
    apdu_len = 0;
    for (index = 0; index < size; index++) {
        entry = Calendar_Date_List_Get(object_instance, index);
        apdu_len += bacapp_encode_CalendarEntry(&apdu[apdu_len], entry);
    }

    return apdu_len;
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
bool Calendar_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[16] = "CALENDAR-4194303";

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(name_text, sizeof(name_text), "CALENDAR-%u", object_instance);
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
bool Calendar_Name_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    BACNET_CHARACTER_STRING object_name;
    BACNET_OBJECT_TYPE found_type = 0;
    uint32_t found_instance = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && new_name) {
        /* All the object names in a device must be unique */
        characterstring_init_ansi(&object_name, new_name);
        if (Device_Valid_Object_Name(
                &object_name, &found_type, &found_instance)) {
            if ((found_type == OBJECT_COLOR) &&
                (found_instance == object_instance)) {
                /* writing same name to same object */
                status = true;
            } else {
                /* duplicate name! */
                status = false;
            }
        } else {
            status = true;
            pObject->Object_Name = new_name;
            Device_Inc_Database_Revision();
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the description
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return description text or NULL if not found
 */
char *Calendar_Description(uint32_t object_instance)
{
    char *name = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Description) {
            name = (char *)pObject->Description;
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
bool Calendar_Description_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && new_name) {
        status = true;
        pObject->Description = new_name;
    }

    return status;
}

static int Calendar_Date_List_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu)
{
    BACNET_CALENDAR_ENTRY *entry = NULL;
    int apdu_len = 0;
    unsigned index = 0;
    unsigned size = 0;

    size = Calendar_Date_List_Count(object_instance);
    for (index = 0; index < size; index++) {
        entry = Calendar_Date_List_Get(object_instance, index);
        apdu_len += bacapp_encode_CalendarEntry(NULL, entry);
    }
    if (apdu_len > max_apdu) {
        return BACNET_STATUS_ABORT;
    }
    apdu_len = 0;
    for (index = 0; index < size; index++) {
        entry = Calendar_Date_List_Get(object_instance, index);
        apdu_len += bacapp_encode_CalendarEntry(&apdu[apdu_len], entry);
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
int Calendar_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
    bool value = false;
    BACNET_COLOR_COMMAND Calendar_command = { 0 };

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
            Calendar_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], rpdata->object_type);
            break;
        case PROP_PRESENT_VALUE:
            if (Calendar_Present_Value(rpdata->object_instance, &value)) {
                apdu_len = encode_application_boolean(apdu, value);
            }
            break;
        case PROP_DATE_LIST:
            apdu_len = Calendar_Date_List_Encode(
                rpdata->object_instance, apdu, apdu_size);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, Calendar_Description(rpdata->object_instance));
            apdu_len = encode_application_character_string(apdu, &char_string);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->object_property != PROP_EVENT_TIME_STAMPS) &&
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
bool Calendar_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    int iOffset;
    uint8_t idx;
    int len = 0;
    BACNET_CALENDAR_ENTRY entry;

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
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Calendar_Present_Value_Set(wp_data->object_instance,
                    value.type.Boolean);
            }
            break;
            
        case PROP_DATE_LIST:
            Calendar_Date_List_Delete_All(wp_data->object_instance);
            idx = 0;
            iOffset = 0;
            /* decode all packed */
            while (iOffset < wp_data->application_data_len) {
                len = bacapp_decode_CalendarEntry(
                    &wp_data->application_data[iOffset],
                    wp_data->application_data_len - iOffset,
                    &entry);
                if (len == BACNET_STATUS_REJECT) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }
                iOffset += len;
                Calendar_Date_List_Add(wp_data->object_instance, &value);
            }
            break;
            
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_TYPE:
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}

/**
 * @brief Sets a callback used when present-value is written from BACnet
 * @param cb - callback used to provide indications
 */
void Calendar_Write_Present_Value_Callback_Set(
    Calendar_write_present_value_callback cb)
{
    Calendar_Write_Present_Value_Callback = cb;
}

/**
 * Creates a Calendar object
 * @param object_instance - object-instance number of the object
 */
bool Calendar_Create(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;
    int index = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (pObject) {
            pObject->Object_Name = NULL;
            pObject->Description = NULL;
            pObject->Present_Value  = false;
            pObject->Date_List = Keylist_Create();
            pObject->Changed = false;
            pObject->Write_Enabled = false;
            /* add to list */
            index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if (index >= 0) {
                status = true;
                Device_Inc_Database_Revision();
            }
        }
    }

    return status;
}

/**
 * Deletes an Calendar object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Calendar_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
        Device_Inc_Database_Revision();
    }

    return status;
}

/**
 * Deletes all the Calendars and their data
 */
void Calendar_Cleanup(void)
{
    struct object_data *pObject;

    if (Object_List) {
        do {
            pObject = Keylist_Data_Pop(Object_List);
            if (pObject) {
                free(pObject);
                Device_Inc_Database_Revision();
            }
        } while (pObject);
        Keylist_Delete(Object_List);
        Object_List = NULL;
    }
}

/**
 * Initializes the Calendar object data
 */
void Calendar_Init(void)
{
    Object_List = Keylist_Create();
    if (Object_List) {
        atexit(Calendar_Cleanup);
    }
}
