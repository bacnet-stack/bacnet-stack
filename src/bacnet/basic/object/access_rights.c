/**
 * @file
 * @brief A basic BACnet Access Rights Objects implementation.
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/property.h"
#include "bacnet/proplist.h"
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
/* me! */
#include "access_rights.h"

static bool Access_Rights_Initialized = false;

static ACCESS_RIGHTS_DESCR ar_descr[MAX_ACCESS_RIGHTSS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_GLOBAL_IDENTIFIER,
    PROP_STATUS_FLAGS,
    PROP_RELIABILITY,
    PROP_ENABLE,
    PROP_NEGATIVE_ACCESS_RULES,
    PROP_POSITIVE_ACCESS_RULES,
    -1
};

static const int Properties_Optional[] = { -1 };

static const int Properties_Proprietary[] = { -1 };

void Access_Rights_Property_Lists(
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

void Access_Rights_Init(void)
{
    unsigned i;

    if (!Access_Rights_Initialized) {
        Access_Rights_Initialized = true;

        for (i = 0; i < MAX_ACCESS_RIGHTSS; i++) {
            ar_descr[i].global_identifier =
                0; /* set to some meaningful value */
            ar_descr[i].reliability = RELIABILITY_NO_FAULT_DETECTED;
            ar_descr[i].enable = false;
            ar_descr[i].negative_access_rules_count = 0;
            ar_descr[i].positive_access_rules_count = 0;
            /* fill in the positive and negative access rules with proper ids */
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Access_Rights_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_ACCESS_RIGHTSS) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Access_Rights_Count(void)
{
    return MAX_ACCESS_RIGHTSS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Access_Rights_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Access_Rights_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_ACCESS_RIGHTSS;

    if (object_instance < MAX_ACCESS_RIGHTSS) {
        index = object_instance;
    }

    return index;
}

/* note: the object name must be unique within this device */
bool Access_Rights_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;

    if (object_instance < MAX_ACCESS_RIGHTSS) {
        snprintf(
            text, sizeof(text), "ACCESS RIGHTS %lu",
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
static int Negative_Access_Rules_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_ACCESS_RULE *rule;
    uint32_t count;

    if (object_instance < MAX_ACCESS_RIGHTSS) {
        count = ar_descr[object_instance].negative_access_rules_count;
        if (index < count) {
            rule = &ar_descr[object_instance].negative_access_rules[index];
            apdu_len = bacapp_encode_access_rule(&apdu[0], rule);
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
static int Positive_Access_Rules_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_ACCESS_RULE *rule;
    uint32_t count;

    if (object_instance < MAX_ACCESS_RIGHTSS) {
        count = ar_descr[object_instance].positive_access_rules_count;
        if (index < count) {
            rule = &ar_descr[object_instance].positive_access_rules[index];
            apdu_len = bacapp_encode_access_rule(&apdu[0], rule);
        }
    }

    return apdu_len;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Access_Rights_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    int apdu_size = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index = 0;
    BACNET_UNSIGNED_INTEGER count;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
    object_index = Access_Rights_Instance_To_Index(rpdata->object_instance);
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_ACCESS_RIGHTS, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Access_Rights_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ACCESS_RIGHTS);
            break;
        case PROP_GLOBAL_IDENTIFIER:
            apdu_len = encode_application_unsigned(
                &apdu[0], ar_descr[object_index].global_identifier);
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
                &apdu[0], ar_descr[object_index].reliability);
            break;
        case PROP_ENABLE:
            apdu_len = encode_application_boolean(
                &apdu[0], ar_descr[object_index].enable);
            break;
        case PROP_NEGATIVE_ACCESS_RULES:
            count = ar_descr[object_index].negative_access_rules_count;
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Negative_Access_Rules_Encode, count, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_POSITIVE_ACCESS_RULES:
            count = ar_descr[object_index].positive_access_rules_count;
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Positive_Access_Rules_Encode, count, apdu, apdu_size);
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
bool Access_Rights_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
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
    object_index = Access_Rights_Instance_To_Index(wp_data->object_instance);
    switch (wp_data->object_property) {
        case PROP_GLOBAL_IDENTIFIER:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                ar_descr[object_index].global_identifier =
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
