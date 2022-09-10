/**************************************************************************
 *
 * Copyright (C) 2015 Nikola Jelic <nikola.jelic@euroicc.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *********************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bactext.h"
#include "bacnet/config.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/proplist.h"
#include "bacnet/timestamp.h"
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

static const int Schedule_Properties_Optional[] = { PROP_WEEKLY_SCHEDULE, -1 };

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

void Schedule_Init(void)
{
    unsigned i, j;

    SCHEDULE_DESCR *psched = &Schedule_Descr[0];

    for (i = 0; i < MAX_SCHEDULES; i++, psched++) {
        /* whole year, change as necessary */
        psched->Start_Date.year = 0xFF;
        psched->Start_Date.month = 1;
        psched->Start_Date.day = 1;
        psched->Start_Date.wday = 0xFF;
        psched->End_Date.year = 0xFF;
        psched->End_Date.month = 12;
        psched->End_Date.day = 31;
        psched->End_Date.wday = 0xFF;
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
    }
}

bool Schedule_Valid_Instance(uint32_t object_instance)
{
    unsigned int index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES) {
        return true;
    } else {
        return false;
    }
}

unsigned Schedule_Count(void)
{
    return MAX_SCHEDULES;
}

uint32_t Schedule_Index_To_Instance(unsigned index)
{
    return index;
}

unsigned Schedule_Instance_To_Index(uint32_t instance)
{
    unsigned index = MAX_SCHEDULES;

    if (instance < MAX_SCHEDULES) {
        index = instance;
    }

    return index;
}

bool Schedule_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = ""; /* okay for single thread */
    unsigned int index;
    bool status = false;

    index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES) {
        sprintf(text_string, "SCHEDULE %lu", (unsigned long)index);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}

/* 	BACnet Testing Observed Incident oi00106
        Out of service was not supported by Schedule object
        Revealed by BACnet Test Client v1.8.16 (
   www.bac-test.com/bacnet-test-client-download ) BITS: BIT00032 Any discussions
   can be directed to edward@bac-test.com Please feel free to remove this
   comment when my changes accepted after suitable time for review by all
   interested parties. Say 6 months -> September 2016 */
void Schedule_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    unsigned index = 0;

    index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES) {
        Schedule_Descr[index].Out_Of_Service = value;
    }
}

int Schedule_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    unsigned object_index = 0;
    SCHEDULE_DESCR *CurrentSC;
    uint8_t *apdu = NULL;
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
            /* 	BACnet Testing Observed Incident oi00110
                    Effective Period of Schedule object not correctly formatted
                    Revealed by BACnet Test Client v1.8.16 (
               www.bac-test.com/bacnet-test-client-download ) BITS: BIT00031 Any
               discussions can be directed to edward@bac-test.com Please feel
               free to remove this comment when my changes accepted after
               suitable time for
                    review by all interested parties. Say 6 months -> September
               2016 */
            apdu_len =
                encode_application_date(&apdu[0], &CurrentSC->Start_Date);
            apdu_len +=
                encode_application_date(&apdu[apdu_len], &CurrentSC->End_Date);
            break;
        case PROP_WEEKLY_SCHEDULE:
            if (rpdata->array_index == 0) { /* count, always 7 */
                apdu_len = encode_application_unsigned(&apdu[0], 7);
            } else if (rpdata->array_index ==
                BACNET_ARRAY_ALL) { /* full array */
                int day;
                for (day = 0; day < 7; day++) {
                    apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                    for (i = 0; i < CurrentSC->Weekly_Schedule[day].TV_Count;
                         i++) {
                        apdu_len += bacnet_time_value_encode(&apdu[apdu_len],
                            &CurrentSC->Weekly_Schedule[day].Time_Values[i]);
                    }
                    apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
                }
            } else if (rpdata->array_index <= 7) { /* some array element */
                int day = rpdata->array_index - 1;
                apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                for (i = 0; i < CurrentSC->Weekly_Schedule[day].TV_Count; i++) {
                    apdu_len += bacnet_time_value_encode(&apdu[apdu_len],
                        &CurrentSC->Weekly_Schedule[day].Time_Values[i]);
                }
                apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
            } else { /* out of bounds */
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
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
            /* 	BACnet Testing Observed Incident oi00106
                    Out of service was not supported by Schedule object
                    Revealed by BACnet Test Client v1.8.16 (
               www.bac-test.com/bacnet-test-client-download ) BITS: BIT00032 Any
               discussions can be directed to edward@bac-test.com Please feel
               free to remove this comment when my changes accepted after
               suitable time for
                    review by all interested parties. Say 6 months -> September
               2016 */
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
    /* Ed->Steve, I know that initializing stack values used to be 'safer', but
       warnings in latest compilers indicate when uninitialized values are being
       used, and I think that the warnings are more useful to reveal bad code
       flow than the "safety: of pre-intializing variables. Please give this
       some thought let me know if you agree we should start to remove
       initializations */
    unsigned object_index;
    bool status = false; /* return value */
    int len;
    BACNET_APPLICATION_DATA_VALUE value;

    /* 	BACnet Testing Observed Incident oi00106
            Out of service was not supported by Schedule object
            Revealed by BACnet Test Client v1.8.16 (
       www.bac-test.com/bacnet-test-client-download ) BITS: BIT00032 Any
       discussions can be directed to edward@bac-test.com Please feel free to
       remove this comment when my changes accepted after suitable time for
            review by all interested parties. Say 6 months -> September 2016 */
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
            /* 	BACnet Testing Observed Incident oi00106
                    Out of service was not supported by Schedule object
                    Revealed by BACnet Test Client v1.8.16 (
               www.bac-test.com/bacnet-test-client-download ) BITS: BIT00032 Any
               discussions can be directed to edward@bac-test.com Please feel
               free to remove this comment when my changes accepted after
               suitable time for
                    review by all interested parties. Say 6 months -> September
               2016 */
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Schedule_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;

        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_PRESENT_VALUE:
        case PROP_EFFECTIVE_PERIOD:
        case PROP_WEEKLY_SCHEDULE:
        case PROP_SCHEDULE_DEFAULT:
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
        case PROP_PRIORITY_FOR_WRITING:
        case PROP_STATUS_FLAGS:
        case PROP_RELIABILITY:
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

void Schedule_Recalculate_PV(
    SCHEDULE_DESCR *desc, BACNET_WEEKDAY wday, BACNET_TIME *time)
{
    int i;
    desc->Present_Value.tag = BACNET_APPLICATION_TAG_NULL;

    /* for future development, here should be the loop for Exception Schedule */

    /* Just a note to developers: We have a paying customer who has asked us to
       fully implement the Schedule Object. In good spirit, they have agreed to
       allow us to release the code we develop back to the Open Source community
       after a 6-12 month waiting period. However, if you are about to work on
       this yourself, please ping us at info@connect-ex.com, we may be able to
       broker an early release on a case-by-case basis. */

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
