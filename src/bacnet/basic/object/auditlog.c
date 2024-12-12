/**
 * @file
 * @brief Audit Log object, customize for your use
 * @details  An Audit Log object combines audit notifications
 *  from operation sources and operation targets and stores the
 *  combined record in an internal buffer for subsequent retrieval.
 *  Each timestamped buffer entry is called an audit log "record."
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacaudit.h"
#include "bacnet/apdu.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/datetime.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "auditlog.h"

#ifndef BACNET_AUDIT_LOG_RECORDS_MAX
#define BACNET_AUDIT_LOG_RECORDS_MAX 128
#endif

struct object_data {
    bool Enable;
    bool Out_Of_Service;
    OS_Keylist Records;
    int Record_Index;
    int Record_Count_Max;
    int Record_Count_Total;
    const char *Object_Name;
    const char *Description;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;

static const int Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
                                           PROP_OBJECT_NAME,
                                           PROP_OBJECT_TYPE,
                                           PROP_STATUS_FLAGS,
                                           PROP_EVENT_STATE,
                                           PROP_ENABLE,
                                           PROP_BUFFER_SIZE,
                                           PROP_LOG_BUFFER,
                                           PROP_RECORD_COUNT,
                                           PROP_TOTAL_RECORD_COUNT,
                                           -1 };

static const int Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int Properties_Proprietary[] = { -1 };

static const int BACnetARRAY_Properties[] = {
    /* standard properties that are arrays for this object */
    PROP_LOG_BUFFER,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_EVENT_MESSAGE_TEXTS_CONFIG,
    PROP_TAGS,
    -1
};

/**
 * @brief Determine if the object property is a BACnetARRAY property
 * @param object_property - object-property to be checked
 * @return true if the property is a BACnetARRAY property
 */
static bool BACnetARRAY_Property(int object_property)
{
    return property_list_member(BACnetARRAY_Properties, object_property);
}

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
void Audit_Log_Property_Lists(
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
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct object_data *Object_Data(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Determines if a given object instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Audit_Log_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of objects
 * @return  Number of objects
 */
unsigned Audit_Log_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * of objects where N is the count.
 * @param  index - 0..N value
 * @return  object instance-number for a valid given index, or UINT32_MAX
 */
uint32_t Audit_Log_Index_To_Instance(unsigned index)
{
    uint32_t instance = UINT32_MAX;

    (void)Keylist_Index_Key(Object_List, index, &instance);

    return instance;
}

/**
 * @brief For a given object instance-number, determines a 0..N index
 * of objects where N is the count.
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number, or count if not valid.
 */
unsigned Audit_Log_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * Get the current time from the Device object
 *
 * @return current time in epoch seconds
 */
static bacnet_time_t Audit_Log_Epoch_Seconds_Now(void)
{
    bacnet_time_t now = 0;
    BACNET_DATE_TIME bdatetime = { 0 };

    if (datetime_local(&bdatetime.date, &bdatetime.time, NULL, NULL)) {
        now = datetime_seconds_since_epoch(&bdatetime);
    }

    return now;
}

/**
 * For a given date-list, deletes the entire data-list.
 * @param  list - the list to be deleted
 */
static void Audit_Log_Records_Clean(OS_Keylist list)
{
    void *data;
    while (Keylist_Count(list) > 0) {
        data = Keylist_Data_Pop(list);
        free(data);
    }
}

/**
 * For a given object instance-number, returns the Audit Log entity by index.
 *
 * @param  object_instance - object-instance number of the object
 * @param  index - index of entity
 *
 * @return Audit Log entity.
 */
BACNET_AUDIT_LOG_RECORD *
Audit_Log_Records_Get(uint32_t object_instance, uint8_t index)
{
    BACNET_AUDIT_LOG_RECORD *entry = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        entry = Keylist_Data_Index(pObject->Records, index);
    }

    return entry;
}

/**
 * For a given object, checks that Audit Log Records are available for adding.
 * @param  pObject - object data
 * @return  true if records are available for adding, false if not.
 */
static bool Audit_Log_Records_Available(struct object_data *pObject)
{
    int count;

    if (!pObject) {
        return false;
    }
    count = Keylist_Count(pObject->Records);
    if (count >= pObject->Record_Count_Max) {
        return false;
    }

    return true;
}

/**
 * For a given object instance-number, adds a Audit Log entity to entities list.
 *
 * @note If the log buffer becomes full, the least recent log records are
 *  overwritten when new log records are added.
 *
 * @param  object_instance - object-instance number of the object
 * @param  entity - Audit Log entity
 *
 * @return  true if the entity is add successfully.
 */
bool Audit_Log_Records_Add(
    uint32_t object_instance, const BACNET_AUDIT_LOG_RECORD *value)
{
    bool status = false;
    BACNET_AUDIT_LOG_RECORD *entry;
    struct object_data *pObject;
    int count;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return false;
    }
    if (Audit_Log_Records_Available(pObject)) {
        entry = calloc(1, sizeof(BACNET_AUDIT_LOG_RECORD));
        if (!entry) {
            return false;
        }
        *entry = *value;
        status = Keylist_Data_Add(pObject->Records, count, entry);
    }

    return status;
}

/**
 * For a given object instance-number, clears to entities list.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if entities list is clear successfully.
 */
bool Audit_Log_Records_Delete_All(uint32_t object_instance)
{
    struct object_data *pObject;
    pObject = Object_Data(object_instance);
    if (!pObject) {
        return false;
    }
    Audit_Log_Records_Clean(pObject->Records);

    return true;
}

/**
 * For a given object instance-number, returns the entities list length.
 *
 * This read-only property shall represent the number of log records currently
 * resident in the log buffer. Unlike other log object types, this property
 * is not writable as audit logs are never expected to be reset.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  size of entities list.
 */
int Audit_Log_Records_Count(uint32_t object_instance)
{
    struct object_data *pObject;
    pObject = Object_Data(object_instance);
    if (!pObject) {
        return 0;
    }

    return Keylist_Count(pObject->Records);
}

/**
 * @brief Get the log record maximum length for this object instance
 * @param  object_instance - object-instance number of the object
 * @return  maximum number of log records
 */
int Audit_Log_Records_Count_Max(uint32_t object_instance)
{
    struct object_data *pObject;
    pObject = Object_Data(object_instance);
    if (!pObject) {
        return 0;
    }

    return pObject->Record_Count_Max;
}

/**
 * @brief Set the log record maximum length for this object instance
 * @param  object_instance - object-instance number of the object
 * @param  max_records - maximum number of log records
 * @return true if the maximum number of log records is set
 */
bool Audit_Log_Records_Count_Max_Set(uint32_t object_instance, int max_records)
{
    struct object_data *pObject;
    pObject = Object_Data(object_instance);
    if (!pObject) {
        return false;
    }
    pObject->Record_Count_Max = max_records;

    return true;
}

/**
 * @brief Get the log record maximum length for this object instance
 *
 * This property, of type Unsigned64, shall represent the total number
 * of log records collected by the Audit Log object since creation.
 * When the value of Total_Record_Count reaches its maximum possible
 * value of 2^64 - 1, the next value it takes shall be one.
 * Once this value has wrapped to one, its semantic value (the total
 * number of log records collected) has been lost.
 *
 * @param  object_instance - object-instance number of the object
 * @return  maximum number of log records
 */
int Audit_Log_Records_Count_Total(uint32_t object_instance)
{
    struct object_data *pObject;
    pObject = Object_Data(object_instance);
    if (!pObject) {
        return 0;
    }

    return pObject->Record_Count_Total;
}

/**
 * @brief Set the log record maximum length for this object instance
 * @param  object_instance - object-instance number of the object
 * @param  max_records - maximum number of log records
 * @return true if the maximum number of log records is set
 */
bool Audit_Log_Records_Count_Total_Set(
    uint32_t object_instance, int total_records)
{
    struct object_data *pObject;
    pObject = Object_Data(object_instance);
    if (!pObject) {
        return false;
    }
    pObject->Record_Count_Total = total_records;

    return true;
}

/**
 * @brief Encode a Audit Log entity list complex data type
 *
 * @param object_instance - object-instance number of the object
 * @param apdu - the APDU buffer
 * @param apdu_size - size of the apdu buffer.
 *
 * @return bytes encoded or zero on error.
 */
static int
Audit_Log_Records_Encode(uint32_t object_instance, uint8_t *apdu, int max_apdu)
{
    BACNET_AUDIT_LOG_RECORD *entry = NULL;
    int apdu_len = 0;
    unsigned index = 0;
    unsigned size = 0;

    size = Audit_Log_Records_Count(object_instance);
    for (index = 0; index < size; index++) {
        entry = Audit_Log_Records_Get(object_instance, index);
        apdu_len += bacnet_audit_log_record_encode(NULL, entry);
    }
    if (apdu_len > max_apdu) {
        return BACNET_STATUS_ABORT;
    }
    apdu_len = 0;
    for (index = 0; index < size; index++) {
        entry = Audit_Log_Records_Get(object_instance, index);
        apdu_len += bacnet_audit_log_record_encode(&apdu[apdu_len], entry);
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
bool Audit_Log_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[32] = "AUDIT-LOG-4194303";

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "AUDIT-LOG-%lu",
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
bool Audit_Log_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
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
const char *Audit_Log_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
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
const char *Audit_Log_Description(uint32_t object_instance)
{
    const char *name = NULL;
    const struct object_data *pObject;

    pObject = Object_Data(object_instance);
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
bool Audit_Log_Description_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = true;
        pObject->Description = new_name;
    }

    return status;
}

/**
 * @brief Determines a object enabled flag state
 *
 * @note Logging occurs if and only if Enable is TRUE.
 * Log_Buffer records of type log-status are recorded
 * without regard to the value of the Enable property.
 *
 * @param object_instance - object-instance number of the object
 * @return  enabled status flag
 */
bool Audit_Log_Enable(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Enable;
    }

    return value;
}

/**
 * @brief Apply the log enabled algormithm
 * @param pObject - object data
 * @param enable log enable flag
 * @return true if the log enable flag is applied
 */
static bool Audit_Log_Enable_Apply(struct object_data *pObject, bool enable)
{
    bool status = false;
    struct object_data *pObject;
    bool previous_enable;

    if (pObject) {
        /* Section 12.25.5 can't enable a full log with stop when full
         * set */
        if ((pObject->Enable == false) &&
            (!Audit_Log_Records_Available(pObject) && (enable == true))) {
            status = false;
        } else if (pObject->Enable != enable) {
            /* Only trigger this validation on a potential change of state */
            previous_enable = pObject->Enable;
            pObject->Enable = enable;
            /* To do: what actions do we need to take on writing ? */
            if (enable == false) {
                if (previous_enable == true) {
                    /* Only insert record if we really were
                        enabled i.e. times and enable flags */
                    Audit_Log_Record_Status_Insert(
                        object_instance, LOG_STATUS_LOG_DISABLED, true);
                }
            } else {
                if (previous_enable == false) {
                    /* Have really gone from disabled to enabled as
                     * enable flag and times were correct */
                    Audit_Log_Record_Status_Insert(
                        object_instance, LOG_STATUS_LOG_DISABLED, false);
                }
            }
            pObject->Enable = enable;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Determines a object enabled flag state
 * @param object_instance - object-instance number of the object
 * @return  enabled status flag
 */
bool Audit_Log_Enable_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    status = Audit_Log_Enable_Apply(pObject, value);

    return status;
}

/**
 * @brief For a given object instance-number, sets the object enabled flag
 * @param object_instance - object-instance number of the object
 * @param enable - holds the value to be set
 * @param error_class - BACnet error class
 * @param error_code - BACnet error code
 * @return true if set
 */
static bool Audit_Log_Enable_Write(
    uint32_t object_instance,
    bool enable,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    bool previous_enable;

    pObject = Object_Data(object_instance);
    status = Audit_Log_Enable_Apply(pObject, enable);
    if (!status) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_LOG_BUFFER_FULL;
    }

    return status;
}

/**
 * @brief Determines a object out-of-service flag state
 * @param object_instance - object-instance number of the object
 * @return out-of-service status flag
 */
bool Audit_Log_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the object out-of-service
 * flag
 * @param object_instance - object-instance number of the object
 * @param value - holds the value to be set
 * @return true if set
 */
bool Audit_Log_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Out_Of_Service = value;
        status = true;
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
int Audit_Log_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
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
            Audit_Log_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], rpdata->object_type);
            break;
        case PROP_ENABLE:
            apdu_len = encode_application_boolean(
                &apdu[0], Audit_Log_Enable(rpdata->object_instance));
            break;

        case PROP_BUFFER_SIZE:
            apdu_len = encode_application_unsigned(
                &apdu[0], Audit_Log_Records_Count_Max(rpdata->object_instance));
            break;

        case PROP_LOG_BUFFER:
            /* You can only read the buffer via the ReadRange service */
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_READ_ACCESS_DENIED;
            apdu_len = BACNET_STATUS_ERROR;
            break;

        case PROP_RECORD_COUNT:
            apdu_len = encode_application_unsigned(
                &apdu[0], Audit_Log_Records_Count(rpdata->object_instance));
            break;

        case PROP_TOTAL_RECORD_COUNT:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Audit_Log_Records_Count_Total(rpdata->object_instance));
            break;

        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;

        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                Audit_Log_Out_Of_Service(rpdata->object_instance));
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (!BACnetARRAY_Property(rpdata->object_property)) &&
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
bool Audit_Log_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
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
    if ((!BACnetARRAY_Property(wp_data->object_property)) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                status = Audit_Log_Enable_Write(
                    wp_data->object_instance, value.type.Boolean,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;

        case PROP_BUFFER_SIZE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Audit_Log_Buffer_Size_Write(
                    wp_data->object_instance, value.type.Unsigned_Int,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;

        case PROP_RECORD_COUNT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Audit_Log_Record_Count_Write(
                    wp_data->object_instance, value.type.Unsigned_Int,
                    &wp_data->error_class, &wp_data->error_code);
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
 * Inserts a status notification into a audit log
 *
 * @param  instance - object-instance number of the object
 * @param  log_status - log status
 * @param  state - notificate state
 */
void Audit_Log_Record_Status_Insert(
    uint32_t object_instance, BACNET_LOG_STATUS log_status, bool state)
{
    BACNET_AUDIT_LOG_RECORD record;
    struct object_data *pObject;
    bool effective_enable;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return;
    }
    record.time_stamp = Audit_Log_Epoch_Seconds_Now();
    record.tag = AUDIT_LOG_DATUM_TAG_STATUS;
    record.datum.log_status = 0;
    /* Note we set the bits in correct order so that we can place them directly
     * into the bitstring structure later on when we have to encode them */
    switch (log_status) {
        case LOG_STATUS_LOG_DISABLED:
            if (state) {
                record.datum.log_status = 1 << LOG_STATUS_LOG_DISABLED;
            }
            break;
        case LOG_STATUS_BUFFER_PURGED:
            if (state) {
                record.datum.log_status = 1 << LOG_STATUS_BUFFER_PURGED;
            }
            break;
        case LOG_STATUS_LOG_INTERRUPTED:
            if (state) {
                record.datum.log_status = 1 << LOG_STATUS_LOG_INTERRUPTED;
            }
            break;
        default:
            break;
    }
    Audit_Log_Records_Add(object_instance, &record);

    Logs[log_index][CurrentLog->iIndex++] = record;
    if (CurrentLog->iIndex >= AL_MAX_ENTRIES) {
        CurrentLog->iIndex = 0;
    }

    CurrentLog->ulTotalRecordCount++;

    if (CurrentLog->ulRecordCount < AL_MAX_ENTRIES) {
        CurrentLog->ulRecordCount++;
    }
}

static bool Audit_Log_Notification_Same(AL_NOTIFICATION *a, AL_NOTIFICATION *b)
{
    return (a->operation == b->operation) &&
        bacnet_recipient_same(&a->source_device, &b->source_device) &&
        bacnet_recipient_same(&a->target_device, &b->target_device);
}

static int AL_Rec_Search(int iLog, AL_LOG_REC *rec)
{
    int i;
    AL_LOG_INFO *CurrentLog = &LogInfo[iLog];
    AL_LOG_REC *r;

    for (i = 0; i < CurrentLog->iIndex; i++) {
        r = &Logs[iLog][i];
        if ((r->ucRecType == AL_TYPE_NOTIFICATION) &&
            Audit_Log_Notification_Same(
                &r->Datum.notification, &rec->Datum.notification)) {
            return i;
        }
    }

    return -1;
}

/**
 * Insert a notification record into a audit log.
 *
 * @param  instance - object-instance number of the object
 * @param  notif - notificatione
 */
void AL_Insert_Notification_Rec(uint32_t instance, AL_NOTIFICATION *notif)
{
    AL_LOG_INFO *CurrentLog;
    AL_LOG_REC TempRec;
    int iLog;
    int index;

    iLog = Audit_Log_Instance_To_Index(instance);
    if (iLog == MAX_AUDIT_LOGS) {
        return;
    }
    CurrentLog = &LogInfo[iLog];

    if (!AL_Enable(instance)) {
        return;
    }

    TempRec.tTimeStamp = Audit_Log_Epoch_Seconds_Now();
    TempRec.ucRecType = AL_TYPE_NOTIFICATION;
    TempRec.Datum.notification = *notif;

    index = AL_Rec_Search(iLog, &TempRec);
    if (index >= 0) {
        Logs[iLog][index] = TempRec;
    } else {
        Logs[iLog][CurrentLog->iIndex++] = TempRec;
        if (CurrentLog->iIndex >= AL_MAX_ENTRIES) {
            CurrentLog->iIndex = 0;
        }
    }

    CurrentLog->ulTotalRecordCount++;

    if (CurrentLog->ulRecordCount < AL_MAX_ENTRIES) {
        CurrentLog->ulRecordCount++;
    }
}

/**
 * For a given object instance-number, determines the enable flag
 *
 * @param  instance - object-instance number of the object
 *
 * @return  enable status of the object
 */
bool AL_Enable(uint32_t instance)
{
    int log_index;

    log_index = Audit_Log_Instance_To_Index(instance);
    if (log_index == MAX_AUDIT_LOGS) {
        return false;
    }
    return LogInfo[log_index].bEnable;
}

/**
 * For a given object instance-number, sets the enable status
 *
 * @param  instance - object-instance number of the object
 * @param  bEnable - new status
 */
void AL_Enable_Set(uint32_t instance, bool bEnable)
{
    int log_index;

    log_index = Audit_Log_Instance_To_Index(instance);
    if (log_index < MAX_AUDIT_LOGS) {
        LogInfo[log_index].bEnable = bEnable;
    }
}

/**
 * For a given object instance-number, determines the Out-Of-Service status
 *
 * @param  instance - object-instance number of the object
 *
 * @return Out-Of-Service status of the object
 */
bool AL_Out_Of_Service(uint32_t instance)
{
    int log_index;

    log_index = Audit_Log_Instance_To_Index(instance);
    if (log_index == MAX_AUDIT_LOGS) {
        return false;
    }
    return LogInfo[log_index].out_of_service;
}

/**
 * For a given object instance-number, sets the Out-Of-Service status
 *
 * @param  instance - object-instance number of the object
 * @param  b - new status
 */
void AL_Out_Of_Service_Set(uint32_t instance, bool b)
{
    int log_index;

    log_index = Audit_Log_Instance_To_Index(instance);
    if (log_index < MAX_AUDIT_LOGS) {
        LogInfo[log_index].out_of_service = b;
    }
}

/*****************************************************************************
 * Convert a BACnet time into a local time in seconds since the local epoch  *
 *****************************************************************************/

bacnet_time_t AL_BAC_Time_To_Local(BACNET_DATE_TIME *bdatetime)
{
    return datetime_seconds_since_epoch(bdatetime);
}

/*****************************************************************************
 * Convert a local time in seconds since the local epoch into a BACnet time  *
 *****************************************************************************/

void AL_Local_Time_To_BAC(BACNET_DATE_TIME *bdatetime, bacnet_time_t seconds)
{
    datetime_since_epoch_seconds(bdatetime, seconds);
}

/****************************************************************************
 * Build a list of Audit Log entries from the Log Buffer property as        *
 * required for the ReadsRange functionality.                               *
 *                                                                          *
 * We have to support By Position, By Sequence and By Time requests.        *
 *                                                                          *
 * We do assume the list cannot change whilst we are accessing it so would  *
 * not be multithread safe if there are other tasks that write to the log.  *
 *                                                                          *
 * We take the simple approach here to filling the buffer by taking a max   *
 * size for a single entry and then stopping if there is less than that     *
 * left in the buffer. You could build each entry in a separate buffer and  *
 * determine the exact length before copying but this is time consuming,    *
 * requires more memory and would probably only let you sqeeeze one more    *
 * entry in on occasion. The value is calculated as 10 bytes for the time   *
 * stamp + 27 bytes for our largest data item + 4 for the context tags to   *
 * give 41.                                                                 *
 ****************************************************************************/

#define AL_MAX_ENC 41 /* Maximum size of encoded log entry, see above */

/**
 * For a given read range request, encodes log records
 *
 * @param apdu - buffer to hold the bytes
 * @param pRequest - the read range request
 *
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int AL_log_encode(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest)
{
    /* Initialise result flags to all false */
    bitstring_init(&pRequest->ResultFlags);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, false);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, false);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, false);
    pRequest->ItemCount = 0; /* Start out with nothing */

    /* Bail out now if nowt - should never happen for a Audit Log but ... */
    if (LogInfo[Audit_Log_Instance_To_Index(pRequest->object_instance)]
            .ulRecordCount == 0) {
        return (0);
    }

    if ((pRequest->RequestType == RR_BY_POSITION) ||
        (pRequest->RequestType == RR_READ_ALL)) {
        return (AL_encode_by_position(apdu, pRequest));
    } else if (pRequest->RequestType == RR_BY_SEQUENCE) {
        return (AL_encode_by_sequence(apdu, pRequest));
    }

    return (AL_encode_by_time(apdu, pRequest));
}

/**
 * Handle encoding for the By Position and All options.
 * Does All option by converting to a By Position request starting at index
 * 1 and of maximum log size length.
 *
 * @param apdu - buffer to hold the bytes
 * @param pRequest - the read range request
 *
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int AL_encode_by_position(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest)
{
    int log_index = 0;
    int iLen = 0;
    int32_t iTemp = 0;
    AL_LOG_INFO *CurrentLog = NULL;

    uint32_t uiIndex = 0; /* Current entry number */
    uint32_t uiFirst = 0; /* Entry number we started encoding from */
    uint32_t uiLast = 0; /* Entry number we finished encoding on */
    uint32_t uiTarget = 0; /* Last entry we are required to encode */
    uint32_t uiRemaining = 0; /* Amount of unused space in packet */

    /* See how much space we have */
    uiRemaining = MAX_APDU - pRequest->Overhead;
    log_index = Audit_Log_Instance_To_Index(pRequest->object_instance);
    CurrentLog = &LogInfo[log_index];
    if (pRequest->RequestType == RR_READ_ALL) {
        /*
         * Read all the list or as much as will fit in the buffer by selecting
         * a range that covers the whole list and falling through to the next
         * section of code
         */
        pRequest->Count = CurrentLog->ulRecordCount; /* Full list */
        pRequest->Range.RefIndex = 1; /* Starting at the beginning */
    }

    if (pRequest->Count <
        0) { /* negative count means work from index backwards */
        /*
         * Convert from end index/negative count to
         * start index/positive count and then process as
         * normal. This assumes that the order to return items
         * is always first to last, if this is not true we will
         * have to handle this differently.
         *
         * Note: We need to be careful about how we convert these
         * values due to the mix of signed and unsigned types - don't
         * try to optimise the code unless you understand all the
         * implications of the data type conversions!
         */

        iTemp = pRequest->Range.RefIndex; /* pull out and convert to signed */
        iTemp +=
            pRequest->Count + 1; /* Adjust backwards, remember count is -ve */
        if (iTemp <
            1) { /* if count is too much, return from 1 to start index */
            pRequest->Count = pRequest->Range.RefIndex;
            pRequest->Range.RefIndex = 1;
        } else { /* Otherwise adjust the start index and make count +ve */
            pRequest->Range.RefIndex = iTemp;
            pRequest->Count = -pRequest->Count;
        }
    }

    /* From here on in we only have a starting point and a positive count */

    if (pRequest->Range.RefIndex >
        CurrentLog->ulRecordCount) { /* Nothing to return as we are past the end
                                      of the list */
        return (0);
    }

    uiTarget = pRequest->Range.RefIndex + pRequest->Count -
        1; /* Index of last required entry */
    if (uiTarget >
        CurrentLog->ulRecordCount) { /* Capped at end of list if necessary */
        uiTarget = CurrentLog->ulRecordCount;
    }

    uiIndex = pRequest->Range.RefIndex;
    uiFirst = uiIndex; /* Record where we started from */
    while (uiIndex <= uiTarget) {
        if (uiRemaining < AL_MAX_ENC) {
            /*
             * Can't fit any more in! We just set the result flag to say there
             * was more and drop out of the loop early
             */
            bitstring_set_bit(
                &pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, true);
            break;
        }

        iTemp = AL_encode_entry(&apdu[iLen], log_index, uiIndex);

        uiRemaining -= iTemp; /* Reduce the remaining space */
        iLen += iTemp; /* and increase the length consumed */
        uiLast = uiIndex; /* Record the last entry encoded */
        uiIndex++; /* and get ready for next one */
        pRequest->ItemCount++; /* Chalk up another one for the response count */
    }

    /* Set remaining result flags if necessary */
    if (uiFirst == 1) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, true);
    }

    if (uiLast == CurrentLog->ulRecordCount) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);
    }

    return (iLen);
}

/**
 * Handle encoding for the By Sequence option.
 * The fact that the buffer always has at least a single entry is used
 * implicetly in the following as we don't have to handle the case of an
 * empty buffer.
 *
 * @param apdu - buffer to hold the bytes
 * @param pRequest - the read range request
 *
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int AL_encode_by_sequence(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest)
{
    int log_index = 0;
    int iLen = 0;
    int32_t iTemp = 0;
    AL_LOG_INFO *CurrentLog = NULL;

    uint32_t uiIndex = 0; /* Current entry number */
    uint32_t uiFirst = 0; /* Entry number we started encoding from */
    uint32_t uiLast = 0; /* Entry number we finished encoding on */
    uint32_t uiSequence = 0; /* Tracking sequenc number when encoding */
    uint32_t uiRemaining = 0; /* Amount of unused space in packet */
    uint32_t uiFirstSeq = 0; /* Sequence number for 1st record in log */

    uint32_t uiBegin = 0; /* Starting Sequence number for request */
    uint32_t uiEnd = 0; /* Ending Sequence number for request */
    bool bWrapReq =
        false; /* Has request sequence range spanned the max for uint32_t? */
    bool bWrapLog =
        false; /* Has log sequence range spanned the max for uint32_t? */

    /* See how much space we have */
    uiRemaining = MAX_APDU - pRequest->Overhead;
    log_index = Audit_Log_Instance_To_Index(pRequest->object_instance);
    CurrentLog = &LogInfo[log_index];
    /* Figure out the sequence number for the first record, last is
     * ulTotalRecordCount */
    uiFirstSeq =
        CurrentLog->ulTotalRecordCount - (CurrentLog->ulRecordCount - 1);

    /* Calculate start and end sequence numbers from request */
    if (pRequest->Count < 0) {
        uiBegin = pRequest->Range.RefSeqNum + pRequest->Count + 1;
        uiEnd = pRequest->Range.RefSeqNum;
    } else {
        uiBegin = pRequest->Range.RefSeqNum;
        uiEnd = pRequest->Range.RefSeqNum + pRequest->Count - 1;
    }
    /* See if we have any wrap around situations */
    if (uiBegin > uiEnd) {
        bWrapReq = true;
    }
    if (uiFirstSeq > CurrentLog->ulTotalRecordCount) {
        bWrapLog = true;
    }

    if ((bWrapReq == false) && (bWrapLog == false)) { /* Simple case no wraps */
        /* If no overlap between request range and buffer contents bail out */
        if ((uiEnd < uiFirstSeq) ||
            (uiBegin > CurrentLog->ulTotalRecordCount)) {
            return (0);
        }

        /* Truncate range if necessary so it is guaranteed to lie
         * between the first and last sequence numbers in the buffer
         * inclusive.
         */
        if (uiBegin < uiFirstSeq) {
            uiBegin = uiFirstSeq;
        }

        if (uiEnd > CurrentLog->ulTotalRecordCount) {
            uiEnd = CurrentLog->ulTotalRecordCount;
        }
    } else { /* There are wrap arounds to contend with */
        /* First check for non overlap condition as it is common to all */
        if ((uiBegin > CurrentLog->ulTotalRecordCount) &&
            (uiEnd < uiFirstSeq)) {
            return (0);
        }

        if (bWrapLog == false) { /* Only request range wraps */
            if (uiEnd < uiFirstSeq) {
                uiEnd = CurrentLog->ulTotalRecordCount;
                if (uiBegin < uiFirstSeq) {
                    uiBegin = uiFirstSeq;
                }
            } else {
                uiBegin = uiFirstSeq;
                if (uiEnd > CurrentLog->ulTotalRecordCount) {
                    uiEnd = CurrentLog->ulTotalRecordCount;
                }
            }
        } else if (bWrapReq == false) { /* Only log wraps */
            if (uiBegin > CurrentLog->ulTotalRecordCount) {
                if (uiBegin > uiFirstSeq) {
                    uiBegin = uiFirstSeq;
                }
            } else {
                if (uiEnd > CurrentLog->ulTotalRecordCount) {
                    uiEnd = CurrentLog->ulTotalRecordCount;
                }
            }
        } else { /* Both wrap */
            if (uiBegin < uiFirstSeq) {
                uiBegin = uiFirstSeq;
            }

            if (uiEnd > CurrentLog->ulTotalRecordCount) {
                uiEnd = CurrentLog->ulTotalRecordCount;
            }
        }
    }

    /* We now have a range that lies completely within the log buffer
     * and we need to figure out where that starts in the buffer.
     */
    uiIndex = uiBegin - uiFirstSeq + 1;
    uiSequence = uiBegin;
    uiFirst = uiIndex; /* Record where we started from */
    while (uiSequence != uiEnd + 1) {
        if (uiRemaining < AL_MAX_ENC) {
            /*
             * Can't fit any more in! We just set the result flag to say there
             * was more and drop out of the loop early
             */
            bitstring_set_bit(
                &pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, true);
            break;
        }

        iTemp = AL_encode_entry(&apdu[iLen], log_index, uiIndex);

        uiRemaining -= iTemp; /* Reduce the remaining space */
        iLen += iTemp; /* and increase the length consumed */
        uiLast = uiIndex; /* Record the last entry encoded */
        uiIndex++; /* and get ready for next one */
        uiSequence++;
        pRequest->ItemCount++; /* Chalk up another one for the response count */
    }

    /* Set remaining result flags if necessary */
    if (uiFirst == 1) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, true);
    }

    if (uiLast == CurrentLog->ulRecordCount) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);
    }

    pRequest->FirstSequence = uiBegin;

    return (iLen);
}

/**
 * Handle encoding for the By Time option.
 * The fact that the buffer always has at least a single entry is used
 * implicetly in the following as we don't have to handle the case of an
 * empty buffer.
 *
 * @param apdu - buffer to hold the bytes
 * @param pRequest - the read range request
 *
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int AL_encode_by_time(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest)
{
    int log_index = 0;
    int iLen = 0;
    int32_t iTemp = 0;
    int iCount = 0;
    AL_LOG_INFO *CurrentLog = NULL;

    uint32_t uiIndex = 0; /* Current entry number */
    uint32_t uiFirst = 0; /* Entry number we started encoding from */
    uint32_t uiLast = 0; /* Entry number we finished encoding on */
    uint32_t uiRemaining = 0; /* Amount of unused space in packet */
    uint32_t uiFirstSeq = 0; /* Sequence number for 1st record in log */
    bacnet_time_t tRefTime = 0; /* The time from the request in local format */

    /* See how much space we have */
    uiRemaining = MAX_APDU - pRequest->Overhead;
    log_index = Audit_Log_Instance_To_Index(pRequest->object_instance);
    CurrentLog = &LogInfo[log_index];

    tRefTime = AL_BAC_Time_To_Local(&pRequest->Range.RefTime);
    /* Find correct position for oldest entry in log */
    if (CurrentLog->ulRecordCount < AL_MAX_ENTRIES) {
        uiIndex = 0;
    } else {
        uiIndex = CurrentLog->iIndex;
    }

    if (pRequest->Count < 0) {
        /* Start at end of log and look for record which has
         * timestamp greater than or equal to the reference.
         */
        iCount = CurrentLog->ulRecordCount - 1;
        /* Start out with the sequence number for the last record */
        uiFirstSeq = CurrentLog->ulTotalRecordCount;
        for (;;) {
            if (Logs[pRequest->object_instance]
                    [(uiIndex + iCount) % AL_MAX_ENTRIES]
                        .tTimeStamp < tRefTime) {
                break;
            }

            uiFirstSeq--;
            iCount--;
            if (iCount < 0) {
                return (0);
            }
        }

        /* We have an and point for our request,
         * now work backwards to find where we should start from
         */

        pRequest->Count = -pRequest->Count; /* Conveert to +ve count */
        /* If count would bring us back beyond the limits
         * Of the buffer then pin it to the start of the buffer
         * otherwise adjust starting point and sequence number
         * appropriately.
         */
        iTemp = pRequest->Count - 1;
        if (iTemp > iCount) {
            uiFirstSeq -= iCount;
            pRequest->Count = iCount + 1;
            iCount = 0;
        } else {
            uiFirstSeq -= iTemp;
            iCount -= iTemp;
        }
    } else {
        /* Start at beginning of log and look for 1st record which has
         * timestamp greater than the reference time.
         */
        iCount = 0;
        /* Figure out the sequence number for the first record, last is
         * ulTotalRecordCount */
        uiFirstSeq =
            CurrentLog->ulTotalRecordCount - (CurrentLog->ulRecordCount - 1);
        for (;;) {
            if (Logs[pRequest->object_instance]
                    [(uiIndex + iCount) % AL_MAX_ENTRIES]
                        .tTimeStamp > tRefTime) {
                break;
            }

            uiFirstSeq++;
            iCount++;
            if ((uint32_t)iCount == CurrentLog->ulRecordCount) {
                return (0);
            }
        }
    }

    /* We now have a starting point for the operation and a +ve count */

    uiIndex = iCount + 1; /* Convert to BACnet 1 based reference */
    uiFirst = uiIndex; /* Record where we started from */
    iCount = pRequest->Count;
    while (iCount != 0) {
        if (uiRemaining < AL_MAX_ENC) {
            /*
             * Can't fit any more in! We just set the result flag to say there
             * was more and drop out of the loop early
             */
            bitstring_set_bit(
                &pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, true);
            break;
        }

        iTemp = AL_encode_entry(&apdu[iLen], log_index, uiIndex);

        uiRemaining -= iTemp; /* Reduce the remaining space */
        iLen += iTemp; /* and increase the length consumed */
        uiLast = uiIndex; /* Record the last entry encoded */
        uiIndex++; /* and get ready for next one */
        pRequest->ItemCount++; /* Chalk up another one for the response count */
        iCount--; /* And finally cross another one off the requested count */

        if (uiIndex >
            CurrentLog
                ->ulRecordCount) { /* Finish up if we hit the end of the log */
            break;
        }
    }

    /* Set remaining result flags if necessary */
    if (uiFirst == 1) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, true);
    }

    if (uiLast == CurrentLog->ulRecordCount) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);
    }

    pRequest->FirstSequence = uiFirstSeq;

    return (iLen);
}

/**
 * Encodes a log record
 *
 * @param apdu - Pointer to the buffer for encoding.
 * @param iLog - log index
 * @param iEntry - record index
 *
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int AL_encode_entry(uint8_t *apdu, int iLog, int iEntry)
{
    int iLen = 0;
    AL_LOG_REC *pSource = NULL;
    BACNET_BIT_STRING TempBits;
    uint8_t ucCount = 0;
    BACNET_DATE_TIME TempTime;

    /* Convert from BACnet 1 based to 0 based array index and then
     * handle wrap around of the circular buffer */

    pSource = &Logs[iLog][(iEntry - 1) % AL_MAX_ENTRIES];

    iLen = 0;
    /* First stick the time stamp in with tag [0] */
    AL_Local_Time_To_BAC(&TempTime, pSource->tTimeStamp);
    iLen += bacapp_encode_context_datetime(apdu, 0, &TempTime);

    /* Next comes the actual entry with tag [1] */
    iLen += encode_opening_tag(&apdu[iLen], 1);
    /* The data entry is tagged individually [0] - [10] to indicate which type
     */
    switch (pSource->ucRecType) {
        case AL_TYPE_STATUS:
            /* Build bit string directly from the stored octet */
            bitstring_init(&TempBits);
            bitstring_set_bits_used(&TempBits, 1, 5);
            bitstring_set_octet(&TempBits, 0, pSource->Datum.ucLogStatus);
            iLen += encode_context_bitstring(
                &apdu[iLen], pSource->ucRecType, &TempBits);
            break;

        case AL_TYPE_NOTIFICATION:
            iLen += encode_opening_tag(&apdu[iLen], pSource->ucRecType);

            /* Now we support tags 2, 4, 10 only */
            iLen += bacnet_recipient_context_encode(
                &apdu[iLen], 2, &pSource->Datum.notification.source_device);
            iLen += encode_context_unsigned(
                &apdu[iLen], 4, pSource->Datum.notification.operation);
            iLen += bacnet_recipient_context_encode(
                &apdu[iLen], 10, &pSource->Datum.notification.target_device);

            iLen += encode_closing_tag(&apdu[iLen], pSource->ucRecType);
            break;

        case AL_TYPE_TIME_CHANGE:
            iLen += encode_context_real(
                &apdu[iLen], pSource->ucRecType, pSource->Datum.time_change);
            break;

        default:
            /* Should never happen as we don't support this at the moment */
            break;
    }

    iLen += encode_closing_tag(&apdu[iLen], 1);

    return (iLen);
}

/**
 * @brief Creates a Audit Log object
 * @param object_instance - object-instance number of the object
 * @return object_instance if the object is created, else BACNET_MAX_INSTANCE
 */
uint32_t Audit_Log_Create(uint32_t object_instance)
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
        pObject->Records = Keylist_Create();
        pObject->Record_Index = 0;
        pObject->Record_Count_Max = BACNET_AUDIT_LOG_RECORDS_MAX;
        pObject->Enable = false;
        pObject->Out_Of_Service = false;
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
 * @brief Deletes an Audit Log object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Audit_Log_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        Audit_Log_Records_Clean(pObject->Records);
        Keylist_Delete(pObject->Records);
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all the Audit Logs and their data
 */
void Audit_Log_Cleanup(void)
{
    struct object_data *pObject;

    if (Object_List) {
        do {
            pObject = Keylist_Data_Pop(Object_List);
            if (pObject) {
                Audit_Log_Records_Clean(pObject->Records);
                Keylist_Delete(pObject->Records);
                free(pObject);
            }
        } while (pObject);
        Keylist_Delete(Object_List);
        Object_List = NULL;
    }
}

/**
 * @brief Initializes the Audit Log object data
 */
void Audit_Log_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
