/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2012 Steve Karg

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
#include <stdint.h>
#include "bacenum.h"
#include "bacdef.h"
#include "rpm.h"
#include "proplist.h"

/** @file proplist.c  List of Required and Optional object properties */

static const int Default_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    -1
};

static const int Device_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_SYSTEM_STATUS,
    PROP_VENDOR_NAME,
    PROP_VENDOR_IDENTIFIER,
    PROP_MODEL_NAME,
    PROP_FIRMWARE_REVISION,
    PROP_APPLICATION_SOFTWARE_VERSION,
    PROP_PROTOCOL_VERSION,
    PROP_PROTOCOL_REVISION,
    PROP_PROTOCOL_SERVICES_SUPPORTED,
    PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
    PROP_OBJECT_LIST,
    PROP_MAX_APDU_LENGTH_ACCEPTED,
    PROP_SEGMENTATION_SUPPORTED,
    PROP_APDU_TIMEOUT,
    PROP_NUMBER_OF_APDU_RETRIES,
    PROP_DEVICE_ADDRESS_BINDING,
    PROP_DATABASE_REVISION,
    -1
};

static const int Device_Properties_Optional[] = {
    PROP_LOCATION,
    PROP_DESCRIPTION,
    PROP_STRUCTURED_OBJECT_LIST,
    PROP_MAX_SEGMENTS_ACCEPTED,
    PROP_VT_CLASSES_SUPPORTED,
    PROP_ACTIVE_VT_SESSIONS,
    PROP_LOCAL_TIME,
    PROP_LOCAL_DATE,
    PROP_UTC_OFFSET,
    PROP_DAYLIGHT_SAVINGS_STATUS,
    PROP_APDU_SEGMENT_TIMEOUT,
    PROP_TIME_SYNCHRONIZATION_RECIPIENTS,
    PROP_MAX_MASTER,
    PROP_MAX_INFO_FRAMES,
    PROP_CONFIGURATION_FILES,
    PROP_LAST_RESTORE_TIME,
    PROP_BACKUP_FAILURE_TIMEOUT,
    PROP_BACKUP_PREPARATION_TIME,
    PROP_RESTORE_PREPARATION_TIME,
    PROP_RESTORE_COMPLETION_TIME,
    PROP_BACKUP_AND_RESTORE_STATE,
    PROP_ACTIVE_COV_SUBSCRIPTIONS,
    PROP_SLAVE_PROXY_ENABLE,
    PROP_MANUAL_SLAVE_ADDRESS_BINDING,
    PROP_AUTO_SLAVE_DISCOVERY,
    PROP_SLAVE_ADDRESS_BINDING,
    PROP_LAST_RESTART_REASON,
    PROP_TIME_OF_DEVICE_RESTART,
    PROP_RESTART_NOTIFICATION_RECIPIENTS,
    PROP_UTC_TIME_SYNCHRONIZATION_RECIPIENTS,
    PROP_TIME_SYNCHRONIZATION_INTERVAL,
    PROP_ALIGN_INTERVALS,
    PROP_INTERVAL_OFFSET,
    PROP_PROFILE_NAME,
    -1
};

static const int Accumulator_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_SCALE,
    PROP_UNITS,
    PROP_MAX_PRES_VALUE,
    -1
};

static const int Accumulator_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_DEVICE_TYPE,
    PROP_RELIABILITY,
    PROP_PRESCALE,
    PROP_VALUE_CHANGE_TIME,
    PROP_VALUE_BEFORE_CHANGE,
    PROP_VALUE_SET,
    PROP_LOGGING_RECORD,
    PROP_LOGGING_OBJECT,
    PROP_PULSE_RATE,
    PROP_HIGH_LIMIT,
    PROP_LOW_LIMIT,
    PROP_LIMIT_MONITORING_INTERVAL,
    PROP_NOTIFICATION_CLASS,
    PROP_TIME_DELAY,
    PROP_LIMIT_ENABLE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Analog_Input_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_UNITS,
    -1
};

static const int Analog_Input_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_DEVICE_TYPE,
    PROP_RELIABILITY,
    PROP_UPDATE_INTERVAL,
    PROP_MIN_PRES_VALUE,
    PROP_MAX_PRES_VALUE,
    PROP_RESOLUTION,
    PROP_COV_INCREMENT,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_HIGH_LIMIT,
    PROP_LOW_LIMIT,
    PROP_DEADBAND,
    PROP_LIMIT_ENABLE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Analog_Output_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_UNITS,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    -1
};

static const int Analog_Output_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_DEVICE_TYPE,
    PROP_RELIABILITY,
    PROP_MIN_PRES_VALUE,
    PROP_MAX_PRES_VALUE,
    PROP_RESOLUTION,
    PROP_COV_INCREMENT,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_HIGH_LIMIT,
    PROP_LOW_LIMIT,
    PROP_DEADBAND,
    PROP_LIMIT_ENABLE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Analog_Value_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_UNITS,
    -1
};

static const int Analog_Value_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_RELIABILITY,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    PROP_COV_INCREMENT,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_HIGH_LIMIT,
    PROP_LOW_LIMIT,
    PROP_DEADBAND,
    PROP_LIMIT_ENABLE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Averaging_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_MINIMUM_VALUE,
    PROP_AVERAGE_VALUE,
    PROP_MAXIMUM_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_UNITS,
    -1
};

static const int Averaging_Properties_Optional[] = {
    PROP_PROFILE_NAME,
    PROP_MINIMUM_VALUE_TIMESTAMP,
    PROP_VARIANCE_VALUE,
    PROP_MAXIMUM_VALUE_TIMESTAMP,
    PROP_DESCRIPTION,
    PROP_ATTEMPTED_SAMPLES,
    PROP_VALID_SAMPLES,
    PROP_OBJECT_PROPERTY_REFERENCE,
    PROP_WINDOW_INTERVAL,
    PROP_WINDOW_SAMPLES,
    -1
};

static const int Binary_Input_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_POLARITY,
    -1
};

static const int Binary_Input_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_DEVICE_TYPE,
    PROP_RELIABILITY,
    PROP_INACTIVE_TEXT,
    PROP_ACTIVE_TEXT,
    PROP_CHANGE_OF_STATE_TIME,
    PROP_CHANGE_OF_STATE_COUNT,
    PROP_TIME_OF_STATE_COUNT_RESET,
    PROP_ELAPSED_ACTIVE_TIME,
    PROP_TIME_OF_ACTIVE_TIME_RESET,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_ALARM_VALUE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Binary_Output_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_POLARITY,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    -1
};

static const int Binary_Output_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_DEVICE_TYPE,
    PROP_RELIABILITY,
    PROP_INACTIVE_TEXT,
    PROP_ACTIVE_TEXT,
    PROP_CHANGE_OF_STATE_TIME,
    PROP_CHANGE_OF_STATE_COUNT,
    PROP_TIME_OF_STATE_COUNT_RESET,
    PROP_ELAPSED_ACTIVE_TIME,
    PROP_TIME_OF_ACTIVE_TIME_RESET,
    PROP_MINIMUM_OFF_TIME,
    PROP_MINIMUM_ON_TIME,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_FEEDBACK_VALUE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Binary_Value_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    -1
};

static const int Binary_Value_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_RELIABILITY,
    PROP_INACTIVE_TEXT,
    PROP_ACTIVE_TEXT,
    PROP_CHANGE_OF_STATE_TIME,
    PROP_CHANGE_OF_STATE_COUNT,
    PROP_TIME_OF_STATE_COUNT_RESET,
    PROP_ELAPSED_ACTIVE_TIME,
    PROP_TIME_OF_ACTIVE_TIME_RESET,
    PROP_MINIMUM_OFF_TIME,
    PROP_MINIMUM_ON_TIME,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_ALARM_VALUE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Calendar_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_DATE_LIST,
    -1
};

static const int Calendar_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_PROFILE_NAME,
    -1
};

static const int Channel_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_LAST_PRIORITY,
    PROP_WRITE_STATUS,
    PROP_STATUS_FLAGS,
    PROP_OUT_OF_SERVICE,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
    PROP_CHANNEL_NUMBER,
    PROP_CONTROL_GROUPS,
    -1
};

static const int Channel_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_RELIABILITY,
    PROP_EXECUTION_DELAY,
    PROP_ALLOW_GROUP_DELAY_INHIBIT,
    PROP_PROFILE_NAME,
    -1
};

static const int Command_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_IN_PROCESS,
    PROP_ALL_WRITES_SUCCESSFUL,
    PROP_ACTION,
    -1
};

static const int Command_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_ACTION_TEXT,
    PROP_PROFILE_NAME,
    -1
};

static const int CharacterString_Value_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    -1
};

static const int CharacterString_Value_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_EVENT_STATE,
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_ALARM_VALUES,
    PROP_FAULT_VALUES,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Load_Control_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_REQUESTED_SHED_LEVEL,
    PROP_START_TIME,
    PROP_SHED_DURATION,
    PROP_DUTY_WINDOW,
    PROP_ENABLE,
    PROP_EXPECTED_SHED_LEVEL,
    PROP_ACTUAL_SHED_LEVEL,
    PROP_SHED_LEVELS,
    PROP_SHED_LEVEL_DESCRIPTIONS,
    -1
};

static const int Load_Control_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_STATE_DESCRIPTION,
    PROP_RELIABILITY,
    PROP_FULL_DUTY_BASELINE,
    PROP_NOTIFICATION_CLASS,
    PROP_TIME_DELAY,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Life_Safety_Point_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_TRACKING_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_RELIABILITY,
    PROP_MODE,
    PROP_ACCEPTED_MODES,
    PROP_SILENCED,
    PROP_OPERATION_EXPECTED,
    -1
};

static const int Life_Safety_Point_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_DEVICE_TYPE,
    PROP_NOTIFICATION_CLASS,
    PROP_LIFE_SAFETY_ALARM_VALUES,
    PROP_ALARM_VALUES,
    PROP_FAULT_VALUES,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_MAINTENANCE_REQUIRED,
    PROP_SETTING,
    PROP_DIRECT_READING,
    PROP_UNITS,
    PROP_MEMBER_OF,
    PROP_PROFILE_NAME,
    -1
};

static const int Multistate_Input_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_NUMBER_OF_STATES,
    -1
};

static const int Multistate_Input_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_DEVICE_TYPE,
    PROP_RELIABILITY,
    PROP_STATE_TEXT,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_ALARM_VALUES,
    PROP_FAULT_VALUES,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Multistate_Output_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_NUMBER_OF_STATES,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    -1
};

static const int Multistate_Output_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_DEVICE_TYPE,
    PROP_RELIABILITY,
    PROP_STATE_TEXT,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_FEEDBACK_VALUE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Multistate_Value_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_NUMBER_OF_STATES,
    -1
};

static const int Multistate_Value_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_RELIABILITY,
    PROP_STATE_TEXT,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_ALARM_VALUES,
    PROP_FAULT_VALUES,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    -1
};

static const int Notification_Class_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_NOTIFICATION_CLASS,
    PROP_PRIORITY,
    PROP_ACK_REQUIRED,
    PROP_RECIPIENT_LIST,
    -1
};

static const int Notification_Class_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_PROFILE_NAME,
    -1
};

static const int Trend_Log_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_ENABLE,
    PROP_STOP_WHEN_FULL,
    PROP_BUFFER_SIZE,
    PROP_LOG_BUFFER,
    PROP_RECORD_COUNT,
    PROP_TOTAL_RECORD_COUNT,
    PROP_EVENT_STATE,
    PROP_LOGGING_TYPE,
    PROP_STATUS_FLAGS,
    -1
};

static const int Trend_Log_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_START_TIME,
    PROP_STOP_TIME,
    PROP_LOG_DEVICE_OBJECT_PROPERTY,
    PROP_LOG_INTERVAL,
    PROP_COV_RESUBSCRIPTION_INTERVAL,
    PROP_CLIENT_COV_INCREMENT,
    PROP_NOTIFICATION_THRESHOLD,
    PROP_RECORDS_SINCE_NOTIFICATION,
    PROP_LAST_NOTIFY_RECORD,
    PROP_NOTIFICATION_CLASS,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_MESSAGE_TEXTS,
    PROP_PROFILE_NAME,
    PROP_ALIGN_INTERVALS,
    PROP_INTERVAL_OFFSET,
    PROP_TRIGGER,
    PROP_RELIABILITY,
    -1
};

static const int File_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_FILE_TYPE,
    PROP_FILE_SIZE,
    PROP_MODIFICATION_DATE,
    PROP_ARCHIVE,
    PROP_READ_ONLY,
    PROP_FILE_ACCESS_METHOD,
    -1
};

static const int File_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_RECORD_COUNT,
    PROP_PROFILE_NAME,
    -1
};

/**
 * Function that returns the number of BACnet object properties in a list
 *
 * @param pList - array of type 'int' that is a list of BACnet object
 * properties, terminated by a '-1' value.
 */
unsigned property_list_count(
    const int *pList)
{
    unsigned property_count = 0;

    if (pList) {
        while (*pList != -1) {
            property_count++;
            pList++;
        }
    }

    return property_count;
}

/**
 * Function that returns the list of Required or Optional properties
 * of known standard objects.
 *
 * @param object_type - enumerated BACNET_OBJECT_TYPE
 * @param pPropertyList - returns a pointer to two '-1' terminated arrays of
 * type 'int' that contain BACnet object properties for the given object
 * type.
 */
void property_list_special(
    BACNET_OBJECT_TYPE object_type,
    struct special_property_list_t *pPropertyList)
{
    if (pPropertyList == NULL) {
        return;
    }
    pPropertyList->Proprietary.pList = NULL;
    switch (object_type) {
        case OBJECT_DEVICE:
            pPropertyList->Required.pList = Device_Properties_Required;
            pPropertyList->Optional.pList = Device_Properties_Optional;
            break;
            break;
        case OBJECT_ACCUMULATOR:
            pPropertyList->Required.pList = Accumulator_Properties_Required;
            pPropertyList->Optional.pList = Accumulator_Properties_Optional;
            break;
        case OBJECT_ANALOG_INPUT:
            pPropertyList->Required.pList = Analog_Input_Properties_Required;
            pPropertyList->Optional.pList = Analog_Input_Properties_Optional;
            break;
        case OBJECT_ANALOG_OUTPUT:
            pPropertyList->Required.pList = Analog_Output_Properties_Required;
            pPropertyList->Optional.pList = Analog_Output_Properties_Optional;
            break;
        case OBJECT_ANALOG_VALUE:
            pPropertyList->Required.pList = Analog_Value_Properties_Required;
            pPropertyList->Optional.pList = Analog_Value_Properties_Optional;
            break;
        case OBJECT_AVERAGING:
            pPropertyList->Required.pList = Averaging_Properties_Required;
            pPropertyList->Optional.pList = Averaging_Properties_Optional;
            break;
        case OBJECT_BINARY_INPUT:
            pPropertyList->Required.pList = Binary_Input_Properties_Required;
            pPropertyList->Optional.pList = Binary_Input_Properties_Optional;
            break;
        case OBJECT_BINARY_OUTPUT:
            pPropertyList->Required.pList = Binary_Output_Properties_Required;
            pPropertyList->Optional.pList = Binary_Output_Properties_Optional;
            break;
        case OBJECT_BINARY_VALUE:
            pPropertyList->Required.pList = Binary_Value_Properties_Required;
            pPropertyList->Optional.pList = Binary_Value_Properties_Optional;
            break;
        case OBJECT_CALENDAR:
            pPropertyList->Required.pList = Calendar_Properties_Required;
            pPropertyList->Optional.pList = Calendar_Properties_Optional;
            break;
        case OBJECT_CHANNEL:
            pPropertyList->Required.pList = Channel_Properties_Required;
            pPropertyList->Optional.pList = Channel_Properties_Optional;
            break;
        case OBJECT_COMMAND:
            pPropertyList->Required.pList = Command_Properties_Required;
            pPropertyList->Optional.pList = Command_Properties_Optional;
            break;
        case OBJECT_CHARACTERSTRING_VALUE:
            pPropertyList->Required.pList =
                CharacterString_Value_Properties_Required;
            pPropertyList->Optional.pList =
                CharacterString_Value_Properties_Optional;
            break;
        case OBJECT_LOAD_CONTROL:
            pPropertyList->Required.pList = Load_Control_Properties_Required;
            pPropertyList->Optional.pList = Load_Control_Properties_Optional;
            break;
        case OBJECT_LIFE_SAFETY_POINT:
            pPropertyList->Required.pList =
                Life_Safety_Point_Properties_Required;
            pPropertyList->Optional.pList =
                Life_Safety_Point_Properties_Optional;
            break;
        case OBJECT_MULTI_STATE_INPUT:
            pPropertyList->Required.pList =
                Multistate_Input_Properties_Required;
            pPropertyList->Optional.pList =
                Multistate_Input_Properties_Optional;
            break;
        case OBJECT_MULTI_STATE_OUTPUT:
            pPropertyList->Required.pList =
                Multistate_Output_Properties_Required;
            pPropertyList->Optional.pList =
                Multistate_Output_Properties_Optional;
            break;
        case OBJECT_MULTI_STATE_VALUE:
            pPropertyList->Required.pList =
                Multistate_Value_Properties_Required;
            pPropertyList->Optional.pList =
                Multistate_Value_Properties_Optional;
            break;
        case OBJECT_NOTIFICATION_CLASS:
            pPropertyList->Required.pList =
                Notification_Class_Properties_Required;
            pPropertyList->Optional.pList =
                Notification_Class_Properties_Optional;
            break;
        case OBJECT_TRENDLOG:
            pPropertyList->Required.pList = Trend_Log_Properties_Required;
            pPropertyList->Optional.pList = Trend_Log_Properties_Optional;
            break;
        case OBJECT_FILE:
            pPropertyList->Required.pList = File_Properties_Required;
            pPropertyList->Optional.pList = File_Properties_Optional;
            break;
        case OBJECT_EVENT_ENROLLMENT:
        case OBJECT_GROUP:
        case OBJECT_LOOP:
        case OBJECT_PROGRAM:
        case OBJECT_SCHEDULE:
        case OBJECT_LIFE_SAFETY_ZONE:
        case OBJECT_PULSE_CONVERTER:
        case OBJECT_EVENT_LOG:
        case OBJECT_GLOBAL_GROUP:
        case OBJECT_TREND_LOG_MULTIPLE:
        case OBJECT_STRUCTURED_VIEW:
        case OBJECT_ACCESS_DOOR:
        case OBJECT_LIGHTING_OUTPUT:
        case OBJECT_ACCESS_CREDENTIAL:
        case OBJECT_ACCESS_POINT:
        case OBJECT_ACCESS_RIGHTS:
        case OBJECT_ACCESS_USER:
        case OBJECT_ACCESS_ZONE:
        case OBJECT_CREDENTIAL_DATA_INPUT:
        case OBJECT_NETWORK_SECURITY:
        case OBJECT_BITSTRING_VALUE:
        case OBJECT_DATE_PATTERN_VALUE:
        case OBJECT_DATE_VALUE:
        case OBJECT_DATETIME_PATTERN_VALUE:
        case OBJECT_DATETIME_VALUE:
        case OBJECT_INTEGER_VALUE:
        case OBJECT_LARGE_ANALOG_VALUE:
        case OBJECT_OCTETSTRING_VALUE:
        case OBJECT_POSITIVE_INTEGER_VALUE:
        case OBJECT_TIME_PATTERN_VALUE:
        case OBJECT_TIME_VALUE:
        case OBJECT_NOTIFICATION_FORWARDER:
        case OBJECT_ALERT_ENROLLMENT:
            pPropertyList->Required.pList = Default_Properties_Required;
            pPropertyList->Optional.pList = NULL;
            pPropertyList->Proprietary.pList = NULL;
            break;
        default:
            /* proprietary objects */
            pPropertyList->Required.pList = Default_Properties_Required;
            pPropertyList->Optional.pList = NULL;
            pPropertyList->Proprietary.pList = NULL;
            break;
    }
    /* Fetch the counts if available otherwise zero them */
    pPropertyList->Required.count =
        pPropertyList->Required.pList ==
        NULL ? 0 : property_list_count(pPropertyList->Required.pList);

    pPropertyList->Optional.count =
        pPropertyList->Optional.pList ==
        NULL ? 0 : property_list_count(pPropertyList->Optional.pList);

    pPropertyList->Proprietary.count = 0;

    return;
}

BACNET_PROPERTY_ID property_list_special_property(
    BACNET_OBJECT_TYPE object_type,
    BACNET_PROPERTY_ID special_property,
    unsigned index)
{
    int property = -1;  /* return value */
    unsigned required, optional, proprietary;
    struct special_property_list_t PropertyList = {{0}};

    property_list_special(object_type, &PropertyList);
    required = PropertyList.Required.count;
    optional = PropertyList.Optional.count;
    proprietary = PropertyList.Proprietary.count;
    if (special_property == PROP_ALL) {
        if (index < required) {
            if (PropertyList.Required.pList) {
                property = PropertyList.Required.pList[index];
            }
        } else if (index < (required + optional)) {
            if (PropertyList.Optional.pList) {
                index -= required;
                property = PropertyList.Optional.pList[index];
            }
        } else if (index < (required + optional + proprietary)) {
            if (PropertyList.Proprietary.pList) {
                index -= (required + optional);
                property = PropertyList.Proprietary.pList[index];
            }
        }
    } else if (special_property == PROP_REQUIRED) {
        if (index < required) {
            if (PropertyList.Required.pList) {
                property = PropertyList.Required.pList[index];
            }
        }
    } else if (special_property == PROP_OPTIONAL) {
        if (index < optional) {
            if (PropertyList.Optional.pList) {
                property = PropertyList.Optional.pList[index];
            }
        }
    }

    return (BACNET_PROPERTY_ID) property;
}

unsigned property_list_special_count(
    BACNET_OBJECT_TYPE object_type,
    BACNET_PROPERTY_ID special_property)
{
    unsigned count = 0; /* return value */
    struct special_property_list_t PropertyList = {{0}};

    property_list_special(object_type, &PropertyList);
    if (special_property == PROP_ALL) {
        count =
            PropertyList.Required.count + PropertyList.Optional.count +
            PropertyList.Proprietary.count;
    } else if (special_property == PROP_REQUIRED) {
        count = PropertyList.Required.count;
    } else if (special_property == PROP_OPTIONAL) {
        count = PropertyList.Optional.count;
    }

    return count;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testPropList(
    Test * pTest)
{
    unsigned i = 0, j = 0;
    unsigned count = 0;
    BACNET_PROPERTY_ID property = MAX_BACNET_PROPERTY_ID;
    unsigned object_id = 0, object_name = 0, object_type = 0;

    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        count = property_list_special_count((BACNET_OBJECT_TYPE)i, PROP_ALL);
        ct_test(pTest, count >= 3);
        object_id = 0;
        object_name = 0;
        object_type = 0;
        for (j = 0; j < count; j++) {
            property = property_list_special_property(
                (BACNET_OBJECT_TYPE)i,
                PROP_ALL,
                j);
            if (property == PROP_OBJECT_TYPE) {
                object_type++;
            }
            if (property == PROP_OBJECT_IDENTIFIER) {
                object_id++;
            }
            if (property == PROP_OBJECT_NAME) {
                object_name++;
            }
        }
        ct_test(pTest, object_type == 1);
        ct_test(pTest, object_id == 1);
        ct_test(pTest, object_name == 1);
    }
}

#ifdef TEST_PROPLIST
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Property List", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testPropList);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_PROPLIST */
#endif /* TEST */
