/**
 * @file
 * @brief BACnet text enumerations lookup functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_TEXT_H
#define BACNET_TEXT_H

/* tiny implementations have no need to print */
#if PRINT_ENABLED
#define BACTEXT_PRINT_ENABLED
#endif

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/indtext.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
const char *bactext_confirmed_service_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_unconfirmed_service_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_application_tag_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_application_tag_index(
    const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
const char *bactext_object_type_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_object_type_index(const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
bool bactext_object_type_strtol(const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
bool bactext_property_name_proprietary(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_property_name(unsigned index);
BACNET_STACK_EXPORT
const char *
bactext_property_name_default(unsigned index, const char *default_string);
BACNET_STACK_EXPORT
bool bactext_property_index(const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
bool bactext_property_strtol(const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
bool bactext_engineering_unit_name_proprietary(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_engineering_unit_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_engineering_unit_index(
    const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
const char *bactext_reject_reason_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_abort_reason_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_error_class_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_error_code_name(unsigned index);
BACNET_STACK_EXPORT
unsigned bactext_property_id(const char *name);
BACNET_STACK_EXPORT
const char *bactext_month_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_week_of_month_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_day_of_week_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_notify_type_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_notify_type_index(const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
const char *bactext_event_state_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_event_state_index(const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
bool bactext_event_state_strtol(const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
const char *bactext_event_type_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_event_type_index(const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
const char *bactext_binary_present_value_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_binary_polarity_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_binary_present_value_index(
    const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
const char *bactext_reliability_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_device_status_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_segmentation_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_segmentation_index(const char *search_name, unsigned *found_index);
BACNET_STACK_EXPORT
const char *bactext_node_type_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_character_string_encoding_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_event_transition_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_event_transition_index(
    const char *search_name, unsigned *found_index);

BACNET_STACK_EXPORT
const char *bactext_days_of_week_name(unsigned index);
BACNET_STACK_EXPORT
bool bactext_days_of_week_index(const char *search_name, unsigned *found_index);

BACNET_STACK_EXPORT
const char *bactext_network_layer_msg_name(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_life_safety_mode_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_life_safety_operation_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_life_safety_state_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_silenced_state_name(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_device_communications_name(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_lighting_operation_name(unsigned index);

BACNET_STACK_EXPORT
bool bactext_lighting_operation_strtol(
    const char *search_name, unsigned *found_index);

BACNET_STACK_EXPORT
const char *bactext_binary_lighting_pv_name(unsigned index);

BACNET_STACK_EXPORT
bool bactext_binary_lighting_pv_names_strtol(
    const char *search_name, unsigned *found_index);

BACNET_STACK_EXPORT
const char *bactext_lighting_in_progress(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_lighting_transition(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_color_operation_name(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_shed_state_name(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_shed_level_type_name(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_log_datum_name(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_restart_reason_name(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_network_port_type_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_network_number_quality_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_network_port_command_name(unsigned index);

BACNET_STACK_EXPORT
const char *bactext_authentication_decision_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_authorization_posture_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_fault_type_name(unsigned index);
BACNET_STACK_EXPORT
const char *bacnet_priority_filter_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_success_filter_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_result_flags_name(unsigned index);
BACNET_STACK_EXPORT
const char *bactext_logging_type_name(unsigned index);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
