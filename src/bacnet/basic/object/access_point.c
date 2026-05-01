/**
 * @file
 * @brief A basic BACnet Access Point Objects implementation.
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
#include "access_point.h"
#include "bacnet/basic/services.h"
/* BACnet Stack Objects */
#include "bacnet/basic/object/device.h"

static bool Access_Point_Initialized = false;

static ACCESS_POINT_DESCR ap_descrs[MAX_NUM_DEVICES][MAX_ACCESS_POINTS];
#ifdef BAC_ROUTING
#define ap_descr (ap_descrs[Routed_Device_Object_Index()])
#else
#define ap_descr (ap_descrs[0])
#endif

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,
    PROP_AUTHENTICATION_STATUS,
    PROP_ACTIVE_AUTHENTICATION_POLICY,
    PROP_NUMBER_OF_AUTHENTICATION_POLICIES,
    PROP_AUTHORIZATION_MODE,
    PROP_ACCESS_EVENT,
    PROP_ACCESS_EVENT_TAG,
    PROP_ACCESS_EVENT_TIME,
    PROP_ACCESS_EVENT_CREDENTIAL,
    PROP_ACCESS_DOORS,
    PROP_PRIORITY_FOR_WRITING,
    -1
};

static const int32_t Properties_Optional[] = { -1 };

static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of writable properties */
    PROP_OUT_OF_SERVICE,
    -1,
};

void Access_Point_Property_Lists(
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
 * @brief Get the list of writable properties for an Access Point object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Access_Point_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

void Access_Point_Init(void)
{
    unsigned i;
    uint16_t dev_id;
#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
#endif

    if (!Access_Point_Initialized) {
        Access_Point_Initialized = true;

        for (dev_id = 0; dev_id < MAX_NUM_DEVICES; dev_id++) {
#ifdef BAC_ROUTING
            Set_Routed_Device_Object_Index(dev_id);
#endif
            for (i = 0; i < MAX_ACCESS_POINTS; i++) {
                ap_descr[i].event_state = EVENT_STATE_NORMAL;
                ap_descr[i].reliability = RELIABILITY_NO_FAULT_DETECTED;
                ap_descr[i].out_of_service = false;
                ap_descr[i].authentication_status =
                    AUTHENTICATION_STATUS_NOT_READY;
                ap_descr[i].active_authentication_policy = 0;
                ap_descr[i].number_of_authentication_policies = 0;
                ap_descr[i].authorization_mode = AUTHORIZATION_MODE_AUTHORIZE;
                ap_descr[i].access_event = ACCESS_EVENT_NONE;
                /* timestamp uninitialized */
                /* access_event_credential should be set to some meaningful
                 * value */
                ap_descr[i].num_doors = 0;
                /* fill in the access doors with proper ids */
                ap_descr[i].priority_for_writing =
                    16; /* lowest possible for now */
            }
        }
    }

#ifdef BAC_ROUTING
    Set_Routed_Device_Object_Index(current_dev_id);
#endif

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
    char text[32] = "";
    bool status = false;

    if (object_instance < MAX_ACCESS_POINTS) {
        snprintf(
            text, sizeof(text), "ACCESS POINT %lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
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

/**
 * @brief For a given object instance-number, returns the number of
 * Subordinate_List elements
 * @param  object_instance - object-instance number of the object
 * @return number of subordinate list elements
 */
unsigned Access_Point_Access_Doors_Count(uint32_t object_instance)
{
    unsigned count = 0;
    unsigned object_index = 0;

    object_index = Access_Point_Instance_To_Index(object_instance);
    if (object_index < MAX_ACCESS_POINTS) {
        count = ap_descr[object_index].num_doors;
    }

    return count;
}

/**
 * @brief For a given object instance-number, returns the number of
 * Subordinate_List elements
 * @param  object_instance - object-instance number of the object
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @return Subordinate_List element or NULL if not found
 */
BACNET_DEVICE_OBJECT_REFERENCE *Access_Point_Access_Doors_Member(
    uint32_t instance, BACNET_ARRAY_INDEX array_index)
{
    BACNET_DEVICE_OBJECT_REFERENCE *element = NULL;
    unsigned object_index = 0;

    object_index = Access_Point_Instance_To_Index(instance);
    if (object_index < MAX_ACCESS_POINTS) {
        if (array_index < ap_descr[object_index].num_doors) {
            element = &ap_descr[object_index].access_doors[array_index];
        }
    }

    return element;
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
int Access_Point_Access_Doors_Member_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_DEVICE_OBJECT_REFERENCE *element = NULL;

    element = Access_Point_Access_Doors_Member(object_instance, array_index);
    if (element) {
        apdu_len = bacapp_encode_device_obj_ref(apdu, element);
    }

    return apdu_len;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Access_Point_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index = 0;
    bool state = false;
    uint8_t *apdu = NULL;
    int apdu_size = 0;
    unsigned count = 0;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
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
            apdu_len = encode_application_unsigned(
                &apdu[0],
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
            count = Access_Point_Access_Doors_Count(rpdata->object_instance);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Access_Point_Access_Doors_Member_Encode, count, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_PRIORITY_FOR_WRITING:
            apdu_len = encode_application_unsigned(
                &apdu[0], ap_descr[object_index].priority_for_writing);
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
bool Access_Point_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

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
    switch (wp_data->object_property) {
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Access_Point_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
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
