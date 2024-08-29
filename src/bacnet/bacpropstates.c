/**
 * @file
 * @brief BACnet property state encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author John Minack <minack@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/timestamp.h"
#include "bacnet/bacpropstates.h"

/**
 * @brief Decodes BACnetPropertyState from bytes into a data structure
 *
 *  BACnetPropertyStates ::= CHOICE {
 *  -- This production represents the possible datatypes for properties that
 *  -- have discrete or enumerated values. The choice shall be consistent with
 *  --  the datatype of the property referenced in the Event Enrollment Object.
 *  boolean-value [0] BOOLEAN,
 *  binary-value [1] BACnetBinaryPV,
 *  ...
 *  extended-value [63] Unsigned32,
 *  -- example-one [256] BACnetExampleTypeOne,
 *  -- example-two [257] BACnetExampleTypeTwo,
 *  ...
 *  }
 *  -- Tag values greater than 254 are not encoded as ASN context tags.
 *  -- In these cases, the tag value is multiplied by 100000 and is added
 *  -- to the enumeration value and the sum is encoded using context tag 63,
 *  -- the extended-value choice.
 *  -- Tag values 0-63 are reserved for definition by ASHRAE.
 *  -- Tag values of 64-254 may be used by others to
 *  -- accommodate vendor specific properties that have discrete
 *  -- or enumerated values, subject to the constraints
 *  -- described in Clause 23.
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - decoded value, if decoded
 *
 * @return number of bytes decoded, or #BACNET_STATUS_ERROR (-1) if malformed
 */
int bacapp_property_state_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_PROPERTY_STATE *value)
{
    BACNET_TAG tag = { 0 };
    uint32_t enum_value = 0;
    int32_t integer_value = 0;
    int apdu_len = 0;
    int len = 0;

    len = bacnet_tag_decode(apdu, apdu_size, &tag);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (!tag.context) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (value) {
        value->tag = (BACNET_PROPERTY_STATES)tag.number;
    }
    if (tag.number == PROP_STATE_BOOLEAN_VALUE) {
        if (tag.len_value_type != 1) {
            return BACNET_STATUS_ERROR;
        }
        if (value) {
            value->state.booleanValue = decode_context_boolean(&apdu[apdu_len]);
            apdu_len++;
        }
    } else if (tag.number == PROP_STATE_INTEGER_VALUE) {
        len = bacnet_signed_decode(&apdu[apdu_len], apdu_size - apdu_len,
            tag.len_value_type, &integer_value);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (value) {
            value->state.integerValue = integer_value;
        }
    } else {
        len = bacnet_enumerated_decode(&apdu[apdu_len], apdu_size - apdu_len,
            tag.len_value_type, &enum_value);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (!value) {
            return apdu_len;
        }
        switch (value->tag) {
            case PROP_STATE_BINARY_VALUE:
                value->state.binaryValue = (BACNET_BINARY_PV)enum_value;
                break;
            case PROP_STATE_EVENT_TYPE:
                value->state.eventType = (BACNET_EVENT_TYPE)enum_value;
                break;
            case PROP_STATE_POLARITY:
                value->state.polarity = (BACNET_POLARITY)enum_value;
                break;
            case PROP_STATE_PROGRAM_CHANGE:
                value->state.programChange = (BACNET_PROGRAM_REQUEST)enum_value;
                break;
            case PROP_STATE_PROGRAM_STATE:
                value->state.programState = (BACNET_PROGRAM_STATE)enum_value;
                break;
            case PROP_STATE_REASON_FOR_HALT:
                value->state.programError = (BACNET_PROGRAM_ERROR)enum_value;
                break;
            case PROP_STATE_RELIABILITY:
                value->state.reliability = (BACNET_RELIABILITY)enum_value;
                break;
            case PROP_STATE_EVENT_STATE:
                value->state.state = (BACNET_EVENT_STATE)enum_value;
                break;
            case PROP_STATE_SYSTEM_STATUS:
                value->state.systemStatus = (BACNET_DEVICE_STATUS)enum_value;
                break;
            case PROP_STATE_UNITS:
                value->state.units = (BACNET_ENGINEERING_UNITS)enum_value;
                break;
            case PROP_STATE_UNSIGNED_VALUE:
                value->state.unsignedValue =
                    (BACNET_UNSIGNED_INTEGER)enum_value;
                break;
            case PROP_STATE_LIFE_SAFETY_MODE:
                value->state.lifeSafetyMode =
                    (BACNET_LIFE_SAFETY_MODE)enum_value;
                break;
            case PROP_STATE_LIFE_SAFETY_STATE:
                value->state.lifeSafetyState =
                    (BACNET_LIFE_SAFETY_STATE)enum_value;
                break;
            case PROP_STATE_RESTART_REASON:
                value->state.restartReason = (BACNET_RESTART_REASON)enum_value;
                break;
            case PROP_STATE_DOOR_ALARM_STATE:
                value->state.doorAlarmState =
                    (BACNET_DOOR_ALARM_STATE)enum_value;
                break;
            case PROP_STATE_ACTION:
                value->state.action = (BACNET_ACTION)enum_value;
                break;
            case PROP_STATE_DOOR_SECURED_STATUS:
                value->state.doorSecuredStatus =
                    (BACNET_DOOR_SECURED_STATUS)enum_value;
                break;
            case PROP_STATE_DOOR_STATUS:
                value->state.doorStatus = (BACNET_DOOR_STATUS)enum_value;
                break;
            case PROP_STATE_DOOR_VALUE:
                value->state.doorValue = (BACNET_DOOR_VALUE)enum_value;
                break;
            case PROP_STATE_FILE_ACCESS_METHOD:
                value->state.fileAccessMethod =
                    (BACNET_FILE_ACCESS_METHOD)enum_value;
                break;
            case PROP_STATE_LOCK_STATUS:
                value->state.lockStatus = (BACNET_LOCK_STATUS)enum_value;
                break;
            case PROP_STATE_LIFE_SAFETY_OPERATION:
                value->state.lifeSafetyOperation =
                    (BACNET_LIFE_SAFETY_OPERATION)enum_value;
                break;
            case PROP_STATE_MAINTENANCE:
                value->state.maintenance = (BACNET_MAINTENANCE)enum_value;
                break;
            case PROP_STATE_NODE_TYPE:
                value->state.nodeType = (BACNET_NODE_TYPE)enum_value;
                break;
            case PROP_STATE_NOTIFY_TYPE:
                value->state.notifyType = (BACNET_NOTIFY_TYPE)enum_value;
                break;
            case PROP_STATE_SECURITY_LEVEL:
                value->state.securityLevel = (BACNET_SECURITY_LEVEL)enum_value;
                break;
            case PROP_STATE_SHED_STATE:
                value->state.shedState = (BACNET_SHED_STATE)enum_value;
                break;
            case PROP_STATE_SILENCED_STATE:
                value->state.silencedState = (BACNET_SILENCED_STATE)enum_value;
                break;
            case PROP_STATE_ACCESS_EVENT:
                value->state.accessEvent = (BACNET_ACCESS_EVENT)enum_value;
                break;
            case PROP_STATE_ZONE_OCCUPANCY_STATE:
                value->state.zoneOccupancyState =
                    (BACNET_ACCESS_ZONE_OCCUPANCY_STATE)enum_value;
                break;
            case PROP_STATE_ACCESS_CRED_DISABLE_REASON:
                value->state.accessCredDisableReason =
                    (BACNET_ACCESS_CREDENTIAL_DISABLE_REASON)enum_value;
                break;
            case PROP_STATE_ACCESS_CRED_DISABLE:
                value->state.accessCredDisable =
                    (BACNET_ACCESS_CREDENTIAL_DISABLE)enum_value;
                break;
            case PROP_STATE_AUTHENTICATION_STATUS:
                value->state.authenticationStatus =
                    (BACNET_AUTHENTICATION_STATUS)enum_value;
                break;
            case PROP_STATE_BACKUP_STATE:
                value->state.backupState = (BACNET_BACKUP_STATE)enum_value;
                break;
            case PROP_STATE_WRITE_STATUS:
                value->state.writeStatus = (BACNET_WRITE_STATUS)enum_value;
                break;
            case PROP_STATE_LIGHTING_IN_PROGRESS:
                value->state.lightingInProgress =
                    (BACNET_LIGHTING_IN_PROGRESS)enum_value;
                break;
            case PROP_STATE_LIGHTING_OPERATION:
                value->state.lightingOperation =
                    (BACNET_LIGHTING_OPERATION)enum_value;
                break;
            case PROP_STATE_LIGHTING_TRANSITION:
                value->state.lightingTransition =
                    (BACNET_LIGHTING_TRANSITION)enum_value;
                break;
            case PROP_STATE_BINARY_LIGHTING_VALUE:
                value->state.binaryLightingValue =
                    (BACNET_BINARY_LIGHTING_PV)enum_value;
                break;
            case PROP_STATE_TIMER_STATE:
                value->state.timerState = (BACNET_TIMER_STATE)enum_value;
                break;
            case PROP_STATE_TIMER_TRANSITION:
                value->state.timerTransition =
                    (BACNET_TIMER_TRANSITION)enum_value;
                break;
            case PROP_STATE_BACNET_IP_MODE:
                value->state.bacnetIPMode = (BACNET_IP_MODE)enum_value;
                break;
            case PROP_STATE_NETWORK_PORT_COMMAND:
                value->state.networkPortCommand =
                    (BACNET_PORT_COMMAND)enum_value;
                break;
            case PROP_STATE_NETWORK_TYPE:
                value->state.networkType = (BACNET_PORT_TYPE)enum_value;
                break;
            case PROP_STATE_NETWORK_NUMBER_QUALITY:
                value->state.networkNumberQuality =
                    (BACNET_PORT_QUALITY)enum_value;
                break;
            case PROP_STATE_ESCALATOR_OPERATION_DIRECTION:
                value->state.escalatorOperationDirection =
                    (BACNET_ESCALATOR_OPERATION_DIRECTION)enum_value;
                break;
            case PROP_STATE_ESCALATOR_FAULT:
                value->state.escalatorFault =
                    (BACNET_ESCALATOR_FAULT)enum_value;
                break;
            case PROP_STATE_ESCALATOR_MODE:
                value->state.escalatorMode = (BACNET_ESCALATOR_MODE)enum_value;
                break;
            case PROP_STATE_LIFT_CAR_DIRECTION:
                value->state.liftCarDirection =
                    (BACNET_LIFT_CAR_DIRECTION)enum_value;
                break;
            case PROP_STATE_LIFT_CAR_DOOR_COMMAND:
                value->state.liftCarDoorCommand =
                    (BACNET_LIFT_CAR_DOOR_COMMAND)enum_value;
                break;
            case PROP_STATE_LIFT_CAR_DRIVE_STATUS:
                value->state.liftCarDriveStatus =
                    (BACNET_LIFT_CAR_DRIVE_STATUS)enum_value;
                break;
            case PROP_STATE_LIFT_CAR_MODE:
                value->state.liftCarMode = (BACNET_LIFT_CAR_MODE)enum_value;
                break;
            case PROP_STATE_LIFT_GROUP_MODE:
                value->state.liftGroupMode = (BACNET_LIFT_GROUP_MODE)enum_value;
                break;
            case PROP_STATE_LIFT_FAULT:
                value->state.liftFault = (BACNET_LIFT_FAULT)enum_value;
                break;
            case PROP_STATE_PROTOCOL_LEVEL:
                value->state.protocolLevel = (BACNET_PROTOCOL_LEVEL)enum_value;
                break;
            case PROP_STATE_AUDIT_LEVEL:
                value->state.auditLevel = (BACNET_AUDIT_LEVEL)enum_value;
                break;
            case PROP_STATE_AUDIT_OPERATION:
                value->state.auditOperation =
                    (BACNET_AUDIT_OPERATION)enum_value;
                break;
            case PROP_STATE_EXTENDED_VALUE:
                value->state.extendedValue = enum_value;
                break;
            default:
                break;
        }
    }

    return apdu_len;
}

int bacapp_decode_property_state(
    const uint8_t *apdu, BACNET_PROPERTY_STATE *value)
{
    return bacapp_property_state_decode(apdu, MAX_APDU, value);
}

int bacapp_decode_context_property_state(
    const uint8_t *apdu, uint8_t tag_number, BACNET_PROPERTY_STATE *value)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_length = bacapp_decode_property_state(&apdu[len], value);

        if (section_length == -1) {
            len = -1;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                len = -1;
            }
        }
    } else {
        len = -1;
    }
    return len;
}

/**
 * @brief Encode the BACnetPropertyState
 * @param apdu  Pointer to the buffer for encoding into, or NULL for length
 * @param value  Pointer to the value used for encoding
 * @return number of bytes encoded, or zero if unable to encode
 */
int bacapp_encode_property_state(
    uint8_t *apdu, const BACNET_PROPERTY_STATE *value)
{
    int len = 0; /* length of each encoding */

    if (value) {
        switch (value->tag) {
            case PROP_STATE_BOOLEAN_VALUE:
                len = encode_context_boolean(
                    apdu, value->tag, value->state.booleanValue);
                break;

            case PROP_STATE_BINARY_VALUE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.binaryValue);
                break;

            case PROP_STATE_EVENT_TYPE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.eventType);
                break;

            case PROP_STATE_POLARITY:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.polarity);
                break;

            case PROP_STATE_PROGRAM_CHANGE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.programChange);
                break;

            case PROP_STATE_PROGRAM_STATE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.programState);
                break;

            case PROP_STATE_REASON_FOR_HALT:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.programError);
                break;

            case PROP_STATE_RELIABILITY:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.reliability);
                break;

            case PROP_STATE_EVENT_STATE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.state);
                break;

            case PROP_STATE_SYSTEM_STATUS:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.systemStatus);
                break;

            case PROP_STATE_UNITS:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.units);
                break;

            case PROP_STATE_UNSIGNED_VALUE:
                len = encode_context_unsigned(
                    apdu, value->tag, value->state.unsignedValue);
                break;

            case PROP_STATE_LIFE_SAFETY_MODE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.lifeSafetyMode);
                break;

            case PROP_STATE_LIFE_SAFETY_STATE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.lifeSafetyState);
                break;
            case PROP_STATE_RESTART_REASON:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.restartReason);
                break;
            case PROP_STATE_DOOR_ALARM_STATE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.doorAlarmState);
                break;
            case PROP_STATE_ACTION:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.action);
                break;
            case PROP_STATE_DOOR_SECURED_STATUS:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.doorSecuredStatus);
                break;
            case PROP_STATE_DOOR_STATUS:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.doorStatus);
                break;
            case PROP_STATE_DOOR_VALUE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.doorValue);
                break;
            case PROP_STATE_FILE_ACCESS_METHOD:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.fileAccessMethod);
                break;
            case PROP_STATE_LOCK_STATUS:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.lockStatus);
                break;
            case PROP_STATE_LIFE_SAFETY_OPERATION:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.lifeSafetyOperation);
                break;
            case PROP_STATE_MAINTENANCE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.maintenance);
                break;
            case PROP_STATE_NODE_TYPE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.nodeType);
                break;
            case PROP_STATE_NOTIFY_TYPE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.notifyType);
                break;
            case PROP_STATE_SECURITY_LEVEL:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.securityLevel);
                break;
            case PROP_STATE_SHED_STATE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.shedState);
                break;
            case PROP_STATE_SILENCED_STATE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.silencedState);
                break;
            case PROP_STATE_ACCESS_EVENT:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.accessEvent);
                break;
            case PROP_STATE_ZONE_OCCUPANCY_STATE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.zoneOccupancyState);
                break;
            case PROP_STATE_ACCESS_CRED_DISABLE_REASON:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.accessCredDisableReason);
                break;
            case PROP_STATE_ACCESS_CRED_DISABLE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.accessCredDisable);
                break;
            case PROP_STATE_AUTHENTICATION_STATUS:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.authenticationStatus);
                break;
            case PROP_STATE_BACKUP_STATE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.backupState);
                break;
            case PROP_STATE_WRITE_STATUS:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.writeStatus);
                break;
            case PROP_STATE_LIGHTING_IN_PROGRESS:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.lightingInProgress);
                break;
            case PROP_STATE_LIGHTING_OPERATION:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.lightingOperation);
                break;
            case PROP_STATE_LIGHTING_TRANSITION:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.lightingTransition);
                break;
            case PROP_STATE_INTEGER_VALUE:
                len = encode_context_signed(
                    apdu, value->tag, value->state.integerValue);
                break;
            case PROP_STATE_BINARY_LIGHTING_VALUE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.binaryLightingValue);
                break;
            case PROP_STATE_TIMER_STATE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.timerState);
                break;
            case PROP_STATE_TIMER_TRANSITION:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.timerTransition);
                break;
            case PROP_STATE_BACNET_IP_MODE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.bacnetIPMode);
                break;
            case PROP_STATE_NETWORK_PORT_COMMAND:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.networkPortCommand);
                break;
            case PROP_STATE_NETWORK_TYPE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.networkType);
                break;
            case PROP_STATE_NETWORK_NUMBER_QUALITY:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.networkNumberQuality);
                break;
            case PROP_STATE_ESCALATOR_OPERATION_DIRECTION:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.escalatorOperationDirection);
                break;
            case PROP_STATE_ESCALATOR_FAULT:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.escalatorFault);
                break;
            case PROP_STATE_ESCALATOR_MODE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.escalatorMode);
                break;
            case PROP_STATE_LIFT_CAR_DIRECTION:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.liftCarDirection);
                break;
            case PROP_STATE_LIFT_CAR_DOOR_COMMAND:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.liftCarDoorCommand);
                break;
            case PROP_STATE_LIFT_CAR_DRIVE_STATUS:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.liftCarDriveStatus);
                break;
            case PROP_STATE_LIFT_CAR_MODE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.liftCarMode);
                break;
            case PROP_STATE_LIFT_GROUP_MODE:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.liftGroupMode);
                break;
            case PROP_STATE_LIFT_FAULT:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.liftFault);
                break;
            case PROP_STATE_PROTOCOL_LEVEL:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.protocolLevel);
                break;
            case PROP_STATE_AUDIT_LEVEL:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.auditLevel);
                break;
            case PROP_STATE_AUDIT_OPERATION:
                len = encode_context_enumerated(
                    apdu, value->tag, value->state.auditOperation);
                break;
            default:
                break;
        }
    }
    return len;
}
