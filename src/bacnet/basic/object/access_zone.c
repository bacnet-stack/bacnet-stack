/**
 * @file
 * @brief A basic BACnet Access Zone Objects implementation.
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
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
/* me! */
#include "access_zone.h"

static bool Access_Zone_Initialized = false;

static ACCESS_ZONE_DESCR az_descr[MAX_ACCESS_ZONES];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,     PROP_OBJECT_TYPE,
    PROP_GLOBAL_IDENTIFIER, PROP_OCCUPANCY_STATE, PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,       PROP_RELIABILITY,     PROP_OUT_OF_SERVICE,
    PROP_ENTRY_POINTS,      PROP_EXIT_POINTS,     -1
};

static const int32_t Properties_Optional[] = { -1 };

static const int32_t Properties_Proprietary[] = { -1 };

void Access_Zone_Property_Lists(
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

void Access_Zone_Init(void)
{
    unsigned i;

    if (!Access_Zone_Initialized) {
        Access_Zone_Initialized = true;

        for (i = 0; i < MAX_ACCESS_ZONES; i++) {
            az_descr[i].global_identifier =
                0; /* set to some meaningful value */
            az_descr[i].occupancy_state = ACCESS_ZONE_OCCUPANCY_STATE_DISABLED;
            az_descr[i].event_state = EVENT_STATE_NORMAL;
            az_descr[i].reliability = RELIABILITY_NO_FAULT_DETECTED;
            az_descr[i].out_of_service = false;
            az_descr[i].entry_points_count = 0;
            az_descr[i].exit_points_count = 0;
            /* fill in the entry points and exit points with proper ids */
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Access_Zone_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_ACCESS_ZONES) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Access_Zone_Count(void)
{
    return MAX_ACCESS_ZONES;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Access_Zone_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Access_Zone_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_ACCESS_ZONES;

    if (object_instance < MAX_ACCESS_ZONES) {
        index = object_instance;
    }

    return index;
}

/* note: the object name must be unique within this device */
bool Access_Zone_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;

    if (object_instance < MAX_ACCESS_ZONES) {
        snprintf(
            text, sizeof(text), "ACCESS ZONE %lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
}

bool Access_Zone_Out_Of_Service(uint32_t instance)
{
    unsigned index = 0;
    bool oos_flag = false;

    index = Access_Zone_Instance_To_Index(instance);
    if (index < MAX_ACCESS_ZONES) {
        oos_flag = az_descr[index].out_of_service;
    }

    return oos_flag;
}

void Access_Zone_Out_Of_Service_Set(uint32_t instance, bool oos_flag)
{
    unsigned index = 0;

    index = Access_Zone_Instance_To_Index(instance);
    if (index < MAX_ACCESS_ZONES) {
        az_descr[index].out_of_service = oos_flag;
    }
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Access_Zone_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
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
    object_index = Access_Zone_Instance_To_Index(rpdata->object_instance);
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_ACCESS_ZONE, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Access_Zone_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ACCESS_ZONE);
            break;
        case PROP_GLOBAL_IDENTIFIER:
            apdu_len = encode_application_unsigned(
                &apdu[0], az_descr[object_index].global_identifier);
            break;
        case PROP_OCCUPANCY_STATE:
            apdu_len = encode_application_enumerated(
                &apdu[0], az_descr[object_index].occupancy_state);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Access_Zone_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(
                &apdu[0], az_descr[object_index].event_state);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], az_descr[object_index].reliability);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Access_Zone_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_ENTRY_POINTS:
            for (i = 0; i < az_descr[object_index].entry_points_count; i++) {
                len = bacapp_encode_device_obj_ref(
                    &apdu[0], &az_descr[object_index].entry_points[i]);
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
        case PROP_EXIT_POINTS:
            for (i = 0; i < az_descr[object_index].exit_points_count; i++) {
                len = bacapp_encode_device_obj_ref(
                    &apdu[0], &az_descr[object_index].exit_points[i]);
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
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/* returns true if successful */
bool Access_Zone_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
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
    object_index = Access_Zone_Instance_To_Index(wp_data->object_instance);
    switch (wp_data->object_property) {
        case PROP_GLOBAL_IDENTIFIER:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                az_descr[object_index].global_identifier =
                    value.type.Unsigned_Int;
            }
            break;
        case PROP_RELIABILITY:
            if (Access_Zone_Out_Of_Service(wp_data->object_instance)) {
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
                if (status) {
                    az_descr[object_index].reliability =
                        (BACNET_RELIABILITY)value.type.Enumerated;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
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
