/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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

#include "indtext.h"
#include "bacenum.h"

static const char *ASHRAE_Reserved_String = "Reserved for Use by ASHRAE";
static const char *Vendor_Proprietary_String = "Vendor Proprietary Value";

INDTEXT_DATA bacnet_confirmed_service_names[] = {
  { SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, "Acknowledge-Alarm" },
  { SERVICE_CONFIRMED_COV_NOTIFICATION, "COV-Notification" },
  { SERVICE_CONFIRMED_EVENT_NOTIFICATION, "Event-Notification" },
  { SERVICE_CONFIRMED_GET_ALARM_SUMMARY, "Get-Alarm-Summary" },
  { SERVICE_CONFIRMED_GET_ENROLLMENT_SUMMARY, "Get-Enrollment-Summary" },
  { SERVICE_CONFIRMED_SUBSCRIBE_COV, "Subscribe-COV" },
  { SERVICE_CONFIRMED_ATOMIC_READ_FILE, "Atomic-Read-File" },
  { SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, "Atomic-Write-File" },
  { SERVICE_CONFIRMED_ADD_LIST_ELEMENT, "Add-List-Element" },
  { SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT, "Remove-List-Element" },
  { SERVICE_CONFIRMED_CREATE_OBJECT, "Create-Object" },
  { SERVICE_CONFIRMED_DELETE_OBJECT, "Delete-Object" },
  { SERVICE_CONFIRMED_READ_PROPERTY, "Read-Property" },
  { SERVICE_CONFIRMED_READ_PROPERTY_CONDITIONAL, "Read-Property-Conditional" },
  { SERVICE_CONFIRMED_READ_PROPERTY_MULTIPLE, "Read-Property-Multiple" },
  { SERVICE_CONFIRMED_WRITE_PROPERTY, "Write-Property" },
  { SERVICE_CONFIRMED_WRITE_PROPERTY_MULTIPLE, "Write-Property-Multiple" },
  { SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL, "Device-Communication-Control" },
  { SERVICE_CONFIRMED_PRIVATE_TRANSFER, "Private-Transfer" },
  { SERVICE_CONFIRMED_TEXT_MESSAGE, "Text-Message" },
  { SERVICE_CONFIRMED_REINITIALIZE_DEVICE, "Reinitialize-Device" },
  { SERVICE_CONFIRMED_VT_OPEN, "VT-Open" },
  { SERVICE_CONFIRMED_VT_CLOSE, "VT-Close" },
  { SERVICE_CONFIRMED_VT_DATA, "VT-Data" },
  { SERVICE_CONFIRMED_AUTHENTICATE, "Authenticate" },
  { SERVICE_CONFIRMED_REQUEST_KEY, "Request-Key" },
  { SERVICE_CONFIRMED_READ_RANGE, "Read-Range" },
  { SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION, "Life-Safety_Operation" },
  { SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY, "Subscribe-COV-Property" },
  { SERVICE_CONFIRMED_GET_EVENT_INFORMATION, "Get-Event-Information" },
  { 0, NULL }
};

const char *bactext_confirmed_service_name(int index)
{
  return indtext_by_index_default(
    bacnet_confirmed_service_names,
    index,ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_unconfirmed_service_names[] = {
  { SERVICE_UNCONFIRMED_I_AM, "I-Am" },
  { SERVICE_UNCONFIRMED_I_HAVE, "I-Have" },
  { SERVICE_UNCONFIRMED_COV_NOTIFICATION, "COV-Notification" },
  { SERVICE_UNCONFIRMED_EVENT_NOTIFICATION, "Event-Notification" },
  { SERVICE_UNCONFIRMED_PRIVATE_TRANSFER, "Private-Transfer" },
  { SERVICE_UNCONFIRMED_TEXT_MESSAGE, "Text-Message" },
  { SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, "Time-Synchronization" },
  { SERVICE_UNCONFIRMED_WHO_HAS, "Who-Has" },
  { SERVICE_UNCONFIRMED_WHO_IS, "Who-Is" },
  { SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, "UTC-Time-Synchronization" },
  { 0, NULL }
};

const char *bactext_unconfirmed_service_name(int index)
{
  return indtext_by_index_default(
    bacnet_unconfirmed_service_names,
    index,ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_application_tag_names[] = {
  { BACNET_APPLICATION_TAG_NULL, "Null" },
  { BACNET_APPLICATION_TAG_BOOLEAN, "Boolean" },
  { BACNET_APPLICATION_TAG_UNSIGNED_INT, "Unsigned Int" },
  { BACNET_APPLICATION_TAG_SIGNED_INT, "Signed Int" },
  { BACNET_APPLICATION_TAG_REAL, "Real" },
  { BACNET_APPLICATION_TAG_DOUBLE, "Double" },
  { BACNET_APPLICATION_TAG_OCTET_STRING, "Octet String" },
  { BACNET_APPLICATION_TAG_CHARACTER_STRING, "Character String" },
  { BACNET_APPLICATION_TAG_BIT_STRING, "Bit String" },
  { BACNET_APPLICATION_TAG_ENUMERATED, "Enumerated" },
  { BACNET_APPLICATION_TAG_DATE, "Date" },
  { BACNET_APPLICATION_TAG_TIME, "Time" },
  { BACNET_APPLICATION_TAG_OBJECT_ID, "Object ID" },
  { BACNET_APPLICATION_TAG_RESERVED1, "Reserved 1" },
  { BACNET_APPLICATION_TAG_RESERVED2, "Reserved 2" },
  { BACNET_APPLICATION_TAG_RESERVED3, "Reserved 3" },
  { 0, NULL }
};

const char *bactext_application_tag_name(int index)
{
  return indtext_by_index_default(
    bacnet_application_tag_names,
    index,ASHRAE_Reserved_String);
}

INDTEXT_DATA bacnet_object_type_names[] = {
  { OBJECT_ANALOG_INPUT, "Analog Input" },
  { OBJECT_ANALOG_OUTPUT, "Analog Output" },
  { OBJECT_ANALOG_VALUE, "Analog Value" },
  { OBJECT_BINARY_INPUT, "Binary Input" },
  { OBJECT_BINARY_OUTPUT, "Binary Output" },
  { OBJECT_BINARY_VALUE, "Binary Value" },
  { OBJECT_CALENDAR, "Calendar" },
  { OBJECT_COMMAND, "Command" },
  { OBJECT_DEVICE, "Device" },
  { OBJECT_EVENT_ENROLLMENT, "Event Enrollment" },
  { OBJECT_FILE, "File" },
  { OBJECT_GROUP, "Group" },
  { OBJECT_LOOP, "Loop" },
  { OBJECT_MULTI_STATE_INPUT, "Multi-State Input" },
  { OBJECT_MULTI_STATE_OUTPUT, "Multi-State Output" },
  { OBJECT_NOTIFICATION_CLASS, "Notification Class" },
  { OBJECT_PROGRAM, "Program" },
  { OBJECT_SCHEDULE, "Schedule" },
  { OBJECT_AVERAGING, "Averaging" },
  { OBJECT_MULTI_STATE_VALUE, "Multi-State Value" },
  { OBJECT_TRENDLOG, "Trendlog" },
  { OBJECT_LIFE_SAFETY_POINT, "Life Safety Point" },
  { OBJECT_LIFE_SAFETY_ZONE, "Life Safety Zone" },
  { OBJECT_ACCUMULATOR, "Accumulator" },
  { OBJECT_PULSE_CONVERTER, "Pulse-Converter" },
  { 0, NULL }
/* Enumerated values 0-127 are reserved for definition by ASHRAE.
   Enumerated values 128-1023 may be used by others subject to
   the procedures and constraints described in Clause 23. */
};

const char *bactext_object_type_name(int index)
{
  return indtext_by_index_split_default(
    bacnet_object_type_names,
    index,
    128,
    ASHRAE_Reserved_String,
    Vendor_Proprietary_String);
}

INDTEXT_DATA bacnet_property_names[] = {
/* FIXME: use the enumerations from bacenum.h */
  { 0, "acked-transitions"},
  { 1, "ack-required"},
  { 2, "action"},
  { 3, "action-text"},
  { 4, "active-text"},
  { 5, "active-vt-sessions"},
  { 6, "alarm-value"},
  { 7, "alarm-values"},
  { 8, "all"},
  { 9, "all-writes-successful"},
  { 10, "apdu-segment-timeout"},
  { 11, "apdu-timeout"},
  { 12, "application-software-version"},
  { 13, "archive"},
  { 14, "bias"},
  { 15, "change-of-state-count"},
  { 16, "change-of-state-time"},
  { 17, "notification-class"},
  { 18, "(deleted in 135-2001)"},
  { 19, "controlled-variable-reference"},
  { 20, "controlled-variable-units"},
  { 21, "controlled-variable-value"},
  { 22, "COV-increment"},
  { 23, "datelist"},
  { 24, "daylight-savings-status"},
  { 25, "deadband"},
  { 26, "derivative-constant"},
  { 27, "derivative-constant-units"},
  { 28, "description"},
  { 29, "description-of-halt"},
  { 30, "device-address-binding"},
  { 31, "device-type"},
  { 32, "effective-period"},
  { 33, "elapsed-active-time"},
  { 34, "error-limit"},
  { 35, "event-enable"},
  { 36, "event-state"},
  { 37, "event-type"},
  { 38, "exception-schedule"},
  { 39, "fault-values"},
  { 40, "feedback-value"},
  { 41, "file-access-method"},
  { 42, "file-size"},
  { 43, "file-type"},
  { 44, "firmware-version"},
  { 45, "high-limit"},
  { 46, "inactive-text"},
  { 47, "in-process"},
  { 48, "instance-of"},
  { 49, "integral-constant"},
  { 50, "integral-constant-units"},
  { 51, "issue-confirmednotifications"},
  { 52, "limit-enable"},
  { 53, "list-of-group-members"},
  { 54, "list-of-object-property-references"},
  { 55, "list-of-session-keys"},
  { 56, "local-date"},
  { 57, "local-time"},
  { 58, "location"},
  { 59, "low-limit"},
  { 60, "manipulated-variable-reference"},
  { 61, "maximum-output"},
  { 62, "max-apdu-length-accepted"},
  { 63, "max-info-frames"},
  { 64, "max-master"},
  { 65, "max-pres-value"},
  { 66, "minimum-off-time"},
  { 67, "minimum-on-time"},
  { 68, "minimum-output"},
  { 69, "min-pres-value"},
  { 70, "model-name"},
  { 71, "modification-date"},
  { 72, "notify-type"},
  { 73, "number-of-APDU-retries"},
  { 74, "number-of-states"},
  { 75, "object-identifier"},
  { 76, "object-list"},
  { 77, "object-name"},
  { 78, "object-property-reference"},
  { 79, "object-type"},
  { 80, "optional"},
  { 81, "out-of-service"},
  { 82, "output-units"},
  { 83, "event-parameters"},
  { 84, "polarity"},
  { 85, "present-value"},
  { 86, "priority"},
  { 87, "priority-array"},
  { 88, "priority-for-writing"},
  { 89, "process-identifier"},
  { 90, "program-change"},
  { 91, "program-location"},
  { 92, "program-state"},
  { 93, "proportional-constant"},
  { 94, "proportional-constant-units"},
  { 95, "protocol-conformance-class"},
  { 96, "protocol-object-types-supported"},
  { 97, "protocol-services-supported"},
  { 98, "protocol-version"},
  { 99, "read-only"},
  { 100, "reason-for-halt"},
  { 101, "recipient"},
  { 102, "recipient-list"},
  { 103, "reliability"},
  { 104, "relinquish-default"},
  { 105, "required"},
  { 106, "resolution"},
  { 107, "segmentation-supported"},
  { 108, "setpoint"},
  { 109, "setpoint-reference"},
  { 110, "state-text"},
  { 111, "status-flags"},
  { 112, "system-status"},
  { 113, "time-delay"},
  { 114, "time-of-active-time-reset"},
  { 115, "time-of-state-count-reset"},
  { 116, "time-synchronization-recipients"},
  { 117, "units"},
  { 118, "update-interval"},
  { 119, "utc-offset"},
  { 120, "vendor-identifier"},
  { 121, "vendor-name"},
  { 122, "vt-classes-supported"},
  { 123, "weekly-schedule"},
  { 124, "attempted-samples"},
  { 125, "average-value"},
  { 126, "buffer-size"},
  { 127, "client-cov-increment"},
  { 128, "cov-resubscription-interval"},
  { 129, "current-notify-time"},
  { 130, "event-time-stamps"},
  { 131, "log-buffer"},
  { 132, "log-device-object-property"},
  { 133, "log-enable"},
  { 134, "log-interval"},
  { 135, "maximum-value"},
  { 136, "minimum-value"},
  { 137, "notification-threshold"},
  { 138, "previous-notify-time"},
  { 139, "protocol-revision"},
  { 140, "records-since-notification"},
  { 141, "record-count"},
  { 142, "start-time"},
  { 143, "stop-time"},
  { 144, "stop-when-full"},
  { 145, "total-record-count"},
  { 146, "valid-samples"},
  { 147, "window-interval"},
  { 148, "window-samples"},
  { 149, "maximum-value-timestamp"},
  { 150, "minimum-value-timestamp"},
  { 151, "variance-value"},
  { 152, "active-cov-subscriptions"},
  { 153, "backup-failure-timeout"},
  { 154, "configuration-files"},
  { 155, "database-revision"},
  { 156, "direct-reading"},
  { 157, "last-restore-time"},
  { 158, "maintenance-required"},
  { 159, "member-of"},
  { 160, "mode"},
  { 161, "operation-expected"},
  { 162, "setting"},
  { 163, "silenced"},
  { 164, "tracking-value"},
  { 165, "zone-members"},
  { 166, "life-safety-alarm-values"},
  { 167, "max-segments-accepted"},
  { 168, "profile-name"},
  { 0, NULL }
  /* Enumerated values 0-511 are reserved for definition by ASHRAE.
     Enumerated values 512-4194303 may be used by others subject to the
     procedures and constraints described in Clause 23. */
};

const char *bactext_property_name(int index)
{
  return indtext_by_index_split_default(
    bacnet_property_names,
    index,
    512,
    ASHRAE_Reserved_String,
    Vendor_Proprietary_String);
}

INDTEXT_DATA bacnet_engineering_unit_names[] = {
/* FIXME: add the first 144 names...*/
/* FIXME: use the enumerations from bacenum.h */
{145,"milliohms"},
{146,"megawatt-hours"},
{147,"kilo-btus"},
{148,"mega-btus"},
{149,"kilojoules-per-kilogram-dry-air"},
{150,"megajoules-per-kilogram-dry-air"},
{151,"kilojoules-per-degree-Kelvin"},
{152,"megajoules-per-degree-Kelvin"},
{153,"newton"},
{154,"grams-per-second"},
{155,"grams-per-minute"},
{156,"tons-per-hour"},
{157,"kilo-btus-per-hour"},
{158,"hundredths-seconds"},
{159,"milliseconds"},
{160,"newton-meters"},
{161,"millimeters-per-second"},
{162,"millimeters-per-minute"},
{163,"meters-per-minute"},
{164,"meters-per-hour"},
{165,"cubic-meters-per-minute"},
{166,"meters-per-second-per-second"},
{167,"amperes-per-meter"},
{168,"amperes-per-square-meter"},
{169,"ampere-square-meters"},
{170,"farads"},
{171,"henrys"},
{172,"ohm-meters"},
{173,"siemens"},
{174,"siemens-per-meter"},
{175,"teslas"},
{176,"volts-per-degree-Kelvin"},
{177,"volts-per-meter"},
{178,"webers"},
{179,"candelas"},
{180,"candelas-per-square-meter"},
{181,"degrees-Kelvin-per-hour"},
{182,"degrees-Kelvin-per-minute"},
{183,"joule-seconds"},
{184,"radians-per-second"},
{185,"square-meters-per-Newton"},
{186,"kilograms-per-cubic-meter"},
{187,"newton-seconds"},
{188,"newtons-per-meter"},
{189,"watts-per-meter-per-degree-Kelvin"},
{0,NULL}
/* Enumerated values 0-255 are reserved for definition by ASHRAE.
   Enumerated values 256-65535 may be used by others subject to
   the procedures and constraints described in Clause 23. */
 };

const char *bactext_engineering_unit_name(int index)
{
  return indtext_by_index_split_default(
    bacnet_engineering_unit_names,
    index,
    256,
    ASHRAE_Reserved_String,
    Vendor_Proprietary_String);
}

#if 0
/* FIXME: add the value */
/* FIXME: use the enumerations from bacenum.h */
INDTEXT_DATA bacnet_reject_reason_names[] = {
  "Other",
  "Buffer Overflow",
  "Inconsistent Parameters",
  "Invalid Parameter Data Type",
  "Invalid Tag",
  "Missing Required Parameter",
  "Parameter Out of Range",
  "Too Many Arguments",
  "Undefined Enumeration",
  "Unrecognized Service"
  {0,NULL}
};
#endif


#if 0
/* FIXME: add the value */
/* FIXME: use the enumerations from bacenum.h */
INDTEXT_DATA bacnet_abort_reason_name[] = {
    "Other",
    "Buffer Overflow",
    "Invalid APDU in this State",
    "Preempted by Higher Priority Task",
    "Segmentation Not Supported"
};
#endif




