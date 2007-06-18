/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005-2006 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#include <stdio.h>
#include "indtext.h"
#include "bacenum.h"

static const char *ASHRAE_Reserved_String = "Reserved for Use by ASHRAE";
static const char *Vendor_Proprietary_String = "Vendor Proprietary Value";

INDTEXT_DATA bacnet_confirmed_service_names[] = {
    {SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, "Acknowledge-Alarm"},
    {SERVICE_CONFIRMED_COV_NOTIFICATION, "COV-Notification"},
    {SERVICE_CONFIRMED_EVENT_NOTIFICATION, "Event-Notification"},
    {SERVICE_CONFIRMED_GET_ALARM_SUMMARY, "Get-Alarm-Summary"},
    {SERVICE_CONFIRMED_GET_ENROLLMENT_SUMMARY, "Get-Enrollment-Summary"},
    {SERVICE_CONFIRMED_SUBSCRIBE_COV, "Subscribe-COV"},
    {SERVICE_CONFIRMED_ATOMIC_READ_FILE, "Atomic-Read-File"},
    {SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, "Atomic-Write-File"},
    {SERVICE_CONFIRMED_ADD_LIST_ELEMENT, "Add-List-Element"},
    {SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT, "Remove-List-Element"},
    {SERVICE_CONFIRMED_CREATE_OBJECT, "Create-Object"},
    {SERVICE_CONFIRMED_DELETE_OBJECT, "Delete-Object"},
    {SERVICE_CONFIRMED_READ_PROPERTY, "Read-Property"},
    {SERVICE_CONFIRMED_READ_PROPERTY_CONDITIONAL,
        "Read-Property-Conditional"},
    {SERVICE_CONFIRMED_READ_PROPERTY_MULTIPLE, "Read-Property-Multiple"},
    {SERVICE_CONFIRMED_WRITE_PROPERTY, "Write-Property"},
    {SERVICE_CONFIRMED_WRITE_PROPERTY_MULTIPLE, "Write-Property-Multiple"},
    {SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        "Device-Communication-Control"},
    {SERVICE_CONFIRMED_PRIVATE_TRANSFER, "Private-Transfer"},
    {SERVICE_CONFIRMED_TEXT_MESSAGE, "Text-Message"},
    {SERVICE_CONFIRMED_REINITIALIZE_DEVICE, "Reinitialize-Device"},
    {SERVICE_CONFIRMED_VT_OPEN, "VT-Open"},
    {SERVICE_CONFIRMED_VT_CLOSE, "VT-Close"},
    {SERVICE_CONFIRMED_VT_DATA, "VT-Data"},
    {SERVICE_CONFIRMED_AUTHENTICATE, "Authenticate"},
    {SERVICE_CONFIRMED_REQUEST_KEY, "Request-Key"},
    {SERVICE_CONFIRMED_READ_RANGE, "Read-Range"},
    {SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION, "Life-Safety_Operation"},
    {SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY, "Subscribe-COV-Property"},
    {SERVICE_CONFIRMED_GET_EVENT_INFORMATION, "Get-Event-Information"},
    {0, NULL}
};

const char *bactext_confirmed_service_name(int index)
{
    return indtext_by_index_default(bacnet_confirmed_service_names,
        index, ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_unconfirmed_service_names[] = {
    {SERVICE_UNCONFIRMED_I_AM, "I-Am"}
    ,
    {SERVICE_UNCONFIRMED_I_HAVE, "I-Have"}
    ,
    {SERVICE_UNCONFIRMED_COV_NOTIFICATION, "COV-Notification"}
    ,
    {SERVICE_UNCONFIRMED_EVENT_NOTIFICATION, "Event-Notification"}
    ,
    {SERVICE_UNCONFIRMED_PRIVATE_TRANSFER, "Private-Transfer"}
    ,
    {SERVICE_UNCONFIRMED_TEXT_MESSAGE, "Text-Message"}
    ,
    {SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, "Time-Synchronization"}
    ,
    {SERVICE_UNCONFIRMED_WHO_HAS, "Who-Has"}
    ,
    {SERVICE_UNCONFIRMED_WHO_IS, "Who-Is"}
    ,
    {SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION,
        "UTC-Time-Synchronization"}
    ,
    {0, NULL}
};

const char *bactext_unconfirmed_service_name(int index)
{
    return indtext_by_index_default(bacnet_unconfirmed_service_names,
        index, ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_application_tag_names[] = {
    {BACNET_APPLICATION_TAG_NULL, "Null"}
    ,
    {BACNET_APPLICATION_TAG_BOOLEAN, "Boolean"}
    ,
    {BACNET_APPLICATION_TAG_UNSIGNED_INT, "Unsigned Int"}
    ,
    {BACNET_APPLICATION_TAG_SIGNED_INT, "Signed Int"}
    ,
    {BACNET_APPLICATION_TAG_REAL, "Real"}
    ,
    {BACNET_APPLICATION_TAG_DOUBLE, "Double"}
    ,
    {BACNET_APPLICATION_TAG_OCTET_STRING, "Octet String"}
    ,
    {BACNET_APPLICATION_TAG_CHARACTER_STRING, "Character String"}
    ,
    {BACNET_APPLICATION_TAG_BIT_STRING, "Bit String"}
    ,
    {BACNET_APPLICATION_TAG_ENUMERATED, "Enumerated"}
    ,
    {BACNET_APPLICATION_TAG_DATE, "Date"}
    ,
    {BACNET_APPLICATION_TAG_TIME, "Time"}
    ,
    {BACNET_APPLICATION_TAG_OBJECT_ID, "Object ID"}
    ,
    {BACNET_APPLICATION_TAG_RESERVED1, "Reserved 1"}
    ,
    {BACNET_APPLICATION_TAG_RESERVED2, "Reserved 2"}
    ,
    {BACNET_APPLICATION_TAG_RESERVED3, "Reserved 3"}
    ,
    {0, NULL}
};

const char *bactext_application_tag_name(int index)
{
    return indtext_by_index_default(bacnet_application_tag_names,
        index, ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_object_type_names[] = {
    {OBJECT_ANALOG_INPUT, "Analog Input"}
    ,
    {OBJECT_ANALOG_OUTPUT, "Analog Output"}
    ,
    {OBJECT_ANALOG_VALUE, "Analog Value"}
    ,
    {OBJECT_BINARY_INPUT, "Binary Input"}
    ,
    {OBJECT_BINARY_OUTPUT, "Binary Output"}
    ,
    {OBJECT_BINARY_VALUE, "Binary Value"}
    ,
    {OBJECT_CALENDAR, "Calendar"}
    ,
    {OBJECT_COMMAND, "Command"}
    ,
    {OBJECT_DEVICE, "Device"}
    ,
    {OBJECT_EVENT_ENROLLMENT, "Event Enrollment"}
    ,
    {OBJECT_FILE, "File"}
    ,
    {OBJECT_GROUP, "Group"}
    ,
    {OBJECT_LOOP, "Loop"}
    ,
    {OBJECT_MULTI_STATE_INPUT, "Multi-State Input"}
    ,
    {OBJECT_MULTI_STATE_OUTPUT, "Multi-State Output"}
    ,
    {OBJECT_NOTIFICATION_CLASS, "Notification Class"}
    ,
    {OBJECT_PROGRAM, "Program"}
    ,
    {OBJECT_SCHEDULE, "Schedule"}
    ,
    {OBJECT_AVERAGING, "Averaging"}
    ,
    {OBJECT_MULTI_STATE_VALUE, "Multi-State Value"}
    ,
    {OBJECT_TRENDLOG, "Trendlog"}
    ,
    {OBJECT_LIFE_SAFETY_POINT, "Life Safety Point"}
    ,
    {OBJECT_LIFE_SAFETY_ZONE, "Life Safety Zone"}
    ,
    {OBJECT_ACCUMULATOR, "Accumulator"}
    ,
    {OBJECT_PULSE_CONVERTER, "Pulse-Converter"}
    ,

    {OBJECT_EVENT_LOG, "Event-Log"}
    ,
    {OBJECT_GLOBAL_GROUP, "Global-Group"}
    ,
    {OBJECT_TREND_LOG_MULTIPLE, "Trend-Log-Multiple"}
    ,
    {OBJECT_LOAD_CONTROL, "Load-Control"}
    ,
    {OBJECT_STRUCTURED_VIEW, "Structured-View"}
    ,

    {0, NULL}
/* Enumerated values 0-127 are reserved for definition by ASHRAE.
   Enumerated values 128-1023 may be used by others subject to
   the procedures and constraints described in Clause 23. */
};

const char *bactext_object_type_name(int index)
{
    return indtext_by_index_split_default(bacnet_object_type_names,
        index, 128, ASHRAE_Reserved_String, Vendor_Proprietary_String);
}

INDTEXT_DATA bacnet_property_names[] = {
/* FIXME: use the enumerations from bacenum.h */
    {PROP_ACKED_TRANSITIONS, "acked-transitions"}
    ,
    {PROP_ACK_REQUIRED, "ack-required"}
    ,
    {PROP_ACTION, "action"}
    ,
    {PROP_ACTION_TEXT, "action-text"}
    ,
    {PROP_ACTIVE_TEXT, "active-text"}
    ,
    {PROP_ACTIVE_VT_SESSIONS, "active-vt-sessions"}
    ,
    {PROP_ALARM_VALUE, "alarm-value"}
    ,
    {PROP_ALARM_VALUES, "alarm-values"}
    ,
    {PROP_ALL, "all"}
    ,
    {PROP_ALL_WRITES_SUCCESSFUL, "all-writes-successful"}
    ,
    {PROP_APDU_SEGMENT_TIMEOUT, "apdu-segment-timeout"}
    ,
    {PROP_APDU_TIMEOUT, "apdu-timeout"}
    ,
    {PROP_APPLICATION_SOFTWARE_VERSION, "application-software-version"}
    ,
    {PROP_ARCHIVE, "archive"}
    ,
    {PROP_BIAS, "bias"}
    ,
    {PROP_CHANGE_OF_STATE_COUNT, "change-of-state-count"}
    ,
    {PROP_CHANGE_OF_STATE_TIME, "change-of-state-time"}
    ,
    {PROP_NOTIFICATION_CLASS, "notification-class"}
    ,
    {PROP_BLANK_1, "(deleted in 135-2001)"}
    ,
    {PROP_CONTROLLED_VARIABLE_REFERENCE, "controlled-variable-reference"}
    ,
    {PROP_CONTROLLED_VARIABLE_UNITS, "controlled-variable-units"}
    ,
    {PROP_CONTROLLED_VARIABLE_VALUE, "controlled-variable-value"}
    ,
    {PROP_COV_INCREMENT, "COV-increment"}
    ,
    {PROP_DATE_LIST, "datelist"}
    ,
    {PROP_DAYLIGHT_SAVINGS_STATUS, "daylight-savings-status"}
    ,
    {PROP_DEADBAND, "deadband"}
    ,
    {PROP_DERIVATIVE_CONSTANT, "derivative-constant"}
    ,
    {PROP_DERIVATIVE_CONSTANT_UNITS, "derivative-constant-units"}
    ,
    {PROP_DESCRIPTION, "description"}
    ,
    {PROP_DESCRIPTION_OF_HALT, "description-of-halt"}
    ,
    {PROP_DEVICE_ADDRESS_BINDING, "device-address-binding"}
    ,
    {PROP_DEVICE_TYPE, "device-type"}
    ,
    {PROP_EFFECTIVE_PERIOD, "effective-period"}
    ,
    {PROP_ELAPSED_ACTIVE_TIME, "elapsed-active-time"}
    ,
    {PROP_ERROR_LIMIT, "error-limit"}
    ,
    {PROP_EVENT_ENABLE, "event-enable"}
    ,
    {PROP_EVENT_STATE, "event-state"}
    ,
    {PROP_EVENT_TYPE, "event-type"}
    ,
    {PROP_EXCEPTION_SCHEDULE, "exception-schedule"}
    ,
    {PROP_FAULT_VALUES, "fault-values"}
    ,
    {PROP_FEEDBACK_VALUE, "feedback-value"}
    ,
    {PROP_FILE_ACCESS_METHOD, "file-access-method"}
    ,
    {PROP_FILE_SIZE, "file-size"}
    ,
    {PROP_FILE_TYPE, "file-type"}
    ,
    {PROP_FIRMWARE_REVISION, "firmware-version"}
    ,
    {PROP_HIGH_LIMIT, "high-limit"}
    ,
    {PROP_INACTIVE_TEXT, "inactive-text"}
    ,
    {PROP_IN_PROCESS, "in-process"}
    ,
    {PROP_INSTANCE_OF, "instance-of"}
    ,
    {PROP_INTEGRAL_CONSTANT, "integral-constant"}
    ,
    {PROP_INTEGRAL_CONSTANT_UNITS, "integral-constant-units"}
    ,
    {PROP_ISSUE_CONFIRMED_NOTIFICATIONS, "issue-confirmednotifications"}
    ,
    {PROP_LIMIT_ENABLE, "limit-enable"}
    ,
    {PROP_LIST_OF_GROUP_MEMBERS, "list-of-group-members"}
    ,
    {PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
        "list-of-object-property-references"}
    ,
    {PROP_LIST_OF_SESSION_KEYS, "list-of-session-keys"}
    ,
    {PROP_LOCAL_DATE, "local-date"}
    ,
    {PROP_LOCAL_TIME, "local-time"}
    ,
    {PROP_LOCATION, "location"}
    ,
    {PROP_LOW_LIMIT, "low-limit"}
    ,
    {PROP_MANIPULATED_VARIABLE_REFERENCE, "manipulated-variable-reference"}
    ,
    {PROP_MAXIMUM_OUTPUT, "maximum-output"}
    ,
    {PROP_MAX_APDU_LENGTH_ACCEPTED, "max-apdu-length-accepted"}
    ,
    {PROP_MAX_INFO_FRAMES, "max-info-frames"}
    ,
    {PROP_MAX_MASTER, "max-master"}
    ,
    {PROP_MAX_PRES_VALUE, "max-pres-value"}
    ,
    {PROP_MINIMUM_OFF_TIME, "minimum-off-time"}
    ,
    {PROP_MINIMUM_ON_TIME, "minimum-on-time"}
    ,
    {PROP_MINIMUM_OUTPUT, "minimum-output"}
    ,
    {PROP_MIN_PRES_VALUE, "min-pres-value"}
    ,
    {PROP_MODEL_NAME, "model-name"}
    ,
    {PROP_MODIFICATION_DATE, "modification-date"}
    ,
    {PROP_NOTIFY_TYPE, "notify-type"}
    ,
    {PROP_NUMBER_OF_APDU_RETRIES, "number-of-APDU-retries"}
    ,
    {PROP_NUMBER_OF_STATES, "number-of-states"}
    ,
    {PROP_OBJECT_IDENTIFIER, "object-identifier"}
    ,
    {PROP_OBJECT_LIST, "object-list"}
    ,
    {PROP_OBJECT_NAME, "object-name"}
    ,
    {PROP_OBJECT_PROPERTY_REFERENCE, "object-property-reference"}
    ,
    {PROP_OBJECT_TYPE, "object-type"}
    ,
    {PROP_OPTIONAL, "optional"}
    ,
    {PROP_OUT_OF_SERVICE, "out-of-service"}
    ,
    {PROP_OUTPUT_UNITS, "output-units"}
    ,
    {PROP_EVENT_PARAMETERS, "event-parameters"}
    ,
    {PROP_POLARITY, "polarity"}
    ,
    {PROP_PRESENT_VALUE, "present-value"}
    ,
    {PROP_PRIORITY, "priority"}
    ,
    {PROP_PRIORITY_ARRAY, "priority-array"}
    ,
    {PROP_PRIORITY_FOR_WRITING, "priority-for-writing"}
    ,
    {PROP_PROCESS_IDENTIFIER, "process-identifier"}
    ,
    {PROP_PROGRAM_CHANGE, "program-change"}
    ,
    {PROP_PROGRAM_LOCATION, "program-location"}
    ,
    {PROP_PROGRAM_STATE, "program-state"}
    ,
    {PROP_PROPORTIONAL_CONSTANT, "proportional-constant"}
    ,
    {PROP_PROPORTIONAL_CONSTANT_UNITS, "proportional-constant-units"}
    ,
    {PROP_PROTOCOL_CONFORMANCE_CLASS, "protocol-conformance-class"}
    ,
    {PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
        "protocol-object-types-supported"}
    ,
    {PROP_PROTOCOL_SERVICES_SUPPORTED, "protocol-services-supported"}
    ,
    {PROP_PROTOCOL_VERSION, "protocol-version"}
    ,
    {PROP_READ_ONLY, "read-only"}
    ,
    {PROP_REASON_FOR_HALT, "reason-for-halt"}
    ,
    {PROP_RECIPIENT, "recipient"}
    ,
    {PROP_RECIPIENT_LIST, "recipient-list"}
    ,
    {PROP_RELIABILITY, "reliability"}
    ,
    {PROP_RELINQUISH_DEFAULT, "relinquish-default"}
    ,
    {PROP_REQUIRED, "required"}
    ,
    {PROP_RESOLUTION, "resolution"}
    ,
    {PROP_SEGMENTATION_SUPPORTED, "segmentation-supported"}
    ,
    {PROP_SETPOINT, "setpoint"}
    ,
    {PROP_SETPOINT_REFERENCE, "setpoint-reference"}
    ,
    {PROP_STATE_TEXT, "state-text"}
    ,
    {PROP_STATUS_FLAGS, "status-flags"}
    ,
    {PROP_SYSTEM_STATUS, "system-status"}
    ,
    {PROP_TIME_DELAY, "time-delay"}
    ,
    {PROP_TIME_OF_ACTIVE_TIME_RESET, "time-of-active-time-reset"}
    ,
    {PROP_TIME_OF_STATE_COUNT_RESET, "time-of-state-count-reset"}
    ,
    {PROP_TIME_SYNCHRONIZATION_RECIPIENTS,
        "time-synchronization-recipients"}
    ,
    {PROP_UNITS, "units"}
    ,
    {PROP_UPDATE_INTERVAL, "update-interval"}
    ,
    {PROP_UTC_OFFSET, "utc-offset"}
    ,
    {PROP_VENDOR_IDENTIFIER, "vendor-identifier"}
    ,
    {PROP_VENDOR_NAME, "vendor-name"}
    ,
    {PROP_VT_CLASSES_SUPPORTED, "vt-classes-supported"}
    ,
    {PROP_WEEKLY_SCHEDULE, "weekly-schedule"}
    ,
    {PROP_ATTEMPTED_SAMPLES, "attempted-samples"}
    ,
    {PROP_AVERAGE_VALUE, "average-value"}
    ,
    {PROP_BUFFER_SIZE, "buffer-size"}
    ,
    {PROP_CLIENT_COV_INCREMENT, "client-cov-increment"}
    ,
    {PROP_COV_RESUBSCRIPTION_INTERVAL, "cov-resubscription-interval"}
    ,
    {PROP_CURRENT_NOTIFY_TIME, "current-notify-time"}
    ,
    {PROP_EVENT_TIME_STAMPS, "event-time-stamps"}
    ,
    {PROP_LOG_BUFFER, "log-buffer"}
    ,
    {PROP_LOG_DEVICE_OBJECT, "log-device-object-property"}
    ,
    {PROP_ENABLE, "enable"}
    ,
    {PROP_LOG_INTERVAL, "log-interval"}
    ,
    {PROP_MAXIMUM_VALUE, "maximum-value"}
    ,
    {PROP_MINIMUM_VALUE, "minimum-value"}
    ,
    {PROP_NOTIFICATION_THRESHOLD, "notification-threshold"}
    ,
    {PROP_PREVIOUS_NOTIFY_TIME, "previous-notify-time"}
    ,
    {PROP_PROTOCOL_REVISION, "protocol-revision"}
    ,
    {PROP_RECORDS_SINCE_NOTIFICATION, "records-since-notification"}
    ,
    {PROP_RECORD_COUNT, "record-count"}
    ,
    {PROP_START_TIME, "start-time"}
    ,
    {PROP_STOP_TIME, "stop-time"}
    ,
    {PROP_STOP_WHEN_FULL, "stop-when-full"}
    ,
    {PROP_TOTAL_RECORD_COUNT, "total-record-count"}
    ,
    {PROP_VALID_SAMPLES, "valid-samples"}
    ,
    {PROP_WINDOW_INTERVAL, "window-interval"}
    ,
    {PROP_WINDOW_SAMPLES, "window-samples"}
    ,
    {PROP_MAXIMUM_VALUE_TIMESTAMP, "maximum-value-timestamp"}
    ,
    {PROP_MINIMUM_VALUE_TIMESTAMP, "minimum-value-timestamp"}
    ,
    {PROP_VARIANCE_VALUE, "variance-value"}
    ,
    {PROP_ACTIVE_COV_SUBSCRIPTIONS, "active-cov-subscriptions"}
    ,
    {PROP_BACKUP_FAILURE_TIMEOUT, "backup-failure-timeout"}
    ,
    {PROP_CONFIGURATION_FILES, "configuration-files"}
    ,
    {PROP_DATABASE_REVISION, "database-revision"}
    ,
    {PROP_DIRECT_READING, "direct-reading"}
    ,
    {PROP_LAST_RESTORE_TIME, "last-restore-time"}
    ,
    {PROP_MAINTENANCE_REQUIRED, "maintenance-required"}
    ,
    {PROP_MEMBER_OF, "member-of"}
    ,
    {PROP_MODE, "mode"}
    ,
    {PROP_OPERATION_EXPECTED, "operation-expected"}
    ,
    {PROP_SETTING, "setting"}
    ,
    {PROP_SILENCED, "silenced"}
    ,
    {PROP_TRACKING_VALUE, "tracking-value"}
    ,
    {PROP_ZONE_MEMBERS, "zone-members"}
    ,
    {PROP_LIFE_SAFETY_ALARM_VALUES, "life-safety-alarm-values"}
    ,
    {PROP_MAX_SEGMENTS_ACCEPTED, "max-segments-accepted"}
    ,
    {PROP_PROFILE_NAME, "profile-name"}
    ,
    {PROP_AUTO_SLAVE_DISCOVERY, "auto-slave-discovery"}
    ,
    {PROP_MANUAL_SLAVE_ADDRESS_BINDING, "manual-slave-address-binding"}
    ,
    {PROP_SLAVE_ADDRESS_BINDING, "slave-address-binding"}
    ,
    {PROP_SLAVE_PROXY_ENABLE, "slave-proxy-enable"}
    ,
    {PROP_LAST_NOTIFY_TIME, "last-notify-time"}
    ,
    {PROP_SCHEDULE_DEFAULT, "schedule-default"}
    ,
    {PROP_ACCEPTED_MODES, "accepted-modes"}
    ,
    {PROP_ADJUST_VALUE, "adjust-value"}
    ,
    {PROP_COUNT, "count"}
    ,
    {PROP_COUNT_BEFORE_CHANGE, "count-before-change"}
    ,
    {PROP_COUNT_CHANGE_TIME, "count-change-time"}
    ,
    {PROP_COV_PERIOD, "COV-period"}
    ,
    {PROP_INPUT_REFERENCE, "input-reference"}
    ,
    {PROP_LIMIT_MONITORING_INTERVAL, "limit-monitoring-interval"}
    ,
    {PROP_LOGGING_DEVICE, "logging-device"}
    ,
    {PROP_LOGGING_RECORD, "logging-record"}
    ,
    {PROP_PRESCALE, "prescale"}
    ,
    {PROP_PULSE_RATE, "pulse-rate"}
    ,
    {PROP_SCALE, "scale"}
    ,
    {PROP_SCALE_FACTOR, "scale-factor"}
    ,
    {PROP_UPDATE_TIME, "update-time"}
    ,
    {PROP_VALUE_BEFORE_CHANGE, "value-before-change"}
    ,
    {PROP_VALUE_SET, "value-set"}
    ,
    {PROP_VALUE_CHANGE_TIME, "value-change-time"}
    ,
    {PROP_ALIGN_INTERVALS, "align-intervals"}
    ,
    {PROP_GROUP_MEMBER_NAMES, "group-member-names"}
    ,
    {PROP_INTERVAL_OFFSET, "interval-offset"}
    ,
    {PROP_LAST_RESTART_REASON, "last-restart-reason"}
    ,
    {PROP_LOGGING_TYPE, "logging-type"}
    ,
    {PROP_MEMBER_STATUS_FLAGS, "member-status-flags"}
    ,
    {PROP_NOTIFICATION_PERIOD, "notification-period"}
    ,
    {PROP_PREVIOUS_NOTIFY_RECORD, "previous-notify-record"}
    ,
    {PROP_REQUESTED_UPDATE_INTERVAL, "requested-update-interval"}
    ,
    {PROP_RESTART_NOTIFICATION_RECIPIENTS,
        "restart-notification-recipients"}
    ,
    {PROP_TIME_OF_DEVICE_RESTART, "time-of-device-restart"}
    ,
    {PROP_TIME_SYNCHRONIZATION_INTERVAL, "time-synchronization-interval"}
    ,
    {PROP_TRIGGER, "trigger"}
    ,
    {PROP_UTC_TIME_SYNCHRONIZATION_RECIPIENTS,
        "UTC-time-synchronization-recipients"}
    ,
    {PROP_NODE_SUBTYPE, "node-subtype"}
    ,
    {PROP_NODE_TYPE, "node-type"}
    ,
    {PROP_STRUCTURED_OBJECT_LIST, "structured-object-list"}
    ,
    {PROP_SUBORDINATE_ANNOTATIONS, "subordinate-annotations"}
    ,
    {PROP_SUBORDINATE_LIST, "subordinate-list"}
    ,
    {PROP_ACTUAL_SHED_LEVEL, "actual-shed-level"}
    ,
    {PROP_DUTY_WINDOW, "duty-window"}
    ,
    {PROP_EXPECTED_SHED_LEVEL, "expected-shed-level"}
    ,
    {PROP_FULL_DUTY_BASELINE, "full-duty-baseline"}
    ,
    {PROP_REQUESTED_SHED_LEVEL, "requested-shed-level"}
    ,
    {PROP_SHED_DURATION, "shed-duration"}
    ,
    {PROP_SHED_LEVEL_DESCRIPTIONS, "shed-level-descriptions"}
    ,
    {PROP_SHED_LEVELS, "shed-levels"}
    ,
    {PROP_STATE_DESCRIPTION, "state-descriptions"}
    ,

    {0, NULL}
    /* Enumerated values 0-511 are reserved for definition by ASHRAE.
       Enumerated values 512-4194303 may be used by others subject to the
       procedures and constraints described in Clause 23. */
};

const char *bactext_property_name(int index)
{
    return indtext_by_index_split_default(bacnet_property_names,
        index, 512, ASHRAE_Reserved_String, Vendor_Proprietary_String);
}

unsigned bactext_property_id(const char *name)
{
    return indtext_by_istring_default(bacnet_property_names, name, 0);
}

INDTEXT_DATA bacnet_engineering_unit_names[] = {
    {UNITS_SQUARE_METERS, "square-meters"}
    ,
    {UNITS_SQUARE_FEET, "square-feet"}
    ,
    {UNITS_MILLIAMPERES, "milliamperes"}
    ,
    {UNITS_AMPERES, "amperes"}
    ,
    {UNITS_OHMS, "ohms"}
    ,
    {UNITS_VOLTS, "volts"}
    ,
    {UNITS_KILOVOLTS, "kilovolts"}
    ,
    {UNITS_MEGAVOLTS, "megavolts"}
    ,
    {UNITS_VOLT_AMPERES, "volt-amperes"}
    ,
    {UNITS_KILOVOLT_AMPERES, "kilovolt-amperes"}
    ,
    {UNITS_MEGAVOLT_AMPERES, "megavolt-amperes"}
    ,
    {UNITS_VOLT_AMPERES_REACTIVE, "volt-amperes-reactive"}
    ,
    {UNITS_KILOVOLT_AMPERES_REACTIVE, "kilovolt-amperes-reactive"}
    ,
    {UNITS_MEGAVOLT_AMPERES_REACTIVE, "megavolt-amperes-reactive"}
    ,
    {UNITS_DEGREES_PHASE, "degrees-phase"}
    ,
    {UNITS_POWER_FACTOR, "power-factor"}
    ,
    {UNITS_JOULES, "joules"}
    ,
    {UNITS_KILOJOULES, "kilojoules"}
    ,
    {UNITS_WATT_HOURS, "watt-hours"}
    ,
    {UNITS_KILOWATT_HOURS, "kilowatt-hours"}
    ,
    {UNITS_BTUS, "btus"}
    ,
    {UNITS_THERMS, "therms"}
    ,
    {UNITS_TON_HOURS, "ton-hours"}
    ,
    {UNITS_JOULES_PER_KILOGRAM_DRY_AIR, "joules-per-kilogram-dry-air"}
    ,
    {UNITS_BTUS_PER_POUND_DRY_AIR, "btus-per-pound-dry-air"}
    ,
    {UNITS_CYCLES_PER_HOUR, "cycles-per-hour"}
    ,
    {UNITS_CYCLES_PER_MINUTE, "cycles-per-minute"}
    ,
    {UNITS_HERTZ, "hertz"}
    ,
    {UNITS_GRAMS_OF_WATER_PER_KILOGRAM_DRY_AIR,
        "grams-of-water-per-kilogram-dry-air"}
    ,
    {UNITS_PERCENT_RELATIVE_HUMIDITY, "percent-relative-humidity"}
    ,
    {UNITS_MILLIMETERS, "millimeters"}
    ,
    {UNITS_METERS, "meters"}
    ,
    {UNITS_INCHES, "inches"}
    ,
    {UNITS_FEET, "feet"}
    ,
    {UNITS_WATTS_PER_SQUARE_FOOT, "watts-per-square-foot"}
    ,
    {UNITS_WATTS_PER_SQUARE_METER, "watts-per-square-meter"}
    ,
    {UNITS_LUMENS, "lumens"}
    ,
    {UNITS_LUXES, "luxes"}
    ,
    {UNITS_FOOT_CANDLES, "foot-candles"}
    ,
    {UNITS_KILOGRAMS, "kilograms"}
    ,
    {UNITS_POUNDS_MASS, "pounds-mass"}
    ,
    {UNITS_TONS, "tons"}
    ,
    {UNITS_KILOGRAMS_PER_SECOND, "kilograms-per-second"}
    ,
    {UNITS_KILOGRAMS_PER_MINUTE, "kilograms-per-minute"}
    ,
    {UNITS_KILOGRAMS_PER_HOUR, "kilograms-per-hour"}
    ,
    {UNITS_POUNDS_MASS_PER_MINUTE, "pounds-mass-per-minute"}
    ,
    {UNITS_POUNDS_MASS_PER_HOUR, "pounds-mass-per-hour"}
    ,
    {UNITS_WATTS, "watts"}
    ,
    {UNITS_KILOWATTS, "kilowatts"}
    ,
    {UNITS_MEGAWATTS, "megawatts"}
    ,
    {UNITS_BTUS_PER_HOUR, "btus-per-hour"}
    ,
    {UNITS_HORSEPOWER, "horsepower"}
    ,
    {UNITS_TONS_REFRIGERATION, "tons-refrigeration"}
    ,
    {UNITS_PASCALS, "pascals"}
    ,
    {UNITS_KILOPASCALS, "kilopascals"}
    ,
    {UNITS_BARS, "bars"}
    ,
    {UNITS_POUNDS_FORCE_PER_SQUARE_INCH, "pounds-force-per-square-inch"}
    ,
    {UNITS_CENTIMETERS_OF_WATER, "centimeters-of-water"}
    ,
    {UNITS_INCHES_OF_WATER, "inches-of-water"}
    ,
    {UNITS_MILLIMETERS_OF_MERCURY, "millimeters-of-mercury"}
    ,
    {UNITS_CENTIMETERS_OF_MERCURY, "centimeters-of-mercury"}
    ,
    {UNITS_INCHES_OF_MERCURY, "inches-of-mercury"}
    ,
    {UNITS_DEGREES_CELSIUS, "degrees-celsius"}
    ,
    {UNITS_DEGREES_KELVIN, "degrees-kelvin"}
    ,
    {UNITS_DEGREES_FAHRENHEIT, "degrees-fahrenheit"}
    ,
    {UNITS_DEGREE_DAYS_CELSIUS, "degree-days-celsius"}
    ,
    {UNITS_DEGREE_DAYS_FAHRENHEIT, "degree-days-fahrenheit"}
    ,
    {UNITS_YEARS, "years"}
    ,
    {UNITS_MONTHS, "months"}
    ,
    {UNITS_WEEKS, "weeks"}
    ,
    {UNITS_DAYS, "days"}
    ,
    {UNITS_HOURS, "hours"}
    ,
    {UNITS_MINUTES, "minutes"}
    ,
    {UNITS_SECONDS, "secondS"}
    ,
    {UNITS_METERS_PER_SECOND, "meters-per-second"}
    ,
    {UNITS_KILOMETERS_PER_HOUR, "kilometers-per-hour"}
    ,
    {UNITS_FEET_PER_SECOND, "feet-per-second"}
    ,
    {UNITS_FEET_PER_MINUTE, "feet-per-minute"}
    ,
    {UNITS_MILES_PER_HOUR, "miles-per-hour"}
    ,
    {UNITS_CUBIC_FEET, "cubic-feet"}
    ,
    {UNITS_CUBIC_METERS, "cubic-meters"}
    ,
    {UNITS_IMPERIAL_GALLONS, "imperial-gallons"}
    ,
    {UNITS_LITERS, "liters"}
    ,
    {UNITS_US_GALLONS, "us-gallons"}
    ,
    {UNITS_CUBIC_FEET_PER_MINUTE, "cubic-feet-per-minute"}
    ,
    {UNITS_CUBIC_METERS_PER_SECOND, "cubic-meters-per-second"}
    ,
    {UNITS_IMPERIAL_GALLONS_PER_MINUTE, "imperial-gallons-per-minute"}
    ,
    {UNITS_LITERS_PER_SECOND, "liters-per-second"}
    ,
    {UNITS_LITERS_PER_MINUTE, "liters-per-minute"}
    ,
    {UNITS_US_GALLONS_PER_MINUTE, "us-gallons-per-minute"}
    ,
    {UNITS_DEGREES_ANGULAR, "degrees-angular"}
    ,
    {UNITS_DEGREES_CELSIUS_PER_HOUR, "degrees-celsius-per-hour"}
    ,
    {UNITS_DEGREES_CELSIUS_PER_MINUTE, "degrees-celsius-per-minute"}
    ,
    {UNITS_DEGREES_FAHRENHEIT_PER_HOUR, "degrees-fahrenheit-per-hour"}
    ,
    {UNITS_DEGREES_FAHRENHEIT_PER_MINUTE, "degrees-fahrenheit-per-minute"}
    ,
    {UNITS_NO_UNITS, "no-units"}
    ,
    {UNITS_PARTS_PER_MILLION, "parts-per-million"}
    ,
    {UNITS_PARTS_PER_BILLION, "parts-per-billion"}
    ,
    {UNITS_PERCENT, "percent"}
    ,
    {UNITS_PERCENT_PER_SECOND, "percent-per-second"}
    ,
    {UNITS_PER_MINUTE, "per-minute"}
    ,
    {UNITS_PER_SECOND, "per-second"}
    ,
    {UNITS_PSI_PER_DEGREE_FAHRENHEIT, "psi-per-degree-fahrenheit"}
    ,
    {UNITS_RADIANS, "radians"}
    ,
    {UNITS_REVOLUTIONS_PER_MINUTE, "revolutions-per-minute"}
    ,
    {UNITS_CURRENCY1, "currency1"}
    ,
    {UNITS_CURRENCY2, "currency2"}
    ,
    {UNITS_CURRENCY3, "currency3"}
    ,
    {UNITS_CURRENCY4, "currency4"}
    ,
    {UNITS_CURRENCY5, "currency5"}
    ,
    {UNITS_CURRENCY6, "currency6"}
    ,
    {UNITS_CURRENCY7, "currency7"}
    ,
    {UNITS_CURRENCY8, "currency8"}
    ,
    {UNITS_CURRENCY9, "currency9"}
    ,
    {UNITS_CURRENCY10, "currency10"}
    ,
    {UNITS_SQUARE_INCHES, "square-inches"}
    ,
    {UNITS_SQUARE_CENTIMETERS, "square-centimeters"}
    ,
    {UNITS_BTUS_PER_POUND, "btus_per-pound"}
    ,
    {UNITS_CENTIMETERS, "centimeters"}
    ,
    {UNITS_POUNDS_MASS_PER_SECOND, "pounds-mass-per-second"}
    ,
    {UNITS_DELTA_DEGREES_FAHRENHEIT, "delta-degrees-fahrenheit"}
    ,
    {UNITS_DELTA_DEGREES_KELVIN, "delta-degrees-kelvin"}
    ,
    {UNITS_KILOHMS, "kilohms"}
    ,
    {UNITS_MEGOHMS, "megohms"}
    ,
    {UNITS_MILLIVOLTS, "millivolts"}
    ,
    {UNITS_KILOJOULES_PER_KILOGRAM, "kilojoules-per-kilogram"}
    ,
    {UNITS_MEGAJOULES, "megajoules"}
    ,
    {UNITS_JOULES_PER_DEGREE_KELVIN, "joules-per-degree-kelvin"}
    ,
    {UNITS_JOULES_PER_KILOGRAM_DEGREE_KELVIN,
        "joules-per-kilogram-degree-kelvin"}
    ,
    {UNITS_KILOHERTZ, "kilohertz"}
    ,
    {UNITS_MEGAHERTZ, "megahertz"}
    ,
    {UNITS_PER_HOUR, "per-hour"}
    ,
    {UNITS_MILLIWATTS, "milliwatts"}
    ,
    {UNITS_HECTOPASCALS, "hectopascals"}
    ,
    {UNITS_MILLIBARS, "millibars"}
    ,
    {UNITS_CUBIC_METERS_PER_HOUR, "cubic-meters-per-hour"}
    ,
    {UNITS_LITERS_PER_HOUR, "liters-per-hour"}
    ,
    {UNITS_KILOWATT_HOURS_PER_SQUARE_METER,
        "kilowatt-hours-per-square-meter"}
    ,
    {UNITS_KILOWATT_HOURS_PER_SQUARE_FOOT, "kilowatt-hours-per-square-foot"}
    ,
    {UNITS_MEGAJOULES_PER_SQUARE_METER, "megajoules-per-square-meter"}
    ,
    {UNITS_MEGAJOULES_PER_SQUARE_FOOT, "megajoules-per-square-foot"}
    ,
    {UNITS_CUBIC_FEET_PER_SECOND, "cubic-feet-per-second"}
    ,
    {UNITS_WATTS_PER_SQUARE_METER_DEGREE_KELVIN,
        "watts-per-square-meter-degree-kelvin"}
    ,
    {UNITS_PERCENT_OBSCURATION_PER_FOOT, "percent-obscuration-per-foot"}
    ,
    {UNITS_PERCENT_OBSCURATION_PER_METER, "percent-obscuration-per-meter"}
    ,
    {UNITS_MILLIOHMS, "milliohms"}
    ,
    {UNITS_MEGAWATT_HOURS, "megawatt-hours"}
    ,
    {UNITS_KILO_BTUS, "kilo-btus"}
    ,
    {UNITS_MEGA_BTUS, "mega-btus"}
    ,
    {UNITS_KILOJOULES_PER_KILOGRAM_DRY_AIR,
        "kilojoules-per-kilogram-dry-air"}
    ,
    {UNITS_MEGAJOULES_PER_KILOGRAM_DRY_AIR,
        "megajoules-per-kilogram-dry-air"}
    ,
    {UNITS_KILOJOULES_PER_DEGREE_KELVIN, "kilojoules-per-degree-Kelvin"}
    ,
    {UNITS_MEGAJOULES_PER_DEGREE_KELVIN, "megajoules-per-degree-Kelvin"}
    ,
    {UNITS_NEWTON, "newton"}
    ,
    {UNITS_GRAMS_PER_SECOND, "grams-per-second"}
    ,
    {UNITS_GRAMS_PER_MINUTE, "grams-per-minute"}
    ,
    {UNITS_TONS_PER_HOUR, "tons-per-hour"}
    ,
    {UNITS_KILO_BTUS_PER_HOUR, "kilo-btus-per-hour"}
    ,
    {UNITS_HUNDREDTHS_SECONDS, "hundredths-seconds"}
    ,
    {UNITS_MILLISECONDS, "milliseconds"}
    ,
    {UNITS_NEWTON_METERS, "newton-meters"}
    ,
    {UNITS_MILLIMETERS_PER_SECOND, "millimeters-per-second"}
    ,
    {UNITS_MILLIMETERS_PER_MINUTE, "millimeters-per-minute"}
    ,
    {UNITS_METERS_PER_MINUTE, "meters-per-minute"}
    ,
    {UNITS_METERS_PER_HOUR, "meters-per-hour"}
    ,
    {UNITS_CUBIC_METERS_PER_MINUTE, "cubic-meters-per-minute"}
    ,
    {UNITS_METERS_PER_SECOND_PER_SECOND, "meters-per-second-per-second"}
    ,
    {UNITS_AMPERES_PER_METER, "amperes-per-meter"}
    ,
    {UNITS_AMPERES_PER_SQUARE_METER, "amperes-per-square-meter"}
    ,
    {UNITS_AMPERE_SQUARE_METERS, "ampere-square-meters"}
    ,
    {UNITS_FARADS, "farads"}
    ,
    {UNITS_HENRYS, "henrys"}
    ,
    {UNITS_OHM_METERS, "ohm-meters"}
    ,
    {UNITS_SIEMENS, "siemens"}
    ,
    {UNITS_SIEMENS_PER_METER, "siemens-per-meter"}
    ,
    {UNITS_TESLAS, "teslas"}
    ,
    {UNITS_VOLTS_PER_DEGREE_KELVIN, "volts-per-degree-Kelvin"}
    ,
    {UNITS_VOLTS_PER_METER, "volts-per-meter"}
    ,
    {UNITS_WEBERS, "webers"}
    ,
    {UNITS_CANDELAS, "candelas"}
    ,
    {UNITS_CANDELAS_PER_SQUARE_METER, "candelas-per-square-meter"}
    ,
    {UNITS_DEGREES_KELVIN_PER_HOUR, "degrees-Kelvin-per-hour"}
    ,
    {UNITS_DEGREES_KELVIN_PER_MINUTE, "degrees-Kelvin-per-minute"}
    ,
    {UNITS_JOULE_SECONDS, "joule-seconds"}
    ,
    {UNITS_RADIANS_PER_SECOND, "radians-per-second"}
    ,
    {UNITS_SQUARE_METERS_PER_NEWTON, "square-meters-per-Newton"}
    ,
    {UNITS_KILOGRAMS_PER_CUBIC_METER, "kilograms-per-cubic-meter"}
    ,
    {UNITS_NEWTON_SECONDS, "newton-seconds"}
    ,
    {UNITS_NEWTONS_PER_METER, "newtons-per-meter"}
    ,
    {UNITS_WATTS_PER_METER_PER_DEGREE_KELVIN,
        "watts-per-meter-per-degree-Kelvin"}
    ,
    {0, NULL}
/* Enumerated values 0-255 are reserved for definition by ASHRAE.
   Enumerated values 256-65535 may be used by others subject to
   the procedures and constraints described in Clause 23. */
};

const char *bactext_engineering_unit_name(int index)
{
    return indtext_by_index_split_default(bacnet_engineering_unit_names,
        index, 256, ASHRAE_Reserved_String, Vendor_Proprietary_String);
}

INDTEXT_DATA bacnet_reject_reason_names[] = {
    {REJECT_REASON_OTHER, "Other"}
    ,
    {REJECT_REASON_BUFFER_OVERFLOW, "Buffer Overflow"}
    ,
    {REJECT_REASON_INCONSISTENT_PARAMETERS, "Inconsistent Parameters"}
    ,
    {REJECT_REASON_INVALID_PARAMETER_DATA_TYPE,
        "Invalid Parameter Data Type"}
    ,
    {REJECT_REASON_INVALID_TAG, "Invalid Tag"}
    ,
    {REJECT_REASON_MISSING_REQUIRED_PARAMETER, "Missing Required Parameter"}
    ,
    {REJECT_REASON_PARAMETER_OUT_OF_RANGE, "Parameter Out of Range"}
    ,
    {REJECT_REASON_TOO_MANY_ARGUMENTS, "Too Many Arguments"}
    ,
    {REJECT_REASON_UNDEFINED_ENUMERATION, "Undefined Enumeration"}
    ,
    {REJECT_REASON_UNRECOGNIZED_SERVICE, "Unrecognized Service"}
    ,
    {0, NULL}
};

const char *bactext_reject_reason_name(int index)
{
    return indtext_by_index_split_default(bacnet_reject_reason_names,
        index,
        FIRST_PROPRIETARY_REJECT_REASON,
        ASHRAE_Reserved_String, Vendor_Proprietary_String);
}

INDTEXT_DATA bacnet_abort_reason_names[] = {
    {ABORT_REASON_OTHER, "Other"}
    ,
    {ABORT_REASON_BUFFER_OVERFLOW, "Buffer Overflow"}
    ,
    {ABORT_REASON_INVALID_APDU_IN_THIS_STATE, "Invalid APDU in this State"}
    ,
    {ABORT_REASON_PREEMPTED_BY_HIGHER_PRIORITY_TASK,
        "Preempted by Higher Priority Task"}
    ,
    {ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, "Segmentation Not Supported"}
    ,
    {0, NULL}
};

const char *bactext_abort_reason_name(int index)
{
    return indtext_by_index_split_default(bacnet_abort_reason_names,
        index,
        FIRST_PROPRIETARY_ABORT_REASON,
        ASHRAE_Reserved_String, Vendor_Proprietary_String);
}

INDTEXT_DATA bacnet_error_class_names[] = {
    {ERROR_CLASS_DEVICE, "device"}
    ,
    {ERROR_CLASS_OBJECT, "object"}
    ,
    {ERROR_CLASS_PROPERTY, "property"}
    ,
    {ERROR_CLASS_RESOURCES, "resources"}
    ,
    {ERROR_CLASS_SECURITY, "security"}
    ,
    {ERROR_CLASS_SERVICES, "services"}
    ,
    {ERROR_CLASS_VT, "vt"}
    ,
    {0, NULL}
};

const char *bactext_error_class_name(int index)
{
    return indtext_by_index_split_default(bacnet_error_class_names,
        index,
        FIRST_PROPRIETARY_ERROR_CLASS,
        ASHRAE_Reserved_String, Vendor_Proprietary_String);
}

INDTEXT_DATA bacnet_error_code_names[] = {
    {ERROR_CODE_OTHER, "other"}
    ,
    {ERROR_CODE_AUTHENTICATION_FAILED, "authentication-failed"}
    ,
    {ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED, "character-set-not-supported"}
    ,
    {ERROR_CODE_CONFIGURATION_IN_PROGRESS, "configuration-in-progress"}
    ,
    {ERROR_CODE_DATATYPE_NOT_SUPPORTED, "datatype-not-supported"}
    ,
    {ERROR_CODE_DEVICE_BUSY, "device-busy"}
    ,
    {ERROR_CODE_DUPLICATE_NAME, "duplicate-name"}
    ,
    {ERROR_CODE_DUPLICATE_OBJECT_ID, "duplicate-object-id"}
    ,
    {ERROR_CODE_DYNAMIC_CREATION_NOT_SUPPORTED,
        "dynamic-creation-not-supported"}
    ,
    {ERROR_CODE_FILE_ACCESS_DENIED, "file-access-denied"}
    ,
    {ERROR_CODE_INCOMPATIBLE_SECURITY_LEVELS,
        "incompatible-security-levels"}
    ,
    {ERROR_CODE_INCONSISTENT_PARAMETERS, "inconsistent-parameters"}
    ,
    {ERROR_CODE_INCONSISTENT_SELECTION_CRITERION,
        "inconsistent-selection-criterion"}
    ,
    {ERROR_CODE_INVALID_ARRAY_INDEX, "invalid-array-index"}
    ,
    {ERROR_CODE_INVALID_CONFIGURATION_DATA, "invalid-configuration-data"}
    ,
    {ERROR_CODE_INVALID_DATA_TYPE, "invalid-data-type"}
    ,
    {ERROR_CODE_INVALID_FILE_ACCESS_METHOD, "invalid-file-access-method"}
    ,
    {ERROR_CODE_ERROR_CODE_INVALID_FILE_START_POSITION,
        "error-code-invalid-file-start-position"}
    ,
    {ERROR_CODE_INVALID_OPERATOR_NAME, "invalid-operator-name"}
    ,
    {ERROR_CODE_INVALID_PARAMETER_DATA_TYPE, "invalid-parameter-data-type"}
    ,
    {ERROR_CODE_INVALID_TIME_STAMP, "invalid-time-stamp"}
    ,
    {ERROR_CODE_KEY_GENERATION_ERROR, "key-generation-error"}
    ,
    {ERROR_CODE_MISSING_REQUIRED_PARAMETER, "missing-required-parameter"}
    ,
    {ERROR_CODE_NO_OBJECTS_OF_SPECIFIED_TYPE,
        "no-objects-of-specified-type"}
    ,
    {ERROR_CODE_NO_SPACE_FOR_OBJECT, "no-space-for-object"}
    ,
    {ERROR_CODE_NO_SPACE_TO_ADD_LIST_ELEMENT,
        "no-space-to-add-list-element"}
    ,
    {ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY, "no-space-to-write-property"}
    ,
    {ERROR_CODE_NO_VT_SESSIONS_AVAILABLE, "no-vt-sessions-available"}
    ,
    {ERROR_CODE_OBJECT_DELETION_NOT_PERMITTED,
        "object-deletion-not-permitted"}
    ,
    {ERROR_CODE_OBJECT_IDENTIFIER_ALREADY_EXISTS,
        "object-identifier-already-exists"}
    ,
    {ERROR_CODE_OPERATIONAL_PROBLEM, "operational-problem"}
    ,
    {ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED,
        "optional-functionality-not-supported"}
    ,
    {ERROR_CODE_PASSWORD_FAILURE, "password-failure"}
    ,
    {ERROR_CODE_PROPERTY_IS_NOT_A_LIST, "property-is-not-a-list"}
    ,
    {ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY, "property-is-not-an-array"}
    ,
    {ERROR_CODE_READ_ACCESS_DENIED, "read-access-denied"}
    ,
    {ERROR_CODE_SECURITY_NOT_SUPPORTED, "security-not-supported"}
    ,
    {ERROR_CODE_SERVICE_REQUEST_DENIED, "service-request-denied"}
    ,
    {ERROR_CODE_TIMEOUT, "timeout"}
    ,
    {ERROR_CODE_UNKNOWN_OBJECT, "unknown-object"}
    ,
    {ERROR_CODE_UNKNOWN_PROPERTY, "unknown-property"}
    ,
    {ERROR_CODE_RESERVED1, "reserved1"}
    ,
    {ERROR_CODE_UNKNOWN_VT_CLASS, "unknown-vt-class"}
    ,
    {ERROR_CODE_UNKNOWN_VT_SESSION, "unknown-vt-session"}
    ,
    {ERROR_CODE_UNSUPPORTED_OBJECT_TYPE, "unsupported-object-type"}
    ,
    {ERROR_CODE_VALUE_OUT_OF_RANGE, "value-out-of-range"}
    ,
    {ERROR_CODE_VT_SESSION_ALREADY_CLOSED, "vt-session-already-closed"}
    ,
    {ERROR_CODE_VT_SESSION_TERMINATION_FAILURE,
        "vt-session-termination-failure"}
    ,
    {ERROR_CODE_WRITE_ACCESS_DENIED, "write-access-denied"}
    ,
    {ERROR_CODE_COV_SUBSCRIPTION_FAILED, "cov-subscription-failed"}
    ,
    {ERROR_CODE_NOT_COV_PROPERTY, "not-cov-property"}
    ,
    {0, NULL}
};

const char *bactext_error_code_name(int index)
{
    return indtext_by_index_split_default(bacnet_error_code_names,
        index,
        FIRST_PROPRIETARY_ERROR_CLASS,
        ASHRAE_Reserved_String, Vendor_Proprietary_String);
}

INDTEXT_DATA bacnet_month_names[] = {
    {1, "January"}
    ,
    {2, "February"}
    ,
    {3, "March"}
    ,
    {4, "April"}
    ,
    {5, "May"}
    ,
    {6, "June"}
    ,
    {7, "July"}
    ,
    {8, "August"}
    ,
    {9, "September"}
    ,
    {10, "October"}
    ,
    {11, "November"}
    ,
    {12, "December"}
    ,
    {255, "Any Month"}
    ,
    {0, NULL}
};

const char *bactext_month_name(int index)
{
    return indtext_by_index_default(bacnet_month_names,
        index, ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_week_of_month_names[] = {
    {1, "days numbered 1-7"}
    ,
    {2, "days numbered 8-14"}
    ,
    {3, "days numbered 15-21"}
    ,
    {4, "days numbered 22-28"}
    ,
    {5, "days numbered 29-31"}
    ,
    {6, "last 7 days of this month"}
    ,
    {255, "any week of this month"}
    ,
    {0, NULL}
};

const char *bactext_week_of_month_name(int index)
{
    return indtext_by_index_default(bacnet_week_of_month_names,
        index, ASHRAE_Reserved_String);
}

/* note: different than DaysOfWeek bit string where 0=monday */
INDTEXT_DATA bacnet_day_of_week_names[] = {
    {1, "Monday"}
    ,
    {2, "Tuesday"}
    ,
    {3, "Wednesday"}
    ,
    {4, "Thursday"}
    ,
    {5, "Friday"}
    ,
    {6, "Saturday"}
    ,
    {7, "Sunday"}
    ,
    {255, "any day of week"}
    ,
    {0, NULL}
};

const char *bactext_day_of_week_name(int index)
{
    return indtext_by_index_default(bacnet_day_of_week_names,
        index, ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_event_state_names[] = {
    {EVENT_STATE_NORMAL, "normal"}
    ,
    {EVENT_STATE_FAULT, "fault"}
    ,
    {EVENT_STATE_OFFNORMAL, "offnormal"}
    ,
    {EVENT_STATE_HIGH_LIMIT, "high limit"}
    ,
    {EVENT_STATE_LOW_LIMIT, "low limit"}
    ,
    {0, NULL}
};

const char *bactext_event_state_name(int index)
{
    return indtext_by_index_default(bacnet_event_state_names,
        index, ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_binary_present_value_names[] = {
    {BINARY_INACTIVE, "inactive"}
    ,
    {BINARY_ACTIVE, "active"}
    ,
    {0, NULL}
};

const char *bactext_binary_present_value_name(int index)
{
    return indtext_by_index_default(bacnet_binary_present_value_names,
        index, ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_reliability_names[] = {
    {RELIABILITY_NO_FAULT_DETECTED, "no-fault-detected"}
    ,
    {RELIABILITY_NO_SENSOR, "no-sensor"}
    ,
    {RELIABILITY_OVER_RANGE, "over-range"}
    ,
    {RELIABILITY_UNDER_RANGE, "under-range"}
    ,
    {RELIABILITY_OPEN_LOOP, "open-loop"}
    ,
    {RELIABILITY_SHORTED_LOOP, "shorted-loop"}
    ,
    {RELIABILITY_NO_OUTPUT, "no-output"}
    ,
    {RELIABILITY_UNRELIABLE_OTHER, "unreliable-other"}
    ,
    {RELIABILITY_PROCESS_ERROR, "process-error"}
    ,
    {RELIABILITY_MULTI_STATE_FAULT, "mult-state-fault"}
    ,
    {RELIABILITY_CONFIGURATION_ERROR, "configuration-error"}
    ,
    {0, NULL}
};

const char *bactext_reliability_name(int index)
{
    return indtext_by_index_default(bacnet_reliability_names,
        index, ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_device_status_names[] = {
    {STATUS_OPERATIONAL, "operational"}
    ,
    {STATUS_OPERATIONAL_READ_ONLY, "operational-read-only"}
    ,
    {STATUS_DOWNLOAD_REQUIRED, "download-required"}
    ,
    {STATUS_DOWNLOAD_IN_PROGRESS, "download-in-progress"}
    ,
    {STATUS_NON_OPERATIONAL, "non-operational"}
    ,
    {0, NULL}
};

const char *bactext_device_status_name(int index)
{
    return indtext_by_index_default(bacnet_device_status_names,
        index, ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_segmentation_names[] = {
    {SEGMENTATION_BOTH, "segmented-both"}
    ,
    {SEGMENTATION_TRANSMIT, "segmented-transmit"}
    ,
    {SEGMENTATION_RECEIVE, "segmented-receive"}
    ,
    {SEGMENTATION_NONE, "no-segmentation"}
    ,
    {0, NULL}
};

const char *bactext_segmentation_name(int index)
{
    return indtext_by_index_default(bacnet_segmentation_names,
        index, ASHRAE_Reserved_String);
}
