/**
 * @file
 * @brief BACnet property state encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author John Minack <minack@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_PROPERTY_STATES_H_
#define BACNET_PROPERTY_STATES_H_

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/timestamp.h"

typedef struct {
    BACNET_PROPERTY_STATES tag;
    union {
        bool booleanValue;
        BACNET_BINARY_PV binaryValue;
        BACNET_EVENT_TYPE eventType;
        BACNET_POLARITY polarity;
        BACNET_PROGRAM_REQUEST programChange;
        BACNET_PROGRAM_STATE programState;
        BACNET_PROGRAM_ERROR programError;
        BACNET_RELIABILITY reliability;
        BACNET_EVENT_STATE state;
        BACNET_DEVICE_STATUS systemStatus;
        BACNET_ENGINEERING_UNITS units;
        BACNET_UNSIGNED_INTEGER unsignedValue;
        BACNET_LIFE_SAFETY_MODE lifeSafetyMode;
        BACNET_LIFE_SAFETY_STATE lifeSafetyState;
        BACNET_RESTART_REASON restartReason;
        BACNET_DOOR_ALARM_STATE doorAlarmState;
        BACNET_ACTION action;
        BACNET_DOOR_SECURED_STATUS doorSecuredStatus;
        BACNET_DOOR_STATUS doorStatus;
        BACNET_DOOR_VALUE doorValue;
        BACNET_FILE_ACCESS_METHOD fileAccessMethod;
        BACNET_LOCK_STATUS lockStatus;
        BACNET_LIFE_SAFETY_OPERATION lifeSafetyOperation;
        BACNET_MAINTENANCE maintenance;
        BACNET_NODE_TYPE nodeType;
        BACNET_NOTIFY_TYPE notifyType;
        BACNET_SECURITY_LEVEL securityLevel;
        BACNET_SHED_STATE shedState;
        BACNET_SILENCED_STATE silencedState;
        BACNET_ACCESS_EVENT accessEvent;
        BACNET_ACCESS_ZONE_OCCUPANCY_STATE zoneOccupancyState;
        BACNET_ACCESS_CREDENTIAL_DISABLE_REASON accessCredDisableReason;
        BACNET_ACCESS_CREDENTIAL_DISABLE accessCredDisable;
        BACNET_AUTHENTICATION_STATUS authenticationStatus;
        BACNET_BACKUP_STATE backupState;
        BACNET_WRITE_STATUS writeStatus;
        BACNET_LIGHTING_IN_PROGRESS lightingInProgress;
        BACNET_LIGHTING_OPERATION lightingOperation;
        BACNET_LIGHTING_TRANSITION lightingTransition;
        int32_t integerValue;
        BACNET_BINARY_LIGHTING_PV binaryLightingValue;
        BACNET_TIMER_STATE timerState;
        BACNET_TIMER_TRANSITION timerTransition;
        BACNET_IP_MODE bacnetIPMode;
        BACNET_PORT_COMMAND networkPortCommand;
        BACNET_PORT_TYPE networkType;
        BACNET_PORT_QUALITY networkNumberQuality;
        BACNET_ESCALATOR_OPERATION_DIRECTION escalatorOperationDirection;
        BACNET_ESCALATOR_FAULT escalatorFault;
        BACNET_ESCALATOR_MODE escalatorMode;
        BACNET_LIFT_CAR_DIRECTION liftCarDirection;
        BACNET_LIFT_CAR_DOOR_COMMAND liftCarDoorCommand;
        BACNET_LIFT_CAR_DRIVE_STATUS liftCarDriveStatus;
        BACNET_LIFT_CAR_MODE liftCarMode;
        BACNET_LIFT_GROUP_MODE liftGroupMode;
        BACNET_LIFT_FAULT liftFault;
        BACNET_PROTOCOL_LEVEL protocolLevel;
        BACNET_AUDIT_LEVEL auditLevel;
        BACNET_AUDIT_OPERATION auditOperation;
        uint32_t extendedValue;
    } state;
} BACNET_PROPERTY_STATE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int bacapp_property_state_decode(
        const uint8_t *apdu,
        uint32_t apdu_size,
        BACNET_PROPERTY_STATE *value);

    BACNET_STACK_EXPORT
    int bacapp_decode_property_state(
        const uint8_t * apdu,
        BACNET_PROPERTY_STATE * value);

    BACNET_STACK_EXPORT
    int bacapp_decode_context_property_state(
        const uint8_t * apdu,
        uint8_t tag_number,
        BACNET_PROPERTY_STATE * value);

    BACNET_STACK_EXPORT
    int bacapp_encode_property_state(
        uint8_t * apdu,
        const BACNET_PROPERTY_STATE * value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
