/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @brief A basic BACnet Schedule object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bactext.h"
#include "bacnet/proplist.h"
#include "bacnet/timestamp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/compare.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/object/device.h" /* me */
#include "bacnet/basic/object/schedule.h"

#define UNUSED(v) (void)(v)

/* per-day dynamic Time-Value storage used by the Weekly_Schedule property.
   The Weekly_Schedule outer BACnetARRAY[7] stays fixed size per the
   standard, but each day's list of Time-Values is stored in a resizable
   OS_Keylist so it can be written and resized like other list properties. */
struct daily_schedule_data {
    OS_Keylist Time_Values; /* keyed 0..N-1, data is BACNET_TIME_VALUE* */
};

/* per-entry dynamic Time-Value storage used by the Exception_Schedule
   property, analogous to struct daily_schedule_data used by
   Weekly_Schedule. The Exception_Schedule outer BACnetARRAY is a resizable
   OS_Keylist of these entries, each of which owns its own resizable
   list-of-time-values. */
struct special_event_data {
    BACNET_SPECIAL_EVENT_PERIOD_TAG periodTag;
    union {
        BACNET_CALENDAR_ENTRY calendarEntry;
        BACNET_OBJECT_ID calendarReference;
    } period;
    OS_Keylist Time_Values; /* keyed 0..N-1, data is BACNET_TIME_VALUE* */
    uint8_t priority;
};

struct object_data {
    BACNET_CHARACTER_CSTRING Object_Name;
    BACNET_CHARACTER_CSTRING Description;
    /* Effective Period: Start and End Date */
    BACNET_DATE Start_Date;
    BACNET_DATE End_Date;
    /* Properties concerning Present Value */
    struct daily_schedule_data Weekly_Schedule[BACNET_WEEKLY_SCHEDULE_SIZE];
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    /* keyed 0..N-1, data is struct special_event_data* */
    OS_Keylist Exception_Schedule;
#endif
    BACNET_APPLICATION_DATA_VALUE Schedule_Default;
    /*
     * Caution: This is a converted to BACNET_PRIMITIVE_APPLICATION_DATA_VALUE.
     * Only some data types may be used!
     *
     * Must be set to a valid value. Default is Schedule_Default.
     */
    BACNET_APPLICATION_DATA_VALUE Present_Value;
    /* keyed 0..N-1, data is BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE* */
    OS_Keylist Object_Property_References;
    uint8_t Priority_For_Writing; /* (1..16) */
    bool Out_Of_Service;
    bool Write_Every_Scheduled_Action;
    /* identity of the currently active scheduled time-value, used to detect
       when a new scheduled action begins (see Schedule_Present_Value_Notify) */
    const BACNET_TIME_VALUE *Active_Time_Value;
    /* re-entrancy guard while writing List_Of_Object_Property_References */
    bool Writeback_Active;
};

/* Key List for storing the object data sorted by instance number */
static OS_Keylist Object_Lists[MAX_NUM_DEVICES];
#ifdef BAC_ROUTING
#define Object_List (Object_Lists[Routed_Device_Object_Index()])
#else
#define Object_List (Object_Lists[0])
#endif

static const int32_t Schedule_Properties_Required[] = {
    /* list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_EFFECTIVE_PERIOD,
    PROP_SCHEDULE_DEFAULT,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
    PROP_PRIORITY_FOR_WRITING,
    PROP_STATUS_FLAGS,
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,
    -1
};

static const int32_t Schedule_Properties_Optional[] = {
    /* list of optional properties */
    PROP_DESCRIPTION, PROP_WEEKLY_SCHEDULE,
#if (BACNET_PROTOCOL_REVISION >= 24)
    PROP_WRITE_EVERY_SCHEDULED_ACTION,
#endif
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    PROP_EXCEPTION_SCHEDULE,
#endif
    -1
};

static const int32_t Schedule_Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of always writable properties */
    PROP_OBJECT_NAME,
    PROP_DESCRIPTION,
    PROP_OUT_OF_SERVICE,
    PROP_WEEKLY_SCHEDULE,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
    PROP_EFFECTIVE_PERIOD,
#if (BACNET_PROTOCOL_REVISION >= 24)
    PROP_WRITE_EVERY_SCHEDULED_ACTION,
#endif
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    PROP_EXCEPTION_SCHEDULE,
#endif
    -1
};

/* registered by the Device object so List_Of_Object_Property_References
   members can be written without a build dependency on those objects */
static write_property_function Write_Property_Internal_Callback;

/**
 * Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 *
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void Schedule_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
{
    if (pRequired) {
        *pRequired = Schedule_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Schedule_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Schedule_Properties_Proprietary;
    }
}

/**
 * @brief Get the list of writable properties for a Schedule object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Schedule_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * @brief Gets an object from the list using an instance number
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct object_data *Object_Data(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Invalidates the cached active Time-Value pointer before any list
 *  mutation can free the underlying node.
 * @param pObject - object whose active action state needs invalidation
 */
static void Schedule_Invalidate_Active_Time_Value(struct object_data *pObject)
{
    if (pObject) {
        pObject->Active_Time_Value = NULL;
    }
}

/**
 * @brief Empty all the Time-Values from a single day of Weekly_Schedule,
 *  keeping the day's Keylist itself intact and ready for reuse
 * @param pDay - daily schedule data to empty
 */
static void
Daily_Schedule_Time_Value_Delete_All(struct daily_schedule_data *pDay)
{
    BACNET_TIME_VALUE *pTV;

    if (pDay) {
        do {
            pTV = Keylist_Data_Pop(pDay->Time_Values);
            free(pTV);
        } while (pTV);
    }
}

/* bounded store used while decoding a written list-of-time-values (a
   Weekly_Schedule day, or an Exception_Schedule entry), since the codec
   itself must not allocate list nodes */
struct time_value_list_write_context {
    BACNET_DAILY_SCHEDULE_ENTRY entries[BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX];
    size_t count;
};

/**
 * @brief bacnet_dailyschedule_entry_store_fn callback that appends a
 *  decoded Time-Value into a bounded, statically-linked entries array
 * @param time_value - decoded Time-Value to store
 * @param ctx - struct time_value_list_write_context to store into
 * @return true if stored, false if the bounded store is full
 */
static bool Schedule_Time_Value_List_Store_Entry(
    const BACNET_TIME_VALUE *time_value, void *ctx)
{
    struct time_value_list_write_context *store = ctx;

    if (!time_value || !store) {
        return false;
    }
    if (store->count >= ARRAY_SIZE(store->entries)) {
        return false;
    }
    store->entries[store->count].Time_Value = *time_value;
    store->entries[store->count].next = NULL;
    if (store->count > 0) {
        store->entries[store->count - 1].next = &store->entries[store->count];
    }
    store->count++;

    return true;
}

#if BACNET_EXCEPTION_SCHEDULE_SIZE
/**
 * @brief Empty all the Time-Values from a single Exception_Schedule entry,
 *  keeping the entry's Keylist itself intact and ready for reuse
 * @param event - special event data to empty
 */
static void
Special_Event_Time_Value_Delete_All(struct special_event_data *event)
{
    BACNET_TIME_VALUE *pTV;

    if (event) {
        do {
            pTV = Keylist_Data_Pop(event->Time_Values);
            free(pTV);
        } while (pTV);
    }
}

/**
 * @brief Free a single Exception_Schedule entry and its Time-Values Keylist
 * @param event - special event data to free
 */
static void Special_Event_Free(struct special_event_data *event)
{
    if (event) {
        Special_Event_Time_Value_Delete_All(event);
        Keylist_Delete(event->Time_Values);
        free(event);
    }
}

/**
 * @brief Empty all the entries from Exception_Schedule, keeping the
 *  Keylist itself intact and ready for reuse
 * @param pObject - object data to empty
 */
static void Exception_Schedule_Delete_All(struct object_data *pObject)
{
    struct special_event_data *special_event;

    if (pObject) {
        Schedule_Invalidate_Active_Time_Value(pObject);
        do {
            special_event = Keylist_Data_Pop(pObject->Exception_Schedule);
            Special_Event_Free(special_event);
        } while (special_event);
    }
}
#endif

/**
 * @brief Empty all the entries from List_Of_Object_Property_References,
 *  keeping the Keylist itself intact and ready for reuse
 * @param pObject - object data to empty
 */
static void Object_Property_References_Delete_All(struct object_data *pObject)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember;

    if (pObject) {
        do {
            pMember = Keylist_Data_Pop(pObject->Object_Property_References);
            free(pMember);
        } while (pMember);
    }
}

/**
 * @brief Frees an object and all of its dynamically allocated data
 * @param pObject - object data to free
 */
static void Schedule_Free_Object(struct object_data *pObject)
{
    unsigned j;

    if (!pObject) {
        return;
    }
    Schedule_Invalidate_Active_Time_Value(pObject);
    for (j = 0; j < BACNET_WEEKLY_SCHEDULE_SIZE; j++) {
        Daily_Schedule_Time_Value_Delete_All(&pObject->Weekly_Schedule[j]);
        Keylist_Delete(pObject->Weekly_Schedule[j].Time_Values);
        pObject->Weekly_Schedule[j].Time_Values = NULL;
    }
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    Exception_Schedule_Delete_All(pObject);
    Keylist_Delete(pObject->Exception_Schedule);
    pObject->Exception_Schedule = NULL;
#endif
    Object_Property_References_Delete_All(pObject);
    Keylist_Delete(pObject->Object_Property_References);
    pObject->Object_Property_References = NULL;
    bacnet_character_cstring_free(&pObject->Object_Name);
    bacnet_character_cstring_free(&pObject->Description);
    free(pObject);
}

/**
 * @brief Creates a Schedule object
 * @param object_instance - object-instance number of the object, or
 *  BACNET_MAX_INSTANCE to auto-select the next available instance
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Schedule_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    unsigned j;
    int index = 0;
    BACNET_DATE start_date = { 0 }, end_date = { 0 };

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
        for (j = 0; j < BACNET_WEEKLY_SCHEDULE_SIZE; j++) {
            pObject->Weekly_Schedule[j].Time_Values = Keylist_Create();
            if (!pObject->Weekly_Schedule[j].Time_Values) {
                Schedule_Free_Object(pObject);
                return BACNET_MAX_INSTANCE;
            }
        }
#if BACNET_EXCEPTION_SCHEDULE_SIZE
        pObject->Exception_Schedule = Keylist_Create();
        if (!pObject->Exception_Schedule) {
            Schedule_Free_Object(pObject);
            return BACNET_MAX_INSTANCE;
        }
#endif
        pObject->Object_Property_References = Keylist_Create();
        if (!pObject->Object_Property_References) {
            Schedule_Free_Object(pObject);
            return BACNET_MAX_INSTANCE;
        }
        /* whole year, change as necessary */
        datetime_set_date(&start_date, 0, 1, 1);
        datetime_wildcard_year_set(&start_date);
        datetime_wildcard_weekday_set(&start_date);
        datetime_set_date(&end_date, 0, 12, 31);
        datetime_wildcard_year_set(&end_date);
        datetime_wildcard_weekday_set(&end_date);
        datetime_copy_date(&pObject->Start_Date, &start_date);
        datetime_copy_date(&pObject->End_Date, &end_date);
        pObject->Schedule_Default.context_specific = false;
        pObject->Schedule_Default.tag = BACNET_APPLICATION_TAG_REAL;
        pObject->Schedule_Default.type.Real = 21.0f; /* 21 C, room temp */
        memcpy(
            &pObject->Present_Value, &pObject->Schedule_Default,
            sizeof(pObject->Present_Value));
        pObject->Priority_For_Writing = 16; /* lowest priority */
        pObject->Out_Of_Service = false;
        /* add to list */
        index = Keylist_Data_Add(Object_List, object_instance, pObject);
        if (index < 0) {
            Schedule_Free_Object(pObject);
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * @brief Deletes a Schedule object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Schedule_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        Schedule_Free_Object(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all the Schedules and their data
 */
void Schedule_Cleanup(void)
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
                if (pObject) {
                    Schedule_Free_Object(pObject);
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
 * @brief Initialize the Schedule object data
 */
void Schedule_Init(void)
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

/**
 * @brief Determines if a given instance is valid
 * @param  object_instance - object-instance number of the object
 * @return true if the instance is valid, and false if not
 */
bool Schedule_Valid_Instance(uint32_t object_instance)
{
    if (Object_Data(object_instance)) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of Schedule objects
 * @return Number of Schedule objects
 */
unsigned Schedule_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance number for a given index
 * @param  index - index number of the object
 * @return object instance number for the given index
 */
uint32_t Schedule_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * @brief Determines the index for a given object instance number
 * @param  instance - object-instance number of the object
 * @return index number for the given object instance number, or
 * Schedule_Count() if the instance is not valid
 */
unsigned Schedule_Instance_To_Index(uint32_t instance)
{
    return Keylist_Index(Object_List, instance);
}

/**
 * @brief Set the object-name property value using write-property context.
 * @param wp_data [in,out] Write property request/response context.
 * @param cstring [in] New object-name value.
 * @return true if object-name was set.
 */
static bool Schedule_Object_Name_Write(
    BACNET_WRITE_PROPERTY_DATA *wp_data, BACNET_CHARACTER_STRING *cstring)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(wp_data->object_instance);
    if (pObject) {
        if (characterstring_utf8_valid(cstring)) {
            status = bacnet_character_cstring_from_characterstring_strdup(
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
 * @brief Determines the object name for a given object instance number.
 * @param object_instance - object-instance number of the object.
 * @param object_name - object name of the object.
 * @return true if the object name is valid, and false if not.
 */
bool Schedule_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    int len = 0;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (characterstring_utf8_valid(object_name)) {
            status = bacnet_character_cstring_to_characterstring(
                object_name, &pObject->Object_Name);
            if (!status) {
                len = characterstring_utf8_snprintf(
                    object_name, "SCHEDULE-%lu",
                    (unsigned long)object_instance);
                if (len > 0) {
                    status = true;
                }
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
bool Schedule_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = bacnet_character_cstring_set(&pObject->Object_Name, new_name);
    }

    return status;
}

/**
 * @brief Return the object name C string.
 * @param object_instance - BACnet object instance number.
 * @return object name or NULL if not found.
 */
const char *Schedule_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        name = bacnet_character_cstring_value_const(&pObject->Object_Name);
    }

    return name;
}

/**
 * @brief Set the description property value using write-property context.
 * @param wp_data [in,out] Write property request/response context.
 * @param cstring [in] New description value.
 * @return true if description was set.
 */
static bool Schedule_Description_Write(
    BACNET_WRITE_PROPERTY_DATA *wp_data, BACNET_CHARACTER_STRING *cstring)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(wp_data->object_instance);
    if (pObject) {
        status = bacnet_character_cstring_from_characterstring_strdup(
            &pObject->Description, cstring);
        if (!status) {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
        }
    } else {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * @brief Return the description for a given object instance number.
 * @param object_instance - object-instance number of the object.
 * @return description text or empty string if not found or unset.
 */
const char *Schedule_Description(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        name =
            bacnet_character_cstring_value_default(&pObject->Description, "");
    }

    return name;
}

/**
 * @brief For a given object instance-number, sets a BACnet character string
 *  by referencing an ANSI C string.
 * @param object_instance object-instance number of the object
 * @param new_name Holds a pointer to a static constant ANSI C string for
 *  zero copy, or NULL to clear it.
 * @return true if description was set
 */
bool Schedule_Description_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = bacnet_character_cstring_set(&pObject->Description, new_name);
    }

    return status;
}

/**
 * @brief Sets a specific Schedule object out of service
 * @param object_instance - object-instance number of the object
 * @param value - true if out of service, and false if not
 */
void Schedule_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Out_Of_Service = value;
    }
}

/**
 * @brief Gets a specific Schedule object out-of-service status
 * @param object_instance - object-instance number of the object
 * @return true if out of service, and false if not
 */
bool Schedule_Out_Of_Service(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        return pObject->Out_Of_Service;
    }

    return false;
}

/**
 * @brief Sets the Write_Every_Scheduled_Action property
 * @param object_instance - object-instance number of the object
 * @param value - true to write every scheduled action, and false to only
 *  write List_Of_Object_Property_References members when Present_Value
 *  changes
 * @return true if set, and false if not
 */
bool Schedule_Write_Every_Scheduled_Action_Set(
    uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Write_Every_Scheduled_Action = value;
        return true;
    }

    return false;
}

/**
 * @brief Gets the Write_Every_Scheduled_Action property
 * @param object_instance - object-instance number of the object
 * @return true if every scheduled action is written, and false if not
 */
bool Schedule_Write_Every_Scheduled_Action(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        return pObject->Write_Every_Scheduled_Action;
    }

    return false;
}

/**
 * @brief Sets callback used to write List_Of_Object_Property_References
 *  members, similar to the Loop and Timer object write-back pattern
 * @param cb - callback used to write referenced properties
 */
void Schedule_Write_Property_Internal_Callback_Set(write_property_function cb)
{
    Write_Property_Internal_Callback = cb;
}

/**
 * @brief Gets the day pointer of the Weekly Schedule for a given object
 *  instance
 * @param pObject - object in which to get the day
 * @param array_index - index of the Weekly Schedule to get 0 to 6
 * @return pointer to the daily schedule data, or NULL if not found
 */
static struct daily_schedule_data *
Weekly_Schedule_Day(struct object_data *pObject, unsigned array_index)
{
    if (pObject && (array_index < BACNET_WEEKLY_SCHEDULE_SIZE)) {
        return &pObject->Weekly_Schedule[array_index];
    }

    return NULL;
}

/**
 * @brief Get the Weekly Schedule for a given object instance
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Weekly Schedule to get 0 to 6
 * @param entries - caller-supplied array of nodes to fill and link;
 *  entries[0] is the head of the returned list when entries_count > 0
 * @param entries_size - number of nodes available in entries
 * @param entries_count - [out] number of nodes filled in entries
 * @return true if the Weekly Schedule was found and copied
 */
bool Schedule_Weekly_Schedule(
    uint32_t object_instance,
    unsigned array_index,
    BACNET_DAILY_SCHEDULE_ENTRY *entries,
    size_t entries_size,
    size_t *entries_count)
{
    struct daily_schedule_data *pDay;
    BACNET_TIME_VALUE *pTV;
    unsigned i, count;

    pDay = Weekly_Schedule_Day(Object_Data(object_instance), array_index);
    if (!pDay || !entries || !entries_size) {
        return false;
    }
    count = (unsigned)Keylist_Count(pDay->Time_Values);
    if (count > entries_size) {
        count = (unsigned)entries_size;
    }
    for (i = 0; i < count; i++) {
        pTV = Keylist_Data_Index(pDay->Time_Values, i);
        if (pTV) {
            entries[i].Time_Value = *pTV;
        }
        if (i + 1 < count) {
            entries[i].next = &entries[i + 1];
        } else {
            entries[i].next = NULL;
        }
    }
    if (entries_count) {
        *entries_count = count;
    }

    return true;
}

/**
 * @brief Set the Weekly Schedule for a given object instance, replacing all
 *  the Time-Values for that day
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Weekly Schedule to set 0 to 6
 * @param entries - head of the linked list of Time-Values to set, or NULL
 *  to empty the day
 * @return true if the Weekly Schedule was set, and false if not
 */
bool Schedule_Weekly_Schedule_Set(
    uint32_t object_instance,
    unsigned array_index,
    const BACNET_DAILY_SCHEDULE_ENTRY *entries)
{
    struct object_data *pObject;
    struct daily_schedule_data *pDay;
    BACNET_TIME_VALUE *pTV;
    const BACNET_DAILY_SCHEDULE_ENTRY *entry;
    unsigned count;

    pObject = Object_Data(object_instance);
    pDay = Weekly_Schedule_Day(pObject, array_index);
    if (!pDay) {
        return false;
    }
    Schedule_Invalidate_Active_Time_Value(pObject);
    Daily_Schedule_Time_Value_Delete_All(pDay);
    count = 0;
    for (entry = entries;
         entry && (count < BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX);
         entry = entry->next, count++) {
        pTV = calloc(1, sizeof(BACNET_TIME_VALUE));
        if (!pTV) {
            /* roll back so the day is not left partially updated */
            Daily_Schedule_Time_Value_Delete_All(pDay);
            return false;
        }
        *pTV = entry->Time_Value;
        if (Keylist_Data_Add(pDay->Time_Values, (KEY)count, pTV) < 0) {
            free(pTV);
            Daily_Schedule_Time_Value_Delete_All(pDay);
            return false;
        }
    }

    return true;
}

/**
 * @brief Get the number of Time-Values in a given day of Weekly_Schedule
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Weekly Schedule day 0 to 6
 * @return number of Time-Values stored for that day
 */
size_t Schedule_Weekly_Schedule_Time_Value_Count(
    uint32_t object_instance, unsigned array_index)
{
    struct daily_schedule_data *pDay;

    pDay = Weekly_Schedule_Day(Object_Data(object_instance), array_index);
    if (!pDay) {
        return 0;
    }

    return (size_t)Keylist_Count(pDay->Time_Values);
}

/**
 * @brief Get a single Time-Value from a given day of Weekly_Schedule
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Weekly Schedule day 0 to 6
 * @param index - 0-based index of the Time-Value within the day
 * @param value - copy of the Time-Value, if found
 * @return true if found and copied
 */
bool Schedule_Weekly_Schedule_Time_Value(
    uint32_t object_instance,
    unsigned array_index,
    unsigned index,
    BACNET_TIME_VALUE *value)
{
    struct daily_schedule_data *pDay;
    BACNET_TIME_VALUE *pTV;

    pDay = Weekly_Schedule_Day(Object_Data(object_instance), array_index);
    if (!pDay || !value) {
        return false;
    }
    pTV = Keylist_Data_Index(pDay->Time_Values, (int)index);
    if (!pTV) {
        return false;
    }
    memcpy(value, pTV, sizeof(BACNET_TIME_VALUE));

    return true;
}

/**
 * @brief Set (replace) or append a single Time-Value in a given day of
 *  Weekly_Schedule
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Weekly Schedule day 0 to 6
 * @param index - 0-based index of the Time-Value within the day; use the
 *  current count to append a new Time-Value
 * @param value - Time-Value to set
 * @return true if set, false if not set (bad index or DoS guard reached)
 */
bool Schedule_Weekly_Schedule_Time_Value_Set(
    uint32_t object_instance,
    unsigned array_index,
    unsigned index,
    const BACNET_TIME_VALUE *value)
{
    struct daily_schedule_data *pDay;
    BACNET_TIME_VALUE *pTV;
    unsigned count;

    pDay = Weekly_Schedule_Day(Object_Data(object_instance), array_index);
    if (!pDay || !value) {
        return false;
    }
    count = (unsigned)Keylist_Count(pDay->Time_Values);
    if (index < count) {
        pTV = Keylist_Data_Index(pDay->Time_Values, (int)index);
        if (!pTV) {
            return false;
        }
        memcpy(pTV, value, sizeof(BACNET_TIME_VALUE));
        return true;
    } else if (index == count) {
        if (count >= BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX) {
            return false;
        }
        pTV = calloc(1, sizeof(BACNET_TIME_VALUE));
        if (!pTV) {
            return false;
        }
        memcpy(pTV, value, sizeof(BACNET_TIME_VALUE));
        if (Keylist_Data_Add(pDay->Time_Values, (KEY)index, pTV) < 0) {
            free(pTV);
            return false;
        }
        return true;
    }

    return false;
}

/**
 * @brief Delete all the Time-Values from a given day of Weekly_Schedule
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Weekly Schedule day 0 to 6
 * @return true if found and emptied
 */
bool Schedule_Weekly_Schedule_Time_Value_Delete_All(
    uint32_t object_instance, unsigned array_index)
{
    struct object_data *pObject;
    struct daily_schedule_data *pDay;

    pObject = Object_Data(object_instance);
    pDay = Weekly_Schedule_Day(pObject, array_index);
    if (!pDay) {
        return false;
    }
    Schedule_Invalidate_Active_Time_Value(pObject);
    Daily_Schedule_Time_Value_Delete_All(pDay);

    return true;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Schedule_Weekly_Schedule_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    BACNET_DAILY_SCHEDULE_ENTRY
    entries[BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX] = { 0 };
    size_t count = 0;

    if (array_index >= BACNET_WEEKLY_SCHEDULE_SIZE) {
        return BACNET_STATUS_ERROR;
    }
    if (!Schedule_Weekly_Schedule(
            object_instance, array_index, entries, ARRAY_SIZE(entries),
            &count)) {
        return BACNET_STATUS_ERROR;
    }

    if (count) {
        return bacnet_dailyschedule_list_context_encode(apdu, 0, &entries[0]);
    } else {
        return bacnet_dailyschedule_list_context_encode(apdu, 0, NULL);
    }
}

#if BACNET_EXCEPTION_SCHEDULE_SIZE
/**
 * @brief Set the periodTag/period/priority/list-of-time-values of an
 *  Exception_Schedule entry from a caller-supplied linked list
 * @param event - special event data to set
 * @param value - periodTag/period/priority and head of the linked list of
 *  Time-Values to set
 * @return true if set, and false if not (allocation failure)
 */
static bool Special_Event_Data_Set(
    struct object_data *pObject,
    struct special_event_data *event,
    const BACNET_SPECIAL_EVENT_ENTRY *value)
{
    BACNET_TIME_VALUE *time_value;
    const BACNET_DAILY_SCHEDULE_ENTRY *entry;
    unsigned count;

    if (!event || !value || (value->priority > BACNET_MAX_PRIORITY)) {
        return false;
    }
    event->periodTag = value->periodTag;
    memcpy(&event->period, &value->period, sizeof(event->period));
    event->priority = value->priority;
    Schedule_Invalidate_Active_Time_Value(pObject);
    Special_Event_Time_Value_Delete_All(event);
    count = 0;
    for (entry = value->timeValues;
         entry && (count < BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX);
         entry = entry->next, count++) {
        time_value = calloc(1, sizeof(BACNET_TIME_VALUE));
        if (!time_value) {
            /* roll back so the entry is not left partially updated */
            Special_Event_Time_Value_Delete_All(event);
            return false;
        }
        *time_value = entry->Time_Value;
        if (Keylist_Data_Add(event->Time_Values, (KEY)count, time_value) < 0) {
            free(time_value);
            Special_Event_Time_Value_Delete_All(event);
            return false;
        }
    }

    return true;
}

/**
 * @brief Get an Exception Schedule entry for a given object instance
 * @param object_instance - object-instance number of the object
 * @param index - 0-based index of the Exception Schedule entry
 * @param value - [out] periodTag/period/priority/timeValues, or NULL
 * @param entries - caller-supplied array of nodes to fill and link the
 *  list-of-time-values; entries[0] is the head of the returned list when
 *  entries_count > 0
 * @param entries_size - number of nodes available in entries
 * @param entries_count - [out] number of nodes filled in entries
 * @return true if the Exception Schedule entry was found and copied
 */
bool Schedule_Exception_Schedule(
    uint32_t object_instance,
    unsigned index,
    BACNET_SPECIAL_EVENT_ENTRY *value,
    BACNET_DAILY_SCHEDULE_ENTRY *entries,
    size_t entries_size,
    size_t *entries_count)
{
    struct object_data *pObject;
    struct special_event_data *event;
    BACNET_TIME_VALUE *pTV;
    unsigned i, count = 0;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return false;
    }
    event = Keylist_Data_Index(pObject->Exception_Schedule, (int)index);
    if (!event) {
        return false;
    }
    if (entries && entries_size) {
        count = (unsigned)Keylist_Count(event->Time_Values);
        if (count > entries_size) {
            count = (unsigned)entries_size;
        }
        for (i = 0; i < count; i++) {
            pTV = Keylist_Data_Index(event->Time_Values, i);
            if (pTV) {
                entries[i].Time_Value = *pTV;
            }
            if (i + 1 < count) {
                entries[i].next = &entries[i + 1];
            } else {
                entries[i].next = NULL;
            }
        }
    }
    if (entries_count) {
        *entries_count = count;
    }
    if (value) {
        value->periodTag = event->periodTag;
        memcpy(&value->period, &event->period, sizeof(value->period));
        value->priority = event->priority;
        if (count) {
            value->timeValues = &entries[0];
        } else {
            value->timeValues = NULL;
        }
    }

    return true;
}

/**
 * @brief Add a new Exception Schedule entry to a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - pointer to the Exception Schedule entry to add
 * @return true if added, false if not added (DoS guard reached)
 */
bool Schedule_Exception_Schedule_Add(
    uint32_t object_instance, const BACNET_SPECIAL_EVENT_ENTRY *value)
{
    struct object_data *pObject;
    struct special_event_data *event;
    unsigned count;

    pObject = Object_Data(object_instance);
    if (!pObject || !value) {
        return false;
    }
    count = (unsigned)Keylist_Count(pObject->Exception_Schedule);
    if (count >= BACNET_EXCEPTION_SCHEDULE_SIZE) {
        return false;
    }
    event = calloc(1, sizeof(struct special_event_data));
    if (!event) {
        return false;
    }
    event->Time_Values = Keylist_Create();
    if (!event->Time_Values) {
        free(event);
        return false;
    }
    if (!Special_Event_Data_Set(pObject, event, value)) {
        Special_Event_Free(event);
        return false;
    }
    if (Keylist_Data_Add(pObject->Exception_Schedule, (KEY)count, event) < 0) {
        Special_Event_Free(event);
        return false;
    }

    return true;
}

/**
 * @brief Set (replace) or append an Exception Schedule entry
 * @param object_instance - object-instance number of the object
 * @param index - 0-based index of the Exception Schedule entry; use the
 *  current count to append a new entry
 * @param value - pointer to the Exception Schedule entry to set
 * @return true if the Exception Schedule entry was set, and false if not
 */
bool Schedule_Exception_Schedule_Set(
    uint32_t object_instance,
    unsigned index,
    const BACNET_SPECIAL_EVENT_ENTRY *value)
{
    struct object_data *pObject;
    struct special_event_data *event;
    unsigned count;

    pObject = Object_Data(object_instance);
    if (!pObject || !value) {
        return false;
    }
    count = (unsigned)Keylist_Count(pObject->Exception_Schedule);
    if (index < count) {
        event = Keylist_Data_Index(pObject->Exception_Schedule, (int)index);
        if (!event) {
            return false;
        }
        return Special_Event_Data_Set(pObject, event, value);
    } else if (index == count) {
        return Schedule_Exception_Schedule_Add(object_instance, value);
    }

    return false;
}

/**
 * @brief Get the number of Exception Schedule entries
 * @param object_instance - object-instance number of the object
 * @return number of Exception Schedule entries
 */
unsigned Schedule_Exception_Schedule_Count(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return 0;
    }

    return (unsigned)Keylist_Count(pObject->Exception_Schedule);
}

/**
 * @brief Delete all the Exception Schedule entries
 * @param object_instance - object-instance number of the object
 * @return true if found and emptied
 */
bool Schedule_Exception_Schedule_Delete_All(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return false;
    }
    Schedule_Invalidate_Active_Time_Value(pObject);
    Exception_Schedule_Delete_All(pObject);

    return true;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet object instance number
 * @param array_index [in] array index requested: 0 to N for individual
 *  array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or BACNET_STATUS_ERROR
 */
static int Schedule_Exception_Schedule_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    BACNET_SPECIAL_EVENT_ENTRY value = { 0 };
    BACNET_DAILY_SCHEDULE_ENTRY
    entries[BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX] = { 0 };
    size_t count = 0;

    if (!Schedule_Exception_Schedule(
            object_instance, array_index, &value, entries, ARRAY_SIZE(entries),
            &count)) {
        return BACNET_STATUS_ERROR;
    }

    return bacnet_special_event_entry_encode(apdu, &value);
}

/**
 * @brief Decode one BACnetARRAY property element to determine its length
 * @param object_instance [in] BACnet object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Schedule_Exception_Schedule_Element_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    (void)object_instance;

    return bacnet_special_event_entry_decode(apdu, apdu_size, NULL, NULL, NULL);
}

/**
 * @brief Write a value to a BACnetARRAY property element value
 * @param object_instance [in] BACnet object instance number
 * @param array_index [in] array index to write:
 *    0=array size (resize, subject to the DoS guard), 1 to N for
 *    individual array members
 * @param array_size [in] The total number of elements in the array,
 *  if writing array size
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Schedule_Exception_Schedule_Element_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *application_data,
    size_t application_data_len)
{
    struct object_data *pObject;
    struct special_event_data *new_event;
    BACNET_SPECIAL_EVENT_ENTRY special_event = { 0 };
    struct time_value_list_write_context store = { 0 };
    unsigned count;
    int len;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return ERROR_CODE_UNKNOWN_OBJECT;
    }
    if (array_index == 0) {
        /* resize the array, growing with empty entries or shrinking from
           the end, subject to the DoS guard */
        if (array_size > BACNET_EXCEPTION_SCHEDULE_SIZE) {
            return ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
        count = (unsigned)Keylist_Count(pObject->Exception_Schedule);
        Schedule_Invalidate_Active_Time_Value(pObject);
        while (count > array_size) {
            new_event = Keylist_Data_Pop(pObject->Exception_Schedule);
            Special_Event_Free(new_event);
            count--;
        }
        while (count < array_size) {
            new_event = calloc(1, sizeof(struct special_event_data));
            if (!new_event) {
                return ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
            }
            new_event->Time_Values = Keylist_Create();
            if (!new_event->Time_Values) {
                free(new_event);
                return ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
            }
            if (Keylist_Data_Add(
                    pObject->Exception_Schedule, (KEY)count, new_event) < 0) {
                Special_Event_Free(new_event);
                return ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
            }
            count++;
        }
        return ERROR_CODE_SUCCESS;
    }
    array_index--; /* 1-based protocol index -> 0-based internal index */
    len = bacnet_special_event_entry_decode(
        application_data, application_data_len, &special_event,
        Schedule_Time_Value_List_Store_Entry, &store);
    if (len <= 0) {
        return ERROR_CODE_INVALID_DATA_TYPE;
    }
    if (store.count) {
        special_event.timeValues = &store.entries[0];
    } else {
        special_event.timeValues = NULL;
    }
    new_event =
        Keylist_Data_Index(pObject->Exception_Schedule, (int)array_index);
    if (!new_event) {
        return ERROR_CODE_INVALID_ARRAY_INDEX;
    }
    if (!Special_Event_Data_Set(pObject, new_event, &special_event)) {
        return ERROR_CODE_INVALID_DATA_TYPE;
    }

    return ERROR_CODE_SUCCESS;
}
#endif

/**
 * @brief Set the Effective Period for a given object instance
 * @param object_instance - object-instance number of the object
 * @param start_date - start date of the effective period
 * @param end_date - end date of the effective period
 * @return true if the effective period was set, and false if not
 */
bool Schedule_Effective_Period_Set(
    uint32_t object_instance,
    const BACNET_DATE *start_date,
    const BACNET_DATE *end_date)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        datetime_copy_date(&pObject->Start_Date, start_date);
        datetime_copy_date(&pObject->End_Date, end_date);
        return true;
    }

    return false;
}

/**
 * @brief Get the Effective Period for a given object instance
 * @param object_instance - object-instance number of the object
 * @param start_date - start date of the effective period
 * @param end_date - end date of the effective period
 * @return true if the effective period was set, and false if not
 */
bool Schedule_Effective_Period(
    uint32_t object_instance, BACNET_DATE *start_date, BACNET_DATE *end_date)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        datetime_copy_date(start_date, &pObject->Start_Date);
        datetime_copy_date(end_date, &pObject->End_Date);
        return true;
    }

    return false;
}

/**
 * @brief Get a member element of the List_Of_Object_Property_References
 *  BACnetLIST property
 * @param object_instance - object-instance number of the object
 * @param index - 0-based list index
 * @param pMember - pointer to member value to fill in
 * @return true if found and copied
 */
bool Schedule_List_Of_Object_Property_References(
    uint32_t object_instance,
    unsigned index,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    struct object_data *pObject;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pEntry;

    pObject = Object_Data(object_instance);
    if (!pObject || !pMember) {
        return false;
    }
    pEntry =
        Keylist_Data_Index(pObject->Object_Property_References, (int)index);
    if (!pEntry) {
        return false;
    }

    return bacnet_device_object_property_reference_copy(pMember, pEntry);
}

/**
 * @brief Add a new member element to the List_Of_Object_Property_References
 *  BACnetLIST property
 * @param object_instance - object-instance number of the object
 * @param pMember - pointer to member value to add
 * @return true if the element was added (or already present), false if not
 *  added (DoS guard reached)
 */
bool Schedule_List_Of_Object_Property_References_Add(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    struct object_data *pObject;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pEntry;
    unsigned i, count;

    pObject = Object_Data(object_instance);
    if (!pObject || !pMember) {
        return false;
    }
    count = (unsigned)Keylist_Count(pObject->Object_Property_References);
    for (i = 0; i < count; i++) {
        pEntry =
            Keylist_Data_Index(pObject->Object_Property_References, (int)i);
        if (pEntry &&
            bacnet_device_object_property_reference_same(pEntry, pMember)) {
            /* already present */
            return true;
        }
    }
    if (count >= BACNET_SCHEDULE_OBJ_PROP_REF_SIZE) {
        return false;
    }
    pEntry = calloc(1, sizeof(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE));
    if (!pEntry) {
        return false;
    }
    bacnet_device_object_property_reference_copy(pEntry, pMember);
    if (Keylist_Data_Add(
            pObject->Object_Property_References, (KEY)count, pEntry) < 0) {
        free(pEntry);
        return false;
    }

    return true;
}

/**
 * @brief Set (replace) or append a member element of the
 *  List_Of_Object_Property_References BACnetLIST property
 * @param object_instance - object-instance number of the object
 * @param index - 0-based list index; use the current count to append
 * @param pMember - pointer to member value
 * @return true if set, false if not set
 */
bool Schedule_List_Of_Object_Property_References_Set(
    uint32_t object_instance,
    unsigned index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    struct object_data *pObject;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pEntry;
    unsigned count;

    pObject = Object_Data(object_instance);
    if (!pObject || !pMember) {
        return false;
    }
    count = (unsigned)Keylist_Count(pObject->Object_Property_References);
    if (index < count) {
        pEntry =
            Keylist_Data_Index(pObject->Object_Property_References, (int)index);
        if (!pEntry) {
            return false;
        }
        return bacnet_device_object_property_reference_copy(pEntry, pMember);
    } else if (index == count) {
        return Schedule_List_Of_Object_Property_References_Add(
            object_instance, pMember);
    }

    return false;
}

/**
 * @brief Get the maximum size of the list of object property references
 * @param object_instance [in] BACnet object instance number
 * @return The maximum size (DoS guard) of the list of object property
 *  references
 */
size_t
Schedule_List_Of_Object_Property_References_Capacity(uint32_t object_instance)
{
    (void)object_instance; /* unused */
    return BACNET_SCHEDULE_OBJ_PROP_REF_SIZE;
}

/**
 * @brief Get the number of List_Of_Object_Property_References entries
 * @param object_instance - object-instance number of the object
 * @return number of List_Of_Object_Property_References entries
 */
unsigned
Schedule_List_Of_Object_Property_References_Count(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return 0;
    }

    return (unsigned)Keylist_Count(pObject->Object_Property_References);
}

/**
 * @brief Delete all the List_Of_Object_Property_References entries
 * @param object_instance - object-instance number of the object
 * @return true if found and emptied
 */
bool Schedule_List_Of_Object_Property_References_Delete_All(
    uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return false;
    }
    Object_Property_References_Delete_All(pObject);

    return true;
}

/**
 * @brief Encode a BACnetLIST property element
 * @param object_instance [in] BACnet object instance number
 * @param list_index [in] list index requested: 0 to N for individual members
 * @param apdu [out] Buffer in which the APDU contents are built
 * @return The length of the apdu encoded or 0 if invalid member
 */
static int Schedule_List_Of_Object_Property_References_Encode(
    uint32_t object_instance, uint32_t list_index, uint8_t *apdu)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE value = { 0 };

    if (!Schedule_List_Of_Object_Property_References(
            object_instance, list_index, &value)) {
        return 0;
    }

    return bacapp_encode_device_obj_property_ref(apdu, &value);
}

/**
 * @brief Decode a BACnetLIST property element to determine the element
 *  length
 * @param object_instance [in] BACnet object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Schedule_List_Of_Object_Property_References_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE value = { 0 };

    (void)object_instance;

    return bacnet_device_object_property_reference_decode(
        apdu, (uint32_t)apdu_size, &value);
}

/**
 * @brief Add one decoded element to the List_Of_Object_Property_References
 *  BACnetLIST, or empty the list when application_data is NULL (per
 *  bacnet_list_write())
 * @param object_instance [in] BACnet object instance number
 * @param application_data [in] encoded element value, or NULL to clear
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE
Schedule_List_Of_Object_Property_References_Element_Add(
    uint32_t object_instance,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE value = { 0 };
    int len;

    if (!application_data || (application_data_len == 0)) {
        Schedule_List_Of_Object_Property_References_Delete_All(object_instance);
        return ERROR_CODE_SUCCESS;
    }
    len = bacnet_device_object_property_reference_decode(
        application_data, (uint32_t)application_data_len, &value);
    if (len <= 0) {
        return ERROR_CODE_INVALID_DATA_TYPE;
    }
    if (Schedule_List_Of_Object_Property_References_Add(
            object_instance, &value)) {
        return ERROR_CODE_SUCCESS;
    }

    return ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
}

/**
 * @brief Remove one matching element from the
 *  List_Of_Object_Property_References BACnetLIST, or empty the list when
 *  application_data is NULL
 * @param object_instance [in] BACnet object instance number
 * @param application_data [in] encoded element value, or NULL to clear
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE
Schedule_List_Of_Object_Property_References_Element_Remove(
    uint32_t object_instance,
    uint8_t *application_data,
    size_t application_data_len)
{
    struct object_data *pObject;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE value = { 0 }, *pEntry;
    unsigned i, count;
    int len, found = -1;

    pObject = Object_Data(object_instance);
    if (!pObject) {
        return ERROR_CODE_UNKNOWN_OBJECT;
    }
    if (!application_data || (application_data_len == 0)) {
        Object_Property_References_Delete_All(pObject);
        return ERROR_CODE_SUCCESS;
    }
    len = bacnet_device_object_property_reference_decode(
        application_data, (uint32_t)application_data_len, &value);
    if (len <= 0) {
        return ERROR_CODE_INVALID_DATA_TYPE;
    }
    count = (unsigned)Keylist_Count(pObject->Object_Property_References);
    for (i = 0; i < count; i++) {
        pEntry =
            Keylist_Data_Index(pObject->Object_Property_References, (int)i);
        if (pEntry &&
            bacnet_device_object_property_reference_same(pEntry, &value)) {
            found = (int)i;
            break;
        }
    }
    if (found < 0) {
        return ERROR_CODE_LIST_ELEMENT_NOT_FOUND;
    }
    pEntry = Keylist_Data_Delete_By_Index(
        pObject->Object_Property_References, found);
    free(pEntry);

    return ERROR_CODE_SUCCESS;
}

/**
 * @brief Read a property from the Schedule object
 * @param rpdata [in] pointer to the read property data structure
 * @return The length of the apdu encoded or BACNET_STATUS_ERROR
 */
int Schedule_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    struct object_data *CurrentSC;
    uint8_t *apdu = NULL;
    uint16_t apdu_max = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned i, imax;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    CurrentSC = Object_Data(rpdata->object_instance);
    if (!CurrentSC) {
        return BACNET_STATUS_ERROR;
    }
    apdu = rpdata->application_data;
    apdu_max = rpdata->application_data_len;
    switch ((int)rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_SCHEDULE, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Schedule_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, Schedule_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_SCHEDULE);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = bacapp_encode_data(&apdu[0], &CurrentSC->Present_Value);
            break;
        case PROP_EFFECTIVE_PERIOD:
            apdu_len =
                encode_application_date(&apdu[0], &CurrentSC->Start_Date);
            apdu_len +=
                encode_application_date(&apdu[apdu_len], &CurrentSC->End_Date);
            break;
        case PROP_WEEKLY_SCHEDULE:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Schedule_Weekly_Schedule_Encode, BACNET_WEEKLY_SCHEDULE_SIZE,
                apdu, apdu_max);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
#if BACNET_EXCEPTION_SCHEDULE_SIZE
        case PROP_EXCEPTION_SCHEDULE:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Schedule_Exception_Schedule_Encode,
                Schedule_Exception_Schedule_Count(rpdata->object_instance),
                apdu, apdu_max);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
#endif
        case PROP_SCHEDULE_DEFAULT:
            apdu_len =
                bacapp_encode_data(&apdu[0], &CurrentSC->Schedule_Default);
            break;
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            imax = Schedule_List_Of_Object_Property_References_Count(
                rpdata->object_instance);
            for (i = 0; i < imax; i++) {
                apdu_len += Schedule_List_Of_Object_Property_References_Encode(
                    rpdata->object_instance, i, &apdu[apdu_len]);
            }
            break;
        case PROP_PRIORITY_FOR_WRITING:
            apdu_len = encode_application_unsigned(
                &apdu[0], CurrentSC->Priority_For_Writing);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                CurrentSC->Out_Of_Service);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], RELIABILITY_NO_FAULT_DETECTED);
            break;

        case PROP_OUT_OF_SERVICE:
            apdu_len =
                encode_application_boolean(&apdu[0], CurrentSC->Out_Of_Service);
            break;
#if (BACNET_PROTOCOL_REVISION >= 24)
        case PROP_WRITE_EVERY_SCHEDULED_ACTION:
            apdu_len = encode_application_boolean(
                &apdu[0], CurrentSC->Write_Every_Scheduled_Action);
            break;
#endif
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/**
 * @brief Write a value to a BACnetARRAY property element value
 * @param object_instance [in] BACnet object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param array_size [in] The total number of elements in the array,
 *  if writing array size
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Schedule_Weekly_Schedule_Element_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    struct object_data *pObject;
    struct time_value_list_write_context store = { 0 };
    BACNET_DAILY_SCHEDULE_ENTRY *head;
    int len = 0;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (array_index == 0) {
            /* This array is not required to be resizable
                through BACnet write services */
            (void)array_size;
            error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        } else {
            array_index--;
            len = bacnet_dailyschedule_list_context_decode(
                application_data, application_data_len, 0,
                Schedule_Time_Value_List_Store_Entry, &store);
            if (len > 0) {
                if (store.count) {
                    head = &store.entries[0];
                } else {
                    head = NULL;
                }
                if (Schedule_Weekly_Schedule_Set(
                        object_instance, array_index, head)) {
                    error_code = ERROR_CODE_SUCCESS;
                } else {
                    error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                }
            } else {
                error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
        }
    }

    return error_code;
}

/**
 * @brief Decode one BACnetARRAY property element to determine its length
 * @param object_instance [in] BACnet object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Schedule_Weekly_Schedule_Element_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    (void)object_instance;

    return bacnet_dailyschedule_list_context_decode(
        apdu, apdu_size, 0, NULL, NULL);
}

/**
 * @brief Write a property to the Schedule object
 * @param wp_data - pointer to the write property data
 * @return true if the write was successful, and false if not
 */
bool Schedule_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    bool boolean_value = false;
    BACNET_CHARACTER_STRING char_string = { 0 };
    BACNET_DATE_RANGE date_range = { 0 };
    struct object_data *pObject;

    /* Valid data? */
    if (wp_data == NULL) {
        return false;
    }
    pObject = Object_Data(wp_data->object_instance);
    if (!pObject) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    switch ((int)wp_data->object_property) {
        case PROP_OUT_OF_SERVICE:
            len = bacnet_boolean_application_decode(
                wp_data->application_data, wp_data->application_data_len,
                &boolean_value);
            if (len <= 0) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                if (len < 0) {
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
                return false;
            }
            Schedule_Out_Of_Service_Set(
                wp_data->object_instance, boolean_value);
            status = true;
            break;
#if (BACNET_PROTOCOL_REVISION >= 24)
        case PROP_WRITE_EVERY_SCHEDULED_ACTION:
            len = bacnet_boolean_application_decode(
                wp_data->application_data, wp_data->application_data_len,
                &boolean_value);
            if (len <= 0) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                if (len < 0) {
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
                return false;
            }
            Schedule_Write_Every_Scheduled_Action_Set(
                wp_data->object_instance, boolean_value);
            status = true;
            break;
#endif
        case PROP_WEEKLY_SCHEDULE:
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                Schedule_Weekly_Schedule_Element_Length,
                Schedule_Weekly_Schedule_Element_Write,
                BACNET_WEEKLY_SCHEDULE_SIZE, wp_data->application_data,
                wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            wp_data->error_code = bacnet_list_write(
                wp_data->object_instance, wp_data->array_index,
                Schedule_List_Of_Object_Property_References_Length,
                Schedule_List_Of_Object_Property_References_Element_Add,
                BACNET_SCHEDULE_OBJ_PROP_REF_SIZE, wp_data->application_data,
                wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
        case PROP_EFFECTIVE_PERIOD:
            len = bacnet_daterange_decode(
                wp_data->application_data, wp_data->application_data_len,
                &date_range);
            if (len <= 0) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                if (len < 0) {
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
                return false;
            }
            /* set the start and end date */
            datetime_copy_date(&pObject->Start_Date, &date_range.startdate);
            datetime_copy_date(&pObject->End_Date, &date_range.enddate);
            status = true;
            break;
#if BACNET_EXCEPTION_SCHEDULE_SIZE
        case PROP_EXCEPTION_SCHEDULE:
            wp_data->error_code = bacnet_array_write_resizable(
                wp_data->object_instance, wp_data->array_index,
                Schedule_Exception_Schedule_Element_Length,
                Schedule_Exception_Schedule_Element_Write,
                Schedule_Exception_Schedule_Count(wp_data->object_instance),
                wp_data->application_data, wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
#endif
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
            status = Schedule_Object_Name_Write(wp_data, &char_string);
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
            status = Schedule_Description_Write(wp_data, &char_string);
            break;
        default:
            if (property_lists_member(
                    Schedule_Properties_Required, Schedule_Properties_Optional,
                    Schedule_Properties_Proprietary,
                    wp_data->object_property)) {
                debug_log_fprintf(
                    DEBUG_LOG_DEBUG, stderr, "Schedule_Write_Property: %s\n",
                    bactext_property_name(wp_data->object_property));
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
 * @brief AddListElement to a BACnetLIST property
 * @param list_element [in] Pointer to the BACnet_List_Element_Data structure,
 * which is packed with the information from the request.
 * @return #BACNET_STATUS_OK or #BACNET_STATUS_ERROR or
 * #BACNET_STATUS_ABORT or #BACNET_STATUS_REJECT
 */
int Schedule_Add_List_Element(BACNET_LIST_ELEMENT_DATA *list_element)
{
    if (!list_element) {
        return BACNET_STATUS_ABORT;
    }
    list_element->error_class = ERROR_CLASS_PROPERTY;
    if (list_element->array_index != BACNET_ARRAY_ALL) {
        list_element->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return BACNET_STATUS_ERROR;
    }
    switch (list_element->object_property) {
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            list_element->error_code =
                Schedule_List_Of_Object_Property_References_Element_Add(
                    list_element->object_instance,
                    list_element->application_data,
                    list_element->application_data_len);
            break;
        default:
            list_element->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }
    if (list_element->error_code == ERROR_CODE_SUCCESS) {
        return BACNET_STATUS_OK;
    }
    if (list_element->error_code == ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY) {
        list_element->error_class = ERROR_CLASS_RESOURCES;
        list_element->error_code = ERROR_CODE_NO_SPACE_TO_ADD_LIST_ELEMENT;
    }

    return BACNET_STATUS_ERROR;
}

/**
 * @brief RemoveListElement from a BACnetLIST property
 * @param list_element [in] Pointer to the BACnet_List_Element_Data structure,
 * which is packed with the information from the request.
 * @return #BACNET_STATUS_OK or #BACNET_STATUS_ERROR or
 * #BACNET_STATUS_ABORT or #BACNET_STATUS_REJECT
 */
int Schedule_Remove_List_Element(BACNET_LIST_ELEMENT_DATA *list_element)
{
    if (!list_element) {
        return BACNET_STATUS_ABORT;
    }
    list_element->error_class = ERROR_CLASS_PROPERTY;
    if (list_element->array_index != BACNET_ARRAY_ALL) {
        list_element->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return BACNET_STATUS_ERROR;
    }
    switch (list_element->object_property) {
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            list_element->error_code =
                Schedule_List_Of_Object_Property_References_Element_Remove(
                    list_element->object_instance,
                    list_element->application_data,
                    list_element->application_data_len);
            break;
        default:
            list_element->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }
    if (list_element->error_code == ERROR_CODE_SUCCESS) {
        return BACNET_STATUS_OK;
    }
    if (list_element->error_code == ERROR_CODE_LIST_ELEMENT_NOT_FOUND) {
        list_element->error_class = ERROR_CLASS_SERVICES;
    }

    return BACNET_STATUS_ERROR;
}

/**
 * @brief Determine if the given date is within the effective period
 * @param object_instance - object-instance number of the object
 * @param date - date to check
 * @return true if the date is within the effective period
 */
bool Schedule_In_Effective_Period(
    uint32_t object_instance, const BACNET_DATE *date)
{
    struct object_data *pObject;
    bool res = false;

    pObject = Object_Data(object_instance);
    if (pObject && date) {
        if (datetime_wildcard_compare_date(&pObject->Start_Date, date) <= 0 &&
            datetime_wildcard_compare_date(&pObject->End_Date, date) >= 0) {
            res = true;
        }
    }

    return res;
}

/**
 * @brief Find the latest BACnetTimeValue in a Weekly_Schedule day whose
 *  time is at or before the given time
 * @param Time_Values - OS_Keylist of BACNET_TIME_VALUE* for one day
 * @param time - time of day to evaluate against
 * @return pointer to the matching BACNET_TIME_VALUE, or NULL if none apply
 */
static const BACNET_TIME_VALUE *Schedule_Weekly_Day_Current_Time_Value(
    OS_Keylist Time_Values, const BACNET_TIME *time)
{
    BACNET_TIME_VALUE *pTV, *pCurrent = NULL;
    unsigned i, count;
    int diff;

    count = (unsigned)Keylist_Count(Time_Values);
    for (i = 0; i < count; i++) {
        pTV = Keylist_Data_Index(Time_Values, (int)i);
        if (!pTV) {
            continue;
        }
        diff = datetime_wildcard_compare_time(time, &pTV->Time);
        if (diff >= 0) {
            if (!pCurrent) {
                pCurrent = pTV;
            } else {
                diff =
                    datetime_wildcard_compare_time(&pTV->Time, &pCurrent->Time);
                if (diff >= 0) {
                    pCurrent = pTV;
                }
            }
        }
    }

    return pCurrent;
}

#if BACNET_EXCEPTION_SCHEDULE_SIZE
/* registered by the Device object so a CalendarReference period can be
   resolved without a build dependency on the Calendar object */
static read_property_function Read_Property_Internal_Callback;

/**
 * @brief Sets callback used to resolve Exception_Schedule CalendarReference
 *  periods by reading the referenced object's Present_Value
 * @param cb - callback used to read referenced properties
 */
void Schedule_Read_Property_Internal_Callback_Set(read_property_function cb)
{
    Read_Property_Internal_Callback = cb;
}

/**
 * @brief Determine if a BACnetSpecialEvent's Period is in effect for a date
 * @param event - Exception_Schedule entry to evaluate
 * @param date - date to check against the event's Period
 * @return true if the event's Period matches the given date
 */
static bool Schedule_Special_Event_In_Effect(
    const struct special_event_data *event, const BACNET_DATE *date)
{
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    uint8_t apdu[8];
    int apdu_len;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    if (event->periodTag == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY) {
        return bacapp_date_in_calendar_entry(
            date, &event->period.calendarEntry);
    }
    /* CalendarReference: ask the Device dispatcher to read the referenced
       object's Present_Value, since this module has no direct dependency
       on the Calendar object */
    if (!Read_Property_Internal_Callback) {
        return false;
    }
    rp_data.object_type = event->period.calendarReference.type;
    rp_data.object_instance = event->period.calendarReference.instance;
    rp_data.object_property = PROP_PRESENT_VALUE;
    rp_data.array_index = BACNET_ARRAY_ALL;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    rp_data.error_class = ERROR_CLASS_PROPERTY;
    rp_data.error_code = ERROR_CODE_UNKNOWN_PROPERTY;
    apdu_len = Read_Property_Internal_Callback(&rp_data);
    if (apdu_len <= 0) {
        return false;
    }
    if (bacapp_decode_application_data(apdu, (uint32_t)apdu_len, &value) <= 0) {
        return false;
    }

    return (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) && value.type.Boolean;
}
#endif

/**
 * @brief Determine if a member reference targets an object on this device.
 * @param pMember [in] Member reference to check.
 * @return true if deviceIdentifier is absent, or names this device.
 */
static bool Schedule_Member_Target_Is_Local(
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    if (!pMember) {
        return false;
    }
    if (pMember->deviceIdentifier.type != OBJECT_DEVICE) {
        /* deviceIdentifier not provided - refers to an object in this
           Device, per 135-2024 clause 21 BACnetDeviceObjectPropertyReference
         */
        return true;
    }
    if (pMember->deviceIdentifier.instance == Device_Object_Instance_Number()) {
        return true;
    }

    return false;
}

/**
 * @brief Write the current Present_Value to every member of
 *  List_Of_Object_Property_References, using the Priority_For_Writing
 *  property. Per 135-2024 12.24.11: "An error writing to any member of
 *  the list shall not stop the Schedule object from writing to the
 *  remaining members."
 * @param object_instance - object-instance number of the object
 * @param pObject - object instance data
 */
static void
Schedule_Write_Members(uint32_t object_instance, struct object_data *pObject)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member = { 0 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    unsigned i, count;

    if (!pObject || !Write_Property_Internal_Callback) {
        return;
    }
    if (pObject->Writeback_Active) {
        /* prevent recursive writeback (e.g. a member reference that
           loops back into this Schedule object) */
        return;
    }
    pObject->Writeback_Active = true;
    count = Schedule_List_Of_Object_Property_References_Count(object_instance);
    for (i = 0; i < count; i++) {
        if (!Schedule_List_Of_Object_Property_References(
                object_instance, i, &member)) {
            continue;
        }
        if (!Schedule_Member_Target_Is_Local(&member)) {
            /* external device references need a network WriteProperty,
               which Write_Property_Internal_Callback does not provide */
            continue;
        }
        wp_data.object_type = member.objectIdentifier.type;
        wp_data.object_instance = member.objectIdentifier.instance;
        wp_data.object_property = member.propertyIdentifier;
        wp_data.array_index = (BACNET_ARRAY_INDEX)member.arrayIndex;
        wp_data.priority = pObject->Priority_For_Writing;
        wp_data.error_class = ERROR_CLASS_PROPERTY;
        wp_data.error_code = ERROR_CODE_SUCCESS;
        wp_data.application_data_len = bacapp_encode_application_data(
            wp_data.application_data, &pObject->Present_Value);
        /* ignore individual failures - continue with the remaining
           members of the list */
        (void)Write_Property_Internal_Callback(&wp_data);
    }
    pObject->Writeback_Active = false;
}

/**
 * @brief Write to List_Of_Object_Property_References members when required
 *  by 135-2024 12.24.4/12.24.25: whenever the Present_Value changes, or
 *  once when a new scheduled action activates and Write_Every_Scheduled_Action
 *  is TRUE
 * @param object_instance - object-instance number of the object
 * @param pObject - object instance data
 * @param old_value - Present_Value prior to recalculation
 * @param action_changed - true if a different scheduled time-value is active
 *  than at the previous evaluation
 */
static void Schedule_Present_Value_Notify(
    uint32_t object_instance,
    struct object_data *pObject,
    const BACNET_APPLICATION_DATA_VALUE *old_value,
    bool action_changed)
{
    if (!pObject) {
        return;
    }
    if ((pObject->Write_Every_Scheduled_Action && action_changed) ||
        !bacapp_same_value(old_value, &pObject->Present_Value)) {
        Schedule_Write_Members(object_instance, pObject);
    }
}

/**
 * @brief Recalculate the Present Value of the Schedule object
 * @param object_instance - object-instance number of the object
 * @param wday - day of the week
 * @param time - time of the day
 */
void Schedule_Recalculate_PV(
    uint32_t object_instance, BACNET_WEEKDAY wday, const BACNET_TIME *time)
{
    struct object_data *pObject;
    const BACNET_TIME_VALUE *pCurrent;
    const BACNET_TIME_VALUE *pActive = NULL;
    bool action_changed;
    BACNET_APPLICATION_DATA_VALUE old_value;

    pObject = Object_Data(object_instance);
    if (!pObject || !time || (wday < 1) || (wday > 7)) {
        return;
    }
    old_value = pObject->Present_Value;
    pObject->Present_Value.tag = BACNET_APPLICATION_TAG_NULL;

    /* for future development, here should be the loop for Exception Schedule
     */

    /*  Note to developers: please ping Edward at info@connect-ex.com
        for a more complete schedule object implementation. */
    pCurrent = Schedule_Weekly_Day_Current_Time_Value(
        pObject->Weekly_Schedule[wday - 1].Time_Values, time);
    if (pCurrent && (pCurrent->Value.tag != BACNET_APPLICATION_TAG_NULL)) {
        pActive = pCurrent;
        bacnet_primitive_to_application_data_value(
            &pObject->Present_Value, &pCurrent->Value);
    } else {
        memcpy(
            &pObject->Present_Value, &pObject->Schedule_Default,
            sizeof(pObject->Present_Value));
    }
    /* a scheduled action occurs only when a new time-value becomes active */
    action_changed = (pActive != pObject->Active_Time_Value);
    pObject->Active_Time_Value = pActive;
    Schedule_Present_Value_Notify(
        object_instance, pObject, &old_value, action_changed);
}

/**
 * @brief Recalculate the Present Value of the Schedule object, taking the
 *  Exception_Schedule into account per 135-2024 12.24.4
 * @param object_instance - object-instance number of the object
 * @param date - current date (used for Exception_Schedule matching and to
 *  select the Weekly_Schedule day via date->wday)
 * @param time - current time of day
 */
void Schedule_Calendar_Present_Value_Update(
    uint32_t object_instance, const BACNET_DATE *date, const BACNET_TIME *time)
{
    struct object_data *pObject;
    const BACNET_TIME_VALUE *pFound = NULL;
    const BACNET_TIME_VALUE *pCandidate;
    bool action_changed;
    BACNET_APPLICATION_DATA_VALUE old_value;
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    const struct special_event_data *event;
    unsigned i, count;
    int best_priority = -1;
#endif

    pObject = Object_Data(object_instance);
    if (!pObject || !date || !time || (date->wday < 1) || (date->wday > 7)) {
        return;
    }
    old_value = pObject->Present_Value;
    pObject->Present_Value.tag = BACNET_APPLICATION_TAG_NULL;

    /* 135-2024 12.24.11: only Weekly_Schedule/Exception_Schedule entries
       within the Effective_Period are considered; outside of it, the
       object falls back to Schedule_Default like any other day with no
       matching entry, rather than freezing the last in-period value */
    if (Schedule_In_Effective_Period(object_instance, date)) {
#if BACNET_EXCEPTION_SCHEDULE_SIZE
        count = (unsigned)Keylist_Count(pObject->Exception_Schedule);
        for (i = 0; i < count; i++) {
            event = Keylist_Data_Index(pObject->Exception_Schedule, (int)i);
            if (!event || !Schedule_Special_Event_In_Effect(event, date)) {
                continue;
            }
            pCandidate = Schedule_Weekly_Day_Current_Time_Value(
                event->Time_Values, time);
            if (!pCandidate ||
                (pCandidate->Value.tag == BACNET_APPLICATION_TAG_NULL)) {
                continue;
            }
            if ((best_priority < 0) ||
                (event->priority < (unsigned)best_priority)) {
                best_priority = event->priority;
                pFound = pCandidate;
            }
        }
#endif
        if (!pFound) {
            pCandidate = Schedule_Weekly_Day_Current_Time_Value(
                pObject->Weekly_Schedule[date->wday - 1].Time_Values, time);
            if (pCandidate &&
                (pCandidate->Value.tag != BACNET_APPLICATION_TAG_NULL)) {
                pFound = pCandidate;
            }
        }
    }
    if (pFound) {
        bacnet_primitive_to_application_data_value(
            &pObject->Present_Value, &pFound->Value);
    } else {
        memcpy(
            &pObject->Present_Value, &pObject->Schedule_Default,
            sizeof(pObject->Present_Value));
    }
    /* a scheduled action occurs only when a new time-value becomes active */
    action_changed = (pFound != pObject->Active_Time_Value);
    pObject->Active_Time_Value = pFound;
    Schedule_Present_Value_Notify(
        object_instance, pObject, &old_value, action_changed);
}

/**
 * @brief Updates the Present Value of the Schedule object
 * @param  object_instance - object-instance number of the object
 * @param milliseconds - Unused parameter
 */
void Schedule_Timer(uint32_t object_instance, uint16_t milliseconds)
{
    struct object_data *pObject;
    BACNET_DATE bdate;
    BACNET_TIME btime;

    UNUSED(milliseconds);
    pObject = Object_Data(object_instance);
    if (pObject) {
        datetime_local(&bdate, &btime, NULL, NULL);
        /* always recalculate: Schedule_Calendar_Present_Value_Update()
           applies Schedule_Default outside the Effective_Period, per
           135-2024 12.24.11 */
        Schedule_Calendar_Present_Value_Update(object_instance, &bdate, &btime);
    }
}
