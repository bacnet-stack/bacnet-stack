/**
 * @file
 * @brief test BACnet Text utility API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/property.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bactext_tests, testBacText)
#else
static void testBacText(void)
#endif
{
    uint32_t i, j, k;
    const char *pString;
    char ascii_number[64] = "";
    uint32_t index, found_index;
    bool status;
    unsigned count;
    BACNET_PROPERTY_ID property;

    /* BACnet_Confirmed_Service_Choice */
    for (i = 0; i < MAX_BACNET_CONFIRMED_SERVICE; i++) {
        pString = bactext_confirmed_service_name(i);
        if (pString) {
            status = bactext_confirmed_service_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }

    /* BACnet_Unconfirmed_Service_Choice */
    for (i = 0; i < MAX_BACNET_UNCONFIRMED_SERVICE; i++) {
        pString = bactext_unconfirmed_service_name(i);
        if (pString) {
            status = bactext_unconfirmed_service_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACNET_REINITIALIZED_STATE */
    for (i = 0; i < BACNET_REINIT_MAX; i++) {
        pString = bactext_reinitialized_state_name(i);
        if (pString) {
            status = bactext_reinitialized_state_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACNET_APPLICATION_TAG */
    for (i = 0; i < BACNET_APPLICATION_TAG_EXTENDED_MAX; i++) {
        pString = bactext_application_tag_name(i);
        if (pString) {
            if (i == MAX_BACNET_APPLICATION_TAG) {
                /* skip no-value */
                continue;
            }
            status = bactext_application_tag_index(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACNET_CHARACTER_STRING_ENCODING */
    for (i = 0; i < MAX_CHARACTER_STRING_ENCODING; i++) {
        pString = bactext_character_string_encoding_name(i);
        if (pString) {
            status = bactext_character_string_encoding_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetObjectType */
    for (i = 0; i < BACNET_OBJECT_TYPE_RESERVED_MIN; i++) {
        pString = bactext_object_type_name(i);
        if (pString) {
            status = bactext_object_type_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
            status = bactext_object_type_index(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
            status = bactext_object_property_strtoul(
                (BACNET_OBJECT_TYPE)i, PROP_OBJECT_TYPE, pString, &found_index);
            zassert_true(status, "i=%u", i);
            zassert_equal(
                index, found_index, "index=%u found_index=%u", index,
                found_index);
        }
        pString = bactext_object_type_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
        pString = bactext_object_type_name_capitalized_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
        pString = bactext_object_type_name_capitalized(i);
        zassert_not_null(pString, "i=%u", i);
        if (pString) {
            status =
                bactext_object_type_name_capitalized_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetObjectType and BACnetPropertyIdentifier */
    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        count = property_list_special_count((BACNET_OBJECT_TYPE)i, PROP_ALL);
        /* BACnetPropertyIdentifier for each object type */
        for (j = 0; j < count; j++) {
            property = property_list_special_property(
                (BACNET_OBJECT_TYPE)i, PROP_ALL, j);
            pString = bactext_property_name(property);
            if (pString) {
                status = bactext_property_index(pString, &index);
                zassert_true(
                    status, "object=%s(%u) property=%s(%u)",
                    bactext_object_type_name(i), i, pString, property);
                zassert_equal(
                    index, property, "index=%u property=%u", index, property);
                /* BACnetPropertyIdentifier enumeration values for specific
                 * object type and property */
                for (k = 0; k < UINT16_MAX; k++) {
                    pString = bactext_object_property_name(
                        (BACNET_OBJECT_TYPE)i, property, k, NULL);
                    if (!pString) {
                        break;
                    }
                    status = bactext_object_property_strtoul(
                        i, property, pString, &found_index);
                    zassert_true(
                        status, "%s i=%u property=%u k=%u", pString, i,
                        property, k);
                    zassert_equal(
                        k, found_index, "%s index=%u found_index=%u", pString,
                        k, found_index);
                }
            }
            pString = bactext_property_name_default(property, NULL);
            zassert_not_null(
                pString, "object=%s(%u) property=%s(%u)",
                bactext_object_type_name(i), i, pString, property);
            index = bactext_property_id(pString);
            zassert_equal(
                index, property, "index=%u property=%u", index, property);
            status = bactext_property_strtol(pString, &found_index);
            zassert_true(status, "i=%u", i);
            zassert_equal(
                index, found_index, "index=%u found_index=%u", index,
                found_index);
        }
    }
    /* non-existant property of the device object */
    pString = bactext_object_property_name(
        OBJECT_DEVICE, PROP_FAULT_SIGNALS, 0, NULL);
    zassert_false(pString, NULL);
    pString = bactext_property_name(PROP_PROPRIETARY_RANGE_MIN);
    zassert_not_null(pString, NULL);

    /* BACnetPropertyStates */
    for (k = 0; k < PROP_STATE_PROPRIETARY_MIN; k++) {
        snprintf(ascii_number, sizeof(ascii_number), "%u", k);
        status = bactext_property_states_strtoul(
            (BACNET_PROPERTY_STATES)k, ascii_number, &index);
        zassert_true(status, "k=%u", k);
        zassert_equal(index, k, "index=%u k=%u", index, k);
        /* BACnetPropertyStates enumeration values */
        for (i = 0; i < 255; i++) {
            pString = bactext_property_states_name(
                (BACNET_PROPERTY_STATES)k, i, NULL);
            if (!pString) {
                break;
            }
            status = bactext_property_states_strtoul(
                (BACNET_PROPERTY_STATES)k, pString, &found_index);
            zassert_true(status, "%s k=%u i=%u", pString, k, i);
            zassert_equal(
                i, found_index, "state=%u[%u]=%s ==>[%u]", k, i, pString,
                found_index);
        }
    }
    /* BACnetEngineeringUnits */
    for (i = 0; i < UNITS_RESERVED_RANGE_MAX; i++) {
        pString = bactext_engineering_unit_name(i);
        if (pString) {
            status = bactext_engineering_unit_index(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
        pString = bactext_engineering_unit_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
    }
    pString = bactext_engineering_unit_name(UNITS_PROPRIETARY_RANGE_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_engineering_unit_name(UNITS_PROPRIETARY_RANGE_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_engineering_unit_name(UNITS_PROPRIETARY_RANGE_MIN2);
    zassert_not_null(pString, NULL);
    pString = bactext_engineering_unit_name(UNITS_PROPRIETARY_RANGE_MAX2);
    zassert_not_null(pString, NULL);
    pString = bactext_engineering_unit_name(UNITS_PROPRIETARY_RANGE_MAX2 + 1);
    zassert_not_null(pString, NULL);
    /* BACNET_REJECT_REASON */
    for (i = 0; i < MAX_BACNET_REJECT_REASON; i++) {
        pString = bactext_reject_reason_name(i);
        if (pString) {
            status = bactext_reject_reason_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
        pString = bactext_reject_reason_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
    }
    /* BACNET_ABORT_REASON */
    for (i = 0; i < MAX_BACNET_ABORT_REASON; i++) {
        pString = bactext_abort_reason_name(i);
        if (pString) {
            status = bactext_abort_reason_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
        pString = bactext_abort_reason_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
    }
    /* BACNET_ERROR_CLASS */
    for (i = 0; i < MAX_BACNET_ERROR_CLASS; i++) {
        pString = bactext_error_class_name(i);
        if (pString) {
            status = bactext_error_class_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
        pString = bactext_error_class_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
    }
    /* BACNET_ERROR_CODE */
    for (i = 0; i < ERROR_CODE_RESERVED_MAX; i++) {
        pString = bactext_error_code_name_default(i, NULL);
        if (pString) {
            pString = bactext_error_code_name(i);
            status = bactext_error_code_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        } else {
            printf("No error code name for %u\n", i);
        }
    }
    /* Month value (1-12) */
    for (i = 0; i <= 255; i++) {
        pString = bactext_month_name_default(i, NULL);
        if (pString) {
            pString = bactext_month_name(i);
            status = bactext_month_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        }
    }
    /* Week of month value (1-6) */
    for (i = 0; i <= 255; i++) {
        pString = bactext_week_of_month_name_default(i, NULL);
        if (pString) {
            pString = bactext_week_of_month_name(i);
            status = bactext_week_of_month_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        }
    }
    /* Day of week value (1-7) */
    for (i = 0; i <= 255; i++) {
        pString = bactext_day_of_week_name_default(i, NULL);
        if (pString) {
            pString = bactext_day_of_week_name(i);
            status = bactext_day_of_week_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        }
    }
    /* BACnetDaysOfWeek */
    for (i = 0; i < 8; i++) {
        pString = bactext_days_of_week_name_default(i, NULL);
        if (pString) {
            pString = bactext_days_of_week_name(i);
            status = bactext_days_of_week_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        }
    }
    /* BACnetNotifyType */
    for (i = 0; i < NOTIFY_MAX; i++) {
        pString = bactext_notify_type_name_default(i, NULL);
        if (pString) {
            pString = bactext_notify_type_name(i);
            status = bactext_notify_type_index(pString, &found_index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(
                i, found_index, "i=%u found_index=%u", i, found_index);
            status = bactext_notify_type_strtol(pString, &found_index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(
                i, found_index, "i=%u found_index=%u", i, found_index);
        }
    }
    /* BACnetEventTransitionBits */
    for (i = 0; i < MAX_BACNET_EVENT_TRANSITION; i++) {
        pString = bactext_event_transition_name(i);
        if (pString) {
            status = bactext_event_transition_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetEventState */
    for (i = 0; i < EVENT_STATE_MAX; i++) {
        pString = bactext_event_state_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_event_state_strtol(pString, &index);
        zassert_true(status, "i=%u", i);
        zassert_equal(index, i, "index=%u i=%u", index, i);
        status = bactext_event_state_index(pString, &index);
        zassert_true(status, "i=%u", i);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    /* BACnetEventType */
    for (i = 0; i <= EVENT_CHANGE_OF_TIMER; i++) {
        pString = bactext_event_type_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_event_type_strtol(pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
        status = bactext_event_type_index(pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    /* BACnetBinaryPV */
    for (i = 0; i < BINARY_PV_MAX; i++) {
        pString = bactext_binary_present_value_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_binary_present_value_index(pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    /* BACnetPolarity */
    for (i = 0; i < MAX_POLARITY; i++) {
        pString = bactext_binary_polarity_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_property_states_strtoul(
            PROP_STATE_POLARITY, pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    /* BACnetReliability */
    for (i = 0; i < RELIABILITY_RESERVED_MIN; i++) {
        pString = bactext_reliability_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_property_states_strtoul(
            PROP_STATE_RELIABILITY, pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    /* BACnetDeviceStatus */
    for (i = 0; i < MAX_DEVICE_STATUS; i++) {
        pString = bactext_device_status_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_property_states_strtoul(
            PROP_STATE_SYSTEM_STATUS, pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    /* BACnetSegmentation */
    for (i = 0; i < MAX_BACNET_SEGMENTATION; i++) {
        pString = bactext_segmentation_name(i);
        if (pString) {
            status = bactext_segmentation_index(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetNodeType */
    for (i = 0; i < BACNET_NODE_TYPE_MAX; i++) {
        pString = bactext_node_type_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_NODE_TYPE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetRelationship */
    for (i = 0; i < BACNET_RELATIONSHIP_RESERVED_MIN; i++) {
        pString = bactext_node_relationship_name(i);
        if (pString) {
            status = bactext_node_relationship_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
        status = bactext_node_relationship_name_proprietary(i);
        zassert_false(status, "i=%u", i);
        status = bactext_node_relationship_name_reserved(i);
        zassert_false(status, "i=%u", i);
    }
    status = bactext_node_relationship_name_proprietary(
        BACNET_RELATIONSHIP_PROPRIETARY_MIN);
    zassert_true(status, NULL);
    status = bactext_node_relationship_name_reserved(
        BACNET_RELATIONSHIP_RESERVED_MIN);
    zassert_true(status, NULL);
    /* BACNET_NETWORK_MESSAGE_TYPE */
    for (i = 0; i < NETWORK_MESSAGE_ASHRAE_RESERVED_MIN; i++) {
        pString = bactext_network_layer_msg_name(i);
        if (pString) {
            status = bactext_network_layer_msg_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString =
        bactext_network_layer_msg_name(NETWORK_MESSAGE_ASHRAE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_network_layer_msg_name(NETWORK_MESSAGE_INVALID - 1);
    zassert_not_null(pString, NULL);
    pString = bactext_network_layer_msg_name(NETWORK_MESSAGE_INVALID);
    zassert_not_null(pString, NULL);
    /* BACnetLifeSafetyMode */
    for (i = 0; i < LIFE_SAFETY_MODE_RESERVED_MIN; i++) {
        pString = bactext_life_safety_mode_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIFE_SAFETY_MODE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_life_safety_mode_name(LIFE_SAFETY_MODE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_life_safety_mode_name(LIFE_SAFETY_MODE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_life_safety_mode_name(LIFE_SAFETY_MODE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_life_safety_mode_name(LIFE_SAFETY_MODE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetLifeSafetyOperation */
    for (i = 0; i < LIFE_SAFETY_OP_RESERVED_MIN; i++) {
        pString = bactext_life_safety_operation_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIFE_SAFETY_OPERATION, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_life_safety_operation_name(LIFE_SAFETY_OP_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_life_safety_operation_name(LIFE_SAFETY_OP_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString =
        bactext_life_safety_operation_name(LIFE_SAFETY_OP_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_life_safety_operation_name(LIFE_SAFETY_OP_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetLifeSafetyState */
    for (i = 0; i < LIFE_SAFETY_STATE_RESERVED_MIN; i++) {
        pString = bactext_life_safety_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIFE_SAFETY_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_life_safety_state_name(LIFE_SAFETY_STATE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_life_safety_state_name(LIFE_SAFETY_STATE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_life_safety_state_name(LIFE_SAFETY_STATE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_life_safety_state_name(LIFE_SAFETY_STATE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetSilencedState */
    for (i = 0; i < SILENCED_STATE_RESERVED_MIN; i++) {
        pString = bactext_silenced_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_SILENCED_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_silenced_state_name(SILENCED_STATE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_silenced_state_name(SILENCED_STATE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_silenced_state_name(SILENCED_STATE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_silenced_state_name(SILENCED_STATE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetLightingInProgress */
    for (i = 0; i < MAX_BACNET_LIGHTING_IN_PROGRESS; i++) {
        pString = bactext_lighting_in_progress(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIGHTING_IN_PROGRESS, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_lighting_in_progress(MAX_BACNET_LIGHTING_IN_PROGRESS);
    zassert_not_null(pString, NULL);
    /* BACnetLightingTransition */
    for (i = 0; i < BACNET_LIGHTING_TRANSITION_RESERVED_MIN; i++) {
        pString = bactext_lighting_transition(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIGHTING_TRANSITION, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString =
        bactext_lighting_transition(BACNET_LIGHTING_TRANSITION_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_lighting_transition(BACNET_LIGHTING_TRANSITION_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString =
        bactext_lighting_transition(BACNET_LIGHTING_TRANSITION_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_lighting_transition(
        BACNET_LIGHTING_TRANSITION_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetLightingOperation */
    for (i = 0; i < BACNET_LIGHTS_RESERVED_MIN; i++) {
        pString = bactext_lighting_operation_name(i);
        if (pString) {
            status = bactext_lighting_operation_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_lighting_operation_name(BACNET_LIGHTS_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_lighting_operation_name(BACNET_LIGHTS_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_lighting_operation_name(BACNET_LIGHTS_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_lighting_operation_name(BACNET_LIGHTS_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetBinaryLightingPV */
    for (i = 0; i < BINARY_LIGHTING_PV_MAX; i++) {
        pString = bactext_binary_lighting_pv_name(i);
        if (pString) {
            status = bactext_binary_lighting_pv_names_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_binary_lighting_pv_name(BINARY_LIGHTING_PV_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_binary_lighting_pv_name(BINARY_LIGHTING_PV_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString =
        bactext_binary_lighting_pv_name(BINARY_LIGHTING_PV_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_binary_lighting_pv_name(BINARY_LIGHTING_PV_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetColorOperation */
    for (i = 0; i < BACNET_COLOR_OPERATION_MAX; i++) {
        pString = bactext_color_operation_name(i);
        if (pString) {
            status = bactext_color_operation_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_color_operation_name(BACNET_COLOR_OPERATION_MAX);
    zassert_not_null(pString, NULL);
    /* DeviceCommunicationControl-Request enable-disable */
    for (i = 0; i < MAX_BACNET_COMMUNICATION_ENABLE_DISABLE; i++) {
        pString = bactext_device_communications_name(i);
        if (pString) {
            status = bactext_device_communications_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_device_communications_name(
        MAX_BACNET_COMMUNICATION_ENABLE_DISABLE);
    zassert_not_null(pString, NULL);
    /* BACnetShedState */
    for (i = 0; i < BACNET_SHED_STATE_MAX; i++) {
        pString = bactext_shed_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_SHED_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetLogDatum */
    for (i = 0; i < BACNET_LOG_DATUM_MAX; i++) {
        pString = bactext_log_datum_name(i);
        if (pString) {
            status = bactext_log_datum_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetRestartReason */
    for (i = 0; i < RESTART_REASON_RESERVED_MIN; i++) {
        pString = bactext_restart_reason_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_RESTART_REASON, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_restart_reason_name(RESTART_REASON_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_restart_reason_name(RESTART_REASON_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_restart_reason_name(RESTART_REASON_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_restart_reason_name(RESTART_REASON_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetShedState */
    for (i = 0; i < BACNET_SHED_STATE_MAX; i++) {
        pString = bactext_shed_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_SHED_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetShedLevelType */
    for (i = 0; i < BACNET_SHED_LEVEL_TYPE_MAX; i++) {
        pString = bactext_shed_level_type_name(i);
        if (pString) {
            status = bactext_shed_level_type_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetNetworkType */
    for (i = 0; i < PORT_TYPE_RESERVED_MIN; i++) {
        pString = bactext_network_port_type_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_NETWORK_TYPE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_network_port_type_name(PORT_TYPE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_network_port_type_name(PORT_TYPE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_network_port_type_name(PORT_TYPE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_network_port_type_name(PORT_TYPE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetNetworkNumberQuality */
    for (i = 0; i < PORT_QUALITY_MAX; i++) {
        pString = bactext_network_number_quality_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_NETWORK_NUMBER_QUALITY, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetProtocolLevel */
    for (i = 0; i < BACNET_PROTOCOL_LEVEL_MAX; i++) {
        pString = bactext_protocol_level_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_PROTOCOL_LEVEL, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetNetworkPortCommand */
    for (i = 0; i < PORT_COMMAND_RESERVED_MIN; i++) {
        pString = bactext_network_port_command_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_NETWORK_PORT_COMMAND, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_network_port_command_name(PORT_COMMAND_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_network_port_command_name(PORT_COMMAND_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_network_port_command_name(PORT_COMMAND_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_network_port_command_name(PORT_COMMAND_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetAuthenticationDecision */
    for (i = 0; i < BACNET_AUTHENTICATION_DECISION_MAX; i++) {
        pString = bactext_authentication_decision_name(i);
        if (pString) {
            status = bactext_authentication_decision_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetAuthorizationPosture */
    for (i = 0; i < BACNET_AUTHORIZATION_POSTURE_MAX; i++) {
        pString = bactext_authorization_posture_name(i);
        if (pString) {
            status = bactext_authorization_posture_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetFaultType */
    for (i = 0; i < BACNET_FAULT_TYPE_MAX; i++) {
        pString = bactext_fault_type_name(i);
        if (pString) {
            status = bactext_fault_type_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetPriorityFilter */
    for (i = 0; i < BACNET_PRIORITY_FILTER_MAX; i++) {
        pString = bactext_priority_filter_name(i);
        if (pString) {
            status = bactext_priority_filter_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetResultFlags */
    for (i = 0; i < BACNET_RESULT_FLAGS_MAX; i++) {
        pString = bactext_result_flags_name(i);
        if (pString) {
            status = bactext_result_flags_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetSuccessFilter */
    for (i = 0; i < BACNET_SUCCESS_FILTER_MAX; i++) {
        pString = bactext_success_filter_name(i);
        if (pString) {
            status = bactext_success_filter_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetLoggingType */
    for (i = 0; i < BACNET_LOGGING_TYPE_MAX; i++) {
        pString = bactext_logging_type_name(i);
        if (pString) {
            status = bactext_logging_type_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetProgramRequest */
    for (i = 0; i < PROGRAM_REQUEST_MAX; i++) {
        pString = bactext_program_request_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_PROGRAM_CHANGE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetProgramState */
    for (i = 0; i < PROGRAM_STATE_MAX; i++) {
        pString = bactext_program_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_PROGRAM_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetProgramError */
    for (i = 0; i < PROGRAM_ERROR_RESERVED_MIN; i++) {
        pString = bactext_program_error_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_REASON_FOR_HALT, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_program_error_name(PROGRAM_ERROR_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_program_error_name(PROGRAM_ERROR_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_program_error_name(PROGRAM_ERROR_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_program_error_name(PROGRAM_ERROR_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetTimerState */
    for (i = 0; i < TIMER_STATE_MAX; i++) {
        pString = bactext_timer_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_TIMER_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetTimerTransition */
    for (i = 0; i < TIMER_TRANSITION_MAX; i++) {
        pString = bactext_timer_transition_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_TIMER_TRANSITION, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetBooleanValue */
    for (i = 0; i < 2; i++) {
        pString = bactext_boolean_value_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_BOOLEAN_VALUE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetAction */
    for (i = 0; i < BACNET_ACTION_MAX; i++) {
        pString = bactext_action_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_ACTION, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetFileAccessMethod */
    for (i = 0; i < BACNET_FILE_ACCESS_METHOD_MAX; i++) {
        pString = bactext_file_access_method_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_FILE_ACCESS_METHOD, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetLockStatus */
    for (i = 0; i < BACNET_LOCK_STATUS_MAX; i++) {
        pString = bactext_lock_status_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LOCK_STATUS, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetDoorAlarmState */
    for (i = 0; i < DOOR_ALARM_STATE_RESERVED_MIN; i++) {
        pString = bactext_door_alarm_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_DOOR_ALARM_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_door_alarm_state_name(DOOR_ALARM_STATE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_door_alarm_state_name(DOOR_ALARM_STATE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_door_alarm_state_name(DOOR_ALARM_STATE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_door_alarm_state_name(DOOR_ALARM_STATE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetDoorStatus */
    for (i = 0; i < DOOR_STATUS_RESERVED_MIN; i++) {
        pString = bactext_door_status_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_DOOR_STATUS, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_door_status_name(DOOR_STATUS_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_door_status_name(DOOR_STATUS_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_door_status_name(DOOR_STATUS_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_door_status_name(DOOR_STATUS_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetDoorSecuredStatus */
    for (i = 0; i < DOOR_SECURED_STATUS_MAX; i++) {
        pString = bactext_door_secured_status_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_DOOR_SECURED_STATUS, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetAccessEvent */
    for (i = 0; i < ACCESS_EVENT_RESERVED_MIN; i++) {
        pString = bactext_access_event_name_default(i, NULL);
        if (pString) {
            pString = bactext_access_event_name(i);
            if (pString) {
                status = bactext_property_states_strtoul(
                    PROP_STATE_ACCESS_EVENT, pString, &index);
                zassert_true(status, "i=%u", i);
                zassert_equal(index, i, "index=%u i=%u", index, i);
            }
        }
    }
    pString =
        bactext_access_event_name_default(ACCESS_EVENT_RESERVED_MAX, NULL);
    zassert_false(pString, NULL);
    pString =
        bactext_access_event_name_default(ACCESS_EVENT_PROPRIETARY_MIN, NULL);
    zassert_false(pString, NULL);
    pString =
        bactext_access_event_name_default(ACCESS_EVENT_PROPRIETARY_MAX, NULL);
    zassert_false(pString, NULL);
    pString = bactext_access_event_name_default(
        ACCESS_EVENT_PROPRIETARY_MAX + 1, NULL);
    zassert_false(pString, NULL);
    /* BACnetAuthenticationStatus */
    for (i = 0; i < AUTHENTICATION_STATUS_MAX; i++) {
        pString = bactext_authentication_status_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_AUTHENTICATION_STATUS, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetAuthorizationMode */
    for (i = 0; i < AUTHORIZATION_MODE_RESERVED_MIN; i++) {
        pString = bactext_authorization_mode_name(i);
        if (pString) {
            status = bactext_authorization_mode_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetAccessCredentialDisable */
    for (i = 0; i < ACCESS_CREDENTIAL_DISABLE_RESERVED_MIN; i++) {
        pString = bactext_access_credential_disable_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_ACCESS_CRED_DISABLE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_access_credential_disable_name(
        ACCESS_CREDENTIAL_DISABLE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_access_credential_disable_name(
        ACCESS_CREDENTIAL_DISABLE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_access_credential_disable_name(
        ACCESS_CREDENTIAL_DISABLE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_access_credential_disable_name(
        ACCESS_CREDENTIAL_DISABLE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetAccessCredentialDisableReason */
    for (i = 0; i < CREDENTIAL_DISABLED_RESERVED_MIN; i++) {
        pString = bactext_access_credential_disable_reason_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_ACCESS_CRED_DISABLE_REASON, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_access_credential_disable_reason_name(
        CREDENTIAL_DISABLED_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_access_credential_disable_reason_name(
        CREDENTIAL_DISABLED_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_access_credential_disable_reason_name(
        CREDENTIAL_DISABLED_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_access_credential_disable_reason_name(
        CREDENTIAL_DISABLED_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetAccessUserType */
    for (i = 0; i < ACCESS_USER_TYPE_RESERVED_MIN; i++) {
        pString = bactext_access_user_type_name(i);
        if (pString) {
            status = bactext_access_user_type_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_access_user_type_name(ACCESS_USER_TYPE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_access_user_type_name(ACCESS_USER_TYPE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_access_user_type_name(ACCESS_USER_TYPE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_access_user_type_name(ACCESS_USER_TYPE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetAccessZoneOccupancyState */
    for (i = 0; i < ACCESS_ZONE_OCCUPANCY_STATE_RESERVED_MIN; i++) {
        pString = bactext_access_zone_occupancy_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_ZONE_OCCUPANCY_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_access_zone_occupancy_state_name(
        ACCESS_ZONE_OCCUPANCY_STATE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_access_zone_occupancy_state_name(
        ACCESS_ZONE_OCCUPANCY_STATE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_access_zone_occupancy_state_name(
        ACCESS_ZONE_OCCUPANCY_STATE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_access_zone_occupancy_state_name(
        ACCESS_ZONE_OCCUPANCY_STATE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetWriteStatus */
    for (i = 0; i < BACNET_WRITE_STATUS_MAX; i++) {
        pString = bactext_write_status_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_WRITE_STATUS, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetIPMode */
    for (i = 0; i < BACNET_IP_MODE_MAX; i++) {
        pString = bactext_ip_mode_name(i);
        if (pString) {
            status = bactext_ip_mode_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetDoorValue */
    for (i = 0; i < DOOR_VALUE_MAX; i++) {
        pString = bactext_door_value_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_DOOR_VALUE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetMaintenance */
    for (i = 0; i < MAINTENANCE_RESERVED_MIN; i++) {
        pString = bactext_maintenance_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_MAINTENANCE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_maintenance_name(MAINTENANCE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_maintenance_name(MAINTENANCE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_maintenance_name(MAINTENANCE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_maintenance_name(MAINTENANCE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetEscalatorFault */
    for (i = 0; i < ESCALATOR_FAULT_RESERVED_MIN; i++) {
        pString = bactext_escalator_fault_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_ESCALATOR_FAULT, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetEscalatorMode */
    for (i = 0; i < ESCALATOR_MODE_RESERVED_MIN; i++) {
        pString = bactext_escalator_mode_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_ESCALATOR_MODE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_escalator_mode_name(ESCALATOR_MODE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_escalator_mode_name(ESCALATOR_MODE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_escalator_mode_name(ESCALATOR_MODE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_escalator_mode_name(ESCALATOR_MODE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetEscalatorOperationDirection */
    for (i = 0; i < ESCALATOR_OPERATION_DIRECTION_RESERVED_MIN; i++) {
        pString = bactext_escalator_operation_direction_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_ESCALATOR_OPERATION_DIRECTION, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_escalator_operation_direction_name(
        ESCALATOR_OPERATION_DIRECTION_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_escalator_operation_direction_name(
        ESCALATOR_OPERATION_DIRECTION_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_escalator_operation_direction_name(
        ESCALATOR_OPERATION_DIRECTION_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_escalator_operation_direction_name(
        ESCALATOR_OPERATION_DIRECTION_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetBackupState */
    for (i = 0; i < BACKUP_STATE_MAX; i++) {
        pString = bactext_backup_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_BACKUP_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetSecurityLevel */
    for (i = 0; i < BACNET_SECURITY_LEVEL_MAX; i++) {
        pString = bactext_security_level_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_SECURITY_LEVEL, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetLiftCarDirection */
    for (i = 0; i < LIFT_CAR_DIRECTION_RESERVED_MIN; i++) {
        pString = bactext_lift_car_direction_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIFT_CAR_DIRECTION, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_lift_car_direction_name(LIFT_CAR_DIRECTION_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_lift_car_direction_name(LIFT_CAR_DIRECTION_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString =
        bactext_lift_car_direction_name(LIFT_CAR_DIRECTION_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString =
        bactext_lift_car_direction_name(LIFT_CAR_DIRECTION_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetLiftCarDoorCommand */
    for (i = 0; i < LIFT_CAR_DOOR_COMMAND_MAX; i++) {
        pString = bactext_lift_car_door_command_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIFT_CAR_DOOR_COMMAND, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetLiftCarDriveStatus */
    for (i = 0; i < LIFT_CAR_DRIVE_STATUS_RESERVED_MIN; i++) {
        pString = bactext_lift_car_drive_status_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIFT_CAR_DRIVE_STATUS, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString =
        bactext_lift_car_drive_status_name(LIFT_CAR_DRIVE_STATUS_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_lift_car_drive_status_name(
        LIFT_CAR_DRIVE_STATUS_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_lift_car_drive_status_name(
        LIFT_CAR_DRIVE_STATUS_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_lift_car_drive_status_name(
        LIFT_CAR_DRIVE_STATUS_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetLiftCarMode */
    for (i = 0; i < LIFT_CAR_MODE_RESERVED_MIN; i++) {
        pString = bactext_lift_car_mode_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIFT_CAR_MODE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_lift_car_mode_name(LIFT_CAR_MODE_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_lift_car_mode_name(LIFT_CAR_MODE_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_lift_car_mode_name(LIFT_CAR_MODE_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_lift_car_mode_name(LIFT_CAR_MODE_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetLiftFault */
    for (i = 0; i < LIFT_FAULT_RESERVED_MIN; i++) {
        pString = bactext_lift_fault_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIFT_FAULT, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_lift_fault_name(LIFT_FAULT_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_lift_fault_name(LIFT_FAULT_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_lift_fault_name(LIFT_FAULT_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_lift_fault_name(LIFT_FAULT_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetLiftGroupMode */
    for (i = 0; i < LIFT_GROUP_MODE_MAX; i++) {
        pString = bactext_lift_group_mode_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_LIFT_GROUP_MODE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetAuditLevel */
    for (i = 0; i < AUDIT_LEVEL_RESERVED_MIN; i++) {
        pString = bactext_audit_level_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_AUDIT_LEVEL, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_audit_level_name(AUDIT_LEVEL_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_audit_level_name(AUDIT_LEVEL_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_audit_level_name(AUDIT_LEVEL_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_audit_level_name(AUDIT_LEVEL_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetAuditOperation */
    for (i = 0; i < AUDIT_OPERATION_RESERVED_MIN; i++) {
        pString = bactext_audit_operation_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_AUDIT_OPERATION, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    pString = bactext_audit_operation_name(AUDIT_OPERATION_RESERVED_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_audit_operation_name(AUDIT_OPERATION_PROPRIETARY_MIN);
    zassert_not_null(pString, NULL);
    pString = bactext_audit_operation_name(AUDIT_OPERATION_PROPRIETARY_MAX);
    zassert_not_null(pString, NULL);
    pString = bactext_audit_operation_name(AUDIT_OPERATION_PROPRIETARY_MAX + 1);
    zassert_not_null(pString, NULL);
    /* BACnetSCHubConnectorState */
    for (i = 0; i < BACNET_SC_HUB_CONNECTOR_STATE_MAX; i++) {
        pString = bactext_sc_hub_connector_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_SC_HUB_CONNECTOR_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    /* BACnetSCConnectionState */
    for (i = 0; i < BACNET_SC_CONNECTION_STATE_MAX; i++) {
        pString = bactext_sc_connection_state_name(i);
        if (pString) {
            status = bactext_property_states_strtoul(
                PROP_STATE_SC_CONNECTION_STATE, pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    status = bactext_object_property_strtoul(
        OBJECT_DEVICE, PROP_PRESENT_VALUE, "8", &found_index);
    zassert_true(status, NULL);
    zassert_equal(found_index, 8, NULL);
    status = bactext_object_property_strtoul(
        OBJECT_DEVICE, PROP_TRACKING_VALUE, "7", &found_index);
    zassert_true(status, NULL);
    zassert_equal(found_index, 7, NULL);
    status = bactext_object_property_strtoul(
        OBJECT_DEVICE, PROP_FEEDBACK_VALUE, "6", &found_index);
    zassert_true(status, NULL);
    zassert_equal(found_index, 6, NULL);
    status = bactext_object_property_strtoul(
        OBJECT_DEVICE, PROP_FAULT_SIGNALS, "5", &found_index);
    zassert_true(status, NULL);
    zassert_equal(found_index, 5, NULL);
}
/*/**
 * @brief
 *
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bactext_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bactext_tests, ztest_unit_test(testBacText));

    ztest_run_test_suite(bactext_tests);
}
#endif
