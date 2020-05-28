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

/* Access Point Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/wp.h"
#include "access_point.h"
#include "bacnet/basic/services.h"

static bool Access_Point_Initialized = false;

static ACCESS_POINT_DESCR ap_descr[MAX_ACCESS_POINTS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_STATUS_FLAGS, PROP_EVENT_STATE,
    PROP_RELIABILITY, PROP_OUT_OF_SERVICE, PROP_AUTHENTICATION_STATUS,
    PROP_ACTIVE_AUTHENTICATION_POLICY, PROP_NUMBER_OF_AUTHENTICATION_POLICIES,
    PROP_AUTHORIZATION_MODE, PROP_ACCESS_EVENT, PROP_ACCESS_EVENT_TAG,
    PROP_ACCESS_EVENT_TIME, PROP_ACCESS_EVENT_CREDENTIAL, PROP_ACCESS_DOORS,
    PROP_PRIORITY_FOR_WRITING, -1 };

static const int Properties_Optional[] = { -1 };

static const int Properties_Proprietary[] = { -1 };

void Access_Point_Property_Lists(
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

void Access_Point_Init(void)
{
    unsigned i;

    if (!Access_Point_Initialized) {
        Access_Point_Initialized = true;

        for (i = 0; i < MAX_ACCESS_POINTS; i++) {
            ap_descr[i].event_state = EVENT_STATE_NORMAL;
            ap_descr[i].reliability = RELIABILITY_NO_FAULT_DETECTED;
            ap_descr[i].out_of_service = false;
            ap_descr[i].authentication_status = AUTHENTICATION_STATUS_NOT_READY;
            ap_descr[i].active_authentication_policy = 0;
            ap_descr[i].number_of_authentication_policies = 0;
            ap_descr[i].authorization_mode = AUTHORIZATION_MODE_AUTHORIZE;
            ap_descr[i].access_event = ACCESS_EVENT_NONE;
            /* timestamp uninitialized */
            /* access_event_credential should be set to some meaningful value */
            ap_descr[i].num_doors = 0;
            /* fill in the access doors with proper ids */
            ap_descr[i].priority_for_writing = 16; /* lowest possible for now */
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Access_Point_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_ACCESS_POINTS) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Access_Point_Count(void)
{
    return MAX_ACCESS_POINTS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Access_Point_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Access_Point_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_ACCESS_POINTS;

    if (object_instance < MAX_ACCESS_POINTS) {
        index = object_instance;
    }

    return index;
}

/* note: the object name must be unique within this device */
bool Access_Point_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = ""; /* okay for single thread */
    bool status = false;

    if (object_instance < MAX_ACCESS_POINTS) {
        sprintf(
            text_string, "ACCESS POINT %lu", (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}

bool Access_Point_Out_Of_Service(uint32_t instance)
{
    unsigned index = 0;
    bool oos_flag = false;

    index = Access_Point_Instance_To_Index(instance);
    if (index < MAX_ACCESS_POINTS) {
        oos_flag = ap_descr[index].out_of_service;
    }

    return oos_flag;
}

void Access_Point_Out_Of_Service_Set(uint32_t instance, bool oos_flag)
{
    unsigned index = 0;

    index = Access_Point_Instance_To_Index(instance);
    if (index < MAX_ACCESS_POINTS) {
        ap_descr[index].out_of_service = oos_flag;
    }
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Access_Point_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
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
    object_index = Access_Point_Instance_To_Index(rpdata->object_instance);
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_ACCESS_POINT, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Access_Point_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ACCESS_POINT);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Access_Point_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(
                &apdu[0], ap_descr[object_index].event_state);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], ap_descr[object_index].reliability);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Access_Point_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_AUTHENTICATION_STATUS:
            apdu_len = encode_application_enumerated(
                &apdu[0], ap_descr[object_index].authentication_status);
            break;
        case PROP_ACTIVE_AUTHENTICATION_POLICY:
            apdu_len = encode_application_unsigned(
                &apdu[0], ap_descr[object_index].active_authentication_policy);
            break;
        case PROP_NUMBER_OF_AUTHENTICATION_POLICIES:
            apdu_len = encode_application_unsigned(&apdu[0],
                ap_descr[object_index].number_of_authentication_policies);
            break;
        case PROP_AUTHORIZATION_MODE:
            apdu_len = encode_application_enumerated(
                &apdu[0], ap_descr[object_index].authorization_mode);
            break;
        case PROP_ACCESS_EVENT:
            apdu_len = encode_application_enumerated(
                &apdu[0], ap_descr[object_index].access_event);
            break;
        case PROP_ACCESS_EVENT_TAG:
            apdu_len = encode_application_unsigned(
                &apdu[0], ap_descr[object_index].access_event_tag);
            break;
        case PROP_ACCESS_EVENT_TIME:
            apdu_len = bacapp_encode_timestamp(
                &apdu[0], &ap_descr[object_index].access_event_time);
            break;
        case PROP_ACCESS_EVENT_CREDENTIAL:
            apdu_len = bacapp_encode_device_obj_ref(
                &apdu[0], &ap_descr[object_index].access_event_credential);
            break;
        case PROP_ACCESS_DOORS:
            if (rpdata->array_index == 0) {
                apdu_len = encode_application_unsigned(
                    &apdu[0], ap_descr[object_index].num_doors);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                for (i = 0; i < ap_descr[object_index].num_doors; i++) {
                    len = bacapp_encode_device_obj_ref(
                        &apdu[0], &ap_descr[object_index].access_doors[i]);
                    if (apdu_len + len < MAX_APDU) {
                        apdu_len += len;
                    } else {
                        rpdata->error_code =
                            ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                        apdu_len = BACNET_STATUS_ABORT;
                        break;
                    }
                }
            } else {
                if (rpdata->array_index <= ap_descr[object_index].num_doors) {
                    apdu_len = bacapp_encode_device_obj_ref(&apdu[0],
                        &ap_descr[object_index]
                             .access_doors[rpdata->array_index - 1]);
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_ACCESS_DOORS) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Access_Point_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
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
    /*  only array properties can have array options */
    if ((wp_data->object_property != PROP_ACCESS_DOORS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }

    switch (wp_data->object_property) {
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        case PROP_RELIABILITY:
        case PROP_OUT_OF_SERVICE:
        case PROP_AUTHENTICATION_STATUS:
        case PROP_ACTIVE_AUTHENTICATION_POLICY:
        case PROP_NUMBER_OF_AUTHENTICATION_POLICIES:
        case PROP_AUTHORIZATION_MODE:
        case PROP_ACCESS_EVENT:
        case PROP_ACCESS_EVENT_TAG:
        case PROP_ACCESS_EVENT_TIME:
        case PROP_ACCESS_EVENT_CREDENTIAL:
        case PROP_ACCESS_DOORS:
        case PROP_PRIORITY_FOR_WRITING:
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

#ifdef BAC_TEST
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

void testAccessPoint(Test *pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint32_t decoded_instance = 0;
    uint16_t decoded_type = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Access_Point_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_POINT;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Access_Point_Read_Property(&rpdata);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == rpdata.object_type);
    ct_test(pTest, decoded_instance == rpdata.object_instance);

    return;
}

#ifdef TEST_ACCESS_POINT
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Access Point", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testAccessPoint);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_ACCESS_POINT */
#endif /* BAC_TEST */
