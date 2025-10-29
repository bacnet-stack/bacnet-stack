/**
 * @file
 * @brief Audit Log object, customize for your use
 * @details An Audit Log object combines audit notifications
 *  from operation sources and operation targets and stores the
 *  combined record in an internal buffer for subsequent retrieval.
 *  Each timestamped buffer entry is called an audit log "record."
 *
 *  Each Audit Log object maintains an internal, persistent, optionally
 *  fixed-size log buffer. This log buffer fills or grows as audit
 *  log records are added. If the log buffer becomes full, the least recent
 *  log records are overwritten when new log records are added.
 *  Log buffers are transferred as a list of BACnetAuditLogRecord values
 *  using the ReadRange and AuditLogQuery services. Each log record in the
 *  log buffer has an implied sequence number that is equal to the value
 *  of the Total_Record_Count property immediately after the record is added.
 *  See Clause 19.6 for a full description of how audit notifications are
 *  added to audit logs.
 *
 *  As records are added into the log, the Audit Log object will scan
 *  existing entries for a matching record. A record is a match if:
 *      (a) the record contains the timestamp for the opposite actor (the
 *      record contains the operation source timestamp when merging
 *      in an operation target notification and vice-versa);
 *      (b) the operation-source, operation, invoke-id, target-device,
 *      target-property, are all equal;
 *      (c) if the user-id, user-role, target-value fields are provided
 *      in both notifications then they are equal; and
 *      (d) if the source-timestamp and target-timestamp values are
 *      approximately equal (+/- APDU_Timeout * 2).
 *
 *  If a match is found, the existing log record is updated.
 *  Otherwise, a new record is created. If a match is found,
 *  and it already contains both an operation source and an
 *  operation target portion, then the notification is dropped.
 *  When creating a new record, those fields which are not supplied
 *  in the notification (such as the 'Source Timestamp' when a
 *  server notification is received) shall be absent from the record.
 *  When updating an existing record, those fields not supplied in the
 *  original notification are updated from the new notification, if present.
 *  For the 'Current Value' field, a value provided by the operation target
 *  device shall always take precedence over a value provided by an operation
 *  source device. As such, if the values provided in the peer notifications
 *  differ, the operation target value shall be the one used in the record.
 *
 *  Logging may be enabled and disabled through the Enable property.
 *  Audit Log enabling and disabling is recorded in the audit log buffer.
 *
 *  Unlike other log objects, Audit Log objects do not use the BUFFER_READY
 *  event algorithm.
 *
 *  The acquisition of log records by remote devices has
 *  no effect upon the state of the Audit Log object itself. This allows
 *  completely independent, but properly sequential, access to its log records
 *  by all remote devices. Any remote device can independently update its
 *  log records at any time.
 *
 *  Audit Log objects may optionally support forwarding of audit notifications
 *  to “parent” audit logs. This functionality improves the reliability of the
 *  audit system by allowing intermediaries to buffer audit notifications
 *  in the case where the ultimate audit logger is offline for a short period
 *  of time. It is expected that intermediaries be capable of storing a larger
 *  number of records than devices which report auditable actions. It is also
 *  useful for buffering of audit notifications so they can be sent in bulk to
 *  the parent audit log. When operating in this mode, with the
 *  Delete_On_Forward property set to TRUE, the object is not required
 *  to perform audit notification matching and combining.
 *
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
    int Buffer_Size;
    OS_Keylist Records;
    int Record_Count_Total;
    const char *Object_Name;
    const char *Description;
    void *Context;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;

static const int Properties_Required[] = {
    /* required properties that are supported for this object */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_ENABLE,
    PROP_BUFFER_SIZE,
    PROP_LOG_BUFFER,
    PROP_RECORD_COUNT,
    PROP_TOTAL_RECORD_COUNT,
    -1
};

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
 * For a given object instance-number, returns the Audit Log entity by index.
 *
 * @param  object_instance - object-instance number of the object
 * @param  index - index of entity
 *
 * @return Audit Log entity, or NULL if not found.
 */
BACNET_AUDIT_LOG_RECORD *
Audit_Log_Record_Entry(uint32_t object_instance, uint32_t index)
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
 * @brief Delete a record entry from the log buffer
 * @param  object_instance - object-instance number of the object
 * @param  index - record index of entity
 */
void Audit_Log_Record_Entry_Delete(uint32_t object_instance, uint32_t index)
{
    BACNET_AUDIT_LOG_RECORD *entry = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        entry = Keylist_Data_Delete(pObject->Records, index);
        free(entry);
    }
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
bool Audit_Log_Record_Entry_Add(
    uint32_t object_instance, const BACNET_AUDIT_LOG_RECORD *value)
{
    BACNET_AUDIT_LOG_RECORD *entry = NULL;
    struct object_data *pObject;
    int index;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return false;
    }
    if (Keylist_Count(pObject->Records) >= pObject->Buffer_Size) {
        /* log is full, so delete oldest record before adding a new record */
        entry = Keylist_Data_Delete_By_Index(pObject->Records, 0);
    }
    if (!entry) {
        entry = calloc(1, sizeof(BACNET_AUDIT_LOG_RECORD));
        if (!entry) {
            return false;
        }
    }
    memcpy(entry, value, sizeof(BACNET_AUDIT_LOG_RECORD));
    index =
        Keylist_Data_Add(pObject->Records, pObject->Record_Count_Total, entry);
    if (index < 0) {
        free(entry);
        return false;
    }
    pObject->Record_Count_Total++;

    return true;
}

/**
 * @brief Get the log record maximum length for this object instance
 * @param  object_instance - object-instance number of the object
 * @return  maximum number of log records
 * @note For products that support very large log objects,
 *  the value 2^32-1 is reserved to indicate that the buffer size is
 *  unknown and is constrained solely by currently available resources.
 */
uint32_t Audit_Log_Buffer_Size(uint32_t object_instance)
{
    struct object_data *pObject;
    pObject = Object_Data(object_instance);
    if (!pObject) {
        return 0;
    }

    return pObject->Buffer_Size;
}

/**
 * @brief Set the log record maximum length for this object instance
 * @param  object_instance - object-instance number of the object
 * @param  buffer_size - maximum number of log records
 * @return true if the maximum number of log records is set
 */
bool Audit_Log_Buffer_Size_Set(uint32_t object_instance, uint32_t buffer_size)
{
    struct object_data *pObject;
    BACNET_AUDIT_LOG_RECORD *entry = NULL;
    int i;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return false;
    }
    if (buffer_size > INT_MAX) {
        return false;
    }
    if (buffer_size < pObject->Buffer_Size) {
        /* The disposition of existing log records when Buffer_Size is written
            is a local matter. We can shrink the log buffer. */
        for (i = buffer_size; i < pObject->Buffer_Size; i++) {
            entry = Keylist_Data_Delete_By_Index(pObject->Records, i);
            free(entry);
        }
    }
    pObject->Buffer_Size = buffer_size;

    return true;
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
bool Audit_Log_Enable_Set(uint32_t object_instance, bool enable)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Enable != enable) {
            /* Only trigger this validation on a potential change of state */
            pObject->Enable = enable;
            if (enable == false) {
                Audit_Log_Record_Status_Insert(
                    object_instance, LOG_STATUS_LOG_DISABLED, true);
            } else {
                Audit_Log_Record_Status_Insert(
                    object_instance, LOG_STATUS_LOG_DISABLED, false);
            }
        }
        status = true;
    }

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

    status = Audit_Log_Enable_Set(object_instance, enable);
    if (!status) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_LOG_BUFFER_FULL;
    }

    return status;
}

/**
 * @brief For a given object instance-number, determines the property value
 * @param  object_instance - object-instance number of the object
 * @return the property value
 */
uint32_t Audit_Log_Record_Count(uint32_t object_instance)
{
    uint32_t record_count = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        record_count = Keylist_Count(pObject->Records);
    }

    return record_count;
}

/**
 * @brief For a given object instance-number, determines the property value
 * @param  object_instance - object-instance number of the object
 * @return the property value
 */
uint32_t Audit_Log_Total_Record_Count(uint32_t object_instance)
{
    uint32_t total_count = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        total_count = pObject->Record_Count_Total;
    }

    return total_count;
}

/**
 * @brief For a given object instance-number, writes the property value
 * @details If writable, it may not be written when Enable is TRUE.
 * @param  object_instance - object-instance number of the object
 * @param  buffer_size - holds the value to be set
 * @param  error_class - BACnet error class
 * @param  error_code - BACnet error code
 * @return true if set
 */
bool Audit_Log_Buffer_Size_Write(
    uint32_t object_instance,
    uint32_t buffer_size,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Enable) {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        } else if (buffer_size > INT_MAX) {
            /* keylist library uses 'int' so that is our limit */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        } else {
            status = Audit_Log_Buffer_Size_Set(object_instance, buffer_size);
        }
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
    BACNET_CHARACTER_STRING char_string = { 0 };
    BACNET_BIT_STRING bit_string = { 0 };
    uint8_t *apdu = NULL;
    int apdu_max = 0;

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
                &apdu[0], Audit_Log_Buffer_Size(rpdata->object_instance));
            break;
        case PROP_LOG_BUFFER:
            /* You can only read the buffer via the ReadRange service */
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_READ_ACCESS_DENIED;
            apdu_len = BACNET_STATUS_ERROR;
            break;
        case PROP_RECORD_COUNT:
            apdu_len = encode_application_unsigned(
                &apdu[0], Audit_Log_Record_Count(rpdata->object_instance));
            break;
        case PROP_TOTAL_RECORD_COUNT:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Audit_Log_Total_Record_Count(rpdata->object_instance));
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
            /* OVERRIDDEN The value of this flag shall be Logical FALSE */
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            /* OUT_OF_SERVICE The value of this flag shall be Logical FALSE */
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, Audit_Log_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
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

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return;
    }
    datetime_local(&record.timestamp.date, &record.timestamp.time, NULL, NULL);
    record.tag = AUDIT_LOG_DATUM_TAG_STATUS;
    record.log_datum.log_status = 0;
    /* Note we set the bits in correct order so that we can place them directly
     * into the bitstring structure later on when we have to encode them */
    switch (log_status) {
        case LOG_STATUS_LOG_DISABLED:
            if (state) {
                record.log_datum.log_status = 1 << LOG_STATUS_LOG_DISABLED;
            }
            break;
        case LOG_STATUS_BUFFER_PURGED:
            if (state) {
                record.log_datum.log_status = 1 << LOG_STATUS_BUFFER_PURGED;
            }
            break;
        case LOG_STATUS_LOG_INTERRUPTED:
            if (state) {
                record.log_datum.log_status = 1 << LOG_STATUS_LOG_INTERRUPTED;
            }
            break;
        default:
            break;
    }
    Audit_Log_Record_Entry_Add(object_instance, &record);
}

/**
 * @brief Determines if a given log notification is the same as another
 * @param object_instance - object-instance number of the object
 * @param record - log record
 * @return the index of the found log record, or -1 if not found
 */
static int Audit_Log_Record_Search(
    uint32_t object_instance, BACNET_AUDIT_LOG_RECORD *record)
{
    int i;
    BACNET_AUDIT_LOG_RECORD *entry;
    struct object_data *pObject;

    if (!record) {
        return -1;
    }
    pObject = Object_Data(object_instance);
    if (!pObject) {
        return -1;
    }
    for (i = 0; i < pObject->Buffer_Size; i++) {
        entry = Audit_Log_Record_Entry(object_instance, i);
        if (!entry) {
            break;
        }
        if (entry->tag == record->tag) {
            if (entry->tag == AUDIT_LOG_DATUM_TAG_STATUS) {
                if (entry->log_datum.log_status ==
                    record->log_datum.log_status) {
                    return i;
                }
            } else if (entry->tag == AUDIT_LOG_DATUM_TAG_NOTIFICATION) {
                if (bacnet_audit_log_notification_same(
                        &entry->log_datum.notification,
                        &record->log_datum.notification)) {
                    return i;
                }
            }
        }
    }

    return -1;
}

/**
 * Insert a notification record into a audit log.
 *
 * @param  object_instance - object-instance number of the object
 * @param  notif - notificatione
 */
void Audit_Log_Record_Notification_Insert(
    uint32_t object_instance, BACNET_AUDIT_NOTIFICATION *notification)
{
    BACNET_AUDIT_LOG_RECORD seek_entry = { 0 };
    struct object_data *pObject;
    int index;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return;
    }
    if (!Audit_Log_Enable(object_instance)) {
        return;
    }
    /*  As records are added into the log, the Audit Log object will scan
        existing entries for a matching record. */
    datetime_local(
        &seek_entry.timestamp.date, &seek_entry.timestamp.time, NULL, NULL);
    seek_entry.tag = AUDIT_LOG_DATUM_TAG_NOTIFICATION;
    memcpy(
        &seek_entry.log_datum.notification, notification,
        sizeof(BACNET_AUDIT_NOTIFICATION));
    index = Audit_Log_Record_Search(object_instance, &seek_entry);
    if (index >= 0) {
        /*  If a match is found, the existing record is updated with the new
            time stamp and the record is moved to the end of the list.
            i.e. delete the old entry and add the new entry */
        Audit_Log_Record_Entry_Delete(object_instance, index);
    }
    Audit_Log_Record_Entry_Add(object_instance, &seek_entry);
}

/**
 * For a given read range request, encodes log records
 *
 * @param apdu - buffer to hold the bytes
 * @param pRequest - the read range request
 *
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int Audit_Log_Read_Range(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest)
{
    int apdu_len = 0;

    /* buffer and buffer size is passed in BACNET_READ_RANGE_DATA */
    (void)apdu;
    /* Initialise result flags to all false */
    bitstring_init(&pRequest->ResultFlags);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, false);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, false);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, false);
    pRequest->ItemCount = 0; /* Start out with nothing */
    if ((pRequest->RequestType == RR_BY_POSITION) ||
        (pRequest->RequestType == RR_READ_ALL)) {
        apdu_len = Audit_Log_Read_Range_By_Position(pRequest);
    } else if (pRequest->RequestType == RR_BY_SEQUENCE) {
        apdu_len = Audit_Log_Read_Range_By_Sequence(pRequest);
    } else {
        apdu_len = Audit_Log_Read_Range_By_Time(pRequest);
    }

    return apdu_len;
}

/**
 * @brief Handle encoding for the By Position and All options.
 *  Does All option by converting to a By Position request starting at index
 *  1 and of maximum log size length.
 * @param pRequest - the read range request
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int Audit_Log_Read_Range_By_Position(BACNET_READ_RANGE_DATA *pRequest)
{
    int log_index = 0;
    int apdu_len = 0;
    size_t apdu_size;
    int len;
    uint8_t *apdu;
    uint32_t record_count = 0;
    BACNET_AUDIT_LOG_RECORD *entry = NULL;
    int32_t iTemp = 0;
    uint32_t uiIndex = 0; /* Current entry number */
    uint32_t uiFirst = 0; /* Entry number we started encoding from */
    uint32_t uiLast = 0; /* Entry number we finished encoding on */
    uint32_t uiTarget = 0; /* Last entry we are required to encode */

    record_count = Audit_Log_Record_Count(pRequest->object_instance);
    /* See how much space we have */
    apdu_size = pRequest->application_data_len - pRequest->Overhead;
    if (pRequest->RequestType == RR_READ_ALL) {
        /*
         * Read all the list or as much as will fit in the buffer by selecting
         * a range that covers the whole list and falling through to the next
         * section of code
         */
        pRequest->Count = record_count;
        /* Starting at the beginning */
        pRequest->Range.RefIndex = 1;
    }
    if (pRequest->Count < 0) {
        /*
         * negative count means work from index backwards
         *
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
        /* pull out and convert to signed */
        iTemp = pRequest->Range.RefIndex;
        /* Adjust backwards, remember count is -ve */
        iTemp += pRequest->Count + 1;
        if (iTemp < 1) {
            /* if count is too much, return from 1 to start index */
            pRequest->Count = pRequest->Range.RefIndex;
            pRequest->Range.RefIndex = 1;
        } else {
            /* Otherwise adjust the start index and make count +ve */
            pRequest->Range.RefIndex = iTemp;
            pRequest->Count = -pRequest->Count;
        }
    }
    /* From here on in we only have a starting point and a positive count */
    if (pRequest->Range.RefIndex > record_count) {
        /* Nothing to return as we are past the end of the list */
        return 0;
    }
    /* Index of last required entry */
    uiTarget = pRequest->Range.RefIndex + pRequest->Count - 1;
    if (uiTarget > record_count) {
        /* Capped at end of list if necessary */
        uiTarget = record_count;
    }
    uiIndex = pRequest->Range.RefIndex;
    /* Record where we started from */
    uiFirst = uiIndex;
    apdu = pRequest->application_data;
    while (uiIndex <= uiTarget) {
        log_index = uiIndex - 1;
        entry = Audit_Log_Record_Entry(pRequest->object_instance, log_index);
        len = bacnet_audit_log_record_encode(NULL, entry);
        if (len > (apdu_size - apdu_len)) {
            /*
             * Can't fit any more in! We just set the result flag to say there
             * was more and drop out of the loop early
             */
            bitstring_set_bit(
                &pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, true);
            break;
        }
        len = bacnet_audit_log_record_encode(apdu, entry);
        apdu += len;
        apdu_len += len;
        /* Record the last entry encoded */
        uiLast = uiIndex;
        /* and get ready for next one */
        uiIndex++;
        /* Chalk up another one for the response count */
        pRequest->ItemCount++;
    }
    /* Set remaining result flags if necessary */
    if (uiFirst == 1) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, true);
    }
    if (uiLast == record_count) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);
    }

    return apdu_len;
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
int Audit_Log_Read_Range_By_Sequence(BACNET_READ_RANGE_DATA *pRequest)
{
    int apdu_len = 0;
    size_t apdu_size;
    int len;
    uint8_t *apdu;
    int record_index = 0;
    uint32_t record_count;
    uint32_t total_record_count;
    BACNET_AUDIT_LOG_RECORD *entry = NULL;
    uint32_t uiIndex = 0; /* Current entry number */
    uint32_t uiFirst = 0; /* Entry number we started encoding from */
    uint32_t uiLast = 0; /* Entry number we finished encoding on */
    uint32_t uiSequence = 0; /* Tracking sequence number when encoding */
    uint32_t uiFirstSeq = 0; /* Sequence number for 1st record in log */
    uint32_t uiBegin = 0; /* Starting Sequence number for request */
    uint32_t uiEnd = 0; /* Ending Sequence number for request */
    bool bWrapReq =
        false; /* Has request sequence range spanned the max for uint32_t? */
    bool bWrapLog =
        false; /* Has log sequence range spanned the max for uint32_t? */

    /* See how much space we have */
    apdu = pRequest->application_data;
    apdu_size = pRequest->application_data_len - pRequest->Overhead;
    record_count = Audit_Log_Record_Count(pRequest->object_instance);
    total_record_count =
        Audit_Log_Total_Record_Count(pRequest->object_instance);
    /* Figure out the sequence number for the first record, last is
     * ulTotalRecordCount */
    uiFirstSeq = total_record_count - record_count - 1;
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
    if (uiFirstSeq > total_record_count) {
        bWrapLog = true;
    }

    if ((bWrapReq == false) && (bWrapLog == false)) {
        /* Simple case no wraps */
        /* If no overlap between request range and buffer contents bail out */
        if ((uiEnd < uiFirstSeq) || (uiBegin > total_record_count)) {
            return (0);
        }
        /* Truncate range if necessary so it is guaranteed to lie
         * between the first and last sequence numbers in the buffer
         * inclusive.
         */
        if (uiBegin < uiFirstSeq) {
            uiBegin = uiFirstSeq;
        }

        if (uiEnd > total_record_count) {
            uiEnd = total_record_count;
        }
    } else {
        /* There are wrap arounds to contend with */
        /* First check for non overlap condition as it is common to all */
        if ((uiBegin > total_record_count) && (uiEnd < uiFirstSeq)) {
            return (0);
        }

        if (bWrapLog == false) {
            /* Only request range wraps */
            if (uiEnd < uiFirstSeq) {
                uiEnd = total_record_count;
                if (uiBegin < uiFirstSeq) {
                    uiBegin = uiFirstSeq;
                }
            } else {
                uiBegin = uiFirstSeq;
                if (uiEnd > total_record_count) {
                    uiEnd = total_record_count;
                }
            }
        } else if (bWrapReq == false) {
            /* Only log wraps */
            if (uiBegin > total_record_count) {
                if (uiBegin > uiFirstSeq) {
                    uiBegin = uiFirstSeq;
                }
            } else {
                if (uiEnd > total_record_count) {
                    uiEnd = total_record_count;
                }
            }
        } else { /* Both wrap */
            if (uiBegin < uiFirstSeq) {
                uiBegin = uiFirstSeq;
            }

            if (uiEnd > total_record_count) {
                uiEnd = total_record_count;
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
        record_index = uiIndex - 1;
        entry = Audit_Log_Record_Entry(pRequest->object_instance, record_index);
        len = bacnet_audit_log_record_encode(NULL, entry);
        if (len > (apdu_size - apdu_len)) {
            /*
             * Can't fit any more in! We just set the result flag to say there
             * was more and drop out of the loop early
             */
            bitstring_set_bit(
                &pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, true);
            break;
        }
        len = bacnet_audit_log_record_encode(apdu, entry);
        apdu += len;
        apdu_len += len;
        uiLast = uiIndex; /* Record the last entry encoded */
        uiIndex++; /* and get ready for next one */
        uiSequence++;
        pRequest->ItemCount++; /* Chalk up another one for the response count */
    }

    /* Set remaining result flags if necessary */
    if (uiFirst == 1) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, true);
    }

    if (uiLast == record_count) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);
    }
    pRequest->FirstSequence = uiBegin;

    return apdu_len;
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
int Audit_Log_Read_Range_By_Time(BACNET_READ_RANGE_DATA *pRequest)
{
    int apdu_len = 0;
    size_t apdu_size;
    int len;
    uint8_t *apdu;
    int record_index = 0;
    uint32_t record_count;
    uint32_t total_record_count;
    BACNET_AUDIT_LOG_RECORD *entry = NULL;
    int diff;

    int32_t iTemp = 0;
    int iCount = 0;
    uint32_t uiIndex = 0; /* Current entry number */
    uint32_t uiFirst = 0; /* Entry number we started encoding from */
    uint32_t uiLast = 0; /* Entry number we finished encoding on */
    uint32_t uiFirstSeq = 0; /* Sequence number for 1st record in log */

    /* See how much space we have */
    apdu = pRequest->application_data;
    apdu_size = pRequest->application_data_len - pRequest->Overhead;
    record_count = Audit_Log_Record_Count(pRequest->object_instance);
    total_record_count =
        Audit_Log_Total_Record_Count(pRequest->object_instance);
    if (pRequest->Count < 0) {
        /* Start at end of log and look for record which has
         * timestamp greater than or equal to the reference.
         */
        iCount = record_count - 1;
        /* Start out with the sequence number for the last record */
        uiFirstSeq = total_record_count;
        for (;;) {
            record_index = iCount;
            entry =
                Audit_Log_Record_Entry(pRequest->object_instance, record_index);
            diff =
                datetime_compare(&entry->timestamp, &pRequest->Range.RefTime);
            if (diff < 0) {
                /* If datetime1 is before datetime2, returns negative.*/
                break;
            }
            uiFirstSeq--;
            if (iCount) {
                iCount--;
            } else {
                /* end of records, not found */
                return 0;
            }
        }
        /* We have an and point for our request,
         * now work backwards to find where we should start from
         */
        pRequest->Count = -pRequest->Count; /* Convert to +ve count */
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
        uiFirstSeq = total_record_count - record_count - 1;
        for (;;) {
            record_index = iCount;
            entry =
                Audit_Log_Record_Entry(pRequest->object_instance, record_index);
            diff =
                datetime_compare(&entry->timestamp, &pRequest->Range.RefTime);
            if (diff > 0) {
                /* If datetime1 is after datetime2, returns positive.*/
                break;
            }
            uiFirstSeq++;
            iCount++;
            if ((uint32_t)iCount == record_count) {
                return (0);
            }
        }
    }

    /* We now have a starting point for the operation and a +ve count */
    uiIndex = iCount + 1; /* Convert to BACnet 1 based reference */
    uiFirst = uiIndex; /* Record where we started from */
    iCount = pRequest->Count;
    while (iCount != 0) {
        record_index = uiIndex - 1;
        entry = Audit_Log_Record_Entry(pRequest->object_instance, record_index);
        len = bacnet_audit_log_record_encode(NULL, entry);
        if (len > (apdu_size - apdu_len)) {
            /*
             * Can't fit any more in! We just set the result flag to say there
             * was more and drop out of the loop early
             */
            bitstring_set_bit(
                &pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, true);
            break;
        }
        len = bacnet_audit_log_record_encode(apdu, entry);
        apdu += len;
        apdu_len += len;
        uiLast = uiIndex; /* Record the last entry encoded */
        uiIndex++; /* and get ready for next one */
        pRequest->ItemCount++; /* Chalk up another one for the response count */
        iCount--; /* And finally cross another one off the requested count */

        if (uiIndex > record_count) {
            /* Finish up if we hit the end of the log */
            break;
        }
    }
    /* Set remaining result flags if necessary */
    if (uiFirst == 1) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, true);
    }
    if (uiLast == record_count) {
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);
    }
    pRequest->FirstSequence = uiFirstSeq;

    return apdu_len;
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Audit_Log_Context_Get(uint32_t object_instance)
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
void Audit_Log_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Context = context;
    }
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
        pObject->Records = Keylist_Create();
        pObject->Buffer_Size = BACNET_AUDIT_LOG_RECORDS_MAX;
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
 * @brief Deletes all the Audit Logs and their data
 */
static void Audit_Log_Records_Cleanup(OS_Keylist list)
{
    BACNET_AUDIT_LOG_RECORD *entry;

    while (Keylist_Count(list) > 0) {
        entry = Keylist_Data_Pop(list);
        free(entry);
    }
    Keylist_Delete(list);
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
        Audit_Log_Records_Cleanup(pObject->Records);
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
                Audit_Log_Records_Cleanup(pObject->Records);
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
