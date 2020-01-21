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

/* Access Door Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/wp.h"
#include "access_door.h"
#include "bacnet/basic/services.h"

static bool Access_Door_Initialized = false;

static ACCESS_DOOR_DESCR ad_descr[MAX_ACCESS_DOORS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_STATUS_FLAGS,
    PROP_EVENT_STATE, PROP_RELIABILITY, PROP_OUT_OF_SERVICE,
    PROP_PRIORITY_ARRAY, PROP_RELINQUISH_DEFAULT, PROP_DOOR_PULSE_TIME,
    PROP_DOOR_EXTENDED_PULSE_TIME, PROP_DOOR_OPEN_TOO_LONG_TIME, -1 };

static const int Properties_Optional[] = { PROP_DOOR_STATUS, PROP_LOCK_STATUS,
    PROP_SECURED_STATUS, PROP_DOOR_UNLOCK_DELAY_TIME, PROP_DOOR_ALARM_STATE,
    -1 };

static const int Properties_Proprietary[] = { -1 };

void Access_Door_Property_Lists(
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

void Access_Door_Init(void)
{
    unsigned i, j;

    if (!Access_Door_Initialized) {
        Access_Door_Initialized = true;

        /* initialize all the access door priority arrays to NULL */
        for (i = 0; i < MAX_ACCESS_DOORS; i++) {
            ad_descr[i].relinquish_default = DOOR_VALUE_LOCK;
            ad_descr[i].event_state = EVENT_STATE_NORMAL;
            ad_descr[i].reliability = RELIABILITY_NO_FAULT_DETECTED;
            ad_descr[i].out_of_service = false;
            ad_descr[i].door_status = DOOR_STATUS_CLOSED;
            ad_descr[i].lock_status = LOCK_STATUS_LOCKED;
            ad_descr[i].secured_status = DOOR_SECURED_STATUS_SECURED;
            ad_descr[i].door_pulse_time = 30; /* 3s */
            ad_descr[i].door_extended_pulse_time = 50; /* 5s */
            ad_descr[i].door_unlock_delay_time = 0; /* 0s */
            ad_descr[i].door_open_too_long_time = 300; /* 30s */
            ad_descr[i].door_alarm_state = DOOR_ALARM_STATE_NORMAL;
            for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
                ad_descr[i].value_active[j] = false;
                /* just to fill in */
                ad_descr[i].priority_array[j] = DOOR_VALUE_LOCK;
            }
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Access_Door_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_ACCESS_DOORS) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Access_Door_Count(void)
{
    return MAX_ACCESS_DOORS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Access_Door_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Access_Door_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_ACCESS_DOORS;

    if (object_instance < MAX_ACCESS_DOORS) {
        index = object_instance;
    }

    return index;
}

BACNET_DOOR_VALUE Access_Door_Present_Value(uint32_t object_instance)
{
    unsigned index = 0;
    unsigned i = 0;
    BACNET_DOOR_VALUE value = DOOR_VALUE_LOCK;

    index = Access_Door_Instance_To_Index(object_instance);
    if (index < MAX_ACCESS_DOORS) {
        value = ad_descr[i].relinquish_default;
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (ad_descr[index].value_active[i]) {
                value = ad_descr[index].priority_array[i];
                break;
            }
        }
    }
    return value;
}

unsigned Access_Door_Present_Value_Priority(uint32_t object_instance)
{
    unsigned index = 0; /* instance to index conversion */
    unsigned i = 0; /* loop counter */
    unsigned priority = 0; /* return value */

    index = Access_Door_Instance_To_Index(object_instance);
    if (index < MAX_ACCESS_DOORS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (ad_descr[index].value_active[i]) {
                priority = i + 1;
                break;
            }
        }
    }

    return priority;
}

bool Access_Door_Present_Value_Set(
    uint32_t object_instance, BACNET_DOOR_VALUE value, unsigned priority)
{
    unsigned index = 0;
    bool status = false;

    index = Access_Door_Instance_To_Index(object_instance);
    if (index < MAX_ACCESS_DOORS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */) && 
            (value <= DOOR_VALUE_EXTENDED_PULSE_UNLOCK)) {
            ad_descr[index].value_active[priority - 1] = true;
            ad_descr[index].priority_array[priority - 1] = value;
            /* Note: you could set the physical output here to the next
               highest priority, or to the relinquish default if no
               priorities are set.
               However, if Out of Service is TRUE, then don't set the
               physical output.  This comment may apply to the
               main loop (i.e. check out of service before changing output) */
            status = true;
        }
    }

    return status;
}

bool Access_Door_Present_Value_Relinquish(
    uint32_t object_instance, unsigned priority)
{
    unsigned index = 0;
    bool status = false;

    index = Access_Door_Instance_To_Index(object_instance);
    if (index < MAX_ACCESS_DOORS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */)) {
            ad_descr[index].value_active[priority - 1] = false;
            /* Note: you could set the physical output here to the next
               highest priority, or to the relinquish default if no
               priorities are set.
               However, if Out of Service is TRUE, then don't set the
               physical output.  This comment may apply to the
               main loop (i.e. check out of service before changing output) */
            status = true;
        }
    }

    return status;
}

BACNET_DOOR_VALUE Access_Door_Relinquish_Default(uint32_t object_instance)
{
    BACNET_DOOR_VALUE status = DOOR_VALUE_LOCK;
    unsigned index = 0;
    index = Access_Door_Instance_To_Index(object_instance);
    if (index < MAX_ACCESS_DOORS) {
        return ad_descr[index].relinquish_default;
    }

    return status;
}

/* note: the object name must be unique within this device */
bool Access_Door_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = ""; /* okay for single thread */
    bool status = false;

    if (object_instance < MAX_ACCESS_DOORS) {
        sprintf(text_string, "ACCESS DOOR %lu", (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}

bool Access_Door_Out_Of_Service(uint32_t instance)
{
    unsigned index = 0;
    bool oos_flag = false;

    index = Access_Door_Instance_To_Index(instance);
    if (index < MAX_ACCESS_DOORS) {
        oos_flag = ad_descr[index].out_of_service;
    }

    return oos_flag;
}

void Access_Door_Out_Of_Service_Set(uint32_t instance, bool oos_flag)
{
    unsigned index = 0;

    index = Access_Door_Instance_To_Index(instance);
    if (index < MAX_ACCESS_DOORS) {
        ad_descr[index].out_of_service = oos_flag;
    }
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Access_Door_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    object_index = Access_Door_Instance_To_Index(rpdata->object_instance);
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_ACCESS_DOOR, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Access_Door_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ACCESS_DOOR);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Access_Door_Present_Value(rpdata->object_instance));
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Access_Door_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(
                &apdu[0], ad_descr[object_index].event_state);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], ad_descr[object_index].reliability);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Access_Door_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_PRIORITY_ARRAY:
            /* Array element zero is the number of elements in the array */
            if (rpdata->array_index == 0) {
                apdu_len =
                    encode_application_unsigned(&apdu[0], BACNET_MAX_PRIORITY);
                /* if no index was specified, then try to encode the entire list
                 */
                /* into one packet. */
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    if (ad_descr[object_index].value_active[i]) {
                        len = encode_application_null(&apdu[apdu_len]);
                    } else {
                        len = encode_application_enumerated(&apdu[apdu_len],
                            ad_descr[object_index].priority_array[i]);
                    }
                    /* add it if we have room */
                    if ((apdu_len + len) < MAX_APDU) {
                        apdu_len += len;
                    } else {
                        rpdata->error_code =
                            ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                        apdu_len = BACNET_STATUS_ABORT;
                        break;
                    }
                }
            } else {
                if (rpdata->array_index <= BACNET_MAX_PRIORITY) {
                    if (ad_descr[object_index].value_active[i]) {
                        apdu_len = encode_application_null(&apdu[0]);
                    } else {
                        apdu_len =
                            encode_application_enumerated(&apdu[apdu_len],
                                ad_descr[object_index].priority_array[i]);
                    }
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        case PROP_RELINQUISH_DEFAULT:
            apdu_len = encode_application_enumerated(&apdu[0],
                Access_Door_Relinquish_Default(rpdata->object_instance));
            break;
        case PROP_DOOR_STATUS:
            apdu_len = encode_application_enumerated(
                &apdu[0], ad_descr[object_index].door_status);
            break;
        case PROP_LOCK_STATUS:
            apdu_len = encode_application_enumerated(
                &apdu[0], ad_descr[object_index].lock_status);
            break;
        case PROP_SECURED_STATUS:
            apdu_len = encode_application_enumerated(
                &apdu[0], ad_descr[object_index].secured_status);
            break;
        case PROP_DOOR_PULSE_TIME:
            apdu_len = encode_application_unsigned(
                &apdu[0], ad_descr[object_index].door_pulse_time);
            break;
        case PROP_DOOR_EXTENDED_PULSE_TIME:
            apdu_len = encode_application_unsigned(
                &apdu[0], ad_descr[object_index].door_extended_pulse_time);
            break;
        case PROP_DOOR_UNLOCK_DELAY_TIME:
            apdu_len = encode_application_unsigned(
                &apdu[0], ad_descr[object_index].door_unlock_delay_time);
            break;
        case PROP_DOOR_OPEN_TOO_LONG_TIME:
            apdu_len = encode_application_unsigned(
                &apdu[0], ad_descr[object_index].door_open_too_long_time);
            break;
        case PROP_DOOR_ALARM_STATE:
            apdu_len = encode_application_enumerated(
                &apdu[0], ad_descr[object_index].door_alarm_state);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Access_Door_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    unsigned object_index = 0;

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
    /*  only array properties can have array options */
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    object_index = Access_Door_Instance_To_Index(wp_data->object_instance);
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                status = Access_Door_Present_Value_Set(wp_data->object_instance,
                    (BACNET_DOOR_VALUE)value.type.Enumerated, wp_data->priority);
                if (wp_data->priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                status = WPValidateArgType(&value, BACNET_APPLICATION_TAG_NULL,
                    &wp_data->error_class, &wp_data->error_code);
                if (status) {
                    status = Access_Door_Present_Value_Relinquish(
                        wp_data->object_instance, wp_data->priority);
                    if (!status) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                Access_Door_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_DOOR_STATUS:
            if (Access_Door_Out_Of_Service(wp_data->object_instance)) {
                status =
                    WPValidateArgType(&value, BACNET_APPLICATION_TAG_ENUMERATED,
                        &wp_data->error_class, &wp_data->error_code);
                if (status) {
                    ad_descr[object_index].door_status =
                        (BACNET_DOOR_STATUS)value.type.Enumerated;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        case PROP_LOCK_STATUS:
            if (Access_Door_Out_Of_Service(wp_data->object_instance)) {
                status =
                    WPValidateArgType(&value, BACNET_APPLICATION_TAG_ENUMERATED,
                        &wp_data->error_class, &wp_data->error_code);
                if (status) {
                    ad_descr[object_index].lock_status =
                        (BACNET_LOCK_STATUS)value.type.Enumerated;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        case PROP_DOOR_ALARM_STATE:
            if (Access_Door_Out_Of_Service(wp_data->object_instance)) {
                status =
                    WPValidateArgType(&value, BACNET_APPLICATION_TAG_ENUMERATED,
                        &wp_data->error_class, &wp_data->error_code);
                if (status) {
                    ad_descr[object_index].door_alarm_state =
                        (BACNET_DOOR_ALARM_STATE)value.type.Enumerated;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        case PROP_RELIABILITY:
        case PROP_PRIORITY_ARRAY:
        case PROP_RELINQUISH_DEFAULT:
        case PROP_SECURED_STATUS:
        case PROP_DOOR_PULSE_TIME:
        case PROP_DOOR_EXTENDED_PULSE_TIME:
        case PROP_DOOR_UNLOCK_DELAY_TIME:
        case PROP_DOOR_OPEN_TOO_LONG_TIME:
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

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

bool WPValidateArgType(BACNET_APPLICATION_DATA_VALUE *pValue,
    uint8_t ucExpectedTag,
    BACNET_ERROR_CLASS *pErrorClass,
    BACNET_ERROR_CODE *pErrorCode)
{
    pValue = pValue;
    ucExpectedTag = ucExpectedTag;
    pErrorClass = pErrorClass;
    pErrorCode = pErrorCode;

    return false;
}

void testAccessDoor(Test *pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint32_t decoded_instance = 0;
    uint16_t decoded_type = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Access_Door_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_DOOR;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Access_Door_Read_Property(&rpdata);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == rpdata.object_type);
    ct_test(pTest, decoded_instance == rpdata.object_instance);

    return;
}

#ifdef TEST_ACCESS_DOOR
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Access Door", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testAccessDoor);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_ACCESS_DOOR */
#endif /* TEST */
