/**
 * @file
 * @brief Property_List property encode decode helper
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/rpm.h"
#include "bacnet/rp.h"
#include "bacnet/proplist.h"

/**
 * Function that returns the number of BACnet object properties in a list
 *
 * @param pList - array of type 'int32_t' that is a list of BACnet object
 * properties, terminated by a '-1' value.
 */
uint32_t property_list_count(const int32_t *pList)
{
    uint32_t property_count = 0;

    if (pList) {
        while (*pList != -1) {
            property_count++;
            pList++;
        }
    }

    return property_count;
}

/**
 * For a given object property, returns the true if in the property list
 *
 * @param pList - array of type 'int32_t' that is a list of BACnet object
 * @param object_property - property enumeration or propritary value
 *
 * @return true if object_property is a member of the property list
 */
bool property_list_member(const int32_t *pList, int32_t object_property)
{
    bool status = false;

    if (pList) {
        while ((*pList) != -1) {
            if (object_property == (*pList)) {
                status = true;
                break;
            }
            pList++;
        }
    }

    return status;
}

/**
 * @brief Determine if the object property is a member of any of the lists
 * @param pRequired - array of type 'int32_t' that is a list of BACnet
 * properties
 * @param pOptional - array of type 'int32_t' that is a list of BACnet
 * properties
 * @param pProprietary - array of type 'int32_t' that is a list of BACnet
 * properties
 * @param object_property - object-property to be checked
 * @return true if the property is a member of any of these lists
 */
bool property_lists_member(
    const int32_t *pRequired,
    const int32_t *pOptional,
    const int32_t *pProprietary,
    int32_t object_property)
{
    bool found = false;

    found = property_list_member(pRequired, object_property);
    if (!found) {
        found = property_list_member(pOptional, object_property);
    }
    if (!found) {
        found = property_list_member(pProprietary, object_property);
    }

    return found;
}

/**
 * ReadProperty handler for this property.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - ReadProperty data, including requested data and
 * data for the reply, or error response.
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int property_list_encode(
    BACNET_READ_PROPERTY_DATA *rpdata,
    const int32_t *pListRequired,
    const int32_t *pListOptional,
    const int32_t *pListProprietary)
{
    int apdu_len = 0; /* return value */
    uint8_t *apdu = NULL;
    int max_apdu_len = 0;
    uint32_t count = 0;
    uint32_t required_count = 0;
    uint32_t optional_count = 0;
    uint32_t proprietary_count = 0;
    int len = 0;
    uint32_t i = 0; /* loop index */

    required_count = property_list_count(pListRequired);
    optional_count = property_list_count(pListOptional);
    proprietary_count = property_list_count(pListProprietary);
    /* total of all counts */
    count = required_count + optional_count + proprietary_count;
    if (required_count >= 3) {
        /* less the 3 always required properties */
        count -= 3;
        if (property_list_member(pListRequired, PROP_PROPERTY_LIST)) {
            /* property-list should not be in the required list
               because this module handles it transparently.
               Handle the case where it might be in the required list. */
            count -= 1;
        }
    }
    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    max_apdu_len = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_PROPERTY_LIST:
            if (rpdata->array_index == 0) {
                /* Array element zero is the number of elements in the array */
                apdu_len = encode_application_unsigned(&apdu[0], count);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                /* if no index was specified, then try to encode the entire list
                 */
                /* into one packet. */
                if (required_count > 3) {
                    for (i = 0; i < required_count; i++) {
                        if ((pListRequired[i] == PROP_OBJECT_TYPE) ||
                            (pListRequired[i] == PROP_OBJECT_IDENTIFIER) ||
                            (pListRequired[i] == PROP_OBJECT_NAME) ||
                            (pListRequired[i] == PROP_PROPERTY_LIST)) {
                            continue;
                        } else {
                            len = encode_application_enumerated(
                                &apdu[apdu_len], (uint32_t)pListRequired[i]);
                        }
                        /* add it if we have room */
                        if ((apdu_len + len) < max_apdu_len) {
                            apdu_len += len;
                        } else {
                            rpdata->error_code =
                                ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                            apdu_len = BACNET_STATUS_ABORT;
                            break;
                        }
                    }
                }
                if (optional_count) {
                    for (i = 0; i < optional_count; i++) {
                        len = encode_application_enumerated(
                            &apdu[apdu_len], (uint32_t)pListOptional[i]);
                        /* add it if we have room */
                        if ((apdu_len + len) < max_apdu_len) {
                            apdu_len += len;
                        } else {
                            rpdata->error_code =
                                ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                            apdu_len = BACNET_STATUS_ABORT;
                            break;
                        }
                    }
                }
                if (proprietary_count) {
                    for (i = 0; i < proprietary_count; i++) {
                        len = encode_application_enumerated(
                            &apdu[apdu_len], (uint32_t)pListProprietary[i]);
                        /* add it if we have room */
                        if ((apdu_len + len) < max_apdu_len) {
                            apdu_len += len;
                        } else {
                            rpdata->error_code =
                                ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                            apdu_len = BACNET_STATUS_ABORT;
                            break;
                        }
                    }
                }
            } else {
                if (rpdata->array_index <= count) {
                    count = 0;
                    if (required_count > 3) {
                        for (i = 0; i < required_count; i++) {
                            if ((pListRequired[i] == PROP_OBJECT_TYPE) ||
                                (pListRequired[i] == PROP_OBJECT_IDENTIFIER) ||
                                (pListRequired[i] == PROP_OBJECT_NAME) ||
                                (pListRequired[i] == PROP_PROPERTY_LIST)) {
                                continue;
                            } else {
                                count++;
                            }
                            if (count == rpdata->array_index) {
                                apdu_len = encode_application_enumerated(
                                    &apdu[apdu_len],
                                    (uint32_t)pListRequired[i]);
                                break;
                            }
                        }
                    }
                    if ((apdu_len == 0) && (optional_count > 0)) {
                        for (i = 0; i < optional_count; i++) {
                            count++;
                            if (count == rpdata->array_index) {
                                apdu_len = encode_application_enumerated(
                                    &apdu[apdu_len],
                                    (uint32_t)pListOptional[i]);
                                break;
                            }
                        }
                    }
                    if ((apdu_len == 0) && (proprietary_count > 0)) {
                        for (i = 0; i < proprietary_count; i++) {
                            count++;
                            if (count == rpdata->array_index) {
                                apdu_len = encode_application_enumerated(
                                    &apdu[apdu_len],
                                    (uint32_t)pListProprietary[i]);
                                break;
                            }
                        }
                    }
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

    return apdu_len;
}

/**
 * ReadProperty handler for common properties.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - ReadProperty data, including requested data and
 * data for the reply, or error response.
 * @param device_instance_number - device instance number
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int property_list_common_encode(
    BACNET_READ_PROPERTY_DATA *rpdata, uint32_t device_instance_number)
{
    int apdu_len = BACNET_STATUS_ERROR;
    uint8_t *apdu = NULL;

    if (!rpdata) {
        return 0;
    }
    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            /* Device Object exception: requested instance
                may not match our instance if a wildcard */
            if (rpdata->object_type == OBJECT_DEVICE) {
                rpdata->object_instance = device_instance_number;
            }
            apdu_len = encode_application_object_id(
                &apdu[0], rpdata->object_type, rpdata->object_instance);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], rpdata->object_type);
            break;
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Determine if the property is a common property
 * @param property - property value for comparison
 * @return true if the property is a common object property
 */
bool property_list_common(BACNET_PROPERTY_ID property)
{
    bool status = false;

    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_TYPE:
            status = true;
            break;
        default:
            break;
    }

    return status;
}

/* standard properties that are arrays
   but not required to be supported in every object */
static const int32_t Properties_BACnetARRAY[] = {
    /* unordered list of properties */
    PROP_OBJECT_LIST,
    PROP_STRUCTURED_OBJECT_LIST,
    PROP_CONFIGURATION_FILES,
    PROP_PROPERTY_LIST,
    PROP_AUTHENTICATION_FACTORS,
    PROP_ASSIGNED_ACCESS_RIGHTS,
    PROP_ACTION,
    PROP_ACTION_TEXT,
    PROP_PRIORITY_ARRAY,
    PROP_VALUE_SOURCE_ARRAY,
    PROP_COMMAND_TIME_ARRAY,
    PROP_ALARM_VALUES,
    PROP_FAULT_VALUES,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_EVENT_MESSAGE_TEXTS_CONFIG,
    PROP_SUPPORTED_FORMATS,
    PROP_SUPPORTED_FORMAT_CLASSES,
    PROP_SUBORDINATE_LIST,
    PROP_SUBORDINATE_ANNOTATIONS,
    PROP_SUBORDINATE_TAGS,
    PROP_SUBORDINATE_NODE_TYPES,
    PROP_SUBORDINATE_RELATIONSHIPS,
    PROP_GROUP_MEMBERS,
    PROP_GROUP_MEMBER_NAMES,
    PROP_EXECUTION_DELAY,
    PROP_CONTROL_GROUPS,
    PROP_BIT_TEXT,
    PROP_PORT_FILTER,
    PROP_STATE_CHANGE_VALUES,
    PROP_LINK_SPEEDS,
    PROP_IP_DNS_SERVER,
    PROP_IPV6_DNS_SERVER,
    PROP_FLOOR_TEXT,
    PROP_CAR_DOOR_TEXT,
    PROP_ASSIGNED_LANDING_CALLS,
    PROP_MAKING_CAR_CALL,
    PROP_REGISTERED_CAR_CALL,
    PROP_CAR_DOOR_STATUS,
    PROP_CAR_DOOR_COMMAND,
    PROP_LANDING_DOOR_STATUS,
    PROP_STAGES,
    PROP_STAGE_NAMES,
    PROP_STATE_TEXT,
    PROP_TARGET_REFERENCES,
    PROP_MONITORED_OBJECTS,
    PROP_SHED_LEVELS,
    PROP_SHED_LEVEL_DESCRIPTIONS,
    PROP_WEEKLY_SCHEDULE,
    PROP_EXCEPTION_SCHEDULE,
    PROP_TAGS,
    PROP_ISSUER_CERTIFICATE_FILES,
    PROP_NEGATIVE_ACCESS_RULES,
    PROP_POSITIVE_ACCESS_RULES,
#if (INT_MAX > 0xFFFF)
    PROP_SC_HUB_FUNCTION_ACCEPT_URIS,
#endif
    -1
};

/**
 * Function that returns the list of Required properties
 * of known standard objects.
 *
 * @param object_type - enumerated BACNET_OBJECT_TYPE
 * @return returns a pointer to a '-1' terminated array of
 * type 'int32_t' that contain BACnet object properties for the given object
 * type.
 */
const int32_t *property_list_bacnet_array(void)
{
    return Properties_BACnetARRAY;
}

/**
 * @brief Determine if the object property is a BACnetARRAY property
 * @param object_type - object-type to be checked
 * @param object_property - object-property to be checked
 * @return true if the property is a BACnetARRAY property
 */
bool property_list_bacnet_array_member(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID object_property)
{
    /* exceptions where a property is an BACnetARRAY or a BACnetLIST
       only in specific object types */
    switch (object_type) {
        case OBJECT_GLOBAL_GROUP:
            switch (object_property) {
                case PROP_PRESENT_VALUE:
                    return true;
                default:
                    break;
            }
            break;
        case OBJECT_CHANNEL:
            switch (object_property) {
                case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
                    return true;
                default:
                    break;
            }
            break;
        case OBJECT_LOOP:
            switch (object_property) {
                case PROP_ACTION:
                    return false;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    if ((object_property >= PROP_PROPRIETARY_RANGE_MIN) &&
        (object_property <= PROP_PROPRIETARY_RANGE_MAX)) {
        /* all proprietary properties could be a BACnetARRAY */
        return true;
    }

    return property_list_member(Properties_BACnetARRAY, object_property);
}

/* standard properties that are BACnetLIST */
static const int32_t Properties_BACnetLIST[] = {
    /* unordered list of properties */
    PROP_DATE_LIST,
    PROP_VT_CLASSES_SUPPORTED,
    PROP_ACTIVE_VT_SESSIONS,
    PROP_TIME_SYNCHRONIZATION_RECIPIENTS,
    PROP_DEVICE_ADDRESS_BINDING,
    PROP_ACTIVE_COV_SUBSCRIPTIONS,
    PROP_RESTART_NOTIFICATION_RECIPIENTS,
    PROP_UTC_TIME_SYNCHRONIZATION_RECIPIENTS,
    PROP_ACTIVE_COV_MULTIPLE_SUBSCRIPTIONS,
    PROP_LIST_OF_GROUP_MEMBERS,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
    PROP_ACCEPTED_MODES,
    PROP_LIFE_SAFETY_ALARM_VALUES,
    PROP_ALARM_VALUES,
    PROP_FAULT_VALUES,
    PROP_MEMBER_OF,
    PROP_ZONE_MEMBERS,
    PROP_RECIPIENT_LIST,
    PROP_LOG_BUFFER,
    PROP_MASKED_ALARM_VALUES,
    PROP_FAILED_ATTEMPT_EVENTS,
    PROP_ACCESS_ALARM_EVENTS,
    PROP_ACCESS_TRANSACTION_EVENTS,
    PROP_CREDENTIALS_IN_ZONE,
    PROP_ENTRY_POINTS,
    PROP_EXIT_POINTS,
    PROP_MEMBERS,
    PROP_CREDENTIALS,
    PROP_REASON_FOR_DISABLE,
    PROP_AUTHORIZATION_EXEMPTIONS,
    PROP_COVU_RECIPIENTS,
    PROP_SUBSCRIBED_RECIPIENTS,
    PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE,
    PROP_BBMD_FOREIGN_DEVICE_TABLE,
    PROP_MANUAL_SLAVE_ADDRESS_BINDING,
    PROP_SLAVE_ADDRESS_BINDING,
    PROP_VIRTUAL_MAC_ADDRESS_TABLE,
    PROP_ROUTING_TABLE,
    PROP_LANDING_CALLS,
    PROP_FAULT_SIGNALS,
    PROP_ADDITIONAL_REFERENCE_PORTS,
    -1
};

/**
 * Returns the list of BACnetLIST properties of known standard objects.
 *
 * @param object_type - enumerated BACNET_OBJECT_TYPE
 * @return returns a pointer to a '-1' terminated array of
 * type 'int32_t' that contain BACnet object properties for the given object
 * type.
 */
const int32_t *property_list_bacnet_list(void)
{
    return Properties_BACnetLIST;
}

/**
 * @brief Determine if the object property is a BACnetLIST property
 * @param object_type - object-type to be checked
 * @param object_property - object-property to be checked
 * @return true if the property is a BACnetLIST property
 */
bool property_list_bacnet_list_member(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID object_property)
{
    /* exceptions where property is an BACnetLIST only in specific objects */
    switch (object_type) {
        case OBJECT_GROUP:
            switch (object_property) {
                case PROP_PRESENT_VALUE:
                    return true;
                default:
                    break;
            }
            break;
        case OBJECT_CHANNEL:
            switch (object_property) {
                case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
                    return false;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    if ((object_property >= PROP_PROPRIETARY_RANGE_MIN) &&
        (object_property <= PROP_PROPRIETARY_RANGE_MAX)) {
        /* all proprietary properties could be a BACnetLIST */
        return true;
    }

    return property_list_member(Properties_BACnetLIST, object_property);
}

/**
 * @brief Determine if the object property is a commandable member
 *
 * 19.2.1.1 Commandable Properties
 * The prioritization scheme is applied to certain properties of objects.
 * The standard commandable properties and objects are as follows.
 *
 * @param object_type - object-type to be checked
 * @param object_property - object-property to be checked
 * @return true if the property is a commandable member
 */
bool property_list_commandable_member(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID object_property)
{
    bool status = false;

    switch (object_type) {
        case OBJECT_ACCESS_DOOR:
        case OBJECT_ANALOG_OUTPUT:
        case OBJECT_ANALOG_VALUE:
        case OBJECT_BINARY_LIGHTING_OUTPUT:
        case OBJECT_BINARY_OUTPUT:
        case OBJECT_BINARY_VALUE:
        case OBJECT_BITSTRING_VALUE:
        case OBJECT_CHANNEL:
        case OBJECT_CHARACTERSTRING_VALUE:
        case OBJECT_DATE_VALUE:
        case OBJECT_DATE_PATTERN_VALUE:
        case OBJECT_DATETIME_VALUE:
        case OBJECT_DATETIME_PATTERN_VALUE:
        case OBJECT_INTEGER_VALUE:
        case OBJECT_LARGE_ANALOG_VALUE:
        case OBJECT_LIGHTING_OUTPUT:
        case OBJECT_MULTI_STATE_OUTPUT:
        case OBJECT_MULTI_STATE_VALUE:
        case OBJECT_OCTETSTRING_VALUE:
        case OBJECT_POSITIVE_INTEGER_VALUE:
        case OBJECT_TIME_VALUE:
        case OBJECT_TIME_PATTERN_VALUE:
            if (object_property == PROP_PRESENT_VALUE) {
                status = true;
            }
            break;
        default:
            break;
    }

    return status;
}
