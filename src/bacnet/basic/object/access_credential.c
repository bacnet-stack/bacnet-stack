/**************************************************************************
 *
 * Copyright (C) 2015 Nikola Jelic <nikola.jelic@euroicc.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the credential to use, copy, modify, merge, publish,
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

/* Access Credential Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/wp.h"
#include "bacnet/basic/object/access_credential.h"
#include "bacnet/basic/services.h"

static bool Access_Credential_Initialized = false;

static ACCESS_CREDENTIAL_DESCR ac_descr[MAX_ACCESS_CREDENTIALS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_GLOBAL_IDENTIFIER,
    PROP_STATUS_FLAGS, PROP_RELIABILITY, PROP_CREDENTIAL_STATUS,
    PROP_REASON_FOR_DISABLE, PROP_AUTHENTICATION_FACTORS, PROP_ACTIVATION_TIME,
    PROP_EXPIRATION_TIME, PROP_CREDENTIAL_DISABLE, PROP_ASSIGNED_ACCESS_RIGHTS,
    -1 };

static const int Properties_Optional[] = { -1 };

static const int Properties_Proprietary[] = { -1 };

void Access_Credential_Property_Lists(
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

void Access_Credential_Init(void)
{
    unsigned i;

    if (!Access_Credential_Initialized) {
        Access_Credential_Initialized = true;

        for (i = 0; i < MAX_ACCESS_CREDENTIALS; i++) {
            ac_descr[i].global_identifier =
                0; /* set to some meaningful value */
            ac_descr[i].reliability = RELIABILITY_NO_FAULT_DETECTED;
            ac_descr[i].credential_status = false;
            ac_descr[i].reasons_count = 0;
            ac_descr[i].auth_factors_count = 0;
            memset(&ac_descr[i].activation_time, 0, sizeof(BACNET_DATE_TIME));
            memset(&ac_descr[i].expiration_time, 0, sizeof(BACNET_DATE_TIME));
            ac_descr[i].credential_disable = ACCESS_CREDENTIAL_DISABLE_NONE;
            ac_descr[i].assigned_access_rights_count = 0;
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Access_Credential_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_ACCESS_CREDENTIALS) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Access_Credential_Count(void)
{
    return MAX_ACCESS_CREDENTIALS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Access_Credential_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Access_Credential_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_ACCESS_CREDENTIALS;

    if (object_instance < MAX_ACCESS_CREDENTIALS) {
        index = object_instance;
    }

    return index;
}

/* note: the object name must be unique within this device */
bool Access_Credential_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = ""; /* okay for single thread */
    bool status = false;

    if (object_instance < MAX_ACCESS_CREDENTIALS) {
        sprintf(text_string, "ACCESS CREDENTIAL %lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Access_Credential_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index = 0;
    unsigned i = 0;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    object_index = Access_Credential_Instance_To_Index(rpdata->object_instance);
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_ACCESS_CREDENTIAL, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Access_Credential_Object_Name(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], OBJECT_ACCESS_CREDENTIAL);
            break;
        case PROP_GLOBAL_IDENTIFIER:
            apdu_len = encode_application_unsigned(
                &apdu[0], ac_descr[object_index].global_identifier);
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
                &apdu[0], ac_descr[object_index].reliability);
            break;
        case PROP_CREDENTIAL_STATUS:
            apdu_len = encode_application_enumerated(
                &apdu[0], ac_descr[object_index].credential_status);
            break;
        case PROP_REASON_FOR_DISABLE:
            for (i = 0; i < ac_descr[object_index].reasons_count; i++) {
                len = encode_application_enumerated(
                    &apdu[0], ac_descr[object_index].reason_for_disable[i]);
                if (apdu_len + len < MAX_APDU) {
                    apdu_len += len;
                } else {
                    rpdata->error_code =
                        ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                    apdu_len = BACNET_STATUS_ABORT;
                    break;
                }
            }
            break;
        case PROP_AUTHENTICATION_FACTORS:
            if (rpdata->array_index == 0) {
                apdu_len = encode_application_unsigned(
                    &apdu[0], ac_descr[object_index].auth_factors_count);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                for (i = 0; i < ac_descr[object_index].auth_factors_count;
                     i++) {
                    len = bacapp_encode_credential_authentication_factor(
                        &apdu[0], &ac_descr[object_index].auth_factors[i]);
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
                if (rpdata->array_index <=
                    ac_descr[object_index].auth_factors_count) {
                    apdu_len =
                        bacapp_encode_credential_authentication_factor(&apdu[0],
                            &ac_descr[object_index]
                                 .auth_factors[rpdata->array_index - 1]);
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        case PROP_ACTIVATION_TIME:
            apdu_len = bacapp_encode_datetime(
                &apdu[0], &ac_descr[object_index].activation_time);
            break;
        case PROP_EXPIRATION_TIME:
            apdu_len = bacapp_encode_datetime(
                &apdu[0], &ac_descr[object_index].expiration_time);
            break;
        case PROP_CREDENTIAL_DISABLE:
            apdu_len = encode_application_enumerated(
                &apdu[0], ac_descr[object_index].credential_disable);
            break;
        case PROP_ASSIGNED_ACCESS_RIGHTS:
            if (rpdata->array_index == 0) {
                apdu_len = encode_application_unsigned(&apdu[0],
                    ac_descr[object_index].assigned_access_rights_count);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                for (i = 0;
                     i < ac_descr[object_index].assigned_access_rights_count;
                     i++) {
                    len = bacapp_encode_assigned_access_rights(&apdu[0],
                        &ac_descr[object_index].assigned_access_rights[i]);
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
                if (rpdata->array_index <=
                    ac_descr[object_index].assigned_access_rights_count) {
                    apdu_len = bacapp_encode_assigned_access_rights(&apdu[0],
                        &ac_descr[object_index]
                             .assigned_access_rights[rpdata->array_index - 1]);
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
    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_AUTHENTICATION_FACTORS) &&
        (rpdata->object_property != PROP_ASSIGNED_ACCESS_RIGHTS) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Access_Credential_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
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
    if ((wp_data->object_property != PROP_AUTHENTICATION_FACTORS) &&
        (wp_data->object_property != PROP_ASSIGNED_ACCESS_RIGHTS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    object_index =
        Access_Credential_Instance_To_Index(wp_data->object_instance);
    switch (wp_data->object_property) {
        case PROP_GLOBAL_IDENTIFIER:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                    &wp_data->error_class, &wp_data->error_code);
            if (status) {
                ac_descr[object_index].global_identifier =
                    value.type.Unsigned_Int;
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_RELIABILITY:
        case PROP_CREDENTIAL_STATUS:
        case PROP_REASON_FOR_DISABLE:
        case PROP_AUTHENTICATION_FACTORS:
        case PROP_ACTIVATION_TIME:
        case PROP_EXPIRATION_TIME:
        case PROP_CREDENTIAL_DISABLE:
        case PROP_ASSIGNED_ACCESS_RIGHTS:
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

void testAccessCredential(Test *pTest)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint32_t decoded_instance = 0;
    uint16_t decoded_type = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Access_Credential_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_CREDENTIAL;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Access_Credential_Read_Property(&rpdata);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == rpdata.object_type);
    ct_test(pTest, decoded_instance == rpdata.object_instance);

    return;
}

#ifdef TEST_ACCESS_CREDENTIAL
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Access Credential", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testAccessCredential);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_ACCESS_CREDENTIAL */
#endif /* BAC_TEST */
