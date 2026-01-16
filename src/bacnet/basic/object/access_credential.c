/**
 * @file
 * @brief A basic BACnet Access Credential Object implementation.
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/wp.h"
#include "bacnet/proplist.h"
#include "bacnet/basic/object/access_credential.h"
#include "bacnet/basic/services.h"

static bool Access_Credential_Initialized = false;

static ACCESS_CREDENTIAL_DESCR ac_descr[MAX_ACCESS_CREDENTIALS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_GLOBAL_IDENTIFIER,
    PROP_STATUS_FLAGS,
    PROP_RELIABILITY,
    PROP_CREDENTIAL_STATUS,
    PROP_REASON_FOR_DISABLE,
    PROP_AUTHENTICATION_FACTORS,
    PROP_ACTIVATION_TIME,
    PROP_EXPIRATION_TIME,
    PROP_CREDENTIAL_DISABLE,
    PROP_ASSIGNED_ACCESS_RIGHTS,
    -1
};

static const int32_t Properties_Optional[] = { -1 };

static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = { PROP_GLOBAL_IDENTIFIER, -1 };

void Access_Credential_Property_Lists(
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
 * @brief Get the list of writable properties for an Access Credential object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Access_Credential_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
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
    char text[32] = "";
    bool status = false;

    if (object_instance < MAX_ACCESS_CREDENTIALS) {
        snprintf(
            text, sizeof(text), "ACCESS CREDENTIAL %lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Access_Credential_Authentication_Factor_Array_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;

    if (object_instance < MAX_ACCESS_CREDENTIALS) {
        if (index < ac_descr[object_instance].auth_factors_count) {
            apdu_len = bacapp_encode_credential_authentication_factor(
                apdu, &ac_descr[object_instance].auth_factors[index]);
        }
    }

    return apdu_len;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Access_Credential_Assigned_Access_Rights_Array_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;

    if (object_instance < MAX_ACCESS_CREDENTIALS) {
        if (index < ac_descr[object_instance].assigned_access_rights_count) {
            apdu_len = bacapp_encode_assigned_access_rights(
                apdu, &ac_descr[object_instance].assigned_access_rights[index]);
        }
    }

    return apdu_len;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Access_Credential_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    int apdu_size;
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
    apdu_size = rpdata->application_data_len;
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
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Access_Credential_Authentication_Factor_Array_Encode,
                ac_descr[object_index].auth_factors_count, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
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
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Access_Credential_Assigned_Access_Rights_Array_Encode,
                ac_descr[object_index].assigned_access_rights_count, apdu,
                apdu_size);
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

/* returns true if successful */
bool Access_Credential_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
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
    object_index =
        Access_Credential_Instance_To_Index(wp_data->object_instance);
    switch (wp_data->object_property) {
        case PROP_GLOBAL_IDENTIFIER:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                ac_descr[object_index].global_identifier =
                    value.type.Unsigned_Int;
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
