/**
 * @file
 * @brief A basic BACnet Access Door Objects implementation.
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
#include "access_door.h"
#include "bacnet/basic/services.h"

static bool Access_Door_Initialized = false;

static ACCESS_DOOR_DESCR ad_descr[MAX_ACCESS_DOORS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    PROP_DOOR_PULSE_TIME,
    PROP_DOOR_EXTENDED_PULSE_TIME,
    PROP_DOOR_OPEN_TOO_LONG_TIME,
    -1
};

static const int32_t Properties_Optional[] = {
    PROP_DOOR_STATUS,      PROP_LOCK_STATUS,
    PROP_SECURED_STATUS,   PROP_DOOR_UNLOCK_DELAY_TIME,
    PROP_DOOR_ALARM_STATE, -1
};

static const int32_t Properties_Proprietary[] = { -1 };

void Access_Door_Property_Lists(
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

/**
 * @brief Determine if a priority-array slot is relinquished
 * @param object_instance [in] BACnet network port object instance number
 * @param  priority - priority-array index value 1..16
 * @return true if the priority-array slot is relinquished
 */
bool Access_Door_Priority_Array_Relinquished(
    uint32_t object_instance, unsigned priority)
{
    bool status = false;
    unsigned index = 0;

    index = Access_Door_Instance_To_Index(object_instance);
    if (index < MAX_ACCESS_DOORS) {
        if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY)) {
            if (!ad_descr[index].value_active[priority - 1]) {
                status = true;
            }
        }
    }

    return status;
}

/**
 * @brief Get the priority-array value from its slot
 * @param object_instance [in] BACnet network port object instance number
 * @param  priority - priority-array index value 1..16
 * @return priority-array value from its slot
 */
BACNET_DOOR_VALUE
Access_Door_Priority_Array_Value(uint32_t object_instance, unsigned priority)
{
    BACNET_DOOR_VALUE value = DOOR_VALUE_LOCK;
    unsigned index = 0;

    index = Access_Door_Instance_To_Index(object_instance);
    if (index < MAX_ACCESS_DOORS) {
        if ((priority >= 1) && (priority <= BACNET_MAX_PRIORITY)) {
            value = ad_descr[index].priority_array[priority - 1];
        }
    }

    return value;
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
static int Access_Door_Priority_Array_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    unsigned object_index = 0;

    object_index = Access_Door_Instance_To_Index(object_instance);
    if (object_index < MAX_ACCESS_DOORS) {
        if (ad_descr[object_index].value_active[array_index]) {
            apdu_len = encode_application_null(apdu);
        } else {
            apdu_len = encode_application_enumerated(
                apdu, ad_descr[object_index].priority_array[array_index]);
        }
    }

    return apdu_len;
}

/* note: the object name must be unique within this device */
bool Access_Door_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;

    if (object_instance < MAX_ACCESS_DOORS) {
        snprintf(
            text, sizeof(text), "ACCESS DOOR %lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
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
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index = 0;
    bool state = false;
    uint8_t *apdu = NULL;
    int apdu_size = 0;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
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
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Access_Door_Priority_Array_Encode, BACNET_MAX_PRIORITY, apdu,
                apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_RELINQUISH_DEFAULT:
            apdu_len = encode_application_enumerated(
                &apdu[0],
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

    return apdu_len;
}

/* returns true if successful */
bool Access_Door_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
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
    object_index = Access_Door_Instance_To_Index(wp_data->object_instance);
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                status = Access_Door_Present_Value_Set(
                    wp_data->object_instance,
                    (BACNET_DOOR_VALUE)value.type.Enumerated,
                    wp_data->priority);
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
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_NULL);
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
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Access_Door_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_DOOR_STATUS:
            if (Access_Door_Out_Of_Service(wp_data->object_instance)) {
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
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
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
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
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
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
