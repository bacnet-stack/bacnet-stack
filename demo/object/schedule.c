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

#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bactext.h"
#include "config.h"
#include "device.h"
#include "handlers.h"
#include "proplist.h"
#include "timestamp.h"
#include "schedule.h"

#ifndef MAX_SCHEDULES
#define MAX_SCHEDULES 4
#endif

SCHEDULE_DESCR Schedule_Descr[MAX_SCHEDULES];

static const int Schedule_Properties_Required[] = {
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
    PROP_WEEKLY_SCHEDULE,
    -1
};

static const int Schedule_Properties_Proprietary[] = {
    -1
};

void Schedule_Property_Lists(const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Schedule_Properties_Required;
    if (pOptional)
        *pOptional = Schedule_Properties_Optional;
    if (pProprietary)
        *pProprietary = Schedule_Properties_Proprietary;
}

void Schedule_Init(void)
{
    unsigned i, j;
    for (i = 0; i < MAX_SCHEDULES; i++) {
        /* whole year, change as neccessary */
        Schedule_Descr[i].Start_Date.year = 0xFF;
        Schedule_Descr[i].Start_Date.month = 1;
        Schedule_Descr[i].Start_Date.day = 1;
        Schedule_Descr[i].Start_Date.wday = 0xFF;
        Schedule_Descr[i].End_Date.year = 0xFF;
        Schedule_Descr[i].End_Date.month = 12;
        Schedule_Descr[i].End_Date.day = 31;
        Schedule_Descr[i].End_Date.wday = 0xFF;
        for (j = 0; j < 7; j++) {
            Schedule_Descr[i].Weekly_Schedule[j].TV_Count = 0;
        }
        Schedule_Descr[i].Present_Value = &Schedule_Descr[i].Schedule_Default;
        Schedule_Descr[i].Schedule_Default.context_specific = false;
        Schedule_Descr[i].Schedule_Default.tag = BACNET_APPLICATION_TAG_REAL;
        Schedule_Descr[i].Schedule_Default.type.Real = 21.0;    /* 21 C, room temperature */
        Schedule_Descr[i].obj_prop_ref_cnt = 0; /* no references, add as needed */
        Schedule_Descr[i].Priority_For_Writing = 16;    /* lowest priority */
        Schedule_Descr[i].Out_Of_Service = false;
    }
}

bool Schedule_Valid_Instance(uint32_t object_instance)
{
    unsigned int index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES)
        return true;
    else
        return false;
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

    if (instance < MAX_SCHEDULES)
        index = instance;

    return index;
}

bool Schedule_Object_Name(uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name)
{
    static char text_string[32] = "";   /* okay for single thread */
    unsigned int index;
    bool status = false;

    index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES) {
        sprintf(text_string, "SCHEDULE %lu", (unsigned long) index);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}

int Schedule_Read_Property(BACNET_READ_PROPERTY_DATA * rpdata)
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
    if (object_index < MAX_SCHEDULES)
        CurrentSC = &Schedule_Descr[object_index];
    else
        return BACNET_STATUS_ERROR;

    apdu = rpdata->application_data;
    switch ((int) rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_SCHEDULE,
                rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Schedule_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_SCHEDULE);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = bacapp_encode_data(&apdu[0], CurrentSC->Present_Value);
            break;
        case PROP_EFFECTIVE_PERIOD:
            apdu_len = encode_bacnet_date(&apdu[0], &CurrentSC->Start_Date);
            apdu_len +=
                encode_bacnet_date(&apdu[apdu_len], &CurrentSC->End_Date);
            break;
        case PROP_WEEKLY_SCHEDULE:
            if (rpdata->array_index == 0)       /* count, always 7 */
                apdu_len = encode_application_unsigned(&apdu[0], 7);
            else if (rpdata->array_index == BACNET_ARRAY_ALL) { /* full array */
                int day;
                for (day = 0; day < 7; day++) {
                    apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                    for (i = 0; i < CurrentSC->Weekly_Schedule[day].TV_Count;
                        i++) {
                        apdu_len +=
                            bacapp_encode_time_value(&apdu[apdu_len],
                            &CurrentSC->Weekly_Schedule[day].Time_Values[i]);
                    }
                    apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
                }
            } else if (rpdata->array_index <= 7) {      /* some array element */
                int day = rpdata->array_index - 1;
                apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                for (i = 0; i < CurrentSC->Weekly_Schedule[day].TV_Count; i++) {
                    apdu_len +=
                        bacapp_encode_time_value(&apdu[apdu_len],
                        &CurrentSC->Weekly_Schedule[day].Time_Values[i]);
                }
                apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
            } else {    /* out of bounds */
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
                apdu_len +=
                    bacapp_encode_device_obj_property_ref(&apdu[apdu_len],
                    &CurrentSC->Object_Property_References[i]);
            }
            break;
        case PROP_PRIORITY_FOR_WRITING:
            apdu_len =
                encode_application_unsigned(&apdu[0],
                CurrentSC->Priority_For_Writing);
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
            apdu_len =
                encode_application_enumerated(&apdu[0],
                RELIABILITY_NO_FAULT_DETECTED);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0], false);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    if ((apdu_len >= 0) && (rpdata->object_property != PROP_WEEKLY_SCHEDULE)
        && (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

bool Schedule_Write_Property(BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    unsigned object_index = 0;
    bool status = false;        /* return value */

    object_index = Schedule_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_SCHEDULES) {
        return false;
    }

    switch ((int) wp_data->object_property) {
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
        case PROP_OUT_OF_SERVICE:
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

bool Schedule_In_Effective_Period(SCHEDULE_DESCR * desc,
    BACNET_DATE * date)
{
    bool res = false;

    if (desc && date) {
        if (datetime_wildcard_compare_date(&desc->Start_Date, date) <= 0 &&
            datetime_wildcard_compare_date(&desc->End_Date, date) >= 0)
            res = true;
    }

    return res;
}

void Schedule_Recalculate_PV(SCHEDULE_DESCR * desc,
    BACNET_WEEKDAY wday,
    BACNET_TIME * time)
{
    int i;
    desc->Present_Value = NULL;

    /* for future development, here should be the loop for Exception Schedule */

    for (i = 0;
        i < desc->Weekly_Schedule[wday - 1].TV_Count &&
        desc->Present_Value == NULL; i++) {
        int diff = datetime_wildcard_compare_time(time,
            &desc->Weekly_Schedule[wday - 1].Time_Values[i].Time);
        if (diff >= 0 &&
            desc->Weekly_Schedule[wday - 1].Time_Values[i].Value.tag !=
            BACNET_APPLICATION_TAG_NULL) {
            desc->Present_Value =
                &desc->Weekly_Schedule[wday - 1].Time_Values[i].Value;
        }
    }

    if (desc->Present_Value == NULL)
        desc->Present_Value = &desc->Schedule_Default;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"


void testSchedule(Test * pTest)
{
    BACNET_READ_PROPERTY_DATA rpdata;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint16_t decoded_type = 0;
    uint32_t decoded_instance = 0;

    Schedule_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_SCHEDULE;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Schedule_Read_Property(&rpdata);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == rpdata.object_type);
    ct_test(pTest, decoded_instance == rpdata.object_instance);

    return;
}


#ifdef TEST_SCHEDULE

int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Schedule", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testSchedule);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}

#endif /* TEST_SCHEDULE */
#endif /* TEST */
