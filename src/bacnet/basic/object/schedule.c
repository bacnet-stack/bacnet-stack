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
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/schedule.h"

#ifndef MAX_SCHEDULES
#define MAX_SCHEDULES 4
#endif

static SCHEDULE_DESCR Schedule_Descr[MAX_SCHEDULES];

static const int Schedule_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE,
    PROP_EFFECTIVE_PERIOD, PROP_SCHEDULE_DEFAULT,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES, PROP_PRIORITY_FOR_WRITING,
    PROP_STATUS_FLAGS, PROP_RELIABILITY, PROP_OUT_OF_SERVICE, -1 };

static const int Schedule_Properties_Optional[] = { PROP_WEEKLY_SCHEDULE,
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    PROP_EXCEPTION_SCHEDULE,
#endif
    -1 };

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
static SCHEDULE_DESCR *Schedule_Object(uint32_t object_instance)
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
    unsigned i, j, e;
    BACNET_DATE start_date = { 0 }, end_date = { 0 };
    BACNET_SPECIAL_EVENT *event;
    SCHEDULE_DESCR *psched;

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
        for (j = 0; j < 7; j++) {
            psched->Weekly_Schedule[j].TV_Count = 0;
        }
        memcpy(&psched->Present_Value, &psched->Schedule_Default,
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
                &event->period.calendarEntry.type.DateRange.enddate,
                &end_date);
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
    static char text[32] = ""; /* okay for single thread */
    unsigned int index;
    bool status = false;

    index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES) {
        snprintf(text, sizeof(text), "SCHEDULE %lu", (unsigned long)object_instance);
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
    int apdu_len = 0, len = 0;
    SCHEDULE_DESCR *pObject;
    int day, i;

    if (array_index >= 7) {
        return BACNET_STATUS_ERROR;
    }
    pObject = Schedule_Object(object_instance);
    if (!pObject) {      
        return BACNET_STATUS_ERROR;
    }
    day = array_index;
    len = encode_opening_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    for (i = 0; i < pObject->Weekly_Schedule[day].TV_Count; i++) {
        len = bacnet_time_value_encode(apdu,
            &pObject->Weekly_Schedule[day].Time_Values[i]);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    len = encode_closing_tag(apdu, 0);
    apdu_len += len;

    return apdu_len;
}

#if BACNET_EXCEPTION_SCHEDULE_SIZE
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
    apdu_len = bacnet_special_event_encode(apdu, 
        &pObject->Exception_Schedule[array_index]);

    return apdu_len;
}
#endif

int Schedule_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    unsigned object_index = 0;
    SCHEDULE_DESCR *CurrentSC;
    uint8_t *apdu = NULL;
    uint16_t apdu_max = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    int i;

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
                Schedule_Weekly_Schedule_Encode, 7, apdu, apdu_max);
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
            for (i = 0; i < CurrentSC->obj_prop_ref_cnt; i++) {
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

    if ((apdu_len >= 0) && (rpdata->object_property != PROP_WEEKLY_SCHEDULE) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

bool Schedule_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    unsigned object_index;
    bool status = false; /* return value */
    int len;
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
        default:
            if (property_lists_member(
                Schedule_Properties_Required, Schedule_Properties_Optional, 
                Schedule_Properties_Proprietary, wp_data->object_property)) {
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
bool Schedule_In_Effective_Period(SCHEDULE_DESCR *desc, BACNET_DATE *date)
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
    SCHEDULE_DESCR *desc, BACNET_WEEKDAY wday, BACNET_TIME *time)
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
            bacnet_primitive_to_application_data_value(&desc->Present_Value,
                &desc->Weekly_Schedule[wday - 1].Time_Values[i].Value);
        }
    }

    if (desc->Present_Value.tag == BACNET_APPLICATION_TAG_NULL) {
        memcpy(&desc->Present_Value, &desc->Schedule_Default,
            sizeof(desc->Present_Value));
    }
}
