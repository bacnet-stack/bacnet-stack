/**
 * @file
 * @brief A basic BACnet Alert Enrollment Object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/event.h"
#include "bacnet/proplist.h"
// /* BACnet Stack Objects */
#include "bacnet/basic/object/nc.h"
// #include "bacnet/basic/object/device.h"
/* me! */
#include "bacnet/basic/object/alert_enrollment.h"
#if BACNET_EVENT_EXTENDED_ENABLED && defined(INTRINSIC_REPORTING)
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_Lists[MAX_NUM_DEVICES];
#ifdef BAC_ROUTING
#define Object_List (Object_Lists[Routed_Device_Object_Index()])
#else
#define Object_List (Object_Lists[0])
#endif
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_ALERT_ENROLLMENT;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,  PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,        PROP_PRESENT_VALUE,
    PROP_EVENT_STATE,        PROP_EVENT_DETECTION_ENABLE,
    PROP_NOTIFICATION_CLASS, PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,  PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,  -1
};

static const int32_t Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_DESCRIPTION,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_EVENT_MESSAGE_TEXTS_CONFIG,
    PROP_EVENT_ALGORITHM_INHIBIT_REF,
    PROP_EVENT_ALGORITHM_INHIBIT,
    PROP_AUDIT_LEVEL,
    PROP_AUDITABLE_OPERATIONS,
    PROP_TAGS,
    PROP_PROFILE_LOCATION,
    PROP_PROFILE_NAME,
    -1
};

static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of always writable properties */
    -1
};

/**
 * Initialize the pointers for the required, the optional and the properitary
 * value properties.
 *
 * @param pRequired - Pointer to the pointer of required values.
 * @param pOptional - Pointer to the pointer of optional values.
 * @param pProprietary - Pointer to the pointer of properitary values.
 */
void Alert_Enrollment_Property_Lists(
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
 * @brief Get the list of writable properties
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Alert_Enrollment_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct alert_enrollment_descr *
Alert_Enrollment_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Determines if a given object instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Alert_Enrollment_Valid_Instance(uint32_t object_instance)
{
    struct alert_enrollment_descr *pObject;

    pObject = Alert_Enrollment_Object(object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of objects
 * @return  Number of objects
 */
unsigned Alert_Enrollment_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance-number for a given 0..(N-1) index
 * of objects where N is Alert_Enrollment_Count().
 * @param  index - 0..(N-1) where N is Alert_Enrollment_Count().
 * @return  object instance-number for the given index
 */
uint32_t Alert_Enrollment_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, (int)index, &key);

    return key;
}

/**
 * @brief For a given object instance-number, determines a 0..(N-1) index
 * of objects where N is Alert_Enrollment_Count().
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number, or >= Alert_Enrollment_Count()
 * if not valid.
 */
unsigned Alert_Enrollment_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * For a given object instance-number, return the name.
 *
 * Note: the object name must be unique within this device
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - object name/string pointer
 *
 * @return  true/false
 */
bool Alert_Enrollment_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text_string[32] = "";
    bool status = false;
    struct alert_enrollment_descr *pObject;

    pObject = Alert_Enrollment_Object(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                text_string, sizeof(text_string), "ALERT ENROLLMENT %lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text_string);
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
bool Alert_Enrollment_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct alert_enrollment_descr *pObject;

    pObject = Alert_Enrollment_Object(object_instance);
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
const char *Alert_Enrollment_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct alert_enrollment_descr *pObject;

    pObject = Alert_Enrollment_Object(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
const char *Alert_Enrollment_Description(uint32_t object_instance)
{
    const char *name = NULL;
    const struct alert_enrollment_descr *pObject;

    pObject = Alert_Enrollment_Object(object_instance);
    if (pObject) {
        name = pObject->Description;
    }

    return name;
}

/**
 * @brief For a given object instance-number, sets the description
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if object-name was set
 */
bool Alert_Enrollment_Description_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct alert_enrollment_descr *pObject;

    pObject = Alert_Enrollment_Object(object_instance);
    if (pObject) {
        pObject->Description = new_name;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, determines the present-value
 * @param  object_instance - object-instance number of the object
 * @return  present-value of the object
 */
BACNET_OBJECT_ID Alert_Enrollment_Present_Value(uint32_t object_instance)
{
    BACNET_OBJECT_ID value = { OBJECT_DEVICE, BACNET_MAX_INSTANCE };
    struct alert_enrollment_descr *pObject;

    pObject = Alert_Enrollment_Object(object_instance);
    if (pObject) {
        value = pObject->Present_Value;
    }

    return value;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACnetObjectIdentifier value
 * @return  true if present-value is set.
 */
void Alert_Enrollment_Present_Value_Set(
    uint32_t object_instance, BACNET_OBJECT_ID value)
{
    struct alert_enrollment_descr *pObject;

    pObject = Alert_Enrollment_Object(object_instance);
    if (pObject) {
        pObject->Present_Value = value;
    }
}

/**
 * For a given object instance-number, returns the notification_class property
 * value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  notification_class property value
 */
uint32_t Alert_Enrollment_Notification_Class(uint32_t object_instance)
{
    uint32_t notification_class = BACNET_MAX_INSTANCE;
    struct alert_enrollment_descr *pObject =
        Alert_Enrollment_Object(object_instance);

    if (pObject) {
        notification_class = pObject->Notification_Class;
    }

    return notification_class;
}

/**
 * For a given object instance-number, sets the notification_class property
 * value
 *
 * @param object_instance - object-instance number of the object
 * @param notification_class - notification_class property value
 *
 * @return true if the notification_class property value was set
 */
bool Alert_Enrollment_Notification_Class_Set(
    uint32_t object_instance, uint32_t notification_class)
{
    bool status = false;
    struct alert_enrollment_descr *pObject =
        Alert_Enrollment_Object(object_instance);

    if (pObject) {
        pObject->Notification_Class = notification_class;
        status = true;
    }

    return status;
}

static int Alert_Enrollment_Event_Time_Stamps_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0, len = 0;
    struct alert_enrollment_descr *pObject;
    BACNET_DATE_TIME never = { { 0xFF, 0xFF, 0xFF, 0xFF },
                               { 0xFF, 0xFF, 0xFF, 0xFF } };

    pObject = Alert_Enrollment_Object(object_instance);
    if (pObject) {
        if (index < MAX_BACNET_EVENT_TRANSITION) {
            len = encode_opening_tag(apdu, TIME_STAMP_DATETIME);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_date(apdu, &never.date);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_time(apdu, &never.time);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, TIME_STAMP_DATETIME);
            apdu_len += len;
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    } else {
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief For a given object instance-number, handles the ReadProperty service
 * @param  rpdata Property requested, see for BACNET_READ_PROPERTY_DATA details.
 * @return apdu len, or BACNET_STATUS_ERROR on error
 */
int Alert_Enrollment_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    uint8_t *apdu = rpdata->application_data;
    int apdu_size = rpdata->application_data_len;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_OBJECT_ID value = { OBJECT_DEVICE, BACNET_MAX_INSTANCE };

    struct alert_enrollment_descr *pObject = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    pObject = Alert_Enrollment_Object(rpdata->object_instance);
    if (!pObject) {
        return BACNET_STATUS_ERROR;
    }

    switch ((int)rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Alert_Enrollment_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            value = Alert_Enrollment_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_object_id(
                &apdu[0], value.type, value.instance);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Alert_Enrollment_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_NOTIFICATION_CLASS:
            apdu_len = encode_application_unsigned(
                &apdu[0], pObject->Notification_Class);
            break;
        case PROP_EVENT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL, true);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT, true);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL, true);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_DETECTION_ENABLE:
            apdu_len = encode_application_boolean(&apdu[0], false);
            break;
        case PROP_ACKED_TRANSITIONS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL, true);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT, true);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL, true);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_NOTIFY_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], NOTIFY_EVENT);
            break;
        case PROP_EVENT_TIME_STAMPS:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Alert_Enrollment_Event_Time_Stamps_Encode,
                MAX_BACNET_EVENT_TRANSITION, apdu, apdu_size);
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
 * @brief WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return false if an error is loaded, true if no errors
 */
bool Alert_Enrollment_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    struct alert_enrollment_descr *pObject;

    /* Valid data? */
    if (wp_data == NULL) {
        return false;
    }
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
    pObject = Alert_Enrollment_Object(wp_data->object_instance);
    if (!pObject) {
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_NOTIFICATION_CLASS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                pObject->Notification_Class = value.type.Unsigned_Int;
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
 * @brief Add an alert to the object's queue of alerts to be sent at the next
 * intrinsic reporting hook
 * @param  object_instance - object-instance number of the object
 * @return true on successful add, false if not added
 */
bool Alert_Enrollment_Queue_Alert(
    uint32_t object_instance, const BACNET_ALERT *alert)
{
    struct alert_enrollment_descr *CurrentInstance =
        Alert_Enrollment_Object(object_instance);
    if (!CurrentInstance) {
        return false;
    }
    CurrentInstance->Present_Value = alert->source;
    return Ringbuf_Put(&CurrentInstance->Alert_Queue, (const uint8_t *)alert);
}

/**
 * @brief Handles alert notifications using the Intrinsic Reporting hook
 * @param  object_instance - object-instance number of the object
 */
void Alert_Enrollment_Intrinsic_Reporting(uint32_t object_instance)
{
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };
    BACNET_ALERT *alert = NULL;
    BACNET_CHARACTER_STRING msgCharString = { 0 };
    struct alert_enrollment_descr *CurrentInstance =
        Alert_Enrollment_Object(object_instance);
    if (!CurrentInstance) {
        return;
    }

    while (
        (alert = (BACNET_ALERT *)Ringbuf_Peek(&CurrentInstance->Alert_Queue))) {
        event_data.eventType = EVENT_EXTENDED;
        event_data.fromState = EVENT_STATE_NORMAL;
        event_data.toState = EVENT_STATE_NORMAL;
        event_data.eventObjectIdentifier.type = Object_Type;
        event_data.eventObjectIdentifier.instance = object_instance;
        event_data.notifyType = NOTIFY_EVENT;
        event_data.notificationClass = CurrentInstance->Notification_Class;
        event_data.timeStamp.tag = TIME_STAMP_DATETIME;
        event_data.timeStamp = alert->timeStamp;
        characterstring_buffer_to_characterstring(
            &msgCharString, &alert->messageText);
        event_data.messageText = &msgCharString;
        event_data.notificationParams.extended.extendedEventType = 0;
        event_data.notificationParams.extended.vendorID = 0;
        event_data.notificationParams.extended.parameters.tag =
            BACNET_APPLICATION_TAG_OBJECT_ID;
        event_data.notificationParams.extended.parameters.type.Object_Id =
            alert->source;

        debug_log_fprintf(
            DEBUG_LOG_DEBUG, stderr,
            "Alert-Enrollment[%d]: Notification Class[%d]-%s "
            "%u/%u/%u-%u:%u:%u.%u!\n",
            object_instance, event_data.notificationClass,
            bactext_event_type_name(event_data.eventType),
            (unsigned)event_data.timeStamp.value.dateTime.date.year,
            (unsigned)event_data.timeStamp.value.dateTime.date.month,
            (unsigned)event_data.timeStamp.value.dateTime.date.day,
            (unsigned)event_data.timeStamp.value.dateTime.time.hour,
            (unsigned)event_data.timeStamp.value.dateTime.time.min,
            (unsigned)event_data.timeStamp.value.dateTime.time.sec,
            (unsigned)event_data.timeStamp.value.dateTime.time.hundredths);
        Notification_Class_common_reporting_function(&event_data);

        characterstring_buffer_free(&alert->messageText);
        (void)Ringbuf_Pop(&CurrentInstance->Alert_Queue, NULL);
    }
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Alert_Enrollment_Context_Get(uint32_t object_instance)
{
    struct alert_enrollment_descr *pObject;

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
void Alert_Enrollment_Context_Set(uint32_t object_instance, void *context)
{
    struct alert_enrollment_descr *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Creates an object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Alert_Enrollment_Create(uint32_t object_instance)
{
    struct alert_enrollment_descr *pObject = NULL;
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
        pObject = calloc(1, sizeof(struct alert_enrollment_descr));
        if (pObject) {
            pObject->Object_Name = NULL;
            pObject->Description = NULL;
            pObject->Present_Value.type = OBJECT_DEVICE;
            pObject->Present_Value.instance = BACNET_MAX_INSTANCE;
            pObject->Notification_Class = BACNET_MAX_INSTANCE;

            index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if (index < 0) {
                free(pObject);
                return BACNET_MAX_INSTANCE;
            }
            Ringbuf_Init(
                &pObject->Alert_Queue, pObject->Alert_Buffer,
                sizeof(BACNET_ALERT), ALERT_ENROLLMENT_ALERT_COUNT);
        } else {
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * @brief Deletes an object
 * @param object_instance - object-instance number of the object
 * @return true if the object-instance was deleted
 */
bool Alert_Enrollment_Delete(uint32_t object_instance)
{
    bool status = false;
    struct alert_enrollment_descr *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all objects and their data
 */
void Alert_Enrollment_Cleanup(void)
{
    struct alert_enrollment_descr *pObject;
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
                if (pObject) {
                    free(pObject);
                }
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
 * @brief Initializes the Analog Input object data
 */
void Alert_Enrollment_Init(void)
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
#endif
