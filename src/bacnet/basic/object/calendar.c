/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @date June 2022
 * @brief The Calendar object is an object with a present-value that
 * uses a BOOLEAN data type, and features a Date_List that is a
 * BACnetLIST of BACnetCalendarEntry.
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
#include "bacnet/abort.h"
#include "bacnet/apdu.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/cov.h"
#include "bacnet/npdu.h"
#include "bacnet/proplist.h"
#include "bacnet/reject.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
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
static calendar_write_present_value_callback
    Calendar_Write_Present_Value_Callback;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Calendar_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME, PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,     PROP_DATE_LIST,   -1
};

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
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
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
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  old_value - boolean, previous value of the Present Value property
 * @param  value - boolean, new value of the Present Value property
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Calendar_Present_Value_Write(
    uint32_t object_instance,
    bool old_value,
    bool value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if (pObject->Write_Enabled) {
            old_value = pObject->Present_Value;
            pObject->Present_Value = value;
            if (Calendar_Write_Present_Value_Callback) {
                Calendar_Write_Present_Value_Callback(
                    object_instance, old_value, value);
            }
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * For a given date-list, deletes the entire data-list.
 * @param  list - the list to be deleted
 */
static void Calendar_Date_List_Clean(OS_Keylist list)
{
    void *data;
    while (Keylist_Count(list) > 0) {
        data = Keylist_Data_Pop(list);
        free(data);
    }
}

/**
 * For a given object instance-number, returns the Calendar entity by index.
 *
 * @param  object_instance - object-instance number of the object
 * @param  index - index of entity
 *
 * @return Calendar entity.
 */
BACNET_CALENDAR_ENTRY *
Calendar_Date_List_Get(uint32_t object_instance, uint8_t index)
{
    BACNET_CALENDAR_ENTRY *entry = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        entry = Keylist_Data_Index(pObject->Date_List, index);
    }

    return entry;
}

/**
 * For a given object instance-number, adds a Calendar entity to entities list.
 *
 * @param  object_instance - object-instance number of the object
 * @param  entity - Calendar entity
 *
 * @return  true if the entity is add successfully.
 */
bool Calendar_Date_List_Add(
    uint32_t object_instance, const BACNET_CALENDAR_ENTRY *value)
{
    int index = 0;
    BACNET_CALENDAR_ENTRY *entry;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return false;
    }

    entry = calloc(1, sizeof(BACNET_CALENDAR_ENTRY));
    if (!entry) {
        return false;
    }
    *entry = *value;
    index = Keylist_Data_Add(
        pObject->Date_List, Keylist_Count(pObject->Date_List), entry);
    if (index < 0) {
        free(entry);
        return false;
    }

    return true;
}

/**
 * For a given object instance-number, clears to entities list.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if entities list is clear successfully.
 */
bool Calendar_Date_List_Delete_All(uint32_t object_instance)
{
    struct object_data *pObject;
    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return false;
    }

    Calendar_Date_List_Clean(pObject->Date_List);

    return true;
}

/**
 * For a given object instance-number, returns the entities list length.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  size of entities list.
 */
int Calendar_Date_List_Count(uint32_t object_instance)
{
    struct object_data *pObject;
    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return 0;
    }

    return Keylist_Count(pObject->Date_List);
}

/**
 * @brief Encode a Calendar entity list complex data type
 *
 * @param object_instance - object-instance number of the object
 * @param apdu - the APDU buffer
 * @param apdu_size - size of the apdu buffer.
 *
 * @return bytes encoded or zero on error.
 */
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
        apdu_len += bacnet_calendar_entry_encode(NULL, entry);
    }
    if (apdu_len > max_apdu) {
        return BACNET_STATUS_ABORT;
    }
    apdu_len = 0;
    for (index = 0; index < size; index++) {
        entry = Calendar_Date_List_Get(object_instance, index);
        apdu_len += bacnet_calendar_entry_encode(&apdu[apdu_len], entry);
    }

    return apdu_len;
}

/**
 * For a given object instance-number, determines the present-value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  present-value of the object
 */
bool Calendar_Present_Value(uint32_t object_instance)
{
    BACNET_DATE date;
    BACNET_TIME time;
    BACNET_CALENDAR_ENTRY *entry = NULL;
    unsigned size = 0;
    int index;

    datetime_local(&date, &time, NULL, NULL);

    size = Calendar_Date_List_Count(object_instance);
    for (index = 0; index < size; index++) {
        entry = Calendar_Date_List_Get(object_instance, index);
        if (bacapp_date_in_calendar_entry(&date, entry)) {
            return true;
        }
    }

    return false;
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
    char name_text[32] = "CALENDAR-4194303";

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "CALENDAR-%lu",
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
bool Calendar_Name_Set(uint32_t object_instance, const char *new_name)
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
const char *Calendar_Name_ASCII(uint32_t object_instance)
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
const char *Calendar_Description(uint32_t object_instance)
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
bool Calendar_Description_Set(uint32_t object_instance, const char *new_name)
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
    int apdu_max = 0;
    bool value = false;

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
            Calendar_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], rpdata->object_type);
            break;
        case PROP_PRESENT_VALUE:
            value = Calendar_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_boolean(apdu, value);
            break;
        case PROP_DATE_LIST:
            apdu_len = Calendar_Date_List_Encode(
                rpdata->object_instance, apdu, apdu_max);
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
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    int iOffset;
    BACNET_CALENDAR_ENTRY entry;
    bool pv_old;
    bool pv;

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
    switch (wp_data->object_property) {
        case PROP_DATE_LIST:
            pv_old = Calendar_Present_Value(wp_data->object_instance);
            Calendar_Date_List_Delete_All(wp_data->object_instance);
            iOffset = 0;
            /* decode all packed */
            while (iOffset < wp_data->application_data_len) {
                len = bacnet_calendar_entry_decode(
                    &wp_data->application_data[iOffset],
                    wp_data->application_data_len - iOffset, &entry);
                if (len == BACNET_STATUS_REJECT) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }
                iOffset += len;
                Calendar_Date_List_Add(wp_data->object_instance, &entry);
            }
            pv = Calendar_Present_Value(wp_data->object_instance);
            status = Calendar_Present_Value_Write(
                wp_data->object_instance, pv_old, pv, wp_data->priority,
                &wp_data->error_class, &wp_data->error_code);
            break;
        default:
            if (property_lists_member(
                    Calendar_Properties_Required, Calendar_Properties_Optional,
                    Calendar_Properties_Proprietary,
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
 * @brief Sets a callback used when present-value is written from BACnet
 * @param cb - callback used to provide indications
 */
void Calendar_Write_Present_Value_Callback_Set(
    calendar_write_present_value_callback cb)
{
    Calendar_Write_Present_Value_Callback = cb;
}

/**
 * @brief Determines a object write-enabled flag state
 * @param object_instance - object-instance number of the object
 * @return  write-enabled status flag
 */
bool Calendar_Write_Enabled(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Write_Enabled;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Calendar_Write_Enable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Write_Enabled = true;
    }
}

/**
 * @brief For a given object instance-number, clears the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Calendar_Write_Disable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Write_Enabled = false;
    }
}

/**
 * Creates a Calendar object
 * @param object_instance - object-instance number of the object
 * @return object_instance if the object is created, else BACNET_MAX_INSTANCE
 */
uint32_t Calendar_Create(uint32_t object_instance)
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
        pObject->Present_Value = false;
        pObject->Date_List = Keylist_Create();
        pObject->Changed = false;
        pObject->Write_Enabled = false;
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
        Calendar_Date_List_Clean(pObject->Date_List);
        Keylist_Delete(pObject->Date_List);
        free(pObject);
        status = true;
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
                Calendar_Date_List_Clean(pObject->Date_List);
                Keylist_Delete(pObject->Date_List);
                free(pObject);
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
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
