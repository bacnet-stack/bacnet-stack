/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @brief A basic BACnet Schedule object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/proplist.h"
#include "bacnet/timestamp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/object/schedule.h"

#ifndef MAX_SCHEDULES
#define MAX_SCHEDULES 4
#endif

static SCHEDULE_DESCR Schedule_Descr[MAX_SCHEDULES];

static const int Schedule_Properties_Required[] = {
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

static const int Schedule_Properties_Optional[] = {
    /* list of optional properties */
    PROP_WEEKLY_SCHEDULE,
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    PROP_EXCEPTION_SCHEDULE,
#endif
    -1
};

static const int Schedule_Properties_Proprietary[] = { -1 };

void Schedule_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
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
 * @brief Gets an object from the list using an instance number
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
SCHEDULE_DESCR *Schedule_Object(uint32_t object_instance)
{
    unsigned int object_index;
    SCHEDULE_DESCR *pObject = NULL;

    object_index = Schedule_Instance_To_Index(object_instance);
    if (object_index < MAX_SCHEDULES) {
        pObject = &Schedule_Descr[object_index];
    }

    return pObject;
}

/**
 * @brief Initialize the Schedule object data
 */
void Schedule_Init(void)
{
    unsigned i, j;
    BACNET_DATE start_date = { 0 }, end_date = { 0 };
    SCHEDULE_DESCR *psched;
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    unsigned e;
    BACNET_SPECIAL_EVENT *event;
#endif

    /* whole year, change as necessary */
    datetime_set_date(&start_date, 0, 1, 1);
    datetime_wildcard_year_set(&start_date);
    datetime_wildcard_weekday_set(&start_date);
    datetime_set_date(&end_date, 0, 12, 31);
    datetime_wildcard_year_set(&end_date);
    datetime_wildcard_weekday_set(&end_date);
    for (i = 0; i < MAX_SCHEDULES; i++, psched++) {
        psched = &Schedule_Descr[i];
        datetime_copy_date(&psched->Start_Date, &start_date);
        datetime_copy_date(&psched->End_Date, &end_date);
        for (j = 0; j < BACNET_WEEKLY_SCHEDULE_SIZE; j++) {
            psched->Weekly_Schedule[j].TV_Count = 0;
        }
        memcpy(
            &psched->Present_Value, &psched->Schedule_Default,
            sizeof(psched->Present_Value));
        psched->Schedule_Default.context_specific = false;
        psched->Schedule_Default.tag = BACNET_APPLICATION_TAG_REAL;
        psched->Schedule_Default.type.Real = 21.0f; /* 21 C, room temperature */
        psched->obj_prop_ref_cnt = 0; /* no references, add as needed */
        psched->Priority_For_Writing = 16; /* lowest priority */
        psched->Out_Of_Service = false;
#if BACNET_EXCEPTION_SCHEDULE_SIZE
        for (e = 0; e < BACNET_EXCEPTION_SCHEDULE_SIZE; e++) {
            event = &psched->Exception_Schedule[e];
            event->periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
            event->period.calendarEntry.tag = BACNET_CALENDAR_DATE_RANGE;
            datetime_copy_date(
                &event->period.calendarEntry.type.DateRange.startdate,
                &start_date);
            datetime_copy_date(
                &event->period.calendarEntry.type.DateRange.enddate, &end_date);
            event->period.calendarEntry.next = NULL;
            event->timeValues.TV_Count = 0;
            event->priority = 16;
        }
#endif
    }
}

/**
 * @brief Determines if a given instance is valid
 * @param  object_instance - object-instance number of the object
 * @return true if the instance is valid, and false if not
 */
bool Schedule_Valid_Instance(uint32_t object_instance)
{
    unsigned int index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Determines the number of Schedule objects
 * @return Number of Schedule objects
 */
unsigned Schedule_Count(void)
{
    return MAX_SCHEDULES;
}

/**
 * @brief Determines the object instance number for a given index
 * @param  index - index number of the object
 * @return object instance number for the given index, or MAX_SCHEDULES if the
 * index is not valid
 */
uint32_t Schedule_Index_To_Instance(unsigned index)
{
    return index;
}

/**
 * @brief Determines the index for a given object instance number
 * @param  instance - object-instance number of the object
 * @return index number for the given object instance number, or MAX_SCHEDULES
 * if the instance is not valid
 */
unsigned Schedule_Instance_To_Index(uint32_t instance)
{
    unsigned index = MAX_SCHEDULES;

    if (instance < MAX_SCHEDULES) {
        index = instance;
    }

    return index;
}

/**
 * @brief Determines the object name for a given object instance number
 * @param  object_instance - object-instance number of the object
 * @param  object_name - object name of the object
 * @return true if the object name is valid, and false if not
 */
bool Schedule_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    unsigned int index;
    bool status = false;

    index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES) {
        snprintf(
            text, sizeof(text), "SCHEDULE %lu", (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
}

/**
 * @brief Sets a specificSchedule object out of service
 * @param object_instance - object-instance number of the object
 * @param value - true if out of service, and false if not
 */
void Schedule_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    unsigned index = 0;

    index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES) {
        Schedule_Descr[index].Out_Of_Service = value;
    }
}

/**
 * @brief Get the Weekly Schedule for a given object instance
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Weekly Schedule to get 0 to 6
 * @return pointer to the Weekly Schedule, or NULL if not found
 */
BACNET_DAILY_SCHEDULE *
Schedule_Weekly_Schedule(uint32_t object_instance, unsigned array_index)
{
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject && (array_index < BACNET_WEEKLY_SCHEDULE_SIZE)) {
        return &pObject->Weekly_Schedule[array_index];
    }

    return NULL;
}

/**
 * @brief Set the Weekly Schedule for a given object instance
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Weekly Schedule to set 0 to 6
 * @param value - pointer to the Weekly Schedule to set
 * @return true if the Weekly Schedule was set, and false if not
 */
bool Schedule_Weekly_Schedule_Set(
    uint32_t object_instance,
    unsigned array_index,
    const BACNET_DAILY_SCHEDULE *value)
{
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject && (array_index < BACNET_WEEKLY_SCHEDULE_SIZE)) {
        memcpy(
            &pObject->Weekly_Schedule[array_index], value,
            sizeof(BACNET_WEEKLY_SCHEDULE));
        return true;
    }

    return false;
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
static int Schedule_Weekly_Schedule_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len;
    SCHEDULE_DESCR *pObject;

    if (array_index >= BACNET_WEEKLY_SCHEDULE_SIZE) {
        return BACNET_STATUS_ERROR;
    }
    pObject = Schedule_Object(object_instance);
    if (!pObject) {
        return BACNET_STATUS_ERROR;
    }

    apdu_len = bacnet_dailyschedule_context_encode(
        apdu, 0, &pObject->Weekly_Schedule[array_index]);

    return apdu_len;
}

#if BACNET_EXCEPTION_SCHEDULE_SIZE
/**
 * @brief Get the Exception Schedule for a given object instance
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Exception Schedule to get 0 to 6
 * @return pointer to the Exception Schedule BACnetSpecialEvent,
 *  or NULL if not found
 */
BACNET_SPECIAL_EVENT *
Schedule_Exception_Schedule(uint32_t object_instance, unsigned array_index)
{
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject && (array_index < BACNET_EXCEPTION_SCHEDULE_SIZE)) {
        return &pObject->Exception_Schedule[array_index];
    }

    return NULL;
}

/**
 * @brief Set the Exception Schedule for a given object instance
 * @param object_instance - object-instance number of the object
 * @param array_index - index of the Exception Schedule to set 0 to 6
 * @param value - pointer to the Weekly Schedule BACnetSpecialEvent to set
 * @return true if the Exception Schedule BACnetSpecialEvent was set,
 *  and false if not
 */
bool Schedule_Exception_Schedule_Set(
    uint32_t object_instance,
    unsigned array_index,
    const BACNET_SPECIAL_EVENT *value)
{
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject && (array_index < BACNET_EXCEPTION_SCHEDULE_SIZE)) {
        memcpy(
            &pObject->Exception_Schedule[array_index], value,
            sizeof(BACNET_SPECIAL_EVENT));
        return true;
    }

    return false;
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
static int Schedule_Exception_Schedule_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len;
    SCHEDULE_DESCR *pObject;

    if (array_index >= BACNET_EXCEPTION_SCHEDULE_SIZE) {
        return BACNET_STATUS_ERROR;
    }
    pObject = Schedule_Object(object_instance);
    if (!pObject) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len = bacnet_special_event_encode(
        apdu, &pObject->Exception_Schedule[array_index]);

    return apdu_len;
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
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
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
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject) {
        datetime_copy_date(start_date, &pObject->Start_Date);
        datetime_copy_date(end_date, &pObject->End_Date);
        return true;
    }

    return false;
}

/**
 * @brief Set a member element of a given BACnetLIST object property
 * @param pObject - object in which to set the value
 * @param index - 0-based array index
 * @param pMember - pointer to member value
 * @return true if set, false if not set
 */
static bool List_Of_Object_Property_References_Set(
    SCHEDULE_DESCR *pObject,
    unsigned index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    bool status = false;
    if (pObject && (index < BACNET_SCHEDULE_OBJ_PROP_REF_SIZE)) {
        if (pMember) {
            status = bacnet_device_object_property_reference_copy(
                &pObject->Object_Property_References[index], pMember);
        }
    }

    return status;
}

/**
 * @brief Set a member element of a given BACnetLIST object property
 * @param pObject - object in which to set the value
 * @param index - 0-based array index
 * @param pMember - pointer to member value
 * @return true if set, false if not set
 */
bool Schedule_List_Of_Object_Property_References_Set(
    uint32_t object_instance,
    unsigned index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    bool status = false;
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    status = List_Of_Object_Property_References_Set(pObject, index, pMember);

    return status;
}

/**
 * @brief Set a member element of a given BACnetLIST object property
 * @param pObject - object in which to set the value
 * @param index - 0-based array index
 * @param pMember - pointer to member value
 * @return true if set, false if not set
 */
bool Schedule_List_Of_Object_Property_References(
    uint32_t object_instance,
    unsigned index,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    bool status = false;
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject && (index < BACNET_SCHEDULE_OBJ_PROP_REF_SIZE)) {
        if (pMember) {
            status = bacnet_device_object_property_reference_copy(
                pMember, &pObject->Object_Property_References[index]);
        }
    }

    return status;
}

/**
 * @brief Get the size of the list of object property references
 * @param object_instance [in] BACnet network port object instance number
 * @return The size of the list of object property references
 */
size_t
Schedule_List_Of_Object_Property_References_Capacity(uint32_t object_instance)
{
    (void)object_instance; /* unused */
    return BACNET_SCHEDULE_OBJ_PROP_REF_SIZE;
}

/**
 * @brief Read a property from the Schedule object
 * @param rpdata [in] pointer to the read property data structure
 * @return The length of the apdu encoded or BACNET_STATUS_ERROR
 */
int Schedule_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    unsigned object_index = 0;
    SCHEDULE_DESCR *CurrentSC;
    uint8_t *apdu = NULL;
    uint16_t apdu_max = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    int i, imax = 0;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    object_index = Schedule_Instance_To_Index(rpdata->object_instance);
    if (object_index < MAX_SCHEDULES) {
        CurrentSC = &Schedule_Descr[object_index];
    } else {
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
                BACNET_EXCEPTION_SCHEDULE_SIZE, apdu, apdu_max);
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
            imax = min(
                CurrentSC->obj_prop_ref_cnt, BACNET_SCHEDULE_OBJ_PROP_REF_SIZE);
            for (i = 0; i < imax; i++) {
                apdu_len += bacapp_encode_device_obj_property_ref(
                    &apdu[apdu_len], &CurrentSC->Object_Property_References[i]);
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
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
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
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Schedule_Weekly_Schedule_Element_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_DAILY_SCHEDULE daily_schedule = { 0 };
    size_t tv, tv_size;
    int len = 0;
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject) {
        if (array_index == 0) {
            error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        } else if (array_index <= BACNET_WEEKLY_SCHEDULE_SIZE) {
            array_index--;
            len = bacnet_dailyschedule_context_decode(
                application_data, application_data_len, 0, &daily_schedule);
            if (len > 0) {
                tv_size =
                    min(daily_schedule.TV_Count,
                        BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE);
                for (tv = 0; tv < tv_size; tv++) {
                    /* copy the time value */
                    memcpy(
                        &pObject->Weekly_Schedule[array_index].Time_Values[tv],
                        &daily_schedule.Time_Values[tv],
                        sizeof(BACNET_TIME_VALUE));
                }
                pObject->Weekly_Schedule[array_index].TV_Count = tv_size;
                error_code = ERROR_CODE_SUCCESS;
            } else {
                error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
        } else {
            error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
        }
    }

    return error_code;
}

/**
 * @brief Decode one BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Schedule_Weekly_Schedule_Element_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    BACNET_DAILY_SCHEDULE daily_schedule = { 0 };
    int len = 0;
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject) {
        len = bacnet_dailyschedule_context_decode(
            apdu, apdu_size, 0, &daily_schedule);
    }

    return len;
}

#if BACNET_EXCEPTION_SCHEDULE_SIZE
/**
 * @brief Write a value to a BACnetARRAY property element value
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Schedule_Exception_Schedule_Element_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_SPECIAL_EVENT special_event = { 0 };
    int len = 0;
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject) {
        if (array_index == 0) {
            error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        } else if (array_index <= BACNET_WEEKLY_SCHEDULE_SIZE) {
            array_index--;
            len = bacnet_special_event_decode(
                application_data, application_data_len, &special_event);
            if (len > 0) {
                bacnet_special_event_copy(
                    &pObject->Exception_Schedule[array_index], &special_event);
                error_code = ERROR_CODE_SUCCESS;
            } else {
                error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
        } else {
            error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
        }
    }

    return error_code;
}

/**
 * @brief Decode one BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Schedule_Exception_Schedule_Element_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    BACNET_SPECIAL_EVENT special_event = { 0 };
    int len = 0;
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject) {
        len = bacnet_special_event_decode(apdu, apdu_size, &special_event);
    }

    return len;
}
#endif

/**
 * @brief Write a value to a BACnetLIST property element value
 *  using a BACnetARRAY write utility function
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Schedule_List_Of_Object_Property_References_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    int len = 0;
    bool status;
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject) {
        if (array_index == 0) {
            error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        } else if (array_index <= BACNET_SCHEDULE_OBJ_PROP_REF_SIZE) {
            len = bacapp_decode_known_property(
                application_data, application_data_len, &value, OBJECT_SCHEDULE,
                PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES);
            if (len > 0) {
                if (value.tag ==
                    BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE) {
                    status = List_Of_Object_Property_References_Set(
                        pObject, array_index - 1,
                        &value.type.Device_Object_Property_Reference);
                    if (status) {
                        pObject->obj_prop_ref_cnt = array_index;
                        error_code = ERROR_CODE_SUCCESS;
                    } else {
                        error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                } else {
                    error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
            } else {
                error_code = ERROR_CODE_ABORT_OTHER;
            }
        } else {
            error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
        }
    }

    return error_code;
}

/**
 * @brief Decode a BACnetLIST property element to determine the element length
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Schedule_List_Of_Object_Property_References_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    int len = 0;
    SCHEDULE_DESCR *pObject;

    pObject = Schedule_Object(object_instance);
    if (pObject) {
        len = bacapp_decode_known_property(
            apdu, apdu_size, &value, OBJECT_SCHEDULE,
            PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES);
    }

    return len;
}

/**
 * @brief Write a property to the Schedule object
 * @param wp_data - pointer to the write property data
 * @return true if the write was successful, and false if not
 */
bool Schedule_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    unsigned object_index;
    bool status = false; /* return value */
    int len;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    /* decode the some of the request */
    len = bacapp_decode_known_array_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        wp_data->object_type, wp_data->object_property, wp_data->array_index);
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    object_index = Schedule_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_SCHEDULES) {
        return false;
    }
    switch ((int)wp_data->object_property) {
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Schedule_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
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
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                Schedule_List_Of_Object_Property_References_Length,
                Schedule_List_Of_Object_Property_References_Write,
                BACNET_SCHEDULE_OBJ_PROP_REF_SIZE, wp_data->application_data,
                wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
        case PROP_EFFECTIVE_PERIOD:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_DATERANGE);
            if (status) {
                /* set the start and end date */
                datetime_copy_date(
                    &Schedule_Descr[object_index].Start_Date,
                    &value.type.Date_Range.startdate);
                datetime_copy_date(
                    &Schedule_Descr[object_index].End_Date,
                    &value.type.Date_Range.enddate);
            }
            break;
#if BACNET_EXCEPTION_SCHEDULE_SIZE
        case PROP_EXCEPTION_SCHEDULE:
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                Schedule_Exception_Schedule_Element_Length,
                Schedule_Exception_Schedule_Element_Write,
                BACNET_EXCEPTION_SCHEDULE_SIZE, wp_data->application_data,
                wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
#endif
        default:
            if (property_lists_member(
                    Schedule_Properties_Required, Schedule_Properties_Optional,
                    Schedule_Properties_Proprietary,
                    wp_data->object_property)) {
                debug_printf(
                    "Schedule_Write_Property: %s\n",
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
 * @brief Determine if the given calendar entry is within the effective period
 * @param desc - schedule descriptor
 * @param date - date to check
 * @return true if the calendar entry is within the effective period
 */
bool Schedule_In_Effective_Period(
    const SCHEDULE_DESCR *desc, const BACNET_DATE *date)
{
    bool res = false;

    if (desc && date) {
        if (datetime_wildcard_compare_date(&desc->Start_Date, date) <= 0 &&
            datetime_wildcard_compare_date(&desc->End_Date, date) >= 0) {
            res = true;
        }
    }

    return res;
}

/**
 * @brief Recalculate the Present Value of the Schedule object
 * @param desc - schedule descriptor
 * @param wday - day of the week
 * @param time - time of the day
 */
void Schedule_Recalculate_PV(
    SCHEDULE_DESCR *desc, BACNET_WEEKDAY wday, const BACNET_TIME *time)
{
    int i;
    desc->Present_Value.tag = BACNET_APPLICATION_TAG_NULL;

    /* for future development, here should be the loop for Exception Schedule */

    /*  Note to developers: please ping Edward at info@connect-ex.com
        for a more complete schedule object implementation. */
    for (i = 0; i < desc->Weekly_Schedule[wday - 1].TV_Count &&
         desc->Present_Value.tag == BACNET_APPLICATION_TAG_NULL;
         i++) {
        int diff = datetime_wildcard_compare_time(
            time, &desc->Weekly_Schedule[wday - 1].Time_Values[i].Time);
        if (diff >= 0 &&
            desc->Weekly_Schedule[wday - 1].Time_Values[i].Value.tag !=
                BACNET_APPLICATION_TAG_NULL) {
            bacnet_primitive_to_application_data_value(
                &desc->Present_Value,
                &desc->Weekly_Schedule[wday - 1].Time_Values[i].Value);
        }
    }

    if (desc->Present_Value.tag == BACNET_APPLICATION_TAG_NULL) {
        memcpy(
            &desc->Present_Value, &desc->Schedule_Default,
            sizeof(desc->Present_Value));
    }
}
