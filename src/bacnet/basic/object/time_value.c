/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @date June 2023
 * @brief Time Value objects used by a BACnet device object
 *
 * @section DESCRIPTION
 *
  * The Time Value object is an object with a present-value that
 * uses an bacnet time data type.
*
 * @section LICENSE
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
#include "bacnet/proplist.h"
#include "bacnet/reject.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "time_value.h"

struct object_data {
    bool Changed : 1;
    bool Write_Enabled : 1;
    bool Out_Of_Service : 1;
    BACNET_TIME Present_Value;
    const char *Object_Name;
    const char *Description;
};

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* callback for present value writes */
static time_value_write_present_value_callback Time_Value_Write_Present_Value_Callback;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Time_Value_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_STATUS_FLAGS,
    -1 };

static const int Time_Value_Properties_Optional[] = { PROP_DESCRIPTION,
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, -1 };

static const int Time_Value_Properties_Proprietary[] = { -1 };

/* standard properties that are arrays for this object,
   but not necessary supported in this object */
static const int BACnetARRAY_Properties[] = { 
    PROP_EVENT_TIME_STAMPS, PROP_EVENT_MESSAGE_TEXTS, 
    PROP_EVENT_MESSAGE_TEXTS_CONFIG,
    PROP_VALUE_SOURCE_ARRAY, PROP_COMMAND_TIME_ARRAY, PROP_TAGS, -1 };

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
void Time_Value_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Time_Value_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Time_Value_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Time_Value_Properties_Proprietary;
    }

    return;
}

/**
 * Determines if a given Time Value instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Time_Value_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Time Value objects
 *
 * @return  Number of Time Value objects
 */
unsigned Time_Value_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Time Value objects where N is Time_Value_Count().
 *
 * @param  index - 0..N where N is Time_Value_Count()
 *
 * @return  object instance-number for the given index
 */
uint32_t Time_Value_Index_To_Instance(unsigned index)
{
    return Keylist_Key(Object_List, index);
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Time Value objects where N is Time_Value_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or Time_Value_Count()
 * if not valid.
 */
unsigned Time_Value_Instance_To_Index(uint32_t object_instance)
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
bool Time_Value_Present_Value(uint32_t object_instance, BACNET_TIME *value)
{
    bool status = false;
    struct object_data *pObject;
    BACNET_DATE date;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Out_Of_Service) {
            *value = pObject->Present_Value;
            status = true;
        } else {
            status = datetime_local(&date, value, NULL, NULL);
        }
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
bool Time_Value_Present_Value_Set(uint32_t object_instance, BACNET_TIME *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Present_Value = *value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - Bacnet time data object
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Time_Value_Present_Value_Write(uint32_t object_instance,
    BACNET_TIME *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    BACNET_TIME old_value = {0};

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if (pObject->Write_Enabled) {
            old_value = pObject->Present_Value;
            pObject->Present_Value = *value;
            if (Time_Value_Write_Present_Value_Callback) {
                Time_Value_Write_Present_Value_Callback(
                    object_instance, &old_value, value);
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
 * For a given object instance-number, returns the Out-of-service value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  Out-of-service value of the object
 */
bool Time_Value_Out_Of_Service(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = pObject->Out_Of_Service;
    }

    return status;
}

/**
 * For a given object instance-number, sets the Out-of-service value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - indicator of 'Out-of-service'
 *
 * @return  true if Out-of-service value is set.
 */
bool Time_Value_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Out_Of_Service = value;
        status = true;
    }

    return status;
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
bool Time_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[16] = "Time-4194303";

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(name_text, sizeof(name_text), "Time-%u", object_instance);
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
bool Time_Value_Name_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && new_name) {
        status = true;
        pObject->Object_Name = new_name;
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
char *Time_Value_Description(uint32_t object_instance)
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
bool Time_Value_Description_Set(uint32_t object_instance, char *new_name)
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
 * @brief Determine if the object property is a member of this object instance
 * @param object_instance - object-instance number of the object
 * @param object_property - object-property to be checked
 * @return true if the property is a member of this object instance
 */
static bool Property_List_Member(
    uint32_t object_instance, int object_property)
{
    bool found = false;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;

    (void)object_instance;
    Time_Value_Property_Lists(
        &pRequired, &pOptional, &pProprietary);
    found = property_list_member(pRequired, object_property);
    if (!found) {
        found = property_list_member(pOptional, object_property);
    }
    if (!found) {
        found = property_list_member(pProprietary, object_property);
    }

    return found;
}

/**
 * @brief Determine if the object property is a BACnetARRAY property
 * @param object_property - object-property to be checked
 * @return true if the property is a BACnetARRAY property
 */
static bool BACnetARRAY_Property(
    int object_property)
{
    return property_list_member(
            BACnetARRAY_Properties, object_property);
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
int Time_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
    BACNET_TIME value;

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
            Time_Value_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], rpdata->object_type);
            break;
        case PROP_PRESENT_VALUE:
            if (Time_Value_Present_Value(rpdata->object_instance, &value)) {
                apdu_len = encode_application_time(apdu, &value);
            }
            break;

        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                Time_Value_Out_Of_Service(rpdata->object_instance));
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0],
                Time_Value_Out_Of_Service(rpdata->object_instance));
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, Time_Value_Description(rpdata->object_instance));
            apdu_len = encode_application_character_string(apdu, &char_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) &&
        (!BACnetARRAY_Property(rpdata->object_property)) &&
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
bool Time_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    BACNET_APPLICATION_DATA_VALUE value;
    int len = 0;

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
    if ((!BACnetARRAY_Property(wp_data->object_property)) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (Time_Value_Out_Of_Service(wp_data->object_instance)) {
                status = write_property_type_valid(wp_data, &value,
                    BACNET_APPLICATION_TAG_TIME);
                if (status) {
                    status = Time_Value_Present_Value_Write(
                        wp_data->object_instance,
                        &value.type.Time, wp_data->priority,
                        &wp_data->error_class, &wp_data->error_code);
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        default:
            if (Property_List_Member(
                    wp_data->object_instance, wp_data->object_property)) {
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
void Time_Value_Write_Present_Value_Callback_Set(
    time_value_write_present_value_callback cb)
{
    Time_Value_Write_Present_Value_Callback = cb;
}

/**
 * @brief Determines the status flags for a given object instance-number
 * @param object_instance - object-instance number of the object
 * @return  status flags bitstring octet
 */
uint8_t Time_Value_Status_Flags(uint32_t object_instance)
{
    BACNET_BIT_STRING bit_string;
 
    bitstring_init(&bit_string);
    bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
    bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
    bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
        Time_Value_Out_Of_Service(object_instance));

    return bitstring_octet(&bit_string, 0);
}

/**
 * @brief Determines a object write-enabled flag state
 * @param object_instance - object-instance number of the object
 * @return  write-enabled status flag
 */
bool Time_Value_Write_Enabled(uint32_t object_instance)
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
void Time_Value_Write_Enable(uint32_t object_instance)
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
void Time_Value_Write_Disable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Write_Enabled = false;
    }
}

/**
 * Creates a Time Value object
 * @param object_instance - object-instance number of the object
 * @return object_instance if the object is created, else BACNET_MAX_INSTANCE
 */
uint32_t Time_Value_Create(uint32_t object_instance)
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
        memset(&pObject->Present_Value, 0, sizeof(pObject->Present_Value));
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
 * Deletes an Time Value object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Time_Value_Delete(uint32_t object_instance)
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
void Time_Value_Cleanup(void)
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
 * Initializes the Time Value object data
 */
void Time_Value_Init(void)
{
    Object_List = Keylist_Create();
    if (Object_List) {
        atexit(Time_Value_Cleanup);
    }
}
